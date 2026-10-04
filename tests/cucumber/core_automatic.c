/* Private procedural script; derived from native contracts. */
#include "test.h"
#include <float.h>
#include "protocol.h"

/* Automatic sessions reject public manual operations without progress. */
TEST(automatic_manual_rejected)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1213112969u);
	sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(3, 1025173293u);
	sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(4, 4040144974u);
	fixture_init(&f, &s, &c);
	cucumber_reply();

	cucumber_wait(5, 1887813085u);
	sync_capture_result_t cr = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(6, 2820440762u);
	EQ(sync_capture(f.server.session, 1, &cr), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(7, 2232962403u);
	CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
	cucumber_reply();

	cucumber_wait(8, 3262297960u);
	sync_apply_result_t ar = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(9, 2338154932u);
	EQ(sync_apply(f.clients[0].session, 1, &ar), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(10, 3051214240u);
	CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Default capture rate follows simulation time and starts with tick one. */
TEST(automatic_default_rate)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 83725177u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 4178138220u);
	CHECK(ecs_progress(f.server.ecs, 0.02f));
	cucumber_reply();

	cucumber_wait(5, 3985373966u);
	EQ(status(f.server.session, 0).last_tick, 0);
	cucumber_reply();

	cucumber_wait(6, 1984537036u);
	CHECK(ecs_progress(f.server.ecs, 0.04f));
	cucumber_reply();

	cucumber_wait(7, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(8, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(9, 1162173616u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(10, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* A stalled progress captures once and retains only the interval remainder. */
TEST(automatic_stall_remainder)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1030511650u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 3834473541u);
	CHECK(ecs_progress(f.server.ecs, 1.0625f));
	cucumber_reply();

	cucumber_wait(5, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(6, 82331896u);
	CHECK(ecs_progress(f.server.ecs, 0.03125f));
	cucumber_reply();

	cucumber_wait(7, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(8, 82331896u);
	CHECK(ecs_progress(f.server.ecs, 0.03125f));
	cucumber_reply();

	cucumber_wait(9, 3993972576u);
	EQ(status(f.server.session, 0).last_tick, 2);
	cucumber_reply();

	cucumber_wait(10, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Rate changes clear accumulated time and do not immediately capture. */
TEST(automatic_rate_reset)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2008736666u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 4 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(5, 3798709696u);
	OK(sync_set_rate(f.server.session, 8));
	cucumber_reply();

	cucumber_wait(6, 3985373966u);
	EQ(status(f.server.session, 0).last_tick, 0);
	cucumber_reply();

	cucumber_wait(7, 189728635u);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	cucumber_reply();

	cucumber_wait(8, 3985373966u);
	EQ(status(f.server.session, 0).last_tick, 0);
	cucumber_reply();

	cucumber_wait(9, 189728635u);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	cucumber_reply();

	cucumber_wait(10, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Invalid rate changes preserve the previous interval and accumulator. */
TEST(automatic_invalid_rate)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1030511650u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 189728635u);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	cucumber_reply();

	cucumber_wait(5, 1541083043u);
	double invalid[] = {0, -1, INFINITY, NAN, DBL_TRUE_MIN};
	cucumber_reply();

	cucumber_wait(6, 1994440002u);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
		EQ(sync_set_rate(f.server.session, invalid[i]),
			SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 189728635u);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	cucumber_reply();

	cucumber_wait(8, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(9, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Manual server rates cannot be changed through the automatic interface. */
TEST(automatic_rate_on_manual)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2531142256u);
	EQ(sync_set_rate(f.server.session, 20), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(4, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

static void client_budget(uint32_t budget)
{
	struct fixture f;
	sync_client_desc_t d = { .schedule = SYNC_AUTOMATIC,
		.updates_per_progress = budget };
	fixture_init(&f, NULL, &d);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	for (uint64_t tick = 1; tick <= 3; tick++) {
		set_position(&f.server, e, (float)tick);
		capture(&f, tick);
	}
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	uint64_t expected = budget ? budget : 1;
	EQ(status(f.clients[0].session, 0).last_tick, expected);
	expect_position(&f.clients[0], network_id(&f.server, e),
		(float)expected);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	EQ(status(f.clients[0].session, 0).last_tick, budget ? 3 : 2);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Default automatic application publishes one complete update per progress. */
TEST(automatic_default_budget) {
	cucumber_wait(1, 3867350946u);
	client_budget(0);
	cucumber_reply();
}
/* Explicit automatic budgets allow exactly the requested update count. */
TEST(automatic_custom_budget) {
	cucumber_wait(1, 3842491340u);
	client_budget(2);
	cucumber_reply();
}

static void writer(ecs_iter_t *it)
{
	struct position *p = ecs_field(it, struct position, 0);
	for (int32_t i = 0; i < it->count; i++) {
		p[i].x = 42;
		p[i].y = 43;
	}
}

struct schedule_context {
	struct fixture *f;
	uint64_t id;
	bool sent, received, read;
};

static void send_system(ecs_iter_t *it)
{
	struct schedule_context *ctx = it->ctx;
	sync_frame_view_t frame;
	OK(sync_outbox_peek(ctx->f->server.session, ctx->f->peers[0], &frame));
	CHECK(frame.info.tick == 1);
	ctx->sent = true;
}

static void receive_system(ecs_iter_t *it)
{
	struct schedule_context *ctx = it->ctx;
	struct fixture *f = ctx->f;
	CHECK(ctx->sent);
	struct packets p = take_packets(f->server.session, f->peers[0]);
	CHECK(p.count > 0);
	receive_packets(f->clients[0].session, &p);
	packets_fini(&p);
	ctx->received = true;
}

static void reader(ecs_iter_t *it)
{
	struct schedule_context *ctx = it->ctx;
	CHECK(ctx->received);
	expect_position(&ctx->f->clients[0], ctx->id, 42);
	ctx->read = true;
}

static ecs_entity_t system_entity(struct world *w, ecs_entity_t phase)
{
	ecs_entity_t e = ecs_new(w->ecs);
	ecs_add_pair(w->ecs, e, EcsDependsOn, phase);
	return e;
}

/* Writers precede capture; receive precedes apply; readers see full state. */
TEST(automatic_phase_order)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3216589220u);
	sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1025173293u);
	sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(4, 4040144974u);
	fixture_init(&f, &s, &c);
	cucumber_reply();

	cucumber_wait(5, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(6, 1014965562u);
	ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, EcsPostUpdate),
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }},
		.callback = writer
	});
	cucumber_reply();

	cucumber_wait(7, 3084290807u);
	struct schedule_context ctx = { .f = &f };
	cucumber_reply();

	cucumber_wait(8, 2936582898u);
	ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, f.server.ids.send_phase),
		.callback = send_system, .ctx = &ctx
	});
	cucumber_reply();

	cucumber_wait(9, 611244110u);
	ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0],
			f.clients[0].ids.receive_phase),
		.callback = receive_system, .ctx = &ctx
	});
	cucumber_reply();

	cucumber_wait(10, 2827371813u);
	ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0], EcsPreUpdate),
		.callback = reader, .ctx = &ctx
	});
	cucumber_reply();

	cucumber_wait(11, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(12, 4194897072u);
	ctx.id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(13, 1052341725u);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(14, 328270663u);
	CHECK(ctx.received && ctx.read);
	cucumber_reply();

	cucumber_wait(15, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Automatic setup rejects dependencies that would create a phase cycle. */
TEST(automatic_cycle_rejected)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 3191133831u);
	ecs_add_pair(w.ecs, EcsPostUpdate, EcsDependsOn, w.ids.capture_phase);
	cucumber_reply();

	cucumber_wait(4, 83725177u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(5, 821420515u);
	sync_result_t r = sync_server_create(w.ecs, &d, &w.session, NULL);
	cucumber_reply();

	cucumber_wait(6, 4093917708u);
	CHECK(r < 0);
	cucumber_reply();

	cucumber_wait(7, 1482625335u);
	CHECK(w.session == NULL);
	cucumber_reply();

	cucumber_wait(8, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Destroyed automatic sessions leave no core systems running on progress. */
TEST(automatic_teardown)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 218485289u);
	int32_t count = ecs_count_id(w.ecs, EcsSystem);
	cucumber_reply();

	cucumber_wait(4, 83725177u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(5, 3140110907u);
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(6, 2437173019u);
	CHECK(ecs_count_id(w.ecs, EcsSystem) > count);
	cucumber_reply();

	cucumber_wait(7, 4119224676u);
	sync_session_destroy(w.session);
	cucumber_reply();

	cucumber_wait(8, 3593229881u);
	w.session = NULL;
	cucumber_reply();

	cucumber_wait(9, 1371882138u);
	EQ(ecs_count_id(w.ecs, EcsSystem), count);
	cucumber_reply();

	cucumber_wait(10, 1069000827u);
	CHECK(ecs_progress(w.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(11, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

static int system_order(ecs_entity_t a, const void *ap,
	ecs_entity_t b, const void *bp)
{
	(void)ap;
	(void)bp;
	return (a > b) - (a < b);
}

/* Custom pipelines must select Sync systems and preserve phase ordering. */
TEST(automatic_custom_pipeline)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1030511650u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 1262417049u);
	ecs_entity_t unrelated = ecs_new(f.server.ecs);
	cucumber_reply();

	cucumber_wait(5, 3729610922u);
	ecs_entity_t omitted = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query.terms = {
			{ .id = EcsSystem }, { .id = unrelated }
		} });
	cucumber_reply();

	cucumber_wait(6, 1290178899u);
	CHECK(omitted != 0);
	cucumber_reply();

	cucumber_wait(7, 1357675216u);
	ecs_set_pipeline(f.server.ecs, omitted);
	cucumber_reply();

	cucumber_wait(8, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(9, 3985373966u);
	EQ(status(f.server.session, 0).last_tick, 0);
	cucumber_reply();

	cucumber_wait(10, 2258666189u);
	ecs_entity_t included = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query = {
			.terms = {
				{ .id = EcsSystem },
				{ .id = EcsPhase, .src.id = EcsCascade,
					.trav = EcsDependsOn }
			}, .order_by_callback = system_order
		} });
	cucumber_reply();

	cucumber_wait(11, 438215941u);
	CHECK(included != 0);
	cucumber_reply();

	cucumber_wait(12, 1524517332u);
	ecs_set_pipeline(f.server.ecs, included);
	cucumber_reply();

	cucumber_wait(13, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(14, 3964244281u);
	EQ(status(f.server.session, 0).last_tick, 1);
	cucumber_reply();

	cucumber_wait(15, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(16, 1162173616u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}
