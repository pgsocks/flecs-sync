@replay
Feature: Bound replay of complete adventure updates

@RPG-REPLAY-001 @rpg
Scenario: Snapshot chunks and deltas count as complete updates
	# When the campaign records its initial update
	# And Aria's health becomes 42
	# And the campaign records another update
	# And Aria replays one update
	# Then Aria sees Aria with health 100 at tick 1
	# When Aria's reader publishes one more update
	# Then Aria sees Aria with health 42 at tick 2
	Given the "rpg" workflow "rpg_replay_001"
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
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		rpg_health(c, RPG_ARIA, 42);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-REPLAY-002 @rpg
Scenario: Frame pumping queues data without publishing characters
	# When the campaign records its initial update
	# And Aria opens the recording for replay
	# And Aria pumps one frame
	# Then Aria has accepted 1 frame and published 0 updates
	# And Aria's replay has no character replicas
	# When Aria's reader publishes one more update
	# Then Aria sees Aria with health 100 at tick 1
	Given the "rpg" workflow "rpg_replay_002"
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
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(c->progress.frames == 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(!c->progress.application.updates_applied);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-REPLAY-003 @rpg
Scenario: Backpressure retains a frame and queued input is applied first
	# Given Aria's receive queue holds only one maximum-sized frame
	# When Aria receives a quest note of 65535 characters
	# And the campaign records its initial update
	# And Aria's health becomes 42
	# And the campaign records another update
	# And Aria opens the recording for replay
	# And Aria pumps until backpressure
	# Then retrying the pending frame reads no additional file bytes
	# When Aria's reader publishes one more update
	# Then Aria sees Aria with health 100 at tick 1
	# When Aria's reader publishes one more update
	# Then Aria sees Aria with health 42 at tick 2
	# And Aria's quest note has 65535 characters
	Given the "rpg" workflow "rpg_replay_003"
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
		c->small_queue = true;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(rpg_client_recreate(c, RPG_ARIA, false));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(rpg_large_quest(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		rpg_health(c, RPG_ARIA, 42);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		RESULT(file_sync_reader_pump(
		c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		file_sync_status_t before = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(before.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		file_sync_status_t after = rpg_reader_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(!c->progress.frames && !c->progress.bytes);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		NEED(!memcmp(&before, &after, sizeof(before)));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 42, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		NEED(rpg_expect_large_quest(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-REPLAY-004 @rpg
Scenario: A fragmented quest update publishes only when complete
	# When the campaign records its initial update
	# And Aria receives a quest note of 65535 characters
	# And the campaign records another update
	# And Aria replays one update
	# Then Aria's quest note is original
	# When Aria's reader publishes one more update
	# Then Aria's quest note has 65535 characters
	Given the "rpg" workflow "rpg_replay_004"
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
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(rpg_large_quest(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(e);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(d && d->quest && !strcmp(d->quest, "Find the lost relic"));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(rpg_expect_large_quest(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-REPLAY-005 @rpg
Scenario: Physical EOF waits for queued client input
	# When the campaign records its initial update
	# And Aria opens the recording for replay
	# And Aria pumps until backpressure
	# Then Aria's reader is EOF_PENDING
	# When Aria applies queued input and finishes replay
	# Then Aria's reader is ENDED
	# And Aria's final replicas remain available
	Given the "rpg" workflow "rpg_replay_005"
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
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(file_sync_reader_pump(
		c->readers[RPG_ARIA], UINT32_MAX, &c->progress),
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_EOF_PENDING);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(sync_apply(c->clients[RPG_ARIA].session, UINT32_MAX, NULL),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(file_sync_reader_pump(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(rpg_reader_status(c, RPG_ARIA).state == FILE_SYNC_ENDED);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-REPLAY-006 @rpg
Scenario: Closing a replay preserves replicas until an explicit reset
	# When the campaign records its initial update
	# And Aria replays one update
	# And Aria closes the replay
	# Then Aria's final replicas remain available
	# And attaching another replay requires a reset
	# When Aria resets and opens the recording again
	# Then Aria's local interface entity survives
	Given the "rpg" workflow "rpg_replay_006"
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
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_result_t result =
		file_sync_reader_close(c->readers[RPG_ARIA], &c->error);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		c->readers[RPG_ARIA] = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(result, SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_expect_retained(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_reader_t *r = (void *)1;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		file_sync_reader_desc_t desc = {.client = c->clients[RPG_ARIA].session};
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		result = file_sync_reader_open(rpg_path(RPG_ARIA), &desc, &r, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		if (!result)
		c->probe_reader = r;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		RESULT(result, SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(!r && rpg_replica(c, RPG_ARIA, c->actors[RPG_ARIA]));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		RESULT(sync_client_reset(c->clients[RPG_ARIA].session), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		for (unsigned a = 0; a < RPG_ACTORS; a++)
		NEED(!rpg_replica(c, RPG_ARIA, c->actors[a]));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		NEED(ecs_is_alive(
		c->clients[RPG_ARIA].ecs, c->clients[RPG_ARIA].local));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
