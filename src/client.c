#include "unimplemented.h"
#include <string.h>

sync_result_t sync_receive(sync_session_t *s, const void *data, size_t size)
{
	/*
	 * Validate role/state/buffer; malformed envelopes fault the client.
	 * Check bounds before allocation; backpressure accepts zero bytes.
	 * Copy one frame then enqueue atomically; fault recoverable allocation
	 * loss.
	 * Do not decode logical values, mutate ECS or retain caller memory.
	 */
	if (!s || !data || !size)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_CLIENT)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED || s->state == SYNC_CLIENT_ENDED)
		return SYNC_ERR_STATE;
	if (size > s->limits.max_frame_bytes)
		return sync_fault(s, SYNC_ERR_LIMIT);
	sync_frame_info_t info;
	sync_result_t r = sync_frame_inspect(data, size, &info);
	if (r)
		return sync_fault(s, r);
	if (size > s->limits.max_receive_bytes - s->queued)
		return SYNC_BACKPRESSURE;
	struct sync_frame *f = sync_alloc(s, sizeof(*f) + size);
	if (!f)
		return sync_fault(s, s->alloc_error);
	f->size = size;
	f->info = info;
	memcpy(f->data, data, size);
	if (s->tail)
		s->tail->next = f;
	else
		s->head = f;
	s->tail = f;
	s->queued += size;
	return SYNC_OK;
}

static sync_result_t manifest(sync_session_t *s, struct sync_reader *r)
{
	uint32_t n = (uint32_t)sync_read_number(r, 4);
	if (r->error)
		return r->error;
	if (n > s->limits.max_types || r->size > s->limits.max_schema_bytes)
		return SYNC_ERR_LIMIT;
	uint32_t *types = n ? sync_alloc(s, (size_t)n * sizeof(*types)) : NULL;
	if (n && !types)
		return s->alloc_error;
	sync_result_t result = SYNC_OK;
	for (uint32_t i = 0; i < n; i++) {
		uint32_t length = (uint32_t)sync_read_number(r, 4);
		const unsigned char *name = sync_read_bytes(r, length);
		uint32_t size = (uint32_t)sync_read_number(r, 4);
		const unsigned char *shape = sync_read_bytes(r, size);
		if (r->error || !length || memchr(name, 0, length)) {
			result = SYNC_ERR_PROTOCOL;
			break;
		}
		bool matched = false;
		for (uint32_t t = 0; t < s->type_count; t++) {
			struct sync_type *local = &s->types[t];
			if (strlen(local->name) != length ||
				memcmp(name, local->name, length))
				continue;
			if (size != local->shape.size ||
				memcmp(shape, local->shape.data, size))
				break;
			types[i] = t;
			matched = true;
			break;
		}
		if (!matched) {
			result = SYNC_ERR_SCHEMA;
			break;
		}
		for (uint32_t j = 0; j < i; j++)
			if (types[j] == types[i])
				result = SYNC_ERR_PROTOCOL;
		if (result)
			break;
	}
	if (!result && r->offset != r->size)
		result = SYNC_ERR_PROTOCOL;
	if (result) {
		sync_free(s, types);
		return result;
	}
	s->wire_types = types;
	s->wire_count = n;
	s->state = SYNC_CLIENT_WAIT_SNAPSHOT;
	return SYNC_OK;
}

struct sync_value {
	void *data;
	unsigned flag;
};
struct sync_plan {
	struct sync_mapping *maps;
	struct sync_value *values;
	uint32_t count;
};

static void plan_fini(sync_session_t *s, struct sync_plan *p)
{
	if (p->values) {
		for (uint32_t i = 0; i < p->count; i++) {
			for (uint32_t t = 0; t < s->wire_count; t++) {
				struct sync_value *v =
					&p->values[(size_t)i * s->wire_count + t];
				if (v->data)
					sync_value_fini(s,
						s->types[s->wire_types[t]].id, v->data);
			}
		}
	}
	sync_free(s, p->values);
	sync_free(s, p->maps);
	memset(p, 0, sizeof(*p));
}

