#include "unimplemented.h"
#include <string.h>
#include <stdatomic.h>

static _Atomic uint64_t capability;

static uint64_t next_capability(void)
{
	uint64_t old = atomic_load_explicit(&capability, memory_order_relaxed);
	while (old != UINT64_MAX) {
		if (atomic_compare_exchange_weak_explicit(&capability,
			&old, old + 1, memory_order_relaxed, memory_order_relaxed))
			return old + 1;
	}
	return 0;
}

sync_result_t sync_peer_open(sync_session_t *s, ecs_entity_t *out_peer)
{
	/*
	 * Zero output; validate server state and peer/identity limits.
	 * Allocate fresh stream and peer atomically; start PENDING without
	 * sampling.
	 * Unwind failure; never recycle a previously issued stream ID.
	 */
	if (out_peer)
		*out_peer = 0;
	if (!s || !out_peer)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED)
		return SYNC_ERR_STATE;
	if (s->peer_count >= s->limits.max_peers ||
		s->stream_counter == UINT64_MAX)
		return SYNC_ERR_LIMIT;
	struct sync_peer *p = sync_alloc(s, sizeof(*p));
	if (!p)
		return s->alloc_error;
	p->entity = ecs_new(s->world);
	p->stream = ++s->stream_counter;
	p->state = SYNC_PEER_PENDING;
	/* Append so issuance order is also capture order. */
	struct sync_peer **tail = &s->peers;
	while (*tail)
		tail = &(*tail)->next;
	*tail = p;
	s->peer_count++;
	*out_peer = p->entity;
	return SYNC_OK;
}

sync_result_t sync_peer_close(sync_session_t *s, ecs_entity_t peer)
{
	/*
	 * Validate ownership before changes, including faulted peers.
	 * Discard queues; remove recipient pairs without deleting their owners.
	 * Delete peer local data and invalidate only its views/tokens.
	 */
	if (!s || !peer)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	struct sync_peer **link = &s->peers;
	while (*link && (*link)->entity != peer)
		link = &(*link)->next;
	if (!*link)
		return SYNC_ERR_NOT_FOUND;
	struct sync_peer *p = *link;
	*link = p->next;
	sync_queue_fini(s, &p->head, &p->tail);
	sync_known_fini(s, p->known, p->count);
	ecs_delete(s->world, p->entity);
	sync_free(s, p);
	s->peer_count--;
	return SYNC_OK;
}

static bool eligible(const sync_session_t *s, ecs_entity_t e)
{
	if (ecs_has_id(s->world, e, EcsPrefab) ||
		ecs_has_id(s->world, e, EcsDisabled) || sync_find_peer(s, e))
		return false;
	if (!ecs_owns_id(s->world, e, s->ids.replicate_to_all) &&
		!ecs_has_pair(s->world, e, s->ids.replicate_to, EcsWildcard))
		return false;
	for (uint32_t i = 0; i < s->type_count; i++)
		if (ecs_owns_id(s->world, e, s->types[i].id))
			return true;
	return false;
}

static bool visible(const sync_session_t *s, ecs_entity_t e,
	const struct sync_peer *p)
{
	return ecs_is_alive(s->world, e) && eligible(s, e) &&
		(ecs_owns_id(s->world, e, s->ids.replicate_to_all) ||
		 ecs_owns_id(s->world, e,
			ecs_pair(s->ids.replicate_to, p->entity)));
}

static void sample_fini(sync_session_t *s, struct sync_mapping *m)
{
	for (uint32_t t = 0; t < s->type_count; t++) {
		sync_buffer_fini(s, &m->samples[t].value);
		sync_buffer_fini(s, &m->samples[t].references);
	}
	sync_free(s, m->samples);
}

