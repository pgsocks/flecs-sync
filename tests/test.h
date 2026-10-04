#ifndef SYNC_TEST_H
#define SYNC_TEST_H
#include <sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

_Noreturn void test_fail(const char *file, int line, const char *expression);
#define CHECK(expr) do { \
	if (!(expr)) \
		test_fail(__FILE__, __LINE__, #expr); \
} while (0)
#define EQ(actual, expected) do { \
	int64_t actual_ = (int64_t)(actual); \
	int64_t expected_ = (int64_t)(expected); \
	if (actual_ != expected_) { \
		fprintf(stderr, "actual=%lld expected=%lld\n", \
			(long long)actual_, (long long)expected_); \
		test_fail(__FILE__, __LINE__, #actual " == " #expected); \
	} \
} while (0)
#define OK(call) EQ(call, SYNC_OK)
#define TEST(name) void name(void)

struct position { float x, y; };
struct record {
	struct position nested;
	int32_t array[3];
	uint64_t number;
	double real;
	char *text;
	ecs_entity_t target;
	bool flag;
};
struct world {
	ecs_world_t *ecs;
	sync_ids_t ids;
	ecs_entity_t position, velocity, health, tag, reference, record;
	ecs_entity_t local;
	sync_session_t *session;
};
struct fixture {
	struct world server, clients[2];
	ecs_entity_t peers[2];
};
struct packet {
	void *data;
	size_t size;
	sync_frame_info_t info;
};
struct packets {
	struct packet *items;
	size_t count;
};
extern int live_records;
void fixture_native_check(void);
void world_init(struct world *w, bool reverse);
void world_fini(struct world *w);
void fixture_init(struct fixture *f, const sync_server_desc_t *server,
	const sync_client_desc_t *client);
void fixture_fini(struct fixture *f);
ecs_entity_t spawn(struct world *w, float x, ecs_entity_t peer);
void set_position(struct world *w, ecs_entity_t entity, float x);
uint64_t network_id(struct world *w, ecs_entity_t entity);
ecs_entity_t replica(struct world *w, uint64_t id);
void expect_position(struct world *w, uint64_t id, float x);
void expect_absent(struct world *w, uint64_t id);
sync_status_t status(sync_session_t *s, ecs_entity_t peer);
void check_status_equal(const sync_status_t *a, const sync_status_t *b);
void check_info_equal(const sync_frame_info_t *a, const sync_frame_info_t *b);
void capture(struct fixture *f, uint64_t tick);
void transfer(struct fixture *f, unsigned client);
void drain(sync_session_t *client);
struct packets take_packets(sync_session_t *server, ecs_entity_t peer);
void packets_fini(struct packets *p);
void receive_packets(sync_session_t *client, const struct packets *p);
void set_record(struct world *w, ecs_entity_t e, struct record value);
ecs_entity_t reflected(struct world *w, const char *name, size_t size,
	size_t alignment, const ecs_member_t *members, size_t count);
#endif
