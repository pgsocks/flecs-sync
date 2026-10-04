/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

TEST(replay_updates)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 2988299257u);
	rpg_health(c, RPG_ARIA, 42);
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(7, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();

	cucumber_wait(8, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(10, 116967533u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
	cucumber_reply();
	return true;
}

TEST(replay_frames)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 3938013237u);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 1103487469u);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 3429412230u);
	NEED(c->progress.frames == 1);
	cucumber_reply();

	cucumber_wait(6, 2242350858u);
	NEED(!c->progress.application.updates_applied);
	cucumber_reply();

	cucumber_wait(7, 428871174u);
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	cucumber_reply();

	cucumber_wait(8, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(10, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();
	return true;
}

TEST(replay_backpressure)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 570062099u);
	c->small_queue = true;
	cucumber_reply();

	cucumber_wait(3, 2656523917u);
	NEED(rpg_client_recreate(c, RPG_ARIA, false));
	cucumber_reply();

	cucumber_wait(4, 3313731231u);
	NEED(rpg_large_quest(c, error, size));
	cucumber_reply();

	cucumber_wait(5, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 2988299257u);
	rpg_health(c, RPG_ARIA, 42);
	cucumber_reply();

	cucumber_wait(7, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 3938013237u);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 59982349u);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(10, 2502035553u);
	file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(11, 146652097u);
	NEED(before.pending_bytes);
	cucumber_reply();

	cucumber_wait(12, 1661114450u);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(13, 4038061338u);
	file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(14, 1708784755u);
	NEED(!c->progress.frames && !c->progress.bytes);
	cucumber_reply();

	cucumber_wait(15, 2644088558u);
	NEED(!memcmp(&before, &after, sizeof(before)));
	cucumber_reply();

	cucumber_wait(16, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(18, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();

	cucumber_wait(19, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(20, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(21, 116967533u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
	cucumber_reply();

	cucumber_wait(22, 2323491759u);
	NEED(rpg_expect_large_quest(c, error, size));
	cucumber_reply();
	return true;
}

TEST(replay_fragments)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 3313731231u);
	NEED(rpg_large_quest(c, error, size));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 808489624u);
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
	cucumber_reply();

	cucumber_wait(7, 1664600299u);
	NEED(e);
	cucumber_reply();

	cucumber_wait(8, 28591961u);
	const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
	cucumber_reply();

	cucumber_wait(9, 1743528091u);
	NEED(d && d->quest && !strcmp(d->quest, "Find the lost relic"));
	cucumber_reply();

	cucumber_wait(10, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(12, 2323491759u);
	NEED(rpg_expect_large_quest(c, error, size));
	cucumber_reply();
	return true;
}

TEST(replay_eof)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 3938013237u);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 59982349u);
	RESULT(file_sync_reader_pump(
		       c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(5, 853074160u);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
	cucumber_reply();

	cucumber_wait(6, 3716535892u);
	RESULT(sync_apply(c->clients[RPG_ARIA].session, UINT32_MAX, NULL),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 4167474622u);
	RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
	cucumber_reply();

	cucumber_wait(8, 2452171949u);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	cucumber_reply();

	cucumber_wait(9, 2764309725u);
	NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
	cucumber_reply();
	return true;
}

TEST(replay_reset)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 2531866366u);
	sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], &c->error);
	cucumber_reply();

	cucumber_wait(5, 4005419691u);
	c->readers[RPG_ARIA] = NULL;
	cucumber_reply();

	cucumber_wait(6, 1492115262u);
	RESULT(result, SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 2764309725u);
	NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
	cucumber_reply();

	cucumber_wait(8, 1415256628u);
	file_sync_reader_t *r = (void *)1;
	cucumber_reply();

	cucumber_wait(9, 205177953u);
	file_sync_reader_desc_t desc = {.client = c->clients[RPG_ARIA].session};
	cucumber_reply();

	cucumber_wait(10, 126931420u);
	result = file_sync_reader_open(rpg_path(RPG_ARIA), &desc, &r, NULL);
	cucumber_reply();

	cucumber_wait(11, 3412357995u);
	if (!result)
		c->probe_reader = r;
	cucumber_reply();

	cucumber_wait(12, 451194884u);
	RESULT(result, SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(13, 84882343u);
	NEED(!r && rpg_replica(c, RPG_ARIA, c->actors[RPG_ARIA]));
	cucumber_reply();

	cucumber_wait(14, 1910222595u);
	RESULT(sync_client_reset(c->clients[RPG_ARIA].session), SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 1272746736u);
	for (unsigned a = 0; a < RPG_ACTORS; a++)
		NEED(!rpg_replica(c, RPG_ARIA, c->actors[a]));
	cucumber_reply();

	cucumber_wait(16, 3938013237u);
	RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 4089581819u);
	NEED(ecs_is_alive(
		c->clients[RPG_ARIA].ecs, c->clients[RPG_ARIA].local));
	cucumber_reply();
	return true;
}
