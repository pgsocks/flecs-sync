#ifndef RPG_MODEL_H
#define RPG_MODEL_H
#include "rpg.h"

/* Test model helpers use real public Sync/FileSync calls.
	Entity policies include every selected peer.
*/
const char *rpg_path(unsigned view);
bool rpg_initialize(struct rpg_context *c);
bool rpg_configure(struct rpg_context *c);
bool rpg_raw_setup(struct rpg_context *c);
bool rpg_client_recreate(struct rpg_context *c, unsigned view, bool bad);
void rpg_policy(struct rpg_context *c, ecs_entity_t entity, unsigned mask);
void rpg_identify(struct rpg_context *c);
uint64_t rpg_network_id(const struct rpg_context *c, ecs_entity_t entity);
ecs_entity_t rpg_replica(
	struct rpg_context *c, unsigned view, ecs_entity_t entity);
sync_result_t rpg_writer_open(struct rpg_context *c, unsigned view);
sync_result_t rpg_writer_close(struct rpg_context *c, unsigned view);
sync_result_t rpg_reader_open(struct rpg_context *c, unsigned view);
sync_result_t rpg_capture(struct rpg_context *c, bool service);
sync_result_t rpg_replay(struct rpg_context *c, unsigned view, unsigned count);
void rpg_health(struct rpg_context *c, unsigned actor, int32_t health);
void rpg_quest(struct rpg_context *c, unsigned actor, const char *quest);
ecs_entity_t rpg_find_stack(
	struct rpg_context *c, unsigned actor, unsigned item);
bool rpg_trade(struct rpg_context *c, unsigned from, unsigned to, unsigned item,
	uint32_t quantity);
bool rpg_split(struct rpg_context *c, unsigned actor, unsigned item,
	uint32_t quantity);
void rpg_merge(struct rpg_context *c, unsigned actor, unsigned item);
void rpg_reveal(
	struct rpg_context *c, unsigned actor, unsigned view, bool visible);
int64_t rpg_quantity(struct rpg_context *c, unsigned view, unsigned actor,
	unsigned item, unsigned *stacks);
bool rpg_references(struct rpg_context *c);
bool rpg_destroy(struct rpg_context *c);
#endif