static sync_result_t decode_plan(sync_session_t *s, struct sync_reader *r,
	struct sync_plan *plan)
{
	uint32_t n = (uint32_t)sync_read_number(r, 4);
	if (r->error)
		return r->error;
	if (n > s->limits.max_entities)
		return SYNC_ERR_LIMIT;
	/* Each entity requires an ID and a flag for each manifest type. */
	if (n && (!s->wire_count || n >
		(r->size - r->offset) / (8 + (size_t)s->wire_count)))
		return SYNC_ERR_PROTOCOL;
	plan->count = n;
	if (n) {
		plan->maps = sync_alloc(s, (size_t)n * sizeof(*plan->maps));
		if (!plan->maps)
			return s->alloc_error;
		if (s->wire_count > SIZE_MAX / n / sizeof(*plan->values))
			return SYNC_ERR_LIMIT;
		plan->values = sync_alloc(s, (size_t)n * s->wire_count *
			sizeof(*plan->values));
		if (!plan->values)
			return s->alloc_error;
	}
	for (uint32_t i = 0; i < n; i++) {
		uint64_t id = sync_read_number(r, 8);
		if (!id || r->error)
			return SYNC_ERR_PROTOCOL;
		for (uint32_t j = 0; j < i; j++)
			if (plan->maps[j].id == id)
				return SYNC_ERR_PROTOCOL;
		struct sync_mapping *old = sync_find_map(s, id);
		plan->maps[i].id = id;
		plan->maps[i].entity = old ? old->entity : 0;
		bool present = false;
		for (uint32_t t = 0; t < s->wire_count; t++) {
			struct sync_value *v =
				&plan->values[(size_t)i * s->wire_count + t];
			v->flag = (unsigned)sync_read_number(r, 1);
			if (r->error || v->flag > 2)
				return SYNC_ERR_PROTOCOL;
			if (!v->flag)
				continue;
			present = true;
			ecs_entity_t type = s->types[s->wire_types[t]].id;
			if (v->flag == 1) {
				if (!old || !ecs_owns_id(s->world, old->entity, type))
					return SYNC_ERR_PROTOCOL;
				continue;
			}
			uint32_t size = (uint32_t)sync_read_number(r, 4);
			const unsigned char *bytes = sync_read_bytes(r, size);
			if (r->error)
				return r->error;
			const ecs_type_info_t *ti = ecs_get_type_info(s->world, type);
			if (!ti || !ti->size) {
				if (size)
					return SYNC_ERR_PROTOCOL;
				continue;
			}
			v->data = sync_alloc(s, (size_t)ti->size);
			if (!v->data)
				return s->alloc_error;
			if (ecs_value_init(s->world, type, v->data))
				return SYNC_ERR_RUNTIME;
			struct sync_reader value = { .data = bytes, .size = size };
			sync_result_t result = sync_decode(s, type, v->data, &value);
			if (result)
				return result;
			if (value.offset != value.size)
				return SYNC_ERR_PROTOCOL;
		}
		if (!present)
			return SYNC_ERR_PROTOCOL;
	}
	if (r->offset != r->size)
		return SYNC_ERR_PROTOCOL;
	for (uint32_t i = 0; i < n; i++) {
		for (uint32_t t = 0; t < s->wire_count; t++) {
			struct sync_value *v =
				&plan->values[(size_t)i * s->wire_count + t];
			void *value = v->data;
			if (!value) {
				if (v->flag != 1)
					continue;
				ecs_entity_t type = s->types[s->wire_types[t]].id;
				const ecs_type_info_t *ti =
					ecs_get_type_info(s->world, type);
				if (!ti || !ti->size)
					continue;
				sync_result_t result = sync_validate_local_refs(s,
					type, ecs_get_id(s->world,
						plan->maps[i].entity, type),
					plan->maps, n);
				if (result)
					return result;
				continue;
			}
			sync_result_t result = sync_validate_refs(s,
				s->types[s->wire_types[t]].id, value, plan->maps, n);
			if (result)
				return result;
		}
	}
	return SYNC_OK;
}

