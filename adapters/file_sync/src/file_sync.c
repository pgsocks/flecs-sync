#include <file_sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Private test controls affect only adapter allocations and I/O wrappers. */
enum fs_fault { FS_ALLOC, FS_WRITE, FS_READ, FS_FLUSH, FS_CLOSE };
#ifdef FILE_SYNC_TESTING
#include "private.h"
#define injected(fault) file_sync_test_fail(fault)
#else
static bool injected(unsigned fault)
{
	(void)fault;
	return false;
}
#endif

struct fs_file {
	FILE *file;
	sync_session_t *session;
	sync_session_info_t info;
	file_sync_status_t status;
	ecs_entity_t system;
};
struct file_sync_writer {
	struct fs_file base;
};
struct file_sync_reader {
	struct fs_file base;
	unsigned char *buffer;
	size_t capacity;
	file_sync_frame_t pending;
};

static const unsigned char file_header[16] = {
	'F', 'I', 'L', 'E', 'S', 'Y', 'N', 'C', 1
};

static void le32_write(unsigned char *out, uint32_t value)
{
	for (unsigned i = 0; i < 4; i++)
		out[i] = (unsigned char)(value >> (8 * i));
}

static uint32_t le32_read(const unsigned char *in)
{
	uint32_t value = 0;
	for (unsigned i = 0; i < 4; i++)
		value |= (uint32_t)in[i] << (8 * i);
	return value;
}

