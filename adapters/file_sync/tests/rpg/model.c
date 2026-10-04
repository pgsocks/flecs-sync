#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *actor_names[] = {"Aria", "Bram", "Mira", "Skulk"};
static const char *item_names[] = {
	"HealingPotion", "Arrow", "IronSword", "RustyDagger", "GoldCoin"};
static const char *paths[] = {"Aria.bin", "Bram.bin", "GM.bin"};
static const char *notes[] = {
	"Find the lost relic", NULL, "Sell stolen goods", ""};
static int live_labels, live_details;

const char *rpg_path(unsigned view)
{
	return paths[view];
}

static void label_ctor(void *ptr, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	memset(ptr, 0, sizeof(struct rpg_label) * (size_t)count);
	live_labels += count;
}
static void label_dtor(void *ptr, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_label *p = ptr;
	for (int32_t i = 0; i < count; i++)
		ecs_os_free(p[i].value);
	live_labels -= count;
}
static void label_copy(
	void *dst, const void *src, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_label *d = dst;
	const struct rpg_label *s = src;
	for (int32_t i = 0; i < count; i++) {
		char *value = s[i].value ? ecs_os_strdup(s[i].value) : NULL;
		ecs_os_free(d[i].value);
		d[i].value = value;
	}
}
static void label_move(
	void *dst, void *src, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_label *d = dst, *s = src;
	for (int32_t i = 0; i < count; i++) {
		ecs_os_free(d[i].value);
		d[i] = s[i];
		s[i].value = NULL;
	}
}
static void details_ctor(void *ptr, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	memset(ptr, 0, sizeof(struct rpg_details) * (size_t)count);
	live_details += count;
}
static void details_dtor(void *ptr, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_details *p = ptr;
	for (int32_t i = 0; i < count; i++)
		ecs_os_free(p[i].quest);
	live_details -= count;
}
static void details_copy(
	void *dst, const void *src, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_details *d = dst;
	const struct rpg_details *s = src;
	for (int32_t i = 0; i < count; i++) {
		char *q = s[i].quest ? ecs_os_strdup(s[i].quest) : NULL;
		ecs_os_free(d[i].quest);
		d[i] = s[i];
		d[i].quest = q;
	}
}
static void details_move(
	void *dst, void *src, int32_t count, const ecs_type_info_t *ti)
{
	(void)ti;
	struct rpg_details *d = dst, *s = src;
	for (int32_t i = 0; i < count; i++) {
		ecs_os_free(d[i].quest);
		d[i] = s[i];
		s[i].quest = NULL;
	}
}

#define MEMBER(native, field, reflected) \
	{.name = #field, \
		.type = reflected, \
		.offset = offsetof(native, field), \
		.use_offset = true}
