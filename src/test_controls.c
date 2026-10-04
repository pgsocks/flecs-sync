#include "private.h"
#include "unimplemented.h"
#include <string.h>

static struct {
	size_t countdown;
	bool armed, hit;
} controls[2];

bool sync_test_fail(unsigned fault)
{
	if (fault >= 2 || !controls[fault].armed)
		return false;
	if (controls[fault].countdown) {
		controls[fault].countdown--;
		return false;
	}
	controls[fault].armed = false;
	controls[fault].hit = true;
	return true;
}

void sync_test_arm(enum sync_test_fault fault, size_t n)
{
	/*  arm a one-shot countdown in test-only allocation/publication
	 * wrappers. After n successful operations, fail once and record a hit.
	 * Never replace the Flecs OS allocator or affect fixture allocations.
	 */
	if ((unsigned)fault >= 2)
		return;
	controls[fault].countdown = n;
	controls[fault].armed = true;
	controls[fault].hit = false;
}

bool sync_test_hit(enum sync_test_fault fault)
{
	/*  copy the fault hit flag without consuming or rearming it. */
	return (unsigned)fault < 2 && controls[fault].hit;
}

void sync_test_clear(void)
{
	/*  clear all countdowns and hit flags without session mutation. */
	memset(controls, 0, sizeof(controls));
}

void sync_test_seed(sync_session_t *s, ecs_entity_t peer,
	enum sync_test_counter counter, uint64_t last_issued)
{
	/*  set the selected last-issued counter at a writable boundary.
	 * Resolve only SEQUENCE through peer; preserve mappings/queued frames.
	 * Clear its exhausted flag when reseeding, not any sticky stream fault.
	 */
	if (!s)
		return;
	if (counter == SYNC_TEST_SEQUENCE) {
		struct sync_peer *p = sync_find_peer(s, peer);
		if (p)
			p->sequence = last_issued;
		return;
	}
	if (peer)
		return;
	switch (counter) {
	case SYNC_TEST_NETWORK_ID: s->network_counter = last_issued; break;
	case SYNC_TEST_STREAM_ID: s->stream_counter = last_issued; break;
	case SYNC_TEST_AUTO_TICK: s->auto_counter = last_issued; break;
	case SYNC_TEST_SEQUENCE: break;
	}
}
