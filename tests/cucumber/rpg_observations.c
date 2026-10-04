#include "test.h"
#include "protocol.h"

file_sync_status_t rpg_writer_status(struct rpg_context *c, unsigned view)
{
	file_sync_status_t s = {0};
	file_sync_writer_get_status(c->writers[view], &s);
	return s;
}

file_sync_status_t rpg_reader_status(struct rpg_context *c, unsigned view)
{
	file_sync_status_t s = {0};
	file_sync_reader_get_status(c->readers[view], &s);
	return s;
}

bool rpg_expect_health(struct rpg_context *c, unsigned v, unsigned actor,
	int32_t health, uint64_t tick, char *error, size_t size)
{
	ecs_entity_t e = rpg_replica(c, v, c->actors[actor]);
	NEED(e);
	const struct rpg_stats *s = ecs_get_id(
		c->clients[v].ecs, e, c->clients[v].types[RPG_STATS]);
	NEED(s);
	cucumber_observe(s->health, health);
	sync_status_t status;
	RESULT(sync_get_status(c->clients[v].session, 0, &status), SYNC_OK);
	cucumber_observe((int64_t)status.last_tick, (int64_t)tick);
	return true;
}

bool rpg_expect_quantity(struct rpg_context *c, unsigned view, unsigned actor,
	unsigned item, int64_t quantity, char *error, size_t size)
{
	unsigned stacks;
	int64_t actual = rpg_quantity(c, view, actor, item, &stacks);
	cucumber_observe(actual, quantity);
	if (actual != quantity) {
		snprintf(error, size,
			"view %u actor %u item %u: "
			"expected %lld, actual %lld",
			view, actor, item, (long long)quantity,
			(long long)actual);
		return false;
	}
	return true;
}

bool rpg_expect_no_replicas(
	struct rpg_context *c, unsigned view, char *error, size_t size)
{
	sync_status_t status;
	RESULT(sync_get_status(c->clients[view].session, 0, &status), SYNC_OK);
	NEED(!status.entities && !status.last_tick);
	for (unsigned actor = 0; actor < RPG_ACTORS; actor++)
		NEED(!rpg_replica(c, view, c->actors[actor]));
	return true;
}

bool rpg_expect_retained(
	struct rpg_context *c, unsigned view, char *error, size_t size)
{
	for (unsigned actor = 0; actor < RPG_ACTORS; actor++)
		NEED(rpg_replica(c, view, c->actors[actor]));
	NEED(rpg_references(c));
	return true;
}

bool rpg_replay_all(
	struct rpg_context *c, unsigned updates, char *error, size_t size)
{
	for (unsigned view = 0; view < RPG_VIEWS; view++) {
		RESULT(rpg_replay(c, view, updates), SYNC_OK);
		NEED(c->progress.application.updates_applied == updates);
	}
	return true;
}

bool rpg_capture_queued(struct rpg_context *c, char *error, size_t size)
{
	RESULT(rpg_capture(c, false), SYNC_OK);
	RESULT(sync_outbox_peek(
		       c->server.session, c->peers[RPG_ARIA], &c->queued),
		SYNC_OK);
	return true;
}

bool rpg_large_quest(struct rpg_context *c, char *error, size_t size)
{
	char *q = malloc(65536);
	NEED(q);
	memset(q, 'Q', 65535);
	q[65535] = 0;
	rpg_quest(c, RPG_ARIA, q);
	free(q);
	return true;
}

bool rpg_expect_large_quest(struct rpg_context *c, char *error, size_t size)
{
	ecs_entity_t e = rpg_replica(c, RPG_ARIA, c->details[RPG_ARIA]);
	NEED(e);
	const struct rpg_details *d = ecs_get_id(c->clients[RPG_ARIA].ecs, e,
		c->clients[RPG_ARIA].types[RPG_DETAILS]);
	NEED(d && d->quest && strlen(d->quest) == 65535);
	bool valid = true;
	for (const char *p = d->quest; *p; p++)
		valid = valid && *p == 'Q';
	NEED(valid);
	return true;
}

bool rpg_expect_stats(struct rpg_context *c, char *error, size_t size)
{
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		struct rpg_world *w = &c->clients[v];
		NEED(w->types[RPG_STATS] != c->server.types[RPG_STATS]);
		for (unsigned a = 0; a < RPG_ACTORS; a++) {
			ecs_entity_t e = rpg_replica(c, v, c->actors[a]);
			NEED(e && e != c->actors[a]);
			const struct rpg_stats *s = ecs_get_id(c->server.ecs,
				c->actors[a], c->server.types[RPG_STATS]);
			const struct rpg_stats *r =
				ecs_get_id(w->ecs, e, w->types[RPG_STATS]);
			NEED(r && r->health == s->health && r->mana == s->mana);
			NEED(r->level == s->level && r->status == s->status);
			NEED(r->position.x == s->position.x);
			NEED(r->position.y == s->position.y);
			NEED(!memcmp(r->attributes, s->attributes,
				sizeof(r->attributes)));
			const struct rpg_label *label =
				ecs_get_id(w->ecs, e, w->types[RPG_LABEL]);
			const struct rpg_label *original =
				ecs_get_id(c->server.ecs, c->actors[a],
					c->server.types[RPG_LABEL]);
			NEED(label && !strcmp(label->value, original->value));
		}
		for (unsigned i = 0; i < RPG_ITEMS; i++) {
			ecs_entity_t e = rpg_replica(c, v, c->items[i]);
			NEED(e);
			const struct rpg_item *r =
				ecs_get_id(w->ecs, e, w->types[RPG_ITEM]);
			NEED(r && r->weight == 0.25 + i && r->rarity == i % 3);
		}
	}
	return true;
}

bool rpg_expect_private(struct rpg_context *c, char *error, size_t size)
{
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		for (unsigned a = 0; a < RPG_ACTORS; a++) {
			ecs_entity_t e = rpg_replica(c, v, c->details[a]);
			bool visible = v == RPG_GM || v == a;
			NEED(!!e == visible);
			NEED(!!rpg_replica(c, v, c->bags[a]) == visible);
			if (!e)
				continue;
			const struct rpg_details *d =
				ecs_get_id(c->clients[v].ecs, e,
					c->clients[v].types[RPG_DETAILS]);
			const struct rpg_details *s = ecs_get_id(c->server.ecs,
				c->details[a], c->server.types[RPG_DETAILS]);
			NEED(d && d->gold == s->gold);
			NEED(d->experience == s->experience);
			NEED(d->owner == rpg_replica(c, v, c->actors[a]));
			NEED((!d->quest && !s->quest) ||
				(d->quest && s->quest &&
					!strcmp(d->quest, s->quest)));
		}
		for (size_t i = 0; i < c->entity_count; i++) {
			ecs_entity_t e = c->entities[i].entity;
			if (!ecs_has_id(c->server.ecs, e,
				    c->server.types[RPG_STACK]))
				continue;
			bool visible =
				!!(c->entities[i].visibility & (1u << v));
			NEED(!!rpg_replica(c, v, e) == visible);
		}
	}
	return true;
}

bool rpg_expect_queued(struct rpg_context *c, char *error, size_t size)
{
	sync_frame_view_t now;
	RESULT(sync_outbox_peek(c->server.session, c->peers[RPG_ARIA], &now),
		SYNC_OK);
	NEED(now.data == c->queued.data && now.size == c->queued.size);
	NEED(!memcmp(&now.token, &c->queued.token, sizeof(now.token)));
	return true;
}