static void register_types(struct rpg_world *w, bool reverse, bool bad)
{
	const char *names[] = {"ActorStats", "Label", "PrivateDetails",
		"Inventory", "ItemStack", "ItemDefinition", "Dead"};
	ecs_entity_t pos = ecs_struct_init(w->ecs,
		&(ecs_struct_desc_t){
			.entity = ecs_entity_init(w->ecs,
				&(ecs_entity_desc_t){.name = "RpgPosition"}),
			.members = {{.name = "x", .type = ecs_id(ecs_f32_t)},
				{.name = "y", .type = ecs_id(ecs_f32_t)}}});
	for (unsigned index = 0; index < RPG_TYPES; index++) {
		unsigned t = reverse ? RPG_TYPES - 1 - index : index;
		w->types[t] = ecs_entity_init(
			w->ecs, &(ecs_entity_desc_t){.name = names[t]});
	}
	size_t sizes[] = {sizeof(struct rpg_stats), sizeof(struct rpg_label),
		sizeof(struct rpg_details), sizeof(struct rpg_bag),
		sizeof(struct rpg_stack), sizeof(struct rpg_item)};
	size_t aligns[] = {_Alignof(struct rpg_stats),
		_Alignof(struct rpg_label), _Alignof(struct rpg_details),
		_Alignof(struct rpg_bag), _Alignof(struct rpg_stack),
		_Alignof(struct rpg_item)};
	for (unsigned t = 0; t < RPG_DEAD; t++) {
		ecs_component_desc_t d = {.entity = w->types[t]};
		d.type.size = (ecs_size_t)sizes[t];
		d.type.alignment = (ecs_size_t)aligns[t];
		ecs_component_init(w->ecs, &d);
	}
	ecs_set_hooks_id(w->ecs, w->types[RPG_LABEL],
		&(ecs_type_hooks_t){.ctor = label_ctor,
			.dtor = label_dtor,
			.copy = label_copy,
			.move = label_move});
	ecs_set_hooks_id(w->ecs, w->types[RPG_DETAILS],
		&(ecs_type_hooks_t){.ctor = details_ctor,
			.dtor = details_dtor,
			.copy = details_copy,
			.move = details_move});
	ecs_struct_desc_t descs[] = {
		{.entity = w->types[RPG_STATS],
			.members = {MEMBER(struct rpg_stats, position, pos),
				MEMBER(struct rpg_stats, health,
					bad ? ecs_id(ecs_f32_t)
					    : ecs_id(ecs_i32_t)),
				MEMBER(struct rpg_stats, mana,
					ecs_id(ecs_i32_t)),
				MEMBER(struct rpg_stats, level,
					ecs_id(ecs_u32_t)),
				MEMBER(struct rpg_stats, status,
					ecs_id(ecs_u32_t)),
				{.name = "attributes",
					.type = ecs_id(ecs_i32_t),
					.count = 3,
					.offset = offsetof(
						struct rpg_stats, attributes),
					.use_offset = true}}},
		{.entity = w->types[RPG_LABEL],
			.members = {MEMBER(struct rpg_label, value,
				ecs_id(ecs_string_t))}},
		{.entity = w->types[RPG_DETAILS],
			.members = {MEMBER(struct rpg_details, owner,
					    ecs_id(ecs_entity_t)),
				MEMBER(struct rpg_details, gold,
					ecs_id(ecs_u32_t)),
				MEMBER(struct rpg_details, experience,
					ecs_id(ecs_u64_t)),
				MEMBER(struct rpg_details, quest,
					ecs_id(ecs_string_t))}},
		{.entity = w->types[RPG_BAG],
			.members = {MEMBER(struct rpg_bag, owner,
					    ecs_id(ecs_entity_t)),
				{.name = "slots",
					.type = ecs_id(ecs_entity_t),
					.count = 8,
					.offset =
						offsetof(struct rpg_bag, slots),
					.use_offset = true},
				MEMBER(struct rpg_bag, equipped,
					ecs_id(ecs_entity_t))}},
		{.entity = w->types[RPG_STACK],
			.members = {MEMBER(struct rpg_stack, inventory,
					    ecs_id(ecs_entity_t)),
				MEMBER(struct rpg_stack, item,
					ecs_id(ecs_entity_t)),
				MEMBER(struct rpg_stack, quantity,
					ecs_id(ecs_u32_t))}},
		{.entity = w->types[RPG_ITEM],
			.members = {MEMBER(struct rpg_item, weight,
					    ecs_id(ecs_f64_t)),
				MEMBER(struct rpg_item, rarity,
					ecs_id(ecs_u32_t))}}};
	for (unsigned t = 0; t < RPG_DEAD; t++)
		ecs_struct_init(w->ecs, &descs[t]);
	for (unsigned t = 0; t < RPG_TYPES; t++)
		ecs_add_id(w->ecs, w->types[t], w->ids.sync);
}
#undef MEMBER

