Feature: module contracts

@core
Scenario: module_import_idempotent
	# When Sync is imported twice into the same world
	# Then Repeated import preserves every declaration and adds no systems.
	Given the "core" workflow "module_import_idempotent"
	When I perform operation 1:
		"""
		ecs_world_t *w = ecs_init();
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		int32_t systems = ecs_count_id(w, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_ids_t a, b;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_import(w, &a));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_import(w, &b));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(a.field != 0 && a.field == ecs_lookup(w, path))
		ID(module, "sync");
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ID(sync, "sync.Sync");
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ID(network_id, "sync.NetworkId");
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ID(replicate_to, "sync.ReplicateTo");
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ID(replicate_to_all, "sync.ReplicateToAll");
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ID(receive_phase, "sync.Receive");
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ID(apply_phase, "sync.Apply");
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ID(capture_phase, "sync.Capture");
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ID(send_phase, "sync.Send");
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(ecs_count_id(w, EcsSystem), systems);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(ecs_has_pair(w, a.apply_phase, EcsDependsOn, a.receive_phase));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		CHECK(ecs_has_pair(w, a.send_phase, EcsDependsOn, a.capture_phase));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		const EcsComponent *c = ecs_get(w, a.network_id, EcsComponent);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		CHECK(c != NULL && c->size == sizeof(sync_network_id_t));
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(ecs_fini(w), 0);
		"""
	Then its observed values match the contract

@core
Scenario: module_world_isolation
	# When server and client sessions use different registration orders
	# Then Import uses independent declarations in independently allocated worlds.
	Given the "core" workflow "module_world_isolation"
	When I perform operation 1:
		"""
		struct world a, b;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_init(&a, false);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_init(&b, true);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(a.position != b.position);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(ecs_lookup(a.ecs, "sync.Sync") == a.ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(ecs_lookup(b.ecs, "sync.Sync") == b.ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_server_create(a.ecs, NULL, &a.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(sync_client_create(b.ecs, NULL, &b.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(a.session, 0).role, SYNC_ROLE_SERVER);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(status(b.session, 0).role, SYNC_ROLE_CLIENT);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		world_fini(&b);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		world_fini(&a);
		"""
	Then its observed values match the contract

@core
Scenario: module_name_conflict
	# When import encounters an incompatible sync.NetworkId declaration
	# Then Incompatible declarations report SCHEMA with zero output IDs.
	Given the "core" workflow "module_name_conflict"
	When I perform operation 1:
		"""
		ecs_world_t *w = ecs_init();
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		ecs_entity_init(w, &(ecs_entity_desc_t){ .name = "sync.NetworkId" });
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_ids_t ids;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		memset(&ids, 0xff, sizeof(ids));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(sync_import(w, &ids), SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(!ids.module && !ids.network_id && !ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!ids.replicate_to && !ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!ids.receive_phase && !ids.apply_phase);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(!ids.capture_phase && !ids.send_phase);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(ecs_fini(w), 0);
		"""
	Then its observed values match the contract

@core
Scenario: module_import_arguments
	# When import is called with a null world or output pointer
	# Then Null world and output arguments return ARGUMENT without dereferencing.
	Given the "core" workflow "module_import_arguments"
	When I perform operation 1:
		"""
		sync_ids_t ids;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		memset(&ids, 0xff, sizeof(ids));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		EQ(sync_import(NULL, &ids), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(!ids.module && !ids.sync && !ids.network_id);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_world_t *w = ecs_init();
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_import(w, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(ecs_fini(w), 0);
		"""
	Then its observed values match the contract

