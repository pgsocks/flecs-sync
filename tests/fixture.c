#include "test.h"

int live_records;

static void record_ctor(void *ptr, int32_t count, const ecs_type_info_t *info)
{
	(void)info;
	memset(ptr, 0, sizeof(struct record) * (size_t)count);
	live_records += count;
}

static void record_dtor(void *ptr, int32_t count, const ecs_type_info_t *info)
{
	(void)info;
	struct record *r = ptr;
	for (int32_t i = 0; i < count; i++)
		ecs_os_free(r[i].text);
	live_records -= count;
}

static void record_copy(void *dst, const void *src, int32_t count,
	const ecs_type_info_t *info)
{
	(void)info;
	struct record *d = dst;
	const struct record *s = src;
	for (int32_t i = 0; i < count; i++) {
		ecs_os_free(d[i].text);
		d[i] = s[i];
		d[i].text = s[i].text ? ecs_os_strdup(s[i].text) : NULL;
	}
}

static void record_move(void *dst, void *src, int32_t count,
	const ecs_type_info_t *info)
{
	(void)info;
	struct record *d = dst, *s = src;
	for (int32_t i = 0; i < count; i++) {
		ecs_os_free(d[i].text);
		d[i] = s[i];
		s[i].text = NULL;
	}
}

ecs_entity_t reflected(struct world *w, const char *name, size_t size,
	size_t alignment, const ecs_member_t *members, size_t count)
{
	ecs_entity_t e = ecs_entity_init(w->ecs,
		&(ecs_entity_desc_t){ .name = name });
	ecs_component_desc_t cd = { .entity = e };
	cd.type.size = (ecs_size_t)size;
	cd.type.alignment = (ecs_size_t)alignment;
	CHECK(ecs_component_init(w->ecs, &cd) == e);
	ecs_struct_desc_t sd = { .entity = e };
	CHECK(count <= sizeof(sd.members) / sizeof(sd.members[0]));
	memcpy(sd.members, members, count * sizeof(*members));
	CHECK(ecs_struct_init(w->ecs, &sd) == e);
	ecs_add_id(w->ecs, e, w->ids.sync);
	return e;
}

static ecs_entity_t position_type(struct world *w, const char *name)
{
	ecs_member_t members[] = {
		{ .name = "x", .type = ecs_id(ecs_f32_t) },
		{ .name = "y", .type = ecs_id(ecs_f32_t) }
	};
	return reflected(w, name, sizeof(struct position),
		_Alignof(struct position), members, 2);
}

static void register_types(struct world *w, bool reverse)
{
	if (reverse) {
		w->velocity = position_type(w, "Velocity");
		w->position = position_type(w, "Position");
	} else {
		w->position = position_type(w, "Position");
		w->velocity = position_type(w, "Velocity");
	}
	ecs_add_pair(w->ecs, w->position, EcsOnInstantiate, EcsInherit);
	ecs_member_t health[] = {
		{ .name = "value", .type = ecs_id(ecs_i32_t) }
	};
	w->health = reflected(w, "Health", sizeof(int32_t),
		_Alignof(int32_t), health, 1);
	w->tag = ecs_entity_init(w->ecs, &(ecs_entity_desc_t){ .name = "Tag" });
	ecs_add_id(w->ecs, w->tag, w->ids.sync);
	ecs_member_t reference[] = {
		{ .name = "target", .type = ecs_id(ecs_entity_t) }
	};
	w->reference = reflected(w, "Reference", sizeof(ecs_entity_t),
		_Alignof(ecs_entity_t), reference, 1);
	w->record = ecs_entity_init(w->ecs,
		&(ecs_entity_desc_t){ .name = "Record" });
	ecs_component_desc_t cd = { .entity = w->record };
	cd.type.size = sizeof(struct record);
	cd.type.alignment = _Alignof(struct record);
	CHECK(ecs_component_init(w->ecs, &cd) == w->record);
	ecs_set_hooks_id(w->ecs, w->record, &(ecs_type_hooks_t){
		.ctor = record_ctor, .dtor = record_dtor,
		.copy = record_copy, .move = record_move
	});
#define MEMBER(field, type_id) { .name = #field, .type = type_id, \
	.offset = offsetof(struct record, field), .use_offset = true }
	ecs_struct_desc_t sd = { .entity = w->record, .members = {
		MEMBER(nested, w->position),
		{ .name = "array", .type = ecs_id(ecs_i32_t), .count = 3,
			.offset = offsetof(struct record, array),
				.use_offset = true },
		MEMBER(number, ecs_id(ecs_u64_t)),
		MEMBER(real, ecs_id(ecs_f64_t)),
		MEMBER(text, ecs_id(ecs_string_t)),
		MEMBER(target, ecs_id(ecs_entity_t)),
		MEMBER(flag, ecs_id(ecs_bool_t))
	} };