static bool world_new(struct rpg_world *w, unsigned offset, bool bad,
	bool automatic, bool small, bool server)
{
	w->ecs = ecs_init();
	if (sync_import(w->ecs, &w->ids))
		return false;
	for (unsigned i = 0; i < offset; i++)
		ecs_new(w->ecs);
	register_types(w, offset != 0, bad);
	w->local = ecs_new(w->ecs);
	if (server) {
		sync_server_desc_t d = {.schedule = automatic,
			.send_hz = automatic ? 8 : 0,
			.session_id.bytes = {0x71}};
		return sync_server_create(w->ecs, &d, &w->session, NULL) == 0;
	}
	sync_client_desc_t d = {.schedule = automatic};
	if (small)
		d.limits.max_receive_bytes = 65536;
	return sync_client_create(w->ecs, &d, &w->session, NULL) == 0;
}

static void world_free(struct rpg_world *w)
{
	if (!w->ecs)
		return;
	sync_session_destroy(w->session);
	ecs_fini(w->ecs);
	memset(w, 0, sizeof(*w));
}

static struct rpg_entity *entry(struct rpg_context *c, ecs_entity_t entity)
{
	for (size_t i = 0; i < c->entity_count; i++)
		if (c->entities[i].entity == entity)
			return &c->entities[i];
	return NULL;
}
void rpg_policy(struct rpg_context *c, ecs_entity_t entity, unsigned mask)
{
	struct rpg_entity *e = entry(c, entity);
	if (e)
		e->visibility = mask;
	if (mask == 7) {
		ecs_add_id(
			c->server.ecs, entity, c->server.ids.replicate_to_all);
		return;
	}
	ecs_remove_id(c->server.ecs, entity, c->server.ids.replicate_to_all);
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		ecs_entity_t peers[] = {c->peers[v], c->live_peers[v]};
		for (unsigned p = 0; p < 2; p++) {
			if (!peers[p])
				continue;
			if (mask & (1u << v))
				ecs_add_pair(c->server.ecs, entity,
					c->server.ids.replicate_to, peers[p]);
			else
				ecs_remove_pair(c->server.ecs, entity,
					c->server.ids.replicate_to, peers[p]);
		}
	}
}

sync_result_t rpg_writer_open(struct rpg_context *c, unsigned v)
{
	file_sync_writer_desc_t d = {.server = c->server.session,
		.schedule = c->writer_auto,
		.frames_per_progress = c->auto_budget};
	sync_result_t r = file_sync_writer_open(
		paths[v], c->raw ? NULL : &d, &c->writers[v], &c->error);
	if (!r && !c->raw) {
		file_sync_status_t s;
		file_sync_writer_get_status(c->writers[v], &s);
		c->peers[v] = s.peer;
		for (size_t i = 0; i < c->entity_count; i++)
			if (ecs_is_alive(c->server.ecs, c->entities[i].entity))
				rpg_policy(c, c->entities[i].entity,
					c->entities[i].visibility);
	}
	return r;
}
sync_result_t rpg_writer_close(struct rpg_context *c, unsigned v)
{
	c->removed_peer = c->peers[v];
	sync_result_t r = file_sync_writer_close(c->writers[v], &c->error);
	c->writers[v] = NULL;
	c->peers[v] = 0;
	return r;
}

static ecs_entity_t entity_new(
	struct rpg_context *c, unsigned mask, const char *name)
{
	if (c->entity_count == 128)
		return 0;
	ecs_entity_t e = ecs_new(c->server.ecs);
	c->entities[c->entity_count++] = (struct rpg_entity){.entity = e};
	struct rpg_label label = {.value = (char *)name};
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_LABEL], sizeof(label),
		&label);
	rpg_policy(c, e, mask);
	return e;
}

static ecs_entity_t stack_new(
	struct rpg_context *c, unsigned actor, unsigned item, uint32_t quantity)
{
	struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[actor], c->server.types[RPG_BAG]);
	unsigned slot = 0;
	while (slot < 8 && bag.slots[slot])
		slot++;
	if (slot == 8)
		return 0;
	unsigned mask = entry(c, c->bags[actor])->visibility;
	ecs_entity_t e = entity_new(c, mask, "Item stack");
	struct rpg_stack s = {.inventory = c->bags[actor],
		.item = c->items[item],
		.quantity = quantity};
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(s), &s);
	bag.slots[slot] = e;
	ecs_set_id(c->server.ecs, c->bags[actor], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	return e;
}

