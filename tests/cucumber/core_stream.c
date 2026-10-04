/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

/* Byte offsets, wire discriminants and payload operation encodings have no
	specification yet. Version-field/opcode mutation fixtures must wait for
	that contract. These tests alter framing and order through public I/O.
*/

/* Empty queues zero views and report EMPTY without changing state. */
TEST(stream_empty)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1137963775u);
	sync_frame_view_t view;
	cucumber_reply();

	cucumber_wait(4, 1584131405u);
	memset(&view, 0xff, sizeof(view));
	cucumber_reply();

	cucumber_wait(5, 4031623488u);
	EQ(sync_outbox_peek(f.server.session, f.peers[0], &view), SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(6, 2414824621u);
	CHECK(!view.data && !view.size && !view.info.sequence);
	cucumber_reply();

	cucumber_wait(7, 3361238844u);
	CHECK(!view.token.opaque[0] && !view.token.opaque[1]);
	cucumber_reply();

	cucumber_wait(8, 2757629439u);
	CHECK(!view.token.opaque[2] && !view.token.opaque[3]);
	cucumber_reply();

	cucumber_wait(9, 3657735025u);
	sync_apply_result_t p = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(10, 944245804u);
	EQ(sync_apply(f.clients[0].session, 1, &p), SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(11, 746868619u);
	CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Borrowed heads survive appends and activity on another peer. */
TEST(stream_borrowed_head)
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

	cucumber_wait(4, 2084419677u);
	sync_frame_view_t a, b;
	cucumber_reply();

	cucumber_wait(5, 1672346370u);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
	cucumber_reply();

	cucumber_wait(6, 2090501831u);
	void *copy = malloc(a.size);
	cucumber_reply();

	cucumber_wait(7, 1501162247u);
	CHECK(copy != NULL);
	cucumber_reply();

	cucumber_wait(8, 129312u);
	memcpy(copy, a.data, a.size);
	cucumber_reply();

	cucumber_wait(9, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(10, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(11, 1897420012u);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
	cucumber_reply();

	cucumber_wait(12, 642784247u);
	CHECK(a.data == b.data && a.size == b.size);
	cucumber_reply();

	cucumber_wait(13, 976469367u);
	CHECK(!memcmp(&a.token, &b.token, sizeof(a.token)));
	cucumber_reply();

	cucumber_wait(14, 2015931789u);
	CHECK(!memcmp(a.data, copy, a.size));
	cucumber_reply();

	cucumber_wait(15, 1881215879u);
	check_info_equal(&a.info, &b.info);
	cucumber_reply();

	cucumber_wait(16, 443077298u);
	OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
	cucumber_reply();

	cucumber_wait(17, 61324569u);
	free(copy);
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Foreign and stale capabilities cannot release the current frame. */
TEST(stream_invalid_tokens)
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

	cucumber_wait(4, 1005926126u);
	sync_frame_view_t a, b, after;
	cucumber_reply();

	cucumber_wait(5, 1672346370u);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
	cucumber_reply();

	cucumber_wait(6, 1717780655u);
	OK(sync_outbox_peek(f.server.session, f.peers[1], &b));
	cucumber_reply();

	cucumber_wait(7, 2861474293u);
	sync_status_t before = status(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(8, 4271976275u);
	EQ(sync_outbox_commit(f.server.session, f.peers[0], b.token),
		SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 4241409412u);
	sync_status_t current = status(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(10, 2009753451u);
	check_status_equal(&before, &current);
	cucumber_reply();

	cucumber_wait(11, 443077298u);
	OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
	cucumber_reply();

	cucumber_wait(12, 2069460292u);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &after));
	cucumber_reply();

	cucumber_wait(13, 1537819711u);
	EQ(sync_outbox_commit(f.server.session, f.peers[0], a.token),
		SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(14, 1897420012u);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
	cucumber_reply();

	cucumber_wait(15, 3330189258u);
	CHECK(after.data == b.data && after.size == b.size);
	cucumber_reply();

	cucumber_wait(16, 2413390376u);
	CHECK(!memcmp(&after.token, &b.token, sizeof(b.token)));
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Inspection agrees with outbox envelopes and sequences start at one. */
TEST(stream_envelopes)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1637123758u);
	spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1274976817u);
	capture(&f, 7);
	cucumber_reply();

	cucumber_wait(5, 1084415244u);
	capture(&f, 8);
	cucumber_reply();

	cucumber_wait(6, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(7, 4290941720u);
	CHECK(p.count >= 4);
	cucumber_reply();

	cucumber_wait(8, 2543220715u);
	EQ(p.items[0].info.kind, SYNC_FRAME_MANIFEST);
	cucumber_reply();

	cucumber_wait(9, 4004190144u);
	bool begin = false, end = false, delta = false;
	cucumber_reply();

	cucumber_wait(10, 1368735344u);
	for (size_t i = 0; i < p.count; i++) {
		sync_frame_info_t info;
		OK(sync_frame_inspect(p.items[i].data, p.items[i].size, &info));
		check_info_equal(&info, &p.items[i].info);
		CHECK(info.sequence == i + 1);
		CHECK(info.payload_bytes < p.items[i].size);
		CHECK(info.stream_id == p.items[0].info.stream_id);
		if (info.kind == SYNC_FRAME_SNAPSHOT_BEGIN)
			begin = true;
		if (info.kind == SYNC_FRAME_SNAPSHOT_END)
			end = true;
		if (info.kind == SYNC_FRAME_DELTA) {
			delta = true;
			CHECK(info.tick == 8);
		} else {
			CHECK(info.tick == 7);
		}
	}
	cucumber_reply();

	cucumber_wait(11, 1281914358u);
	CHECK(begin && end && delta);
	cucumber_reply();

	cucumber_wait(12, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Inspection rejects null inputs, zero size and absent output. */
TEST(stream_inspect_arguments)
{
	cucumber_wait(1, 902623069u);
	unsigned char byte = 0;
	cucumber_reply();

	cucumber_wait(2, 393334897u);
	sync_frame_info_t info;
	cucumber_reply();

	cucumber_wait(3, 2965940602u);
	memset(&info, 0xff, sizeof(info));
	cucumber_reply();

	cucumber_wait(4, 1687593670u);
	EQ(sync_frame_inspect(NULL, 1, &info), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 2246768278u);
	CHECK(!info.protocol_version && !info.kind && !info.sequence);
	cucumber_reply();

	cucumber_wait(6, 1336541014u);
	CHECK(!info.tick && !info.stream_id && !info.payload_bytes);
	cucumber_reply();

	cucumber_wait(7, 3149230406u);
	CHECK(!memcmp(info.session_id.bytes, (uint8_t[16]){0}, 16));
	cucumber_reply();

	cucumber_wait(8, 3325338834u);
	EQ(sync_frame_inspect(&byte, 0, &info), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 51378170u);
	EQ(sync_frame_inspect(&byte, 1, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();
}

/* Frame boundaries reject shortened, concatenated and trailing bytes. */
TEST(stream_invalid_lengths)
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

	cucumber_wait(5, 2293812295u);
	CHECK(p.count != 0 && p.items[0].size > 1);
	cucumber_reply();

	cucumber_wait(6, 1118858115u);
	struct packet *v = &p.items[0];
	cucumber_reply();

	cucumber_wait(7, 393334897u);
	sync_frame_info_t info;
	cucumber_reply();

	cucumber_wait(8, 166680288u);
	EQ(sync_frame_inspect(v->data, v->size - 1, &info), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(9, 1107920160u);
	CHECK(!info.sequence && !info.payload_bytes);
	cucumber_reply();

	cucumber_wait(10, 1343857071u);
	void *twice = malloc(v->size * 2);
	cucumber_reply();

	cucumber_wait(11, 2920739654u);
	CHECK(twice != NULL);
	cucumber_reply();

	cucumber_wait(12, 3919083549u);
	memcpy(twice, v->data, v->size);
	cucumber_reply();

	cucumber_wait(13, 1285964980u);
	memcpy((char *)twice + v->size, v->data, v->size);
	cucumber_reply();

	cucumber_wait(14, 1138022102u);
	EQ(sync_frame_inspect(twice, v->size * 2, &info), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(15, 3157781487u);
	EQ(sync_receive(f.clients[0].session, twice, v->size + 1),
		SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(16, 436514399u);
	EQ(status(f.clients[0].session, 0).state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(17, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(18, 431844637u);
	free(twice);
	cucumber_reply();

	cucumber_wait(19, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(20, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Receive copies accepted bytes before the caller overwrites its buffer. */
TEST(stream_receive_copies)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2468513887u);
	ecs_entity_t e = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(6, 1598679674u);
	for (size_t i = 0; i < p.count; i++) {
		OK(sync_receive(f.clients[0].session,
			p.items[i].data, p.items[i].size));
		memset(p.items[i].data, 0xcc, p.items[i].size);
	}
	cucumber_reply();

	cucumber_wait(7, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(8, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(9, 1806946877u);
	drain(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(10, 3604524096u);
	expect_position(&f.clients[0], network_id(&f.server, e), 2);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Invalid receive arguments do not change a healthy stream. */
TEST(stream_receive_arguments)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2319369337u);
	sync_session_t *c = f.clients[0].session;
	cucumber_reply();

	cucumber_wait(4, 879449520u);
	sync_status_t before = status(c, 0);
	cucumber_reply();

	cucumber_wait(5, 902623069u);
	unsigned char byte = 0;
	cucumber_reply();

	cucumber_wait(6, 641116809u);
	EQ(sync_receive(c, NULL, 1), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 607482028u);
	EQ(sync_receive(c, &byte, 0), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(8, 963395714u);
	sync_status_t after = status(c, 0);
	cucumber_reply();

	cucumber_wait(9, 4118347807u);
	check_status_equal(&before, &after);
	cucumber_reply();

	cucumber_wait(10, 3657735025u);
	sync_apply_result_t p = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(11, 4095281918u);
	EQ(sync_apply(c, 0, &p), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(12, 746868619u);
	CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Oversized input faults with LIMIT and accepts none of the buffer. */
TEST(stream_oversized_frame)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 280812745u);
	void *data = calloc(65537, 1);
	cucumber_reply();

	cucumber_wait(4, 3512816518u);
	CHECK(data != NULL);
	cucumber_reply();

	cucumber_wait(5, 2116253063u);
	EQ(sync_receive(f.clients[0].session, data, 65537), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(6, 1203789371u);
	sync_status_t s = status(f.clients[0].session, 0);
	cucumber_reply();

	cucumber_wait(7, 1163676783u);
	EQ(s.state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(8, 459683004u);
	EQ(s.queued_bytes, 0);
	cucumber_reply();

	cucumber_wait(9, 525196998u);
	EQ(s.fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(10, 1019565663u);
	free(data);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

static void sequence_error(unsigned kind)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	spawn(&f.server, 1, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	struct packets q = take_packets(f.server.session, f.peers[1]);
	CHECK(p.count >= 3 && q.count >= 3);
	sync_session_t *c = f.clients[0].session;
	OK(sync_receive(c, p.items[0].data, p.items[0].size));
	const struct packet *next = &p.items[kind == 0 ? 0 : 2];
	if (kind == 2)
		next = &q.items[1];
	OK(sync_receive(c, next->data, next->size));
	sync_apply_result_t progress;
	EQ(sync_apply(c, UINT32_MAX, &progress), SYNC_ERR_SEQUENCE);
	CHECK(!progress.updates_applied && !progress.last_tick);
	EQ(status(c, 0).state, SYNC_FAULTED);
	EQ(status(c, 0).entities, 0);
	packets_fini(&q);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Replaying the manifest sequence faults without publishing a snapshot. */
TEST(stream_duplicate_sequence) {
	cucumber_wait(1, 3578893u);
	sequence_error(0);
	cucumber_reply();
}
/* A missing or out-of-order frame faults during apply, not receive. */
TEST(stream_missing_sequence) {
	cucumber_wait(1, 32831034u);
	sequence_error(1);
	cucumber_reply();
}
/* Frames from another peer stream cannot continue an established stream. */
TEST(stream_foreign_stream) {
	cucumber_wait(1, 62015587u);
	sequence_error(2);
	cucumber_reply();
}

/* Apply counts complete updates and preserves the remainder for later calls. */
TEST(stream_apply_budget)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 1801865358u);
	set_position(&f.server, e, 2);
	cucumber_reply();

	cucumber_wait(6, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(7, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(8, 2011819897u);
	receive_packets(f.clients[0].session, &p);
	cucumber_reply();

	cucumber_wait(9, 3607501460u);
	sync_apply_result_t progress;
	cucumber_reply();

	cucumber_wait(10, 3810000145u);
	OK(sync_apply(f.clients[0].session, 1, &progress));
	cucumber_reply();

	cucumber_wait(11, 2177484889u);
	EQ(progress.updates_applied, 1);
	cucumber_reply();

	cucumber_wait(12, 2883783614u);
	EQ(progress.last_tick, 1);
	cucumber_reply();

	cucumber_wait(13, 3523242826u);
	CHECK(progress.frames_consumed > 0);
	cucumber_reply();

	cucumber_wait(14, 3567129113u);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	cucumber_reply();

	cucumber_wait(15, 3810000145u);
	OK(sync_apply(f.clients[0].session, 1, &progress));
	cucumber_reply();

	cucumber_wait(16, 2177484889u);
	EQ(progress.updates_applied, 1);
	cucumber_reply();

	cucumber_wait(17, 2846209511u);
	EQ(progress.last_tick, 2);
	cucumber_reply();

	cucumber_wait(18, 3604524096u);
	expect_position(&f.clients[0], network_id(&f.server, e), 2);
	cucumber_reply();

	cucumber_wait(19, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(20, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* A later sequence failure keeps the earlier update and reports progress. */
TEST(stream_later_failure_keeps_update)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(6, 2011819897u);
	receive_packets(f.clients[0].session, &p);
	cucumber_reply();

	cucumber_wait(7, 2968788154u);
	OK(sync_receive(f.clients[0].session, p.items[0].data,
		p.items[0].size));
	cucumber_reply();

	cucumber_wait(8, 3607501460u);
	sync_apply_result_t progress;
	cucumber_reply();

	cucumber_wait(9, 1435751546u);
	EQ(sync_apply(f.clients[0].session, UINT32_MAX, &progress),
		SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(10, 3170060256u);
	CHECK(progress.updates_applied == 1 && progress.last_tick == 1);
	cucumber_reply();

	cucumber_wait(11, 3567129113u);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	cucumber_reply();

	cucumber_wait(12, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Snapshot chunks can be consumed without exposing an incomplete replica. */
TEST(stream_incomplete_snapshot)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(6, 1939340748u);
	CHECK(p.count > 2);
	cucumber_reply();

	cucumber_wait(7, 1420697243u);
	EQ(p.items[p.count - 1].info.kind, SYNC_FRAME_SNAPSHOT_END);
	cucumber_reply();

	cucumber_wait(8, 3901009995u);
	for (size_t i = 0; i + 1 < p.count; i++)
		OK(sync_receive(f.clients[0].session,
			p.items[i].data, p.items[i].size));
	cucumber_reply();

	cucumber_wait(9, 3607501460u);
	sync_apply_result_t progress;
	cucumber_reply();

	cucumber_wait(10, 2217312359u);
	EQ(sync_apply(f.clients[0].session, 1, &progress), SYNC_NEED_MORE);
	cucumber_reply();

	cucumber_wait(11, 1874196462u);
	CHECK(!progress.updates_applied && progress.frames_consumed > 0);
	cucumber_reply();

	cucumber_wait(12, 4103346256u);
	expect_absent(&f.clients[0], network_id(&f.server, e));
	cucumber_reply();

	cucumber_wait(13, 40653u);
	EQ(sync_client_finish(f.clients[0].session), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(14, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(15, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(16, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Finishing an uninitialized stream reports truncation and a sticky fault. */
TEST(stream_empty_eof)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2319369337u);
	sync_session_t *c = f.clients[0].session;
	cucumber_reply();

	cucumber_wait(4, 3869866024u);
	EQ(sync_client_finish(c), SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(5, 879449520u);
	sync_status_t before = status(c, 0);
	cucumber_reply();

	cucumber_wait(6, 2966815892u);
	EQ(before.state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(7, 4225498900u);
	EQ(before.fault.code, SYNC_ERR_TRUNCATED);
	cucumber_reply();

	cucumber_wait(8, 2929855738u);
	EQ(sync_client_finish(c), SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(9, 963395714u);
	sync_status_t after = status(c, 0);
	cucumber_reply();

	cucumber_wait(10, 4118347807u);
	check_status_equal(&before, &after);
	cucumber_reply();

	cucumber_wait(11, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Clean EOF is idempotent and retains final replicas for inspection. */
TEST(stream_clean_eof)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1484676090u);
	ecs_entity_t e = spawn(&f.server, 3, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(6, 2011819897u);
	receive_packets(f.clients[0].session, &p);
	cucumber_reply();

	cucumber_wait(7, 151564267u);
	EQ(sync_client_finish(f.clients[0].session), SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(8, 1806946877u);
	drain(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(9, 2252433789u);
	OK(sync_client_finish(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(10, 2252433789u);
	OK(sync_client_finish(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(11, 2627748879u);
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_END);
	cucumber_reply();

	cucumber_wait(12, 1256160446u);
	EQ(sync_receive(f.clients[0].session, p.items[0].data, p.items[0].size),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(13, 3608856183u);
	expect_position(&f.clients[0], network_id(&f.server, e), 3);
	cucumber_reply();

	cucumber_wait(14, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(15, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Reset after a fault discards old state and accepts a fresh snapshot. */
TEST(stream_reset_reconnect)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1484676090u);
	ecs_entity_t e = spawn(&f.server, 3, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(7, 1432765860u);
	ecs_entity_t old = replica(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(8, 460649552u);
	unsigned char garbage = 0;
	cucumber_reply();

	cucumber_wait(9, 3056474714u);
	EQ(sync_receive(f.clients[0].session, &garbage, 1), SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(10, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(11, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(12, 1203789371u);
	sync_status_t s = status(f.clients[0].session, 0);
	cucumber_reply();

	cucumber_wait(13, 2067853766u);
	EQ(s.state, SYNC_CLIENT_WAIT_MANIFEST);
	cucumber_reply();

	cucumber_wait(14, 4196266426u);
	CHECK(!s.entities && !s.queued_bytes && !s.assembly_bytes);
	cucumber_reply();

	cucumber_wait(15, 1556948760u);
	CHECK(!s.fault.code && !s.stream_id && !s.last_tick);
	cucumber_reply();

	cucumber_wait(16, 130567596u);
	CHECK(!ecs_is_alive(f.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(17, 3176110705u);
	OK(sync_peer_close(f.server.session, f.peers[0]));
	cucumber_reply();

	cucumber_wait(18, 1631208582u);
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	cucumber_reply();

	cucumber_wait(19, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(20, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(21, 2004647210u);
	expect_position(&f.clients[0], id, 3);
	cucumber_reply();

	cucumber_wait(22, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Fragment assembly frees a small inbox and publishes only complete values. */
TEST(stream_fragmented_updates)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2714231960u);
	sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
	cucumber_reply();

	cucumber_wait(3, 2289245690u);
	fixture_init(&f, NULL, &client);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 2259524145u);
	char *text = malloc(65536);
	cucumber_reply();

	cucumber_wait(6, 2915115124u);
	CHECK(text != NULL);
	cucumber_reply();

	cucumber_wait(7, 1153590984u);
	memset(text, 'x', 65535);
	cucumber_reply();

	cucumber_wait(8, 2263684211u);
	text[65535] = 0;
	cucumber_reply();

	cucumber_wait(9, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(10, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(11, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(12, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(13, 1198275623u);
	text[0] = 'y';
	cucumber_reply();

	cucumber_wait(14, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(15, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(16, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(17, 4216319914u);
	CHECK(p.count >= 2);
	cucumber_reply();

	cucumber_wait(18, 205105184u);
	for (size_t i = 0; i < p.count; i++) {
		EQ(p.items[i].info.kind, SYNC_FRAME_FRAGMENT);
		OK(sync_receive(f.clients[0].session,
			p.items[i].data, p.items[i].size));
		sync_apply_result_t progress;
		sync_result_t r = sync_apply(f.clients[0].session, 1,
			&progress);
		const struct record *value = ecs_get_id(f.clients[0].ecs,
			replica(&f.clients[0], id), f.clients[0].record);
		CHECK(value && value->text);
		if (i + 1 < p.count) {
			EQ(r, SYNC_NEED_MORE);
			CHECK(!progress.updates_applied &&
				value->text[0] == 'x');
		} else {
			OK(r);
			EQ(progress.updates_applied, 1);
			CHECK(!strcmp(value->text, text));
		}
		CHECK(progress.frames_consumed > 0);
		EQ(status(f.clients[0].session, 0).queued_bytes, 0);
	}
	cucumber_reply();

	cucumber_wait(19, 2594712615u);
	free(text);
	cucumber_reply();

	cucumber_wait(20, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(21, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Backpressure accepts zero bytes and preserves the exact retry head. */
TEST(stream_backpressure_retry)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2714231960u);
	sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
	cucumber_reply();

	cucumber_wait(3, 2289245690u);
	fixture_init(&f, NULL, &client);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 2259524145u);
	char *text = malloc(65536);
	cucumber_reply();

	cucumber_wait(6, 2915115124u);
	CHECK(text != NULL);
	cucumber_reply();

	cucumber_wait(7, 2440666246u);
	memset(text, 'a', 65535);
	cucumber_reply();

	cucumber_wait(8, 2263684211u);
	text[65535] = 0;
	cucumber_reply();

	cucumber_wait(9, 2866049360u);
	set_record(&f.server, e, (struct record){ .text = text });
	cucumber_reply();

	cucumber_wait(10, 2594712615u);
	free(text);
	cucumber_reply();

	cucumber_wait(11, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(12, 3210255158u);
	bool blocked = false;
	cucumber_reply();

	cucumber_wait(13, 3808587678u);
	for (unsigned i = 0; i < 10000; i++) {
		sync_frame_view_t v, again;
		OK(sync_outbox_peek(f.server.session, f.peers[0], &v));
		sync_status_t before = status(f.clients[0].session, 0);
		sync_result_t r = sync_receive(f.clients[0].session, v.data,
			v.size);
		if (r == SYNC_BACKPRESSURE) {
			sync_status_t after = status(f.clients[0].session, 0);
			check_status_equal(&before, &after);
			OK(sync_outbox_peek(f.server.session, f.peers[0],
				&again));
			CHECK(v.data == again.data && v.size == again.size);
			CHECK(!memcmp(&v.token, &again.token, sizeof(v.token)));
			blocked = true;
			break;
		}
		OK(r);
		OK(sync_outbox_commit(f.server.session, f.peers[0], v.token));
	}
	cucumber_reply();

	cucumber_wait(14, 1859086435u);
	CHECK(blocked);
	cucumber_reply();

	cucumber_wait(15, 1806946877u);
	drain(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(16, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(17, 3567129113u);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Stalled outboxes fault independently and keep borrowed data valid. */
TEST(stream_stalled_peer)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 2780564526u);
	sync_server_desc_t desc = { .limits.max_pending_bytes = 65536 };
	cucumber_reply();

	cucumber_wait(3, 4160598831u);
	fixture_init(&f, &desc, NULL);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(7, 2956168682u);
	sync_frame_view_t head;
	cucumber_reply();

	cucumber_wait(8, 3966840395u);
	OK(sync_outbox_peek(f.server.session, f.peers[1], &head));
	cucumber_reply();

	cucumber_wait(9, 3499726335u);
	void *copy = malloc(head.size);
	cucumber_reply();

	cucumber_wait(10, 1501162247u);
	CHECK(copy != NULL);
	cucumber_reply();

	cucumber_wait(11, 1258504368u);
	memcpy(copy, head.data, head.size);
	cucumber_reply();

	cucumber_wait(12, 2048845042u);
	bool faulted = false;
	cucumber_reply();

	cucumber_wait(13, 3255873823u);
	for (uint64_t tick = 2; tick < 10000; tick++) {
		set_position(&f.server, e, (float)tick);
		sync_capture_result_t progress;
		sync_result_t r = sync_capture(f.server.session, tick,
			&progress);
		CHECK(r == SYNC_OK || r == SYNC_PARTIAL);
		transfer(&f, 0);
		expect_position(&f.clients[0], network_id(&f.server, e),
			(float)tick);
		if (r == SYNC_PARTIAL) {
			EQ(progress.peers_faulted, 1);
			EQ(progress.peers_queued, 1);
			faulted = true;
			break;
		}
	}
	cucumber_reply();

	cucumber_wait(14, 2020498872u);
	CHECK(faulted);
	cucumber_reply();

	cucumber_wait(15, 1949013717u);
	CHECK(!memcmp(head.data, copy, head.size));
	cucumber_reply();

	cucumber_wait(16, 3848971567u);
	sync_status_t fault = status(f.server.session, f.peers[1]);
	cucumber_reply();

	cucumber_wait(17, 3797551683u);
	EQ(fault.state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(18, 2444079171u);
	EQ(fault.fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(19, 636557382u);
	CHECK(fault.queued_bytes <= desc.limits.max_pending_bytes);
	cucumber_reply();

	cucumber_wait(20, 2950742092u);
	sync_frame_view_t ignored;
	cucumber_reply();

	cucumber_wait(21, 2465143909u);
	EQ(sync_outbox_peek(f.server.session, f.peers[1], &ignored),
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(22, 2150609857u);
	OK(sync_peer_close(f.server.session, f.peers[1]));
	cucumber_reply();

	cucumber_wait(23, 61324569u);
	free(copy);
	cucumber_reply();

	cucumber_wait(24, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}
