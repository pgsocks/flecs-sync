#include "test.h"

TEST(gameplay_trade)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_ARROW, 8));
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 2, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_ARROW, 12, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_BRAM, RPG_ARROW, -1, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_ARROW, -1, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_BRAM, RPG_ARROW, 8, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_ARROW, 12, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_BRAM, RPG_ARROW, 8, error, size));
	NEED(rpg_references(c));
	return true;
}

TEST(gameplay_potion)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_health(c, RPG_ARIA, 76);
	ecs_entity_t e = rpg_find_stack(c, RPG_ARIA, RPG_POTION);
	NEED(e);
	struct rpg_stack stack = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, e, c->server.types[RPG_STACK]);
	NEED(stack.quantity == 5);
	stack.quantity--;
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(stack),
		&stack);
	rpg_health(c, RPG_ARIA, 96);
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 2, error, size));
	NEED(rpg_expect_health(c, RPG_BRAM, RPG_ARIA, 96, 2, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_POTION, 4, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_POTION, -1, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_POTION, 4, error, size));
	return true;
}

TEST(gameplay_merge)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_split(c, RPG_ARIA, RPG_ARROW, 8));
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_merge(c, RPG_ARIA, RPG_ARROW);
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 3, error, size));
	unsigned stacks;
	NEED(rpg_quantity(c, RPG_ARIA, RPG_ARIA, RPG_ARROW, &stacks) == 20);
	NEED(stacks == 1);
	for (size_t i = 0; i < c->entity_count; i++)
		if (!ecs_is_alive(c->server.ecs, c->entities[i].entity))
			for (unsigned view = 0; view < RPG_VIEWS; view++)
				NEED(!rpg_replica(
					c, view, c->entities[i].entity));
	NEED(rpg_references(c));
	return true;
}

TEST(gameplay_equipment)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG]);
	ecs_entity_t sword = rpg_find_stack(c, RPG_BRAM, RPG_SWORD);
	NEED(sword);
	bag.equipped = sword;
	ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_BRAM, 2), SYNC_OK);
	ecs_entity_t local = rpg_replica(c, RPG_BRAM, c->bags[RPG_BRAM]);
	NEED(local);
	const struct rpg_bag *replayed = ecs_get_id(c->clients[RPG_BRAM].ecs,
		local, c->clients[RPG_BRAM].types[RPG_BAG]);
	NEED(replayed && replayed->equipped);
	NEED(replayed->equipped == rpg_replica(c, RPG_BRAM, sword));
	NEED(replayed->equipped != sword);
	bag.equipped = 0;
	ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(file_sync_reader_step(c->readers[RPG_BRAM], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	replayed = ecs_get_id(c->clients[RPG_BRAM].ecs, local,
		c->clients[RPG_BRAM].types[RPG_BAG]);
	NEED(replayed && !replayed->equipped);
	return true;
}

TEST(gameplay_visibility)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	uint64_t id = rpg_network_id(c, c->bags[RPG_MIRA]);
	NEED(id);
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, 12, error, size));
	NEED(!rpg_replica(c, RPG_ARIA, c->details[RPG_MIRA]));
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, false);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, -1, error, size));
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	NEED(c->progress.application.updates_applied == 1);
	NEED(rpg_network_id(c, c->bags[RPG_MIRA]) == id);
	NEED(rpg_replica(c, RPG_ARIA, c->bags[RPG_MIRA]));
	NEED(rpg_references(c));
	return true;
}

TEST(gameplay_loot)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_trade(c, RPG_SKULK, RPG_ARIA, RPG_COIN, 7));
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 2, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_COIN, 7, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_COIN, -1, error, size));
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_SKULK, RPG_COIN, 0, error, size));
	NEED(rpg_references(c));
	return true;
}

TEST(gameplay_identity)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_POTION, 5));
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 2, error, size));
	NEED(c->transferred && c->transferred_id);
	NEED(rpg_network_id(c, c->transferred) == c->transferred_id);
	ecs_entity_t e = rpg_replica(c, RPG_BRAM, c->transferred);
	NEED(e);
	const sync_network_id_t *id = ecs_get_id(c->clients[RPG_BRAM].ecs, e,
		c->clients[RPG_BRAM].ids.network_id);
	NEED(id && id->value == c->transferred_id);
	NEED(!rpg_replica(c, RPG_ARIA, c->transferred));
	NEED(rpg_references(c));
	return true;
}

TEST(gameplay_combat)
{
	(void)example;
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_health(c, RPG_SKULK, 20);
	rpg_health(c, RPG_SKULK, 0);
	ecs_add_id(
		c->server.ecs, c->actors[RPG_SKULK], c->server.types[RPG_DEAD]);
	RESULT(rpg_capture(c, true), SYNC_OK);
	NEED(rpg_replay_all(c, 2, error, size));
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_SKULK, 0, 2, error, size));
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		ecs_entity_t e = rpg_replica(c, v, c->actors[RPG_SKULK]);
		NEED(e &&
			ecs_has_id(c->clients[v].ecs, e,
				c->clients[v].types[RPG_DEAD]));
	}
	return true;
}

TEST(gameplay_quest)
{
	NEED(!strcmp(example, "null") || !strcmp(example, "empty"));
	RESULT(rpg_capture(c, true), SYNC_OK);
	rpg_quest(c, RPG_ARIA, !strcmp(example, "null") ? NULL : "");
	RESULT(rpg_capture(c, true), SYNC_OK);
	RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
	NEED(e);
	const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
	NEED(d);
	if (!strcmp(example, "null"))
		NEED(!d->quest);
	else
		NEED(d->quest && !*d->quest);
	return true;
}