bool rpg_initialize(struct rpg_context *c)
{
	if (!world_new(&c->server, 0, false, false, false, true))
		return false;
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		if (!world_new(&c->clients[v], 37 + 19 * v, false, false, false,
			    false) ||
			rpg_writer_open(c, v))
			return false;
	}
	for (unsigned i = 0; i < RPG_ITEMS; i++) {
		c->items[i] = entity_new(c, 7, item_names[i]);
		struct rpg_item item = {.weight = 0.25 + i, .rarity = i % 3};
		ecs_set_id(c->server.ecs, c->items[i],
			c->server.types[RPG_ITEM], sizeof(item), &item);
	}
	const int health[] = {100, 120, 40, 35}, mana[] = {30, 10, 0, 0};
	for (unsigned a = 0; a < RPG_ACTORS; a++) {
		c->actors[a] = entity_new(c, 7, actor_names[a]);
		struct rpg_stats stats = {.position = {(float)a, (float)a + 1},
			.health = health[a],
			.mana = mana[a],
			.level = a < 2 ? 5 : 2,
			.attributes = {10 + (int32_t)a, 20, 30}};
		ecs_set_id(c->server.ecs, c->actors[a],
			c->server.types[RPG_STATS], sizeof(stats), &stats);
		unsigned mask = a < 2 ? 4u | (1u << a) : 4u;
		c->details[a] = entity_new(c, mask, "Private details");
		struct rpg_details d = {.owner = c->actors[a],
			.gold = 100 + a,
			.experience = UINT64_C(1000) + a,
			.quest = (char *)notes[a]};
		ecs_set_id(c->server.ecs, c->details[a],
			c->server.types[RPG_DETAILS], sizeof(d), &d);
		c->bags[a] = entity_new(c, mask, "Inventory");
		struct rpg_bag bag = {.owner = c->actors[a]};
		ecs_set_id(c->server.ecs, c->bags[a], c->server.types[RPG_BAG],
			sizeof(bag), &bag);
	}
	stack_new(c, 0, RPG_POTION, 5);
	stack_new(c, 0, RPG_ARROW, 20);
	stack_new(c, 1, RPG_POTION, 3);
	stack_new(c, 1, RPG_SWORD, 1);
	stack_new(c, 2, RPG_POTION, 12);
	stack_new(c, 2, RPG_ARROW, 40);
	stack_new(c, 3, RPG_DAGGER, 1);
	stack_new(c, 3, RPG_COIN, 7);
	return true;
}

bool rpg_client_recreate(struct rpg_context *c, unsigned v, bool bad)
{
	if (c->readers[v])
		return false;
	world_free(&c->clients[v]);
	return world_new(&c->clients[v], 37 + 19 * v, bad, c->client_auto,
		c->small_queue, false);
}
bool rpg_configure(struct rpg_context *c)
{
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		if (rpg_writer_close(c, v))
			return false;
		if (!rpg_client_recreate(c, v, false))
			return false;
	}
	sync_session_destroy(c->server.session);
	c->server.session = NULL;
	sync_server_desc_t d = {.schedule = c->server_auto,
		.send_hz = c->server_auto ? 8 : 0,
		.session_id.bytes = {0x71}};
	if (sync_server_create(c->server.ecs, &d, &c->server.session, NULL))
		return false;
	for (unsigned v = 0; v < RPG_VIEWS; v++)
		if (rpg_writer_open(c, v))
			return false;
	return true;
}

bool rpg_raw_setup(struct rpg_context *c)
{
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		if (rpg_writer_close(c, v))
			return false;
		if (!world_new(
			    &c->live[v], 121 + v, false, false, false, false))
			return false;
		if (sync_peer_open(c->server.session, &c->live_peers[v]))
			return false;
	}
	c->raw = true;
	for (unsigned v = 0; v < RPG_VIEWS; v++)
		if (rpg_writer_open(c, v))
			return false;
	for (size_t i = 0; i < c->entity_count; i++)
		rpg_policy(c, c->entities[i].entity, c->entities[i].visibility);
	return true;
}

