/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

TEST(recording_views)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 229429122u);
	NEED(rpg_replay_all(c, 1, error, size));
	cucumber_reply();

	cucumber_wait(4, 525259181u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_POTION, 5, error, size));
	cucumber_reply();

	cucumber_wait(5, 3860732282u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_BRAM, RPG_POTION, -1, error, size));
	cucumber_reply();

	cucumber_wait(6, 998078598u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, -1, error, size));
	cucumber_reply();

	cucumber_wait(7, 3972134537u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_BRAM, RPG_SWORD, 1, error, size));
	cucumber_reply();

	cucumber_wait(8, 1496546758u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_ARROW, -1, error, size));
	cucumber_reply();

	cucumber_wait(9, 1703628343u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_MIRA, RPG_POTION, 12, error, size));
	cucumber_reply();

	cucumber_wait(10, 2946570722u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_SKULK, RPG_COIN, 7, error, size));
	cucumber_reply();

	cucumber_wait(11, 1899146152u);
	NEED(rpg_expect_stats(c, error, size));
	cucumber_reply();

	cucumber_wait(12, 373478231u);
	NEED(rpg_expect_private(c, error, size));
	cucumber_reply();

	cucumber_wait(13, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(recording_late_start)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 2812400843u);
	rpg_health(c, RPG_ARIA, 76);
	cucumber_reply();

	cucumber_wait(5, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 104355150u);
	RESULT(rpg_writer_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 3031263892u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 76, 3, error, size));
	cucumber_reply();

	cucumber_wait(10, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

static bool recording_delivered_frames(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		RESULT(file_sync_reader_open(
			       rpg_path(v), NULL, &c->probe_reader, NULL),
			SYNC_OK);
		file_sync_reader_t *r = c->probe_reader;
		uint64_t frames = 0;
		size_t bytes = 0;
		bool equal = true;
		file_sync_frame_t f;
		sync_result_t result;
		while (!(result = file_sync_reader_peek(r, &f))) {
			if (bytes + f.size > c->delivered_bytes[v] ||
				memcmp(f.data, c->delivered[v] + bytes, f.size))
				equal = false;
			bytes += f.size;
			frames++;
			if (file_sync_reader_consume(r)) {
				equal = false;
				break;
			}
		}
		sync_result_t closed = file_sync_reader_close(r, NULL);
		c->probe_reader = NULL;
		NEED(equal && result == SYNC_END && !closed);
		NEED(frames == c->delivered_frames[v]);
		NEED(bytes == c->delivered_bytes[v] && frames > 0);
		sync_status_t live;
		RESULT(sync_get_status(c->live[v].session, 0, &live), SYNC_OK);
		NEED(live.last_tick == c->tick && live.entities > 0);
	}
	return true;
}

static bool recording_raw_peek(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	RESULT(file_sync_reader_open(
		       rpg_path(RPG_ARIA), NULL, &c->readers[RPG_ARIA], NULL),
		SYNC_OK);
	file_sync_reader_t *r = c->readers[RPG_ARIA];
	file_sync_frame_t first, second;
	RESULT(file_sync_reader_peek(r, &first), SYNC_OK);
	file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
	RESULT(file_sync_reader_peek(r, &second), SYNC_OK);
	file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
	NEED(first.data == second.data && first.size == second.size);
	NEED(first.info.kind == SYNC_FRAME_MANIFEST);
	NEED(!memcmp(&before, &after, sizeof(before)));
	RESULT(file_sync_reader_consume(r), SYNC_OK);
	RESULT(file_sync_reader_consume(r), SYNC_EMPTY);
	return true;
}

static bool recording_raw_end(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	file_sync_reader_t *r = c->readers[RPG_ARIA];
	file_sync_frame_t f;
	sync_result_t result;
	while (!(result = file_sync_reader_peek(r, &f)))
		RESULT(file_sync_reader_consume(r), SYNC_OK);
	NEED(result == SYNC_END && !f.data && !f.size);
	NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
	sync_status_t s;
	RESULT(sync_get_status(c->clients[RPG_ARIA].session, 0, &s), SYNC_OK);
	NEED(!s.entities && s.state == SYNC_CLIENT_WAIT_MANIFEST);
	return true;
}

static bool recording_copied_options(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	file_sync_writer_desc_t d = {
		.server = c->server.session, .max_frame_bytes = 32768};
	char path[64];
	snprintf(path, sizeof(path), "%s", rpg_path(RPG_ARIA));
	RESULT(file_sync_writer_open(path, &d, &c->writers[RPG_ARIA], NULL),
		SYNC_OK);
	d.server = NULL;
	d.max_frame_bytes = 1;
	memset(path, 'X', sizeof(path));
	file_sync_status_t s = rpg_writer_status(c, RPG_ARIA);
	NEED(s.peer && s.max_frame_bytes == 32768 && s.schedule == SYNC_MANUAL);
	c->peers[RPG_ARIA] = s.peer;
	sync_session_info_t info;
	RESULT(sync_get_session_info(c->server.session, &info), SYNC_OK);
	NEED(info.world == c->server.ecs &&
		info.ids.sync == c->server.ids.sync);
	NEED(info.role == SYNC_ROLE_SERVER && info.schedule == SYNC_MANUAL);
	info.limits.max_frame_bytes = 1;
	RESULT(sync_get_session_info(c->server.session, &info), SYNC_OK);
	NEED(info.limits.max_frame_bytes == 65536);
	RESULT(file_sync_writer_flush(c->writers[RPG_ARIA]), SYNC_OK);
	file_sync_reader_desc_t rd = {.client = c->clients[RPG_ARIA].session,
		.max_frame_bytes = 32768};
	snprintf(path, sizeof(path), "%s", rpg_path(RPG_ARIA));
	RESULT(file_sync_reader_open(path, &rd, &c->readers[RPG_ARIA], NULL),
		SYNC_OK);
	rd.client = NULL;
	rd.max_frame_bytes = 1;
	memset(path, 'X', sizeof(path));
	s = rpg_reader_status(c, RPG_ARIA);
	NEED(s.max_frame_bytes == 32768 && s.schedule == SYNC_MANUAL);
	NEED(!s.peer && s.state == FILE_SYNC_ACTIVE);
	return true;
}

