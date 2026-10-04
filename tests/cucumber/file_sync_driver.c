#include "test.h"
#include "protocol.h"
#define CASE(name) void name(void);
CASE(file_session_info)
CASE(file_roundtrip)
CASE(file_format)
CASE(file_checksum_vector)
CASE(file_arguments)
CASE(file_options)
CASE(file_io_open)
CASE(file_replace)
CASE(file_header_errors)
CASE(file_crc_and_limits)
CASE(file_partial_records)
CASE(file_logical_eof)
CASE(file_writer_sequence)
CASE(file_fragmented_backpressure)
CASE(file_eof_pending)
CASE(file_borrowed_client)
CASE(file_automatic)
CASE(file_auto_manual_client)
CASE(file_phase_order)
CASE(file_writer_budget)
CASE(file_later_failure)
CASE(file_schema_failure)
#ifdef FILE_SYNC_TESTING
CASE(file_fault_allocations)
CASE(file_fault_write_commit)
CASE(file_fault_flush_close)
CASE(file_fault_read)
#endif

#undef CASE
#define CASE(name) {#name, name},
struct script { const char *name; void (*operations)(void); };
static const struct script scripts[] = {
CASE(file_session_info)
CASE(file_roundtrip)
CASE(file_format)
CASE(file_checksum_vector)
CASE(file_arguments)
CASE(file_options)
CASE(file_io_open)
CASE(file_replace)
CASE(file_header_errors)
CASE(file_crc_and_limits)
CASE(file_partial_records)
CASE(file_logical_eof)
CASE(file_writer_sequence)
CASE(file_fragmented_backpressure)
CASE(file_eof_pending)
CASE(file_borrowed_client)
CASE(file_automatic)
CASE(file_auto_manual_client)
CASE(file_phase_order)
CASE(file_writer_budget)
CASE(file_later_failure)
CASE(file_schema_failure)
#ifdef FILE_SYNC_TESTING
CASE(file_fault_allocations)
CASE(file_fault_write_commit)
CASE(file_fault_flush_close)
CASE(file_fault_read)
#endif

};
#undef CASE

int main(void)
{
	char name[128];

	cucumber_begin(name, sizeof(name));
	for (unsigned i = 0; i < sizeof(scripts) / sizeof(*scripts); i++) {
		if (strcmp(name, scripts[i].name))
			continue;
		scripts[i].operations();
		cucumber_done();
		return 0;
	}
	fprintf(stderr, "unknown workflow: %s\n", name);
	return 1;
}
