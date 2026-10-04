@automatic
Feature: Service RPG recordings through Flecs phases

@RPG-AUTO-001 @rpg
Scenario: Gameplay writes precede recording and publication precedes readers
	# Given automatic campaign capture and automatic file service
	# When a gameplay system writes Aria's health before capture
	# And the server world progresses once
	# And automatic replay opens for Aria
	# And Aria's world progresses once
	# Then a downstream reader sees Aria with health 42
	# And Aria's reader is EOF_PENDING
	# When Aria's world progresses once
	# Then Aria's reader is ENDED
	Given the "rpg" workflow "rpg_auto_001"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(automatic_setup(c, true, 0, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t sys = ecs_new(c->server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_add_pair(c->server.ecs, sys, EcsDependsOn, EcsPostUpdate);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		c->game_system = ecs_system_init(c->server.ecs,
		&(ecs_system_desc_t){.entity = sys,
		.callback = gameplay_write,
		.ctx = c,
		.query.terms = {{.id = c->server.types[RPG_STATS],
		.inout = EcsOut}}});
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(c->game_system);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(server_progress(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(automatic_reader(c, 0, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(c->phase_seen);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-AUTO-002 @rpg
Scenario: Automatic file service also supports explicit campaign captures
	# Given manual campaign capture and automatic file service
	# And the automatic recording budget is 1 frame
	# When a campaign update is captured without servicing recordings
	# And the server world progresses once
	# Then Aria's recording has written 1 frames
	# When the server world progresses once
	# Then Aria's recording has written 2 frames
	# When Aria's recording closes
	# And Aria replays one update
	# Then Aria sees Aria with health 100 at tick 1
	Given the "rpg" workflow "rpg_auto_002"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(automatic_setup(c, false, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(rpg_capture_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(server_progress(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_writer_status(c, RPG_ARIA).state == FILE_SYNC_ACTIVE);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(rpg_writer_status(c, RPG_ARIA).frames == 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(server_progress(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(rpg_writer_status(c, RPG_ARIA).frames == 2);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-AUTO-003 @rpg
Scenario: Manual feeding supports an automatic application client
	# Given Aria's core client applies automatically
	# When the campaign records its initial update
	# And Aria opens the recording for replay
	# Then manual update stepping is rejected before reading
	# When Aria pumps until backpressure
	# And Aria's world progresses once
	# And Aria finishes feeding the recording
	# Then Aria's reader is ENDED
	Given the "rpg" workflow "rpg_auto_003"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		c->client_auto = true;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(rpg_client_recreate(c, RPG_ARIA, false));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(!memcmp(&before, &after, sizeof(before)));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(!c->progress.frames && !c->progress.application.updates_applied);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		RESULT(file_sync_reader_pump(
		c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-AUTO-004 @rpg
Scenario: Automatic replay budgets frames rather than complete updates
	# Given Aria's core client applies automatically
	# When the campaign records its initial update
	# And automatic replay opens for Aria with a budget of 1 frame
	# And Aria's world progresses once
	# Then Aria's reader has accepted 1 frames
	# And Aria's replay has no character replicas
	# When Aria's world progresses until replay ends
	# Then Aria's reader is ENDED
	# And Aria sees Aria with health 100 at tick 1
	Given the "rpg" workflow "rpg_auto_004"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		c->client_auto = true;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(rpg_client_recreate(c, RPG_ARIA, false));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(automatic_reader(c, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_status_t status = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(status.state == FILE_SYNC_ACTIVE && status.frames == 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		bool ended = false;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		for (unsigned i = 0; i < 32; i++) {
		file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
		NEED(after.frames >= before.frames);
		NEED(after.frames - before.frames <= after.frames_per_progress);
		if (after.state == FILE_SYNC_ENDED) {
		ended = true;
		break;
		}
		NEED(after.state != FILE_SYNC_FAULTED);
		}
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(ended && rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-AUTO-005 @rpg
Scenario: Automatic handles reject manual transfers and clean up systems
	# Given automatic campaign capture and automatic file service
	# When the server world progresses once
	# And automatic replay opens for Aria
	# Then manual operations on automatic handles are rejected
	# When Aria closes both file handles
	# Then adapter systems disappear and the worlds still progress
	Given the "rpg" workflow "rpg_auto_005"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(automatic_setup(c, true, 0, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(server_progress(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(automatic_reader(c, 0, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_status_t w = rpg_writer_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		file_sync_status_t r = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(!c->progress.frames);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		RESULT(file_sync_writer_write(c->writers[RPG_ARIA], "x", 1),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		file_sync_frame_t frame;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		RESULT(file_sync_reader_peek(c->readers[RPG_ARIA], &frame),
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(!frame.data && !frame.size);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		RESULT(file_sync_reader_consume(c->readers[RPG_ARIA]), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		file_sync_status_t wa = rpg_writer_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		file_sync_status_t ra = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		NEED(!memcmp(&w, &wa, sizeof(w)) && !memcmp(&r, &ra, sizeof(r)));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		int32_t server_systems = ecs_count_id(c->server.ecs, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		int32_t client_systems =
		ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], NULL);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		c->readers[RPG_ARIA] = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		RESULT(result, SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		NEED(ecs_count_id(c->server.ecs, EcsSystem) == server_systems - 1);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		NEED(ecs_count_id(c->clients[RPG_ARIA].ecs, EcsSystem) ==
		client_systems - 1);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		for (unsigned v = 1; v < RPG_VIEWS; v++)
		RESULT(rpg_writer_close(c, v), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		NEED(ecs_is_alive(c->clients[RPG_ARIA].ecs, c->downstream_system));
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		NEED(ecs_progress(c->server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		NEED(ecs_progress(c->clients[RPG_ARIA].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
