/**
 * @file file_sync.h
 * @brief Sequential recording and replay of unchanged Sync frames.
 *
 * C11 API; includable from C++17. FileSync opens binary files by path and owns
 * them. All operations are nonreentrant and use the owning thread. A bound
 * handle borrows its Sync session/world: close FileSync before destroying
 * that session or world. Sessions/peers must not be reset/closed externally
 * while bound. Opening/closing bound handles and stepping updates require a
 * writable world boundary under Sync's common contract. No call captures.
 *
 * Input/output objects must be valid and disjoint. Non-NULL outputs are
 * zeroed before fallible work. Descriptors are copied; paths are used only
 * during open. NULL and invalid scalar arguments return ARGUMENT. Dangling
 * handles or violated Flecs invariants are caller bugs. errno is unspecified;
 * I/O diagnostics preserve its value when available.
 *
 * Each file contains one stream: FILESYNC magic, LE32 container version 1,
 * LE32 reserved zero; records contain LE32 length, original complete frame,
 * LE32 IEEE CRC-32 of length plus frame (reflected polynomial 0xedb88320,
 * initial/final XOR 0xffffffff). The container and Sync protocol versions are
 * independent. No append, seek, compression, or playback clock is provided.
 * FileSync checks envelopes, identity and contiguous sequences; only Sync
 * validates schemas, logical updates and reference closure.
 *
 * Negative data/I/O/allocation/core failures are sticky on live handles;
 * subsequent transfers return STATE. Detectable ARGUMENT/ROLE/STATE control
 * errors leave healthy handles unchanged. Earlier progress is preserved on
 * later errors. Inspect FileSync status separately from Sync session status.
 * File errors never fabricate a core fault or roll back published replicas.
 */
#ifndef FILE_SYNC_H
#define FILE_SYNC_H
#include <sync.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Owned writer; bound writers additionally own a fresh Sync peer. */
typedef struct file_sync_writer file_sync_writer_t;
/** Owned reader; never owns the optional client or its replicas. */
typedef struct file_sync_reader file_sync_reader_t;

/** Observable adapter states; no pointer survives close. */
typedef enum file_sync_state {
	FILE_SYNC_ACTIVE = 1, /**< May transfer data. */
	FILE_SYNC_EOF_PENDING = 2, /**< Physical EOF; client needs drainage. */
	FILE_SYNC_ENDED = 3, /**< Raw EOF or clean client finish. */
	FILE_SYNC_FAULTED = 4 /**< Inspect fault, then close. */
} file_sync_state_t;

/** Bounded diagnostic; zero denotes success/no fault. */
typedef struct file_sync_error {
	/** Code/context/message, including core faults. */
	sync_error_t detail;
	uint64_t offset; /**< Byte offset of failure in the container. */
	uint64_t record; /**< One-based attempted record; zero for header. */
	int system_error; /**< Captured OS error, or zero when unavailable. */
} file_sync_error_t;

/** Optional copied writer configuration; zero/NULL means raw manual I/O. */
typedef struct file_sync_writer_desc {
	sync_session_t *server; /**< Optional borrowed active server. */
	sync_schedule_t schedule; /**< MANUAL=0, AUTOMATIC=1; immutable. */
	uint32_t frames_per_progress; /**< Auto: zero means 64; manual: zero. */
	/** Zero inherits server limit, or selects 65536 for raw I/O.
	 * Explicit values are positive and cannot exceed the server limit.
	 */
	uint32_t max_frame_bytes;
} file_sync_writer_desc_t;

/** Optional copied reader configuration; zero/NULL means raw manual I/O. */
typedef struct file_sync_reader_desc {
	sync_session_t *client; /**< Optional borrowed fresh client. */
	sync_schedule_t schedule; /**< MANUAL=0, AUTOMATIC=1; immutable. */
	uint32_t frames_per_progress; /**< Auto: zero means 64; manual: zero. */
	/** Zero inherits client limit, or selects 65536 for raw I/O.
	 * Explicit values are positive and cannot exceed the client limit.
	 */
	uint32_t max_frame_bytes;
} file_sync_reader_desc_t;

/** Borrowed verified frame, immutable until consume/close. No commit token. */
typedef struct file_sync_frame {
	const void *data; /**< Owned by reader; non-NULL on OK. */
	size_t size; /**< Complete frame bytes, excluding file framing. */
	/** Decoded envelope, not payload validation. */
	sync_frame_info_t info;
} file_sync_frame_t;

