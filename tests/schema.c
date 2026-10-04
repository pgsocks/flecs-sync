#include "test.h"

/* Nested values, arrays, integer boundaries and string bytes round-trip. */
TEST(schema_record_values)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char bytes[] = { (char)0xff, (char)0x80, 'x', 0 };
	struct record values[] = {
		{ .nested = {1, 2}, .array = {INT32_MIN, 0, INT32_MAX},
			.number = UINT64_MAX, .real = -0.0, .text = NULL },
		{ .number = 0, .real = INFINITY, .text = "", .flag = true },
		{ .real = -INFINITY, .text = bytes }
	};
	for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
		set_record(&f.server, e, values[i]);
		capture(&f, i + 1);
		transfer(&f, 0);
		const struct record *r = ecs_get_id(f.clients[0].ecs,
			replica(&f.clients[0], network_id(&f.server, e)),
			f.clients[0].record);
		CHECK(r != NULL);
		CHECK(r->nested.x == values[i].nested.x);
		CHECK(r->nested.y == values[i].nested.y);
		CHECK(!memcmp(r->array, values[i].array, sizeof(r->array)));
		CHECK(r->number == values[i].number &&
			r->flag == values[i].flag);
		CHECK(!memcmp(&r->real, &values[i].real, sizeof(r->real)));
		if (values[i].text)
			CHECK(r->text && !strcmp(r->text, values[i].text));
		else
			CHECK(r->text == NULL);
	}
	fixture_fini(&f);
}

/* IEEE NaN payloads and negative zero retain their exact representations. */
TEST(schema_float_bits)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	uint64_t bits = UINT64_C(0x7ff8000000001234);
	uint32_t small_bits = UINT32_C(0x7fc05678);
	struct record v = {0};
	memcpy(&v.real, &bits, sizeof(bits));
	memcpy(&v.nested.x, &small_bits, sizeof(small_bits));
	v.nested.y = -0.0f;
	set_record(&f.server, e, v);
	capture(&f, 1);
	transfer(&f, 0);
	const struct record *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)),
		f.clients[0].record);
	CHECK(r != NULL);
	CHECK(!memcmp(&r->real, &v.real, sizeof(v.real)));
	CHECK(!memcmp(&r->nested, &v.nested, sizeof(v.nested)));
	fixture_fini(&f);
}

static void connect_worlds(struct fixture *f)
{
	OK(sync_server_create(f->server.ecs, NULL, &f->server.session, NULL));
	for (unsigned i = 0; i < 2; i++) {
		OK(sync_client_create(f->clients[i].ecs, NULL,
			&f->clients[i].session, NULL));
		OK(sync_peer_open(f->server.session, &f->peers[i]));
	}
}

static void unconnected(struct fixture *f)
{
	memset(f, 0, sizeof(*f));
	world_init(&f->server, false);
	world_init(&f->clients[0], true);
	world_init(&f->clients[1], true);
}

