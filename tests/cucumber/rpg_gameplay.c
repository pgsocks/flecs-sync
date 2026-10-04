/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

TEST(gameplay_trade)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1121246172u);
	NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_ARROW, 8));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1580635142u);
	NEED(rpg_replay_all(c, 2, error, size));
	cucumber_reply();

	cucumber_wait(6, 3000691986u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_ARROW, 12, error, size));
	cucumber_reply();

	cucumber_wait(7, 2796683834u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_BRAM, RPG_ARROW, -1, error, size));
	cucumber_reply();

	cucumber_wait(8, 1496546758u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_ARROW, -1, error, size));
	cucumber_reply();

	cucumber_wait(9, 1198484464u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_BRAM, RPG_ARROW, 8, error, size));
	cucumber_reply();

	cucumber_wait(10, 831788980u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_ARROW, 12, error, size));
	cucumber_reply();

	cucumber_wait(11, 2520229116u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_BRAM, RPG_ARROW, 8, error, size));
	cucumber_reply();

	cucumber_wait(12, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(gameplay_potion)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 2812400843u);
	rpg_health(c, RPG_ARIA, 76);
	cucumber_reply();

	cucumber_wait(4, 1820068491u);
	ecs_entity_t e = rpg_find_stack(c, RPG_ARIA, RPG_POTION);
	cucumber_reply();

	cucumber_wait(5, 1664600299u);
	NEED(e);
	cucumber_reply();

	cucumber_wait(6, 717654455u);
	struct rpg_stack stack = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, e, c->server.types[RPG_STACK]);
	cucumber_reply();

	cucumber_wait(7, 1801532724u);
	NEED(stack.quantity == 5);
	cucumber_reply();

	cucumber_wait(8, 3412935548u);
	stack.quantity--;
	cucumber_reply();

	cucumber_wait(9, 2529898690u);
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(stack),
		&stack);
	cucumber_reply();

	cucumber_wait(10, 1199481848u);
	rpg_health(c, RPG_ARIA, 96);
	cucumber_reply();

	cucumber_wait(11, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 1580635142u);
	NEED(rpg_replay_all(c, 2, error, size));
	cucumber_reply();

	cucumber_wait(13, 495646763u);
	NEED(rpg_expect_health(c, RPG_BRAM, RPG_ARIA, 96, 2, error, size));
	cucumber_reply();

	cucumber_wait(14, 2565348078u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_POTION, 4, error, size));
	cucumber_reply();

	cucumber_wait(15, 1391076914u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_POTION, -1, error, size));
	cucumber_reply();

	cucumber_wait(16, 463921224u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_ARIA, RPG_POTION, 4, error, size));
	cucumber_reply();
	return true;
}