static sync_result_t collect(sync_session_t *s)
{
	/* Retire generation-qualified identities only when their entity dies. */
	for (uint32_t i = 0; i < s->map_count; ) {
		if (ecs_is_alive(s->world, s->maps[i].entity)) {
			i++;
			continue;
		}
		sample_fini(s, &s->maps[i]);
		memmove(&s->maps[i], &s->maps[i + 1],
			(s->map_count - i - 1) * sizeof(*s->maps));
		s->map_count--;
	}
	ecs_entities_t all = ecs_get_entities(s->world);
	for (int32_t i = 0; i < all.alive_count; i++) {
		ecs_entity_t e = all.ids[i];
		if (!eligible(s, e))
			continue;
		bool mapped = false;
		for (uint32_t j = 0; j < s->map_count; j++)
			if (s->maps[j].entity == e)
				mapped = true;
		if (mapped)
			continue;
		if (s->map_count >= s->limits.max_entities ||
			s->network_counter == UINT64_MAX)
			return SYNC_ERR_LIMIT;
		struct sync_mapping *maps = sync_alloc(s,
			((size_t)s->map_count + 1) * sizeof(*maps));
		if (!maps)
			return s->alloc_error;
		struct sync_sample *samples = sync_alloc(s,
			(size_t)s->type_count * sizeof(*samples));
		if (!samples) {
			sync_free(s, maps);
			return s->alloc_error;
		}
		if (s->map_count)
			memcpy(maps, s->maps, s->map_count * sizeof(*maps));
		sync_free(s, s->maps);
		s->maps = maps;
		maps[s->map_count++] = (struct sync_mapping){
			.entity = e, .id = ++s->network_counter,
			.samples = samples
		};
		sync_network_id_t id = { s->network_counter };
		ecs_set_id(s->world, e, s->ids.network_id, sizeof(id), &id);
		/* Adding components does not create/delete entity identifiers. */
	}
	/* Reconcile ownership before consuming each shared dirty query once. */
	for (uint32_t i = 0; i < s->map_count; i++) {
		for (uint32_t t = 0; t < s->type_count; t++) {
			struct sync_sample *v = &s->maps[i].samples[t];
			bool present = ecs_owns_id(s->world,
				s->maps[i].entity, s->types[t].id);
			v->dirty = present && !v->present;
			v->present = present;
			if (!present) {
				sync_buffer_fini(s, &v->value);
				sync_buffer_fini(s, &v->references);
				v->revision = 0;
				v->error = SYNC_OK;
			}
		}
	}
	for (uint32_t t = 0; t < s->type_count; t++) {
		ecs_iter_t it = ecs_query_iter(s->world, s->types[t].query);
		while (ecs_query_next(&it)) {
			bool dirty = ecs_iter_changed(&it);
			if (!dirty)
				continue;
			for (int32_t row = 0; row < it.count; row++)
				for (uint32_t i = 0; i < s->map_count; i++)
					if (s->maps[i].entity == it.entities[row])
						s->maps[i].samples[t].dirty = true;
		}
	}
	for (uint32_t i = 0; i < s->map_count; i++) {
		for (uint32_t t = 0; t < s->type_count; t++) {
			struct sync_sample *v = &s->maps[i].samples[t];
			if (!v->present || (!v->dirty && !v->error))
				continue;
			v->value.size = v->references.size = 0;
			const ecs_type_info_t *ti =
				ecs_get_type_info(s->world, s->types[t].id);
			if (ti && ti->size) {
				sync_result_t r = sync_encode(s, s->types[t].id,
					ecs_get_id(s->world, s->maps[i].entity,
						s->types[t].id),
					&v->value, &v->references);
				if (r == SYNC_ERR_NOMEM)
					return r;
				v->error = r;
			}
			v->revision = s->tick;
		}
	}
	return SYNC_OK;
}

static sync_result_t frame_append(sync_session_t *s, struct sync_peer *p,
	struct sync_frame **head, struct sync_frame **tail, uint64_t *sequence,
	sync_frame_kind_t kind, const void *payload, size_t size)
{
	if (*sequence == UINT64_MAX)
		return SYNC_ERR_LIMIT;
	uint64_t token = next_capability();
	if (!token)
		return SYNC_ERR_LIMIT;
	struct sync_frame *f = sync_alloc(s, sizeof(*f) + SYNC_HEADER + size);
	if (!f)
		return s->alloc_error;
	f->size = SYNC_HEADER + size;
	f->info = (sync_frame_info_t){
		.protocol_version = 1, .kind = kind,
		.session_id = s->identity, .stream_id = p->stream,
		.sequence = ++*sequence, .tick = s->tick,
		.payload_bytes = (uint32_t)size
	};
	f->token = (sync_outbox_token_t){ .opaque = {
		token, (uint64_t)(uintptr_t)s, p->stream, *sequence
	} };
	sync_write_header(f->data, &f->info);
	if (size)
		memcpy(f->data + SYNC_HEADER, payload, size);
	if (*tail)
		(*tail)->next = f;
	else
		*head = f;
	*tail = f;
	return SYNC_OK;
}

static struct sync_known *known_find(struct sync_peer *p, uint64_t id)
{
	for (uint32_t i = 0; i < p->count; i++)
		if (p->known[i].id == id)
			return &p->known[i];
	return NULL;
}

static bool known_contains(const struct sync_known *known, uint32_t count,
	uint64_t id)
{
	for (uint32_t i = 0; i < count; i++)
		if (known[i].id == id)
			return true;
	return false;
}

