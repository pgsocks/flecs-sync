#include "unimplemented.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

/* Keep every allocation aligned for native reflected values. */
union sync_allocation {
	max_align_t alignment;
	size_t size;
};

#ifndef SYNC_TESTING
bool sync_test_fail(unsigned fault)
{
	(void)fault;
	return false;
}
#endif

void *sync_alloc(sync_session_t *s, size_t size)
{
	if (size > SIZE_MAX - sizeof(union sync_allocation) ||
		size + sizeof(union sync_allocation) >
		s->limits.max_memory_bytes - s->memory) {
		s->alloc_error = SYNC_ERR_LIMIT;
		return NULL;
	}
	if (sync_test_fail(0)) {
		s->alloc_error = SYNC_ERR_NOMEM;
		return NULL;
	}
	union sync_allocation *p = calloc(1, sizeof(*p) + size);
	if (!p) {
		s->alloc_error = SYNC_ERR_NOMEM;
		return NULL;
	}
	p->size = sizeof(*p) + size;
	s->memory += p->size;
	return p + 1;
}

void sync_free(sync_session_t *s, void *ptr)
{
	if (!ptr)
		return;
	union sync_allocation *p = (union sync_allocation *)ptr - 1;
	s->memory -= p->size;
	free(p);
}

sync_result_t sync_reserve(sync_session_t *s, struct sync_buffer *b,
	size_t extra, size_t limit)
{
	if (b->size > limit || extra > limit - b->size)
		return SYNC_ERR_LIMIT;
	size_t need = b->size + extra;
	if (need <= b->capacity)
		return SYNC_OK;
	size_t cap = b->capacity ? b->capacity : 64;
	while (cap < need && cap <= limit / 2)
		cap *= 2;
	if (cap < need)
		cap = need;
	if (cap > limit)
		cap = limit;
	unsigned char *p = sync_alloc(s, cap);
	if (!p)
		return s->alloc_error;
	if (b->size)
		memcpy(p, b->data, b->size);
	sync_free(s, b->data);
	b->data = p;
	b->capacity = cap;
	return SYNC_OK;
}

sync_result_t sync_bytes(sync_session_t *s, struct sync_buffer *b,
	const void *data, size_t size, size_t limit)
{
	sync_result_t r = sync_reserve(s, b, size, limit);
	if (r != SYNC_OK)
		return r;
	if (size)
		memcpy(b->data + b->size, data, size);
	b->size += size;
	return SYNC_OK;
}

sync_result_t sync_number(sync_session_t *s, struct sync_buffer *b,
	uint64_t n, unsigned width, size_t limit)
{
	unsigned char bytes[8];
	for (unsigned i = 0; i < width; i++)
		bytes[i] = (unsigned char)(n >> (8 * i));
	return sync_bytes(s, b, bytes, width, limit);
}

const unsigned char *sync_read_bytes(struct sync_reader *r, size_t size)
{
	if (r->error || size > r->size - r->offset) {
		r->error = SYNC_ERR_PROTOCOL;
		return NULL;
	}
	const unsigned char *p = r->data + r->offset;
	r->offset += size;
	return p;
}

uint64_t sync_read_number(struct sync_reader *r, unsigned width)
{
	const unsigned char *p = sync_read_bytes(r, width);
	uint64_t n = 0;
	if (p)
		for (unsigned i = 0; i < width; i++)
			n |= (uint64_t)p[i] << (8 * i);
	return n;
}

void sync_buffer_fini(sync_session_t *s, struct sync_buffer *b)
{
	sync_free(s, b->data);
	memset(b, 0, sizeof(*b));
}

void sync_diagnostic(sync_error_t *e, sync_result_t code,
	ecs_entity_t peer, uint64_t tick, uint64_t sequence)
{
	memset(e, 0, sizeof(*e));
	e->code = code;
	e->peer = peer;
	e->tick = tick;
	e->sequence = sequence;
	snprintf(e->message, sizeof(e->message),
		"Replication failed (%d)", code);
}

sync_result_t sync_fault(sync_session_t *s, sync_result_t code)
{
	s->state = SYNC_FAULTED;
	sync_diagnostic(&s->fault, code, 0, s->assembly_tick,
		s->sequence);
	return code;
}