/* Unused manifest types still require a compatible marked client type. */
TEST(schema_missing_type)
{
	struct fixture f;
	unconnected(&f);
	ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
	connect_worlds(&f);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
	EQ(status(f.clients[0].session, 0).entities, 0);
	transfer(&f, 1);
	EQ(status(f.clients[1].session, 0).state, SYNC_CLIENT_READY);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Extra marked client types are allowed and never become adopted replicas. */
TEST(schema_extra_client_type)
{
	struct fixture f;
	unconnected(&f);
	ecs_entity_t tag = ecs_entity_init(f.clients[0].ecs,
		&(ecs_entity_desc_t){ .name = "Extra" });
	ecs_add_id(f.clients[0].ecs, tag, f.clients[0].ids.sync);
	ecs_add_id(f.clients[0].ecs, f.clients[0].local, tag);
	connect_worlds(&f);
	capture(&f, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	CHECK(ecs_has_id(f.clients[0].ecs, f.clients[0].local, tag));
	fixture_fini(&f);
}

/* Schema matching uses member shapes, not native padding or offsets. */
TEST(schema_native_offsets)
{
	struct fixture f;
	unconnected(&f);
	struct native_a { int32_t x; double y; };
	struct native_b { double padding; double y; int32_t x; };
	ecs_member_t a[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
			.offset = offsetof(struct native_a, x),
				.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
			.offset = offsetof(struct native_a, y),
				.use_offset = true }
	};
	ecs_member_t b[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
			.offset = offsetof(struct native_b, x),
				.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
			.offset = offsetof(struct native_b, y),
				.use_offset = true }
	};
	ecs_entity_t sa = reflected(&f.server, "Padded",
		sizeof(struct native_a),
		_Alignof(struct native_a), a, 2);
	ecs_entity_t cb[2];
	for (unsigned i = 0; i < 2; i++)
		cb[i] = reflected(&f.clients[i], "Padded",
			sizeof(struct native_b),
			_Alignof(struct native_b), b, 2);
	connect_worlds(&f);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	struct native_a value = {INT32_MIN, 1.25};
	ecs_set_id(f.server.ecs, e, sa, sizeof(value), &value);
	capture(&f, 1);
	transfer(&f, 0);
	const struct native_b *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)), cb[0]);
	CHECK(r && r->x == value.x && r->y == value.y);
	fixture_fini(&f);
}

/* Equal native widths do not make incompatible reflected scalars compatible. */
TEST(schema_shape_mismatch)
{
	struct fixture f;
	unconnected(&f);
	for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		ecs_member_t member = { .name = "value",
			.type = i ? ecs_id(ecs_f32_t) : ecs_id(ecs_i32_t) };
		reflected(w, "Mismatch", 4, 4, &member, 1);
	}
	connect_worlds(&f);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
	EQ(status(f.clients[0].session, 0).entities, 0);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Unnamed enum values and bitmask bits survive reflection-based encoding. */
TEST(schema_enum_bitmask)
{
	struct fixture f;
	unconnected(&f);
	ecs_entity_t enums[3], masks[3];
	for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		enums[i] = ecs_enum_init(w->ecs, &(ecs_enum_desc_t){
			.entity = ecs_entity_init(w->ecs,
				&(ecs_entity_desc_t){ .name = "Mode" }),
			.underlying_type = ecs_id(ecs_i32_t),
			.constants = {{ .name = "Known", .value = 1 }}
		});
		masks[i] = ecs_bitmask_init(w->ecs, &(ecs_bitmask_desc_t){
			.entity = ecs_entity_init(w->ecs,
				&(ecs_entity_desc_t){ .name = "Flags" }),
			.constants = {{ .name = "Known", .value = 1 }}
		});
		CHECK(enums[i] && masks[i]);
		ecs_add_id(w->ecs, enums[i], w->ids.sync);
		ecs_add_id(w->ecs, masks[i], w->ids.sync);
	}
	connect_worlds(&f);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	int32_t mode = -123;
	uint32_t flags = UINT32_C(0x80000001);
	ecs_set_id(f.server.ecs, e, enums[0], sizeof(mode), &mode);
	ecs_set_id(f.server.ecs, e, masks[0], sizeof(flags), &flags);
	capture(&f, 1);
	transfer(&f, 0);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	const int32_t *m = ecs_get_id(f.clients[0].ecs, r, enums[1]);
	const uint32_t *b = ecs_get_id(f.clients[0].ecs, r, masks[1]);
	CHECK(m && *m == mode && b && *b == flags);
	fixture_fini(&f);
}

