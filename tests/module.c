#include "test.h"
#include <float.h>

/* Repeated import preserves every declaration and adds no systems. */
TEST(module_import_idempotent)
{
	ecs_world_t *w = ecs_init();
	int32_t systems = ecs_count_id(w, EcsSystem);
	sync_ids_t a, b;
	OK(sync_import(w, &a));
	OK(sync_import(w, &b));
#define ID(field, path) CHECK(a.field == b.field); \
	CHECK(a.field != 0 && a.field == ecs_lookup(w, path))
	ID(module, "sync");
	ID(sync, "sync.Sync");
	ID(network_id, "sync.NetworkId");
	ID(replicate_to, "sync.ReplicateTo");
	ID(replicate_to_all, "sync.ReplicateToAll");
	ID(receive_phase, "sync.Receive");
	ID(apply_phase, "sync.Apply");
	ID(capture_phase, "sync.Capture");
	ID(send_phase, "sync.Send");
#undef ID
	EQ(ecs_count_id(w, EcsSystem), systems);
	CHECK(ecs_has_pair(w, a.apply_phase, EcsDependsOn, a.receive_phase));
	CHECK(ecs_has_pair(w, a.send_phase, EcsDependsOn, a.capture_phase));
	const EcsComponent *c = ecs_get(w, a.network_id, EcsComponent);
	CHECK(c != NULL && c->size == sizeof(sync_network_id_t));
	EQ(ecs_fini(w), 0);
}

/* Import uses independent declarations in independently allocated worlds. */
TEST(module_world_isolation)
{
	struct world a, b;
	world_init(&a, false);
	world_init(&b, true);
	CHECK(a.position != b.position);
	CHECK(ecs_lookup(a.ecs, "sync.Sync") == a.ids.sync);
	CHECK(ecs_lookup(b.ecs, "sync.Sync") == b.ids.sync);
	OK(sync_server_create(a.ecs, NULL, &a.session, NULL));
	OK(sync_client_create(b.ecs, NULL, &b.session, NULL));
	EQ(status(a.session, 0).role, SYNC_ROLE_SERVER);
	EQ(status(b.session, 0).role, SYNC_ROLE_CLIENT);
	world_fini(&b);
	world_fini(&a);
}

/* Incompatible declarations report SCHEMA with zero output IDs. */
TEST(module_name_conflict)
{
	ecs_world_t *w = ecs_init();
	ecs_entity_init(w, &(ecs_entity_desc_t){ .name = "sync.NetworkId" });
	sync_ids_t ids;
	memset(&ids, 0xff, sizeof(ids));
	EQ(sync_import(w, &ids), SYNC_ERR_SCHEMA);
	CHECK(!ids.module && !ids.network_id && !ids.sync);
	CHECK(!ids.replicate_to && !ids.replicate_to_all);
	CHECK(!ids.receive_phase && !ids.apply_phase);
	CHECK(!ids.capture_phase && !ids.send_phase);
	EQ(ecs_fini(w), 0);
}

/* Null world and output arguments return ARGUMENT without dereferencing. */
TEST(module_import_arguments)
{
	sync_ids_t ids;
	memset(&ids, 0xff, sizeof(ids));
	EQ(sync_import(NULL, &ids), SYNC_ERR_ARGUMENT);
	CHECK(!ids.module && !ids.sync && !ids.network_id);
	ecs_world_t *w = ecs_init();
	EQ(sync_import(w, NULL), SYNC_ERR_ARGUMENT);
	EQ(ecs_fini(w), 0);
}

/* Creation has finite defaults, no peers, and no adopted client entities. */
TEST(module_default_sessions)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_status_t s = status(f.server.session, 0);
	sync_status_t c = status(f.clients[0].session, 0);
	EQ(s.state, SYNC_SERVER_READY);
	EQ(s.peers, 2);
	EQ(s.last_tick, 0);
	EQ(s.last_sequence, 0);
	EQ(s.queued_bytes, 0);
	EQ(c.state, SYNC_CLIENT_WAIT_MANIFEST);
	EQ(c.entities, 0);
	EQ(c.stream_id, 0);
	EQ(c.fault.code, SYNC_OK);
	CHECK(memcmp(s.session_id.bytes, (uint8_t[16]){0}, 16));
	CHECK(!memcmp(c.session_id.bytes, (uint8_t[16]){0}, 16));
	fixture_fini(&f);
}