struct sync_peer *sync_find_peer(const sync_session_t *s, ecs_entity_t e)
{
	for (struct sync_peer *p = s->peers; p; p = p->next)
		if (p->entity == e)
			return p;
	return NULL;
}

struct sync_mapping *sync_find_map(const sync_session_t *s, uint64_t id)
{
	for (uint32_t i = 0; i < s->map_count; i++)
		if (s->maps[i].id == id &&
			ecs_is_alive(s->world, s->maps[i].entity))
			return &s->maps[i];
	return NULL;
}

void sync_queue_fini(sync_session_t *s, struct sync_frame **head,
	struct sync_frame **tail)
{
	while (*head) {
		struct sync_frame *next = (*head)->next;
		sync_free(s, *head);
		*head = next;
	}
	*tail = NULL;
}

void sync_maps_fini(sync_session_t *s, bool delete_entities)
{
	for (uint32_t i = 0; i < s->map_count; i++) {
		struct sync_mapping *m = &s->maps[i];
		if (ecs_is_alive(s->world, m->entity)) {
			if (delete_entities)
				ecs_delete(s->world, m->entity);
			else
				ecs_remove_id(s->world, m->entity,
					s->ids.network_id);
		}
		if (m->samples) {
			for (uint32_t t = 0; t < s->type_count; t++) {
				sync_buffer_fini(s, &m->samples[t].value);
				sync_buffer_fini(s, &m->samples[t].references);
			}
			sync_free(s, m->samples);
		}
	}
	sync_free(s, s->maps);
	s->maps = NULL;
	s->map_count = 0;
}

void sync_known_fini(sync_session_t *s, struct sync_known *known,
	uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
		sync_free(s, known[i].revisions);
	sync_free(s, known);
}

static sync_result_t shape_text(sync_session_t *s, struct sync_buffer *b,
	const char *text)
{
	size_t n = strlen(text);
	sync_result_t r = sync_number(s, b, n, 4,
		s->limits.max_schema_bytes);
	if (r)
		return r;
	return sync_bytes(s, b, text, n, s->limits.max_schema_bytes);
}

static sync_result_t shape(sync_session_t *s, ecs_entity_t type,
	struct sync_buffer *b, uint32_t depth)
{
	if (depth > s->limits.max_depth)
		return SYNC_ERR_LIMIT;
	const EcsComponent *c = ecs_get(s->world, type, EcsComponent);
	const EcsType *meta = ecs_get(s->world, type, EcsType);
	if (!c || !c->size)
		return sync_number(s, b, 0, 1, s->limits.max_schema_bytes);
	if (!meta || c->alignment > (ecs_size_t)_Alignof(max_align_t))
		return SYNC_ERR_UNSUPPORTED;
	const ecs_type_info_t *info = ecs_get_type_info(s->world, type);
	if (!info || (info->hooks.flags & (ECS_TYPE_HOOK_CTOR_ILLEGAL |
		ECS_TYPE_HOOK_DTOR_ILLEGAL | ECS_TYPE_HOOK_COPY_ILLEGAL)))
		return SYNC_ERR_UNSUPPORTED;
	sync_result_t r = sync_number(s, b, meta->kind + 1, 1,
		s->limits.max_schema_bytes);
	if (r)
		return r;
	if (meta->kind == EcsPrimitiveType) {
		const EcsPrimitive *p = ecs_get(s->world, type, EcsPrimitive);
		if (p->kind == EcsUPtr || p->kind == EcsIPtr ||
			p->kind == EcsId)
			return SYNC_ERR_UNSUPPORTED;
		return sync_number(s, b, p->kind, 1,
			s->limits.max_schema_bytes);
	}
	if (meta->kind == EcsStructType) {
		const EcsStruct *st = ecs_get(s->world, type, EcsStruct);
		int32_t count = ecs_vec_count(&st->members);
		ecs_member_t *m = ecs_vec_first(&st->members);
		r = sync_number(s, b, (uint32_t)count, 4,
			s->limits.max_schema_bytes);
		for (int32_t i = 0; !r && i < count; i++) {
			uint32_t n = m[i].count > 0 ? (uint32_t)m[i].count : 1;
			if (n > s->limits.max_elements)
				return SYNC_ERR_LIMIT;
			const ecs_type_info_t *ti =
				ecs_get_type_info(s->world, m[i].type);
			if (!ti || m[i].offset < 0 ||
				(uint64_t)m[i].offset + (uint64_t)n * ti->size >
				(uint64_t)c->size)
				return SYNC_ERR_SCHEMA;
			r = shape_text(s, b, m[i].name);
			if (!r)
				r = sync_number(s, b, n, 4,
					s->limits.max_schema_bytes);
			if (!r)
				r = shape(s, m[i].type, b, depth + 1);
		}
		return r;
	}
	if (meta->kind == EcsArrayType) {
		const EcsArray *a = ecs_get(s->world, type, EcsArray);
		if (a->count <= 0 || (uint32_t)a->count >
			s->limits.max_elements)
			return SYNC_ERR_LIMIT;
		r = sync_number(s, b, (uint32_t)a->count, 4,
			s->limits.max_schema_bytes);
		return r ? r : shape(s, a->type, b, depth + 1);
	}
	if (meta->kind == EcsEnumType || meta->kind == EcsBitmaskType) {
		ecs_entity_t underlying = ecs_id(ecs_u32_t);
		if (meta->kind == EcsEnumType)
			underlying = ecs_get(s->world, type,
				EcsEnum)->underlying_type;
		r = shape(s, underlying, b, depth + 1);
		const EcsConstants *cc = ecs_get(s->world, type, EcsConstants);
		int32_t n = cc ? ecs_vec_count(&cc->ordered_constants) : 0;
		ecs_enum_constant_t *v = cc
			? ecs_vec_first(&cc->ordered_constants) : NULL;
		if (!r)
			r = sync_number(s, b, (uint32_t)n, 4,
				s->limits.max_schema_bytes);
		/* Constants are keyed canonically by name, not registration ID. */
		const char *last = NULL;
		for (int32_t i = 0; !r && i < n; i++) {
			ecs_enum_constant_t *best = NULL;
			for (int32_t j = 0; j < n; j++)
				if ((!last || strcmp(v[j].name, last) > 0) &&
					(!best || strcmp(v[j].name, best->name) < 0))
					best = &v[j];
			if (!best)
				return SYNC_ERR_SCHEMA;
			r = shape_text(s, b, best->name);
			if (!r)
				r = sync_number(s, b, (uint64_t)best->value, 8,
					s->limits.max_schema_bytes);
			last = best->name;
		}
		return r;
	}
	return SYNC_ERR_UNSUPPORTED;
}

