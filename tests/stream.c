#include "test.h"

/* Byte offsets, wire discriminants and payload operation encodings have no
	specification yet. Version-field/opcode mutation fixtures must wait for
	that contract. These tests alter framing and order through public I/O.
*/

/* Empty queues zero views and report EMPTY without changing state. */
TEST(stream_empty)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_frame_view_t view;
	memset(&view, 0xff, sizeof(view));
	EQ(sync_outbox_peek(f.server.session, f.peers[0], &view), SYNC_EMPTY);
	CHECK(!view.data && !view.size && !view.info.sequence);
	CHECK(!view.token.opaque[0] && !view.token.opaque[1]);
	CHECK(!view.token.opaque[2] && !view.token.opaque[3]);
	sync_apply_result_t p = {99, 99, 99};
	EQ(sync_apply(f.clients[0].session, 1, &p), SYNC_EMPTY);
	CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
	fixture_fini(&f);
}

/* Borrowed heads survive appends and activity on another peer. */
TEST(stream_borrowed_head)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	sync_frame_view_t a, b;
	OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
	void *copy = malloc(a.size);
	CHECK(copy != NULL);
	memcpy(copy, a.data, a.size);
	capture(&f, 2);
	transfer(&f, 1);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
	CHECK(a.data == b.data && a.size == b.size);
	CHECK(!memcmp(&a.token, &b.token, sizeof(a.token)));
	CHECK(!memcmp(a.data, copy, a.size));
	check_info_equal(&a.info, &b.info);
	OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
	free(copy);
	fixture_fini(&f);
}

/* Foreign and stale capabilities cannot release the current frame. */
TEST(stream_invalid_tokens)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	sync_frame_view_t a, b, after;
	OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
	OK(sync_outbox_peek(f.server.session, f.peers[1], &b));
	sync_status_t before = status(f.server.session, f.peers[0]);
	EQ(sync_outbox_commit(f.server.session, f.peers[0], b.token),
		SYNC_ERR_ARGUMENT);
	sync_status_t current = status(f.server.session, f.peers[0]);
	check_status_equal(&before, &current);
	OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
	OK(sync_outbox_peek(f.server.session, f.peers[0], &after));
	EQ(sync_outbox_commit(f.server.session, f.peers[0], a.token),
		SYNC_ERR_ARGUMENT);
	OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
	CHECK(after.data == b.data && after.size == b.size);
	CHECK(!memcmp(&after.token, &b.token, sizeof(b.token)));
	fixture_fini(&f);
}

/* Inspection agrees with outbox envelopes and sequences start at one. */
TEST(stream_envelopes)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	spawn(&f.server, 1, 0);
	capture(&f, 7);
	capture(&f, 8);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	CHECK(p.count >= 4);
	EQ(p.items[0].info.kind, SYNC_FRAME_MANIFEST);
	bool begin = false, end = false, delta = false;
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
	CHECK(begin && end && delta);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Inspection rejects null inputs, zero size and absent output. */
TEST(stream_inspect_arguments)
{
	unsigned char byte = 0;
	sync_frame_info_t info;
	memset(&info, 0xff, sizeof(info));
	EQ(sync_frame_inspect(NULL, 1, &info), SYNC_ERR_ARGUMENT);
	CHECK(!info.protocol_version && !info.kind && !info.sequence);
	CHECK(!info.tick && !info.stream_id && !info.payload_bytes);
	CHECK(!memcmp(info.session_id.bytes, (uint8_t[16]){0}, 16));
	EQ(sync_frame_inspect(&byte, 0, &info), SYNC_ERR_ARGUMENT);
	EQ(sync_frame_inspect(&byte, 1, NULL), SYNC_ERR_ARGUMENT);
}

/* Frame boundaries reject shortened, concatenated and trailing bytes. */
TEST(stream_invalid_lengths)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	CHECK(p.count != 0 && p.items[0].size > 1);
	struct packet *v = &p.items[0];
	sync_frame_info_t info;
	EQ(sync_frame_inspect(v->data, v->size - 1, &info), SYNC_ERR_PROTOCOL);
	CHECK(!info.sequence && !info.payload_bytes);
	void *twice = malloc(v->size * 2);
	CHECK(twice != NULL);
	memcpy(twice, v->data, v->size);
	memcpy((char *)twice + v->size, v->data, v->size);
	EQ(sync_frame_inspect(twice, v->size * 2, &info), SYNC_ERR_PROTOCOL);
	EQ(sync_receive(f.clients[0].session, twice, v->size + 1),
		SYNC_ERR_PROTOCOL);
	EQ(status(f.clients[0].session, 0).state, SYNC_FAULTED);
	EQ(status(f.clients[0].session, 0).entities, 0);
	free(twice);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Receive copies accepted bytes before the caller overwrites its buffer. */
