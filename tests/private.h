#ifndef SYNC_PRIVATE_TEST_H
#define SYNC_PRIVATE_TEST_H
#include <sync.h>
#ifndef SYNC_TESTING
#error "Private controls require the instrumented test build"
#endif

/* Test-build-only controls. These are not installed or part of the API.
	Each case runs in its own process; controls are owning-thread-only.
	Arm a one-shot failure after n successful operations of the given kind.
	ALLOCATION affects only Sync-owned allocations, never Flecs allocations.
	PUBLICATION fails before an ECS mutation after n completed mutations.
	Clear disarms controls and resets hit flags, without world work.
	Seed sets the last issued counter, so the next issuance checks overflow.
	Only SEQUENCE accepts a peer; all other counters require peer == 0.
	These definitions remain stubs until the core is implemented.
*/
enum sync_test_fault { SYNC_TEST_ALLOCATION, SYNC_TEST_PUBLICATION };
enum sync_test_counter {
	SYNC_TEST_NETWORK_ID, SYNC_TEST_STREAM_ID,
	SYNC_TEST_SEQUENCE, SYNC_TEST_AUTO_TICK
};
void sync_test_arm(enum sync_test_fault fault, size_t n);
bool sync_test_hit(enum sync_test_fault fault);
void sync_test_clear(void);
void sync_test_seed(sync_session_t *session, ecs_entity_t peer,
	enum sync_test_counter counter, uint64_t last_issued);
#endif