void rpg_identify(struct rpg_context *c)
{
	for (size_t i = 0; i < c->entity_count; i++) {
		struct rpg_entity *e = &c->entities[i];
		if (!ecs_is_alive(c->server.ecs, e->entity))
			continue;
		const sync_network_id_t *id = ecs_get_id(
			c->server.ecs, e->entity, c->server.ids.network_id);
		if (id)
			e->network_id = id->value;
	}
}
uint64_t rpg_network_id(const struct rpg_context *c, ecs_entity_t entity)
{
	for (size_t i = 0; i < c->entity_count; i++)
		if (c->entities[i].entity == entity)
			return c->entities[i].network_id;
	return 0;
}
ecs_entity_t rpg_replica(struct rpg_context *c, unsigned v, ecs_entity_t e)
{
	ecs_entity_t local = 0;
	uint64_t id = rpg_network_id(c, e);
	if (id)
		sync_lookup(c->clients[v].session, id, &local);
	return local;
}

sync_result_t rpg_capture(struct rpg_context *c, bool service)
{
	sync_result_t result = sync_capture(c->server.session, ++c->tick, NULL);
	rpg_identify(c);
	if (result < 0 || !service)
		return result;
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		if (!c->writers[v])
			continue;
		if (c->raw) {
			sync_frame_view_t frame;
			while (!(result = sync_outbox_peek(c->server.session,
					 c->live_peers[v], &frame))) {
				result = file_sync_writer_write(
					c->writers[v], frame.data, frame.size);
				if (!result)
					result =
						sync_receive(c->live[v].session,
							frame.data, frame.size);
				if (result)
					return result;
				size_t n = c->delivered_bytes[v];
				unsigned char *copy = realloc(
					c->delivered[v], n + frame.size);
				if (!copy)
					return SYNC_ERR_NOMEM;
				c->delivered[v] = copy;
				memcpy(copy + n, frame.data, frame.size);
				c->delivered_bytes[v] += frame.size;
				c->delivered_frames[v]++;
				result = sync_outbox_commit(c->server.session,
					c->live_peers[v], frame.token);
				if (result)
					return result;
			}
			if (result != SYNC_EMPTY)
				return result;
			result = sync_apply(
				c->live[v].session, UINT32_MAX, NULL);
			if (result < 0)
				return result;
		} else if (!c->writer_auto) {
			result = file_sync_writer_pump(
				c->writers[v], UINT32_MAX, &c->progress);
			if (result < 0)
				return result;
		}
		result = file_sync_writer_flush(c->writers[v]);
		if (result)
			return result;
	}
	return SYNC_OK;
}
sync_result_t rpg_reader_open(struct rpg_context *c, unsigned v)
{
	if (c->writers[v]) {
		sync_result_t r = file_sync_writer_flush(c->writers[v]);
		if (r)
			return r;
	}
	file_sync_reader_desc_t d = {.client = c->clients[v].session,
		.schedule = c->reader_auto,
		.frames_per_progress = c->reader_budget};
	return file_sync_reader_open(paths[v], &d, &c->readers[v], &c->error);
}
sync_result_t rpg_replay(struct rpg_context *c, unsigned v, unsigned count)
{
	sync_result_t r = SYNC_OK;
	if (!c->readers[v])
		r = rpg_reader_open(c, v);
	if (!r)
		r = file_sync_reader_step(c->readers[v], count, &c->progress);
	return r;
}
void rpg_health(struct rpg_context *c, unsigned actor, int32_t health)
{
	struct rpg_stats s = *(const struct rpg_stats *)ecs_get_id(
		c->server.ecs, c->actors[actor], c->server.types[RPG_STATS]);
	s.health = health;
	ecs_set_id(c->server.ecs, c->actors[actor], c->server.types[RPG_STATS],
		sizeof(s), &s);
}
void rpg_quest(struct rpg_context *c, unsigned actor, const char *quest)
{
	struct rpg_details d = *(const struct rpg_details *)ecs_get_id(
		c->server.ecs, c->details[actor], c->server.types[RPG_DETAILS]);
	d.quest = (char *)quest;
	ecs_set_id(c->server.ecs, c->details[actor],
		c->server.types[RPG_DETAILS], sizeof(d), &d);
}
ecs_entity_t rpg_find_stack(
	struct rpg_context *c, unsigned actor, unsigned item)
{
	const struct rpg_bag *bag = ecs_get_id(
		c->server.ecs, c->bags[actor], c->server.types[RPG_BAG]);
	for (unsigned i = 0; i < 8; i++) {
		if (!bag->slots[i])
			continue;
		const struct rpg_stack *s = ecs_get_id(c->server.ecs,
			bag->slots[i], c->server.types[RPG_STACK]);
		if (s->item == c->items[item])
			return bag->slots[i];
	}
	return 0;
}
bool rpg_split(struct rpg_context *c, unsigned a, unsigned item, uint32_t n)
{
	ecs_entity_t e = rpg_find_stack(c, a, item);
	if (!e)
		return false;
	struct rpg_stack s = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, e, c->server.types[RPG_STACK]);
	if (!n || n >= s.quantity || !stack_new(c, a, item, n))
		return false;
	s.quantity -= n;
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(s), &s);
	return true;
}
bool rpg_trade(struct rpg_context *c, unsigned from, unsigned to, unsigned item,
	uint32_t n)
{
	ecs_entity_t e = rpg_find_stack(c, from, item);
	if (!e)
		return false;
	struct rpg_stack s = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, e, c->server.types[RPG_STACK]);
	if (!n || n > s.quantity)
		return false;
	if (n < s.quantity) {
		s.quantity -= n;
		ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK],
			sizeof(s), &s);
		return stack_new(c, to, item, n) != 0;
	}
	struct rpg_bag old = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[from], c->server.types[RPG_BAG]);
	struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[to], c->server.types[RPG_BAG]);
	unsigned slot = 0;
	while (slot < 8 && bag.slots[slot])
		slot++;
	if (slot == 8)
		return false;
	for (unsigned i = 0; i < 8; i++)
		if (old.slots[i] == e)
			old.slots[i] = 0;
	if (old.equipped == e)
		old.equipped = 0;
	bag.slots[slot] = e;
	s.inventory = c->bags[to];
	ecs_set_id(c->server.ecs, c->bags[from], c->server.types[RPG_BAG],
		sizeof(old), &old);
	ecs_set_id(c->server.ecs, c->bags[to], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
	ecs_set_id(c->server.ecs, e, c->server.types[RPG_STACK], sizeof(s), &s);
	rpg_policy(c, e, entry(c, c->bags[to])->visibility);
	c->transferred = e;
	c->transferred_id = rpg_network_id(c, e);
	return true;
}
void rpg_merge(struct rpg_context *c, unsigned a, unsigned item)
{
	struct rpg_bag bag = *(const struct rpg_bag *)ecs_get_id(
		c->server.ecs, c->bags[a], c->server.types[RPG_BAG]);
	ecs_entity_t first = rpg_find_stack(c, a, item);
	struct rpg_stack total = *(const struct rpg_stack *)ecs_get_id(
		c->server.ecs, first, c->server.types[RPG_STACK]);
	for (unsigned i = 0; i < 8; i++) {
		ecs_entity_t e = bag.slots[i];
		if (!e || e == first)
			continue;
		const struct rpg_stack *s = ecs_get_id(
			c->server.ecs, e, c->server.types[RPG_STACK]);
		if (s->item != c->items[item])
			continue;
		total.quantity += s->quantity;
		bag.slots[i] = 0;
		if (bag.equipped == e)
			bag.equipped = first;
		ecs_delete(c->server.ecs, e);
	}
	ecs_set_id(c->server.ecs, first, c->server.types[RPG_STACK],
		sizeof(total), &total);
	ecs_set_id(c->server.ecs, c->bags[a], c->server.types[RPG_BAG],
		sizeof(bag), &bag);
}
void rpg_reveal(struct rpg_context *c, unsigned a, unsigned v, bool visible)
{
	unsigned mask = entry(c, c->bags[a])->visibility;
	mask = visible ? mask | (1u << v) : mask & ~(1u << v);
	rpg_policy(c, c->bags[a], mask);
	const struct rpg_bag *bag =
		ecs_get_id(c->server.ecs, c->bags[a], c->server.types[RPG_BAG]);
	for (unsigned i = 0; i < 8; i++)
		if (bag->slots[i])
			rpg_policy(c, bag->slots[i], mask);
}
int64_t rpg_quantity(struct rpg_context *c, unsigned v, unsigned a,
	unsigned item, unsigned *stacks)
{
	*stacks = 0;
	ecs_entity_t e = rpg_replica(c, v, c->bags[a]);
	if (!e)
		return -1;
	const struct rpg_bag *bag =
		ecs_get_id(c->clients[v].ecs, e, c->clients[v].types[RPG_BAG]);
	ecs_entity_t definition = rpg_replica(c, v, c->items[item]);
	int64_t quantity = 0;
	for (unsigned i = 0; i < 8; i++) {
		if (!bag->slots[i])
			continue;
		const struct rpg_stack *s = ecs_get_id(c->clients[v].ecs,
			bag->slots[i], c->clients[v].types[RPG_STACK]);
		if (!s)
			return -2;
		if (s->item == definition) {
			quantity += s->quantity;
			(*stacks)++;
		}
	}
	return quantity;
}
bool rpg_references(struct rpg_context *c)
{
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		struct rpg_world *w = &c->clients[v];
		for (unsigned a = 0; a < RPG_ACTORS; a++) {
			ecs_entity_t actor = rpg_replica(c, v, c->actors[a]);
			ecs_entity_t details = rpg_replica(c, v, c->details[a]);
			if (details) {
				const struct rpg_details *d = ecs_get_id(
					w->ecs, details, w->types[RPG_DETAILS]);
				if (!actor || !d || d->owner != actor)
					return false;
			}
			ecs_entity_t e = rpg_replica(c, v, c->bags[a]);
			if (!e)
				continue;
			const struct rpg_bag *bag =
				ecs_get_id(w->ecs, e, w->types[RPG_BAG]);
			if (!actor || actor == c->actors[a] ||
				bag->owner != actor)
				return false;
			bool equipped_found = !bag->equipped;
			for (unsigned i = 0; i < 8; i++) {
				if (!bag->slots[i])
					continue;
				const struct rpg_stack *s = ecs_get_id(w->ecs,
					bag->slots[i], w->types[RPG_STACK]);
				if (!s || s->inventory != e || !s->quantity)
					return false;
				bool item_found = false;
				for (unsigned item = 0; item < RPG_ITEMS;
					item++)
					if (s->item ==
						rpg_replica(
							c, v, c->items[item]))
						item_found = true;
				if (!item_found)
					return false;
				if (bag->equipped == bag->slots[i])
					equipped_found = true;
			}
			if (!equipped_found)
				return false;
		}
	}
	return true;
}
bool rpg_destroy(struct rpg_context *c)
{
	bool clean = true;
	file_sync_reader_close(c->probe_reader, NULL);
	file_sync_writer_close(c->probe_writer, NULL);
	c->probe_reader = NULL;
	c->probe_writer = NULL;
	for (unsigned v = 0; v < RPG_VIEWS; v++) {
		file_sync_reader_close(c->readers[v], NULL);
		file_sync_writer_close(c->writers[v], NULL);
		c->readers[v] = NULL;
		c->writers[v] = NULL;
		if (c->clients[v].ecs &&
			!ecs_is_alive(c->clients[v].ecs, c->clients[v].local))
			clean = false;
		world_free(&c->clients[v]);
		world_free(&c->live[v]);
		free(c->delivered[v]);
		c->delivered[v] = NULL;
		remove(paths[v]);
	}
	world_free(&c->server);
	remove("open-test.bin");
	remove("bad.bin");
	return clean && !live_labels && !live_details;
}
