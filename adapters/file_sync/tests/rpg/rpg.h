#ifndef RPG_FIXTURE_H
#define RPG_FIXTURE_H
#include <file_sync.h>
#include <stdint.h>

enum { RPG_ARIA, RPG_BRAM, RPG_GM, RPG_VIEWS };
enum { RPG_MIRA = 2, RPG_SKULK = 3, RPG_ACTORS = 4 };
enum { RPG_POTION, RPG_ARROW, RPG_SWORD, RPG_DAGGER, RPG_COIN, RPG_ITEMS };
enum {
	RPG_STATS,
	RPG_LABEL,
	RPG_DETAILS,
	RPG_BAG,
	RPG_STACK,
	RPG_ITEM,
	RPG_DEAD,
	RPG_TYPES
};
struct rpg_position {
	float x, y;
};
struct rpg_stats {
	struct rpg_position position;
	int32_t health, mana;
	uint32_t level, status;
	int32_t attributes[3];
};
struct rpg_label {
	char *value;
};
struct rpg_details {
	ecs_entity_t owner;
	uint32_t gold;
	uint64_t experience;
	char *quest;
};
struct rpg_bag {
	ecs_entity_t owner, slots[8], equipped;
};
struct rpg_stack {
	ecs_entity_t inventory, item;
	uint32_t quantity;
};
struct rpg_item {
	double weight;
	uint32_t rarity;
};
struct rpg_world {
	ecs_world_t *ecs;
	sync_ids_t ids;
	ecs_entity_t types[RPG_TYPES], local;
	sync_session_t *session;
};
struct rpg_entity {
	ecs_entity_t entity;
	uint64_t network_id;
	unsigned visibility;
};
struct rpg_context {
	struct rpg_world server, clients[RPG_VIEWS], live[RPG_VIEWS];
	file_sync_writer_t *writers[RPG_VIEWS];
	file_sync_reader_t *readers[RPG_VIEWS];
	file_sync_writer_t *probe_writer;
	file_sync_reader_t *probe_reader;
	ecs_entity_t peers[RPG_VIEWS], live_peers[RPG_VIEWS];
	ecs_entity_t actors[RPG_ACTORS], details[RPG_ACTORS];
	ecs_entity_t bags[RPG_ACTORS], items[RPG_ITEMS];
	struct rpg_entity entities[128];
	size_t entity_count;
	uint64_t tick, transferred_id, bag_id;
	ecs_entity_t transferred, removed_peer;
	bool raw, writer_auto, server_auto, client_auto, reader_auto;
	bool small_queue, phase_seen, close_released;
	uint32_t auto_budget, reader_budget;
	ecs_entity_t game_system, downstream_system;
	int32_t systems_before, client_systems_before;
	file_sync_progress_t progress;
	sync_result_t result;
	file_sync_error_t error, first_fault;
	sync_frame_view_t queued;
	unsigned char *delivered[RPG_VIEWS];
	size_t delivered_bytes[RPG_VIEWS];
	uint64_t delivered_frames[RPG_VIEWS];
};

#endif
