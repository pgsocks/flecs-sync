#include "test.h"
#include "private.h"

static uint32_t le32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
		(uint32_t)p[3] << 24;
}
static unsigned char *read_file(const char *path, size_t *size)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	unsigned char *bytes = NULL;
	if (fseek(f, 0, SEEK_END))
		goto done;
	long n = ftell(f);
	if (n < 0 || fseek(f, 0, SEEK_SET))
		goto done;
	bytes = malloc((size_t)n + 1);
	if (bytes && fread(bytes, 1, (size_t)n, f) != (size_t)n) {
		free(bytes);
		bytes = NULL;
	}
	*size = (size_t)n;
done:
	if (fclose(f)) {
		free(bytes);
		bytes = NULL;
	}
	return bytes;
}
static bool write_file(const char *path, const void *bytes, size_t size)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	bool written = fwrite(bytes, 1, size, f) == size;
	return !fclose(f) && written;
}
static size_t record_end(const unsigned char *p, size_t n, size_t offset)
{
	if (offset > n || n - offset < 8)
		return 0;
	uint32_t length = le32(p + offset);
	return length && length <= n - offset - 8 ? offset + length + 8 : 0;
}
static bool damage_file(const char *damage)
{
	size_t n = 0;
	unsigned char *p = read_file(rpg_path(RPG_ARIA), &n);
	unsigned char *other = NULL, *out = NULL;
	bool success = false;
	if (!p || n < 16)
		goto done;
	size_t first_end = record_end(p, n, 16);
	size_t second_end = record_end(p, n, first_end);
	if (!first_end || !second_end)
		goto done;
	uint32_t length = le32(p + 16);
	if (!strcmp(damage, "checksum")) {
		p[20] ^= 1;
	} else if (!strcmp(damage, "zero-length")) {
		memset(p + 16, 0, 4);
	} else if (!strcmp(damage, "oversized-length")) {
		memset(p + 16, 0xff, 4);
	} else if (!strcmp(damage, "partial-length")) {
		n = 17;
	} else if (!strcmp(damage, "partial-frame")) {
		n = 20 + length / 2;
	} else if (!strcmp(damage, "partial-checksum")) {
		n = 20 + length + 1;
	} else if (!strcmp(damage, "bad-magic")) {
		p[0] ^= 1;
	} else if (!strcmp(damage, "reserved-header")) {
		p[12] = 1;
	} else if (!strcmp(damage, "future-version")) {
		p[8] = 2;
	} else if (!strcmp(damage, "short-header")) {
		n = 15;
	} else if (!strcmp(damage, "missing-first")) {
		memmove(p + 16, p + first_end, n - first_end);
		n -= first_end - 16;
	} else if (!strcmp(damage, "skipped-record")) {
		memmove(p + first_end, p + second_end, n - second_end);
		n -= second_end - first_end;
	} else if (!strcmp(damage, "duplicate-first") ||
		!strcmp(damage, "a-late-duplicate")) {
		size_t added = first_end - 16;
		out = malloc(n + added);
		if (!out)
			goto done;
		size_t where =
			!strcmp(damage, "duplicate-first") ? first_end : n;
		memcpy(out, p, where);
		memcpy(out + where, p + 16, added);
		memcpy(out + where + added, p + where, n - where);
		success = write_file(rpg_path(RPG_ARIA), out, n + added);
		goto done;
	} else if (!strcmp(damage, "foreign-stream")) {
		size_t m = 0;
		other = read_file(rpg_path(RPG_BRAM), &m);
		if (!other)
			goto done;
		size_t start = record_end(other, m, 16);
		size_t end = record_end(other, m, start);
		if (!start || !end)
			goto done;
		size_t total = first_end + end - start + n - second_end;
		out = malloc(total);
		if (!out)
			goto done;
		memcpy(out, p, first_end);
		memcpy(out + first_end, other + start, end - start);
		memcpy(out + first_end + end - start, p + second_end,
			n - second_end);
		success = write_file(rpg_path(RPG_ARIA), out, total);
		goto done;
	} else {
		goto done;
	}
	success = write_file(rpg_path(RPG_ARIA), p, n);
done:
	free(p);
	free(other);
	free(out);
	return success;
}
static bool truncate_recording(
	struct rpg_context *c, const char *boundary, char *error, size_t size)
{
	NEED(!c->writers[RPG_ARIA]);
	size_t n = 0;
	unsigned char *p = read_file(rpg_path(RPG_ARIA), &n);
	NEED(p);
	size_t cut = 0;
	if (!strcmp(boundary, "header-only")) {
		cut = 16;
	} else {
		for (size_t offset = 16, end; offset < n; offset = end) {
			end = record_end(p, n, offset);
			if (!end)
				break;
			sync_frame_info_t info;
			if (sync_frame_inspect(
				    p + offset + 4, le32(p + offset), &info))
				break;
			if (!strcmp(boundary, "unfinished-snapshot") &&
				info.kind == SYNC_FRAME_SNAPSHOT_END) {
				cut = offset;
				break;
			}
			/* The last fragment is removed;
				* earlier ones remain valid. */
			if (!strcmp(boundary, "unfinished-delta") &&
				info.kind == SYNC_FRAME_FRAGMENT && end == n)
				cut = offset;
		}
	}
	bool written =
		cut >= 16 && cut < n && write_file(rpg_path(RPG_ARIA), p, cut);
	free(p);
	NEED(written);
	return true;
}

