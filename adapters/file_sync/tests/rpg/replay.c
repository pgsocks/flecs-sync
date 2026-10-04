#include "test.h"

TEST(replay_updates)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_health(c, RPG_ARIA, 42);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
	return true;
}

TEST(replay_frames)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.frames == 1);
	NEED(!c->progress.application.updates_applied);
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	return true;
}

TEST(replay_backpressure)
{
	(void)example;
	c->small_queue = true;
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	NEED(rpg_large_quest(c, error, size));
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_health(c, RPG_ARIA, 42);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
	NEED(before.pending_bytes);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_BACKPRESSURE);
	file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
	NEED(!c->progress.frames && !c->progress.bytes);
	NEED(!memcmp(&before, &after, sizeof(before)));
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
	NEED(rpg_expect_large_quest(c, error, size));
	return true;
}

TEST(replay_fragments)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_large_quest(c, error, size));
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
	NEED(e);
	const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
	NEED(d && d->quest && !strcmp(d->quest, "Find the lost relic"));
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_large_quest(c, error, size));
	return true;
}

TEST(replay_eof)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
	RESULT(sync_apply(c->clients[RPG_ARIA].session, UINT32_MAX, NULL),
		SYNC_OK);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
	return true;
}

TEST(replay_reset)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], &c->error);
	c->readers[RPG_ARIA] = NULL;
	RESULT(result, SYNC_OK);
	NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
	file_sync_reader_t *r = (void *)1;
	file_sync_reader_desc_t desc = {.client = c->clients[RPG_ARIA].session};
	result = file_sync_reader_open(rpg_path(RPG_ARIA), &desc, &r, NULL);
	if (!result)
		c->probe_reader = r;
	RESULT(result, SYNC_ERR_STATE);
	NEED(!r && rpg_replica(c, RPG_ARIA, c->actors[RPG_ARIA]));
	RESULT(sync_client_reset(c->clients[RPG_ARIA].session), SYNC_OK);
	for (unsigned a = 0; a < RPG_ACTORS; a++)
		NEED(!rpg_replica(c, RPG_ARIA, c->actors[a]));
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	NEED(ecs_is_alive(
		c->clients[RPG_ARIA].ecs, c->clients[RPG_ARIA].local));
	return true;
}