TEST(gameplay_merge)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 2117608811u);
	NEED(rpg_split(c, RPG_ARIA, RPG_ARROW, 8));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 3767987210u);
	rpg_merge(c, RPG_ARIA, RPG_ARROW);
	cucumber_reply();

	cucumber_wait(6, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 3650117445u);
	NEED(rpg_replay_all(c, 3, error, size));
	cucumber_reply();

	cucumber_wait(8, 1503076150u);
	unsigned stacks;
	cucumber_reply();

	cucumber_wait(9, 1598912958u);
	NEED(rpg_quantity(c, RPG_ARIA, RPG_ARIA, RPG_ARROW, &stacks) == 20);
	cucumber_reply();

	cucumber_wait(10, 2934023438u);
	NEED(stacks == 1);
	cucumber_reply();

	cucumber_wait(11, 2770865875u);
	for (size_t i = 0; i < c->entity_count; i++)
		if (!ecs_is_alive(c->server.ecs, c->entities[i].entity))
			for (unsigned view = 0; view < RPG_VIEWS; view++)
				NEED(!rpg_replica(
					c, view, c->entities[i].entity));
	cucumber_reply();

	cucumber_wait(12, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(gameplay_equipment)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 714245738u);
	struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG]);
	cucumber_reply();

	cucumber_wait(4, 1803850638u);
	ecs_entity_t sword = rpg_find_stack(c, RPG_BRAM, RPG_SWORD);
	cucumber_reply();

	cucumber_wait(5, 2139527767u);
	NEED(sword);
	cucumber_reply();

	cucumber_wait(6, 2115866817u);
	bag.equipped = sword;
	cucumber_reply();

	cucumber_wait(7, 593658466u);
	ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	cucumber_reply();

	cucumber_wait(8, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 1006865120u);
	RESULT(rpg_replay(c, RPG_BRAM, 2), SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 2789975653u);
	ecs_entity_t local = rpg_replica(c, RPG_BRAM, c->bags[RPG_BRAM]);
	cucumber_reply();

	cucumber_wait(11, 2296033870u);
	NEED(local);
	cucumber_reply();

	cucumber_wait(12, 3964943177u);
	const struct rpg_bag *replayed = ecs_get_id(c->clients[RPG_BRAM].ecs,
		local, c->clients[RPG_BRAM].types[RPG_BAG]);
	cucumber_reply();

	cucumber_wait(13, 3574804714u);
	NEED(replayed && replayed->equipped);
	cucumber_reply();

	cucumber_wait(14, 1485694427u);
	NEED(replayed->equipped == rpg_replica(c, RPG_BRAM, sword));
	cucumber_reply();

	cucumber_wait(15, 1664297693u);
	NEED(replayed->equipped != sword);
	cucumber_reply();

	cucumber_wait(16, 2884882970u);
	bag.equipped = 0;
	cucumber_reply();

	cucumber_wait(17, 593658466u);
	ecs_set_id(c->server.ecs, c->bags[RPG_BRAM], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	cucumber_reply();

	cucumber_wait(18, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(19, 121922910u);
	RESULT(file_sync_reader_step(c->readers[RPG_BRAM], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(20, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(21, 1593756615u);
	replayed = ecs_get_id(c->clients[RPG_BRAM].ecs, local,
		c->clients[RPG_BRAM].types[RPG_BAG]);
	cucumber_reply();

	cucumber_wait(22, 1122691604u);
	NEED(replayed && !replayed->equipped);
	cucumber_reply();
	return true;
}

TEST(gameplay_visibility)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1926141955u);
	uint64_t id = rpg_network_id(c, c->bags[RPG_MIRA]);
	cucumber_reply();

	cucumber_wait(4, 517135209u);
	NEED(id);
	cucumber_reply();

	cucumber_wait(5, 1482305659u);
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
	cucumber_reply();

	cucumber_wait(6, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 3731329055u);
	RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 1522337296u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, 12, error, size));
	cucumber_reply();

	cucumber_wait(9, 3646509947u);
	NEED(!rpg_replica(c, RPG_ARIA, c->details[RPG_MIRA]));
	cucumber_reply();

	cucumber_wait(10, 3363041336u);
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, false);
	cucumber_reply();

	cucumber_wait(11, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(13, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(14, 998078598u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_MIRA, RPG_POTION, -1, error, size));
	cucumber_reply();

	cucumber_wait(15, 1482305659u);
	rpg_reveal(c, RPG_MIRA, RPG_ARIA, true);
	cucumber_reply();

	cucumber_wait(16, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 3312863132u);
	RESULT(file_sync_reader_step(c->readers[RPG_ARIA], 1, &c->progress),
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(18, 3715667087u);
	NEED(c->progress.application.updates_applied == 1);
	cucumber_reply();

	cucumber_wait(19, 16471511u);
	NEED(rpg_network_id(c, c->bags[RPG_MIRA]) == id);
	cucumber_reply();

	cucumber_wait(20, 2145284925u);
	NEED(rpg_replica(c, RPG_ARIA, c->bags[RPG_MIRA]));
	cucumber_reply();

	cucumber_wait(21, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(gameplay_loot)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 643703987u);
	NEED(rpg_trade(c, RPG_SKULK, RPG_ARIA, RPG_COIN, 7));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1580635142u);
	NEED(rpg_replay_all(c, 2, error, size));
	cucumber_reply();

	cucumber_wait(6, 1922629731u);
	NEED(rpg_expect_quantity(
		c, RPG_ARIA, RPG_ARIA, RPG_COIN, 7, error, size));
	cucumber_reply();

	cucumber_wait(7, 4293972638u);
	NEED(rpg_expect_quantity(
		c, RPG_BRAM, RPG_ARIA, RPG_COIN, -1, error, size));
	cucumber_reply();

	cucumber_wait(8, 2402510249u);
	NEED(rpg_expect_quantity(
		c, RPG_GM, RPG_SKULK, RPG_COIN, 0, error, size));
	cucumber_reply();

	cucumber_wait(9, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(gameplay_identity)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 2532023937u);
	NEED(rpg_trade(c, RPG_ARIA, RPG_BRAM, RPG_POTION, 5));
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 1580635142u);
	NEED(rpg_replay_all(c, 2, error, size));
	cucumber_reply();

	cucumber_wait(6, 3793811234u);
	NEED(c->transferred && c->transferred_id);
	cucumber_reply();

	cucumber_wait(7, 2447913088u);
	NEED(rpg_network_id(c, c->transferred) == c->transferred_id);
	cucumber_reply();

	cucumber_wait(8, 2865080791u);
	ecs_entity_t e = rpg_replica(c, RPG_BRAM, c->transferred);
	cucumber_reply();

	cucumber_wait(9, 1664600299u);
	NEED(e);
	cucumber_reply();

	cucumber_wait(10, 588261033u);
	const sync_network_id_t *id = ecs_get_id(c->clients[RPG_BRAM].ecs, e,
		c->clients[RPG_BRAM].ids.network_id);
	cucumber_reply();

	cucumber_wait(11, 4073027342u);
	NEED(id && id->value == c->transferred_id);
	cucumber_reply();

	cucumber_wait(12, 2562367461u);
	NEED(!rpg_replica(c, RPG_ARIA, c->transferred));
	cucumber_reply();

	cucumber_wait(13, 3828074051u);
	NEED(rpg_references(c));
	cucumber_reply();
	return true;
}