static sync_result_t retain(sync_session_t *s, struct sync_peer *p)
{
	uint32_t count = 0;
	for (uint32_t i = 0; i < s->map_count; i++)
		count += visible(s, s->maps[i].entity, p);
	struct sync_known *next = NULL;
	if (count) {
		next = sync_alloc(s, (size_t)count * sizeof(*next));
		if (!next)
			return s->alloc_error;
	}
	sync_result_t r = SYNC_OK;
	uint32_t n = 0;
	for (uint32_t i = 0; i < s->map_count; i++) {
		if (!visible(s, s->maps[i].entity, p))
			continue;
		next[n].id = s->maps[i].id;
		next[n].revisions = sync_alloc(s,
			(size_t)s->type_count * sizeof(uint64_t));
		if (!next[n++].revisions) {
			r = s->alloc_error;
			goto done;
		}
	}
	struct sync_buffer update = {0};
	r = sync_number(s, &update, count, 4, s->limits.max_update_bytes);
	n = 0;
	for (uint32_t i = 0; !r && i < s->map_count; i++) {
		struct sync_mapping *m = &s->maps[i];
		if (!visible(s, m->entity, p))
			continue;
		struct sync_known *old = known_find(p, m->id);
		r = sync_number(s, &update, m->id, 8,
			s->limits.max_update_bytes);
		for (uint32_t t = 0; !r && t < s->type_count; t++) {
			struct sync_sample *v = &m->samples[t];
			if (v->present && v->error) {
				r = v->error;
				break;
			}
			struct sync_reader refs = { .data = v->references.data,
				.size = v->references.size };
			while (v->present && refs.offset < refs.size) {
				uint64_t id = sync_read_number(&refs, 8);
				if (!known_contains(next, count, id)) {
					r = SYNC_ERR_REFERENCE;
					break;
				}
			}
			if (r)
				break;
			bool changed = !old ||
				old->revisions[t] != v->revision;
			unsigned flag = !v->present ? 0 : changed ? 2 : 1;
			r = sync_number(s, &update, flag, 1,
				s->limits.max_update_bytes);
			if (!r && flag == 2)
				r = sync_number(s, &update, v->value.size, 4,
					s->limits.max_update_bytes);
			if (!r && flag == 2)
				r = sync_bytes(s, &update, v->value.data,
					v->value.size, s->limits.max_update_bytes);
			next[n].revisions[t] = v->revision;
		}
		n++;
	}
	struct sync_frame *head = NULL, *tail = NULL;
	uint64_t sequence = p->sequence;
	bool snapshot = p->state == SYNC_PEER_PENDING;
	size_t capacity = s->limits.max_frame_bytes - SYNC_HEADER - 8;
	uint32_t chunks = (uint32_t)((update.size + capacity - 1) / capacity);
	if (!r && chunks > s->limits.max_fragments)
		r = SYNC_ERR_LIMIT;
	if (!r && snapshot)
		r = frame_append(s, p, &head, &tail, &sequence,
			SYNC_FRAME_MANIFEST, s->manifest.data, s->manifest.size);
	unsigned char prefix[8];
	for (unsigned i = 0; i < 4; i++) {
		prefix[i] = (unsigned char)(update.size >> (8 * i));
		prefix[4 + i] = (unsigned char)(chunks >> (8 * i));
	}
	if (!r && snapshot)
		r = frame_append(s, p, &head, &tail, &sequence,
			SYNC_FRAME_SNAPSHOT_BEGIN, prefix, sizeof(prefix));
	if (!r && !snapshot && update.size <=
		s->limits.max_frame_bytes - SYNC_HEADER) {
		r = frame_append(s, p, &head, &tail, &sequence,
			SYNC_FRAME_DELTA, update.data, update.size);
	} else {
		struct sync_buffer chunk = {0};
		for (uint32_t i = 0; !r && i < chunks; i++) {
			chunk.size = 0;
			size_t offset = (size_t)i * capacity;
			size_t size = update.size - offset;
			if (size > capacity)
				size = capacity;
			r = sync_number(s, &chunk, i, 4,
				s->limits.max_frame_bytes);
			if (!r)
				r = sync_number(s, &chunk, chunks, 4,
					s->limits.max_frame_bytes);
			if (!r)
				r = sync_bytes(s, &chunk, update.data + offset,
					size, s->limits.max_frame_bytes);
			if (!r)
				r = frame_append(s, p, &head, &tail, &sequence,
					snapshot ? SYNC_FRAME_SNAPSHOT_DATA
					: SYNC_FRAME_FRAGMENT, chunk.data, chunk.size);
		}
		sync_buffer_fini(s, &chunk);
	}
	if (!r && snapshot)
		r = frame_append(s, p, &head, &tail, &sequence,
			SYNC_FRAME_SNAPSHOT_END, NULL, 0);
	size_t bytes = 0;
	for (struct sync_frame *f = head; f; f = f->next)
		bytes += f->size;
	if (!r && bytes > s->limits.max_pending_bytes - p->queued)
		r = SYNC_ERR_LIMIT;
	if (!r) {
		if (p->tail)
			p->tail->next = head;
		else
			p->head = head;
		p->tail = tail;
		p->queued += bytes;
		p->sequence = sequence;
		p->tick = s->tick;
		p->state = SYNC_PEER_READY;
		sync_known_fini(s, p->known, p->count);
		p->known = next;
		p->count = count;
		next = NULL;
	} else {
		sync_queue_fini(s, &head, &tail);
	}
	sync_buffer_fini(s, &update);
done:
	sync_known_fini(s, next, next ? count : 0);
	return r;
}