static int type_order(const void *a, const void *b)
{
	const struct sync_type *x = a, *y = b;
	return strcmp(x->name, y->name);
}

sync_result_t sync_schema(sync_session_t *s)
{
	ecs_iter_t it = ecs_each_id(s->world, s->ids.sync);
	uint32_t count = 0;
	while (ecs_each_next(&it))
		count += (uint32_t)it.count;
	if (count > s->limits.max_types)
		return SYNC_ERR_LIMIT;
	if (count) {
		s->types = sync_alloc(s, count * sizeof(*s->types));
		if (!s->types)
			return s->alloc_error;
	}
	it = ecs_each_id(s->world, s->ids.sync);
	sync_result_t result = SYNC_OK;
	while (ecs_each_next(&it)) {
		for (int32_t i = 0; i < it.count; i++) {
			struct sync_type *t = &s->types[s->type_count++];
			t->id = it.entities[i];
			char *path = ecs_get_path(s->world, t->id);
			t->name = sync_alloc(s, strlen(path) + 1);
			if (t->name)
				strcpy(t->name, path);
			ecs_os_free(path);
			if (!t->name) {
				result = s->alloc_error;
				break;
			}
			result = shape(s, t->id, &t->shape, 1);
			if (result)
				break;
		}
		if (result) {
			ecs_iter_fini(&it);
			return result;
		}
	}
	if (count)
		qsort(s->types, count, sizeof(*s->types), type_order);
	result = sync_number(s, &s->manifest, count, 4,
		s->limits.max_schema_bytes);
	for (uint32_t i = 0; !result && i < count; i++) {
		struct sync_type *t = &s->types[i];
		result = shape_text(s, &s->manifest, t->name);
		if (!result)
			result = sync_number(s, &s->manifest, t->shape.size, 4,
				s->limits.max_schema_bytes);
		if (!result)
			result = sync_bytes(s, &s->manifest, t->shape.data,
				t->shape.size, s->limits.max_schema_bytes);
		if (s->role == SYNC_ROLE_SERVER && !result) {
			t->query = ecs_query_init(s->world,
				&(ecs_query_desc_t){
					.terms = {{ .id = t->id, .inout = EcsIn,
						.src.id = EcsSelf }},
					.cache_kind = EcsQueryCacheAuto,
					.flags = EcsQueryDetectChanges
				});
			if (!t->query)
				result = SYNC_ERR_SCHEMA;
		}
	}
	return result;
}