static uint32_t crc_bytes(uint32_t crc, const void *data, size_t size)
{
	const unsigned char *bytes = data;
	for (size_t i = 0; i < size; i++) {
		crc ^= bytes[i];
		for (unsigned bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (UINT32_C(0xedb88320) &
				(0u - (crc & 1u)));
	}
	return crc;
}

static void *allocate(size_t size)
{
	return injected(FS_ALLOC) ? NULL : calloc(1, size);
}

static uint64_t current_record(const struct fs_file *f)
{
	return f->status.frames == UINT64_MAX ? UINT64_MAX
		: f->status.frames + 1;
}

static sync_result_t fail(struct fs_file *f, sync_result_t code,
	const char *message, int system_error, uint64_t record)
{
	if (f->status.state != FILE_SYNC_FAULTED) {
		file_sync_error_t *e = &f->status.fault;
		e->detail.code = code;
		e->detail.peer = f->status.peer;
		e->detail.tick = f->status.last_frame.tick;
		e->detail.sequence = f->status.last_frame.sequence;
		snprintf(e->detail.message, sizeof(e->detail.message),
			"%s", message);
		e->offset = f->status.offset;
		e->record = record;
		e->system_error = system_error;
		f->status.state = FILE_SYNC_FAULTED;
	}
	return code;
}

static sync_result_t core_failure(struct fs_file *f, sync_result_t code)
{
	if (f->status.state == FILE_SYNC_FAULTED)
		return f->status.fault.detail.code;
	sync_status_t s = {0};
	sync_get_status(f->session, f->status.peer, &s);
	if (s.fault.code)
		code = s.fault.code;
	fail(f, code, "Sync stream operation failed", 0, current_record(f));
	if (s.fault.code)
		f->status.fault.detail = s.fault;
	return code;
}

static sync_result_t write_bytes(struct fs_file *f, const void *data,
	size_t size, uint64_t record)
{
	if (size > UINT64_MAX - f->status.offset)
		return fail(f, SYNC_ERR_LIMIT, "file offset exhausted", 0,
			record);
	errno = 0;
	bool short_write = injected(FS_WRITE);
	size_t wanted = short_write ? size / 2 : size;
	size_t n = fwrite(data, 1, wanted, f->file);
	f->status.offset += n;
	if (short_write)
		errno = EIO;
	if (n != size)
		return fail(f, SYNC_ERR_IO, "short file write", errno, record);
	return SYNC_OK;
}

static sync_result_t read_bytes(struct fs_file *f, void *data, size_t size,
	bool allow_eof, uint64_t record)
{
	if (size > UINT64_MAX - f->status.offset)
		return fail(f, SYNC_ERR_LIMIT, "file offset exhausted", 0,
			record);
	errno = 0;
	if (injected(FS_READ))
		return fail(f, SYNC_ERR_IO, "file read failed", EIO, record);
	size_t n = fread(data, 1, size, f->file);
	f->status.offset += n;
	if (n == size)
		return SYNC_OK;
	if (ferror(f->file))
		return fail(f, SYNC_ERR_IO, "file read failed", errno, record);
	if (!n && allow_eof)
		return SYNC_END;
	return fail(f, SYNC_ERR_TRUNCATED, "incomplete file record",
		0, record);
}

static sync_result_t flush(struct fs_file *f)
{
	errno = 0;
	if (injected(FS_FLUSH))
		return fail(f, SYNC_ERR_IO, "file flush failed", EIO,
			f->status.frames);
	if (fflush(f->file))
		return fail(f, SYNC_ERR_IO, "file flush failed", errno,
			f->status.frames);
	return SYNC_OK;
}

static void close_file(struct fs_file *f)
{
	if (!f->file)
		return;
	errno = 0;
	bool failure = injected(FS_CLOSE);
	int result = fclose(f->file);
	f->file = NULL;
	if (failure || result)
		fail(f, SYNC_ERR_IO, "file close failed",
			failure ? EIO : errno, f->status.frames);
}

static bool writable(const ecs_world_t *world)
{
	return !(ecs_world_get_flags(world) & EcsWorldReadonly) &&
		!ecs_is_deferred(world);
}

static sync_result_t configure(struct fs_file *f, sync_session_t *session,
	sync_role_t role, sync_schedule_t schedule, uint32_t budget,
	uint32_t frame_limit)
{
	f->status.state = FILE_SYNC_ACTIVE;
	if ((schedule != SYNC_MANUAL && schedule != SYNC_AUTOMATIC) ||
		(schedule == SYNC_MANUAL && budget) ||
		(schedule == SYNC_AUTOMATIC && !session))
		return SYNC_ERR_ARGUMENT;
	if (session) {
		sync_result_t r = sync_get_session_info(session, &f->info);
		if (r)
			return r;
		if (f->info.role != role)
			return SYNC_ERR_ROLE;
		if (!writable(f->info.world))
			return SYNC_ERR_STATE;
		sync_status_t s;
		r = sync_get_status(session, 0, &s);
		if (r)
			return r;
		if ((role == SYNC_ROLE_SERVER &&
			 s.state != SYNC_SERVER_READY) ||
			(role == SYNC_ROLE_CLIENT &&
			 (s.state != SYNC_CLIENT_WAIT_MANIFEST ||
			  s.queued_bytes || s.assembly_bytes || s.entities)))
			return SYNC_ERR_STATE;
		if (role == SYNC_ROLE_CLIENT && schedule == SYNC_AUTOMATIC &&
			f->info.schedule != SYNC_AUTOMATIC)
			return SYNC_ERR_STATE;
		if (frame_limit > f->info.limits.max_frame_bytes)
			return SYNC_ERR_ARGUMENT;
		if (!frame_limit)
			frame_limit = f->info.limits.max_frame_bytes;
	}
	f->session = session;
	f->status.schedule = schedule;
	f->status.frames_per_progress = schedule == SYNC_AUTOMATIC
		? (budget ? budget : 64) : 0;
	f->status.max_frame_bytes = frame_limit ? frame_limit : 65536;
	return SYNC_OK;
}

static sync_result_t validate_stream(struct fs_file *f,
	const sync_frame_info_t *info)
{
	const sync_frame_info_t *last = &f->status.last_frame;
	if (!last->sequence) {
		if (info->sequence == 1 && info->kind == SYNC_FRAME_MANIFEST)
			return SYNC_OK;
	} else if (last->sequence != UINT64_MAX &&
		info->sequence == last->sequence + 1 &&
		info->stream_id == last->stream_id &&
		!memcmp(info->session_id.bytes, last->session_id.bytes, 16)) {
		return SYNC_OK;
	}
	return fail(f, SYNC_ERR_SEQUENCE, "unexpected frame sequence/stream",
		0, current_record(f));
}

static sync_result_t write_frame(file_sync_writer_t *w,
	const void *data, size_t size)
{
	struct fs_file *f = &w->base;
	if (size > f->status.max_frame_bytes)
		return fail(f, SYNC_ERR_LIMIT, "frame exceeds file limit", 0,
			current_record(f));
	if (f->status.frames == UINT64_MAX ||
		size > UINT64_MAX - f->status.bytes)
		return fail(f, SYNC_ERR_LIMIT, "record counters exhausted", 0,
			current_record(f));
	sync_frame_info_t info;
	sync_result_t r = sync_frame_inspect(data, size, &info);
	if (r)
		return fail(f, r, "invalid Sync frame envelope", 0,
			current_record(f));
	r = validate_stream(f, &info);
	if (r)
		return r;
	unsigned char length[4], checksum[4];
	le32_write(length, (uint32_t)size);
	uint32_t crc = crc_bytes(UINT32_MAX, length, 4);
	crc = crc_bytes(crc, data, size) ^ UINT32_MAX;
	le32_write(checksum, crc);
	uint64_t record = current_record(f);
	r = write_bytes(f, length, 4, record);
	if (!r)
		r = write_bytes(f, data, size, record);
	if (!r)
		r = write_bytes(f, checksum, 4, record);
	if (r)
		return r;
	f->status.frames++;
	f->status.bytes += size;
	f->status.last_frame = info;
	return SYNC_OK;
}

static sync_result_t writer_pump(file_sync_writer_t *w, uint32_t budget,
	file_sync_progress_t *out)
{
	file_sync_progress_t p = {0};
	sync_result_t r = SYNC_OK;
	for (uint32_t i = 0; i < budget; i++) {
		sync_frame_view_t v;
		r = sync_outbox_peek(w->base.session, w->base.status.peer, &v);
		if (r == SYNC_EMPTY)
			break;
		if (r) {
			r = core_failure(&w->base, r);
			break;
		}
		r = write_frame(w, v.data, v.size);
		if (r)
			break;
		r = sync_outbox_commit(w->base.session, w->base.status.peer,
			v.token);
		if (r) {
			r = core_failure(&w->base, r);
			break;
		}
		p.frames++;
		p.bytes += v.size;
	}
	if (out)
		*out = p;
	if (r < 0)
		return r;
	return p.frames ? SYNC_OK : SYNC_EMPTY;
}

static sync_result_t reader_peek(file_sync_reader_t *reader,
	file_sync_frame_t *out)
{
	struct fs_file *f = &reader->base;
	if (reader->pending.data) {
		*out = reader->pending;
		return SYNC_OK;
	}
	if (f->status.state == FILE_SYNC_EOF_PENDING ||
		f->status.state == FILE_SYNC_ENDED)
		return SYNC_END;
	unsigned char length[4], checksum[4];
	uint64_t record = current_record(f);
	sync_result_t r = read_bytes(f, length, 4, true, record);
	if (r == SYNC_END) {
		f->status.state = f->session ? FILE_SYNC_EOF_PENDING
			: FILE_SYNC_ENDED;
		return r;
	}
	if (r)
		return r;
	uint32_t size = le32_read(length);
	if (!size)
		return fail(f, SYNC_ERR_PROTOCOL, "zero record length", 0,
			record);
	if (size > f->status.max_frame_bytes)
		return fail(f, SYNC_ERR_LIMIT, "record exceeds file limit", 0,
			record);
	if (f->status.frames == UINT64_MAX ||
		size > UINT64_MAX - f->status.bytes)
		return fail(f, SYNC_ERR_LIMIT, "record counters exhausted", 0,
			record);
	if (size > reader->capacity) {
		unsigned char *buffer = allocate(size);
		if (!buffer)
			return fail(f, SYNC_ERR_NOMEM,
				"frame allocation failed", 0, record);
		free(reader->buffer);
		reader->buffer = buffer;
		reader->capacity = size;
	}
	r = read_bytes(f, reader->buffer, size, false, record);
	if (!r)
		r = read_bytes(f, checksum, 4, false, record);
	if (r)
		return r;
	uint32_t crc = crc_bytes(UINT32_MAX, length, 4);
	crc = crc_bytes(crc, reader->buffer, size) ^ UINT32_MAX;
	if (crc != le32_read(checksum))
		return fail(f, SYNC_ERR_PROTOCOL, "record checksum mismatch", 0,
			record);
	sync_frame_info_t info;
	r = sync_frame_inspect(reader->buffer, size, &info);
	if (r)
		return fail(f, r, "invalid Sync frame envelope", 0, record);
	r = validate_stream(f, &info);
	if (r)
		return r;
	reader->pending = (file_sync_frame_t){ .data = reader->buffer,
		.size = size, .info = info };
	f->status.pending_bytes = size;
	*out = reader->pending;
	return SYNC_OK;
}

static void reader_consume(file_sync_reader_t *r)
{
	r->base.status.frames++;
	r->base.status.bytes += r->pending.size;
	r->base.status.last_frame = r->pending.info;
	r->base.status.pending_bytes = 0;
	memset(&r->pending, 0, sizeof(r->pending));
}

static sync_result_t finish(file_sync_reader_t *r)
{
	if (r->base.status.state == FILE_SYNC_ENDED)
		return SYNC_END;
	sync_result_t result = sync_client_finish(r->base.session);
	if (result == SYNC_OK) {
		r->base.status.state = FILE_SYNC_ENDED;
		return SYNC_END;
	}
	if (result == SYNC_BACKPRESSURE)
		return result;
	return core_failure(&r->base, result);
}

static sync_result_t receive_frame(file_sync_reader_t *r,
	file_sync_progress_t *p)
{
	file_sync_frame_t v = {0};
	sync_result_t result = reader_peek(r, &v);
	if (result == SYNC_END)
		return finish(r);
	if (result)
		return result;
	result = sync_receive(r->base.session, v.data, v.size);
	if (result == SYNC_BACKPRESSURE)
		return result;
	if (result)
		return core_failure(&r->base, result);
	p->frames++;
	p->bytes += v.size;
	reader_consume(r);
	return SYNC_OK;
}

static sync_result_t reader_pump(file_sync_reader_t *r, uint32_t budget,
	file_sync_progress_t *out)
{
	file_sync_progress_t p = {0};
	sync_result_t result = SYNC_OK;
	for (uint32_t i = 0; i < budget; i++) {
		result = receive_frame(r, &p);
		if (result)
			break;
	}
	if (out)
		*out = p;
	return result;
}

static void writer_system(ecs_iter_t *it)
{
	file_sync_writer_t *w = it->ctx;
	if (w->base.status.state == FILE_SYNC_ACTIVE)
		writer_pump(w, w->base.status.frames_per_progress, NULL);
}

static void reader_system(ecs_iter_t *it)
{
	file_sync_reader_t *r = it->ctx;
	if (r->base.status.state != FILE_SYNC_FAULTED &&
		r->base.status.state != FILE_SYNC_ENDED)
		reader_pump(r, r->base.status.frames_per_progress, NULL);
}

static sync_result_t install_system(struct fs_file *f,
	ecs_iter_action_t callback, void *ctx, ecs_entity_t phase)
{
	if (f->status.schedule != SYNC_AUTOMATIC)
		return SYNC_OK;
	f->system = ecs_new(f->info.world);
	ecs_add_pair(f->info.world, f->system, EcsDependsOn, phase);
	if (!ecs_system_init(f->info.world, &(ecs_system_desc_t){
		.entity = f->system, .callback = callback,
		.ctx = ctx, .immediate = true }))
		return SYNC_ERR_RUNTIME;
	return SYNC_OK;
}

static void remove_system(struct fs_file *f)
{
	if (f->system) {
		ecs_delete(f->info.world, f->system);
		f->system = 0;
	}
}

static sync_result_t open_error(struct fs_file *f, sync_result_t result,
	file_sync_error_t *out)
{
	if (f->status.state != FILE_SYNC_FAULTED)
		fail(f, result, "file adapter setup failed", 0, 0);
	if (out)
		*out = f->status.fault;
	return result;
}

sync_result_t file_sync_writer_open(const char *path,
	const file_sync_writer_desc_t *desc, file_sync_writer_t **out_writer,
	file_sync_error_t *out_error)
{
	/* Implement the documented file adapter operation. */
	if (out_writer)
		*out_writer = NULL;
	if (out_error)
		memset(out_error, 0, sizeof(*out_error));
	struct fs_file setup = {0};
	if (!path || !*path || !out_writer)
		return open_error(&setup, SYNC_ERR_ARGUMENT, out_error);
	file_sync_writer_desc_t d = desc ? *desc : (file_sync_writer_desc_t){0};
	sync_result_t result = configure(&setup, d.server, SYNC_ROLE_SERVER,
		d.schedule, d.frames_per_progress, d.max_frame_bytes);
	if (result)
		return open_error(&setup, result, out_error);
	file_sync_writer_t *w = allocate(sizeof(*w));
	if (!w)
		return open_error(&setup, SYNC_ERR_NOMEM, out_error);
	w->base = setup;
	struct fs_file *f = &w->base;
	if (f->session)
		result = sync_peer_open(f->session, &f->status.peer);
	if (!result) {
		errno = 0;
		f->file = fopen(path, "wb");
		if (!f->file)
			result = fail(f, SYNC_ERR_IO, "cannot open output file",
				errno, 0);
	}
	if (!result)
		result = write_bytes(f, file_header, sizeof(file_header), 0);
	if (!result)
		result = install_system(f, writer_system, w,
			f->info.ids.send_phase);
	if (result) {
		open_error(f, result, out_error);
		remove_system(f);
		close_file(f);
		if (f->status.peer)
			sync_peer_close(f->session, f->status.peer);
		free(w);
		return result;
	}
	*out_writer = w;
	return SYNC_OK;
}

sync_result_t file_sync_writer_write(file_sync_writer_t *writer,
	const void *data, size_t size)
{
	/* Implement the documented file adapter operation. */
	if (!writer || !data || !size)
		return SYNC_ERR_ARGUMENT;
	if (writer->base.status.state != FILE_SYNC_ACTIVE ||
		writer->base.session ||
		writer->base.status.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	return write_frame(writer, data, size);
}

sync_result_t file_sync_writer_pump(file_sync_writer_t *writer,
	uint32_t max_frames, file_sync_progress_t *out_progress)
{
	/* Implement the documented file adapter operation. */
	if (out_progress)
		memset(out_progress, 0, sizeof(*out_progress));
	if (!writer || !max_frames)
		return SYNC_ERR_ARGUMENT;
	if (!writer->base.session ||
		writer->base.status.state != FILE_SYNC_ACTIVE ||
		writer->base.status.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	return writer_pump(writer, max_frames, out_progress);
}

sync_result_t file_sync_writer_flush(file_sync_writer_t *writer)
{
	/* Implement the documented file adapter operation. */
	if (!writer)
		return SYNC_ERR_ARGUMENT;
	if (writer->base.status.state != FILE_SYNC_ACTIVE)
		return SYNC_ERR_STATE;
	return flush(&writer->base);
}

sync_result_t file_sync_writer_get_status(const file_sync_writer_t *writer,
	file_sync_status_t *out_status)
{
	/* Implement the documented file adapter operation. */
	if (out_status)
		memset(out_status, 0, sizeof(*out_status));
	if (!writer || !out_status)
		return SYNC_ERR_ARGUMENT;
	*out_status = writer->base.status;
	return SYNC_OK;
}

sync_result_t file_sync_writer_close(file_sync_writer_t *writer,
	file_sync_error_t *out_error)
{
	/* Implement the documented file adapter operation. */
	if (out_error)
		memset(out_error, 0, sizeof(*out_error));
	if (!writer)
		return SYNC_OK;
	struct fs_file *f = &writer->base;
	remove_system(f);
	if (f->session && f->status.state == FILE_SYNC_ACTIVE) {
		while (writer_pump(writer, UINT32_MAX, NULL) == SYNC_OK)
			;
	}
	if (f->status.state == FILE_SYNC_ACTIVE)
		flush(f);
	close_file(f);
	if (f->status.peer) {
		sync_result_t r = sync_peer_close(f->session, f->status.peer);
		if (r)
			core_failure(f, r);
	}
	sync_result_t result = f->status.fault.detail.code;
	if (out_error)
		*out_error = f->status.fault;
	free(writer);
	return result;
}

sync_result_t file_sync_reader_open(const char *path,
	const file_sync_reader_desc_t *desc, file_sync_reader_t **out_reader,
	file_sync_error_t *out_error)
{
	/* Implement the documented file adapter operation. */
	if (out_reader)
		*out_reader = NULL;
	if (out_error)
		memset(out_error, 0, sizeof(*out_error));
	struct fs_file setup = {0};
	if (!path || !*path || !out_reader)
		return open_error(&setup, SYNC_ERR_ARGUMENT, out_error);
	file_sync_reader_desc_t d = desc ? *desc : (file_sync_reader_desc_t){0};
	sync_result_t result = configure(&setup, d.client, SYNC_ROLE_CLIENT,
		d.schedule, d.frames_per_progress, d.max_frame_bytes);
	if (result)
		return open_error(&setup, result, out_error);
	file_sync_reader_t *r = allocate(sizeof(*r));
	if (!r)
		return open_error(&setup, SYNC_ERR_NOMEM, out_error);
	r->base = setup;
	struct fs_file *f = &r->base;
	errno = 0;
	f->file = fopen(path, "rb");
	if (!f->file)
		result = fail(f, SYNC_ERR_IO, "cannot open input file",
			errno, 0);
	unsigned char header[16];
	if (!result)
		result = read_bytes(f, header, sizeof(header), false, 0);
	if (!result &&
		(memcmp(header, file_header, 8) || le32_read(header + 12)))
		result = fail(f, SYNC_ERR_PROTOCOL, "invalid file header",
			0, 0);
	if (!result && le32_read(header + 8) != 1)
		result = fail(f, SYNC_ERR_UNSUPPORTED,
			"unsupported file version", 0, 0);
	if (!result)
		result = install_system(f, reader_system, r,
			f->info.ids.receive_phase);
	if (result) {
		open_error(f, result, out_error);
		remove_system(f);
		close_file(f);
		free(r);
		return result;
	}
	*out_reader = r;
	return SYNC_OK;
}

sync_result_t file_sync_reader_peek(file_sync_reader_t *reader,
	file_sync_frame_t *out_frame)
{
	/* Implement the documented file adapter operation. */
	if (out_frame)
		memset(out_frame, 0, sizeof(*out_frame));
	if (!reader || !out_frame)
		return SYNC_ERR_ARGUMENT;
	if (reader->base.session ||
		reader->base.status.state == FILE_SYNC_FAULTED ||
		reader->base.status.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	return reader_peek(reader, out_frame);
}

sync_result_t file_sync_reader_consume(file_sync_reader_t *reader)
{
	/* Implement the documented file adapter operation. */
	if (!reader)
		return SYNC_ERR_ARGUMENT;
	if (reader->base.session ||
		reader->base.status.state == FILE_SYNC_FAULTED ||
		reader->base.status.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	if (!reader->pending.data)
		return SYNC_EMPTY;
	reader_consume(reader);
	return SYNC_OK;
}

sync_result_t file_sync_reader_pump(file_sync_reader_t *reader,
	uint32_t max_frames, file_sync_progress_t *out_progress)
{
	/* Implement the documented file adapter operation. */
	if (out_progress)
		memset(out_progress, 0, sizeof(*out_progress));
	if (!reader || !max_frames)
		return SYNC_ERR_ARGUMENT;
	if (!reader->base.session ||
		reader->base.status.state == FILE_SYNC_FAULTED ||
		reader->base.status.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	return reader_pump(reader, max_frames, out_progress);
}

sync_result_t file_sync_reader_step(file_sync_reader_t *reader,
	uint32_t max_updates, file_sync_progress_t *out_progress)
{
	/* Implement the documented file adapter operation. */
	if (out_progress)
		memset(out_progress, 0, sizeof(*out_progress));
	if (!reader || !max_updates)
		return SYNC_ERR_ARGUMENT;
	struct fs_file *f = &reader->base;
	if (!f->session || f->status.state == FILE_SYNC_FAULTED ||
		f->status.schedule != SYNC_MANUAL ||
		f->info.schedule != SYNC_MANUAL)
		return SYNC_ERR_STATE;
	file_sync_progress_t p = {0};
	sync_result_t result = SYNC_OK;
	while (p.application.updates_applied < max_updates) {
		sync_apply_result_t a;
		result = sync_apply(f->session,
			max_updates - p.application.updates_applied, &a);
		p.application.updates_applied += a.updates_applied;
		p.application.frames_consumed += a.frames_consumed;
		if (a.last_tick)
			p.application.last_tick = a.last_tick;
		if (result < 0) {
			result = core_failure(f, result);
			break;
		}
		if (result == SYNC_END) {
			f->status.state = FILE_SYNC_ENDED;
			break;
		}
		if (p.application.updates_applied == max_updates) {
			result = SYNC_OK;
			break;
		}
		result = receive_frame(reader, &p);
		if (result != SYNC_OK)
			break;
	}
	if (out_progress)
		*out_progress = p;
	return result;
}

sync_result_t file_sync_reader_get_status(const file_sync_reader_t *reader,
	file_sync_status_t *out_status)
{
	/* Implement the documented file adapter operation. */
	if (out_status)
		memset(out_status, 0, sizeof(*out_status));
	if (!reader || !out_status)
		return SYNC_ERR_ARGUMENT;
	*out_status = reader->base.status;
	return SYNC_OK;
}

sync_result_t file_sync_reader_close(file_sync_reader_t *reader,
	file_sync_error_t *out_error)
{
	/* Implement the documented file adapter operation. */
	if (out_error)
		memset(out_error, 0, sizeof(*out_error));
	if (!reader)
		return SYNC_OK;
	struct fs_file *f = &reader->base;
	remove_system(f);
	close_file(f);
	sync_result_t result = f->status.fault.detail.code;
	if (out_error)
		*out_error = f->status.fault;
	free(reader->buffer);
	free(reader);
	return result;
}