TEST(gameplay_combat)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3563310280u);
	(void)example;
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 2629454112u);
	rpg_health(c, RPG_SKULK, 20);
	cucumber_reply();

	cucumber_wait(4, 3608737255u);
	rpg_health(c, RPG_SKULK, 0);
	cucumber_reply();

	cucumber_wait(5, 1148675503u);
	ecs_add_id(
		c->server.ecs, c->actors[RPG_SKULK], c->server.types[RPG_DEAD]);
	cucumber_reply();

	cucumber_wait(6, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 1580635142u);
	NEED(rpg_replay_all(c, 2, error, size));
	cucumber_reply();

	cucumber_wait(8, 802874576u);
	NEED(rpg_expect_health(c, RPG_ARIA, RPG_SKULK, 0, 2, error, size));
	cucumber_reply();

	cucumber_wait(9, 1369458831u);
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		ecs_entity_t e = rpg_replica(c, v, c->actors[RPG_SKULK]);
		NEED(e &&
			ecs_has_id(c->clients[v].ecs, e,
				c->clients[v].types[RPG_DEAD]));
	}
	cucumber_reply();
	return true;
}

TEST(gameplay_quest)
{
	(void)error;
	(void)size;
	cucumber_wait(1, 3796207634u);
	NEED(!strcmp(example, "null") || !strcmp(example, "empty"));
	cucumber_reply();

	cucumber_wait(2, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(3, 1651804357u);
	rpg_quest(c, RPG_ARIA, !strcmp(example, "null") ? NULL : "");
	cucumber_reply();

	cucumber_wait(4, 3887883319u);
	RESULT(rpg_capture(c, true), SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 3731329055u);
	RESULT(rpg_replay(c, RPG_ARIA, 2), SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 808489624u);
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
	cucumber_reply();

	cucumber_wait(7, 1664600299u);
	NEED(e);
	cucumber_reply();

	cucumber_wait(8, 28591961u);
	const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
	cucumber_reply();

	cucumber_wait(9, 1660265180u);
	NEED(d);
	cucumber_reply();

	cucumber_wait(10, 1369607044u);
	if (!strcmp(example, "null"))
		NEED(!d->quest);
	else
		NEED(d->quest && !*d->quest);
	cucumber_reply();
	return true;
}