TEST(stream_receive_copies)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 2, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	for (size_t i = 0; i < p.count; i++) {
		OK(sync_receive(f.clients[0].session,
			p.items[i].data, p.items[i].size));
		memset(p.items[i].data, 0xcc, p.items[i].size);
	}
	packets_fini(&p);
	EQ(status(f.clients[0].session, 0).entities, 0);
	drain(f.clients[0].session);
	expect_position(&f.clients[0], network_id(&f.server, e), 2);
	fixture_fini(&f);
}

/* Invalid receive arguments do not change a healthy stream. */
TEST(stream_receive_arguments)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_session_t *c = f.clients[0].session;
	sync_status_t before = status(c, 0);
	unsigned char byte = 0;
	EQ(sync_receive(c, NULL, 1), SYNC_ERR_ARGUMENT);
	EQ(sync_receive(c, &byte, 0), SYNC_ERR_ARGUMENT);
	sync_status_t after = status(c, 0);
	check_status_equal(&before, &after);
	sync_apply_result_t p = {99, 99, 99};
	EQ(sync_apply(c, 0, &p), SYNC_ERR_ARGUMENT);
	CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
	fixture_fini(&f);
}

/* Oversized input faults with LIMIT and accepts none of the buffer. */
TEST(stream_oversized_frame)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	void *data = calloc(65537, 1);
	CHECK(data != NULL);
	EQ(sync_receive(f.clients[0].session, data, 65537), SYNC_ERR_LIMIT);
	sync_status_t s = status(f.clients[0].session, 0);
	EQ(s.state, SYNC_FAULTED);
	EQ(s.queued_bytes, 0);
	EQ(s.fault.code, SYNC_ERR_LIMIT);
	free(data);
	fixture_fini(&f);
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
TEST(stream_duplicate_sequence) { sequence_error(0); }
/* A missing or out-of-order frame faults during apply, not receive. */
TEST(stream_missing_sequence) { sequence_error(1); }
/* Frames from another peer stream cannot continue an established stream. */
TEST(stream_foreign_stream) { sequence_error(2); }

/* Apply counts complete updates and preserves the remainder for later calls. */
TEST(stream_apply_budget)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	set_position(&f.server, e, 2);
	capture(&f, 2);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	sync_apply_result_t progress;
	OK(sync_apply(f.clients[0].session, 1, &progress));
	EQ(progress.updates_applied, 1);
	EQ(progress.last_tick, 1);
	CHECK(progress.frames_consumed > 0);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	OK(sync_apply(f.clients[0].session, 1, &progress));
	EQ(progress.updates_applied, 1);
	EQ(progress.last_tick, 2);
	expect_position(&f.clients[0], network_id(&f.server, e), 2);
	packets_fini(&p);
	fixture_fini(&f);
}

/* A later sequence failure keeps the earlier update and reports progress. */
TEST(stream_later_failure_keeps_update)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	OK(sync_receive(f.clients[0].session, p.items[0].data,
		p.items[0].size));
	sync_apply_result_t progress;
	EQ(sync_apply(f.clients[0].session, UINT32_MAX, &progress),
		SYNC_ERR_SEQUENCE);
	CHECK(progress.updates_applied == 1 && progress.last_tick == 1);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Snapshot chunks can be consumed without exposing an incomplete replica. */
TEST(stream_incomplete_snapshot)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	CHECK(p.count > 2);
	EQ(p.items[p.count - 1].info.kind, SYNC_FRAME_SNAPSHOT_END);
	for (size_t i = 0; i + 1 < p.count; i++)
		OK(sync_receive(f.clients[0].session,
			p.items[i].data, p.items[i].size));
	sync_apply_result_t progress;
	EQ(sync_apply(f.clients[0].session, 1, &progress), SYNC_NEED_MORE);
	CHECK(!progress.updates_applied && progress.frames_consumed > 0);
	expect_absent(&f.clients[0], network_id(&f.server, e));
	EQ(sync_client_finish(f.clients[0].session), SYNC_ERR_TRUNCATED);
	EQ(status(f.clients[0].session, 0).entities, 0);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Finishing an uninitialized stream reports truncation and a sticky fault. */
