@gameplay
Feature: Replay sampled RPG inventory and character changes

@RPG-GAME-001 @rpg
Scenario: A recorded trade keeps both inventories private
	# When Aria gives Bram 8 Arrow items
	# And the campaign records another update
	# And every recording is replayed through two updates
	# Then the visible inventories are:
	# | View | Owner | Item  | Quantity |
	# | Aria | Aria  | Arrow | 12       |
	# | Aria | Bram  | Arrow | hidden   |
	# | Bram | Aria  | Arrow | hidden   |
	# | Bram | Bram  | Arrow | 8        |
	# | GM   | Aria  | Arrow | 12       |
	# | GM   | Bram  | Arrow | 8        |
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_game_001"
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
		NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_ARROW, 8));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_replay_all(c, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_ARROW, 12, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Aria | Arrow | 12 |
	When I perform operation 7:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_BRAM, RPG_ARROW, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Bram | Arrow | hidden |
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
		c, RPG_BRAM, RPG_BRAM, RPG_ARROW, 8, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Bram | Bram | Arrow | 8 |
	When I perform operation 10:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_ARROW, 12, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Aria | Arrow | 12 |
	When I perform operation 11:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_BRAM, RPG_ARROW, 8, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Bram | Arrow | 8 |
	When I perform operation 12:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-002 @rpg
Scenario: Drinking a potion changes public health and private quantity
	# When Aria's health becomes 76
	# And Aria consumes one HealingPotion
	# And Aria's health becomes 96
	# And the campaign records another update
	# And every recording is replayed through two updates
	# Then Bram sees Aria with health 96 at tick 2
	# And the visible inventories are:
	# | View | Owner | Item          | Quantity |
	# | Aria | Aria  | HealingPotion | 4        |
	# | Bram | Aria  | HealingPotion | hidden   |
	# | GM   | Aria  | HealingPotion | 4        |
	Given the "rpg" workflow "rpg_game_002"
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
		rpg_health(c, RPG_ARIA, 76);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = rpg_find_stack(c, RPG_ARIA, RPG_POTION);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(e);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct rpg_stack stack = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, e, c->server.types[RPG_STACK]);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(stack.quantity == 5);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		stack.quantity--;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(stack),
		&stack);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		rpg_health(c, RPG_ARIA, 96);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(rpg_replay_all(c, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(rpg_expect_health(c, RPG_BRAM, RPG_ARIA, 96, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_POTION, 4, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Aria | HealingPotion | 4 |
	When I perform operation 15:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_POTION, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Bram | Aria | HealingPotion | hidden |
	When I perform operation 16:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_POTION, 4, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Aria | HealingPotion | 4 |
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-003 @rpg
Scenario: Stack splitting and merging retain quantity and clear references
	# When Aria splits 8 Arrow items into another stack
	# And the campaign records another update
	# And Aria merges the Arrow stacks
	# And the campaign records another update
	# And every recording is replayed through three updates
	# Then Aria has one Arrow stack containing 20 items
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_game_003"
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
		NEED(rpg_split(c, RPG_ARIA, RPG_ARROW, 8));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		rpg_merge(c, RPG_ARIA, RPG_ARROW);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_replay_all(c, 3, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		unsigned stacks;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(rpg_quantity(c, RPG_ARIA, RPG_ARIA, RPG_ARROW, &stacks) == 20);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(stacks == 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		for (size_t i = 0; i < c->entity_count; i++)
		if (!ecs_is_alive(c->server.ecs, c->entities[i].entity))
		for (unsigned view = 0; view < RPG_VIEWS; view++)
		NEED(!rpg_replica(
		c, view, c->entities[i].entity));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-004 @rpg
Scenario: Equipment uses translated and nullable stack references
	# When Bram equips IronSword
	# And the campaign records another update
	# And Bram replays two updates
	# Then Bram's equipped stack refers to the local IronSword stack
	# When Bram unequips the weapon
	# And the campaign records another update
	# And Bram's reader publishes one more update
	# Then Bram's equipped stack is null
	Given the "rpg" workflow "rpg_game_004"
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
		struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG]);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t sword = rpg_find_stack(c, RPG_BRAM, RPG_SWORD);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(sword);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		bag.equipped = sword;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		RESULT(rpg_replay(c, RPG_BRAM, 2), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_entity_t local = rpg_replica(c, RPG_BRAM, c->bags[RPG_BRAM]);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(local);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		const struct rpg_bag *replayed = ecs_get_id(c->clients[RPG_BRAM].ecs,
		local, c->clients[RPG_BRAM].types[RPG_BAG]);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(replayed && replayed->equipped);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(replayed->equipped == rpg_replica(c, RPG_BRAM, sword));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		NEED(replayed->equipped != sword);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		bag.equipped = 0;
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_BRAM], 1, &c->progress),
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
		replayed = ecs_get_id(c->clients[RPG_BRAM].ecs, local,
		c->clients[RPG_BRAM].types[RPG_BAG]);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		NEED(replayed && !replayed->equipped);
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-005 @rpg
Scenario: NPC inventory access is explicitly revealed and revoked
	# When Mira reveals the inventory to Aria
	# And the campaign records another update
	# And Aria replays two updates
	# Then the visible inventories are:
	# | View | Owner | Item          | Quantity |
	# | Aria | Mira  | HealingPotion | 12       |
	# And Mira's private quest remains hidden from Aria
	# When Mira hides the inventory from Aria
	# And the campaign records another update
	# And Aria's reader publishes one more update
	# Then the visible inventories are:
	# | View | Owner | Item          | Quantity |
	# | Aria | Mira  | HealingPotion | hidden   |
	# When Mira reveals the inventory to Aria
	# And the campaign records another update
	# And Aria's reader publishes one more update
	# Then Mira's inventory returns with its original network identity
	Given the "rpg" workflow "rpg_game_005"
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
		uint64_t id = rpg_network_id(c, c->bags[RPG_MIRA]);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(id);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, 12, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Mira | HealingPotion | 12 |
	When I perform operation 9:
		"""
		NEED(!rpg_replica(c, RPG_ARIA, c->details[RPG_MIRA]));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		rpg_reveal(c, RPG_MIRA, RPG_ARIA, false);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Mira | HealingPotion | hidden |
	When I perform operation 15:
		"""
		rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		NEED(rpg_network_id(c, c->bags[RPG_MIRA]) == id);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		NEED(rpg_replica(c, RPG_ARIA, c->bags[RPG_MIRA]));
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-006 @rpg
Scenario: Looting an NPC grants the recipient the stack's new ownership
	# When Skulk gives Aria 7 GoldCoin items
	# And the campaign records another update
	# And every recording is replayed through two updates
	# Then the visible inventories are:
	# | View | Owner | Item     | Quantity |
	# | Aria | Aria  | GoldCoin | 7        |
	# | Bram | Aria  | GoldCoin | hidden   |
	# | GM   | Skulk | GoldCoin | 0        |
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_game_006"
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
		NEED(rpg_trade(c, RPG_SKULK, RPG_ARIA, RPG_COIN, 7));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_replay_all(c, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_COIN, 7, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Aria | Aria | GoldCoin | 7 |
	When I perform operation 7:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_COIN, -1, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| Bram | Aria | GoldCoin | hidden |
	When I perform operation 8:
		"""
		NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_SKULK, RPG_COIN, 0, error, size));
		"""
	Then its observed values match the contract
	Then the visible inventories are:
		| View | Owner | Item | Quantity |
		| GM | Skulk | GoldCoin | 0 |
	When I perform operation 9:
		"""
		NEED(rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-007 @rpg
Scenario: A complete stack moves without recycling its network identity
	# When Aria gives Bram 5 HealingPotion items
	# And the campaign records another update
	# And every recording is replayed through two updates
	# Then the transferred stack retains its network identity
	# And every visible entity reference resolves locally
	Given the "rpg" workflow "rpg_game_007"
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
		NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_POTION, 5));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_replay_all(c, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(c->transferred && c->transferred_id);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_network_id(c, c->transferred) == c->transferred_id);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t e = rpg_replica(c, RPG_BRAM, c->transferred);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(e);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		const sync_network_id_t *id = ecs_get_id(c->clients[RPG_BRAM].ecs, e,
		c->clients[RPG_BRAM].ids.network_id);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(id && id->value == c->transferred_id);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(!rpg_replica(c, RPG_ARIA, c->transferred));
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

@RPG-GAME-008 @rpg
Scenario: Combat records tags and the final sampled health
	# When Skulk's health becomes 20
	# And Skulk's health becomes 0
	# And Skulk becomes dead
	# And the campaign records another update
	# And every recording is replayed through two updates
	# Then Aria sees Skulk with health 0 at tick 2
	# And every view sees Skulk's dead tag
	Given the "rpg" workflow "rpg_game_008"
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
		rpg_health(c, RPG_SKULK, 20);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		rpg_health(c, RPG_SKULK, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_id(
		c->server.ecs, c->actors[RPG_SKULK], c->server.types[RPG_DEAD]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_replay_all(c, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_SKULK, 0, 2, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		for (unsigned v = 0; v < RPG_VIEWS; v++) {
		ecs_entity_t e = rpg_replica(c, v, c->actors[RPG_SKULK]);
		NEED(e &&
		ecs_has_id(c->clients[v].ecs, e,
		c->clients[v].types[RPG_DEAD]));
		}
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-GAME-009 @rpg
Scenario Outline: Owned quest notes preserve null and empty values
	# When Aria's quest note becomes <note>
	# And the campaign records another update
	# And Aria replays two updates
	# Then Aria's quest note is <note>
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<note>"
	When I perform operation 1:
		"""
		NEED(!strcmp(example, "null") || !strcmp(example, "empty"));
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		rpg_quest(c, RPG_ARIA, !strcmp(example, "null") ? NULL : "");
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
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
		NEED(d);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		if (!strcmp(example, "null"))
		NEED(!d->quest);
		else
		NEED(d->quest && !*d->quest);
		"""
	Then its observed values match the contract
	Then the example quest note is "<note>"
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| note   | workflow |
		| null   | rpg_game_009_null |
		| empty  | rpg_game_009_empty |
