Feature: schema contracts

@core
Scenario: schema_record_values
	# When nested values and null, empty and non-UTF-8 strings are sent
	# Then Nested values, arrays, integer boundaries and string bytes round-trip.
	Given the "core" workflow "schema_record_values"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		char bytes[] = { (char)0xff, (char)0x80, 'x', 0 };
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct record values[] = {
		{ .nested = {1, 2}, .array = {INT32_MIN, 0, INT32_MAX},
		.number = UINT64_MAX, .real = -0.0, .text = NULL },
		{ .number = 0, .real = INFINITY, .text = "", .flag = true },
		{ .real = -INFINITY, .text = bytes }
		};
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_float_bits
	# When values contain NaN payloads and negative zero
	# Then IEEE NaN payloads and negative zero retain their exact representations.
	Given the "core" workflow "schema_float_bits"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		uint64_t bits = UINT64_C(0x7ff8000000001234);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		uint32_t small_bits = UINT32_C(0x7fc05678);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct record v = {0};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		memcpy(&v.real, &bits, sizeof(bits));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		memcpy(&v.nested.x, &small_bits, sizeof(small_bits));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		v.nested.y = -0.0f;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		set_record(&f.server, e, v);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		const struct record *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)),
		f.clients[0].record);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(r != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!memcmp(&r->real, &v.real, sizeof(v.real)));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(!memcmp(&r->nested, &v.nested, sizeof(v.nested)));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_missing_type
	# When a client lacks a marked type included in the manifest
	# Then Unused manifest types still require a compatible marked client type.
	Given the "core" workflow "schema_missing_type"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		receive_packets(f.clients[0].session, &p);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(status(f.clients[1].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_extra_client_type
	# When the client marks a type absent from the server schema
	# Then Extra marked client types are allowed and never become adopted replicas.
	Given the "core" workflow "schema_extra_client_type"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t tag = ecs_entity_init(f.clients[0].ecs,
		&(ecs_entity_desc_t){ .name = "Extra" });
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_add_id(f.clients[0].ecs, tag, f.clients[0].ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_id(f.clients[0].ecs, f.clients[0].local, tag);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(ecs_has_id(f.clients[0].ecs, f.clients[0].local, tag));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_native_offsets
	# When matching reflected members have different native offsets
	# Then Schema matching uses member shapes, not native padding or offsets.
	Given the "core" workflow "schema_native_offsets"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		struct native_a { int32_t x; double y; };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		struct native_b { double padding; double y; int32_t x; };
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_member_t a[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
		.offset = offsetof(struct native_a, x),
		.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
		.offset = offsetof(struct native_a, y),
		.use_offset = true }
		};
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_member_t b[] = {
		{ .name = "x", .type = ecs_id(ecs_i32_t),
		.offset = offsetof(struct native_b, x),
		.use_offset = true },
		{ .name = "y", .type = ecs_id(ecs_f64_t),
		.offset = offsetof(struct native_b, y),
		.use_offset = true }
		};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t sa = reflected(&f.server, "Padded",
		sizeof(struct native_a),
		_Alignof(struct native_a), a, 2);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t cb[2];
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		for (unsigned i = 0; i < 2; i++)
		cb[i] = reflected(&f.clients[i], "Padded",
		sizeof(struct native_b),
		_Alignof(struct native_b), b, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		struct native_a value = {INT32_MIN, 1.25};
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_set_id(f.server.ecs, e, sa, sizeof(value), &value);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		const struct native_b *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)), cb[0]);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		CHECK(r && r->x == value.x && r->y == value.y);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_shape_mismatch
	# When matching type names describe different scalar shapes
	# Then Equal native widths do not make incompatible reflected scalars
	# compatible.
	Given the "core" workflow "schema_shape_mismatch"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		ecs_member_t member = { .name = "value",
		.type = i ? ecs_id(ecs_f32_t) : ecs_id(ecs_i32_t) };
		reflected(w, "Mismatch", 4, 4, &member, 1);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		receive_packets(f.clients[0].session, &p);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_enum_bitmask
	# When unnamed enum values and unknown bitmask bits are sent
	# Then Unnamed enum values and bitmask bits survive reflection-based encoding.
	Given the "core" workflow "schema_enum_bitmask"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t enums[3], masks[3];
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		int32_t mode = -123;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		uint32_t flags = UINT32_C(0x80000001);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_set_id(f.server.ecs, e, enums[0], sizeof(mode), &mode);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_set_id(f.server.ecs, e, masks[0], sizeof(flags), &flags);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		const int32_t *m = ecs_get_id(f.clients[0].ecs, r, enums[1]);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		const uint32_t *b = ecs_get_id(f.clients[0].ecs, r, masks[1]);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(m && *m == mode && b && *b == flags);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_unsupported_pointer_scalar
	# When a marked struct contains a platform-sized scalar
	# Then Platform-sized scalars are rejected even on a fixed-width native
	# platform.
	Given the "core" workflow "schema_unsupported_pointer_scalar"
	When I perform operation 1:
		"""
		struct world w;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_init(&w, false);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_member_t m = { .name = "pointer", .type = ecs_id(ecs_uptr_t) };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		reflected(&w, "Pointer", sizeof(uintptr_t), _Alignof(uintptr_t), &m, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_server_create(w.ecs, NULL, &w.session, &error),
		SYNC_ERR_UNSUPPORTED);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!w.session);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(error.code, SYNC_ERR_UNSUPPORTED);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: schema_unsupported_vector
	# When a marked type is a dynamic reflected vector
	# Then Dynamic reflected vectors are outside the supported schema contract.
	Given the "core" workflow "schema_unsupported_vector"
	When I perform operation 1:
		"""
		struct world w;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_init(&w, false);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t v = ecs_vector_init(w.ecs, &(ecs_vector_desc_t){
		.entity = ecs_entity_init(w.ecs,
		&(ecs_entity_desc_t){ .name = "Dynamic" }),
		.type = ecs_id(ecs_i32_t)
		});
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(v != 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_id(w.ecs, v, w.ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_client_create(w.ecs, NULL, &w.session, NULL),
		SYNC_ERR_UNSUPPORTED);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!w.session);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: schema_limits
	# When schema size, type, array, depth and memory limits are crossed
	# Then Type-count, array-count and nesting bounds reject excessive schemas.
	Given the "core" workflow "schema_limits"
	When I perform operation 1:
		"""
		struct world w;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_init(&w, false);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_server_desc_t d[] = {
		{ .limits.max_types = 5 },
		{ .limits.max_elements = 2 },
		{ .limits.max_depth = 1 },
		{ .limits.max_schema_bytes = 1 },
		{ .limits.max_memory_bytes = 1 }
		};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); i++) {
		EQ(sync_server_create(w.ecs, &d[i], &w.session, NULL),
		SYNC_ERR_LIMIT);
		CHECK(w.session == NULL);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_server_desc_t exact = { .limits = {
		.max_types = 6, .max_elements = 3
		} };
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_server_create(w.ecs, &exact, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: schema_string_limit
	# When a string grows from the configured maximum to one byte beyond
	# Then Strings at their configured byte bound pass; the next byte faults.
	Given the "core" workflow "schema_string_limit"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .limits.max_string_bytes = 3 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_record(&f.server, e, (struct record){ .text = "abc" });
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		set_record(&f.server, e, (struct record){ .text = "abcd" });
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_capture_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_result_t r = sync_capture(f.server.session, 2, &progress);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(progress.tick == 2);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_entity_limit
	# When a new visible entity exceeds the configured mapping count
	# Then Entity bounds fault rather than silently omitting newly visible state.
	Given the "core" workflow "schema_entity_limit"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .limits.max_entities = 1 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_result_t r = sync_capture(f.server.session, 2, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(r == SYNC_ERR_LIMIT || r == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(status(f.server.session, 0).entities <= 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(status(f.server.session, f.peers[0]).state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_owned_cleanup
	# When owned strings are replaced and the client is reset
	# Then Nontrivial values release all native storage after replacement and
	# reset.
	Given the "core" workflow "schema_owned_cleanup"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (uint64_t tick = 1; tick < 5; tick++) {
		set_record(&f.server, e,
		(struct record){ .text = tick % 2 ? "owned" : NULL });
		capture(&f, tick);
		transfer(&f, 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(live_records > 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(live_records, 0);
		"""
	Then its observed values match the contract