static bool expect_first_fault(struct rpg_context *c, char *error, size_t size)
{
	NEED(c->first_fault.detail.code == c->result);
	NEED(c->first_fault.detail.message[0]);
	NEED(c->first_fault.offset >= 16 && c->first_fault.record > 0);
	if (c->result == SYNC_ERR_IO)
		NEED(c->first_fault.system_error);
	if (c->close_released) {
		NEED(!c->writers[RPG_ARIA] && !c->peers[RPG_ARIA]);
		NEED(!ecs_is_alive(c->server.ecs, c->removed_peer));
		NEED(!memcmp(&c->error, &c->first_fault, sizeof(c->error)));
		sync_status_t server;
		RESULT(sync_get_status(c->server.session, 0, &server), SYNC_OK);
		NEED(server.peers == 2 && !server.faulted_peers);
		NEED(!server.fault.code);
		return true;
	}
	file_sync_error_t closed;
	if (c->readers[RPG_ARIA]) {
		file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
		RESULT(file_sync_reader_step(
			       c->readers[RPG_ARIA], 1, &c->progress),
			SYNC_ERR_STATE);
		file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
		NEED(!memcmp(&before, &after, sizeof(before)));
		sync_result_t result =
			file_sync_reader_close(c->readers[RPG_ARIA], &closed);
		c->readers[RPG_ARIA] = NULL;
		RESULT(result, c->result);
		sync_status_t client;
		RESULT(sync_get_status(
			       c->clients[RPG_ARIA].session, 0, &client),
			SYNC_OK);
		NEED(client.state == SYNC_CLIENT_WAIT_MANIFEST &&
			!client.entities);
		NEED(!client.fault.code);
	} else {
		file_sync_status_t before = rpg_writer_status(c, RPG_ARIA);
		RESULT(file_sync_writer_pump(
			       c->writers[RPG_ARIA], 1, &c->progress),
			SYNC_ERR_STATE);
		file_sync_status_t after = rpg_writer_status(c, RPG_ARIA);
		NEED(!memcmp(&before, &after, sizeof(before)));
		sync_result_t result =
			file_sync_writer_close(c->writers[RPG_ARIA], &closed);
		c->writers[RPG_ARIA] = NULL;
		c->peers[RPG_ARIA] = 0;
		RESULT(result, c->result);
		sync_status_t server;
		RESULT(sync_get_status(c->server.session, 0, &server), SYNC_OK);
		NEED(!server.faulted_peers && server.peers == 2);
		NEED(!server.fault.code);
	}
	NEED(!memcmp(&closed, &c->first_fault, sizeof(closed)));
	NEED(ecs_is_alive(c->server.ecs, c->actors[RPG_ARIA]));
	return true;
}

static bool missing_paths(struct rpg_context *c, char *error, size_t size)
{
	file_sync_reader_t *r = (void *)1;
	file_sync_reader_desc_t d = {.client = c->clients[RPG_ARIA].session};
	sync_status_t before, after;
	RESULT(sync_get_status(c->clients[RPG_ARIA].session, 0, &before),
		SYNC_OK);
	sync_result_t result = file_sync_reader_open(
		"missing-rpg-directory/Aria.bin", &d, &r, &c->error);
	if (!result)
		c->probe_reader = r;
	RESULT(result, SYNC_ERR_IO);
	NEED(!r && c->error.system_error &&
		c->error.detail.code == SYNC_ERR_IO);
	RESULT(sync_get_status(c->clients[RPG_ARIA].session, 0, &after),
		SYNC_OK);
	NEED(!memcmp(&before, &after, sizeof(before)));
	file_sync_writer_t *w = (void *)1;
	file_sync_writer_desc_t wd = {
		.server = c->server.session, .schedule = SYNC_AUTOMATIC};
	int32_t systems = ecs_count_id(c->server.ecs, EcsSystem);
	result = file_sync_writer_open(
		"missing-rpg-directory/Aria.bin", &wd, &w, &c->error);
	if (!result)
		c->probe_writer = w;
	RESULT(result, SYNC_ERR_IO);
	NEED(!w && c->error.system_error);
	RESULT(sync_get_status(c->server.session, 0, &after), SYNC_OK);
	NEED(after.peers == 3 && !after.faulted_peers);
	NEED(after.state == SYNC_SERVER_READY && !after.fault.code);
	NEED(ecs_count_id(c->server.ecs, EcsSystem) == systems);
	return true;
}

static bool open_allocation(struct rpg_context *c, char *error, size_t size)
{
	bool failed = false, completed = false;
	int32_t systems = ecs_count_id(c->server.ecs, EcsSystem);
	for (unsigned n = 0; n < 8; n++) {
		file_sync_writer_t *w = (void *)1;
		file_sync_writer_desc_t d = {.server = c->server.session,
			.schedule = SYNC_AUTOMATIC};
		file_sync_test_arm(FILE_SYNC_TEST_ALLOC, n);
		sync_result_t result = file_sync_writer_open(
			"open-test.bin", &d, &w, &c->error);
		if (!result)
			c->probe_writer = w;
		bool hit = file_sync_test_hit(FILE_SYNC_TEST_ALLOC);
		file_sync_test_clear();
		if (hit) {
			failed = true;
			NEED(!w && result == SYNC_ERR_NOMEM);
			NEED(c->error.detail.code == SYNC_ERR_NOMEM);
		} else {
			NEED(!result && w);
			sync_result_t closed = file_sync_writer_close(w, NULL);
			c->probe_writer = NULL;
			RESULT(closed, SYNC_OK);
			completed = true;
		}
		sync_status_t s;
		RESULT(sync_get_status(c->server.session, 0, &s), SYNC_OK);
		NEED(s.peers == 3 && !s.faulted_peers);
		NEED(ecs_count_id(c->server.ecs, EcsSystem) == systems);
		if (completed)
			break;
	}
	remove("open-test.bin");
	NEED(failed && completed);
	return true;
}

static bool zero(const void *data, size_t size)
{
	const unsigned char *bytes = data;
	for (size_t i = 0; i < size; i++)
		if (bytes[i])
			return false;
	return true;
}

static bool invalid_arguments(struct rpg_context *c, char *error, size_t size)
{
	file_sync_writer_t *w = (void *)1;
	file_sync_reader_t *r = (void *)1;
	file_sync_status_t status;
	file_sync_frame_t frame;
	file_sync_progress_t progress;
	file_sync_error_t detail;
	file_sync_status_t before = rpg_writer_status(c, RPG_ARIA);
	RESULT(file_sync_writer_open(NULL, NULL, &w, &detail),
		SYNC_ERR_ARGUMENT);
	NEED(!w && detail.detail.code == SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_open("", NULL, &w, NULL), SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_open("bad.bin", NULL, NULL, NULL),
		SYNC_ERR_ARGUMENT);
	RESULT(file_sync_reader_open(NULL, NULL, &r, &detail),
		SYNC_ERR_ARGUMENT);
	NEED(!r && detail.detail.code == SYNC_ERR_ARGUMENT);
	RESULT(file_sync_reader_open("", NULL, &r, NULL), SYNC_ERR_ARGUMENT);
	RESULT(file_sync_reader_open("bad.bin", NULL, NULL, NULL),
		SYNC_ERR_ARGUMENT);
	memset(&progress, 0xaa, sizeof(progress));
	RESULT(file_sync_writer_pump(NULL, 1, &progress), SYNC_ERR_ARGUMENT);
	NEED(zero(&progress, sizeof(progress)));
	RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 0, &progress),
		SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_write(NULL, "x", 1), SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_write(c->writers[RPG_ARIA], NULL, 1),
		SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_write(c->writers[RPG_ARIA], "x", 0),
		SYNC_ERR_ARGUMENT);
	RESULT(file_sync_writer_flush(NULL), SYNC_ERR_ARGUMENT);
	memset(&status, 0xaa, sizeof(status));
	RESULT(file_sync_writer_get_status(NULL, &status), SYNC_ERR_ARGUMENT);
	NEED(zero(&status, sizeof(status)));
	RESULT(file_sync_writer_get_status(c->writers[RPG_ARIA], NULL),
		SYNC_ERR_ARGUMENT);
	memset(&frame, 0xaa, sizeof(frame));
	RESULT(file_sync_reader_peek(NULL, &frame), SYNC_ERR_ARGUMENT);
	NEED(zero(&frame, sizeof(frame)));
	RESULT(file_sync_reader_consume(NULL), SYNC_ERR_ARGUMENT);
	memset(&progress, 0xaa, sizeof(progress));
	RESULT(file_sync_reader_pump(NULL, 1, &progress), SYNC_ERR_ARGUMENT);
	NEED(zero(&progress, sizeof(progress)));
	memset(&progress, 0xaa, sizeof(progress));
	RESULT(file_sync_reader_step(NULL, 1, &progress), SYNC_ERR_ARGUMENT);
	NEED(zero(&progress, sizeof(progress)));
	memset(&status, 0xaa, sizeof(status));
	RESULT(file_sync_reader_get_status(NULL, &status), SYNC_ERR_ARGUMENT);
	NEED(zero(&status, sizeof(status)));
	memset(&detail, 0xaa, sizeof(detail));
	RESULT(file_sync_writer_close(NULL, &detail), SYNC_OK);
	NEED(zero(&detail, sizeof(detail)));
	memset(&detail, 0xaa, sizeof(detail));
	RESULT(file_sync_reader_close(NULL, &detail), SYNC_OK);
	NEED(zero(&detail, sizeof(detail)));
	file_sync_status_t after = rpg_writer_status(c, RPG_ARIA);
	NEED(!memcmp(&before, &after, sizeof(before)));
	return true;
}

static bool invalid_options(struct rpg_context *c, char *error, size_t size)
{
	sync_status_t before, after;
	RESULT(sync_get_status(c->server.session, 0, &before), SYNC_OK);
	int32_t server_systems = ecs_count_id(c->server.ecs, EcsSystem);
	int32_t client_systems =
		ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem);
	file_sync_writer_desc_t writers[] = {{.schedule = (sync_schedule_t)2},
		{.schedule = SYNC_AUTOMATIC}, {.frames_per_progress = 1},
		{.server = c->clients[RPG_ARIA].session},
		{.server = c->server.session, .max_frame_bytes = 65537}};
	for (unsigned i = 0; i < sizeof(writers) / sizeof(*writers); i++) {
		file_sync_writer_t *w = (void *)1;
		sync_result_t result =
			file_sync_writer_open("bad.bin", &writers[i], &w, NULL);
		if (!result)
			c->probe_writer = w;
		RESULT(result, i == 3 ? SYNC_ERR_ROLE : SYNC_ERR_ARGUMENT);
		NEED(!w);
	}
	file_sync_reader_desc_t readers[] = {{.schedule = (sync_schedule_t)2},
		{.schedule = SYNC_AUTOMATIC}, {.frames_per_progress = 1},
		{.client = c->server.session},
		{.client = c->clients[RPG_ARIA].session,
			.max_frame_bytes = 65537},
		{.client = c->clients[RPG_ARIA].session,
			.schedule = SYNC_AUTOMATIC}};
	for (unsigned i = 0; i < sizeof(readers) / sizeof(*readers); i++) {
		file_sync_reader_t *r = (void *)1;
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
			: i == 5		? SYNC_ERR_STATE
						: SYNC_ERR_ARGUMENT;
		sync_result_t result =
			file_sync_reader_open("bad.bin", &readers[i], &r, NULL);
		if (!result)
			c->probe_reader = r;
		RESULT(result, expected);
		NEED(!r);
	}
	RESULT(sync_get_status(c->server.session, 0, &after), SYNC_OK);
	NEED(!memcmp(&before, &after, sizeof(before)));
	NEED(ecs_count_id(c->server.ecs, EcsSystem) == server_systems);
	NEED(ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem) ==
		client_systems);
	return true;
}

static int operation(const char *name)
{
	const char *names[] = {"read", "flush", "close", "allocate"};
	const int faults[] = {FILE_SYNC_TEST_READ, FILE_SYNC_TEST_FLUSH,
		FILE_SYNC_TEST_CLOSE, FILE_SYNC_TEST_ALLOC};
	for (unsigned i = 0; i < 4; i++)
		if (!strcmp(name, names[i]))
			return faults[i];
	return -1;
}

static bool attempt_replay(
	struct rpg_context *c, sync_result_t expected, char *error, size_t size)
{
	memset(&c->progress, 0, sizeof(c->progress));
	c->result = rpg_reader_open(c, RPG_ARIA);
	if (!c->result)
		c->result = file_sync_reader_step(
			c->readers[RPG_ARIA], UINT32_MAX, &c->progress);
	RESULT(c->result, expected);
	return true;
}

TEST(failure_records)
{
	const char *damages[] = {"checksum", "zero-length", "oversized-length",
		"partial-length", "partial-frame", "partial-checksum",
		"bad-magic", "reserved-header", "future-version",
		"short-header"};
	const sync_result_t results[] = {SYNC_ERR_PROTOCOL, SYNC_ERR_PROTOCOL,
		SYNC_ERR_LIMIT, SYNC_ERR_TRUNCATED, SYNC_ERR_TRUNCATED,
		SYNC_ERR_TRUNCATED, SYNC_ERR_PROTOCOL, SYNC_ERR_PROTOCOL,
		SYNC_ERR_UNSUPPORTED, SYNC_ERR_TRUNCATED};
	sync_result_t expected = SYNC_OK;
	for (unsigned i = 0; i < sizeof(damages) / sizeof(*damages); i++)
		if (!strcmp(example, damages[i]))
			expected = results[i];
	NEED(expected < 0);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	NEED(damage_file(example));
	NEED(attempt_replay(c, expected, error, size));
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	return true;
}

TEST(failure_eof)
{
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_large_quest(c, error, size));
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	NEED(truncate_recording(c, example, error, size));
	NEED(attempt_replay(c, SYNC_ERR_TRUNCATED, error, size));
	return true;
}

TEST(failure_sequence)
{
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_BRAM), SYNC_OK);
	NEED(damage_file(example));
	NEED(attempt_replay(c, SYNC_ERR_SEQUENCE, error, size));
	return true;
}

TEST(failure_later)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
	NEED(damage_file("a-late-duplicate"));
	NEED(attempt_replay(c, SYNC_ERR_SEQUENCE, error, size));
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
	return true;
}

TEST(failure_schema)
{
	(void)example;
	NEED(rpg_client_recreate(c, RPG_ARIA, true));
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(attempt_replay(c, SYNC_ERR_SCHEMA, error, size));
	NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
	return true;
}

TEST(failure_reference)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	ecs_entity_t e = rpg_find_stack(c, RPG_ARIA, RPG_POTION);
	NEED(e);
	rpg_policy(c, e, 1u << RPG_GM);
	RESULT(rpg_capture(c, false), SYNC_PARTIAL);
	RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_REFERENCE);
	file_sync_status_t status = rpg_writer_status(c, RPG_ARIA);
	NEED(status.state == FILE_SYNC_FAULTED);
	RESULT(status.fault.detail.code, SYNC_ERR_REFERENCE);
	sync_status_t peer;
	RESULT(sync_get_status(c->server.session, c->peers[RPG_ARIA], &peer),
		SYNC_OK);
	NEED(peer.state == SYNC_FAULTED);
	RESULT(peer.fault.code, SYNC_ERR_REFERENCE);
	RESULT(sync_get_status(c->server.session, 0, &peer), SYNC_OK);
	NEED(peer.state == SYNC_SERVER_READY && peer.faulted_peers == 1);
	RESULT(file_sync_writer_pump(
		       c->writers[RPG_GM], UINT32_MAX, &c->progress),
		SYNC_OK);
	RESULT(file_sync_writer_flush(c->writers[RPG_GM]), SYNC_OK);
	RESULT(rpg_replay(c, RPG_GM, 2), SYNC_OK);
	NEED(c->progress.application.last_tick == 2 && rpg_references(c));
	NEED(rpg_replica(c, RPG_GM, e));
	return true;
}

TEST(failure_write)
{
	NEED(!strcmp(example, "0") || !strcmp(example, "1") ||
		!strcmp(example, "2"));
	NEED(rpg_capture_queued(c, error, size));
	file_sync_test_arm(FILE_SYNC_TEST_WRITE, (size_t)(example[0] - '0'));
	c->result =
		file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress);
	bool hit = file_sync_test_hit(FILE_SYNC_TEST_WRITE);
	c->first_fault = rpg_writer_status(c, RPG_ARIA).fault;
	file_sync_test_clear();
	NEED(hit && !c->progress.frames && !c->progress.bytes);
	NEED(c->first_fault.record == 1);
	RESULT(c->result, SYNC_ERR_IO);
	NEED(rpg_expect_queued(c, error, size));
	NEED(expect_first_fault(c, error, size));
	return true;
}

TEST(failure_operation)
{
	RESULT(rpg_capture(c, true), SYNC_OK);
	int fault = operation(example);
	NEED(fault >= 0);
	if (fault == FILE_SYNC_TEST_READ || fault == FILE_SYNC_TEST_ALLOC)
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
	file_sync_test_arm((enum file_sync_test_fault)fault, 0);
	if (fault == FILE_SYNC_TEST_READ || fault == FILE_SYNC_TEST_ALLOC) {
		c->result = file_sync_reader_step(
			c->readers[RPG_ARIA], 1, &c->progress);
		c->first_fault = rpg_reader_status(c, RPG_ARIA).fault;
	} else if (fault == FILE_SYNC_TEST_FLUSH) {
		c->result = file_sync_writer_flush(c->writers[RPG_ARIA]);
		c->first_fault = rpg_writer_status(c, RPG_ARIA).fault;
	} else {
		c->result = rpg_writer_close(c, RPG_ARIA);
		c->first_fault = c->error;
		c->close_released = true;
	}
	bool hit = file_sync_test_hit((enum file_sync_test_fault)fault);
	file_sync_test_clear();
	NEED(hit);
	RESULT(c->result,
		fault == FILE_SYNC_TEST_ALLOC ? SYNC_ERR_NOMEM : SYNC_ERR_IO);
	NEED(expect_first_fault(c, error, size));
	return true;
}

TEST(failure_open)
{
	(void)example;
	NEED(missing_paths(c, error, size));
	NEED(open_allocation(c, error, size));
	return true;
}

TEST(failure_arguments)
{
	(void)example;
	NEED(invalid_arguments(c, error, size));
	NEED(invalid_options(c, error, size));
	return true;
}
