/* Private procedural script; derived from native contracts. */
#include "test.h"
#include <float.h>
#include "protocol.h"

/* Repeated import preserves every declaration and adds no systems. */
TEST(module_import_idempotent)
{
	cucumber_wait(1, 2938930055u);
	ecs_world_t *w = ecs_init();
	cucumber_reply();

	cucumber_wait(2, 3081288225u);
	int32_t systems = ecs_count_id(w, EcsSystem);
	cucumber_reply();

	cucumber_wait(3, 3919979360u);
	sync_ids_t a, b;
	cucumber_reply();

	cucumber_wait(4, 2392552092u);
	OK(sync_import(w, &a));
	cucumber_reply();

	cucumber_wait(5, 2620317042u);
	OK(sync_import(w, &b));
	cucumber_reply();

	cucumber_wait(6, 615063778u);
#define ID(field, path) CHECK(a.field == b.field); \
	CHECK(a.field != 0 && a.field == ecs_lookup(w, path))
	ID(module, "sync");
	cucumber_reply();

	cucumber_wait(7, 3648794568u);
	ID(sync, "sync.Sync");
	cucumber_reply();

	cucumber_wait(8, 218687695u);
	ID(network_id, "sync.NetworkId");
	cucumber_reply();

	cucumber_wait(9, 1710841149u);
	ID(replicate_to, "sync.ReplicateTo");
	cucumber_reply();

	cucumber_wait(10, 3144930990u);
	ID(replicate_to_all, "sync.ReplicateToAll");
	cucumber_reply();

	cucumber_wait(11, 2592943153u);
	ID(receive_phase, "sync.Receive");
	cucumber_reply();

	cucumber_wait(12, 1505605427u);
	ID(apply_phase, "sync.Apply");
	cucumber_reply();

	cucumber_wait(13, 1108865400u);
	ID(capture_phase, "sync.Capture");
	cucumber_reply();

	cucumber_wait(14, 764446057u);
	ID(send_phase, "sync.Send");
	cucumber_reply();

	cucumber_wait(15, 1682086196u);
#undef ID
	EQ(ecs_count_id(w, EcsSystem), systems);
	cucumber_reply();

	cucumber_wait(16, 3463816134u);
	CHECK(ecs_has_pair(w, a.apply_phase, EcsDependsOn, a.receive_phase));
	cucumber_reply();

	cucumber_wait(17, 2916640934u);
	CHECK(ecs_has_pair(w, a.send_phase, EcsDependsOn, a.capture_phase));
	cucumber_reply();

	cucumber_wait(18, 2309090080u);
	const EcsComponent *c = ecs_get(w, a.network_id, EcsComponent);
	cucumber_reply();

	cucumber_wait(19, 3235090255u);
	CHECK(c != NULL && c->size == sizeof(sync_network_id_t));
	cucumber_reply();

	cucumber_wait(20, 925375332u);
	EQ(ecs_fini(w), 0);
	cucumber_reply();
}

/* Import uses independent declarations in independently allocated worlds. */
TEST(module_world_isolation)
{
	cucumber_wait(1, 3927849454u);
	struct world a, b;
	cucumber_reply();

	cucumber_wait(2, 2461108116u);
	world_init(&a, false);
	cucumber_reply();

	cucumber_wait(3, 2793388864u);
	world_init(&b, true);
	cucumber_reply();

	cucumber_wait(4, 1453524947u);
	CHECK(a.position != b.position);
	cucumber_reply();

	cucumber_wait(5, 1118616894u);
	CHECK(ecs_lookup(a.ecs, "sync.Sync") == a.ids.sync);
	cucumber_reply();

	cucumber_wait(6, 3202437591u);
	CHECK(ecs_lookup(b.ecs, "sync.Sync") == b.ids.sync);
	cucumber_reply();

	cucumber_wait(7, 3538502616u);
	OK(sync_server_create(a.ecs, NULL, &a.session, NULL));
	cucumber_reply();

	cucumber_wait(8, 2875914548u);
	OK(sync_client_create(b.ecs, NULL, &b.session, NULL));
	cucumber_reply();

	cucumber_wait(9, 3400087772u);
	EQ(status(a.session, 0).role, SYNC_ROLE_SERVER);
	cucumber_reply();

	cucumber_wait(10, 4089424122u);
	EQ(status(b.session, 0).role, SYNC_ROLE_CLIENT);
	cucumber_reply();

	cucumber_wait(11, 293399173u);
	world_fini(&b);
	cucumber_reply();

	cucumber_wait(12, 322589916u);
	world_fini(&a);
	cucumber_reply();
}

