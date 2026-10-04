#ifndef SYNC_CUCUMBER_PROTOCOL_H
#define SYNC_CUCUMBER_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cucumber_wait(unsigned operation, uint32_t signature);
void cucumber_reply(void);
void cucumber_observe(int64_t actual, int64_t expected);
void cucumber_done(void);
void cucumber_queries(bool (*query)(const char *));
void cucumber_begin(char *name, size_t size);

/* Assertions in private scripts return observations to the Ruby runner. */
#undef CHECK
#undef EQ
#undef OK
#undef assert
#undef NEED
#undef RESULT
#define CHECK(x) cucumber_observe(!!(x), 1)
#define EQ(x, y) cucumber_observe((int64_t)(x), (int64_t)(y))
#define OK(x) EQ(x, SYNC_OK)
#define assert(x) CHECK(x)
#define NEED(x) do { \
	(void)error; \
	(void)size; \
	CHECK(x); \
} while (0)
#define RESULT(x, y) do { \
	(void)error; \
	(void)size; \
	EQ(x, y); \
} while (0)
#endif
