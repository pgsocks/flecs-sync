@recording
Feature: Record each adventurer's permitted view

@RPG-RECORD-001 @rpg
Scenario: Public stats and private inventories have separate recordings
	# When the campaign records its initial update
	# And every recording is replayed through one update
	# Then the visible inventories are:
	# | View | Owner | Item          | Quantity |
	# | Aria | Aria  | HealingPotion | 5        |
	# | Aria | Bram  | HealingPotion | hidden   |
	# | Aria | Mira  | HealingPotion | hidden   |
	# | Bram | Bram  | IronSword     | 1        |
	# | Bram | Aria  | Arrow         | hidden   |
	# | GM   | Mira  | HealingPotion | 12       |
	# | GM   | Skulk | GoldCoin      | 7        |
	# And every view has each character's individual public stats
	# And private details are visible only to their owner and the GM
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_record_001"
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
		NEED(rpg_replay_all(c, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_POTION, 5, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Aria | HealingPotion | 5 |
	When I perform operation 5:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_BRAM, RPG_POTION, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Bram | HealingPotion | hidden |
	When I perform operation 6:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Mira | HealingPotion | hidden |
	When I perform operation 7:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_BRAM, RPG_SWORD, 1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Bram | Bram | IronSword | 1 |
	When I perform operation 8:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_ARROW, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Bram | Aria | Arrow | hidden |
	When I perform operation 9:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_MIRA, RPG_POTION, 12, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Mira | HealingPotion | 12 |
	When I perform operation 10:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_SKULK, RPG_COIN, 7, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Skulk | GoldCoin | 7 |
	When I perform operation 11:
		"""
		NEED(rpg_expect_stats(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(rpg_expect_private(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-002 @rpg
Scenario: A recording started mid-adventure begins with current state
	# When the campaign records its initial update
	# And Aria's recording closes
	# And Aria's health becomes 76
	# And the campaign records another update
	# And Aria starts a new recording
	# And the campaign records another update
	# And Aria replays one update
	# Then Aria sees Aria with health 76 at tick 3
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_record_002"
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
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		rpg_health(c, RPG_ARIA, 76);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_writer_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 76, 3, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-003 @rpg
Scenario: A network consumer also records the exact delivered stream
	# Given recording uses the existing delivery consumer
	# When the campaign records its initial update
	# And every recording is replayed through one update
	# Then recorded frames equal the frames accepted by the consumer
	# And private details are visible only to their owner and the GM
	Given the "rpg" workflow "rpg_record_003"
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
		NEED(rpg_raw_setup(c));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(rpg_replay_all(c, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(recording_delivered_frames(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(rpg_expect_private(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-004 @rpg
Scenario: Raw inspection borrows frames without applying RPG updates
	# When the campaign records its initial update
	# And Aria's recording closes
	# Then raw inspection preserves peeks until consumption
	# And raw inspection reaches physical EOF without publishing characters
	Given the "rpg" workflow "rpg_record_004"
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
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(recording_raw_peek(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(recording_raw_end(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-005 @rpg
Scenario: Options and status are copied without advancing recordings
	# Then recording options and session information are copied
	# And status inspection changes neither file offsets nor counters
	Given the "rpg" workflow "rpg_record_005"
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
		NEED(recording_copied_options(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(recording_pure_status(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-006 @rpg
Scenario: Flush does not capture or drain the campaign outbox
	# When a campaign update is captured without servicing recordings
	# And Aria flushes the recording
	# Then Aria's queued frame remains uncommitted
	# And Aria's recording has written 0 frames
	Given the "rpg" workflow "rpg_record_006"
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
		NEED(rpg_capture_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(file_sync_writer_flush(c->writers[RPG_ARIA]), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(rpg_expect_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_status_t status = rpg_writer_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(status.state == FILE_SYNC_ACTIVE && !status.frames);
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-007 @rpg
Scenario: Healthy close drains frames and releases only its recording peer
	# When a campaign update is captured without servicing recordings
	# And Aria's recording closes
	# And Aria replays one update
	# Then Aria sees Aria with health 100 at tick 1
	# And the recording peer is gone while campaign entities survive
	Given the "rpg" workflow "rpg_record_007"
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
		NEED(rpg_capture_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 1), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(recording_peer_gone(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-RECORD-008 @rpg
Scenario: A new recording replaces an existing adventure file
	# When the campaign records its initial update
	# And Aria's recording closes
	# And Aria starts a new recording
	# And Aria's recording closes
	# Then Aria's recording contains only its container header
	Given the "rpg" workflow "rpg_record_008"
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
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_writer_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(recording_header_only(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
