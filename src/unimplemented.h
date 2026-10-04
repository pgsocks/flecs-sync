#ifndef SYNC_INTERNAL_H
#define SYNC_INTERNAL_H
#include <sync.h>
#include <stdbool.h>

#define SYNC_HEADER 56u

struct sync_buffer {
	unsigned char *data;
	size_t size, capacity;
};
struct sync_reader {
	const unsigned char *data;
	size_t size, offset;
	sync_result_t error;
};
struct sync_type {
	ecs_entity_t id;
	char *name;
	struct sync_buffer shape;
	ecs_query_t *query;
};
struct sync_sample {
	struct sync_buffer value, references;
	uint64_t revision;
	bool present, dirty;
	sync_result_t error;
};
struct sync_mapping {
	ecs_entity_t entity;
	uint64_t id;
	struct sync_sample *samples;
};
struct sync_known {
	uint64_t id;
	uint64_t *revisions;
};
struct sync_frame {
	struct sync_frame *next;
	sync_frame_info_t info;
	sync_outbox_token_t token;
	size_t size;
	unsigned char data[];
};
struct sync_peer {
	struct sync_peer *next;
	ecs_entity_t entity;
	sync_state_t state;
	uint64_t stream, sequence, tick;
	sync_error_t fault;
	struct sync_frame *head, *tail;
	size_t queued;
	struct sync_known *known;
	uint32_t count;
};
struct sync_session {
	struct sync_session *next;
	ecs_world_t *world;
	sync_ids_t ids;
	sync_role_t role;
	sync_state_t state;
	sync_schedule_t schedule;
	sync_limits_t limits;
	sync_session_id_t identity;
	sync_error_t fault;
	size_t memory;
	sync_result_t alloc_error;
	struct sync_type *types;
	uint32_t type_count;
	struct sync_buffer manifest;
	struct sync_mapping *maps;
	uint32_t map_count;
	struct sync_peer *peers;
	uint32_t peer_count;
	uint64_t network_counter, stream_counter, auto_counter;
	uint64_t tick, sequence, stream;
	ecs_entity_t system;
	double interval, accumulator;
	uint32_t budget;
	struct sync_frame *head, *tail;
	size_t queued;
	struct sync_buffer assembly;
	uint64_t assembly_tick;
	uint32_t assembly_fragments, expected_fragments;
	bool snapshot;
	size_t expected_bytes;
	uint32_t *wire_types;
	uint32_t wire_count;
};

void *sync_alloc(sync_session_t *s, size_t size);
void sync_free(sync_session_t *s, void *ptr);
sync_result_t sync_reserve(sync_session_t *s, struct sync_buffer *b,
	size_t extra, size_t limit);
sync_result_t sync_bytes(sync_session_t *s, struct sync_buffer *b,
	const void *data, size_t size, size_t limit);
sync_result_t sync_number(sync_session_t *s, struct sync_buffer *b,
	uint64_t n, unsigned width, size_t limit);
uint64_t sync_read_number(struct sync_reader *r, unsigned width);
const unsigned char *sync_read_bytes(struct sync_reader *r, size_t size);
void sync_buffer_fini(sync_session_t *s, struct sync_buffer *b);
void sync_diagnostic(sync_error_t *e, sync_result_t code,
	ecs_entity_t peer, uint64_t tick, uint64_t sequence);
sync_result_t sync_fault(sync_session_t *s, sync_result_t code);
struct sync_peer *sync_find_peer(const sync_session_t *s, ecs_entity_t e);
struct sync_mapping *sync_find_map(const sync_session_t *s, uint64_t id);
void sync_queue_fini(sync_session_t *s, struct sync_frame **head,
	struct sync_frame **tail);
void sync_maps_fini(sync_session_t *s, bool delete_entities);
void sync_known_fini(sync_session_t *s, struct sync_known *known,
	uint32_t count);
bool sync_test_fail(unsigned fault);
sync_result_t sync_schema(sync_session_t *s);
sync_result_t sync_encode(sync_session_t *s, ecs_entity_t type,
	const void *value, struct sync_buffer *b, struct sync_buffer *refs);
sync_result_t sync_decode(sync_session_t *s, ecs_entity_t type,
	void *value, struct sync_reader *r);
sync_result_t sync_translate(sync_session_t *s, ecs_entity_t type,
	void *value, const struct sync_mapping *maps, uint32_t count);
sync_result_t sync_capture_core(sync_session_t *s, uint64_t tick,
	sync_capture_result_t *out);
sync_result_t sync_apply_core(sync_session_t *s, uint32_t budget,
	sync_apply_result_t *out);
void sync_value_fini(sync_session_t *s, ecs_entity_t type, void *value);
sync_result_t sync_validate_refs(sync_session_t *s, ecs_entity_t type,
	void *value, const struct sync_mapping *maps, uint32_t count);
sync_result_t sync_validate_local_refs(sync_session_t *s, ecs_entity_t type,
	const void *value, const struct sync_mapping *maps, uint32_t count);
void sync_write_header(unsigned char *data, const sync_frame_info_t *info);
#endif