/* Incompatible declarations report SCHEMA with zero output IDs. */
TEST(module_name_conflict)
{
	cucumber_wait(1, 2938930055u);
	ecs_world_t *w = ecs_init();
	cucumber_reply();

	cucumber_wait(2, 3677922168u);
	ecs_entity_init(w, &(ecs_entity_desc_t){ .name = "sync.NetworkId" });
	cucumber_reply();

	cucumber_wait(3, 4075828811u);
	sync_ids_t ids;
	cucumber_reply();

	cucumber_wait(4, 1119805693u);
	memset(&ids, 0xff, sizeof(ids));
	cucumber_reply();

	cucumber_wait(5, 2422300302u);
	EQ(sync_import(w, &ids), SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(6, 3886612785u);
	CHECK(!ids.module && !ids.network_id && !ids.sync);
	cucumber_reply();

	cucumber_wait(7, 3678024530u);
	CHECK(!ids.replicate_to && !ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(8, 3278956367u);
	CHECK(!ids.receive_phase && !ids.apply_phase);
	cucumber_reply();

	cucumber_wait(9, 1742709697u);
	CHECK(!ids.capture_phase && !ids.send_phase);
	cucumber_reply();

	cucumber_wait(10, 925375332u);
	EQ(ecs_fini(w), 0);
	cucumber_reply();
}

/* Null world and output arguments return ARGUMENT without dereferencing. */
TEST(module_import_arguments)
{
	cucumber_wait(1, 4075828811u);
	sync_ids_t ids;
	cucumber_reply();

	cucumber_wait(2, 1119805693u);
	memset(&ids, 0xff, sizeof(ids));
	cucumber_reply();

	cucumber_wait(3, 193672373u);
	EQ(sync_import(NULL, &ids), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(4, 1944631959u);
	CHECK(!ids.module && !ids.sync && !ids.network_id);
	cucumber_reply();

	cucumber_wait(5, 2938930055u);
	ecs_world_t *w = ecs_init();
	cucumber_reply();

	cucumber_wait(6, 3544615117u);
	EQ(sync_import(w, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 925375332u);
	EQ(ecs_fini(w), 0);
	cucumber_reply();
}

/* Creation has finite defaults, no peers, and no adopted client entities. */
TEST(module_default_sessions)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 540813826u);
	sync_status_t s = status(f.server.session, 0);
	cucumber_reply();

	cucumber_wait(4, 2793487463u);
	sync_status_t c = status(f.clients[0].session, 0);
	cucumber_reply();

	cucumber_wait(5, 3890023639u);
	EQ(s.state, SYNC_SERVER_READY);
	cucumber_reply();

	cucumber_wait(6, 46069884u);
	EQ(s.peers, 2);
	cucumber_reply();

	cucumber_wait(7, 1719902883u);
	EQ(s.last_tick, 0);
	cucumber_reply();

	cucumber_wait(8, 549995481u);
	EQ(s.last_sequence, 0);
	cucumber_reply();

	cucumber_wait(9, 459683004u);
	EQ(s.queued_bytes, 0);
	cucumber_reply();

	cucumber_wait(10, 2583794586u);
	EQ(c.state, SYNC_CLIENT_WAIT_MANIFEST);
	cucumber_reply();

	cucumber_wait(11, 923606112u);
	EQ(c.entities, 0);
	cucumber_reply();

	cucumber_wait(12, 2376771530u);
	EQ(c.stream_id, 0);
	cucumber_reply();

	cucumber_wait(13, 3835747497u);
	EQ(c.fault.code, SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 1651815341u);
	CHECK(memcmp(s.session_id.bytes, (uint8_t[16]){0}, 16));
	cucumber_reply();

	cucumber_wait(15, 4147234670u);
	CHECK(!memcmp(c.session_id.bytes, (uint8_t[16]){0}, 16));
	cucumber_reply();

	cucumber_wait(16, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Zero descriptors select the same manual behavior as omitted descriptors. */
TEST(module_zero_descriptors)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 81121894u);
	sync_server_desc_t s = {0};
	cucumber_reply();

	cucumber_wait(3, 1408395900u);
	sync_client_desc_t c = {0};
	cucumber_reply();

	cucumber_wait(4, 4040144974u);
	fixture_init(&f, &s, &c);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(7, 1162173616u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(8, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* A second role cannot replace the world's active session. */
TEST(module_one_session)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 2787170963u);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(4, 218480551u);
	sync_session_t *other = w.session;
	cucumber_reply();

	cucumber_wait(5, 1548599521u);
	sync_error_t error;
	cucumber_reply();

	cucumber_wait(6, 331056500u);
	EQ(sync_client_create(w.ecs, NULL, &other, &error), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(7, 3077829476u);
	CHECK(other == NULL);
	cucumber_reply();

	cucumber_wait(8, 219629224u);
	EQ(error.code, SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(9, 1135130310u);
	EQ(sync_server_create(w.ecs, NULL, &other, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(10, 3077829476u);
	CHECK(other == NULL);
	cucumber_reply();

	cucumber_wait(11, 3627373961u);
	EQ(status(w.session, 0).state, SYNC_SERVER_READY);
	cucumber_reply();

	cucumber_wait(12, 4119224676u);
	sync_session_destroy(w.session);
	cucumber_reply();

	cucumber_wait(13, 3593229881u);
	w.session = NULL;
	cucumber_reply();

	cucumber_wait(14, 4231706018u);
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(15, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Invalid schedules/rates fail before activation and permit a later create. */
TEST(module_invalid_server_options)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 3796738758u);
	sync_server_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = (sync_schedule_t)2 },
		{ .send_hz = 20 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = -1 },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = INFINITY },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = NAN },
		{ .schedule = SYNC_AUTOMATIC, .send_hz = DBL_TRUE_MIN }
	};
	cucumber_reply();

	cucumber_wait(4, 2363538045u);
	int32_t systems = ecs_count_id(w.ecs, EcsSystem);
	cucumber_reply();

	cucumber_wait(5, 929571672u);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		sync_error_t e;
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, &e),
			SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
		EQ(e.code, SYNC_ERR_ARGUMENT);
		EQ(ecs_count_id(w.ecs, EcsSystem), systems);
	}
	cucumber_reply();

	cucumber_wait(6, 2787170963u);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(7, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Client manual budgets and unknown schedules fail without activation. */
TEST(module_invalid_client_options)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 986607321u);
	sync_client_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)7 },
		{ .updates_per_progress = 1 }
	};
	cucumber_reply();

	cucumber_wait(4, 1206716638u);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_client_create(w.ecs, &invalid[i], &w.session, NULL),
			SYNC_ERR_ARGUMENT);
		CHECK(w.session == NULL);
	}
	cucumber_reply();

	cucumber_wait(5, 4231706018u);
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(6, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Creation requires output handles and clears handles on invalid worlds. */
TEST(module_creation_arguments)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 2184180024u);
	EQ(sync_server_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(4, 3765025829u);
	EQ(sync_client_create(w.ecs, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 1548599521u);
	sync_error_t error;
	cucumber_reply();

	cucumber_wait(6, 238271847u);
	EQ(sync_server_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 1482625335u);
	CHECK(w.session == NULL);
	cucumber_reply();

	cucumber_wait(8, 1326131907u);
	EQ(error.code, SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 4250979258u);
	EQ(sync_client_create(NULL, NULL, &w.session, &error),
		SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(10, 1482625335u);
	CHECK(w.session == NULL);
	cucumber_reply();

	cucumber_wait(11, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Descriptors are copied, and success clears prior diagnostic fields. */
TEST(module_copied_options)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 1644936894u);
	sync_server_desc_t desc = { .session_id.bytes = {1, 2, 3} };
	cucumber_reply();

	cucumber_wait(4, 1548599521u);
	sync_error_t error;
	cucumber_reply();

	cucumber_wait(5, 167331391u);
	memset(&error, 0xff, sizeof(error));
	cucumber_reply();

	cucumber_wait(6, 3005354384u);
	OK(sync_server_create(w.ecs, &desc, &w.session, &error));
	cucumber_reply();

	cucumber_wait(7, 217027277u);
	CHECK(!error.code && !error.peer && !error.type && !error.tick);
	cucumber_reply();

	cucumber_wait(8, 1478600946u);
	CHECK(!error.sequence && !error.network_id);
	cucumber_reply();

	cucumber_wait(9, 891535362u);
	CHECK(!error.field[0] && !error.message[0]);
	cucumber_reply();

	cucumber_wait(10, 3614021805u);
	sync_session_id_t saved = desc.session_id;
	cucumber_reply();

	cucumber_wait(11, 1340457426u);
	memset(&desc, 0xff, sizeof(desc));
	cucumber_reply();

	cucumber_wait(12, 1436666477u);
	sync_status_t s = status(w.session, 0);
	cucumber_reply();

	cucumber_wait(13, 1748953068u);
	CHECK(!memcmp(s.session_id.bytes, saved.bytes, 16));
	cucumber_reply();

	cucumber_wait(14, 1438720570u);
	OK(sync_capture(w.session, 1, NULL));
	cucumber_reply();

	cucumber_wait(15, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Wrong-role calls preserve state and initialize their outputs. */
TEST(module_role_errors)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 772107645u);
	sync_session_t *s = f.server.session, *c = f.clients[0].session;
	cucumber_reply();

	cucumber_wait(4, 3196478831u);
	ecs_entity_t peer = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(5, 3377748850u);
	EQ(sync_peer_open(c, &peer), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(6, 6331850u);
	CHECK(!peer);
	cucumber_reply();

	cucumber_wait(7, 2566194084u);
	EQ(sync_peer_close(c, f.peers[0]), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(8, 1237071712u);
	EQ(sync_set_rate(c, 20), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(9, 1887813085u);
	sync_capture_result_t cr = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(10, 493052470u);
	EQ(sync_capture(c, 1, &cr), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(11, 2232962403u);
	CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
	cucumber_reply();

	cucumber_wait(12, 1137963775u);
	sync_frame_view_t view;
	cucumber_reply();

	cucumber_wait(13, 1584131405u);
	memset(&view, 0xff, sizeof(view));
	cucumber_reply();

	cucumber_wait(14, 1578591341u);
	EQ(sync_outbox_peek(c, f.peers[0], &view), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(15, 2885502894u);
	CHECK(!view.data && !view.size);
	cucumber_reply();

	cucumber_wait(16, 4259098464u);
	EQ(sync_outbox_commit(c, f.peers[0], (sync_outbox_token_t){0}),
		SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(17, 902623069u);
	unsigned char byte = 0;
	cucumber_reply();

	cucumber_wait(18, 3703642207u);
	EQ(sync_receive(s, &byte, 1), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(19, 3262297960u);
	sync_apply_result_t ar = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(20, 4094270955u);
	EQ(sync_apply(s, 1, &ar), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(21, 3051214240u);
	CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
	cucumber_reply();

	cucumber_wait(22, 1717422768u);
	EQ(sync_client_finish(s), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(23, 3793289589u);
	EQ(sync_client_reset(s), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(24, 3812741267u);
	sync_status_t out;
	cucumber_reply();

	cucumber_wait(25, 2187364876u);
	EQ(sync_get_status(c, f.peers[0], &out), SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(26, 3878826882u);
	CHECK(!out.role && !out.state && !out.queued_bytes);
	cucumber_reply();

	cucumber_wait(27, 1517033764u);
	EQ(status(s, 0).last_tick, 0);
	cucumber_reply();

	cucumber_wait(28, 319247242u);
	EQ(status(c, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
	cucumber_reply();

	cucumber_wait(29, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Getters are repeatable and do not consume errors, queues or progress. */
TEST(module_status_is_pure)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 94390782u);
	sync_status_t a = status(f.server.session, 0);
	cucumber_reply();

	cucumber_wait(5, 3435086145u);
	sync_status_t b = status(f.server.session, 0);
	cucumber_reply();

	cucumber_wait(6, 2322184131u);
	check_status_equal(&a, &b);
	cucumber_reply();

	cucumber_wait(7, 3778490418u);
	sync_status_t p = status(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(8, 2979842038u);
	sync_status_t q = status(f.server.session, f.peers[1]);
	cucumber_reply();

	cucumber_wait(9, 2325401706u);
	CHECK(a.queued_bytes == p.queued_bytes + q.queued_bytes);
	cucumber_reply();

	cucumber_wait(10, 2430051744u);
	CHECK(a.last_sequence == 0 && p.last_sequence > 0);
	cucumber_reply();

	cucumber_wait(11, 3975148290u);
	EQ(sync_get_status(f.server.session, 0, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(12, 1964102372u);
	EQ(sync_get_status(f.server.session, f.server.local, &b),
		SYNC_ERR_NOT_FOUND);
	cucumber_reply();

	cucumber_wait(13, 253606582u);
	CHECK(!b.role && !b.queued_bytes && !b.fault.code);
	cucumber_reply();

	cucumber_wait(14, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Null destruction is harmless, and teardown permits world reuse. */
TEST(module_destroy_null)
{
	cucumber_wait(1, 3608919598u);
	sync_session_destroy(NULL);
	cucumber_reply();

	cucumber_wait(2, 3013333617u);
	CHECK(true);
	cucumber_reply();
}

/* Closing peers removes policy pairs but preserves their gameplay owners. */
TEST(module_peer_lifecycle)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 3553176945u);
	ecs_entity_t e = spawn(&f.server, 1, f.peers[0]);
	cucumber_reply();

	cucumber_wait(4, 3778490418u);
	sync_status_t p = status(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 2154646878u);
	EQ(p.state, SYNC_PEER_PENDING);
	cucumber_reply();

	cucumber_wait(6, 2490192708u);
	CHECK(p.stream_id != 0 && p.queued_bytes == 0);
	cucumber_reply();

	cucumber_wait(7, 3176110705u);
	OK(sync_peer_close(f.server.session, f.peers[0]));
	cucumber_reply();

	cucumber_wait(8, 801986897u);
	CHECK(ecs_is_alive(f.server.ecs, e));
	cucumber_reply();

	cucumber_wait(9, 2503171111u);
	CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
	cucumber_reply();

	cucumber_wait(10, 2602003460u);
	CHECK(!ecs_has_pair(f.server.ecs, e,
		f.server.ids.replicate_to, f.peers[0]));
	cucumber_reply();

	cucumber_wait(11, 1631208582u);
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	cucumber_reply();

	cucumber_wait(12, 1960739540u);
	CHECK(status(f.server.session, f.peers[0]).stream_id != p.stream_id);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Invalid live peers fail without altering queues or session status. */
TEST(module_peer_arguments)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2053397140u);
	sync_status_t before = status(f.server.session, 0);
	cucumber_reply();

	cucumber_wait(4, 1167817851u);
	EQ(sync_peer_open(f.server.session, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 137085929u);
	EQ(sync_peer_close(f.server.session, 0), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(6, 2760238342u);
	EQ(sync_peer_close(f.server.session, f.server.local),
		SYNC_ERR_NOT_FOUND);
	cucumber_reply();

	cucumber_wait(7, 458352243u);
	sync_frame_view_t v;
	cucumber_reply();

	cucumber_wait(8, 873889237u);
	EQ(sync_outbox_peek(f.server.session, f.server.local, &v),
		SYNC_ERR_NOT_FOUND);
	cucumber_reply();

	cucumber_wait(9, 1014638056u);
	CHECK(!v.data && !v.size);
	cucumber_reply();

	cucumber_wait(10, 529014627u);
	sync_status_t after = status(f.server.session, 0);
	cucumber_reply();

	cucumber_wait(11, 4118347807u);
	check_status_equal(&before, &after);
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Inconsistent frame, schema and queue limits fail before activation. */
TEST(module_inconsistent_limits)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 328647699u);
	sync_server_desc_t invalid[] = {
		{ .limits.max_frame_bytes = 1 },
		{ .limits.max_pending_bytes = 65535 },
		{ .limits.max_schema_bytes = 65536 }
	};
	cucumber_reply();

	cucumber_wait(4, 1944994884u);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
		EQ(sync_server_create(w.ecs, &invalid[i], &w.session, NULL),
			SYNC_ERR_ARGUMENT);
		CHECK(!w.session);
	}
	cucumber_reply();

	cucumber_wait(5, 2647345326u);
	sync_client_desc_t c = { .limits.max_receive_bytes = 65535 };
	cucumber_reply();

	cucumber_wait(6, 2279538255u);
	EQ(sync_client_create(w.ecs, &c, &w.session, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 2691842903u);
	CHECK(!w.session);
	cucumber_reply();

	cucumber_wait(8, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Client creation ignores server-only pending-byte and peer limits. */
TEST(module_client_ignores_server_limits)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 819211762u);
	sync_client_desc_t c = { .limits = {
		.max_pending_bytes = 1, .max_peers = 1
	} };
	cucumber_reply();

	cucumber_wait(4, 3025855656u);
	OK(sync_client_create(w.ecs, &c, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(5, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Peer count limits reject the next peer and release capacity after close. */
TEST(module_peer_limit)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 1363514254u);
	sync_server_desc_t d = { .limits.max_peers = 1 };
	cucumber_reply();

	cucumber_wait(4, 3140110907u);
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(5, 1405278748u);
	ecs_entity_t a, b = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(6, 1951728229u);
	OK(sync_peer_open(w.session, &a));
	cucumber_reply();

	cucumber_wait(7, 2260467697u);
	EQ(sync_peer_open(w.session, &b), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(8, 2126921910u);
	CHECK(b == 0);
	cucumber_reply();

	cucumber_wait(9, 3792648295u);
	OK(sync_peer_close(w.session, a));
	cucumber_reply();

	cucumber_wait(10, 1726000523u);
	OK(sync_peer_open(w.session, &b));
	cucumber_reply();

	cucumber_wait(11, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Detectable null session arguments fail consistently across the API. */
TEST(module_null_sessions)
{
	cucumber_wait(1, 4112354359u);
	ecs_entity_t entity = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(2, 856339287u);
	sync_capture_result_t capture_result = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(3, 1548670678u);
	sync_apply_result_t apply_result = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(4, 1137963775u);
	sync_frame_view_t view;
	cucumber_reply();

	cucumber_wait(5, 1636960885u);
	sync_status_t state;
	cucumber_reply();

	cucumber_wait(6, 902623069u);
	unsigned char byte = 0;
	cucumber_reply();

	cucumber_wait(7, 1449004611u);
	EQ(sync_peer_open(NULL, &entity), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(8, 2829067940u);
	CHECK(!entity);
	cucumber_reply();

	cucumber_wait(9, 2390827701u);
	EQ(sync_peer_close(NULL, 1), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(10, 948414340u);
	EQ(sync_set_rate(NULL, 20), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(11, 927602770u);
	EQ(sync_capture(NULL, 1, &capture_result), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(12, 4010064607u);
	CHECK(!capture_result.tick && !capture_result.peers_queued);
	cucumber_reply();

	cucumber_wait(13, 2792615581u);
	EQ(sync_outbox_peek(NULL, 1, &view), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(14, 2885502894u);
	CHECK(!view.data && !view.size);
	cucumber_reply();

	cucumber_wait(15, 2606161145u);
	EQ(sync_outbox_commit(NULL, 1, (sync_outbox_token_t){0}),
		SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(16, 2766674673u);
	EQ(sync_receive(NULL, &byte, 1), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(17, 41048251u);
	EQ(sync_apply(NULL, 1, &apply_result), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(18, 3616065496u);
	CHECK(!apply_result.updates_applied && !apply_result.frames_consumed);
	cucumber_reply();

	cucumber_wait(19, 3774626365u);
	EQ(sync_client_finish(NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(20, 3401497475u);
	EQ(sync_client_reset(NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(21, 923603444u);
	EQ(sync_get_status(NULL, 0, &state), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(22, 968389539u);
	CHECK(!state.role && !state.state);
	cucumber_reply();

	cucumber_wait(23, 226672347u);
	EQ(sync_lookup(NULL, 1, &entity), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(24, 2829067940u);
	CHECK(!entity);
	cucumber_reply();
}

/* A schema with no marked types still supports an empty snapshot. */
TEST(module_empty_schema)
{
	cucumber_wait(1, 4174094828u);
	struct fixture f = {0};
	cucumber_reply();

	cucumber_wait(2, 2290828774u);
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
	cucumber_reply();

	cucumber_wait(3, 1631208582u);
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	cucumber_reply();

	cucumber_wait(4, 1549430070u);
	OK(sync_peer_open(f.server.session, &f.peers[1]));
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(7, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(8, 2252433789u);
	OK(sync_client_finish(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(9, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* A zero peer limit selects the finite default of sixty-four peers. */
TEST(module_default_peer_limit)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 2787170963u);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(4, 473821242u);
	for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		OK(sync_peer_open(w.session, &peer));
		CHECK(peer != 0);
	}
	cucumber_reply();

	cucumber_wait(5, 3196478831u);
	ecs_entity_t peer = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(6, 3419132337u);
	EQ(sync_peer_open(w.session, &peer), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(7, 1947373778u);
	CHECK(peer == 0);
	cucumber_reply();

	cucumber_wait(8, 1891854889u);
	EQ(status(w.session, 0).peers, 64);
	cucumber_reply();

	cucumber_wait(9, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Immutable adapter information is copied for active and faulted sessions. */
TEST(module_session_info)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 988849720u);
	sync_server_desc_t d = {
		.schedule = SYNC_AUTOMATIC, .limits.max_peers = 3
	};
	cucumber_reply();

	cucumber_wait(4, 3140110907u);
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(5, 2534961643u);
	sync_status_t before = status(w.session, 0);
	cucumber_reply();

	cucumber_wait(6, 2283740495u);
	sync_session_info_t info;
	cucumber_reply();

	cucumber_wait(7, 929710428u);
	OK(sync_get_session_info(w.session, &info));
	cucumber_reply();

	cucumber_wait(8, 1456337426u);
	CHECK(info.world == w.ecs);
	cucumber_reply();

	cucumber_wait(9, 246908875u);
	CHECK(info.ids.module == w.ids.module);
	cucumber_reply();

	cucumber_wait(10, 1726106440u);
	CHECK(info.ids.send_phase == w.ids.send_phase);
	cucumber_reply();

	cucumber_wait(11, 9728669u);
	EQ(info.role, SYNC_ROLE_SERVER);
	cucumber_reply();

	cucumber_wait(12, 3756907892u);
	EQ(info.schedule, SYNC_AUTOMATIC);
	cucumber_reply();

	cucumber_wait(13, 858321514u);
	EQ(info.limits.max_peers, 3);
	cucumber_reply();

	cucumber_wait(14, 2231762950u);
	EQ(info.limits.max_frame_bytes, 65536);
	cucumber_reply();

	cucumber_wait(15, 3658330187u);
	info.limits.max_peers = 100;
	cucumber_reply();

	cucumber_wait(16, 929710428u);
	OK(sync_get_session_info(w.session, &info));
	cucumber_reply();

	cucumber_wait(17, 858321514u);
	EQ(info.limits.max_peers, 3);
	cucumber_reply();

	cucumber_wait(18, 2156094091u);
	sync_status_t after = status(w.session, 0);
	cucumber_reply();

	cucumber_wait(19, 4118347807u);
	check_status_equal(&before, &after);
	cucumber_reply();

	cucumber_wait(20, 950178470u);
	EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(21, 630619471u);
	CHECK(!info.world && !info.role && !info.ids.module);
	cucumber_reply();

	cucumber_wait(22, 2140806408u);
	EQ(sync_get_session_info(w.session, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(23, 4119224676u);
	sync_session_destroy(w.session);
	cucumber_reply();

	cucumber_wait(24, 3593229881u);
	w.session = NULL;
	cucumber_reply();

	cucumber_wait(25, 4231706018u);
	OK(sync_client_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(26, 2935943216u);
	EQ(sync_client_finish(w.session), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(27, 929710428u);
	OK(sync_get_session_info(w.session, &info));
	cucumber_reply();

	cucumber_wait(28, 1456337426u);
	CHECK(info.world == w.ecs);
	cucumber_reply();

	cucumber_wait(29, 1953161948u);
	EQ(info.role, SYNC_ROLE_CLIENT);
	cucumber_reply();

	cucumber_wait(30, 2035620176u);
	EQ(info.schedule, SYNC_MANUAL);
	cucumber_reply();

	cucumber_wait(31, 194087710u);
	world_fini(&w);
	cucumber_reply();
}