/* The same recursive walk handles scalars, nested structs and arrays.
 * mode 0 encodes, mode 1 decodes, mode 2 resolves decoded references.
 */
static sync_result_t value_walk(sync_session_t *s, ecs_entity_t type,
	void *value, struct sync_buffer *b, struct sync_buffer *refs,
	struct sync_reader *r, const struct sync_mapping *maps,
	uint32_t count, unsigned mode)
{
	const EcsType *meta = ecs_get(s->world, type, EcsType);
	if (!meta)
		return SYNC_ERR_SCHEMA;
	if (meta->kind == EcsStructType) {
		const EcsStruct *st = ecs_get(s->world, type, EcsStruct);
		ecs_member_t *m = ecs_vec_first(&st->members);
		int32_t n = ecs_vec_count(&st->members);
		for (int32_t i = 0; i < n; i++) {
			int32_t elements = m[i].count > 0 ? m[i].count : 1;
			const ecs_type_info_t *ti =
				ecs_get_type_info(s->world, m[i].type);
			for (int32_t j = 0; j < elements; j++) {
				void *ptr = (char *)value + m[i].offset +
					(size_t)j * ti->size;
				sync_result_t result = value_walk(s, m[i].type,
					ptr, b, refs, r, maps, count, mode);
				if (result)
					return result;
			}
		}
		return SYNC_OK;
	}
	if (meta->kind == EcsArrayType) {
		const EcsArray *a = ecs_get(s->world, type, EcsArray);
		const ecs_type_info_t *ti = ecs_get_type_info(s->world, a->type);
		for (int32_t i = 0; i < a->count; i++) {
			sync_result_t result = value_walk(s, a->type,
				(char *)value + (size_t)i * ti->size, b, refs,
				r, maps, count, mode);
			if (result)
				return result;
		}
		return SYNC_OK;
	}
	if (meta->kind == EcsEnumType || meta->kind == EcsBitmaskType) {
		ecs_entity_t underlying = meta->kind == EcsEnumType
			? ecs_get(s->world, type, EcsEnum)->underlying_type
			: ecs_id(ecs_u32_t);
		return value_walk(s, underlying, value, b, refs, r,
			maps, count, mode);
	}
	if (meta->kind != EcsPrimitiveType)
		return SYNC_ERR_UNSUPPORTED;
	ecs_primitive_kind_t kind = ecs_get(s->world, type, EcsPrimitive)->kind;
	const ecs_type_info_t *ti = ecs_get_type_info(s->world, type);
	unsigned width = (unsigned)ti->size;
	if (mode == 2 || mode == 4 || mode == 5) {
		if (kind != EcsEntity)
			return SYNC_OK;
		uint64_t id;
		memcpy(&id, value, 8);
		if (!id)
			return SYNC_OK;
		for (uint32_t i = 0; i < count; i++) {
			if ((mode == 5 ? maps[i].entity : maps[i].id) == id) {
				if (mode == 2)
					memcpy(value, &maps[i].entity, 8);
				return SYNC_OK;
			}
		}
		return SYNC_ERR_REFERENCE;
	}
	if (mode == 3) {
		if (kind == EcsString) {
			char *text;
			memcpy(&text, value, sizeof(text));
			if (text)
				s->memory -= strlen(text) + 1;
		}
		return SYNC_OK;
	}
	if (kind == EcsString) {
		if (!mode) {
			const char *text;
			memcpy(&text, value, sizeof(text));
			size_t n = text ? strlen(text) : 0;
			if (n > s->limits.max_string_bytes)
				return SYNC_ERR_LIMIT;
			sync_result_t result = sync_number(s, b,
				text ? n : UINT32_MAX, 4,
				s->limits.max_update_bytes);
			if (result || !text)
				return result;
			return sync_bytes(s, b, text, n,
				s->limits.max_update_bytes);
		}
		uint64_t n = sync_read_number(r, 4);
		if (r->error)
			return r->error;
		if (n == UINT32_MAX)
			return SYNC_OK;
		if (n > s->limits.max_string_bytes)
			return SYNC_ERR_LIMIT;
		const unsigned char *text = sync_read_bytes(r, (size_t)n);
		if (!text || memchr(text, 0, (size_t)n))
			return SYNC_ERR_PROTOCOL;
		/* Native lifecycle hooks own this allocation after copy; account
		 * its temporary storage while the decode plan is alive.
		 */
		if (n >= INT32_MAX || n + 1 >
			s->limits.max_memory_bytes - s->memory)
			return SYNC_ERR_LIMIT;
		if (sync_test_fail(0))
			return SYNC_ERR_NOMEM;
		/* Use the native allocator so destructors receive ordinary
		 * strings and can observe their contents during cleanup.
		 */
		char *owned = ecs_os_malloc((ecs_size_t)n + 1);
		if (!owned)
			return SYNC_ERR_NOMEM;
		s->memory += (size_t)n + 1;
		memcpy(owned, text, (size_t)n);
		owned[n] = 0;
		memcpy(value, &owned, sizeof(owned));
		return SYNC_OK;
	}
	uint64_t n = 0;
	if (!mode) {
		/* Read each native width rather than relying on host byte order. */
		switch (width) {
		case 1: { uint8_t v; memcpy(&v, value, 1); n = v; break; }
		case 2: { uint16_t v; memcpy(&v, value, 2); n = v; break; }
		case 4: { uint32_t v; memcpy(&v, value, 4); n = v; break; }
		case 8: memcpy(&n, value, 8); break;
		default: return SYNC_ERR_UNSUPPORTED;
		}
		if (kind == EcsEntity && n) {
			uint64_t id = 0;
			for (uint32_t i = 0; i < s->map_count; i++)
				if (s->maps[i].entity == n)
					id = s->maps[i].id;
			/* A missing target must remain distinguishable from null. */
			if (!id)
				return SYNC_ERR_REFERENCE;
			n = id;
			sync_result_t result = sync_number(s, refs, n, 8,
				s->limits.max_update_bytes);
			if (result)
				return result;
		}
		return sync_number(s, b, n, width,
			s->limits.max_update_bytes);
	}
	n = sync_read_number(r, width);
	if (r->error)
		return r->error;
	if (kind == EcsBool && n > 1)
		return SYNC_ERR_PROTOCOL;
	switch (width) {
	case 1: { uint8_t v = (uint8_t)n; memcpy(value, &v, 1); break; }
	case 2: { uint16_t v = (uint16_t)n; memcpy(value, &v, 2); break; }
	case 4: { uint32_t v = (uint32_t)n; memcpy(value, &v, 4); break; }
	case 8: memcpy(value, &n, 8); break;
	default: return SYNC_ERR_UNSUPPORTED;
	}
	return SYNC_OK;
}

