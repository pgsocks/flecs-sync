#ifndef RPG_TEST_H
#define RPG_TEST_H
#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The main function always destroys the fixture after a failed check. */
#define NEED(x) \
	do { \
		if (!(x)) { \
			if (!*error) \
				snprintf(error, size, "%s:%d: %s", __FILE__, \
					__LINE__, #x); \
			return false; \
		} \
	} while (0)
#define RESULT(call, expected) \
	do { \
		sync_result_t actual = (call); \
		if (actual != (expected)) { \
			snprintf(error, size, \
				"%s:%d: %s: expected %d, actual %d", __FILE__, \
				__LINE__, #call, (int)(expected), \
				(int)actual); \
			return false; \
		} \
	} while (0)
#define TEST(name) \
	bool name(struct rpg_context *c, const char *example, char *error, \
		size_t size)
#define CASE(id, example, function) TEST(function);
#include "cases.def"
#undef CASE

file_sync_status_t rpg_writer_status(struct rpg_context *c, unsigned view);
file_sync_status_t rpg_reader_status(struct rpg_context *c, unsigned view);
bool rpg_expect_stats(struct rpg_context *c, char *error, size_t size);
bool rpg_expect_private(struct rpg_context *c, char *error, size_t size);
bool rpg_expect_health(struct rpg_context *c, unsigned view, unsigned actor,
	int32_t health, uint64_t tick, char *error, size_t size);
bool rpg_expect_quantity(struct rpg_context *c, unsigned view, unsigned actor,
	unsigned item, int64_t quantity, char *error, size_t size);
bool rpg_expect_no_replicas(
	struct rpg_context *c, unsigned view, char *error, size_t size);
bool rpg_expect_queued(struct rpg_context *c, char *error, size_t size);
bool rpg_expect_retained(
	struct rpg_context *c, unsigned view, char *error, size_t size);
bool rpg_replay_all(
	struct rpg_context *c, unsigned updates, char *error, size_t size);
bool rpg_capture_queued(struct rpg_context *c, char *error, size_t size);
bool rpg_large_quest(struct rpg_context *c, char *error, size_t size);
bool rpg_expect_large_quest(struct rpg_context *c, char *error, size_t size);
#endif
