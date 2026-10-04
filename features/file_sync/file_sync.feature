Feature: FileSync recording and replay

@file_sync
Scenario: file_session_info
	# When session information is queried repeatedly
	# Then Session integration information is copied without mutation
	Given the "file_sync" workflow "file_session_info"
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
		sync_session_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(sync_get_session_info(f.server.session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(info.world == f.server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(info.ids.sync == f.server.ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(info.role, SYNC_ROLE_SERVER);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(info.schedule, SYNC_MANUAL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(info.limits.max_frame_bytes, 65536);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(info.limits.max_peers, 64);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		info.limits.max_frame_bytes = 1;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		OK(sync_get_session_info(f.server.session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(info.limits.max_frame_bytes, 65536);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(sync_get_session_info(NULL, &info), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!info.world && !info.ids.sync && !info.role);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(sync_get_session_info(f.server.session, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_roundtrip
	# When a private recording peer captures a snapshot and delta
	# Then Replay preserves values, references, recipient visibility and peer
	# cleanup
	Given the "file_sync" workflow "file_roundtrip"
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
		file_sync_writer_t *w = writer(&f);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t peer = writer_status(w).peer;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(peer != f.peers[0] && peer != f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_entity_t a = spawn(&f.server, 1, peer);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t b = spawn(&f.server, 2, peer);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t hidden = spawn(&f.server, 3, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_set_id(f.server.ecs, a, f.server.reference, sizeof(b), &b);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		set_record(&f.server, a, (struct record){ .text = "owned" });
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_capture(f.server.session, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		file_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		OK(file_sync_writer_pump(w, UINT32_MAX, &p));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(p.frames >= 4 && p.bytes > 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(file_sync_writer_pump(w, 1, &p), SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(!p.frames && !p.bytes);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		set_position(&f.server, a, 9);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		OK(sync_capture(f.server.session, 2, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		CHECK(!ecs_is_alive(f.server.ecs, peer));
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		CHECK(ecs_is_alive(f.server.ecs, a));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		CHECK(!ecs_has_pair(f.server.ecs, a, f.server.ids.replicate_to, peer));
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		OK(file_sync_reader_step(r, 1, &p));
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		EQ(p.application.updates_applied, 1);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		EQ(p.application.last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		uint64_t id = network_id(&f.server, a);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		expect_position(&f.clients[0], id, 1);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		expect_absent(&f.clients[0], network_id(&f.server, hidden));
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		CHECK(ref && *ref == replica(&f.clients[0], network_id(&f.server, b)));
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		EQ(file_sync_reader_step(r, 10, &p), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 33:
		"""
		EQ(p.application.updates_applied, 1);
		"""
	Then its observed values match the contract
	When I perform operation 34:
		"""
		expect_position(&f.clients[0], id, 9);
		"""
	Then its observed values match the contract
	When I perform operation 35:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 36:
		"""
		sync_session_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 37:
		"""
		OK(sync_get_session_info(f.clients[0].session, &info));
		"""
	Then its observed values match the contract
	When I perform operation 38:
		"""
		CHECK(info.world == f.clients[0].ecs && info.role == SYNC_ROLE_CLIENT);
		"""
	Then its observed values match the contract
	When I perform operation 39:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 40:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_format
	# When raw frames are written and inspected from a file
	# Then The container holds unchanged frames and peeks retain their borrowed
	# bytes
	Given the "file_sync" workflow "file_format"
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
		OK(sync_capture(f.server.session, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		struct packets packets = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		for (size_t i = 0; i < packets.count; i++)
		OK(file_sync_writer_write(w, packets.items[i].data,
		packets.items[i].size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(file_sync_writer_flush(w));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		unsigned char *bytes = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(size > 16 && !memcmp(bytes, "FILESYNC", 8));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(bytes[8] == 1 && bytes[9] == 0 && bytes[15] == 0);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(bytes[16] == (packets.items[0].size & 255));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		file_sync_frame_t frame;
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(file_sync_reader_peek(r, &frame), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		CHECK(!frame.data && !frame.size);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		free(bytes);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		packets_fini(&packets);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_checksum_vector
	# When a fixed envelope is recorded with an independently checked CRC value
	# Then The encoded length and IEEE CRC-32 match the format's reference bytes
	Given the "file_sync" workflow "file_checksum_vector"
	When I perform operation 1:
		"""
		const unsigned char frame[56] = {
		[0] = 'S', [1] = 'Y', [2] = 'N', [3] = 'C',
		[4] = 1, [5] = 1, [8] = 1, [24] = 1, [32] = 1, [40] = 1
		};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		const unsigned char crc[4] = { 0x21, 0xe7, 0xde, 0x62 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(file_sync_writer_write(w, frame, sizeof(frame)));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		unsigned char *bytes = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(size, 80);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(bytes[16] == 56 && !bytes[17] && !bytes[18] && !bytes[19]);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!memcmp(bytes + 20, frame, sizeof(frame)));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!memcmp(bytes + 76, crc, sizeof(crc)));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		free(bytes);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		file_sync_frame_t v;
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		OK(file_sync_reader_peek(r, &v));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(v.size == sizeof(frame) && !memcmp(v.data, frame, sizeof(frame)));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_arguments
	# When calls receive null pointers or invalid scalar arguments
	# Then Outputs are initialized and healthy state is preserved
	Given the "file_sync" workflow "file_arguments"
	When I perform operation 1:
		"""
		file_sync_writer_t *w = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		file_sync_reader_t *r = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		file_sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_status_t s;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_frame_t v;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(file_sync_writer_open(NULL, NULL, &w, &error), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!w && error.detail.code == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(file_sync_writer_open("", NULL, &w, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_writer_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(file_sync_reader_open(NULL, NULL, &r, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(file_sync_reader_open(path, NULL, NULL, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(file_sync_writer_write(NULL, "x", 1), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(file_sync_writer_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!p.frames && !p.application.updates_applied);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(file_sync_writer_flush(NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(file_sync_writer_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(!s.state && !s.offset);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(file_sync_reader_peek(NULL, &v), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		CHECK(!v.data && !v.size);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		EQ(file_sync_reader_consume(NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(file_sync_reader_pump(NULL, 1, &p), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		EQ(file_sync_reader_step(NULL, 1, &p), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		EQ(file_sync_reader_get_status(NULL, &s), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		OK(file_sync_writer_close(NULL, &error));
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		CHECK(!error.detail.code);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		OK(file_sync_reader_close(NULL, &error));
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		EQ(file_sync_writer_write(w, NULL, 1), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		EQ(file_sync_writer_write(w, "x", 0), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		EQ(file_sync_writer_get_status(w, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 33:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 34:
		"""
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 35:
		"""
		EQ(file_sync_reader_peek(r, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 36:
		"""
		EQ(file_sync_reader_consume(r), SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 37:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 38:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 39:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_options
	# When descriptors contain invalid roles, schedules or limits
	# Then Open rejects them without retaining peers or systems
	Given the "file_sync" workflow "file_options"
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
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_writer_desc_t bad[] = {
		{ .schedule = (sync_schedule_t)2 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .server = f.clients[0].session },
		{ .server = f.server.session, .max_frame_bytes = 65537 }
		};
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
		: SYNC_ERR_ARGUMENT;
		EQ(file_sync_writer_open(path, &bad[i], &w, NULL), expected);
		CHECK(!w);
		}
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(status(f.server.session, 0).peers, 2);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_writer_desc_t d = { .server = f.server.session };
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(file_sync_writer_open(path, &d, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		memset(&d, 0xff, sizeof(d));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(writer_status(w).peer != 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(writer_status(w).max_frame_bytes, 65536);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(file_sync_writer_pump(w, 0, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(file_sync_writer_write(w, "x", 1), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		file_sync_reader_desc_t invalid[] = {
		{ .schedule = (sync_schedule_t)-1 },
		{ .schedule = SYNC_AUTOMATIC },
		{ .frames_per_progress = 1 },
		{ .client = f.server.session },
		{ .client = f.clients[0].session, .max_frame_bytes = 65537 },
		{ .client = f.clients[0].session, .schedule = SYNC_AUTOMATIC }
		};
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
		sync_result_t expected = i == 3 ? SYNC_ERR_ROLE
		: i == 5 ? SYNC_ERR_STATE : SYNC_ERR_ARGUMENT;
		EQ(file_sync_reader_open(path, &invalid[i], &r, NULL),
		expected);
		CHECK(!r);
		}
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_io_open
	# When a path cannot be opened
	# Then An IO diagnostic is returned without a live handle
	Given the "file_sync" workflow "file_io_open"
	When I perform operation 1:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		file_sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		EQ(file_sync_reader_open("missing/record.bin", NULL, &r, &error),
		SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(!r && error.detail.code == SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(file_sync_writer_open("missing/record.bin", NULL, &w, &error),
		SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!w && error.detail.code == SYNC_ERR_IO);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_replace
	# When a recording path already exists
	# Then Opening the writer replaces its old contents
	Given the "file_sync" workflow "file_replace"
	When I perform operation 1:
		"""
		unsigned char trash[1024];
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		memset(trash, 0xff, sizeof(trash));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		byte_write(trash, sizeof(trash));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		unsigned char *p = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(size, 16);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		free(p);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_header_errors
	# When a header is incomplete, invalid or unsupported
	# Then Open returns truncation, protocol or unsupported diagnostics
	Given the "file_sync" workflow "file_header_errors"
	When I perform operation 1:
		"""
		unsigned char header[16] = {'F', 'I', 'L', 'E', 'S', 'Y', 'N', 'C', 1};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		file_sync_error_t e;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (size_t n = 0; n < 16; n++) {
		byte_write(header, n);
		EQ(file_sync_reader_open(path, NULL, &r, &e),
		SYNC_ERR_TRUNCATED);
		CHECK(!r && e.record == 0 && e.offset == n);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		header[0] = 'X';
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		byte_write(header, 16);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		header[0] = 'F';
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		header[8] = 2;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		byte_write(header, 16);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_UNSUPPORTED);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		header[8] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		header[12] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		byte_write(header, 16);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(file_sync_reader_open(path, NULL, &r, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_crc_and_limits
	# When record checksums or bounded lengths are invalid
	# Then Replay faults before delivering the offending record
	Given the "file_sync" workflow "file_crc_and_limits"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		unsigned char *bytes = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		bytes[22] ^= 1;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		byte_write(bytes, size);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		file_sync_status_t s = reader_status(r);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(s.state, FILE_SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(s.fault.record, 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(strstr(s.fault.detail.message, "checksum"));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		bytes[22] ^= 1;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		bytes[16] = bytes[17] = bytes[18] = bytes[19] = 0xff;
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		byte_write(bytes, size);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(reader_status(r).pending_bytes, 0);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		memset(bytes + 16, 0, 4);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		byte_write(bytes, size);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		free(bytes);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_partial_records
	# When EOF interrupts a length, frame or checksum
	# Then Reading reports physical truncation
	Given the "file_sync" workflow "file_partial_records"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		unsigned char *bytes = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint32_t length = (uint32_t)bytes[16] | (uint32_t)bytes[17] << 8 |
		(uint32_t)bytes[18] << 16 | (uint32_t)bytes[19] << 24;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(length > 8);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		size_t cuts[] = {17, 19, 20, 20 + length / 2,
		20 + length, 20 + length + 3};
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		for (size_t i = 0; i < sizeof(cuts) / sizeof(*cuts); i++) {
		byte_write(bytes, cuts[i]);
		file_sync_reader_t *r;
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		file_sync_frame_t v;
		EQ(file_sync_reader_peek(r, &v), SYNC_ERR_TRUNCATED);
		CHECK(!v.data && !v.size);
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
		}
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		free(bytes);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_logical_eof
	# When EOF occurs before an initial snapshot completes
	# Then Sync reports logical truncation without publishing replicas
	Given the "file_sync" workflow "file_logical_eof"
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
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		for (size_t i = 0; i + 1 < p.count; i++)
		OK(file_sync_writer_write(w, p.items[i].data, p.items[i].size));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		file_sync_progress_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		EQ(file_sync_reader_step(r, 1, &progress), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		CHECK(!progress.application.updates_applied && progress.frames > 0);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_TRUNCATED);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_writer_sequence
	# When frames are repeated, skipped or foreign
	# Then The writer faults without writing the offending record
	Given the "file_sync" workflow "file_writer_sequence"
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
		struct packets q = take_packets(f.server.session, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(file_sync_writer_write(w, p.items[1].data, p.items[1].size),
		SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_writer_close(w, NULL), SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		packets_fini(&q);
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

@file_sync
Scenario: file_fragmented_backpressure
	# When large updates meet a small receive queue
	# Then The same pending record is retried and only complete updates publish
	Given the "file_sync" workflow "file_fragmented_backpressure"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_client_desc_t cd = { .limits.max_receive_bytes = 65536 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, NULL, &cd);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_writer_t *w = writer(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		char *text = malloc(65536);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(text != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		memset(text, 'a', 65535);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		text[65535] = 0;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(sync_capture(f.server.session, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		OK(file_sync_writer_pump(w, UINT32_MAX, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		text[0] = 'b';
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		set_record(&f.server, e, (struct record){ .text = text });
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(sync_capture(f.server.session, 2, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		file_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(file_sync_reader_pump(r, UINT32_MAX, &p), SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		CHECK(p.frames > 0 && reader_status(r).pending_bytes > 0);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		file_sync_status_t before = reader_status(r);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(file_sync_reader_pump(r, 1, &p), SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		CHECK(!p.frames && !p.bytes);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		file_sync_status_t after = reader_status(r);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		CHECK(!memcmp(&before, &after, sizeof(before)));
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		OK(file_sync_reader_step(r, 1, &p));
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		EQ(p.application.last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		const struct record *v = ecs_get_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].record);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		CHECK(v && v->text[0] == 'a');
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		OK(file_sync_reader_step(r, 1, &p));
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		v = ecs_get_id(f.clients[0].ecs, replica(&f.clients[0], id),
		f.clients[0].record);
		"""
	Then its observed values match the contract
	When I perform operation 33:
		"""
		CHECK(v && !strcmp(v->text, text));
		"""
	Then its observed values match the contract
	When I perform operation 34:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 35:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 36:
		"""
		free(text);
		"""
	Then its observed values match the contract
	When I perform operation 37:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_eof_pending
	# When physical EOF arrives while the client has queued frames
	# Then EOF stays pending until the client drains and finishes cleanly
	Given the "file_sync" workflow "file_eof_pending"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!reader_status(r).pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		drain(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_borrowed_client
	# When a replay reader closes after publication
	# Then Its borrowed client and replicas remain until an explicit reset
	Given the "file_sync" workflow "file_borrowed_client"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(file_sync_reader_step(r, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_entity_t local = f.clients[0].local;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(status(f.clients[0].session, 0).entities, 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(ecs_is_alive(f.clients[0].ecs, local));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		file_sync_reader_desc_t d = { .client = f.clients[0].session };
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(file_sync_reader_open(path, &d, &r, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!r);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(file_sync_reader_open(path, &d, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_automatic
	# When automatic send and receive systems progress
	# Then Automatic budgets, phase ordering and system cleanup are preserved
	Given the "file_sync" workflow "file_automatic"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t sd = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_init(&f, &sd, &cd);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		int32_t systems = ecs_count_id(f.server.ecs, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_writer_desc_t wd = { .server = f.server.session,
		.schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		OK(file_sync_writer_open(path, &wd, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(ecs_count_id(f.server.ecs, EcsSystem) == systems + 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(writer_status(w).frames_per_progress, 64);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_entity_t e = spawn(&f.server, 7, writer_status(w).peer);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(writer_status(w).frames >= 4);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(file_sync_writer_pump(w, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(file_sync_writer_flush(w));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(ecs_count_id(f.server.ecs, EcsSystem), systems);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		file_sync_reader_desc_t rd = { .client = f.clients[0].session,
		.schedule = SYNC_AUTOMATIC, .frames_per_progress = 1 };
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		OK(file_sync_reader_open(path, &rd, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		file_sync_progress_t p = { .frames = 99 };
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		EQ(file_sync_reader_step(r, 1, &p), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		CHECK(!p.frames);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		for (unsigned i = 0; i < 32; i++) {
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		CHECK(reader_status(r).frames <= i + 1);
		if (reader_status(r).state == FILE_SYNC_ENDED)
		break;
		}
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		EQ(reader_status(r).state, FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 7);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_auto_manual_client
	# When update stepping targets an automatic client
	# Then The call is rejected before I/O while manual feeding remains supported
	Given the "file_sync" workflow "file_auto_manual_client"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, NULL, &cd);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(reader_status(r).offset, 16);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(file_sync_reader_pump(r, UINT32_MAX, NULL), SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_reader_pump(r, 1, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_fault_allocations
	# When FileSync allocations fail during open or replay
	# Then NOMEM is reported and owned peers, systems and buffers are released
	Given the "file_sync" workflow "file_fault_allocations"
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
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		bool done = false;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(done);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_fault_write_commit
	# When a record write is short
	# Then The exact outbox head remains uncommitted and the writer cannot retry
	Given the "file_sync" workflow "file_fault_write_commit"
	When I perform operation 1:
		"""
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
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_fault_flush_close
	# When flush or close fails
	# Then The first IO diagnostic survives complete cleanup
	Given the "file_sync" workflow "file_fault_flush_close"
	When I perform operation 1:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		OK(file_sync_writer_open(path, NULL, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_test_arm(FILE_SYNC_TEST_FLUSH, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(file_sync_writer_flush(w), SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_error_t first = writer_status(w).fault;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		file_sync_error_t error;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_writer_close(w, &error), SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(!memcmp(&first, &error, sizeof(first)));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		OK(file_sync_reader_open(path, NULL, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		file_sync_test_arm(FILE_SYNC_TEST_CLOSE, 0);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(file_sync_test_hit(FILE_SYNC_TEST_CLOSE));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_fault_read
	# When an input operation fails
	# Then FileSync faults without fabricating a core stream fault
	Given the "file_sync" workflow "file_fault_read"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_test_arm(FILE_SYNC_TEST_READ, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(file_sync_test_hit(FILE_SYNC_TEST_READ));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_WAIT_MANIFEST);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_phase_order
	# When game writers and readers surround automatic file I/O in the pipeline
	# Then Capture follows writes and replay publication precedes client readers
	Given the "file_sync" workflow "file_phase_order"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t sd = {
		.schedule = SYNC_AUTOMATIC, .send_hz = 8
		};
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_client_desc_t cd = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_init(&f, &sd, &cd);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC
		};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(file_sync_writer_open(path, &wd, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t e = spawn(&f.server, 1, writer_status(w).peer);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_entity_t sys = ecs_new(f.server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_add_pair(f.server.ecs, sys, EcsDependsOn, EcsPostUpdate);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_writer,
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }}
		}));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(writer_status(w).frames > 0);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		file_sync_reader_t *r;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		file_sync_reader_desc_t rd = {
		.client = f.clients[0].session, .schedule = SYNC_AUTOMATIC
		};
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		OK(file_sync_reader_open(path, &rd, &r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		EQ(reader_status(r).frames_per_progress, 64);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		struct phase_context ctx = {
		.fixture = &f, .id = network_id(&f.server, e)
		};
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		sys = ecs_new(f.clients[0].ecs);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		ecs_add_pair(f.clients[0].ecs, sys, EcsDependsOn, EcsPreUpdate);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		CHECK(ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = sys, .callback = phase_reader, .ctx = &ctx
		}));
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		CHECK(ctx.read);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		EQ(reader_status(r).state, FILE_SYNC_EOF_PENDING);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		EQ(reader_status(r).state, FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_writer_budget
	# When an automatic writer services a manually captured server
	# Then Its explicit frame budget is obeyed and close drains the remainder
	Given the "file_sync" workflow "file_writer_budget"
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
		file_sync_writer_t *w;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		file_sync_writer_desc_t wd = {
		.server = f.server.session, .schedule = SYNC_AUTOMATIC,
		.frames_per_progress = 1
		};
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(file_sync_writer_open(path, &wd, &w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_capture(f.server.session, 1, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(writer_status(w).frames, 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(writer_status(w).frames, 2);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		OK(file_sync_writer_close(w, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(file_sync_reader_step(r, 10, NULL), SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(status(f.clients[0].session, 0).entities, 1);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(file_sync_reader_close(r, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_later_failure
	# When a duplicate record follows a complete snapshot
	# Then Earlier publications survive with progress and a FileSync sequence fault
	Given the "file_sync" workflow "file_later_failure"
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
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		size_t size;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		unsigned char *bytes = byte_read(&size);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint32_t length = (uint32_t)bytes[16] |
		(uint32_t)bytes[17] << 8 | (uint32_t)bytes[18] << 16 |
		(uint32_t)bytes[19] << 24;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		FILE *file = fopen(path, "ab");
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(file != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(fwrite(bytes + 16, 1, (size_t)length + 8, file) ==
		(size_t)length + 8);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(fclose(file), 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		free(bytes);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		file_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(file_sync_reader_step(r, 10, &p), SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		EQ(p.application.updates_applied, 1);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(p.application.last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(status(f.clients[0].session, 0).entities, 1);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(reader_status(r).state, FILE_SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@file_sync
Scenario: file_schema_failure
	# When a recording manifest mismatches the local marked client schema
	# Then Core schema diagnostics propagate without creating replicas
	Given the "file_sync" workflow "file_schema_failure"
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
		sync_session_destroy(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		f.clients[0].session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_remove_id(f.clients[0].ecs, f.clients[0].health,
		f.clients[0].ids.sync);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		OK(sync_client_create(f.clients[0].ecs, NULL,
		&f.clients[0].session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		record_one(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_reader_t *r = reader(&f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(file_sync_reader_step(r, 1, NULL), SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(reader_status(r).fault.detail.code, SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(file_sync_reader_close(r, NULL), SYNC_ERR_SCHEMA);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