/* Zero descriptors select the same manual behavior as omitted descriptors. */
TEST(module_zero_descriptors)
{
	struct fixture f;
	sync_server_desc_t s = {0};
	sync_client_desc_t c = {0};
	fixture_init(&f, &s, &c);
	capture(&f, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	fixture_fini(&f);
}

/* A second role cannot replace the world's active session. */
TEST(module_one_session)
{
	struct world w;
	world_init(&w, false);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	sync_session_t *other = w.session;
	sync_error_t error;
	EQ(sync_client_create(w.ecs, NULL, &other, &error), SYNC_ERR_STATE);
	CHECK(other == NULL);
	EQ(error.code, SYNC_ERR_STATE);
	EQ(sync_server_create(w.ecs, NULL, &other, NULL), SYNC_ERR_STATE);
	CHECK(other == NULL);
	EQ(status(w.session, 0).state, SYNC_SERVER_READY);
	sync_session_destroy(w.session);
	w.session = NULL;
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	world_fini(&w);
}

/* Invalid schedules/rates fail before activation and permit a later create. */
TEST(module_invalid_server_options)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = (sync_schedule_t)2 },
		{ .send_hz = 20 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = -1 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = INFINITY },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = NAN },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = DBL_TRUE_MIN }
	};
	int32_t systems = ecs_count_id(w.ecs, EcsSystem);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		sync_error_t e;
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, &e),
			SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
		EQ(e.code, SYNC_ERR_ARGUMENT);
		EQ(ecs_count_id(w.ecs, EcsSystem), systems);
	}
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	world_fini(&w);
}

/* Client manual budgets and unknown schedules fail without activation. */
TEST(module_invalid_client_options)
{
	struct world w;
	world_init(&w, false);
	sync_client_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)7 },
		{ .updates_per_progress = 1 }
	};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_client_create(w.ecs, &invalid[i], &w.session, NULL),
			SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
	}
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	world_fini(&w);
}

