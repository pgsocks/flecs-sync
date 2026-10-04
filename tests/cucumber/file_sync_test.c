/* Private procedural script; derived from native contracts. */
#include "test.h"
#include <file_sync.h>
#ifdef FILE_SYNC_TESTING
#include "../../adapters/file_sync/tests/private.h"
#endif
#include "protocol.h"

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
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2283740495u);
	sync_session_info_t info;
	cucumber_reply();

	cucumber_wait(4, 1602894114u);
	OK(sync_get_session_info(f.server.session, &info));
	cucumber_reply();

	cucumber_wait(5, 2782292197u);
	CHECK(info.world == f.server.ecs);
	cucumber_reply();

	cucumber_wait(6, 3425405084u);
	CHECK(info.ids.sync == f.server.ids.sync);
	cucumber_reply();

	cucumber_wait(7, 9728669u);
	EQ(info.role, SYNC_ROLE_SERVER);
	cucumber_reply();

	cucumber_wait(8, 2035620176u);
	EQ(info.schedule, SYNC_MANUAL);
	cucumber_reply();

	cucumber_wait(9, 2231762950u);
	EQ(info.limits.max_frame_bytes, 65536);
	cucumber_reply();

	cucumber_wait(10, 2508789471u);
	EQ(info.limits.max_peers, 64);
	cucumber_reply();

	cucumber_wait(11, 1083054191u);
	info.limits.max_frame_bytes = 1;
	cucumber_reply();

	cucumber_wait(12, 1602894114u);
	OK(sync_get_session_info(f.server.session, &info));
	cucumber_reply();

	cucumber_wait(13, 2231762950u);
	EQ(info.limits.max_frame_bytes, 65536);
	cucumber_reply();

	cucumber_wait(14, 950178470u);
	EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(15, 1603693245u);
	CHECK(!info.world && !info.ids.sync && !info.role);
	cucumber_reply();

	cucumber_wait(16, 255881866u);
	EQ(sync_get_session_info(f.server.session, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_roundtrip)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1794765093u);
	file_sync_writer_t *w = writer(&f);
	cucumber_reply();

	cucumber_wait(4, 2231748017u);
	ecs_entity_t peer = writer_status(w).peer;
	cucumber_reply();

	cucumber_wait(5, 4002888009u);
	CHECK(peer != f.peers[0] && peer != f.peers[1]);
	cucumber_reply();

	cucumber_wait(6, 1831099914u);
	ecs_entity_t a = spawn(&f.server, 1, peer);
	cucumber_reply();

	cucumber_wait(7, 268978135u);
	ecs_entity_t b = spawn(&f.server, 2, peer);
	cucumber_reply();

	cucumber_wait(8, 544446199u);
	ecs_entity_t hidden = spawn(&f.server, 3, f.peers[0]);
	cucumber_reply();

	cucumber_wait(9, 1308941482u);
	ecs_set_id(f.server.ecs, a, f.server.reference, sizeof(b), &b);
	cucumber_reply();

	cucumber_wait(10, 463438277u);
	set_record(&f.server, a, (struct record){ .text = "owned" });
	cucumber_reply();

	cucumber_wait(11, 2878314050u);
	OK(sync_capture(f.server.session, 1, NULL));
	cucumber_reply();

	cucumber_wait(12, 1223216047u);
	file_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(13, 2177494946u);
	OK(file_sync_writer_pump(w, UINT32_MAX, &p));
	cucumber_reply();

	cucumber_wait(14, 4051824959u);
	CHECK(p.frames >= 4 && p.bytes > 0);
	cucumber_reply();

	cucumber_wait(15, 790939128u);
	EQ(file_sync_writer_pump(w, 1, &p), SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(16, 2166661372u);
	CHECK(!p.frames && !p.bytes);
	cucumber_reply();

	cucumber_wait(17, 4238534009u);
	set_position(&f.server, a, 9);
	cucumber_reply();

	cucumber_wait(18, 1085812033u);
	OK(sync_capture(f.server.session, 2, NULL));
	cucumber_reply();

	cucumber_wait(19, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(20, 3593313067u);
	CHECK(!ecs_is_alive(f.server.ecs, peer));
	cucumber_reply();

	cucumber_wait(21, 2695876102u);
	CHECK(ecs_is_alive(f.server.ecs, a));
	cucumber_reply();

	cucumber_wait(22, 3479779191u);
	CHECK(!ecs_has_pair(f.server.ecs, a, f.server.ids.replicate_to, peer));
	cucumber_reply();

	cucumber_wait(23, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(24, 2372877457u);
	OK(file_sync_reader_step(r, 1, &p));
	cucumber_reply();

	cucumber_wait(25, 910344632u);
	EQ(p.application.updates_applied, 1);
	cucumber_reply();

	cucumber_wait(26, 3046570383u);
	EQ(p.application.last_tick, 1);
	cucumber_reply();

	cucumber_wait(27, 4173218971u);
	uint64_t id = network_id(&f.server, a);
	cucumber_reply();

	cucumber_wait(28, 1962453316u);
	expect_position(&f.clients[0], id, 1);
	cucumber_reply();

	cucumber_wait(29, 2843380671u);
	expect_absent(&f.clients[0], network_id(&f.server, hidden));
	cucumber_reply();

	cucumber_wait(30, 1622926124u);
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(31, 2141778629u);
	CHECK(ref && *ref == replica(&f.clients[0], network_id(&f.server, b)));
	cucumber_reply();

	cucumber_wait(32, 1032120925u);
	EQ(file_sync_reader_step(r, 10, &p), SYNC_END);
	cucumber_reply();

	cucumber_wait(33, 910344632u);
	EQ(p.application.updates_applied, 1);
	cucumber_reply();

	cucumber_wait(34, 2062285052u);
	expect_position(&f.clients[0], id, 9);
	cucumber_reply();

	cucumber_wait(35, 3914978821u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_ENDED);
	cucumber_reply();

	cucumber_wait(36, 2283740495u);
	sync_session_info_t info;
	cucumber_reply();

	cucumber_wait(37, 304272964u);
	OK(sync_get_session_info(f.clients[0].session, &info));
	cucumber_reply();

	cucumber_wait(38, 3644523484u);
	CHECK(info.world == f.clients[0].ecs && info.role == SYNC_ROLE_CLIENT);
	cucumber_reply();

	cucumber_wait(39, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(40, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_checksum_vector)
{
	cucumber_wait(1, 1871979902u);
	/* Fixed protocol envelope; its payload is deliberately opaque to
	 * raw I/O.
	 * CRC32(length + frame) = 0x62dee721, independently checked with zlib.
	 */
	const unsigned char frame[56] = {
		[0] = 'S', [1] = 'Y', [2] = 'N', [3] = 'C',
		[4] = 1, [5] = 1, [8] = 1, [24] = 1, [32] = 1, [40] = 1
	};
	cucumber_reply();

	cucumber_wait(2, 709273082u);
	const unsigned char crc[4] = { 0x21, 0xe7, 0xde, 0x62 };
	cucumber_reply();

	cucumber_wait(3, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(4, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(5, 345881604u);
	OK(file_sync_writer_write(w, frame, sizeof(frame)));
	cucumber_reply();

	cucumber_wait(6, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(7, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(8, 1609962508u);
	unsigned char *bytes = byte_read(&size);
	cucumber_reply();

	cucumber_wait(9, 1759597293u);
	EQ(size, 80);
	cucumber_reply();

	cucumber_wait(10, 248259248u);
	CHECK(bytes[16] == 56 && !bytes[17] && !bytes[18] && !bytes[19]);
	cucumber_reply();

	cucumber_wait(11, 2667260376u);
	CHECK(!memcmp(bytes + 20, frame, sizeof(frame)));
	cucumber_reply();

	cucumber_wait(12, 4288237414u);
	CHECK(!memcmp(bytes + 76, crc, sizeof(crc)));
	cucumber_reply();

	cucumber_wait(13, 2181658292u);
	free(bytes);
	cucumber_reply();

	cucumber_wait(14, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(15, 3048511915u);
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	cucumber_reply();

	cucumber_wait(16, 989363182u);
	file_sync_frame_t v;
	cucumber_reply();

	cucumber_wait(17, 4224049624u);
	OK(file_sync_reader_peek(r, &v));
	cucumber_reply();

	cucumber_wait(18, 3658497034u);
	CHECK(v.size == sizeof(frame) && !memcmp(v.data, frame, sizeof(frame)));
	cucumber_reply();

	cucumber_wait(19, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();
}

TEST(file_format)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2878314050u);
	OK(sync_capture(f.server.session, 1, NULL));
	cucumber_reply();

	cucumber_wait(4, 2526222188u);
	struct packets packets = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(6, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(7, 2085956734u);
	for (size_t i = 0; i < packets.count; i++)
		OK(file_sync_writer_write(w, packets.items[i].data,
			packets.items[i].size));
	cucumber_reply();

	cucumber_wait(8, 1594972750u);
	OK(file_sync_writer_flush(w));
	cucumber_reply();

	cucumber_wait(9, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(10, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(11, 1609962508u);
	unsigned char *bytes = byte_read(&size);
	cucumber_reply();

	cucumber_wait(12, 3321244685u);
	CHECK(size > 16 && !memcmp(bytes, "FILESYNC", 8));
	cucumber_reply();

	cucumber_wait(13, 695627426u);
	CHECK(bytes[8] == 1 && bytes[9] == 0 && bytes[15] == 0);
	cucumber_reply();

	cucumber_wait(14, 4178262515u);
	CHECK(bytes[16] == (packets.items[0].size & 255));
	cucumber_reply();

	cucumber_wait(15, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(16, 3048511915u);
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	cucumber_reply();

	cucumber_wait(17, 3140498054u);
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
	cucumber_reply();

	cucumber_wait(18, 1762155217u);
	file_sync_frame_t frame;
	cucumber_reply();

	cucumber_wait(19, 2284985488u);
	EQ(file_sync_reader_peek(r, &frame), SYNC_END);
	cucumber_reply();

	cucumber_wait(20, 2456814416u);
	CHECK(!frame.data && !frame.size);
	cucumber_reply();

	cucumber_wait(21, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(22, 2181658292u);
	free(bytes);
	cucumber_reply();

	cucumber_wait(23, 2639877249u);
	packets_fini(&packets);
	cucumber_reply();

	cucumber_wait(24, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_arguments)
{
	cucumber_wait(1, 1804844576u);
	file_sync_writer_t *w = NULL;
	cucumber_reply();

	cucumber_wait(2, 3493089705u);
	file_sync_reader_t *r = NULL;
	cucumber_reply();

	cucumber_wait(3, 835631473u);
	file_sync_error_t error;
	cucumber_reply();

	cucumber_wait(4, 546654991u);
	file_sync_status_t s;
	cucumber_reply();

	cucumber_wait(5, 1223216047u);
	file_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(6, 989363182u);
	file_sync_frame_t v;
	cucumber_reply();

	cucumber_wait(7, 1802147661u);
	EQ(file_sync_writer_open(NULL, NULL, &w, &error), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(8, 486041912u);
	CHECK(!w && error.detail.code == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 481637832u);
	EQ(file_sync_writer_open("", NULL, &w, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(10, 866772788u);
	EQ(file_sync_writer_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(11, 3106784886u);
	EQ(file_sync_reader_open(NULL, NULL, &r, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(12, 1990773160u);
	EQ(file_sync_reader_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(13, 2336190974u);
	EQ(file_sync_writer_write(NULL, "x", 1), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(14, 4253722151u);
	EQ(file_sync_writer_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(15, 2897143651u);
	CHECK(!p.frames && !p.application.updates_applied);
	cucumber_reply();

	cucumber_wait(16, 1754429994u);
	EQ(file_sync_writer_flush(NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(17, 1946953911u);
	EQ(file_sync_writer_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(18, 2186842050u);
	CHECK(!s.state && !s.offset);
	cucumber_reply();

	cucumber_wait(19, 2402385768u);
	EQ(file_sync_reader_peek(NULL, &v), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(20, 1014638056u);
	CHECK(!v.data && !v.size);
	cucumber_reply();

	cucumber_wait(21, 3801627109u);
	EQ(file_sync_reader_consume(NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(22, 3794129952u);
	EQ(file_sync_reader_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(23, 1604153974u);
	EQ(file_sync_reader_step(NULL, 1, &p), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(24, 706976544u);
	EQ(file_sync_reader_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(25, 823793312u);
	OK(file_sync_writer_close(NULL, &error));
	cucumber_reply();

	cucumber_wait(26, 2123059007u);
	CHECK(!error.detail.code);
	cucumber_reply();

	cucumber_wait(27, 3085433032u);
	OK(file_sync_reader_close(NULL, &error));
	cucumber_reply();

	cucumber_wait(28, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(29, 2617032636u);
	EQ(file_sync_writer_write(w, NULL, 1), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(30, 806448543u);
	EQ(file_sync_writer_write(w, "x", 0), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(31, 1781795965u);
	EQ(file_sync_writer_get_status(w, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(32, 1416087218u);
	EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(33, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(34, 3048511915u);
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	cucumber_reply();

	cucumber_wait(35, 4241488407u);
	EQ(file_sync_reader_peek(r, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(36, 105053894u);
	EQ(file_sync_reader_consume(r), SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(37, 1008138159u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(38, 2222366802u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(39, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();
}

TEST(file_options)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(4, 1672858072u);
	file_sync_writer_desc_t bad[] = {
		{ .schedule = (sync_schedule_t)2 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .server = f.clients[0].session },
		{ .server = f.server.session, .max_frame_bytes = 65537 }
	};
	cucumber_reply();

	cucumber_wait(5, 4060837433u);
	for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
			: SYNC_ERR_ARGUMENT;
		EQ(file_sync_writer_open(path, &bad[i], &w, NULL), expected);
		CHECK(!w);
	}
	cucumber_reply();

	cucumber_wait(6, 458143264u);
	EQ(status(f.server.session, 0).peers, 2);
	cucumber_reply();

	cucumber_wait(7, 418405530u);
	file_sync_writer_desc_t d = { .server = f.server.session };
	cucumber_reply();

	cucumber_wait(8, 21098717u);
	OK(file_sync_writer_open(path, &d, &w, NULL));
	cucumber_reply();

	cucumber_wait(9, 3391436121u);
	memset(&d, 0xff, sizeof(d));
	cucumber_reply();

	cucumber_wait(10, 3283005883u);
	CHECK(writer_status(w).peer != 0);
	cucumber_reply();

	cucumber_wait(11, 1289564287u);
	EQ(writer_status(w).max_frame_bytes, 65536);
	cucumber_reply();

	cucumber_wait(12, 1446722671u);
	EQ(file_sync_writer_pump(w, 0, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(13, 339135387u);
	EQ(file_sync_writer_write(w, "x", 1), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(14, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(15, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(16, 447048407u);
	file_sync_reader_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .client = f.server.session },
		{ .client = f.clients[0].session, .max_frame_bytes = 65537 },
		{ .client = f.clients[0].session, .schedule = SYNC_AUTOMATIC }
	};
	cucumber_reply();

	cucumber_wait(17, 4250394346u);
	for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
			: i == 5 ? SYNC_ERR_STATE : SYNC_ERR_ARGUMENT;
		EQ(file_sync_reader_open(path, &invalid[i], &r, NULL),
			expected);
		CHECK(!r);
	}
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_io_open)
{
	cucumber_wait(1, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(2, 835631473u);
	file_sync_error_t error;
	cucumber_reply();

	cucumber_wait(3, 1547693250u);
	EQ(file_sync_reader_open("missing/record.bin", NULL, &r, &error),
		SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(4, 4020547815u);
	CHECK(!r && error.detail.code == SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(5, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(6, 1228168481u);
	EQ(file_sync_writer_open("missing/record.bin", NULL, &w, &error),
		SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(7, 1081643947u);
	CHECK(!w && error.detail.code == SYNC_ERR_IO);
	cucumber_reply();
}

TEST(file_replace)
{
	cucumber_wait(1, 883425841u);
	unsigned char trash[1024];
	cucumber_reply();

	cucumber_wait(2, 2535072018u);
	memset(trash, 0xff, sizeof(trash));
	cucumber_reply();

	cucumber_wait(3, 2532885554u);
	byte_write(trash, sizeof(trash));
	cucumber_reply();

	cucumber_wait(4, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(5, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(6, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(7, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(8, 3055292033u);
	unsigned char *p = byte_read(&size);
	cucumber_reply();

	cucumber_wait(9, 291791317u);
	EQ(size, 16);
	cucumber_reply();

	cucumber_wait(10, 3313518797u);
	free(p);
	cucumber_reply();
}

TEST(file_header_errors)
{
	cucumber_wait(1, 1304661206u);
	unsigned char header[16] = {'F', 'I', 'L', 'E', 'S', 'Y', 'N', 'C', 1};
	cucumber_reply();

	cucumber_wait(2, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(3, 3411580093u);
	file_sync_error_t e;
	cucumber_reply();

	cucumber_wait(4, 768321564u);
	for (size_t n = 0; n < 16; n++) {
		byte_write(header, n);
		EQ(file_sync_reader_open(path, NULL, &r, &e),
			SYNC_ERR_TRUNCATED);
		CHECK(!r && e.record == 0 && e.offset == n);
	}
	cucumber_reply();

	cucumber_wait(5, 1032676887u);
	header[0] = 'X';
	cucumber_reply();

	cucumber_wait(6, 284470037u);
	byte_write(header, 16);
	cucumber_reply();

	cucumber_wait(7, 3822622495u);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(8, 724953197u);
	header[0] = 'F';
	cucumber_reply();

	cucumber_wait(9, 1139051374u);
	header[8] = 2;
	cucumber_reply();

	cucumber_wait(10, 284470037u);
	byte_write(header, 16);
	cucumber_reply();

	cucumber_wait(11, 2983013722u);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_UNSUPPORTED);
	cucumber_reply();

	cucumber_wait(12, 1758058669u);
	header[8] = 1;
	cucumber_reply();

	cucumber_wait(13, 3271808160u);
	header[12] = 1;
	cucumber_reply();

	cucumber_wait(14, 284470037u);
	byte_write(header, 16);
	cucumber_reply();

	cucumber_wait(15, 3822622495u);
	EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();
}

TEST(file_crc_and_limits)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(5, 1609962508u);
	unsigned char *bytes = byte_read(&size);
	cucumber_reply();

	cucumber_wait(6, 3710007138u);
	bytes[22] ^= 1;
	cucumber_reply();

	cucumber_wait(7, 133398239u);
	byte_write(bytes, size);
	cucumber_reply();

	cucumber_wait(8, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(9, 272032313u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(10, 640004173u);
	file_sync_status_t s = reader_status(r);
	cucumber_reply();

	cucumber_wait(11, 3580807486u);
	EQ(s.state, FILE_SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(12, 728963409u);
	EQ(s.fault.record, 1);
	cucumber_reply();

	cucumber_wait(13, 3772001471u);
	CHECK(strstr(s.fault.detail.message, "checksum"));
	cucumber_reply();

	cucumber_wait(14, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(15, 2222366802u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(16, 727908428u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(17, 3710007138u);
	bytes[22] ^= 1;
	cucumber_reply();

	cucumber_wait(18, 3633414181u);
	bytes[16] = bytes[17] = bytes[18] = bytes[19] = 0xff;
	cucumber_reply();

	cucumber_wait(19, 133398239u);
	byte_write(bytes, size);
	cucumber_reply();

	cucumber_wait(20, 816563193u);
	r = reader(&f);
	cucumber_reply();

	cucumber_wait(21, 1756340681u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(22, 1816753584u);
	EQ(reader_status(r).pending_bytes, 0);
	cucumber_reply();

	cucumber_wait(23, 4057896479u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(24, 883662176u);
	memset(bytes + 16, 0, 4);
	cucumber_reply();

	cucumber_wait(25, 133398239u);
	byte_write(bytes, size);
	cucumber_reply();

	cucumber_wait(26, 816563193u);
	r = reader(&f);
	cucumber_reply();

	cucumber_wait(27, 358787273u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(28, 727908428u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(29, 2181658292u);
	free(bytes);
	cucumber_reply();

	cucumber_wait(30, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_partial_records)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(5, 1609962508u);
	unsigned char *bytes = byte_read(&size);
	cucumber_reply();

	cucumber_wait(6, 3883614572u);
	uint32_t length = (uint32_t)bytes[16] | (uint32_t)bytes[17] << 8 |
		(uint32_t)bytes[18] << 16 | (uint32_t)bytes[19] << 24;
	cucumber_reply();

	cucumber_wait(7, 3910473460u);
	CHECK(length > 8);
	cucumber_reply();

	cucumber_wait(8, 3076807150u);
	size_t cuts[] = {17, 19, 20, 20 + length / 2,
		20 + length, 20 + length + 3};
	cucumber_reply();

	cucumber_wait(9, 1866144969u);
	for (size_t i = 0; i < sizeof(cuts) / sizeof(*cuts); i++) {
		byte_write(bytes, cuts[i]);
		file_sync_reader_t *r;
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		file_sync_frame_t v;
		EQ(file_sync_reader_peek(r, &v), SYNC_ERR_TRUNCATED);
		CHECK(!v.data && !v.size);
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	}
	cucumber_reply();

	cucumber_wait(10, 2181658292u);
	free(bytes);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_logical_eof)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(4, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(5, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(6, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(7, 2163942411u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(8, 2695885914u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(9, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(10, 1637123758u);
	spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(11, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(12, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(13, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(14, 1101544823u);
	for (size_t i = 0; i + 1 < p.count; i++)
		OK(file_sync_writer_write(w, p.items[i].data, p.items[i].size));
	cucumber_reply();

	cucumber_wait(15, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(16, 816563193u);
	r = reader(&f);
	cucumber_reply();

	cucumber_wait(17, 591205868u);
	file_sync_progress_t progress;
	cucumber_reply();

	cucumber_wait(18, 3238742748u);
	EQ(file_sync_reader_step(r, 1, &progress), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(19, 3081136502u);
	CHECK(!progress.application.updates_applied && progress.frames > 0);
	cucumber_reply();

	cucumber_wait(20, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(21, 2695885914u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(22, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(23, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_writer_sequence)
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

	cucumber_wait(4, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 671137205u);
	struct packets q = take_packets(f.server.session, f.peers[1]);
	cucumber_reply();

	cucumber_wait(6, 1264489777u);
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
	cucumber_reply();

	cucumber_wait(7, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(8, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(9, 2604241668u);
	EQ(file_sync_writer_write(w, p.items[1].data, p.items[1].size),
		SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(10, 1096488591u);
	EQ(file_sync_writer_close(w, NULL), SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(11, 1906460780u);
	packets_fini(&q);
	cucumber_reply();

	cucumber_wait(12, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_fragmented_backpressure)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 4029931655u);
	sync_client_desc_t cd = { .limits.max_receive_bytes = 65536 };
	cucumber_reply();

	cucumber_wait(3, 3392614432u);
	fixture_init(&f, NULL, &cd);
	cucumber_reply();

	cucumber_wait(4, 1794765093u);
	file_sync_writer_t *w = writer(&f);
	cucumber_reply();

	cucumber_wait(5, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(6, 2259524145u);
	char *text = malloc(65536);
	cucumber_reply();

	cucumber_wait(7, 2915115124u);
	CHECK(text != NULL);
	cucumber_reply();

	cucumber_wait(8, 2440666246u);
	memset(text, 'a', 65535);
	cucumber_reply();

	cucumber_wait(9, 2263684211u);
	text[65535] = 0;
	cucumber_reply();

	cucumber_wait(10, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(11, 2878314050u);
	OK(sync_capture(f.server.session, 1, NULL));
	cucumber_reply();

	cucumber_wait(12, 3047217774u);
	OK(file_sync_writer_pump(w, UINT32_MAX, NULL));
	cucumber_reply();

	cucumber_wait(13, 1461678262u);
	text[0] = 'b';
	cucumber_reply();

	cucumber_wait(14, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(15, 1085812033u);
	OK(sync_capture(f.server.session, 2, NULL));
	cucumber_reply();

	cucumber_wait(16, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(17, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(18, 1223216047u);
	file_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(19, 3154599189u);
	EQ(file_sync_reader_pump(r, UINT32_MAX, &p), SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(20, 3887488711u);
	CHECK(p.frames > 0 && reader_status(r).pending_bytes > 0);
	cucumber_reply();

	cucumber_wait(21, 2889140635u);
	file_sync_status_t before = reader_status(r);
	cucumber_reply();

	cucumber_wait(22, 548196828u);
	EQ(file_sync_reader_pump(r, 1, &p), SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(23, 2166661372u);
	CHECK(!p.frames && !p.bytes);
	cucumber_reply();

	cucumber_wait(24, 3054045836u);
	file_sync_status_t after = reader_status(r);
	cucumber_reply();

	cucumber_wait(25, 796988521u);
	CHECK(!memcmp(&before, &after, sizeof(before)));
	cucumber_reply();

	cucumber_wait(26, 2372877457u);
	OK(file_sync_reader_step(r, 1, &p));
	cucumber_reply();

	cucumber_wait(27, 3046570383u);
	EQ(p.application.last_tick, 1);
	cucumber_reply();

	cucumber_wait(28, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(29, 2767373872u);
	const struct record *v = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].record);
	cucumber_reply();

	cucumber_wait(30, 1035792611u);
	CHECK(v && v->text[0] == 'a');
	cucumber_reply();

	cucumber_wait(31, 2372877457u);
	OK(file_sync_reader_step(r, 1, &p));
	cucumber_reply();

	cucumber_wait(32, 61622775u);
	v = ecs_get_id(f.clients[0].ecs, replica(&f.clients[0], id),
		f.clients[0].record);
	cucumber_reply();

	cucumber_wait(33, 3471748780u);
	CHECK(v && !strcmp(v->text, text));
	cucumber_reply();

	cucumber_wait(34, 3306811965u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(35, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(36, 2594712615u);
	free(text);
	cucumber_reply();

	cucumber_wait(37, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_eof_pending)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(5, 1823987497u);
	EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(6, 1641277721u);
	EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
	cucumber_reply();

	cucumber_wait(7, 3167231743u);
	CHECK(!reader_status(r).pending_bytes);
	cucumber_reply();

	cucumber_wait(8, 1806946877u);
	drain(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(9, 4234331111u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(10, 4234331111u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(11, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_borrowed_client)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(5, 1578184854u);
	OK(file_sync_reader_step(r, 1, NULL));
	cucumber_reply();

	cucumber_wait(6, 3347215536u);
	ecs_entity_t local = f.clients[0].local;
	cucumber_reply();

	cucumber_wait(7, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(8, 587561286u);
	EQ(status(f.clients[0].session, 0).entities, 1);
	cucumber_reply();

	cucumber_wait(9, 4072468712u);
	CHECK(ecs_is_alive(f.clients[0].ecs, local));
	cucumber_reply();

	cucumber_wait(10, 762874417u);
	file_sync_reader_desc_t d = { .client = f.clients[0].session };
	cucumber_reply();

	cucumber_wait(11, 4170303681u);
	EQ(file_sync_reader_open(path, &d, &r, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(12, 3377154663u);
	CHECK(!r);
	cucumber_reply();

	cucumber_wait(13, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(14, 2823810640u);
	OK(file_sync_reader_open(path, &d, &r, NULL));
	cucumber_reply();

	cucumber_wait(15, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(16, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_automatic)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2211889321u);
	sync_server_desc_t sd = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1126542434u);
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(4, 4144995094u);
	fixture_init(&f, &sd, &cd);
	cucumber_reply();

	cucumber_wait(5, 1935936823u);
	int32_t systems = ecs_count_id(f.server.ecs, EcsSystem);
	cucumber_reply();

	cucumber_wait(6, 1832628606u);
	file_sync_writer_desc_t wd = { .server = f.server.session,
		.schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(7, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(8, 663698329u);
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	cucumber_reply();

	cucumber_wait(9, 1462232291u);
	CHECK(ecs_count_id(f.server.ecs, EcsSystem) == systems + 1);
	cucumber_reply();

	cucumber_wait(10, 2763791459u);
	EQ(writer_status(w).frames_per_progress, 64);
	cucumber_reply();

	cucumber_wait(11, 2387839278u);
	ecs_entity_t e = spawn(&f.server, 7, writer_status(w).peer);
	cucumber_reply();

	cucumber_wait(12, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(13, 936011636u);
	CHECK(writer_status(w).frames >= 4);
	cucumber_reply();

	cucumber_wait(14, 1416087218u);
	EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(15, 1594972750u);
	OK(file_sync_writer_flush(w));
	cucumber_reply();

	cucumber_wait(16, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(17, 705344045u);
	EQ(ecs_count_id(f.server.ecs, EcsSystem), systems);
	cucumber_reply();

	cucumber_wait(18, 3383213442u);
	file_sync_reader_desc_t rd = { .client = f.clients[0].session,
		.schedule = SYNC_AUTOMATIC, .frames_per_progress = 1 };
	cucumber_reply();

	cucumber_wait(19, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(20, 282465175u);
	OK(file_sync_reader_open(path, &rd, &r, NULL));
	cucumber_reply();

	cucumber_wait(21, 3989101673u);
	file_sync_progress_t p = { .frames = 99 };
	cucumber_reply();

	cucumber_wait(22, 2898129887u);
	EQ(file_sync_reader_step(r, 1, &p), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(23, 2173705950u);
	CHECK(!p.frames);
	cucumber_reply();

	cucumber_wait(24, 3726191775u);
	for (unsigned i = 0; i < 32; i++) {
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		CHECK(reader_status(r).frames <= i + 1);
		if (reader_status(r).state == FILE_SYNC_ENDED)
			break;
	}
	cucumber_reply();

	cucumber_wait(25, 3882243571u);
	EQ(reader_status(r).state, FILE_SYNC_ENDED);
	cucumber_reply();

	cucumber_wait(26, 3490934443u);
	expect_position(&f.clients[0], network_id(&f.server, e), 7);
	cucumber_reply();

	cucumber_wait(27, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(28, 1052341725u);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(29, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_auto_manual_client)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1126542434u);
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(3, 3392614432u);
	fixture_init(&f, NULL, &cd);
	cucumber_reply();

	cucumber_wait(4, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(6, 2222366802u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(7, 3157189984u);
	EQ(reader_status(r).offset, 16);
	cucumber_reply();

	cucumber_wait(8, 1823987497u);
	EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(9, 1052341725u);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(10, 4234331111u);
	EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(11, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
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
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2211889321u);
	sync_server_desc_t sd = {
		.schedule = SYNC_AUTOMATIC, .send_hz = 8
	};
	cucumber_reply();

	cucumber_wait(3, 1126542434u);
	sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
	cucumber_reply();

	cucumber_wait(4, 4144995094u);
	fixture_init(&f, &sd, &cd);
	cucumber_reply();

	cucumber_wait(5, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(6, 1832628606u);
	file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC
	};
	cucumber_reply();

	cucumber_wait(7, 663698329u);
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	cucumber_reply();

	cucumber_wait(8, 2978829471u);
	ecs_entity_t e = spawn(&f.server, 1, writer_status(w).peer);
	cucumber_reply();

	cucumber_wait(9, 1251690230u);
	ecs_entity_t sys = ecs_new(f.server.ecs);
	cucumber_reply();

	cucumber_wait(10, 1809082824u);
	ecs_add_pair(f.server.ecs, sys, EcsDependsOn, EcsPostUpdate);
	cucumber_reply();

	cucumber_wait(11, 2025950846u);
	CHECK(ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_writer,
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }}
	}));
	cucumber_reply();

	cucumber_wait(12, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(13, 2947990928u);
	CHECK(writer_status(w).frames > 0);
	cucumber_reply();

	cucumber_wait(14, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(15, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(16, 3568683935u);
	file_sync_reader_desc_t rd = {
		.client = f.clients[0].session, .schedule = SYNC_AUTOMATIC
	};
	cucumber_reply();

	cucumber_wait(17, 282465175u);
	OK(file_sync_reader_open(path, &rd, &r, NULL));
	cucumber_reply();

	cucumber_wait(18, 223569866u);
	EQ(reader_status(r).frames_per_progress, 64);
	cucumber_reply();

	cucumber_wait(19, 1874780201u);
	struct phase_context ctx = {
		.fixture = &f, .id = network_id(&f.server, e)
	};
	cucumber_reply();

	cucumber_wait(20, 766118131u);
	sys = ecs_new(f.clients[0].ecs);
	cucumber_reply();

	cucumber_wait(21, 2804210537u);
	ecs_add_pair(f.clients[0].ecs, sys, EcsDependsOn, EcsPreUpdate);
	cucumber_reply();

	cucumber_wait(22, 2051046211u);
	CHECK(ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_reader, .ctx = &ctx
	}));
	cucumber_reply();

	cucumber_wait(23, 1052341725u);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(24, 253072350u);
	CHECK(ctx.read);
	cucumber_reply();

	cucumber_wait(25, 1641277721u);
	EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
	cucumber_reply();

	cucumber_wait(26, 1052341725u);
	CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(27, 3882243571u);
	EQ(reader_status(r).state, FILE_SYNC_ENDED);
	cucumber_reply();

	cucumber_wait(28, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(29, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_writer_budget)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(4, 1682557411u);
	file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC,
		.frames_per_progress = 1
	};
	cucumber_reply();

	cucumber_wait(5, 663698329u);
	OK(file_sync_writer_open(path, &wd, &w, NULL));
	cucumber_reply();

	cucumber_wait(6, 1637123758u);
	spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(7, 2878314050u);
	OK(sync_capture(f.server.session, 1, NULL));
	cucumber_reply();

	cucumber_wait(8, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(9, 1055691776u);
	EQ(writer_status(w).frames, 1);
	cucumber_reply();

	cucumber_wait(10, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(11, 1017784921u);
	EQ(writer_status(w).frames, 2);
	cucumber_reply();

	cucumber_wait(12, 282992492u);
	OK(file_sync_writer_close(w, NULL));
	cucumber_reply();

	cucumber_wait(13, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(14, 2655264011u);
	EQ(file_sync_reader_step(r, 10, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(15, 587561286u);
	EQ(status(f.clients[0].session, 0).entities, 1);
	cucumber_reply();

	cucumber_wait(16, 1690454333u);
	OK(file_sync_reader_close(r, NULL));
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_later_failure)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 1865500126u);
	size_t size;
	cucumber_reply();

	cucumber_wait(5, 1609962508u);
	unsigned char *bytes = byte_read(&size);
	cucumber_reply();

	cucumber_wait(6, 3883614572u);
	uint32_t length = (uint32_t)bytes[16] |
		(uint32_t)bytes[17] << 8 | (uint32_t)bytes[18] << 16 |
		(uint32_t)bytes[19] << 24;
	cucumber_reply();

	cucumber_wait(7, 4082956866u);
	FILE *file = fopen(path, "ab");
	cucumber_reply();

	cucumber_wait(8, 3502353606u);
	CHECK(file != NULL);
	cucumber_reply();

	cucumber_wait(9, 684619104u);
	CHECK(fwrite(bytes + 16, 1, (size_t)length + 8, file) ==
		(size_t)length + 8);
	cucumber_reply();

	cucumber_wait(10, 1730674253u);
	EQ(fclose(file), 0);
	cucumber_reply();

	cucumber_wait(11, 2181658292u);
	free(bytes);
	cucumber_reply();

	cucumber_wait(12, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(13, 1223216047u);
	file_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(14, 1678318205u);
	EQ(file_sync_reader_step(r, 10, &p), SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(15, 910344632u);
	EQ(p.application.updates_applied, 1);
	cucumber_reply();

	cucumber_wait(16, 3046570383u);
	EQ(p.application.last_tick, 1);
	cucumber_reply();

	cucumber_wait(17, 587561286u);
	EQ(status(f.clients[0].session, 0).entities, 1);
	cucumber_reply();

	cucumber_wait(18, 1162173616u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(19, 3635580124u);
	EQ(reader_status(r).state, FILE_SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(20, 3686784954u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(21, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_schema_failure)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2778804864u);
	sync_session_destroy(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(4, 970535443u);
	f.clients[0].session = NULL;
	cucumber_reply();

	cucumber_wait(5, 4051492360u);
	ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
	cucumber_reply();

	cucumber_wait(6, 977888369u);
	OK(sync_client_create(f.clients[0].ecs, NULL,
		&f.clients[0].session, NULL));
	cucumber_reply();

	cucumber_wait(7, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(8, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(9, 3315257295u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(10, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(11, 1216310945u);
	EQ(reader_status(r).fault.detail.code, SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(12, 1762076655u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SCHEMA);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

#ifdef FILE_SYNC_TESTING
TEST(file_fault_allocations)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1587200873u);
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
	cucumber_reply();

	cucumber_wait(4, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 1262149825u);
	bool done = false;
	cucumber_reply();

	cucumber_wait(6, 3452139414u);
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
	cucumber_reply();

	cucumber_wait(7, 2938458951u);
	CHECK(done);
	cucumber_reply();

	cucumber_wait(8, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

TEST(file_fault_write_commit)
{
	cucumber_wait(1, 34526060u);
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
	cucumber_reply();
}

TEST(file_fault_flush_close)
{
	cucumber_wait(1, 1594468377u);
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
	cucumber_reply();

	cucumber_wait(2, 4003401746u);
	file_sync_writer_t *w;
	cucumber_reply();

	cucumber_wait(3, 3206305684u);
	OK(file_sync_writer_open(path, NULL, &w, NULL));
	cucumber_reply();

	cucumber_wait(4, 3528535678u);
	file_sync_test_arm(FILE_SYNC_TEST_FLUSH, 0);
	cucumber_reply();

	cucumber_wait(5, 4107419938u);
	EQ(file_sync_writer_flush(w), SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(6, 2438009126u);
	file_sync_error_t first = writer_status(w).fault;
	cucumber_reply();

	cucumber_wait(7, 930891489u);
	file_sync_test_clear();
	cucumber_reply();

	cucumber_wait(8, 1704844039u);
	file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
	cucumber_reply();

	cucumber_wait(9, 835631473u);
	file_sync_error_t error;
	cucumber_reply();

	cucumber_wait(10, 1357407609u);
	EQ(file_sync_writer_close(w, &error), SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(11, 529364961u);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
	cucumber_reply();

	cucumber_wait(12, 930891489u);
	file_sync_test_clear();
	cucumber_reply();

	cucumber_wait(13, 3587812506u);
	CHECK(!memcmp(&first, &error, sizeof(first)));
	cucumber_reply();

	cucumber_wait(14, 865157258u);
	file_sync_reader_t *r;
	cucumber_reply();

	cucumber_wait(15, 3048511915u);
	OK(file_sync_reader_open(path, NULL, &r, NULL));
	cucumber_reply();

	cucumber_wait(16, 1704844039u);
	file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
	cucumber_reply();

	cucumber_wait(17, 2449987226u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(18, 529364961u);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
	cucumber_reply();

	cucumber_wait(19, 930891489u);
	file_sync_test_clear();
	cucumber_reply();
}

TEST(file_fault_read)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2536876501u);
	record_one(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 2469622166u);
	file_sync_reader_t *r = reader(&f);
	cucumber_reply();

	cucumber_wait(5, 1645962374u);
	file_sync_test_arm(FILE_SYNC_TEST_READ, 0);
	cucumber_reply();

	cucumber_wait(6, 3721220765u);
	EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(7, 2125638455u);
	CHECK(file_sync_test_hit(FILE_SYNC_TEST_READ));
	cucumber_reply();

	cucumber_wait(8, 930891489u);
	file_sync_test_clear();
	cucumber_reply();

	cucumber_wait(9, 1391812666u);
	EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
	cucumber_reply();

	cucumber_wait(10, 2449987226u);
	EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}
#endif