static bool plan_contains(struct sync_plan *p, uint64_t id)
{
	for (uint32_t i = 0; i < p->count; i++)
		if (p->maps[i].id == id)
			return true;
	return false;
}

static sync_result_t publish(sync_session_t *s, struct sync_plan *p)
{
	uint32_t new_count = 0;
	for (uint32_t i = 0; i < p->count; i++)
		new_count += !p->maps[i].entity;
	/* Reserve ownership before mutation so reset can always clean up a
	 * partially published introduction after a runtime failure.
	 */
	if (new_count) {
		struct sync_mapping *owned = sync_alloc(s,
			((size_t)s->map_count + new_count) * sizeof(*owned));
		if (!owned)
			return s->alloc_error;
		if (s->map_count)
			memcpy(owned, s->maps, s->map_count * sizeof(*owned));
		sync_free(s, s->maps);
		s->maps = owned;
	}
	for (uint32_t i = 0; i < p->count; i++) {
		if (p->maps[i].entity)
			continue;
		if (sync_test_fail(1))
			return SYNC_ERR_RUNTIME;
		p->maps[i].entity = ecs_new(s->world);
		s->maps[s->map_count++] = p->maps[i];
		if (sync_test_fail(1))
			return SYNC_ERR_RUNTIME;
		sync_network_id_t id = { p->maps[i].id };
		ecs_set_id(s->world, p->maps[i].entity,
			s->ids.network_id, sizeof(id), &id);
	}
	/* All references were checked before mutation; targets now exist. */
	for (uint32_t i = 0; i < p->count; i++) {
		for (uint32_t t = 0; t < s->wire_count; t++) {
			struct sync_value *v =
				&p->values[(size_t)i * s->wire_count + t];
			if (v->flag != 2)
				continue;
			ecs_entity_t type = s->types[s->wire_types[t]].id;
			if (v->data) {
				sync_result_t r = sync_translate(s, type,
					v->data, p->maps, p->count);
				if (r)
					return r;
			}
			if (sync_test_fail(1))
				return SYNC_ERR_RUNTIME;
			if (v->data) {
				const ecs_type_info_t *ti =
					ecs_get_type_info(s->world, type);
				ecs_set_id(s->world, p->maps[i].entity, type,
					(size_t)ti->size, v->data);
			} else {
				ecs_add_id(s->world, p->maps[i].entity, type);
			}
		}
	}
	/* Clear referring values before component/replica removals. */
	for (uint32_t i = 0; i < p->count; i++) {
		for (uint32_t t = 0; t < s->wire_count; t++) {
			if (p->values[(size_t)i * s->wire_count + t].flag)
				continue;
			ecs_entity_t type = s->types[s->wire_types[t]].id;
			if (!ecs_owns_id(s->world, p->maps[i].entity, type))
				continue;
			if (sync_test_fail(1))
				return SYNC_ERR_RUNTIME;
			ecs_remove_id(s->world, p->maps[i].entity, type);
		}
	}
	for (uint32_t i = 0; i < s->map_count; i++) {
		if (plan_contains(p, s->maps[i].id))
			continue;
		if (sync_test_fail(1))
			return SYNC_ERR_RUNTIME;
		ecs_delete(s->world, s->maps[i].entity);
	}
	sync_free(s, s->maps);
	s->maps = p->maps;
	s->map_count = p->count;
	p->maps = NULL;
	return SYNC_OK;
}

static sync_result_t complete(sync_session_t *s, struct sync_reader *r)
{
	struct sync_plan plan = {0};
	sync_result_t result = decode_plan(s, r, &plan);
	if (!result)
		result = publish(s, &plan);
	plan_fini(s, &plan);
	return result;
}