TEST(stream_empty_eof)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_session_t *c = f.clients[0].session;
	EQ(sync_client_finish(c), SYNC_ERR_TRUNCATED);
	sync_status_t before = status(c, 0);
	EQ(before.state, SYNC_FAULTED);
	EQ(before.fault.code, SYNC_ERR_TRUNCATED);
	EQ(sync_client_finish(c), SYNC_ERR_STATE);
	sync_status_t after = status(c, 0);
	check_status_equal(&before, &after);
	fixture_fini(&f);
}

/* Clean EOF is idempotent and retains final replicas for inspection. */
TEST(stream_clean_eof)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 3, 0);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	EQ(sync_client_finish(f.clients[0].session), SYNC_BACKPRESSURE);
	drain(f.clients[0].session);
	OK(sync_client_finish(f.clients[0].session));
	OK(sync_client_finish(f.clients[0].session));
	EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_END);
	EQ(sync_receive(f.clients[0].session, p.items[0].data, p.items[0].size),
		SYNC_ERR_STATE);
	expect_position(&f.clients[0], network_id(&f.server, e), 3);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Reset after a fault discards old state and accepts a fresh snapshot. */
TEST(stream_reset_reconnect)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 3, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, e);
	ecs_entity_t old = replica(&f.clients[0], id);
	unsigned char garbage = 0;
	EQ(sync_receive(f.clients[0].session, &garbage, 1), SYNC_ERR_PROTOCOL);
	OK(sync_client_reset(f.clients[0].session));
	OK(sync_client_reset(f.clients[0].session));
	sync_status_t s = status(f.clients[0].session, 0);
	EQ(s.state, SYNC_CLIENT_WAIT_MANIFEST);
	CHECK(!s.entities && !s.queued_bytes && !s.assembly_bytes);
	CHECK(!s.fault.code && !s.stream_id && !s.last_tick);
	CHECK(!ecs_is_alive(f.clients[0].ecs, old));
	OK(sync_peer_close(f.server.session, f.peers[0]));
	OK(sync_peer_open(f.server.session, &f.peers[0]));
	capture(&f, 2);
	transfer(&f, 0);
	expect_position(&f.clients[0], id, 3);
	fixture_fini(&f);
}

/* Fragment assembly frees a small inbox and publishes only complete values. */
TEST(stream_fragmented_updates)
{
	struct fixture f;
	sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
	fixture_init(&f, NULL, &client);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char *text = malloc(65536);
	CHECK(text != NULL);
	memset(text, 'x', 65535);
	text[65535] = 0;
	set_record(&f.server, e, (struct record){ .text = text });
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, e);
	text[0] = 'y';
	set_record(&f.server, e, (struct record){ .text = text });
	capture(&f, 2);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	CHECK(p.count >= 2);
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
	free(text);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Backpressure accepts zero bytes and preserves the exact retry head. */
TEST(stream_backpressure_retry)
{
	struct fixture f;
	sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
	fixture_init(&f, NULL, &client);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	char *text = malloc(65536);
	CHECK(text != NULL);
	memset(text, 'a', 65535);
	text[65535] = 0;
	set_record(&f.server, e, (struct record){ .text = text });
	free(text);
	capture(&f, 1);
	bool blocked = false;
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
	CHECK(blocked);
	drain(f.clients[0].session);
	transfer(&f, 0);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	fixture_fini(&f);
}

/* Stalled outboxes fault independently and keep borrowed data valid. */
TEST(stream_stalled_peer)
{
	struct fixture f;
	sync_server_desc_t desc = { .limits.max_pending_bytes = 65536 };
	fixture_init(&f, &desc, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	sync_frame_view_t head;
	OK(sync_outbox_peek(f.server.session, f.peers[1], &head));
	void *copy = malloc(head.size);
	CHECK(copy != NULL);
	memcpy(copy, head.data, head.size);
	bool faulted = false;
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
	CHECK(faulted);
	CHECK(!memcmp(head.data, copy, head.size));
	sync_status_t fault = status(f.server.session, f.peers[1]);
	EQ(fault.state, SYNC_FAULTED);
	EQ(fault.fault.code, SYNC_ERR_LIMIT);
	CHECK(fault.queued_bytes <= desc.limits.max_pending_bytes);
	sync_frame_view_t ignored;
	EQ(sync_outbox_peek(f.server.session, f.peers[1], &ignored),
		SYNC_ERR_STATE);
	OK(sync_peer_close(f.server.session, f.peers[1]));
	free(copy);
	fixture_fini(&f);
}
