/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

/* Nested values, arrays, integer boundaries and string bytes round-trip. */
TEST(schema_record_values)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 2090119613u);
	char bytes[] = { (char)0xff, (char)0x80, 'x', 0 };
	cucumber_reply();

	cucumber_wait(5, 2016034857u);
	struct record values[] = {
		{ .nested = {1, 2}, .array = {INT32_MIN, 0, INT32_MAX},
			.number = UINT64_MAX, .real = -0.0, .text = NULL },
		{ .number = 0, .real = INFINITY, .text = "", .flag = true },
		{ .real = -INFINITY, .text = bytes }
	};
	cucumber_reply();

	cucumber_wait(6, 182382677u);
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
	cucumber_reply();

	cucumber_wait(7, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* IEEE NaN payloads and negative zero retain their exact representations. */
TEST(schema_float_bits)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 264906011u);
	uint64_t bits = UINT64_C(0x7ff8000000001234);
	cucumber_reply();

	cucumber_wait(5, 3920163163u);
	uint32_t small_bits = UINT32_C(0x7fc05678);
	cucumber_reply();

	cucumber_wait(6, 3527781173u);
	struct record v = {0};
	cucumber_reply();

	cucumber_wait(7, 14897615u);
	memcpy(&v.real, &bits, sizeof(bits));
	cucumber_reply();

	cucumber_wait(8, 3592982236u);
	memcpy(&v.nested.x, &small_bits, sizeof(small_bits));
	cucumber_reply();

	cucumber_wait(9, 2815018413u);
	v.nested.y = -0.0f;
	cucumber_reply();

	cucumber_wait(10, 1847557009u);
	set_record(&f.server, e, v);
	cucumber_reply();

	cucumber_wait(11, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 1864182664u);
	const struct record *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)),
		f.clients[0].record);
	cucumber_reply();

	cucumber_wait(14, 801242577u);
	CHECK(r != NULL);
	cucumber_reply();

	cucumber_wait(15, 152143336u);
	CHECK(!memcmp(&r->real, &v.real, sizeof(v.real)));
	cucumber_reply();

	cucumber_wait(16, 3636679360u);
	CHECK(!memcmp(&r->nested, &v.nested, sizeof(v.nested)));
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
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
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 4051492360u);
	ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
	cucumber_reply();

	cucumber_wait(4, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(7, 2011819897u);
	receive_packets(f.clients[0].session, &p);
	cucumber_reply();

	cucumber_wait(8, 2636620177u);
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(9, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(10, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(11, 2907884561u);
	EQ(status(f.clients[1].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(12, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Extra marked client types are allowed and never become adopted replicas. */
TEST(schema_extra_client_type)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 565557418u);
	ecs_entity_t tag = ecs_entity_init(f.clients[0].ecs,
		&(ecs_entity_desc_t){ .name = "Extra" });
	cucumber_reply();

	cucumber_wait(4, 1428344644u);
	ecs_add_id(f.clients[0].ecs, tag, f.clients[0].ids.sync);
	cucumber_reply();

	cucumber_wait(5, 2080417983u);
	ecs_add_id(f.clients[0].ecs, f.clients[0].local, tag);
	cucumber_reply();

	cucumber_wait(6, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(7, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(8, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(9, 1162173616u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(10, 2813104410u);
	CHECK(ecs_has_id(f.clients[0].ecs, f.clients[0].local, tag));
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Schema matching uses member shapes, not native padding or offsets. */
TEST(schema_native_offsets)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 1059022998u);
	struct native_a { int32_t x; double y; };
	cucumber_reply();

	cucumber_wait(4, 2344411564u);
	struct native_b { double padding; double y; int32_t x; };
	cucumber_reply();

	cucumber_wait(5, 527524912u);
	ecs_member_t a[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
			.offset = offsetof(struct native_a, x),
				.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
			.offset = offsetof(struct native_a, y),
				.use_offset = true }
	};
	cucumber_reply();

	cucumber_wait(6, 2325055948u);
	ecs_member_t b[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
			.offset = offsetof(struct native_b, x),
				.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
			.offset = offsetof(struct native_b, y),
				.use_offset = true }
	};
	cucumber_reply();

	cucumber_wait(7, 2479399768u);
	ecs_entity_t sa = reflected(&f.server, "Padded",
		sizeof(struct native_a),
		_Alignof(struct native_a), a, 2);
	cucumber_reply();

	cucumber_wait(8, 3960675827u);
	ecs_entity_t cb[2];
	cucumber_reply();

	cucumber_wait(9, 3260767500u);
	for (unsigned i = 0; i < 2; i++)
		cb[i] = reflected(&f.clients[i], "Padded",
			sizeof(struct native_b),
			_Alignof(struct native_b), b, 2);
	cucumber_reply();

	cucumber_wait(10, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(11, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(12, 2337064854u);
	struct native_a value = {INT32_MIN, 1.25};
	cucumber_reply();

	cucumber_wait(13, 790542581u);
	ecs_set_id(f.server.ecs, e, sa, sizeof(value), &value);
	cucumber_reply();

	cucumber_wait(14, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(15, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(16, 2275353741u);
	const struct native_b *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)), cb[0]);
	cucumber_reply();

	cucumber_wait(17, 1206619107u);
	CHECK(r && r->x == value.x && r->y == value.y);
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Equal native widths do not make incompatible reflected scalars compatible. */
TEST(schema_shape_mismatch)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 3005819296u);
	for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		ecs_member_t member = { .name = "value",
			.type = i ? ecs_id(ecs_f32_t) : ecs_id(ecs_i32_t) };
		reflected(w, "Mismatch", 4, 4, &member, 1);
	}
	cucumber_reply();

	cucumber_wait(4, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(7, 2011819897u);
	receive_packets(f.clients[0].session, &p);
	cucumber_reply();

	cucumber_wait(8, 2636620177u);
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(9, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(10, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Unnamed enum values and bitmask bits survive reflection-based encoding. */
TEST(schema_enum_bitmask)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 2382944867u);
	ecs_entity_t enums[3], masks[3];
	cucumber_reply();

	cucumber_wait(4, 4031223618u);
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
	cucumber_reply();

	cucumber_wait(5, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(6, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(7, 4230909079u);
	int32_t mode = -123;
	cucumber_reply();

	cucumber_wait(8, 3360523917u);
	uint32_t flags = UINT32_C(0x80000001);
	cucumber_reply();

	cucumber_wait(9, 3711701002u);
	ecs_set_id(f.server.ecs, e, enums[0], sizeof(mode), &mode);
	cucumber_reply();

	cucumber_wait(10, 1461751638u);
	ecs_set_id(f.server.ecs, e, masks[0], sizeof(flags), &flags);
	cucumber_reply();

	cucumber_wait(11, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 3826206884u);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	cucumber_reply();

	cucumber_wait(14, 3353001825u);
	const int32_t *m = ecs_get_id(f.clients[0].ecs, r, enums[1]);
	cucumber_reply();

	cucumber_wait(15, 1061082252u);
	const uint32_t *b = ecs_get_id(f.clients[0].ecs, r, masks[1]);
	cucumber_reply();

	cucumber_wait(16, 1358678094u);
	CHECK(m && *m == mode && b && *b == flags);
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Platform-sized scalars are rejected even on a fixed-width native platform. */
TEST(schema_unsupported_pointer_scalar)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 3280519958u);
	ecs_member_t m = { .name = "pointer", .type = ecs_id(ecs_uptr_t) };
	cucumber_reply();

	cucumber_wait(4, 3210103854u);
	reflected(&w, "Pointer", sizeof(uintptr_t), _Alignof(uintptr_t), &m, 1);
	cucumber_reply();

	cucumber_wait(5, 1548599521u);
	sync_error_t error;
	cucumber_reply();

	cucumber_wait(6, 4011609831u);
	EQ(sync_server_create(w.ecs, NULL, &w.session, &error),
		SYNC_ERR_UNSUPPORTED);
	cucumber_reply();

	cucumber_wait(7, 2691842903u);
	CHECK(!w.session);
	cucumber_reply();

	cucumber_wait(8, 3860131166u);
	EQ(error.code, SYNC_ERR_UNSUPPORTED);
	cucumber_reply();

	cucumber_wait(9, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Dynamic reflected vectors are outside the supported schema contract. */
TEST(schema_unsupported_vector)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 963122336u);
	ecs_entity_t v = ecs_vector_init(w.ecs, &(ecs_vector_desc_t){
		.entity = ecs_entity_init(w.ecs,
			&(ecs_entity_desc_t){ .name = "Dynamic" }),
		.type = ecs_id(ecs_i32_t)
	});
	cucumber_reply();

	cucumber_wait(4, 4062428551u);
	CHECK(v != 0);
	cucumber_reply();

	cucumber_wait(5, 2049175558u);
	ecs_add_id(w.ecs, v, w.ids.sync);
	cucumber_reply();

	cucumber_wait(6, 532506223u);
	EQ(sync_client_create(w.ecs, NULL, &w.session, NULL),
		SYNC_ERR_UNSUPPORTED);
	cucumber_reply();

	cucumber_wait(7, 2691842903u);
	CHECK(!w.session);
	cucumber_reply();

	cucumber_wait(8, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Type-count, array-count and nesting bounds reject excessive schemas. */
TEST(schema_limits)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 3152328478u);
	sync_server_desc_t d[] = {
		{ .limits.max_types = 5 },
		{ .limits.max_elements = 2 },
		{ .limits.max_depth = 1 },
		{ .limits.max_schema_bytes = 1 },
		{ .limits.max_memory_bytes = 1 }
	};
	cucumber_reply();

	cucumber_wait(4, 1859419710u);
	for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); i++) {
		EQ(sync_server_create(w.ecs, &d[i], &w.session, NULL),
			SYNC_ERR_LIMIT);
		CHECK(w.session == NULL);
	}
	cucumber_reply();

	cucumber_wait(5, 1987020046u);
	sync_server_desc_t exact = { .limits = {
		.max_types = 6, .max_elements = 3
	} };
	cucumber_reply();

	cucumber_wait(6, 3583783765u);
	OK(sync_server_create(w.ecs, &exact, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(7, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Strings at their configured byte bound pass; the next byte faults. */
TEST(schema_string_limit)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2359813629u);
	sync_server_desc_t d = { .limits.max_string_bytes = 3 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 3553176945u);
	ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 5322281u);
	set_record(&f.server, e, (struct record){ .text = "abc" });
	cucumber_reply();

	cucumber_wait(6, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(8, 2824623138u);
	set_record(&f.server, e, (struct record){ .text = "abcd" });
	cucumber_reply();

	cucumber_wait(9, 4179896338u);
	sync_capture_result_t progress;
	cucumber_reply();

	cucumber_wait(10, 553714294u);
	sync_result_t r = sync_capture(f.server.session, 2, &progress);
	cucumber_reply();

	cucumber_wait(11, 241301110u);
	CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(12, 1425176444u);
	CHECK(progress.tick == 2);
	cucumber_reply();

	cucumber_wait(13, 3187904027u);
	EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(14, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Entity bounds fault rather than silently omitting newly visible state. */
TEST(schema_entity_limit)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 4282991735u);
	sync_server_desc_t d = { .limits.max_entities = 1 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 1637123758u);
	spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 3875599360u);
	spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(7, 3215356182u);
	sync_result_t r = sync_capture(f.server.session, 2, NULL);
	cucumber_reply();

	cucumber_wait(8, 241301110u);
	CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(9, 4274295959u);
	CHECK(status(f.server.session, 0).entities <= 1);
	cucumber_reply();

	cucumber_wait(10, 3187904027u);
	EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Nontrivial values release all native storage after replacement and reset. */
TEST(schema_owned_cleanup)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 502718977u);
	for (uint64_t tick = 1; tick < 5; tick++) {
		set_record(&f.server, e,
			(struct record){ .text = tick % 2 ? "owned" : NULL });
		capture(&f, tick);
		transfer(&f, 0);
	}
	cucumber_reply();

	cucumber_wait(5, 654932273u);
	CHECK(live_records > 0);
	cucumber_reply();

	cucumber_wait(6, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(7, 477425722u);
	fixture_fini(&f);
	cucumber_reply();

	cucumber_wait(8, 3944394909u);
	EQ(live_records, 0);
	cucumber_reply();
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
	cucumber_wait(1, 2884640623u);
	struct packets a = deterministic_history();
	cucumber_reply();

	cucumber_wait(2, 2838370376u);
	struct packets b = deterministic_history();
	cucumber_reply();

	cucumber_wait(3, 2207817963u);
	CHECK(a.count == b.count && a.count != 0);
	cucumber_reply();

	cucumber_wait(4, 3457317291u);
	for (size_t i = 0; i < a.count; i++) {
		CHECK(a.items[i].size == b.items[i].size);
		CHECK(!memcmp(a.items[i].data, b.items[i].data,
			a.items[i].size));
		check_info_equal(&a.items[i].info, &b.items[i].info);
	}
	cucumber_reply();

	cucumber_wait(5, 1875006789u);
	packets_fini(&b);
	cucumber_reply();

	cucumber_wait(6, 1837427484u);
	packets_fini(&a);
	cucumber_reply();
}

/* All fixed-width signed and unsigned integers preserve extrema. */
TEST(schema_integer_widths)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 18434358u);
	unconnected(&f);
	cucumber_reply();

	cucumber_wait(3, 3284875034u);
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
	cucumber_reply();

	cucumber_wait(4, 3648919473u);
#define SCALAR(field, scalar) { .name = #field, .type = ecs_id(scalar), \
	.offset = offsetof(struct scalars, field), .use_offset = true }
	ecs_member_t members[] = {
		SCALAR(i8, ecs_i8_t), SCALAR(u8, ecs_u8_t),
		SCALAR(i16, ecs_i16_t), SCALAR(u16, ecs_u16_t),
		SCALAR(i32, ecs_i32_t), SCALAR(u32, ecs_u32_t),
		SCALAR(i64, ecs_i64_t), SCALAR(u64, ecs_u64_t)
	};
	cucumber_reply();

	cucumber_wait(5, 3686046155u);
#undef SCALAR
	ecs_entity_t types[3];
	cucumber_reply();

	cucumber_wait(6, 24029471u);
	for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		types[i] = reflected(w, "Scalars", sizeof(struct scalars),
			_Alignof(struct scalars), members, 8);
	}
	cucumber_reply();

	cucumber_wait(7, 211282762u);
	connect_worlds(&f);
	cucumber_reply();

	cucumber_wait(8, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(9, 3120581109u);
	struct scalars values[] = {
		{INT8_MIN, UINT8_MAX, INT16_MIN, UINT16_MAX,
			INT32_MIN, UINT32_MAX, INT64_MIN, UINT64_MAX},
		{INT8_MAX, 0, INT16_MAX, 0, INT32_MAX, 0, INT64_MAX, 0}
	};
	cucumber_reply();

	cucumber_wait(10, 2191602597u);
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
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Snapshot totals obey logical-update bounds just like deltas. */
TEST(schema_update_limit)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3387822306u);
	sync_server_desc_t d = { .limits.max_update_bytes = 1024 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 2604027767u);
	char text[2049];
	cucumber_reply();

	cucumber_wait(6, 6321540u);
	memset(text, 'x', sizeof(text) - 1);
	cucumber_reply();

	cucumber_wait(7, 1241318640u);
	text[sizeof(text) - 1] = 0;
	cucumber_reply();

	cucumber_wait(8, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(9, 4179896338u);
	sync_capture_result_t progress;
	cucumber_reply();

	cucumber_wait(10, 1566582701u);
	sync_result_t r = sync_capture(f.server.session, 1, &progress);
	cucumber_reply();

	cucumber_wait(11, 1066587633u);
	CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(12, 1076680702u);
	CHECK(progress.tick == 1 && progress.peers_queued == 0);
	cucumber_reply();

	cucumber_wait(13, 2960022781u);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(14, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Fragment bounds fault oversized updates instead of retaining partial data. */
TEST(schema_fragment_limit)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2409588680u);
	sync_server_desc_t d = { .limits.max_fragments = 1 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 2259524145u);
	char *text = malloc(65536);
	cucumber_reply();

	cucumber_wait(6, 2915115124u);
	CHECK(text != NULL);
	cucumber_reply();

	cucumber_wait(7, 1153590984u);
	memset(text, 'x', 65535);
	cucumber_reply();

	cucumber_wait(8, 2263684211u);
	text[65535] = 0;
	cucumber_reply();

	cucumber_wait(9, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(10, 2594712615u);
	free(text);
	cucumber_reply();

	cucumber_wait(11, 4179896338u);
	sync_capture_result_t progress;
	cucumber_reply();

	cucumber_wait(12, 1566582701u);
	sync_result_t r = sync_capture(f.server.session, 1, &progress);
	cucumber_reply();

	cucumber_wait(13, 1066587633u);
	CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(14, 2821227224u);
	CHECK(!progress.peers_queued);
	cucumber_reply();

	cucumber_wait(15, 2960022781u);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(16, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}