/* Creation requires output handles and clears handles on invalid worlds. */
TEST(module_creation_arguments)
{
	struct world w;
	world_init(&w, false);
	EQ(sync_server_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	EQ(sync_client_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	sync_error_t error;
	EQ(sync_server_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
	CHECK(w.session == NULL);
	EQ(error.code, SYNC_ERR_ARGUMENT);
	EQ(sync_client_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
	CHECK(w.session == NULL);
	world_fini(&w);
}

/* Descriptors are copied, and success clears prior diagnostic fields. */
TEST(module_copied_options)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t desc = { .session_id.bytes = {1, 2, 3} };
	sync_error_t error;
	memset(&error, 0xff, sizeof(error));
	OK(sync_server_create(w.ecs, &desc, &w.session, &error));
	CHECK(!error.code && !error.peer && !error.type && !error.tick);
	CHECK(!error.sequence && !error.network_id);
	CHECK(!error.field[0] && !error.message[0]);
	sync_session_id_t saved = desc.session_id;
	memset(&desc, 0xff, sizeof(desc));
	sync_status_t s = status(w.session, 0);
	CHECK(!memcmp(s.session_id.bytes, saved.bytes, 16));
	OK(sync_capture(w.session, 1, NULL));
	world_fini(&w);
}

/* Wrong-role calls preserve state and initialize their outputs. */
TEST(module_role_errors)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_session_t *s = f.server.session, *c = f.clients[0].session;
	ecs_entity_t peer = UINT64_MAX;
	EQ(sync_peer_open(c, &peer), SYNC_ERR_ROLE);
	CHECK(!peer);
	EQ(sync_peer_close(c, f.peers[0]), SYNC_ERR_ROLE);
	EQ(sync_set_rate(c, 20), SYNC_ERR_ROLE);
	sync_capture_result_t cr = {99, 99, 99};
	EQ(sync_capture(c, 1, &cr), SYNC_ERR_ROLE);
	CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
	sync_frame_view_t view;
	memset(&view, 0xff, sizeof(view));
	EQ(sync_outbox_peek(c, f.peers[0], &view), SYNC_ERR_ROLE);
	CHECK(!view.data && !view.size);
	EQ(sync_outbox_commit(c, f.peers[0], (sync_outbox_token_t){0}),
		SYNC_ERR_ROLE);
	unsigned char byte = 0;
	EQ(sync_receive(s, &byte, 1), SYNC_ERR_ROLE);
	sync_apply_result_t ar = {99, 99, 99};
	EQ(sync_apply(s, 1, &ar), SYNC_ERR_ROLE);
	CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
	EQ(sync_client_finish(s), SYNC_ERR_ROLE);
	EQ(sync_client_reset(s), SYNC_ERR_ROLE);
	sync_status_t out;
	EQ(sync_get_status(c, f.peers[0], &out), SYNC_ERR_ROLE);
	CHECK(!out.role && !out.state && !out.queued_bytes);
	EQ(status(s, 0).last_tick, 0);
	EQ(status(c, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
	fixture_fini(&f);
}

/* Getters are repeatable and do not consume errors, queues or progress. */
TEST(module_status_is_pure)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	sync_status_t a = status(f.server.session, 0);
	sync_status_t b = status(f.server.session, 0);
	check_status_equal(&a, &b);
	sync_status_t p = status(f.server.session, f.peers[0]);
	sync_status_t q = status(f.server.session, f.peers[1]);
	CHECK(a.queued_bytes == p.queued_bytes + q.queued_bytes);
	CHECK(a.last_sequence == 0 && p.last_sequence > 0);
	EQ(sync_get_status(f.server.session, 0, NULL), SYNC_ERR_ARGUMENT);
	EQ(sync_get_status(f.server.session, f.server.local, &b),
		SYNC_ERR_NOT_FOUND);
	CHECK(!b.role && !b.queued_bytes && !b.fault.code);
	fixture_fini(&f);
}

/* Null destruction is harmless, and teardown permits world reuse. */
TEST(module_destroy_null)
{
	sync_session_destroy(NULL);
	CHECK(true);
}

/* Closing peers removes policy pairs but preserves their gameplay owners. */
TEST(module_peer_lifecycle)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
	sync_status_t p = status(f.server.session, f.peers[0]);
	EQ(p.state, SYNC_PEER_PENDING);
	CHECK(p.stream_id != 0 && p.queued_bytes == 0);
	OK(sync_peer_close(f.server.session, f.peers[0]));
	CHECK(ecs_is_alive(f.server.ecs, e));
	CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
	CHECK(!ecs_has_pair(f.server.ecs, e,
		f.server.ids.replicate_to, f.peers[0]));
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	CHECK(status(f.server.session, f.peers[0]).stream_id != p.stream_id);
	fixture_fini(&f);
}

/* Invalid live peers fail without altering queues or session status. */
TEST(module_peer_arguments)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_status_t before = status(f.server.session, 0);
	EQ(sync_peer_open(f.server.session, NULL), SYNC_ERR_ARGUMENT);
	EQ(sync_peer_close(f.server.session, 0), SYNC_ERR_ARGUMENT);
	EQ(sync_peer_close(f.server.session, f.server.local),
		SYNC_ERR_NOT_FOUND);
	sync_frame_view_t v;
	EQ(sync_outbox_peek(f.server.session, f.server.local, &v),
		SYNC_ERR_NOT_FOUND);
	CHECK(!v.data && !v.size);
	sync_status_t after = status(f.server.session, 0);
	check_status_equal(&before, &after);
	fixture_fini(&f);
}

/* Inconsistent frame, schema and queue limits fail before activation. */
TEST(module_inconsistent_limits)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t invalid[] = {
		{ .limits.max_frame_bytes = 1 },
		{ .limits.max_pending_bytes = 65535 },
		{ .limits.max_schema_bytes = 65536 }
	};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, NULL),
			SYNC_ERR_ARGUMENT);
		CHECK(!w.session);
	}
	sync_client_desc_t c = { .limits.max_receive_bytes = 65535 };
	EQ(sync_client_create(w.ecs, &c, &w.session, NULL), SYNC_ERR_ARGUMENT);
	CHECK(!w.session);
	world_fini(&w);
}

/* Client creation ignores server-only pending-byte and peer limits. */
TEST(module_client_ignores_server_limits)
{
	struct world w;
	world_init(&w, false);
	sync_client_desc_t c = { .limits = {
		.max_pending_bytes = 1, .max_peers = 1
	} };
	OK(sync_client_create(w.ecs, &c, &w.session, NULL));
	world_fini(&w);
}

/* Peer count limits reject the next peer and release capacity after close. */
TEST(module_peer_limit)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t d = { .limits.max_peers = 1 };
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	ecs_entity_t a, b = UINT64_MAX;
	OK(sync_peer_open(w.session, &a));
	EQ(sync_peer_open(w.session, &b), SYNC_ERR_LIMIT);
	CHECK(b == 0);
	OK(sync_peer_close(w.session, a));
	OK(sync_peer_open(w.session, &b));
	world_fini(&w);
}