sync_result_t sync_capture_core(sync_session_t *s, uint64_t tick,
	sync_capture_result_t *out)
{
	sync_capture_result_t progress = { .tick = tick };
	s->tick = tick;
	sync_result_t r = collect(s);
	if (r) {
		sync_fault(s, r);
		s->fault.tick = tick;
		for (struct sync_peer *p = s->peers; p; p = p->next) {
			if (p->state == SYNC_FAULTED)
				continue;
			p->state = SYNC_FAULTED;
			sync_diagnostic(&p->fault, r, p->entity, tick, p->sequence);
			progress.peers_faulted++;
		}
	} else {
		for (struct sync_peer *p = s->peers; p; p = p->next) {
			if (p->state == SYNC_FAULTED)
				continue;
			sync_result_t result = retain(s, p);
			if (result) {
				p->state = SYNC_FAULTED;
				sync_diagnostic(&p->fault, result, p->entity,
					tick, p->sequence);
				progress.peers_faulted++;
				r = SYNC_PARTIAL;
			} else {
				progress.peers_queued++;
			}
		}
	}
	if (out)
		*out = progress;
	return r;
}

sync_result_t sync_capture(sync_session_t *s, uint64_t tick,
	sync_capture_result_t *out_result)
{
	/*
	 * Validate manual state/tick before work; initialize progress output.
	 * Consume tick when sampling starts; assign IDs and collect tracked
	 * changes.
	 * Reconcile policy and reference closure separately for every healthy
	 * peer.
	 * Retain each complete update before advancing that peer baseline.
	 * Fault local overflow/reference failures independently; report
	 * PARTIAL.
	 * Global failure faults healthy peers; retain progress and diagnostics.
	 * Automatic wrapper shares this core after merge, at most once per
	 * progress.
	 */
	if (out_result)
		memset(out_result, 0, sizeof(*out_result));
	if (!s)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED || s->schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	if (!tick || tick <= s->tick)
		return SYNC_ERR_ARGUMENT;
	return sync_capture_core(s, tick, out_result);
}

sync_result_t sync_outbox_peek(const sync_session_t *s,
	ecs_entity_t peer, sync_frame_view_t *out_view)
{
	/*
	 * Zero output; validate server, peer and state; EMPTY without a head.
	 * Borrow immutable head bytes, metadata and capability without
	 * dequeuing.
	 * Preserve allocation across tail appends and peer fault transitions.
	 */
	if (out_view)
		memset(out_view, 0, sizeof(*out_view));
	if (!s || !out_view || !peer)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	struct sync_peer *p = sync_find_peer(s, peer);
	if (!p)
		return SYNC_ERR_NOT_FOUND;
	if (p->state == SYNC_FAULTED || s->state == SYNC_FAULTED)
		return SYNC_ERR_STATE;
	if (!p->head)
		return SYNC_EMPTY;
	*out_view = (sync_frame_view_t){ .data = p->head->data,
		.size = p->head->size, .token = p->head->token,
		.info = p->head->info };
	return SYNC_OK;
}

sync_result_t sync_outbox_commit(sync_session_t *s,
	ecs_entity_t peer, sync_outbox_token_t token)
{
	/*
	 * Validate peer and exact current capability before any changes.
	 * Reject stale/foreign tokens; free only the accepted head.
	 * Invalidate views/update accounting; leave capture baseline unchanged.
	 */
	if (!s || !peer)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	struct sync_peer *p = sync_find_peer(s, peer);
	if (!p)
		return SYNC_ERR_NOT_FOUND;
	if (p->state == SYNC_FAULTED || s->state == SYNC_FAULTED)
		return SYNC_ERR_STATE;
	if (!p->head || memcmp(&p->head->token, &token, sizeof(token)))
		return SYNC_ERR_ARGUMENT;
	struct sync_frame *f = p->head;
	p->head = f->next;
	if (!p->head)
		p->tail = NULL;
	p->queued -= f->size;
	sync_free(s, f);
	return SYNC_OK;
}
