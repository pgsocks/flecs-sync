#include "private.h"
#include <string.h>

static struct {
	bool armed, hit;
	size_t remaining;
} controls[FILE_SYNC_TEST_FAULTS];

void file_sync_test_arm(enum file_sync_test_fault fault, size_t n)
{
	controls[fault].armed = true;
	controls[fault].hit = false;
	controls[fault].remaining = n;
}

bool file_sync_test_hit(enum file_sync_test_fault fault)
{
	return controls[fault].hit;
}

void file_sync_test_clear(void)
{
	memset(controls, 0, sizeof(controls));
}

bool file_sync_test_fail(unsigned fault)
{
	if (fault >= FILE_SYNC_TEST_FAULTS || !controls[fault].armed)
		return false;
	if (controls[fault].remaining) {
		controls[fault].remaining--;
		return false;
	}
	controls[fault].armed = false;
	controls[fault].hit = true;
	return true;
}
