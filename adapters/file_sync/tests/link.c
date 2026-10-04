#include <file_sync.h>

static sync_result_t (*volatile ref_file_sync_writer_open)(
	const char *path,
	const file_sync_writer_desc_t *desc, file_sync_writer_t **out_writer,
	file_sync_error_t *out_error) =
	file_sync_writer_open;
static sync_result_t (*volatile ref_file_sync_writer_write)(
	file_sync_writer_t *writer,
	const void *data, size_t size) =
	file_sync_writer_write;
static sync_result_t (*volatile ref_file_sync_writer_pump)(
	file_sync_writer_t *writer,
	uint32_t max_frames, file_sync_progress_t *out_progress) =
	file_sync_writer_pump;
static sync_result_t (*volatile ref_file_sync_writer_flush)(
	file_sync_writer_t *writer) =
	file_sync_writer_flush;
static sync_result_t (*volatile ref_file_sync_writer_get_status)(
	const file_sync_writer_t *writer,
	file_sync_status_t *out_status) =
	file_sync_writer_get_status;
static sync_result_t (*volatile ref_file_sync_writer_close)(
	file_sync_writer_t *writer,
	file_sync_error_t *out_error) =
	file_sync_writer_close;
static sync_result_t (*volatile ref_file_sync_reader_open)(
	const char *path,
	const file_sync_reader_desc_t *desc, file_sync_reader_t **out_reader,
	file_sync_error_t *out_error) =
	file_sync_reader_open;
static sync_result_t (*volatile ref_file_sync_reader_peek)(
	file_sync_reader_t *reader,
	file_sync_frame_t *out_frame) =
	file_sync_reader_peek;
static sync_result_t (*volatile ref_file_sync_reader_consume)(
	file_sync_reader_t *reader) =
	file_sync_reader_consume;
static sync_result_t (*volatile ref_file_sync_reader_pump)(
	file_sync_reader_t *reader,
	uint32_t max_frames, file_sync_progress_t *out_progress) =
	file_sync_reader_pump;
static sync_result_t (*volatile ref_file_sync_reader_step)(
	file_sync_reader_t *reader,
	uint32_t max_updates, file_sync_progress_t *out_progress) =
	file_sync_reader_step;
static sync_result_t (*volatile ref_file_sync_reader_get_status)(
	const file_sync_reader_t *reader,
	file_sync_status_t *out_status) =
	file_sync_reader_get_status;
static sync_result_t (*volatile ref_file_sync_reader_close)(
	file_sync_reader_t *reader,
	file_sync_error_t *out_error) =
	file_sync_reader_close;

int main(void)
{
	if (!ref_file_sync_writer_open)
		return 1;
	if (!ref_file_sync_writer_write)
		return 1;
	if (!ref_file_sync_writer_pump)
		return 1;
	if (!ref_file_sync_writer_flush)
		return 1;
	if (!ref_file_sync_writer_get_status)
		return 1;
	if (!ref_file_sync_writer_close)
		return 1;
	if (!ref_file_sync_reader_open)
		return 1;
	if (!ref_file_sync_reader_peek)
		return 1;
	if (!ref_file_sync_reader_consume)
		return 1;
	if (!ref_file_sync_reader_pump)
		return 1;
	if (!ref_file_sync_reader_step)
		return 1;
	if (!ref_file_sync_reader_get_status)
		return 1;
	if (!ref_file_sync_reader_close)
		return 1;
	return 0;
}