/** Transfer progress, valid on success, backpressure, EOF and errors. */
typedef struct file_sync_progress {
	uint64_t frames; /**< Frames written/accepted by client this call. */
	/** Complete Sync frame bytes, excluding file framing. */
	uint64_t bytes;
	/** Step-only client publication progress. */
	sync_apply_result_t application;
} file_sync_progress_t;

/** Side-effect-free copied adapter status; contains no frame pointers. */
typedef struct file_sync_status {
	file_sync_state_t state; /**< Active, EOF pending, ended or faulted. */
	sync_schedule_t schedule; /**< Fixed adapter schedule. */
	/** Writer-owned peer; zero for raw writers/readers. */
	ecs_entity_t peer;
	uint32_t max_frame_bytes; /**< Effective finite frame bound. */
	uint32_t frames_per_progress; /**< Automatic budget; zero for manual. */
	/** Written/consumed file records, excluding header. */
	uint64_t frames;
	uint64_t bytes; /**< Written/consumed Sync frame bytes. */
	/** File bytes written/read, including pending record. */
	uint64_t offset;
	/** Verified reader frame awaiting consumption. */
	size_t pending_bytes;
	sync_frame_info_t last_frame; /**< Last written/consumed envelope. */
	file_sync_error_t fault; /**< Sticky first failure, or zero. */
} file_sync_status_t;

/**
 * Open path for replacement (binary write); existing contents are truncated.
 * @param[in] path Required nonempty filename; no '-' or special-path syntax.
 * @param[in] desc Optional copied options.
 * @param[out] out_writer Required; owned handle on OK, NULL on failure.
 * @param[out] out_error Optional copied diagnostic, zero on OK.
 * @return OK, ARGUMENT, ROLE, STATE, LIMIT, NOMEM, IO or core setup error.
 * With server, create a fresh pending peer exposed by writer status. Set its
 * policy before capture. Without server, write supplied frames manually.
 * AUTOMATIC requires server and installs a system in sync.Send. Its server
 * may use either scheduling mode. Failure leaves no owned peer/system; a
 * created/truncated file may remain after an I/O/setup failure.
 */
sync_result_t file_sync_writer_open(const char *path,
	const file_sync_writer_desc_t *desc, file_sync_writer_t **out_writer,
	file_sync_error_t *out_error);

/**
 * Write one complete frame to a raw manual writer; copies/retains no input.
 * @param[in,out] writer Raw manual writer.
 * @param[in] data Required valid buffer for this call only.
 * @param[in] size Positive frame size, at most the configured bound.
 * @return OK, ARGUMENT, STATE, LIMIT, PROTOCOL, SEQUENCE, UNSUPPORTED or IO.
 * First frame must be manifest/sequence one; subsequent frames must match
 * identity/stream and exact next sequence. OK means buffered write completion,
 * not flush/durability. A partial record faults; never retry its beginning.
 */
sync_result_t file_sync_writer_write(file_sync_writer_t *writer,
	const void *data, size_t size);

/**
 * Write and commit up to max_frames from a bound manual writer's peer.
 * @param[in,out] writer Bound manual writer; sole consumer of its outbox.
 * @param[in] max_frames Positive record budget, not a time/byte budget.
 * @param[out] out_progress Optional actual progress, also on errors.
 * @return OK if frames committed, EMPTY if none, STATE/ARGUMENT or a fault.
 * Commit only after the complete record is written. Never captures or flushes.
 */
sync_result_t file_sync_writer_pump(file_sync_writer_t *writer,
	uint32_t max_frames, file_sync_progress_t *out_progress);

/**
 * Flush previously written bytes using checked fflush; no outbox drainage.
 * @param[in,out] writer Live healthy writer in either schedule.
 * @return OK, ARGUMENT, STATE or IO. No power-loss durability guarantee.
 */
sync_result_t file_sync_writer_flush(file_sync_writer_t *writer);

/**
 * Copy writer status; required output is zero on ARGUMENT.
 * @param[in] writer Live writer, including faulted.
 * @param[out] out_status Required copied status.
 * @return OK or ARGUMENT. Does not consume progress/faults or allocate.
 */
sync_result_t file_sync_writer_get_status(const file_sync_writer_t *writer,
	file_sync_status_t *out_status);

/**
 * Release writer and all borrowed views; NULL is a successful no-op.
 * @param[in,out] writer Handle to release, or NULL; invalid on EVERY return.
 * @param[out] out_error Optional first diagnostic; zero on successful close.
 * @return OK, sticky earlier failure, drainage error, or IO.
 * Stop the system, drain all queued frames if healthy/bound, flush/close,
 * close the owned peer, then free. Never capture. Cleanup continues on error.
 * Peer recipient pairs disappear; gameplay and the borrowed session survive.
 * Caller must discard the handle even when close reports an error.
 */