/* Detectable null session arguments fail consistently across the API. */
TEST(module_null_sessions)
{
	ecs_entity_t entity = UINT64_MAX;
	sync_capture_result_t capture_result = {99, 99, 99};
	sync_apply_result_t apply_result = {99, 99, 99};
	sync_frame_view_t view;
	sync_status_t state;
	unsigned char byte = 0;
	EQ(sync_peer_open(NULL, &entity), SYNC_ERR_ARGUMENT);
	CHECK(!entity);
	EQ(sync_peer_close(NULL, 1), SYNC_ERR_ARGUMENT);
	EQ(sync_set_rate(NULL, 20), SYNC_ERR_ARGUMENT);
	EQ(sync_capture(NULL, 1, &capture_result), SYNC_ERR_ARGUMENT);
	CHECK(!capture_result.tick && !capture_result.peers_queued);
	EQ(sync_outbox_peek(NULL, 1, &view), SYNC_ERR_ARGUMENT);
	CHECK(!view.data && !view.size);
	EQ(sync_outbox_commit(NULL, 1, (sync_outbox_token_t){0}),
		SYNC_ERR_ARGUMENT);
	EQ(sync_receive(NULL, &byte, 1), SYNC_ERR_ARGUMENT);
	EQ(sync_apply(NULL, 1, &apply_result), SYNC_ERR_ARGUMENT);
	CHECK(!apply_result.updates_applied && !apply_result.frames_consumed);
	EQ(sync_client_finish(NULL), SYNC_ERR_ARGUMENT);
	EQ(sync_client_reset(NULL), SYNC_ERR_ARGUMENT);
	EQ(sync_get_status(NULL, 0, &state), SYNC_ERR_ARGUMENT);
	CHECK(!state.role && !state.state);
	EQ(sync_lookup(NULL, 1, &entity), SYNC_ERR_ARGUMENT);
	CHECK(!entity);
}

/* A schema with no marked types still supports an empty snapshot. */
TEST(module_empty_schema)
{
	struct fixture f = {0};
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
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	OK(sync_peer_open(f.server.session, &f.peers[1]));
	capture(&f, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).entities, 0);
	OK(sync_client_finish(f.clients[0].session));
	fixture_fini(&f);
}

/* A zero peer limit selects the finite default of sixty-four peers. */
TEST(module_default_peer_limit)
{
	struct world w;
	world_init(&w, false);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		OK(sync_peer_open(w.session, &peer));
		CHECK(peer != 0);
	}
	ecs_entity_t peer = UINT64_MAX;
	EQ(sync_peer_open(w.session, &peer), SYNC_ERR_LIMIT);
	CHECK(peer == 0);
	EQ(status(w.session, 0).peers, 64);
	world_fini(&w);
}

/* Immutable adapter information is copied for active and faulted sessions. */
TEST(module_session_info)
{
	struct world w;
	world_init(&w, false);
	sync_server_desc_t d = {
		.schedule = SYNC_AUTOMATIC, .limits.max_peers = 3
	};
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	sync_status_t before = status(w.session, 0);
	sync_session_info_t info;
	OK(sync_get_session_info(w.session, &info));
	CHECK(info.world == w.ecs);
	CHECK(info.ids.module == w.ids.module);
	CHECK(info.ids.send_phase == w.ids.send_phase);
	EQ(info.role, SYNC_ROLE_SERVER);
	EQ(info.schedule, SYNC_AUTOMATIC);
	EQ(info.limits.max_peers, 3);
	EQ(info.limits.max_frame_bytes, 65536);
	info.limits.max_peers = 100;
	OK(sync_get_session_info(w.session, &info));
	EQ(info.limits.max_peers, 3);
	sync_status_t after = status(w.session, 0);
	check_status_equal(&before, &after);
	EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
	CHECK(!info.world && !info.role && !info.ids.module);
	EQ(sync_get_session_info(w.session, NULL), SYNC_ERR_ARGUMENT);
	sync_session_destroy(w.session);
	w.session = NULL;
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	EQ(sync_client_finish(w.session), SYNC_ERR_TRUNCATED);
	OK(sync_get_session_info(w.session, &info));
	CHECK(info.world == w.ecs);
	EQ(info.role, SYNC_ROLE_CLIENT);
	EQ(info.schedule, SYNC_MANUAL);
	world_fini(&w);
}