static sync_result_t process(sync_session_t *s, struct sync_frame *f,
	bool *published)
{
	sync_frame_info_t *info = &f->info;
	struct sync_reader r = { .data = f->data + SYNC_HEADER,
		.size = info->payload_bytes };
	if (s->sequence == UINT64_MAX || info->sequence != s->sequence + 1)
		return SYNC_ERR_SEQUENCE;
	if (s->state == SYNC_CLIENT_WAIT_MANIFEST) {
		if (info->kind != SYNC_FRAME_MANIFEST)
			return SYNC_ERR_PROTOCOL;
		sync_result_t result = manifest(s, &r);
		if (result)
			return result;
		s->identity = info->session_id;
		s->stream = info->stream_id;
		s->assembly_tick = info->tick;
		s->sequence = info->sequence;
		return SYNC_OK;
	}
	if (info->stream_id != s->stream ||
		memcmp(info->session_id.bytes, s->identity.bytes, 16))
		return SYNC_ERR_SEQUENCE;
	s->sequence = info->sequence;
	bool initial = s->state == SYNC_CLIENT_WAIT_SNAPSHOT;
	if (initial || s->snapshot || s->assembly_fragments) {
		if (info->tick != s->assembly_tick)
			return SYNC_ERR_SEQUENCE;
	} else if (info->tick <= s->tick) {
		return SYNC_ERR_SEQUENCE;
	}
	if (info->kind == SYNC_FRAME_SNAPSHOT_BEGIN) {
		if (!initial || s->snapshot || r.size != 8)
			return SYNC_ERR_PROTOCOL;
		s->expected_bytes = (size_t)sync_read_number(&r, 4);
		s->expected_fragments = (uint32_t)sync_read_number(&r, 4);
		if (!s->expected_fragments ||
			s->expected_fragments > s->limits.max_fragments ||
			s->expected_bytes > s->limits.max_update_bytes)
			return SYNC_ERR_LIMIT;
		s->snapshot = true;
		return SYNC_OK;
	}
	if (info->kind == SYNC_FRAME_DELTA) {
		if (initial || s->snapshot || s->assembly_fragments)
			return SYNC_ERR_PROTOCOL;
		if (r.size > s->limits.max_update_bytes)
			return SYNC_ERR_LIMIT;
		s->assembly_tick = info->tick;
		sync_result_t result = complete(s, &r);
		if (!result)
			*published = true;
		return result;
	}
	if (info->kind == SYNC_FRAME_SNAPSHOT_END) {
		if (!s->snapshot || r.size ||
			s->assembly_fragments != s->expected_fragments ||
			s->assembly.size != s->expected_bytes)
			return SYNC_ERR_PROTOCOL;
		struct sync_reader update = { .data = s->assembly.data,
			.size = s->assembly.size };
		sync_result_t result = complete(s, &update);
		if (!result)
			*published = true;
		return result;
	}
	if (info->kind != SYNC_FRAME_FRAGMENT &&
		info->kind != SYNC_FRAME_SNAPSHOT_DATA)
		return SYNC_ERR_PROTOCOL;
	if ((info->kind == SYNC_FRAME_SNAPSHOT_DATA && !s->snapshot) ||
		(info->kind == SYNC_FRAME_FRAGMENT && (initial || s->snapshot)))
		return SYNC_ERR_PROTOCOL;
	uint32_t index = (uint32_t)sync_read_number(&r, 4);
	uint32_t count = (uint32_t)sync_read_number(&r, 4);
	if (r.error || !count || index >= count ||
		index != s->assembly_fragments)
		return SYNC_ERR_PROTOCOL;
	if (count > s->limits.max_fragments)
		return SYNC_ERR_LIMIT;
	if (!s->snapshot && !index) {
		s->assembly_tick = info->tick;
		s->expected_fragments = count;
	}
	if (count != s->expected_fragments)
		return SYNC_ERR_PROTOCOL;
	sync_result_t result = sync_bytes(s, &s->assembly,
		r.data + r.offset, r.size - r.offset, s->limits.max_update_bytes);
	if (result)
		return result;
	s->assembly_fragments++;
	if (s->snapshot || s->assembly_fragments < count)
		return SYNC_OK;
	struct sync_reader update = { .data = s->assembly.data,
		.size = s->assembly.size };
	result = complete(s, &update);
	if (!result)
		*published = true;
	return result;
}

