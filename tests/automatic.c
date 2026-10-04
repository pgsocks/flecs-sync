#include "test.h"
#include <float.h>

/* Automatic sessions reject public manual operations without progress. */
TEST(automatic_manual_rejected)
{
	struct fixture f;
	sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC };
	sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, &s, &c);
	sync_capture_result_t cr = {99, 99, 99};
	EQ(sync_capture(f.server.session, 1, &cr), SYNC_ERR_STATE);
	CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
	sync_apply_result_t ar = {99, 99, 99};
	EQ(sync_apply(f.clients[0].session, 1, &ar), SYNC_ERR_STATE);
	CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
	fixture_fini(&f);
}

/* Default capture rate follows simulation time and starts with tick one. */
TEST(automatic_default_rate)
{
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, &d, NULL);
	CHECK(ecs_progress(f.server.ecs, 0.02f));
	EQ(status(f.server.session, 0).last_tick, 0);
	CHECK(ecs_progress(f.server.ecs, 0.04f));
	EQ(status(f.server.session, 0).last_tick, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	fixture_fini(&f);
}

/* A stalled progress captures once and retains only the interval remainder. */
TEST(automatic_stall_remainder)
{
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	fixture_init(&f, &d, NULL);
	CHECK(ecs_progress(f.server.ecs, 1.0625f));
	EQ(status(f.server.session, 0).last_tick, 1);
	CHECK(ecs_progress(f.server.ecs, 0.03125f));
	EQ(status(f.server.session, 0).last_tick, 1);
	CHECK(ecs_progress(f.server.ecs, 0.03125f));
	EQ(status(f.server.session, 0).last_tick, 2);
	fixture_fini(&f);
}

/* Rate changes clear accumulated time and do not immediately capture. */
TEST(automatic_rate_reset)
{
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 4 };
	fixture_init(&f, &d, NULL);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	OK(sync_set_rate(f.server.session, 8));
	EQ(status(f.server.session, 0).last_tick, 0);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	EQ(status(f.server.session, 0).last_tick, 0);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	EQ(status(f.server.session, 0).last_tick, 1);
	fixture_fini(&f);
}

/* Invalid rate changes preserve the previous interval and accumulator. */
TEST(automatic_invalid_rate)
{
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	fixture_init(&f, &d, NULL);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	double invalid[] = {0, -1, INFINITY, NAN, DBL_TRUE_MIN};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
		EQ(sync_set_rate(f.server.session, invalid[i]),
			SYNC_ERR_ARGUMENT);
	CHECK(ecs_progress(f.server.ecs, 0.0625f));
	EQ(status(f.server.session, 0).last_tick, 1);
	fixture_fini(&f);
}

/* Manual server rates cannot be changed through the automatic interface. */
TEST(automatic_rate_on_manual)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	EQ(sync_set_rate(f.server.session, 20), SYNC_ERR_STATE);
	fixture_fini(&f);
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
TEST(automatic_default_budget) { client_budget(0); }
/* Explicit automatic budgets allow exactly the requested update count. */
TEST(automatic_custom_budget) { client_budget(2); }

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
	struct fixture f;
	sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, &s, &c);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, EcsPostUpdate),
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }},
		.callback = writer
	});
	struct schedule_context ctx = { .f = &f };
	ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, f.server.ids.send_phase),
		.callback = send_system, .ctx = &ctx
	});
	ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0],
			f.clients[0].ids.receive_phase),
		.callback = receive_system, .ctx = &ctx
	});
	ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0], EcsPreUpdate),
		.callback = reader, .ctx = &ctx
	});
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	ctx.id = network_id(&f.server, e);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	CHECK(ctx.received && ctx.read);
	fixture_fini(&f);
}

/* Automatic setup rejects dependencies that would create a phase cycle. */
TEST(automatic_cycle_rejected)
{
	struct world w;
	world_init(&w, false);
	ecs_add_pair(w.ecs, EcsPostUpdate, EcsDependsOn, w.ids.capture_phase);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	sync_result_t r = sync_server_create(w.ecs, &d, &w.session, NULL);
	CHECK(r < 0);
	CHECK(w.session == NULL);
	world_fini(&w);
}

/* Destroyed automatic sessions leave no core systems running on progress. */
TEST(automatic_teardown)
{
	struct world w;
	world_init(&w, false);
	int32_t count = ecs_count_id(w.ecs, EcsSystem);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
	OK(sync_server_create(w.ecs, &d, &w.session, NULL));
	CHECK(ecs_count_id(w.ecs, EcsSystem) > count);
	sync_session_destroy(w.session);
	w.session = NULL;
	EQ(ecs_count_id(w.ecs, EcsSystem), count);
	CHECK(ecs_progress(w.ecs, 0.125f));
	world_fini(&w);
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
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	fixture_init(&f, &d, NULL);
	ecs_entity_t unrelated = ecs_new(f.server.ecs);
	ecs_entity_t omitted = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query.terms = {
			{ .id = EcsSystem }, { .id = unrelated }
		} });
	CHECK(omitted != 0);
	ecs_set_pipeline(f.server.ecs, omitted);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	EQ(status(f.server.session, 0).last_tick, 0);
	ecs_entity_t included = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query = {
			.terms = {
				{ .id = EcsSystem },
				{ .id = EcsPhase, .src.id = EcsCascade,
					.trav = EcsDependsOn }
			}, .order_by_callback = system_order
		} });
	CHECK(included != 0);
	ecs_set_pipeline(f.server.ecs, included);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	EQ(status(f.server.session, 0).last_tick, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	fixture_fini(&f);
}