sync_result_t file_sync_writer_close(file_sync_writer_t *writer,
	file_sync_error_t *out_error);

/**
 * Open path in binary read mode and validate its complete container header.
 * @param[in] path Required nonempty filename; no special-path interpretation.
 * @param[in] desc Optional copied options.
 * @param[out] out_reader Required; owned handle on OK, NULL on failure.
 * @param[out] out_error Optional diagnostic, zero on OK.
 * @return OK, ARGUMENT, ROLE, STATE, NOMEM, IO, PROTOCOL, UNSUPPORTED or
 * RUNTIME.
 * Incomplete header returns TRUNCATED. Bound clients must be WAIT_MANIFEST
 * with no queued input/assembly. AUTOMATIC requires an automatic client and
 * installs a system in sync.Receive. Open never enqueues or publishes frames.
 * Failure leaves the client unchanged and no adapter system/handle.
 */
sync_result_t file_sync_reader_open(const char *path,
	const file_sync_reader_desc_t *desc, file_sync_reader_t **out_reader,
	file_sync_error_t *out_error);

/**
 * Borrow the next verified record from a raw manual reader without consuming.
 * @param[in,out] reader Raw manual reader.
 * @param[out] out_frame Required; zero on END/error.
 * @return OK, END at physical EOF, ARGUMENT/STATE, or a sticky data/I/O fault.
 * Repeated peeks preserve bytes/pointer until consume/close. Checks length
 * before allocation, then CRC/envelope/identity/sequence. No ECS interaction.
 * END alone does not establish logical completeness for a replay.
 */
sync_result_t file_sync_reader_peek(file_sync_reader_t *reader,
	file_sync_frame_t *out_frame);

/**
 * Consume the pending raw manual reader frame; invalidates its views.
 * @param[in,out] reader Raw manual reader.
 * @return OK, EMPTY without a pending record, ARGUMENT or STATE.
 * Reads no bytes and publishes nothing.
 */
sync_result_t file_sync_reader_consume(file_sync_reader_t *reader);

/**
 * Feed up to max_frames into a bound client's queue without applying updates.
 * @param[in,out] reader Bound manual reader; sole incoming stream producer.
 * @param[in] max_frames Positive frame budget.
 * @param[out] out_progress Optional actual transfer progress on every return.
 * @return OK, BACKPRESSURE, END, ARGUMENT/STATE, or a sticky data/core fault.
 * Accept only complete verified frames. Backpressure retains the exact pending
 * record; no later record is read. At EOF call sync_client_finish; retry EOF
 * after draining input on BACKPRESSURE. The client may use either schedule.
 */
sync_result_t file_sync_reader_pump(file_sync_reader_t *reader,
	uint32_t max_frames, file_sync_progress_t *out_progress);

/**
 * Publish up to max_updates through a bound manual reader AND manual client.
 * @param[in,out] reader Bound manual reader at a writable world boundary.
 * @param[in] max_updates Positive completed-update budget.
 * @param[out] out_progress Optional progress, including earlier publications.
 * @return OK if budget published, END at clean EOF, ARGUMENT/STATE or fault.
 * Apply queued input first; then alternate one-frame receive and application.
 * Snapshots count as one update. Invalid modes reject before I/O/publication.
 * Runtime failures do not roll back earlier or partially mutated core state.
 */
sync_result_t file_sync_reader_step(file_sync_reader_t *reader,
	uint32_t max_updates, file_sync_progress_t *out_progress);

/**
 * Copy reader status, including pending bytes and sticky errors.
 * @param[in] reader Live reader, including ended/faulted.
 * @param[out] out_status Required; zero on ARGUMENT.
 * @return OK or ARGUMENT. Does not consume, allocate or publish.
 */
sync_result_t file_sync_reader_get_status(const file_sync_reader_t *reader,
	file_sync_status_t *out_status);

/**
 * Stop automatic input, close file and release reader; NULL is a no-op.
 * @param[in,out] reader Invalid on EVERY return, including error.
 * @param[out] out_error Optional first diagnostic, zero on success.
 * @return OK, sticky earlier failure, or IO.
 * Discard pending input. Never finish/reset/destroy the borrowed client or
 * delete its replicas. Caller must reset before attaching another stream.
 */
sync_result_t file_sync_reader_close(file_sync_reader_t *reader,
	file_sync_error_t *out_error);

#ifdef __cplusplus
}
#endif
#endif