#undef MEMBER
	CHECK(ecs_struct_init(w->ecs, &sd) == w->record);
	ecs_add_id(w->ecs, w->record, w->ids.sync);
	w->local = ecs_new(w->ecs);
}

void world_init(struct world *w, bool reverse)
{
	memset(w, 0, sizeof(*w));
	w->ecs = ecs_init();
	CHECK(w->ecs != NULL);
	if (reverse)
		for (unsigned i = 0; i < 23; i++)
			ecs_new(w->ecs);
	OK(sync_import(w->ecs, &w->ids));
	register_types(w, reverse);
}

void world_fini(struct world *w)
{
	sync_session_destroy(w->session);
	w->session = NULL;
	CHECK(ecs_is_alive(w->ecs, w->local));
	EQ(ecs_fini(w->ecs), 0);
	w->ecs = NULL;
}

void fixture_init(struct fixture *f, const sync_server_desc_t *server,
	const sync_client_desc_t *client)
{
	memset(f, 0, sizeof(*f));
	world_init(&f->server, false);
	OK(sync_server_create(f->server.ecs, server,
		&f->server.session, NULL));
	for (unsigned i = 0; i < 2; i++) {
		world_init(&f->clients[i], true);
		OK(sync_client_create(f->clients[i].ecs, client,
			&f->clients[i].session, NULL));
		OK(sync_peer_open(f->server.session, &f->peers[i]));
	}
}

void fixture_fini(struct fixture *f)
{
	world_fini(&f->clients[1]);
	world_fini(&f->clients[0]);
	world_fini(&f->server);
	EQ(live_records, 0);
}

void set_position(struct world *w, ecs_entity_t e, float x)
{
	struct position value = { x, x + 1 };
	ecs_set_id(w->ecs, e, w->position, sizeof(value), &value);
}

ecs_entity_t spawn(struct world *w, float x, ecs_entity_t peer)
{
	ecs_entity_t e = ecs_new(w->ecs);
	set_position(w, e, x);
	if (peer)
		ecs_add_pair(w->ecs, e, w->ids.replicate_to, peer);
	else
		ecs_add_id(w->ecs, e, w->ids.replicate_to_all);
	return e;
}

uint64_t network_id(struct world *w, ecs_entity_t e)
{
	const sync_network_id_t *id = ecs_get_id(w->ecs, e, w->ids.network_id);
	CHECK(id != NULL);
	CHECK(id->value != 0);
	return id->value;
}

ecs_entity_t replica(struct world *w, uint64_t id)
{
	ecs_entity_t e = 0;
	OK(sync_lookup(w->session, id, &e));
	CHECK(ecs_is_alive(w->ecs, e));
	return e;
}

void expect_position(struct world *w, uint64_t id, float x)
{
	const struct position *p = ecs_get_id(w->ecs,
		replica(w, id), w->position);
	CHECK(p != NULL);
	CHECK(p->x == x && p->y == x + 1);
}

void expect_absent(struct world *w, uint64_t id)
{
	ecs_entity_t e = UINT64_MAX;
	EQ(sync_lookup(w->session, id, &e), SYNC_ERR_NOT_FOUND);
	CHECK(e == 0);
}

sync_status_t status(sync_session_t *s, ecs_entity_t peer)
{
	sync_status_t result;
	OK(sync_get_status(s, peer, &result));
	CHECK(memchr(result.fault.field, 0, sizeof(result.fault.field)));
	CHECK(memchr(result.fault.message, 0, sizeof(result.fault.message)));
	return result;
}

void check_status_equal(const sync_status_t *a, const sync_status_t *b)
{
#define FIELD(f) CHECK(a->f == b->f)
	FIELD(role); FIELD(state); FIELD(stream_id); FIELD(last_tick);
	FIELD(last_sequence); FIELD(queued_bytes); FIELD(assembly_bytes);
	FIELD(memory_bytes);
	FIELD(entities);
	FIELD(peers);
	FIELD(faulted_peers);
	FIELD(fault.code); FIELD(fault.peer); FIELD(fault.type);
	FIELD(fault.tick); FIELD(fault.sequence); FIELD(fault.network_id);
#undef FIELD
	CHECK(!memcmp(a->session_id.bytes, b->session_id.bytes, 16));
	CHECK(!strcmp(a->fault.field, b->fault.field));
	CHECK(!strcmp(a->fault.message, b->fault.message));
}