sync_result_t sync_encode(sync_session_t *s, ecs_entity_t type,
	const void *value, struct sync_buffer *b, struct sync_buffer *refs)
{
	return value_walk(s, type, (void *)value, b, refs, NULL,
		NULL, 0, 0);
}

sync_result_t sync_decode(sync_session_t *s, ecs_entity_t type,
	void *value, struct sync_reader *r)
{
	return value_walk(s, type, value, NULL, NULL, r, NULL, 0, 1);
}

sync_result_t sync_translate(sync_session_t *s, ecs_entity_t type,
	void *value, const struct sync_mapping *maps, uint32_t count)
{
	return value_walk(s, type, value, NULL, NULL, NULL, maps, count, 2);
}

void sync_value_fini(sync_session_t *s, ecs_entity_t type, void *value)
{
	value_walk(s, type, value, NULL, NULL, NULL, NULL, 0, 3);
	ecs_value_fini(s->world, type, value);
	sync_free(s, value);
}

sync_result_t sync_validate_refs(sync_session_t *s, ecs_entity_t type,
	void *value, const struct sync_mapping *maps, uint32_t count)
{
	return value_walk(s, type, value, NULL, NULL, NULL, maps, count, 4);
}

sync_result_t sync_validate_local_refs(sync_session_t *s, ecs_entity_t type,
	const void *value, const struct sync_mapping *maps, uint32_t count)
{
	return value_walk(s, type, (void *)value, NULL, NULL, NULL,
		maps, count, 5);
}