sync_result_t sync_apply_core(sync_session_t *s, uint32_t budget,
	sync_apply_result_t *out)
{
	sync_apply_result_t progress = {0};
	sync_result_t result = SYNC_OK;
	while (s->head && progress.updates_applied < budget) {
		struct sync_frame *f = s->head;
		s->head = f->next;
		if (!s->head)
			s->tail = NULL;
		s->queued -= f->size;
		progress.frames_consumed++;
		bool published = false;
		result = process(s, f, &published);
		sync_free(s, f);
		if (result) {
			sync_fault(s, result);
			break;
		}
		if (published) {
			s->tick = s->assembly_tick;
			s->state = SYNC_CLIENT_READY;
			progress.updates_applied++;
			progress.last_tick = s->tick;
			s->assembly.size = 0;
			s->assembly_fragments = s->expected_fragments = 0;
			s->snapshot = false;
		}
	}
	if (out)
		*out = progress;
	if (result)
		return result;
	if (progress.updates_applied)
		return SYNC_OK;
	return progress.frames_consumed || s->snapshot || s->assembly_fragments
		? SYNC_NEED_MORE : SYNC_EMPTY;
}

sync_result_t sync_apply(sync_session_t *s, uint32_t max_updates,
	sync_apply_result_t *out_result)
{
	/*
	 * Validate manual state/positive budget; initialize progress output.
	 * Drain input to bounded assembly; validate schema, sequences and
	 * ticks.
	 * Decode whole updates and clean temporary owned values on every exit.
	 * Allocate introductions before references; replace values before
	 * removals.
	 * Honor private publication failpoints at test-build mutation
	 * boundaries.
	 * Count completed updates; preserve earlier publications on later
	 * failure.
	 * Fault runtime failure without rollback; scheduled wrapper shares
	 * core.
	 */
	if (out_result)
		memset(out_result, 0, sizeof(*out_result));
	if (!s || !max_updates)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_CLIENT)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED || s->schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	if (s->state == SYNC_CLIENT_ENDED)
		return SYNC_END;
	return sync_apply_core(s, max_updates, out_result);
}

sync_result_t sync_client_finish(sync_session_t *s)
{
	/*
	 * Validate state; queued frames return unchanged BACKPRESSURE.
	 * Fault missing/incomplete initial or logical state as TRUNCATED.
	 * Enter ENDED idempotently at clean EOF and retain final replicas.
	 */
	if (!s)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_CLIENT)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED)
		return SYNC_ERR_STATE;
	if (s->head)
		return SYNC_BACKPRESSURE;
	if (s->state == SYNC_CLIENT_ENDED)
		return SYNC_OK;
	if (s->state != SYNC_CLIENT_READY || s->snapshot ||
		s->assembly_fragments)
		return sync_fault(s, SYNC_ERR_TRUNCATED);
	s->state = SYNC_CLIENT_ENDED;
	return SYNC_OK;
}

sync_result_t sync_client_reset(sync_session_t *s)
{
	/*
	 * Validate client; delete owned replicas and their local-only values.
	 * Clear mappings/queues/assembly/faults; retain schema and schedule.
	 * Return to WAIT_MANIFEST; preserve unrelated entities; allow repeat
	 * reset.
	 */
	if (!s)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_CLIENT)
		return SYNC_ERR_ROLE;
	sync_maps_fini(s, true);
	sync_queue_fini(s, &s->head, &s->tail);
	sync_buffer_fini(s, &s->assembly);
	sync_free(s, s->wire_types);
	s->wire_types = NULL;
	s->wire_count = 0;
	s->queued = 0;
	s->tick = s->sequence = s->stream = s->assembly_tick = 0;
	s->snapshot = false;
	s->assembly_fragments = s->expected_fragments = 0;
	s->expected_bytes = 0;
	memset(&s->fault, 0, sizeof(s->fault));
	memset(&s->identity, 0, sizeof(s->identity));
	s->state = SYNC_CLIENT_WAIT_MANIFEST;
	return SYNC_OK;
}
