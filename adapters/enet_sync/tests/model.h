#ifndef ES_MODEL_H
#define ES_MODEL_H
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <enet_sync.h>
#ifdef ENET_SYNC_TESTING
#include "test_transport.h"
#endif
#ifdef ES_FILE_SYNC
#include <file_sync.h>
#endif

struct health { int32_t value; };
struct item { ecs_entity_t owner; int32_t quantity; };
struct world {
	ecs_world_t *ecs;
	sync_ids_t ids;
	sync_session_t *session;
	ecs_entity_t health, item;
};
struct game {
	struct world server, clients[3];
	enet_sync_server_t *listener;
	enet_sync_client_t *connections[3];
	ecs_entity_t peers[3], actors[3], items[3];
	uint32_t connected, disconnected, faults;
	uint64_t recorded, fragments, max_packet;
	sync_result_t recording_error;
	uint32_t mask[3];
	bool check_reentry;
#ifdef ES_FILE_SYNC
	file_sync_writer_t *writers[3];
#endif
};
void world_open(struct world *, bool, bool, uint32_t, size_t);
void world_close(struct world *);
void game_open(struct game *, bool, uint32_t, size_t,
	const enet_sync_options_t *, unsigned);
void game_close(struct game *);
void game_connect(struct game *, unsigned);
void game_settle(struct game *, unsigned, uint64_t, bool);
void game_capture(struct game *, uint64_t);
ecs_entity_t replica(struct game *, unsigned, ecs_entity_t);
void game_check(struct game *, unsigned);
void game_policies(struct game *);
#endif
