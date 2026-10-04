Feature: stream contracts

@core
Scenario: stream_empty
	# When peek and apply are called before any capture or delivery
	# Then Empty queues zero views and report EMPTY without changing state.
	Given the "core" workflow "stream_empty"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_frame_view_t view;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		memset(&view, 0xff, sizeof(view));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(sync_outbox_peek(f.server.session, f.peers[0], &view), SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(!view.data && !view.size && !view.info.sequence);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!view.token.opaque[0] && !view.token.opaque[1]);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!view.token.opaque[2] && !view.token.opaque[3]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_apply_result_t p = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(sync_apply(f.clients[0].session, 1, &p), SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_borrowed_head
	# When capture appends frames while another peer is drained
	# Then Borrowed heads survive appends and activity on another peer.
	Given the "core" workflow "stream_borrowed_head"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_frame_view_t a, b;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		void *copy = malloc(a.size);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(copy != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		memcpy(copy, a.data, a.size);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(a.data == b.data && a.size == b.size);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(!memcmp(&a.token, &b.token, sizeof(a.token)));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!memcmp(a.data, copy, a.size));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		check_info_equal(&a.info, &b.info);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		free(copy);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_invalid_tokens
	# When commit receives another peer's token or a previously used token
	# Then Foreign and stale capabilities cannot release the current frame.
	Given the "core" workflow "stream_invalid_tokens"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_frame_view_t a, b, after;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[0], &a));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[1], &b));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_status_t before = status(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_outbox_commit(f.server.session, f.peers[0], b.token),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_status_t current = status(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		check_status_equal(&before, &current);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_outbox_commit(f.server.session, f.peers[0], a.token));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[0], &after));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(sync_outbox_commit(f.server.session, f.peers[0], a.token),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[0], &b));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(after.data == b.data && after.size == b.size);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(!memcmp(&after.token, &b.token, sizeof(b.token)));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_envelopes
	# When snapshot and delta envelopes are inspected in queue order
	# Then Inspection agrees with outbox envelopes and sequences start at one.
	Given the "core" workflow "stream_envelopes"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 7);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 8);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(p.count >= 4);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(p.items[0].info.kind, SYNC_FRAME_MANIFEST);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		bool begin = false, end = false, delta = false;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(begin && end && delta);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_inspect_arguments
	# When inspection receives null pointers or zero length
	# Then Inspection rejects null inputs, zero size and absent output.
	Given the "core" workflow "stream_inspect_arguments"
	When I perform operation 1:
		"""
		unsigned char byte = 0;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_frame_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		memset(&info, 0xff, sizeof(info));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_frame_inspect(NULL, 1, &info), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(!info.protocol_version && !info.kind && !info.sequence);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(!info.tick && !info.stream_id && !info.payload_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!memcmp(info.session_id.bytes, (uint8_t[16]){0}, 16));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_frame_inspect(&byte, 0, &info), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_frame_inspect(&byte, 1, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract

@core
Scenario: stream_invalid_lengths
	# When valid frames are shortened, concatenated or extended
	# Then Frame boundaries reject shortened, concatenated and trailing bytes.
	Given the "core" workflow "stream_invalid_lengths"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(p.count != 0 && p.items[0].size > 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct packet *v = &p.items[0];
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_frame_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_frame_inspect(v->data, v->size - 1, &info), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(!info.sequence && !info.payload_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		void *twice = malloc(v->size * 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(twice != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		memcpy(twice, v->data, v->size);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		memcpy((char *)twice + v->size, v->data, v->size);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(sync_frame_inspect(twice, v->size * 2, &info), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(sync_receive(f.clients[0].session, twice, v->size + 1),
		SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		free(twice);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_receive_copies
	# When the caller overwrites and frees accepted frame buffers
	# Then Receive copies accepted bytes before the caller overwrites its buffer.
	Given the "core" workflow "stream_receive_copies"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		for (size_t i = 0; i < p.count; i++) {
		OK(sync_receive(f.clients[0].session,
		p.items[i].data, p.items[i].size));
		memset(p.items[i].data, 0xcc, p.items[i].size);
		}
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		drain(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_receive_arguments
	# When receive and apply get invalid buffers, lengths or budgets
	# Then Invalid receive arguments do not change a healthy stream.
	Given the "core" workflow "stream_receive_arguments"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_session_t *c = f.clients[0].session;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_status_t before = status(c, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		unsigned char byte = 0;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_receive(c, NULL, 1), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_receive(c, &byte, 0), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		sync_status_t after = status(c, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		check_status_equal(&before, &after);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_apply_result_t p = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(sync_apply(c, 0, &p), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!p.updates_applied && !p.frames_consumed && !p.last_tick);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_oversized_frame
	# When a client receives more than its maximum frame size
	# Then Oversized input faults with LIMIT and accepts none of the buffer.
	Given the "core" workflow "stream_oversized_frame"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		void *data = calloc(65537, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(data != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(sync_receive(f.clients[0].session, data, 65537), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		sync_status_t s = status(f.clients[0].session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(s.state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(s.queued_bytes, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(s.fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		free(data);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_duplicate_sequence
	# When a previously delivered manifest is delivered again
	# Then Replaying the manifest sequence faults without publishing a snapshot.
	Given the "core" workflow "stream_duplicate_sequence"
	When I perform operation 1:
		"""
		sequence_error(0);
		"""
	Then its observed values match the contract

@core
Scenario: stream_missing_sequence
	# When a snapshot frame is skipped before the next frame arrives
	# Then A missing or out-of-order frame faults during apply, not receive.
	Given the "core" workflow "stream_missing_sequence"
	When I perform operation 1:
		"""
		sequence_error(1);
		"""
	Then its observed values match the contract

@core
Scenario: stream_foreign_stream
	# When a frame from another peer follows the accepted manifest
	# Then Frames from another peer stream cannot continue an established stream.
	Given the "core" workflow "stream_foreign_stream"
	When I perform operation 1:
		"""
		sequence_error(2);
		"""
	Then its observed values match the contract

@core
Scenario: stream_apply_budget
	# When two queued updates are applied with a budget of one
	# Then Apply counts complete updates and preserves the remainder for later
	# calls.
	Given the "core" workflow "stream_apply_budget"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_position(&f.server, e, 2);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		receive_packets(f.clients[0].session, &p);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_apply_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		OK(sync_apply(f.clients[0].session, 1, &progress));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(progress.updates_applied, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(progress.last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(progress.frames_consumed > 0);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 1);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(sync_apply(f.clients[0].session, 1, &progress));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(progress.updates_applied, 1);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(progress.last_tick, 2);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 2);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_later_failure_keeps_update
	# When a valid snapshot is followed by a repeated manifest
	# Then A later sequence failure keeps the earlier update and reports progress.
	Given the "core" workflow "stream_later_failure_keeps_update"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		receive_packets(f.clients[0].session, &p);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_receive(f.clients[0].session, p.items[0].data,
		p.items[0].size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		sync_apply_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_apply(f.clients[0].session, UINT32_MAX, &progress),
		SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(progress.updates_applied == 1 && progress.last_tick == 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_incomplete_snapshot
	# When EOF follows snapshot data without its ending frame
	# Then Snapshot chunks can be consumed without exposing an incomplete replica.
	Given the "core" workflow "stream_incomplete_snapshot"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(p.count > 2);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(p.items[p.count - 1].info.kind, SYNC_FRAME_SNAPSHOT_END);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		for (size_t i = 0; i + 1 < p.count; i++)
		OK(sync_receive(f.clients[0].session,
		p.items[i].data, p.items[i].size));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_apply_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(sync_apply(f.clients[0].session, 1, &progress), SYNC_NEED_MORE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!progress.updates_applied && progress.frames_consumed > 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		expect_absent(&f.clients[0], network_id(&f.server, e));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(sync_client_finish(f.clients[0].session), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_empty_eof
	# When EOF is declared before the initial stream exists
	# Then Finishing an uninitialized stream reports truncation and a sticky fault.
	Given the "core" workflow "stream_empty_eof"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_session_t *c = f.clients[0].session;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_client_finish(c), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_status_t before = status(c, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(before.state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(before.fault.code, SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_client_finish(c), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_status_t after = status(c, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		check_status_equal(&before, &after);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_clean_eof
	# When EOF is declared before and after draining a complete snapshot
	# Then Clean EOF is idempotent and retains final replicas for inspection.
	Given the "core" workflow "stream_clean_eof"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 3, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		receive_packets(f.clients[0].session, &p);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_client_finish(f.clients[0].session), SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		drain(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		OK(sync_client_finish(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		OK(sync_client_finish(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(sync_apply(f.clients[0].session, 1, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(sync_receive(f.clients[0].session, p.items[0].data, p.items[0].size),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 3);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_reset_reconnect
	# When a faulted client resets and receives a fresh peer snapshot
	# Then Reset after a fault discards old state and accepts a fresh snapshot.
	Given the "core" workflow "stream_reset_reconnect"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = spawn(&f.server, 3, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t old = replica(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		unsigned char garbage = 0;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_receive(f.clients[0].session, &garbage, 1), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_status_t s = status(f.clients[0].session, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(s.state, SYNC_CLIENT_WAIT_MANIFEST);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!s.entities && !s.queued_bytes && !s.assembly_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!s.fault.code && !s.stream_id && !s.last_tick);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(!ecs_is_alive(f.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		OK(sync_peer_close(f.server.session, f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		OK(sync_peer_open(f.server.session, &f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		expect_position(&f.clients[0], id, 3);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_fragmented_updates
	# When a large value arrives through a one-frame receive queue
	# Then Fragment assembly frees a small inbox and publishes only complete
	# values.
	Given the "core" workflow "stream_fragmented_updates"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, NULL, &client);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		char *text = malloc(65536);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(text != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		memset(text, 'x', 65535);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		text[65535] = 0;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		text[0] = 'y';
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		CHECK(p.count >= 2);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		free(text);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_backpressure_retry
	# When delivery fills the inbox before apply drains it
	# Then Backpressure accepts zero bytes and preserves the exact retry head.
	Given the "core" workflow "stream_backpressure_retry"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_client_desc_t client = { .limits.max_receive_bytes = 65536 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, NULL, &client);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		char *text = malloc(65536);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(text != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		memset(text, 'a', 65535);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		text[65535] = 0;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		free(text);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		bool blocked = false;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(blocked);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		drain(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 1);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: stream_stalled_peer
	# When only one peer is drained while capture continues
	# Then Stalled outboxes fault independently and keep borrowed data valid.
	Given the "core" workflow "stream_stalled_peer"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t desc = { .limits.max_pending_bytes = 65536 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &desc, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_frame_view_t head;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(sync_outbox_peek(f.server.session, f.peers[1], &head));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		void *copy = malloc(head.size);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(copy != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		memcpy(copy, head.data, head.size);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		bool faulted = false;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(faulted);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!memcmp(head.data, copy, head.size));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		sync_status_t fault = status(f.server.session, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(fault.state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		EQ(fault.fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		CHECK(fault.queued_bytes <= desc.limits.max_pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		sync_frame_view_t ignored;
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		EQ(sync_outbox_peek(f.server.session, f.peers[1], &ignored),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		OK(sync_peer_close(f.server.session, f.peers[1]));
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		free(copy);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