static bool recording_pure_status(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	file_sync_status_t w1 = rpg_writer_status(c, RPG_ARIA);
	file_sync_status_t w2 = rpg_writer_status(c, RPG_ARIA);
	NEED(!memcmp(&w1, &w2, sizeof(w1)));
	NEED(w1.offset == 16 && !w1.frames && !w1.bytes);
	file_sync_status_t r1 = rpg_reader_status(c, RPG_ARIA);
	file_sync_status_t r2 = rpg_reader_status(c, RPG_ARIA);
	NEED(!memcmp(&r1, &r2, sizeof(r1)));
	NEED(r1.offset == 16 && !r1.frames && !r1.bytes && !r1.pending_bytes);
	return true;
}

static bool recording_peer_gone(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)example;
	NEED(c->removed_peer && !ecs_is_alive(c->server.ecs, c->removed_peer));
	for (size_t i = 0; i < c->entity_count; i++) {
		ecs_entity_t e = c->entities[i].entity;
		NEED(ecs_is_alive(c->server.ecs, e));
		NEED(!ecs_has_pair(c->server.ecs, e, c->server.ids.replicate_to,
			c->removed_peer));
	}
	NEED(ecs_is_alive(c->server.ecs, c->peers[RPG_BRAM]));
	sync_status_t s;
	RESULT(sync_get_status(c->server.session, 0, &s), SYNC_OK);
	NEED(s.peers == 2 && s.state == SYNC_SERVER_READY);
	return true;
}

static bool recording_header_only(
	struct rpg_context *c, const char *example, char *error, size_t size)
{
	(void)c;
	(void)example;
	FILE *f = fopen(rpg_path(RPG_ARIA), "rb");
	NEED(f);
	unsigned char bytes[17];
	size_t n = fread(bytes, 1, sizeof(bytes), f);
	int closed = fclose(f);
	NEED(n == 16 && !closed);
	NEED(!memcmp(bytes, "FILESYNC", 8));
	NEED(bytes[8] == 1 && !bytes[9] && !bytes[10] && !bytes[11]);
	NEED(!bytes[12] && !bytes[13] && !bytes[14] && !bytes[15]);
	return true;
}

TEST(recording_delivery)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 1869399221u);
	NEED(rpg_raw_setup(c));
	cucumber_reply();

	cucumber_wait(3, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 229429122u);
	NEED(rpg_replay_all(c, 1, error, size));
	cucumber_reply();

	cucumber_wait(5, 2577980119u);
	NEED(recording_delivered_frames(c, example, error, size));
	cucumber_reply();

	cucumber_wait(6, 373478231u);
	NEED(rpg_expect_private(c, error, size));
	cucumber_reply();
	return true;
}

TEST(recording_inspection)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 1835522377u);
	NEED(recording_raw_peek(c, example, error, size));
	cucumber_reply();

	cucumber_wait(5, 4153465573u);
	NEED(recording_raw_end(c, example, error, size));
	cucumber_reply();
	return true;
}

TEST(recording_options)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 2732467269u);
	NEED(recording_copied_options(c, example, error, size));
	cucumber_reply();

	cucumber_wait(3, 788801315u);
	NEED(recording_pure_status(c, example, error, size));
	cucumber_reply();
	return true;
}

TEST(recording_flush)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 2048373052u);
	NEED(rpg_capture_queued(c, error, size));
	cucumber_reply();

	cucumber_wait(3, 1244740706u);
	RESULT(file_sync_writer_flush(c->writers[RPG_ARIA]), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 304268859u);
	NEED(rpg_expect_queued(c, error, size));
	cucumber_reply();

	cucumber_wait(5, 2965155689u);
	file_sync_status_t status = rpg_writer_status(c, RPG_ARIA);
	cucumber_reply();

	cucumber_wait(6, 3327939179u);
	NEED(status.state == FILE_SYNC_ACTIVE && !status.frames);
	cucumber_reply();
	return true;
}

TEST(recording_close)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 2048373052u);
	NEED(rpg_capture_queued(c, error, size));
	cucumber_reply();

	cucumber_wait(3, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 1672346833u);
	RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1061088850u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	cucumber_reply();

	cucumber_wait(6, 2331532807u);
	NEED(recording_peer_gone(c, example, error, size));
	cucumber_reply();
	return true;
}

TEST(recording_replace)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(4, 104355150u);
	RESULT(rpg_writer_open(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1660713771u);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 3753990545u);
	NEED(recording_header_only(c, example, error, size));
	cucumber_reply();
	return true;
}