@core
Scenario: module_default_sessions
	# When sessions are created without descriptors
	# Then Creation has finite defaults, no peers, and no adopted client entities.
	Given the "core" workflow "module_default_sessions"
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
		sync_status_t s = status(f.server.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_status_t c = status(f.clients[0].session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(s.state, SYNC_SERVER_READY);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(s.peers, 2);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(s.last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(s.last_sequence, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(s.queued_bytes, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(c.state, SYNC_CLIENT_WAIT_MANIFEST);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(c.entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(c.stream_id, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(c.fault.code, SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(memcmp(s.session_id.bytes, (uint8_t[16]){0}, 16));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!memcmp(c.session_id.bytes, (uint8_t[16]){0}, 16));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_zero_descriptors
	# When sessions use zero-initialized descriptors
	# Then Zero descriptors select the same manual behavior as omitted descriptors.
	Given the "core" workflow "module_zero_descriptors"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t s = {0};
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_client_desc_t c = {0};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_init(&f, &s, &c);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_one_session
	# When another session is created before and after destruction
	# Then A second role cannot replace the world's active session.
	Given the "core" workflow "module_one_session"
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
		OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_session_t *other = w.session;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_client_create(w.ecs, NULL, &other, &error), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(other == NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(error.code, SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_server_create(w.ecs, NULL, &other, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(other == NULL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(status(w.session, 0).state, SYNC_SERVER_READY);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_session_destroy(w.session);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		w.session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_invalid_server_options
	# When server creation receives invalid schedules or rates
	# Then Invalid schedules/rates fail before activation and permit a later
	# create.
	Given the "core" workflow "module_invalid_server_options"
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
		sync_server_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = (sync_schedule_t)2 },
		{ .send_hz = 20 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = -1 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = INFINITY },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = NAN },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = DBL_TRUE_MIN }
		};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		int32_t systems = ecs_count_id(w.ecs, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		sync_error_t e;
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, &e),
		SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
		EQ(e.code, SYNC_ERR_ARGUMENT);
		EQ(ecs_count_id(w.ecs, EcsSystem), systems);
		}
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_invalid_client_options
	# When client creation receives invalid schedules or budgets
	# Then Client manual budgets and unknown schedules fail without activation.
	Given the "core" workflow "module_invalid_client_options"
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
		sync_client_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)7 },
		{ .updates_per_progress = 1 }
		};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_client_create(w.ecs, &invalid[i], &w.session, NULL),
		SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_creation_arguments
	# When creation receives null world or output pointers
	# Then Creation requires output handles and clears handles on invalid worlds.
	Given the "core" workflow "module_creation_arguments"
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
		EQ(sync_server_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_client_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_server_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(w.session == NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(error.code, SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_client_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(w.session == NULL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_copied_options
	# When the caller overwrites its descriptor after creation
	# Then Descriptors are copied, and success clears prior diagnostic fields.
	Given the "core" workflow "module_copied_options"
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
		sync_server_desc_t desc = { .session_id.bytes = {1, 2, 3} };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		memset(&error, 0xff, sizeof(error));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_server_create(w.ecs, &desc, &w.session, &error));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!error.code && !error.peer && !error.type && !error.tick);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!error.sequence && !error.network_id);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(!error.field[0] && !error.message[0]);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_session_id_t saved = desc.session_id;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		memset(&desc, 0xff, sizeof(desc));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_status_t s = status(w.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(!memcmp(s.session_id.bytes, saved.bytes, 16));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(sync_capture(w.session, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_role_errors
	# When server operations target a client and vice versa
	# Then Wrong-role calls preserve state and initialize their outputs.
	Given the "core" workflow "module_role_errors"
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
		sync_session_t *s = f.server.session, *c = f.clients[0].session;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t peer = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(sync_peer_open(c, &peer), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(!peer);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_peer_close(c, f.peers[0]), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_set_rate(c, 20), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_capture_result_t cr = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(sync_capture(c, 1, &cr), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_frame_view_t view;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		memset(&view, 0xff, sizeof(view));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(sync_outbox_peek(c, f.peers[0], &view), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!view.data && !view.size);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(sync_outbox_commit(c, f.peers[0], (sync_outbox_token_t){0}),
		SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		unsigned char byte = 0;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		EQ(sync_receive(s, &byte, 1), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		sync_apply_result_t ar = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(sync_apply(s, 1, &ar), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(sync_client_finish(s), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		EQ(sync_client_reset(s), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		sync_status_t out;
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		EQ(sync_get_status(c, f.peers[0], &out), SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		CHECK(!out.role && !out.state && !out.queued_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		EQ(status(s, 0).last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		EQ(status(c, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_status_is_pure
	# When status is queried repeatedly while frames remain queued
	# Then Getters are repeatable and do not consume errors, queues or progress.
	Given the "core" workflow "module_status_is_pure"
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
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_status_t a = status(f.server.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_status_t b = status(f.server.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		check_status_equal(&a, &b);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_status_t p = status(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		sync_status_t q = status(f.server.session, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(a.queued_bytes == p.queued_bytes + q.queued_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(a.last_sequence == 0 && p.last_sequence > 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(sync_get_status(f.server.session, 0, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(sync_get_status(f.server.session, f.server.local, &b),
		SYNC_ERR_NOT_FOUND);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(!b.role && !b.queued_bytes && !b.fault.code);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_destroy_null
	# When the null session is destroyed
	# Then Null destruction is harmless, and teardown permits world reuse.
	Given the "core" workflow "module_destroy_null"
	When I perform operation 1:
		"""
		sync_session_destroy(NULL);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		CHECK(true);
		"""
	Then its observed values match the contract

@core
Scenario: module_peer_lifecycle
	# When a pending peer is closed and a replacement is opened
	# Then Closing peers removes policy pairs but preserves their gameplay owners.
	Given the "core" workflow "module_peer_lifecycle"
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
		ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_status_t p = status(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(p.state, SYNC_PEER_PENDING);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(p.stream_id != 0 && p.queued_bytes == 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_peer_close(f.server.session, f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(ecs_is_alive(f.server.ecs, e));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(!ecs_has_pair(f.server.ecs, e,
		f.server.ids.replicate_to, f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_peer_open(f.server.session, &f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(status(f.server.session, f.peers[0]).stream_id != p.stream_id);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_peer_arguments
	# When peer operations receive zero or unrelated live entities
	# Then Invalid live peers fail without altering queues or session status.
	Given the "core" workflow "module_peer_arguments"
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
		sync_status_t before = status(f.server.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_peer_open(f.server.session, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(sync_peer_close(f.server.session, 0), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_peer_close(f.server.session, f.server.local),
		SYNC_ERR_NOT_FOUND);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_frame_view_t v;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_outbox_peek(f.server.session, f.server.local, &v),
		SYNC_ERR_NOT_FOUND);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(!v.data && !v.size);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_status_t after = status(f.server.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		check_status_equal(&before, &after);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_inconsistent_limits
	# When creation receives incompatible frame and queue bounds
	# Then Inconsistent frame, schema and queue limits fail before activation.
	Given the "core" workflow "module_inconsistent_limits"
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
		sync_server_desc_t invalid[] = {
		{ .limits.max_frame_bytes = 1 },
		{ .limits.max_pending_bytes = 65535 },
		{ .limits.max_schema_bytes = 65536 }
		};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, NULL),
		SYNC_ERR_ARGUMENT);
		CHECK(!w.session);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_client_desc_t c = { .limits.max_receive_bytes = 65535 };
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_client_create(w.ecs, &c, &w.session, NULL), SYNC_ERR_ARGUMENT);
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
Scenario: module_client_ignores_server_limits
	# When client options include small server-only limits
	# Then Client creation ignores server-only pending-byte and peer limits.
	Given the "core" workflow "module_client_ignores_server_limits"
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
		sync_client_desc_t c = { .limits = {
		.max_pending_bytes = 1, .max_peers = 1
		} };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_client_create(w.ecs, &c, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_peer_limit
	# When the peer count crosses a configured limit and a peer is closed
	# Then Peer count limits reject the next peer and release capacity after close.
	Given the "core" workflow "module_peer_limit"
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
		sync_server_desc_t d = { .limits.max_peers = 1 };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_server_create(w.ecs, &d, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t a, b = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_peer_open(w.session, &a));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_peer_open(w.session, &b), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(b == 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		OK(sync_peer_close(w.session, a));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		OK(sync_peer_open(w.session, &b));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_null_sessions
	# When each session operation receives a null session
	# Then Detectable null session arguments fail consistently across the API.
	Given the "core" workflow "module_null_sessions"
	When I perform operation 1:
		"""
		ecs_entity_t entity = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_capture_result_t capture_result = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_apply_result_t apply_result = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_frame_view_t view;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_status_t state;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		unsigned char byte = 0;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_peer_open(NULL, &entity), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!entity);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_peer_close(NULL, 1), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(sync_set_rate(NULL, 20), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(sync_capture(NULL, 1, &capture_result), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!capture_result.tick && !capture_result.peers_queued);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(sync_outbox_peek(NULL, 1, &view), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!view.data && !view.size);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(sync_outbox_commit(NULL, 1, (sync_outbox_token_t){0}),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(sync_receive(NULL, &byte, 1), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(sync_apply(NULL, 1, &apply_result), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(!apply_result.updates_applied && !apply_result.frames_consumed);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(sync_client_finish(NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(sync_client_reset(NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		EQ(sync_get_status(NULL, 0, &state), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		CHECK(!state.role && !state.state);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		EQ(sync_lookup(NULL, 1, &entity), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		CHECK(!entity);
		"""
	Then its observed values match the contract

@core
Scenario: module_empty_schema
	# When a server with no marked types sends an empty snapshot
	# Then A schema with no marked types still supports an empty snapshot.
	Given the "core" workflow "module_empty_schema"
	When I perform operation 1:
		"""
		struct fixture f = {0};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		for (unsigned i = 0; i < 3; i++) {
		struct world *w = i ? &f.clients[i - 1] : &f.server;
		w->ecs = ecs_init();
		OK(sync_import(w->ecs, &w->ids));
		w->local = ecs_new(w->ecs);
		if (i)
		OK(sync_client_create(w->ecs, NULL, &w->session, NULL));
		else
		OK(sync_server_create(w->ecs, NULL, &w->session, NULL));
		}
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		OK(sync_peer_open(f.server.session, &f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_peer_open(f.server.session, &f.peers[1]));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(sync_client_finish(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: module_default_peer_limit
	# When a server using defaults opens a sixty-fifth peer
	# Then A zero peer limit selects the finite default of sixty-four peers.
	Given the "core" workflow "module_default_peer_limit"
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
		OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		OK(sync_peer_open(w.session, &peer));
		CHECK(peer != 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t peer = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_peer_open(w.session, &peer), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(peer == 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(status(w.session, 0).peers, 64);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: module_session_info
	# When adapters query immutable session information in active or faulted states
	# Then The world, metadata IDs, schedule and normalized limits are copied
	Given the "core" workflow "module_session_info"
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
		sync_server_desc_t d = {
		.schedule = SYNC_AUTOMATIC, .limits.max_peers = 3
		};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_server_create(w.ecs, &d, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_status_t before = status(w.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		sync_session_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_get_session_info(w.session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(info.world == w.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(info.ids.module == w.ids.module);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(info.ids.send_phase == w.ids.send_phase);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(info.role, SYNC_ROLE_SERVER);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(info.schedule, SYNC_AUTOMATIC);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(info.limits.max_peers, 3);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(info.limits.max_frame_bytes, 65536);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		info.limits.max_peers = 100;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(sync_get_session_info(w.session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(info.limits.max_peers, 3);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		sync_status_t after = status(w.session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		check_status_equal(&before, &after);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		CHECK(!info.world && !info.role && !info.ids.module);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(sync_get_session_info(w.session, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		sync_session_destroy(w.session);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		w.session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		EQ(sync_client_finish(w.session), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		OK(sync_get_session_info(w.session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		CHECK(info.world == w.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		EQ(info.role, SYNC_ROLE_CLIENT);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		EQ(info.schedule, SYNC_MANUAL);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract
