#include "test.h"
#include <file_sync.h>
#ifdef FILE_SYNC_TESTING
#include "private.h"
#endif

static const char *path = "recording.bin";

static file_sync_status_t writer_status(file_sync_writer_t *w)
{
	file_sync_status_t s;
	OK(file_sync_writer_get_status(w, &s));
	return s;
}

static file_sync_status_t reader_status(file_sync_reader_t *r)
{
	file_sync_status_t s;
	OK(file_sync_reader_get_status(r, &s));
	return s;
}

static file_sync_writer_t *writer(struct fixture *f)
{
	file_sync_writer_t *w;
	file_sync_writer_desc_t d = { .server = f->server.session };
	OK(file_sync_writer_open(path, &d, &w, NULL));
	return w;
}

static file_sync_reader_t *reader(struct fixture *f)
{
	file_sync_reader_t *r;
	file_sync_reader_desc_t d = { .client = f->clients[0].session };
	OK(file_sync_reader_open(path, &d, &r, NULL));
	return r;
}

static void byte_write(const unsigned char *data, size_t n)
{
	FILE *f = fopen(path, "wb");
	CHECK(f != NULL);
	CHECK(fwrite(data, 1, n, f) == n);
	EQ(fclose(f), 0);
}

static unsigned char *byte_read(size_t *size)
{
	FILE *f = fopen(path, "rb");
	CHECK(f != NULL);
	EQ(fseek(f, 0, SEEK_END), 0);
	long n = ftell(f);
	CHECK(n >= 0);
	rewind(f);
	unsigned char *p = malloc((size_t)n + 1);
	CHECK(p != NULL);
	CHECK(fread(p, 1, (size_t)n, f) == (size_t)n);
	EQ(fclose(f), 0);
	*size = (size_t)n;
	return p;
}

static void record_one(struct fixture *f, float x)
{
	file_sync_writer_t *w = writer(f);
	spawn(&f->server, x, 0);
	OK(sync_capture(f->server.session, 1, NULL));
	OK(file_sync_writer_close(w, NULL));
}

TEST(file_session_info)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_session_info_t info;
	OK(sync_get_session_info(f.server.session, &info));
	CHECK(info.world == f.server.ecs);
	CHECK(info.ids.sync == f.server.ids.sync);
	EQ(info.role, SYNC_ROLE_SERVER);
	EQ(info.schedule, SYNC_MANUAL);
	EQ(info.limits.max_frame_bytes, 65536);
	EQ(info.limits.max_peers, 64);
	info.limits.max_frame_bytes = 1;
	OK(sync_get_session_info(f.server.session, &info));
	EQ(info.limits.max_frame_bytes, 65536);
	EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
	CHECK(!info.world && !info.ids.sync && !info.role);
	EQ(sync_get_session_info(f.server.session, NULL), SYNC_ERR_ARGUMENT);
	fixture_fini(&f);
}

TEST(file_roundtrip)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	file_sync_writer_t *w = writer(&f);
	ecs_entity_t peer = writer_status(w).peer;
	CHECK(peer != f.peers[0] && peer != f.peers[1]);
	ecs_entity_t a = spawn(&f.server, 1, peer);
	ecs_entity_t b = spawn(&f.server, 2, peer);
	ecs_entity_t hidden = spawn(&f.server, 3, f.peers[0]);
	ecs_set_id(f.server.ecs, a, f.server.reference, sizeof(b), &b);
	set_record(&f.server, a, (struct record){ .text = "owned" });
	OK(sync_capture(f.server.session, 1, NULL));
	file_sync_progress_t p;
	OK(file_sync_writer_pump(w, UINT32_MAX, &p));
	CHECK(p.frames >= 4 && p.bytes > 0);
	EQ(file_sync_writer_pump(w, 1, &p), SYNC_EMPTY);
	CHECK(!p.frames && !p.bytes);
	set_position(&f.server, a, 9);
	OK(sync_capture(f.server.session, 2, NULL));
	OK(file_sync_writer_close(w, NULL));
	CHECK(!ecs_is_alive(f.server.ecs, peer));
	CHECK(ecs_is_alive(f.server.ecs, a));
	CHECK(!ecs_has_pair(f.server.ecs, a, f.server.ids.replicate_to, peer));
	file_sync_reader_t *r = reader(&f);
	OK(file_sync_reader_step(r, 1, &p));
	EQ(p.application.updates_applied, 1);
	EQ(p.application.last_tick, 1);
	uint64_t id = network_id(&f.server, a);
	expect_position(&f.clients[0], id, 1);
	expect_absent(&f.clients[0], network_id(&f.server, hidden));
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].reference);
	CHECK(ref && *ref == replica(&f.clients[0], network_id(&f.server, b)));
	EQ(file_sync_reader_step(r, 10, &p), SYNC_END);
	EQ(p.application.updates_applied, 1);
	expect_position(&f.clients[0], id, 9);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_ENDED);
	sync_session_info_t info;
	OK(sync_get_session_info(f.clients[0].session, &info));
	CHECK(info.world == f.clients[0].ecs && info.role == SYNC_ROLE_CLIENT);
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

TEST(file_checksum_vector)
{
	/* Fixed protocol envelope; its payload is deliberately opaque to
	 * raw I/O.
	 * CRC32(length + frame) = 0x62dee721, independently checked with zlib.
	 */
	const unsigned char frame[56] = {
		[0] = 'S', [1] = 'Y', [2] = 'N', [3] = 'C',
		[4] = 1, [5] = 1, [8] = 1, [24] = 1, [32] = 1, [40] = 1
	};
	const unsigned char crc[4] = { 0x21, 0xe7, 0xde, 0x62 };
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	OK(file_sync_writer_write(w, frame, sizeof(frame)));
	OK(file_sync_writer_close(w, NULL));
	size_t size;
	unsigned char *bytes = byte_read(&size);
	EQ(size, 80);
	CHECK(bytes[16] == 56 && !bytes[17] && !bytes[18] && !bytes[19]);
	CHECK(!memcmp(bytes + 20, frame, sizeof(frame)));
	CHECK(!memcmp(bytes + 76, crc, sizeof(crc)));
	free(bytes);
	file_sync_reader_t *r;
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	file_sync_frame_t v;
	OK(file_sync_reader_peek(r, &v));
	CHECK(v.size == sizeof(frame) && !memcmp(v.data, frame, sizeof(frame)));
	OK(file_sync_reader_close(r, NULL));
}

TEST(file_format)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	OK(sync_capture(f.server.session, 1, NULL));
	struct packets packets = take_packets(f.server.session, f.peers[0]);
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	for (size_t i = 0; i < packets.count; i++)
		OK(file_sync_writer_write(w, packets.items[i].data,
			packets.items[i].size));
	OK(file_sync_writer_flush(w));
	OK(file_sync_writer_close(w, NULL));
	size_t size;
	unsigned char *bytes = byte_read(&size);
	CHECK(size > 16 && !memcmp(bytes, "FILESYNC", 8));
	CHECK(bytes[8] == 1 && bytes[9] == 0 && bytes[15] == 0);
	CHECK(bytes[16] == (packets.items[0].size & 255));
	file_sync_reader_t *r;
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	for (size_t i = 0; i < packets.count; i++) {
		file_sync_frame_t a, b;
		OK(file_sync_reader_peek(r, &a));
		file_sync_status_t before = reader_status(r);
		OK(file_sync_reader_peek(r, &b));
		CHECK(a.data == b.data && a.size == b.size);
		CHECK(a.size == packets.items[i].size);
		CHECK(!memcmp(a.data, packets.items[i].data, a.size));
		check_info_equal(&a.info, &packets.items[i].info);
		file_sync_status_t after = reader_status(r);
		CHECK(!memcmp(&before, &after, sizeof(before)));
		OK(file_sync_reader_consume(r));
	}
	file_sync_frame_t frame;
	EQ(file_sync_reader_peek(r, &frame), SYNC_END);
	CHECK(!frame.data && !frame.size);
	OK(file_sync_reader_close(r, NULL));
	free(bytes);
	packets_fini(&packets);
	fixture_fini(&f);
}

TEST(file_arguments)
{
	file_sync_writer_t *w = NULL;
	file_sync_reader_t *r = NULL;
	file_sync_error_t error;
	file_sync_status_t s;
	file_sync_progress_t p;
	file_sync_frame_t v;
	EQ(file_sync_writer_open(NULL, NULL, &w, &error), SYNC_ERR_ARGUMENT);
	CHECK(!w && error.detail.code == SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_open("", NULL, &w, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_open(NULL, NULL, &r, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_write(NULL, "x", 1), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	CHECK(!p.frames && !p.application.updates_applied);
	EQ(file_sync_writer_flush(NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
	CHECK(!s.state && !s.offset);
	EQ(file_sync_reader_peek(NULL, &v), SYNC_ERR_ARGUMENT);
	CHECK(!v.data && !v.size);
	EQ(file_sync_reader_consume(NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_step(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
	OK(file_sync_writer_close(NULL, &error));
	CHECK(!error.detail.code);
	OK(file_sync_reader_close(NULL, &error));
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	EQ(file_sync_writer_write(w, NULL, 1), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_write(w, "x", 0), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_get_status(w, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
	OK(file_sync_writer_close(w, NULL));
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	EQ(file_sync_reader_peek(r, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_reader_consume(r), SYNC_EMPTY);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_STATE);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	OK(file_sync_reader_close(r, NULL));
}

TEST(file_options)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	file_sync_writer_t *w;
	file_sync_writer_desc_t bad[] = {
		{ .schedule = (sync_schedule_t)2 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .server = f.clients[0].session },
		{ .server = f.server.session, .max_frame_bytes = 65537 }
	};
	for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
			: SYNC_ERR_ARGUMENT;
		EQ(file_sync_writer_open(path, &bad[i], &w, NULL), expected);
		CHECK(!w);
	}
	EQ(status(f.server.session, 0).peers, 2);
	file_sync_writer_desc_t d = { .server = f.server.session };
	OK(file_sync_writer_open(path, &d, &w, NULL));
	memset(&d, 0xff, sizeof(d));
	CHECK(writer_status(w).peer != 0);
	EQ(writer_status(w).max_frame_bytes, 65536);
	EQ(file_sync_writer_pump(w, 0, NULL), SYNC_ERR_ARGUMENT);
	EQ(file_sync_writer_write(w, "x", 1), SYNC_ERR_STATE);
	OK(file_sync_writer_close(w, NULL));
	file_sync_reader_t *r;
	file_sync_reader_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .client = f.server.session },
		{ .client = f.clients[0].session, .max_frame_bytes = 65537 },
		{ .client = f.clients[0].session, .schedule = SYNC_AUTOMATIC }
	};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
			: i == 5 ? SYNC_ERR_STATE : SYNC_ERR_ARGUMENT;
		EQ(file_sync_reader_open(path, &invalid[i], &r, NULL),
			expected);
		CHECK(!r);
	}
	fixture_fini(&f);
}

TEST(file_io_open)
{
	file_sync_reader_t *r;
	file_sync_error_t error;
	EQ(file_sync_reader_open("missing/record.bin", NULL, &r, &error),
		SYNC_ERR_IO);
	CHECK(!r && error.detail.code == SYNC_ERR_IO);
	file_sync_writer_t *w;
	EQ(file_sync_writer_open("missing/record.bin", NULL, &w, &error),
		SYNC_ERR_IO);
	CHECK(!w && error.detail.code == SYNC_ERR_IO);
}

TEST(file_replace)
{
	unsigned char trash[1024];
	memset(trash, 0xff, sizeof(trash));
	byte_write(trash, sizeof(trash));
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	OK(file_sync_writer_close(w, NULL));
	size_t size;
	unsigned char *p = byte_read(&size);
	EQ(size, 16);
	free(p);
}

TEST(file_header_errors)
{
	unsigned char header[16] = {'F', 'I', 'L', 'E', 'S', 'Y', 'N', 'C', 1};
	file_sync_reader_t *r;
	file_sync_error_t e;
	for (size_t n = 0; n < 16; n++) {
		byte_write(header, n);
		EQ(file_sync_reader_open(path, NULL, &r, &e),
			SYNC_ERR_TRUNCATED);
		CHECK(!r && e.record == 0 && e.offset == n);
	}
	header[0] = 'X';
	byte_write(header, 16);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
	header[0] = 'F';
	header[8] = 2;
	byte_write(header, 16);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_UNSUPPORTED);
	header[8] = 1;
	header[12] = 1;
	byte_write(header, 16);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
}

TEST(file_crc_and_limits)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	size_t size;
	unsigned char *bytes = byte_read(&size);
	bytes[22] ^= 1;
	byte_write(bytes, size);
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_PROTOCOL);
	file_sync_status_t s = reader_status(r);
	EQ(s.state, FILE_SYNC_FAULTED);
	EQ(s.fault.record, 1);
	CHECK(strstr(s.fault.detail.message, "checksum"));
	EQ(status(f.clients[0].session, 0).entities, 0);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
	bytes[22] ^= 1;
	bytes[16] = bytes[17] = bytes[18] = bytes[19] = 0xff;
	byte_write(bytes, size);
	r = reader(&f);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_LIMIT);
	EQ(reader_status(r).pending_bytes, 0);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_LIMIT);
	memset(bytes + 16, 0, 4);
	byte_write(bytes, size);
	r = reader(&f);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_PROTOCOL);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
	free(bytes);
	fixture_fini(&f);
}

TEST(file_partial_records)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	size_t size;
	unsigned char *bytes = byte_read(&size);
	uint32_t length = (uint32_t)bytes[16] | (uint32_t)bytes[17] << 8 |
		(uint32_t)bytes[18] << 16 | (uint32_t)bytes[19] << 24;
	CHECK(length > 8);
	size_t cuts[] = {17, 19, 20, 20 + length / 2,
		20 + length, 20 + length + 3};
	for (size_t i = 0; i < sizeof(cuts) / sizeof(*cuts); i++) {
		byte_write(bytes, cuts[i]);
		file_sync_reader_t *r;
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		file_sync_frame_t v;
		EQ(file_sync_reader_peek(r, &v), SYNC_ERR_TRUNCATED);
		CHECK(!v.data && !v.size);
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	}
	free(bytes);
	fixture_fini(&f);
}

TEST(file_logical_eof)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	OK(file_sync_writer_close(w, NULL));
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_TRUNCATED);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	OK(sync_client_reset(f.clients[0].session));
	spawn(&f.server, 1, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	for (size_t i = 0; i + 1 < p.count; i++)
		OK(file_sync_writer_write(w, p.items[i].data, p.items[i].size));
	OK(file_sync_writer_close(w, NULL));
	r = reader(&f);
	file_sync_progress_t progress;
	EQ(file_sync_reader_step(r, 1, &progress), SYNC_ERR_TRUNCATED);
	CHECK(!progress.application.updates_applied && progress.frames > 0);
	EQ(status(f.clients[0].session, 0).entities, 0);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	packets_fini(&p);
	fixture_fini(&f);
}

TEST(file_writer_sequence)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	struct packets q = take_packets(f.server.session, f.peers[1]);
	for (unsigned mode = 0; mode < 3; mode++) {
		file_sync_writer_t *w;
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		OK(file_sync_writer_write(w, p.items[0].data, p.items[0].size));
		struct packet *v = mode == 2 ? &q.items[1]
			: &p.items[mode == 0 ? 0 : 2];
		EQ(file_sync_writer_write(w, v->data, v->size),
			SYNC_ERR_SEQUENCE);
		EQ(writer_status(w).frames, 1);
		EQ(file_sync_writer_close(w, NULL), SYNC_ERR_SEQUENCE);
	}
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	EQ(file_sync_writer_write(w, p.items[1].data, p.items[1].size),
		SYNC_ERR_SEQUENCE);
	EQ(file_sync_writer_close(w, NULL), SYNC_ERR_SEQUENCE);
	packets_fini(&q);
	packets_fini(&p);
	fixture_fini(&f);
}

TEST(file_fragmented_backpressure)
{
	struct fixture f;
	sync_client_desc_t cd = { .limits.max_receive_bytes = 65536 };
	fixture_init(&f, NULL, &cd);
	file_sync_writer_t *w = writer(&f);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char *text = malloc(65536);
	CHECK(text != NULL);
	memset(text, 'a', 65535);
	text[65535] = 0;
	set_record(&f.server, e, (struct record){ .text = text });
	OK(sync_capture(f.server.session, 1, NULL));
	OK(file_sync_writer_pump(w, UINT32_MAX, NULL));
	text[0] = 'b';
	set_record(&f.server, e, (struct record){ .text = text });
	OK(sync_capture(f.server.session, 2, NULL));
	OK(file_sync_writer_close(w, NULL));
	file_sync_reader_t *r = reader(&f);
	file_sync_progress_t p;
	EQ(file_sync_reader_pump(r, UINT32_MAX, &p), SYNC_BACKPRESSURE);
	CHECK(p.frames > 0 && reader_status(r).pending_bytes > 0);
	file_sync_status_t before = reader_status(r);
	EQ(file_sync_reader_pump(r, 1, &p), SYNC_BACKPRESSURE);
	CHECK(!p.frames && !p.bytes);
	file_sync_status_t after = reader_status(r);
	CHECK(!memcmp(&before, &after, sizeof(before)));
	OK(file_sync_reader_step(r, 1, &p));
	EQ(p.application.last_tick, 1);
	uint64_t id = network_id(&f.server, e);
	const struct record *v = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].record);
	CHECK(v && v->text[0] == 'a');
	OK(file_sync_reader_step(r, 1, &p));
	v = ecs_get_id(f.clients[0].ecs, replica(&f.clients[0], id),
		f.clients[0].record);
	CHECK(v && !strcmp(v->text, text));
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_END);
	OK(file_sync_reader_close(r, NULL));
	free(text);
	fixture_fini(&f);
}

TEST(file_eof_pending)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
	EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
	CHECK(!reader_status(r).pending_bytes);
	drain(f.clients[0].session);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

TEST(file_borrowed_client)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	file_sync_reader_t *r = reader(&f);
	OK(file_sync_reader_step(r, 1, NULL));
	ecs_entity_t local = f.clients[0].local;
	OK(file_sync_reader_close(r, NULL));
	EQ(status(f.clients[0].session, 0).entities, 1);
	CHECK(ecs_is_alive(f.clients[0].ecs, local));
	file_sync_reader_desc_t d = { .client = f.clients[0].session };
	EQ(file_sync_reader_open(path, &d, &r, NULL), SYNC_ERR_STATE);
	CHECK(!r);
	OK(sync_client_reset(f.clients[0].session));
	OK(file_sync_reader_open(path, &d, &r, NULL));
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

TEST(file_automatic)
{
	struct fixture f;
	sync_server_desc_t sd = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, &sd, &cd);
	int32_t systems = ecs_count_id(f.server.ecs, EcsSystem);
	file_sync_writer_desc_t wd = { .server = f.server.session,
		.schedule = SYNC_AUTOMATIC };
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	CHECK(ecs_count_id(f.server.ecs, EcsSystem) == systems + 1);
	EQ(writer_status(w).frames_per_progress, 64);
	ecs_entity_t e = spawn(&f.server, 7, writer_status(w).peer);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	CHECK(writer_status(w).frames >= 4);
	EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
	OK(file_sync_writer_flush(w));
	OK(file_sync_writer_close(w, NULL));
	EQ(ecs_count_id(f.server.ecs, EcsSystem), systems);
	file_sync_reader_desc_t rd = { .client = f.clients[0].session,
		.schedule = SYNC_AUTOMATIC, .frames_per_progress = 1 };
	file_sync_reader_t *r;
	OK(file_sync_reader_open(path, &rd, &r, NULL));
	file_sync_progress_t p = { .frames = 99 };
	EQ(file_sync_reader_step(r, 1, &p), SYNC_ERR_STATE);
	CHECK(!p.frames);
	for (unsigned i = 0; i < 32; i++) {
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		CHECK(reader_status(r).frames <= i + 1);
		if (reader_status(r).state == FILE_SYNC_ENDED)
			break;
	}
	EQ(reader_status(r).state, FILE_SYNC_ENDED);
	expect_position(&f.clients[0], network_id(&f.server, e), 7);
	OK(file_sync_reader_close(r, NULL));
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	fixture_fini(&f);
}

TEST(file_auto_manual_client)
{
	struct fixture f;
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, NULL, &cd);
	record_one(&f, 1);
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	EQ(reader_status(r).offset, 16);
	EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

static void phase_writer(ecs_iter_t *it)
{
	struct position *p = ecs_field(it, struct position, 0);
	for (int32_t i = 0; i < it->count; i++) {
		p[i].x = 42;
		p[i].y = 43;
	}
}

struct phase_context {
	struct fixture *fixture;
	uint64_t id;
	bool read;
};

static void phase_reader(ecs_iter_t *it)
{
	struct phase_context *ctx = it->ctx;
	expect_position(&ctx->fixture->clients[0], ctx->id, 42);
	ctx->read = true;
}

TEST(file_phase_order)
{
	struct fixture f;
	sync_server_desc_t sd = {
		.schedule = SYNC_AUTOMATIC, .send_hz = 8
	};
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	fixture_init(&f, &sd, &cd);
	file_sync_writer_t *w;
	file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC
	};
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	ecs_entity_t e = spawn(&f.server, 1, writer_status(w).peer);
	ecs_entity_t sys = ecs_new(f.server.ecs);
	ecs_add_pair(f.server.ecs, sys, EcsDependsOn, EcsPostUpdate);
	CHECK(ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_writer,
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }}
	}));
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	CHECK(writer_status(w).frames > 0);
	OK(file_sync_writer_close(w, NULL));
	file_sync_reader_t *r;
	file_sync_reader_desc_t rd = {
		.client = f.clients[0].session, .schedule = SYNC_AUTOMATIC
	};
	OK(file_sync_reader_open(path, &rd, &r, NULL));
	EQ(reader_status(r).frames_per_progress, 64);
	struct phase_context ctx = {
		.fixture = &f, .id = network_id(&f.server, e)
	};
	sys = ecs_new(f.clients[0].ecs);
	ecs_add_pair(f.clients[0].ecs, sys, EcsDependsOn, EcsPreUpdate);
	CHECK(ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_reader, .ctx = &ctx
	}));
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	CHECK(ctx.read);
	EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	EQ(reader_status(r).state, FILE_SYNC_ENDED);
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

TEST(file_writer_budget)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	file_sync_writer_t *w;
	file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC,
		.frames_per_progress = 1
	};
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	spawn(&f.server, 1, 0);
	OK(sync_capture(f.server.session, 1, NULL));
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	EQ(writer_status(w).frames, 1);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	EQ(writer_status(w).frames, 2);
	OK(file_sync_writer_close(w, NULL));
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_step(r, 10, NULL), SYNC_END);
	EQ(status(f.clients[0].session, 0).entities, 1);
	OK(file_sync_reader_close(r, NULL));
	fixture_fini(&f);
}

TEST(file_later_failure)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	size_t size;
	unsigned char *bytes = byte_read(&size);
	uint32_t length = (uint32_t)bytes[16] |
		(uint32_t)bytes[17] << 8 | (uint32_t)bytes[18] << 16 |
		(uint32_t)bytes[19] << 24;
	FILE *file = fopen(path, "ab");
	CHECK(file != NULL);
	CHECK(fwrite(bytes + 16, 1, (size_t)length + 8, file) ==
		(size_t)length + 8);
	EQ(fclose(file), 0);
	free(bytes);
	file_sync_reader_t *r = reader(&f);
	file_sync_progress_t p;
	EQ(file_sync_reader_step(r, 10, &p), SYNC_ERR_SEQUENCE);
	EQ(p.application.updates_applied, 1);
	EQ(p.application.last_tick, 1);
	EQ(status(f.clients[0].session, 0).entities, 1);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	EQ(reader_status(r).state, FILE_SYNC_FAULTED);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SEQUENCE);
	fixture_fini(&f);
}

TEST(file_schema_failure)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_session_destroy(f.clients[0].session);
	f.clients[0].session = NULL;
	ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
	OK(sync_client_create(f.clients[0].ecs, NULL,
		&f.clients[0].session, NULL));
	record_one(&f, 1);
	file_sync_reader_t *r = reader(&f);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_SCHEMA);
	EQ(status(f.clients[0].session, 0).entities, 0);
	EQ(reader_status(r).fault.detail.code, SYNC_ERR_SCHEMA);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SCHEMA);
	fixture_fini(&f);
}

#ifdef FILE_SYNC_TESTING
TEST(file_fault_allocations)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	for (size_t n = 0; n < 32; n++) {
		file_sync_test_arm(FILE_SYNC_TEST_ALLOC, n);
		file_sync_writer_t *w;
		file_sync_writer_desc_t d = { .server = f.server.session,
			.schedule = SYNC_AUTOMATIC };
		file_sync_error_t error;
		sync_result_t result = file_sync_writer_open(path, &d, &w,
			&error);
		bool hit = file_sync_test_hit(FILE_SYNC_TEST_ALLOC);
		file_sync_test_clear();
		if (hit) {
			EQ(result, SYNC_ERR_NOMEM);
			CHECK(!w && error.detail.code == SYNC_ERR_NOMEM);
		} else {
			OK(result);
			OK(file_sync_writer_close(w, NULL));
		}
		EQ(status(f.server.session, 0).peers, 2);
		if (!hit)
			break;
	}
	record_one(&f, 1);
	bool done = false;
	for (size_t n = 0; n < 32; n++) {
		file_sync_reader_t *r;
		file_sync_reader_desc_t d = { .client = f.clients[0].session };
		file_sync_test_arm(FILE_SYNC_TEST_ALLOC, n);
		sync_result_t result = file_sync_reader_open(path, &d, &r,
			NULL);
		if (result == SYNC_OK)
			result = file_sync_reader_step(r, 1, NULL);
		bool hit = file_sync_test_hit(FILE_SYNC_TEST_ALLOC);
		file_sync_test_clear();
		if (hit) {
			EQ(result, SYNC_ERR_NOMEM);
			if (r)
				EQ(file_sync_reader_close(r, NULL),
					SYNC_ERR_NOMEM);
		} else {
			OK(result);
			OK(file_sync_reader_close(r, NULL));
			done = true;
		}
		OK(sync_client_reset(f.clients[0].session));
		if (done)
			break;
	}
	CHECK(done);
	fixture_fini(&f);
}

TEST(file_fault_write_commit)
{
	for (size_t n = 0; n < 3; n++) {
		struct fixture f;
		fixture_init(&f, NULL, NULL);
		file_sync_writer_t *w = writer(&f);
		ecs_entity_t peer = writer_status(w).peer;
		OK(sync_capture(f.server.session, 1, NULL));
		sync_frame_view_t before, after;
		OK(sync_outbox_peek(f.server.session, peer, &before));
		file_sync_test_arm(FILE_SYNC_TEST_WRITE, n);
		file_sync_progress_t p;
		EQ(file_sync_writer_pump(w, 1, &p), SYNC_ERR_IO);
		CHECK(file_sync_test_hit(FILE_SYNC_TEST_WRITE));
		file_sync_test_clear();
		CHECK(!p.frames);
		OK(sync_outbox_peek(f.server.session, peer, &after));
		CHECK(before.data == after.data);
		CHECK(!memcmp(&before.token, &after.token,
			sizeof(before.token)));
		EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
		file_sync_error_t error;
		EQ(file_sync_writer_close(w, &error), SYNC_ERR_IO);
		CHECK(error.system_error != 0 && error.record == 1);
		EQ(status(f.server.session, 0).peers, 2);
		fixture_fini(&f);
	}
}

TEST(file_fault_flush_close)
{
	for (unsigned i = 0; i < 2; i++) {
		file_sync_writer_t *w;
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		unsigned fault = i ? FILE_SYNC_TEST_CLOSE
			: FILE_SYNC_TEST_FLUSH;
		file_sync_test_arm(fault, 0);
		file_sync_error_t error;
		EQ(file_sync_writer_close(w, &error), SYNC_ERR_IO);
		CHECK(file_sync_test_hit(fault));
		file_sync_test_clear();
		CHECK(error.detail.code == SYNC_ERR_IO);
	}
	file_sync_writer_t *w;
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	file_sync_test_arm(FILE_SYNC_TEST_FLUSH, 0);
	EQ(file_sync_writer_flush(w), SYNC_ERR_IO);
	file_sync_error_t first = writer_status(w).fault;
	file_sync_test_clear();
	file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
	file_sync_error_t error;
	EQ(file_sync_writer_close(w, &error), SYNC_ERR_IO);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
	file_sync_test_clear();
	CHECK(!memcmp(&first, &error, sizeof(first)));
	file_sync_reader_t *r;
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
	file_sync_test_clear();
}

TEST(file_fault_read)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	record_one(&f, 1);
	file_sync_reader_t *r = reader(&f);
	file_sync_test_arm(FILE_SYNC_TEST_READ, 0);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_IO);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_READ));
	file_sync_test_clear();
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
	fixture_fini(&f);
}
#endif

static const struct {
	const char *name;
	void (*run)(void);
} cases[] = {
#define CASE(name) { #name, name },
#include "cases.def"
#undef CASE
};

int main(int argc, char **argv)
{
	CHECK(argc == 2);
	for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
		if (strcmp(argv[1], cases[i].name))
			continue;
		cases[i].run();
		remove(path);
		return 0;
	}
	return 1;
}