@core
Scenario: schema_deterministic_bytes
	# When the same fixed-identity capture history is repeated
	# Then Identical sequential fixture histories yield identical canonical bytes.
	Given the "core" workflow "schema_deterministic_bytes"
	When I perform operation 1:
		"""
		struct packets a = deterministic_history();
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct packets b = deterministic_history();
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		CHECK(a.count == b.count && a.count != 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (size_t i = 0; i < a.count; i++) {
		CHECK(a.items[i].size == b.items[i].size);
		CHECK(!memcmp(a.items[i].data, b.items[i].data,
		a.items[i].size));
		check_info_equal(&a.items[i].info, &b.items[i].info);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		packets_fini(&b);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		packets_fini(&a);
		"""
	Then its observed values match the contract

@core
Scenario: schema_integer_widths
	# When signed and unsigned values at every supported width are sent
	# Then All fixed-width signed and unsigned integers preserve extrema.
	Given the "core" workflow "schema_integer_widths"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		unconnected(&f);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		.offset = offsetof(struct scalars, field), .use_offset = true }
		ecs_member_t members[] = {
		SCALAR(i8, ecs_i8_t), SCALAR(u8, ecs_u8_t),
		SCALAR(i16, ecs_i16_t), SCALAR(u16, ecs_u16_t),
		SCALAR(i32, ecs_i32_t), SCALAR(u32, ecs_u32_t),
		SCALAR(i64, ecs_i64_t), SCALAR(u64, ecs_u64_t)
		};
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t types[3];
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		types[i] = reflected(w, "Scalars", sizeof(struct scalars),
		_Alignof(struct scalars), members, 8);
		}
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		connect_worlds(&f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		struct scalars values[] = {
		{INT8_MIN, UINT8_MAX, INT16_MIN, UINT16_MAX,
		INT32_MIN, UINT32_MAX, INT64_MIN, UINT64_MAX},
		{INT8_MAX, 0, INT16_MAX, 0, INT32_MAX, 0, INT64_MAX, 0}
		};
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		for (size_t i = 0; i < 2; i++) {
		ecs_set_id(f.server.ecs, e, types[0], sizeof(values[i]),
		&values[i]);
		capture(&f, i + 1);
		transfer(&f, 0);
		const struct scalars *r = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], network_id(&f.server, e)),
		types[1]);
		CHECK(r != NULL);
		
		VALUE(i8); VALUE(u8); VALUE(i16); VALUE(u16);
		VALUE(i32); VALUE(u32); VALUE(i64); VALUE(u64);
		
		}
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_update_limit
	# When a snapshot exceeds the configured logical-update size
	# Then Snapshot totals obey logical-update bounds just like deltas.
	Given the "core" workflow "schema_update_limit"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .limits.max_update_bytes = 1024 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		char text[2049];
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		memset(text, 'x', sizeof(text) - 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		text[sizeof(text) - 1] = 0;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_capture_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_result_t r = sync_capture(f.server.session, 1, &progress);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(progress.tick == 1 && progress.peers_queued == 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: schema_fragment_limit
	# When an update needs more frames than the fragment limit
	# Then Fragment bounds fault oversized updates instead of retaining partial
	# data.
	Given the "core" workflow "schema_fragment_limit"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .limits.max_fragments = 1 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		char *text = malloc(65536);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(text != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		memset(text, 'x', 65535);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		text[65535] = 0;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		free(text);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		sync_capture_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_result_t r = sync_capture(f.server.session, 1, &progress);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(r == SYNC_PARTIAL || r == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!progress.peers_queued);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
