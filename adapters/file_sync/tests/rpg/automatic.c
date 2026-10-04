#include "test.h"

static void gameplay_write(ecs_iter_t *it)
{
	struct rpg_stats *s = ecs_field(it, struct rpg_stats, 0);
	struct rpg_context *c = it->ctx;
	for (int32_t i = 0; i < it->count; i++)
		if (it->entities[i] == c->actors[RPG_ARIA])
			s[i].health = 42;
}
static void downstream_read(ecs_iter_t *it)
{
	struct rpg_context *c = it->ctx;
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->actors[RPG_ARIA]);
	if (!e) {
		c->phase_seen = false;
		return;
	}
	const struct rpg_stats *s =
		ecs_get_id(it->world, e, c->clients[RPG_ARIA].types[RPG_STATS]);
	c->phase_seen = s && s->health == 42;
}

static bool automatic_setup(struct rpg_context *c, bool capture,
	uint32_t budget, char *error, size_t size)
{
	c->server_auto = capture;
	c->writer_auto = true;
	c->client_auto = capture;
	c->auto_budget = budget;
	NEED(rpg_configure(c));
	return true;
}

static bool server_progress(struct rpg_context *c, char *error, size_t size)
{
	NEED(ecs_progress(c->server.ecs, 0.125f));
	rpg_identify(c);
	for (unsigned v = 0; v < RPG_VIEWS; v++)
		if (c->writers[v])
			RESULT(file_sync_writer_flush(c->writers[v]), SYNC_OK);
	return true;
}

static bool automatic_reader(
	struct rpg_context *c, uint32_t budget, char *error, size_t size)
{
	c->reader_auto = true;
	c->reader_budget = budget;
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	file_sync_status_t status = rpg_reader_status(c, RPG_ARIA);
	NEED(status.frames_per_progress == (budget ? budget : 64));
	ecs_entity_t sys = ecs_new(c->clients[RPG_ARIA].ecs);
	ecs_add_pair(c->clients[RPG_ARIA].ecs, sys, EcsDependsOn, EcsPreUpdate);
	c->downstream_system = ecs_system_init(c->clients[RPG_ARIA].ecs,
		&(ecs_system_desc_t){
			.entity = sys, .callback = downstream_read, .ctx = c});
	NEED(c->downstream_system);
	return true;
}

TEST(automatic_phases)
{
	(void)example;
	NEED(automatic_setup(c, true, 0, error, size));
	ecs_entity_t sys = ecs_new(c->server.ecs);
	ecs_add_pair(c->server.ecs, sys, EcsDependsOn, EcsPostUpdate);
	c->game_system = ecs_system_init(c->server.ecs,
		&(ecs_system_desc_t){.entity = sys,
			.callback = gameplay_write,
			.ctx = c,
			.query.terms = {{.id = c->server.types[RPG_STATS],
				.inout = EcsOut}}});
	NEED(c->game_system);
	NEED(server_progress(c, error, size));
	NEED(automatic_reader(c, 0, error, size));
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	NEED(c->phase_seen);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	return true;
}

TEST(automatic_recording_budget)
{
	(void)example;
	NEED(automatic_setup(c, false, 1, error, size));
	NEED(rpg_capture_queued(c, error, size));
	NEED(server_progress(c, error, size));
	NEED(rpg_writer_status(c, RPG_ARIA).state == FILE_SYNC_ACTIVE);
	NEED(rpg_writer_status(c, RPG_ARIA).frames == 1);
	NEED(server_progress(c, error, size));
	NEED(rpg_writer_status(c, RPG_ARIA).frames == 2);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	return true;
}

TEST(automatic_application)
{
	(void)example;
	c->client_auto = true;
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
	NEED(!memcmp(&before, &after, sizeof(before)));
	NEED(!c->progress.frames && !c->progress.application.updates_applied);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	return true;
}

TEST(automatic_replay_budget)
{
	(void)example;
	c->client_auto = true;
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(automatic_reader(c, 1, error, size));
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	file_sync_status_t status = rpg_reader_status(c, RPG_ARIA);
	NEED(status.state == FILE_SYNC_ACTIVE && status.frames == 1);
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	bool ended = false;
	for (unsigned i = 0; i < 32; i++) {
		file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
		NEED(after.frames >= before.frames);
		NEED(after.frames - before.frames <= after.frames_per_progress);
		if (after.state == FILE_SYNC_ENDED) {
			ended = true;
			break;
		}
		NEED(after.state != FILE_SYNC_FAULTED);
	}
	NEED(ended && rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	return true;
}

TEST(automatic_cleanup)
{
	(void)example;
	NEED(automatic_setup(c, true, 0, error, size));
	NEED(server_progress(c, error, size));
	NEED(automatic_reader(c, 0, error, size));
	file_sync_status_t w = rpg_writer_status(c, RPG_ARIA);
	file_sync_status_t r = rpg_reader_status(c, RPG_ARIA);
	RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	NEED(!c->progress.frames);
	RESULT(file_sync_writer_write(c->writers[RPG_ARIA], "x", 1),
		SYNC_ERR_STATE);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	file_sync_frame_t frame;
	RESULT(file_sync_reader_peek(c->readers[RPG_ARIA], &frame),
		SYNC_ERR_STATE);
	NEED(!frame.data && !frame.size);
	RESULT(file_sync_reader_consume(c->readers[RPG_ARIA]), SYNC_ERR_STATE);
	file_sync_status_t wa = rpg_writer_status(c, RPG_ARIA);
	file_sync_status_t ra = rpg_reader_status(c, RPG_ARIA);
	NEED(!memcmp(&w, &wa, sizeof(w)) && !memcmp(&r, &ra, sizeof(r)));
	int32_t server_systems = ecs_count_id(c->server.ecs, EcsSystem);
	int32_t client_systems =
		ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem);
	sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], NULL);
	c->readers[RPG_ARIA] = NULL;
	RESULT(result, SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	NEED(ecs_count_id(c->server.ecs, EcsSystem) == server_systems - 1);
	NEED(ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem) ==
		client_systems - 1);
	/* Close the remaining recordings, so only core systems remain. */
	for (unsigned v = 1; v < RPG_VIEWS; v++)
		RESULT(rpg_writer_close(c, v), SYNC_OK);
	/* The application reader and the automatic core still belong to us. */
	NEED(ecs_is_alive(c->clients[RPG_ARIA].ecs, c->downstream_system));
	NEED(ecs_progress(c->server.ecs, 0.125f));
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	return true;
}
