#ifndef FILE_SYNC_PRIVATE_TEST_H
#define FILE_SYNC_PRIVATE_TEST_H
#ifndef FILE_SYNC_TESTING
#error "FileSync controls are test-only"
#endif
#include <stdbool.h>
#include <stddef.h>

enum file_sync_test_fault {
	FILE_SYNC_TEST_ALLOC,
	FILE_SYNC_TEST_WRITE,
	FILE_SYNC_TEST_READ,
	FILE_SYNC_TEST_FLUSH,
	FILE_SYNC_TEST_CLOSE,
	FILE_SYNC_TEST_FAULTS
};
/* Fail once after n successful wrapper calls. WRITE emits a partial write;
 * CLOSE still closes the real file. Controls never affect fixture I/O.
 */
void file_sync_test_arm(enum file_sync_test_fault fault, size_t n);
bool file_sync_test_hit(enum file_sync_test_fault fault);
void file_sync_test_clear(void);
bool file_sync_test_fail(unsigned fault);
#endif