void check_info_equal(const sync_frame_info_t *a, const sync_frame_info_t *b)
{
	CHECK(a->protocol_version == b->protocol_version);
	CHECK(a->kind == b->kind && a->stream_id == b->stream_id);
	CHECK(a->sequence == b->sequence && a->tick == b->tick);
	CHECK(a->payload_bytes == b->payload_bytes);
	CHECK(!memcmp(a->session_id.bytes, b->session_id.bytes, 16));
}

void capture(struct fixture *f, uint64_t tick)
{
	sync_capture_result_t result;
	OK(sync_capture(f->server.session, tick, &result));
	CHECK(result.tick == tick);
	EQ(result.peers_queued, 2);
	EQ(result.peers_faulted, 0);
}

void drain(sync_session_t *client)
{
	for (unsigned i = 0; i < 100000; i++) {
		sync_apply_result_t p;
		sync_result_t r = sync_apply(client, UINT32_MAX, &p);
		if (r == SYNC_EMPTY || r == SYNC_NEED_MORE || r == SYNC_END)
			return;
		OK(r);
		CHECK(p.updates_applied != 0);
	}
	CHECK(!"apply failed to quiesce");
}

void transfer(struct fixture *f, unsigned client)
{
	sync_session_t *s = f->server.session;
	sync_session_t *c = f->clients[client].session;
	for (unsigned i = 0; i < 100000; i++) {
		sync_frame_view_t v;
		sync_result_t r = sync_outbox_peek(s, f->peers[client], &v);
		if (r == SYNC_EMPTY) {
			drain(c);
			return;
		}
		OK(r);
		r = sync_receive(c, v.data, v.size);
		if (r == SYNC_BACKPRESSURE) {
			sync_apply_result_t p;
			r = sync_apply(c, UINT32_MAX, &p);
			CHECK(r >= 0 && p.frames_consumed != 0);
			continue;
		}
		OK(r);
		OK(sync_outbox_commit(s, f->peers[client], v.token));
	}
	CHECK(!"delivery failed to quiesce");
}

struct packets take_packets(sync_session_t *s, ecs_entity_t peer)
{
	struct packets p = {0};
	for (unsigned i = 0; i < 100000; i++) {
		sync_frame_view_t v;
		sync_result_t r = sync_outbox_peek(s, peer, &v);
		if (r == SYNC_EMPTY)
			return p;
		OK(r);
		void *array = realloc(p.items,
			(p.count + 1) * sizeof(*p.items));
		CHECK(array != NULL);
		p.items = array;
		struct packet *item = &p.items[p.count++];
		item->data = malloc(v.size);
		CHECK(item->data != NULL);
		memcpy(item->data, v.data, v.size);
		item->size = v.size;
		item->info = v.info;
		OK(sync_outbox_commit(s, peer, v.token));
	}
	CHECK(!"outbox failed to quiesce");
	return p;
}

void packets_fini(struct packets *p)
{
	for (size_t i = 0; i < p->count; i++)
		free(p->items[i].data);
	free(p->items);
	memset(p, 0, sizeof(*p));
}

void receive_packets(sync_session_t *c, const struct packets *p)
{
	for (size_t i = 0; i < p->count; i++)
		OK(sync_receive(c, p->items[i].data, p->items[i].size));
}

void set_record(struct world *w, ecs_entity_t e, struct record value)
{
	ecs_set_id(w->ecs, e, w->record, sizeof(value), &value);
}

void fixture_native_check(void)
{
	struct world w = { .ecs = ecs_init() };
	w.ids.sync = ecs_new(w.ecs);
	register_types(&w, false);
	ecs_entity_t e = ecs_new(w.ecs);
	set_record(&w, e, (struct record){ .text = "owned", .number = 42 });
	const struct record *r = ecs_get_id(w.ecs, e, w.record);
	CHECK(r && r->text && !strcmp(r->text, "owned") && r->number == 42);
	ecs_add_id(w.ecs, e, w.tag);
	ecs_remove_id(w.ecs, e, w.tag);
	set_record(&w, e, (struct record){ .text = "replacement" });
	ecs_delete(w.ecs, e);
	EQ(ecs_fini(w.ecs), 0);
	EQ(live_records, 0);
}