/* Platform-sized scalars are rejected even on a fixed-width native platform. */
TEST(schema_unsupported_pointer_scalar)
{
	struct world w;
	world_init(&w, false);
	ecs_member_t m = { .name = "pointer", .type = ecs_id(ecs_uptr_t) };
	reflected(&w, "Pointer", sizeof(uintptr_t), _Alignof(uintptr_t), &m, 1);
	sync_error_t error;
	EQ(sync_server_create(w.ecs, NULL, &w.session, &error),
		SYNC_ERR_UNSUPPORTED);
	CHECK(!w.session);
	EQ(error.code, SYNC_ERR_UNSUPPORTED);
	world_fini(&w);
}

/* Dynamic reflected vectors are outside the supported schema contract. */
TEST(schema_unsupported_vector)
{
	struct world w;
	world_init(&w, false);
	ecs_entity_t v = ecs_vector_init(w.ecs, &(ecs_vector_desc_t){
		.entity = ecs_entity_init(w.ecs,
			&(ecs_entity_desc_t){ .name = "Dynamic" }),
		.type = ecs_id(ecs_i32_t)
	});
	CHECK(v != 0);
	ecs_add_id(w.ecs, v, w.ids.sync);
	EQ(sync_client_create(w.ecs, NULL, &w.session, NULL),
		SYNC_ERR_UNSUPPORTED);
	CHECK(!w.session);
	world_fini(&w);
}

/* Type-count, array-count and nesting bounds reject excessive schemas. */
TEST(schema_limits)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t d[] = {
		{ .limits.max_types = 5 },
		{ .limits.max_elements = 2 },
		{ .limits.max_depth = 1 },
		{ .limits.max_schema_bytes = 1 },
		{ .limits.max_memory_bytes = 1 }
	};
	for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); i++) {
		EQ(sync_server_create(w.ecs, &d[i], &w.session, NULL),
			SYNC_ERR_LIMIT);
		CHECK(w.session == NULL);
	}
	sync_server_desc_t exact = { .limits = {
		.max_types = 6, .max_elements = 3
	} };
	OK(sync_server_create(w.ecs, &exact, &w.session, NULL));
	world_fini(&w);
}

/* Strings at their configured byte bound pass; the next byte faults. */
TEST(schema_string_limit)
{
	struct fixture f;
	sync_server_desc_t d = { .limits.max_string_bytes = 3 };
	fixture_init(&f, &d, NULL);
	ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
	set_record(&f.server, e, (struct record){ .text = "abc" });
	capture(&f, 1);
	transfer(&f, 0);
	set_record(&f.server, e, (struct record){ .text = "abcd" });
	sync_capture_result_t progress;
	sync_result_t r = sync_capture(f.server.session, 2, &progress);
	CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
	CHECK(progress.tick == 2);
	EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
	fixture_fini(&f);
}

/* Entity bounds fault rather than silently omitting newly visible state. */
TEST(schema_entity_limit)
{
	struct fixture f;
	sync_server_desc_t d = { .limits.max_entities = 1 };
	fixture_init(&f, &d, NULL);
	spawn(&f.server, 1, 0);
	capture(&f, 1);
	spawn(&f.server, 2, 0);
	sync_result_t r = sync_capture(f.server.session, 2, NULL);
	CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
	CHECK(status(f.server.session, 0).entities <= 1);
	EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
	fixture_fini(&f);
}

/* Nontrivial values release all native storage after replacement and reset. */
TEST(schema_owned_cleanup)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	for (uint64_t tick = 1; tick < 5; tick++) {
		set_record(&f.server, e,
			(struct record){ .text = tick % 2 ? "owned" : NULL });
		capture(&f, tick);
		transfer(&f, 0);
	}
	CHECK(live_records > 0);
	OK(sync_client_reset(f.clients[0].session));
	fixture_fini(&f);
	EQ(live_records, 0);
}

