/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

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
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 820423211u);
	NEED(automatic_setup(c, true, 0, error, size));
	cucumber_reply();

	cucumber_wait(3, 1267130125u);
	ecs_entity_t sys = ecs_new(c->server.ecs);
	cucumber_reply();

	cucumber_wait(4, 1679082660u);
	ecs_add_pair(c->server.ecs, sys, EcsDependsOn, EcsPostUpdate);
	cucumber_reply();

	cucumber_wait(5, 1584302932u);
	c->game_system = ecs_system_init(c->server.ecs,
		&(ecs_system_desc_t){.entity = sys,
			.callback = gameplay_write,
			.ctx = c,
			.query.terms = {{.id = c->server.types[RPG_STATS],
				.inout = EcsOut}}});
	cucumber_reply();

	cucumber_wait(6, 3558891051u);
	NEED(c->game_system);
	cucumber_reply();

	cucumber_wait(7, 1400382778u);
	NEED(server_progress(c, error, size));
	cucumber_reply();

	cucumber_wait(8, 2904614880u);
	NEED(automatic_reader(c, 0, error, size));
	cucumber_reply();

	cucumber_wait(9, 327635627u);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(10, 2203591094u);
	NEED(c->phase_seen);
	cucumber_reply();

	cucumber_wait(11, 853074160u);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
	cucumber_reply();

	cucumber_wait(12, 327635627u);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(13, 2452171949u);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	cucumber_reply();
	return true;
}

TEST(automatic_recording_budget)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 2669166959u);
	NEED(automatic_setup(c, false, 1, error, size));
	cucumber_reply();

	cucumber_wait(3, 2048373052u);
	NEED(rpg_capture_queued(c, error, size));
	cucumber_reply();

	cucumber_wait(4, 1400382778u);
	NEED(server_progress(c, error, size));
	cucumber_reply();

	cucumber_wait(5, 3793306910u);
	NEED(rpg_writer_status(c, RPG_ARIA).state == FILE_SYNC_ACTIVE);
	cucumber_reply();

	cucumber_wait(6, 2475761097u);
	NEED(rpg_writer_status(c, RPG_ARIA).frames == 1);
	cucumber_reply();

	cucumber_wait(7, 1400382778u);
	NEED(server_progress(c, error, size));
	cucumber_reply();

	cucumber_wait(8, 2446828432u);
	NEED(rpg_writer_status(c, RPG_ARIA).frames == 2);
	cucumber_reply();

	cucumber_wait(9, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();
	return true;
}

TEST(automatic_application)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 1590165425u);
	c->client_auto = true;
	cucumber_reply();

	cucumber_wait(3, 2656523917u);
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 3938013237u);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 2502035553u);
	file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(7, 2832833141u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(8, 4038061338u);
	file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(9, 2644088558u);
	NEED(!memcmp(&before, &after, sizeof(before)));
	cucumber_reply();

	cucumber_wait(10, 3175136779u);
	NEED(!c->progress.frames && !c->progress.application.updates_applied);
	cucumber_reply();

	cucumber_wait(11, 59982349u);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(12, 327635627u);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(13, 4167474622u);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
	cucumber_reply();

	cucumber_wait(14, 2452171949u);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	cucumber_reply();
	return true;
}

TEST(automatic_replay_budget)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 1590165425u);
	c->client_auto = true;
	cucumber_reply();

	cucumber_wait(3, 2656523917u);
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 713436323u);
	NEED(automatic_reader(c, 1, error, size));
	cucumber_reply();

	cucumber_wait(6, 327635627u);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(7, 2659193785u);
	file_sync_status_t status = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(8, 385957030u);
	NEED(status.state == FILE_SYNC_ACTIVE && status.frames == 1);
	cucumber_reply();

	cucumber_wait(9, 428871174u);
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	cucumber_reply();

	cucumber_wait(10, 4266157152u);
	bool ended = false;
	cucumber_reply();

	cucumber_wait(11, 2900887160u);
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
	cucumber_reply();

	cucumber_wait(12, 907219058u);
	NEED(ended && rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	cucumber_reply();

	cucumber_wait(13, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();
	return true;
}

TEST(automatic_cleanup)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 820423211u);
	NEED(automatic_setup(c, true, 0, error, size));
	cucumber_reply();

	cucumber_wait(3, 1400382778u);
	NEED(server_progress(c, error, size));
	cucumber_reply();

	cucumber_wait(4, 2904614880u);
	NEED(automatic_reader(c, 0, error, size));
	cucumber_reply();

	cucumber_wait(5, 1353462334u);
	file_sync_status_t w = rpg_writer_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(6, 966052484u);
	file_sync_status_t r = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(7, 2783956472u);
	RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(8, 348959869u);
	NEED(!c->progress.frames);
	cucumber_reply();

	cucumber_wait(9, 2883634186u);
	RESULT(file_sync_writer_write(c->writers[RPG_ARIA], "x", 1),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(10, 417325233u);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(11, 2832833141u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(12, 1762155217u);
	file_sync_frame_t frame;
	cucumber_reply();

	cucumber_wait(13, 814341053u);
	RESULT(file_sync_reader_peek(c->readers[RPG_ARIA], &frame),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(14, 1325950932u);
	NEED(!frame.data && !frame.size);
	cucumber_reply();

	cucumber_wait(15, 670598432u);
	RESULT(file_sync_reader_consume(c->readers[RPG_ARIA]), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(16, 51956207u);
	file_sync_status_t wa = rpg_writer_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(17, 2147809597u);
	file_sync_status_t ra = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(18, 1072946857u);
	NEED(!memcmp(&w, &wa, sizeof(w)) && !memcmp(&r, &ra, sizeof(r)));
	cucumber_reply();

	cucumber_wait(19, 40175152u);
	int32_t server_systems = ecs_count_id(c->server.ecs, EcsSystem);
	cucumber_reply();

	cucumber_wait(20, 2365637107u);
	int32_t client_systems =
		ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem);
	cucumber_reply();

	cucumber_wait(21, 1398474662u);
	sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], NULL);
	cucumber_reply();

	cucumber_wait(22, 4005419691u);
	c->readers[RPG_ARIA] = NULL;
	cucumber_reply();

	cucumber_wait(23, 1492115262u);
	RESULT(result, SYNC_OK);
	cucumber_reply();

	cucumber_wait(24, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(25, 2857024558u);
	NEED(ecs_count_id(c->server.ecs, EcsSystem) == server_systems - 1);
	cucumber_reply();

	cucumber_wait(26, 2472499570u);
	NEED(ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem) ==
		client_systems - 1);
	cucumber_reply();

	cucumber_wait(27, 3042114913u);
	/* Close the remaining recordings, so only core systems remain. */
	for (unsigned v = 1; v < RPG_VIEWS; v++)
		RESULT(rpg_writer_close(c, v), SYNC_OK);
	cucumber_reply();

	cucumber_wait(28, 2482463463u);
	/* The application reader and the automatic core still belong to us. */
	NEED(ecs_is_alive(c->clients[RPG_ARIA].ecs, c->downstream_system));
	cucumber_reply();

	cucumber_wait(29, 2308527057u);
	NEED(ecs_progress(c->server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(30, 327635627u);
	NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
	cucumber_reply();
	return true;
}