static struct packets deterministic_history(void)
{
	struct fixture f;
	sync_server_desc_t d = { .session_id.bytes = {9, 8, 7, 6} };
	fixture_init(&f, &d, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	set_position(&f.server, e, 2);
	capture(&f, 2);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	fixture_fini(&f);
	return p;
}

/* Identical sequential fixture histories yield identical canonical bytes. */
TEST(schema_deterministic_bytes)
{
	struct packets a = deterministic_history();
	struct packets b = deterministic_history();
	CHECK(a.count == b.count && a.count != 0);
	for (size_t i = 0; i < a.count; i++) {
		CHECK(a.items[i].size == b.items[i].size);
		CHECK(!memcmp(a.items[i].data, b.items[i].data,
			a.items[i].size));
		check_info_equal(&a.items[i].info, &b.items[i].info);
	}
	packets_fini(&b);
	packets_fini(&a);
}

/* All fixed-width signed and unsigned integers preserve extrema. */
TEST(schema_integer_widths)
{
	struct fixture f;
	unconnected(&f);
	struct scalars {
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
	};
#define SCALAR(field, scalar) { .name = #field, .type = ecs_id(scalar), \
	.offset = offsetof(struct scalars, field), .use_offset = true }
	ecs_member_t members[] = {
		SCALAR(i8, ecs_i8_t), SCALAR(u8, ecs_u8_t),
		SCALAR(i16, ecs_i16_t), SCALAR(u16, ecs_u16_t),
		SCALAR(i32, ecs_i32_t), SCALAR(u32, ecs_u32_t),
		SCALAR(i64, ecs_i64_t), SCALAR(u64, ecs_u64_t)
	};
#undef SCALAR
	ecs_entity_t types[3];
	for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		types[i] = reflected(w, "Scalars", sizeof(struct scalars),
			_Alignof(struct scalars), members, 8);
	}
	connect_worlds(&f);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	struct scalars values[] = {
		{INT8_MIN, UINT8_MAX, INT16_MIN, UINT16_MAX,
			INT32_MIN, UINT32_MAX, INT64_MIN, UINT64_MAX},
		{INT8_MAX, 0, INT16_MAX, 0, INT32_MAX, 0, INT64_MAX, 0}
	};
	for (size_t i = 0; i < 2; i++) {
		ecs_set_id(f.server.ecs, e, types[0], sizeof(values[i]),
			&values[i]);
		capture(&f, i + 1);
		transfer(&f, 0);
		const struct scalars *r = ecs_get_id(f.clients[0].ecs,
			replica(&f.clients[0], network_id(&f.server, e)),
				types[1]);
		CHECK(r != NULL);
#define VALUE(field) CHECK(r->field == values[i].field)
		VALUE(i8); VALUE(u8); VALUE(i16); VALUE(u16);
		VALUE(i32); VALUE(u32); VALUE(i64); VALUE(u64);
#undef VALUE
	}
	fixture_fini(&f);
}

/* Snapshot totals obey logical-update bounds just like deltas. */
TEST(schema_update_limit)
{
	struct fixture f;
	sync_server_desc_t d = { .limits.max_update_bytes = 1024 };
	fixture_init(&f, &d, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char text[2049];
	memset(text, 'x', sizeof(text) - 1);
	text[sizeof(text) - 1] = 0;
	set_record(&f.server, e, (struct record){ .text = text });
	sync_capture_result_t progress;
	sync_result_t r = sync_capture(f.server.session, 1, &progress);
	CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
	CHECK(progress.tick == 1 && progress.peers_queued == 0);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	fixture_fini(&f);
}

/* Fragment bounds fault oversized updates instead of retaining partial data. */
TEST(schema_fragment_limit)
{
	struct fixture f;
	sync_server_desc_t d = { .limits.max_fragments = 1 };
	fixture_init(&f, &d, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char *text = malloc(65536);
	CHECK(text != NULL);
	memset(text, 'x', 65535);
	text[65535] = 0;
	set_record(&f.server, e, (struct record){ .text = text });
	free(text);
	sync_capture_result_t progress;
	sync_result_t r = sync_capture(f.server.session, 1, &progress);
	CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
	CHECK(!progress.peers_queued);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	fixture_fini(&f);
}
