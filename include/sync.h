/**
 * @file sync.h
 * @brief Proposed C API for transport-independent Flecs replication.
 *
 * Design only: this header declares no production function bodies.
 * Syntax baseline: Flecs 4.1.1, C11; also includable from C++17.
 *
 * COMMON CONTRACT
 * - One session per world. A session borrows the real world, never a stage.
 * - All calls use the world's owning thread, are nonreentrant, and require
 *   live handles. Except frame inspection, calls must not overlap on a world.
 * - Mutators require a writable boundary: no active iterators, readonly mode,
 *   outstanding deferred commands, or concurrent world access. receive,
 *   peek, commit, finish, rate and getters do not mutate ECS storage, but
 *   still require the owning thread and no concurrent/reentrant Sync call.
 * - Observers/hooks invoked by Sync must not call Sync, delete peers/replicas,
 *   mutate replication metadata, or throw across this C ABI. They may enqueue
 *   application work for a later boundary. Local entities must not depend on
 *   replicas through cascading-delete policies (for example ChildOf).
 * - Required pointer arguments are non-NULL. Caller-provided input and output
 *   objects are valid for the call and mutually disjoint, including buffers.
 *   NULL/invalid scalar arguments return SYNC_ERR_ARGUMENT when detectable;
 *   dangling pointers/handles and violated Flecs invariants are caller bugs.
 * - Every non-NULL output is initialized to zero before fallible work unless
 *   explicitly stated otherwise. Arguments/configuration are copied, not
 *   retained, except the borrowed world. No function sets errno as an API.
 * - Returning an error does not imply rollback of Flecs observer effects.
 *   Flecs fatal errors/abort-on-OOM are not converted to recoverable errors.
 * - Public enums are host API values, never wire encodings. Public structs
 *   are source-level contracts, not serialized layouts or a stable ABI.
 * - Every enum input must be a declared value. Reserved/unknown values fail.
 *
 * ECS SEMANTICS
 * Mark component/tag definitions with ids.sync before session creation.
 * Mark runtime entities with ids.replicate_to_all or one or more direct
 * (ids.replicate_to, peer) pairs. No policy means no recipient. Only directly
 * owned registered components/tags participate. Prefabs, disabled entities,
 * peer entities, arbitrary gameplay pairs and inherited values are excluded.
 * NetworkId and all policy markers are excluded from replicated payloads.
 *
 * Types and reflection are frozen for the session lifetime. Clients require
 * a compatible, locally marked type for every manifest type; extra local
 * types are permitted. Type names, shapes and enum values establish wire
 * compatibility; local IDs, offsets and registration order do not.
 *
 * Values are sampled, not events. Writers must use Flecs tracked operations
 * or report ecs_modified_id. A dirty table can resend unchanged row values.
 * Non-null entity references must target the recipient's resulting visible
 * set. A violation faults only that recipient; it never exposes the target.
 *
 * Ordinary scalars, enum/bitmask values, reflected structs, fixed arrays,
 * strings and entity references are supported. Native lifecycle operations
 * must correctly initialize, copy/move and destroy nontrivial values.
 * Unsupported opaque/dynamic containers and unmanaged pointer fields fail
 * schema validation; no component-specific network callbacks are required.
 * Fixed-width integers and IEEE-754 binary32/binary64 values are supported;
 * intptr_t/uintptr_t, size_t and opaque resources are not wire scalars.
 * Strings are NUL-terminated byte strings (no embedded NUL), not required to
 * be UTF-8; NULL and empty differ. Enum values use the declared fixed-width
 * underlying integer; unnamed representable values/bits are preserved.
 */
#ifndef FLECS_SYNC_H
#define FLECS_SYNC_H

#include <flecs.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque per-world session; destroy before its borrowed world. */
typedef struct sync_session sync_session_t;

/** Results: nonnegative values are specified outcomes, negatives errors. */
typedef enum sync_result {
	SYNC_OK = 0, /**< Requested work succeeded. */
	SYNC_EMPTY = 1, /**< No frame/input was available; no progress. */
	/** Input processed; more frames needed to publish. */
	SYNC_NEED_MORE = 2,
	/** Frame not accepted; retry unchanged later. */
	SYNC_BACKPRESSURE = 3,
	/** A previously finished client stream is fully drained. */
	SYNC_END = 4,
	/** Capture completed, but one or more peers faulted. */
	SYNC_PARTIAL = 5,
	SYNC_ERR_ARGUMENT = -1, /**< Invalid pointer, number, token or enum. */
	SYNC_ERR_ROLE = -2, /**< Server/client operation used on wrong role. */
	SYNC_ERR_STATE = -3, /**< Operation forbidden in the current state. */
	SYNC_ERR_SCHEMA = -4, /**< Missing, changed or incompatible schema. */
	/** Unsupported type or platform facility. */
	SYNC_ERR_UNSUPPORTED = -5,
	/** Malformed frame, payload or operation set. */
	SYNC_ERR_PROTOCOL = -6,
	SYNC_ERR_SEQUENCE = -7, /**< Wrong stream, tick or frame sequence. */
	SYNC_ERR_REFERENCE = -8, /**< Missing or invisible reference target. */
	SYNC_ERR_LIMIT = -9, /**< Configured bound or ID space exhausted. */
	SYNC_ERR_NOMEM = -10, /**< Recoverable allocation failure. */
	/** EOF before complete initial/update state. */
	SYNC_ERR_TRUNCATED = -11,
	/** Detected failure during ECS publication. */
	SYNC_ERR_RUNTIME = -12,
	SYNC_ERR_NOT_FOUND = -13, /**< Unknown peer or network identity. */
	SYNC_ERR_IO = -14, /**< Reserved for a delivery adapter's I/O error. */
	SYNC_ERR_TRANSPORT = -15 /**< Reserved for a delivery adapter error. */
} sync_result_t;

/** Explicit session role; assigned by the selected create function. */
typedef enum sync_role {
	SYNC_ROLE_SERVER = 1, /**< Authoritative capture and outgoing peers. */
	SYNC_ROLE_CLIENT = 2 /**< One incoming authoritative stream. */
} sync_role_t;

/** Fixed for a session lifetime; zero-initialized descriptors are manual. */
typedef enum sync_schedule {
	SYNC_MANUAL = 0, /**< Application explicitly captures/applies. */
	SYNC_AUTOMATIC = 1 /**< Core Flecs systems capture/apply. */
} sync_schedule_t;

/** Session or peer state; READY never means remote delivery acknowledged. */
typedef enum sync_state {
	SYNC_SERVER_READY = 1, /**< Can open peers and capture. */
	SYNC_PEER_PENDING = 2, /**< Waiting for the next capture's snapshot. */
	/** Snapshot queued; subsequent captures delta. */
	SYNC_PEER_READY = 3,
	SYNC_CLIENT_WAIT_MANIFEST = 4, /**< Fresh/reset client. */
	SYNC_CLIENT_WAIT_SNAPSHOT = 5, /**< Manifest accepted by apply. */
	SYNC_CLIENT_READY = 6, /**< Initial snapshot published. */
	/** Clean EOF; last replicas remain readable. */
	SYNC_CLIENT_ENDED = 7,
	SYNC_FAULTED = 8 /**< Close/reset/recreate before progressing. */
} sync_state_t;

/** A 128-bit identity, compared as 16 bytes; all-zero is unassigned. */
typedef struct sync_session_id {
	/** Opaque identity, not a native integer/UUID ABI. */
	uint8_t bytes[16];
} sync_session_id_t;

/** ECS storage for sync.NetworkId. Read-only to applications. */
typedef struct sync_network_id {
	/** 1..UINT64_MAX; zero only denotes a null reference. */
	uint64_t value;
} sync_network_id_t;

/** World-local declarations. Never reuse these IDs in another world. */
typedef struct sync_ids {
	ecs_entity_t module; /**< Module namespace, canonical path "sync". */
	/** Tag at sync.Sync; placed on type definitions. */
	ecs_entity_t sync;
	/** sync.NetworkId; sync_network_id_t data. */
	ecs_entity_t network_id;
	/** sync.ReplicateTo; nonexclusive relation. */
	ecs_entity_t replicate_to;
	ecs_entity_t replicate_to_all; /**< sync.ReplicateToAll runtime tag. */
	/** sync.Receive; after OnLoad, before Apply/PreUpdate. */
	ecs_entity_t receive_phase;
	ecs_entity_t apply_phase; /**< sync.Apply; publication before EcsPreUpdate. */
	ecs_entity_t capture_phase; /**< sync.Capture; core server sampling. */
	ecs_entity_t send_phase; /**< sync.Send; adapters drain outboxes. */
} sync_ids_t;

/** Bounded diagnostic copy. Zero context fields mean unavailable. */
typedef struct sync_error {
	sync_result_t code; /**< Fault result, or SYNC_OK for no fault. */
	ecs_entity_t peer; /**< Server peer entity; zero for client/session. */
	/** Local affected component definition, if known. */
	ecs_entity_t type;
	uint64_t tick; /**< Positive attempted/published capture tick. */
	uint64_t sequence; /**< Positive frame sequence, when known. */
	uint64_t network_id; /**< Affected network identity, if known. */
	char field[128]; /**< NUL-terminated member path; may be truncated. */
	/** NUL-terminated human detail, not for parsing. */
	char message[192];
} sync_error_t;

/**
 * Configuration limits. Zero selects the stated default, never unlimited.
 * uint32_t fields otherwise accept 1..UINT32_MAX; size_t fields accept
 * 1..SIZE_MAX, subject to checked platform representability. Bounds apply
 * before allocation. Inconsistent combinations fail creation with ARGUMENT.
 * Crossing a bound at runtime returns LIMIT or faults an affected stream.
 * These are maxima, not reservations; default totals need not fit at once.
 * max_frame_bytes must fit one legal envelope. max_schema_bytes must fit
 * the manifest payload capacity of max_frame_bytes: v1 has one manifest
 * frame and no schema fragmentation. max_pending_bytes (server) and
 * max_receive_bytes (client) must each accommodate max_frame_bytes.
 * max_memory_bytes counts allocated core buffers, plans, indexes and owned
 * temporary values, including capacity and accounting overhead. Shared
 * blocks count once. Flecs/application storage and transport queues are not
 * included. A frame can be drained into separate bounded assembly storage;
 * max_update_bytes need not fit max_receive_bytes. Snapshot totals, including
 * introductions/values, obey max_update_bytes just as deltas do.
 */
typedef struct sync_limits {
	uint32_t max_types; /**< Top-level synchronized types; default 1024. */
	uint32_t max_schema_bytes; /**< Manifest payload; default 32768. */
	uint32_t max_frame_bytes; /**< Entire frame; default 65536. */
	/** Logical update payload; default 16 MiB. */
	uint32_t max_update_bytes;
	/** Live mapped entities per session; 100000. */
	uint32_t max_entities;
	uint32_t max_peers; /**< Server peers, including faulted peers; 64. */
	/** Bytes excluding NUL per string; 65535. */
	uint32_t max_string_bytes;
	/** Elements in each reflected fixed array; 4096. */
	uint32_t max_elements;
	uint32_t max_depth; /**< Nested reflected types; default 32. */
	/** Frames in one snapshot/delta; default 8192. */
	uint32_t max_fragments;
	size_t max_pending_bytes; /**< Outbox bytes per server peer; 32 MiB. */
	size_t max_receive_bytes; /**< Queued incoming frame bytes; 32 MiB. */
	size_t max_memory_bytes; /**< All core-owned allocations; 256 MiB. */
} sync_limits_t;

/** Server creation options. NULL or an all-zero object means manual mode. */
typedef struct sync_server_desc {
	sync_schedule_t schedule; /**< Manual or automatic; immutable. */
	double send_hz; /**< Automatic: 0 means 20; else finite and >0. */
	sync_session_id_t session_id; /**< Zero generates a fresh identity. */
	sync_limits_t limits; /**< Bounds, normalized as documented above. */
} sync_server_desc_t;

/** Client creation options. NULL or an all-zero object means manual mode. */
typedef struct sync_client_desc {
	sync_schedule_t schedule; /**< Manual or automatic; immutable. */
	/** Automatic apply budget; 0 means 1. */
	uint32_t updates_per_progress;
	sync_limits_t limits; /**< Bounds; server-only fields are ignored. */
} sync_client_desc_t;

/** Public frame kinds. These values do not specify wire discriminants. */
typedef enum sync_frame_kind {
	SYNC_FRAME_MANIFEST = 1, /**< Schema for a new stream. */
	SYNC_FRAME_SNAPSHOT_BEGIN = 2, /**< Begin a replacement snapshot. */
	SYNC_FRAME_SNAPSHOT_DATA = 3, /**< One bounded snapshot data frame. */
	SYNC_FRAME_SNAPSHOT_END = 4, /**< Complete initial snapshot. */
	SYNC_FRAME_DELTA = 5, /**< Complete unfragmented logical delta. */
	SYNC_FRAME_FRAGMENT = 6 /**< Fragment of one logical delta. */
} sync_frame_kind_t;

/** Decoded envelope only; not evidence of valid schema/payload/sequence. */
typedef struct sync_frame_info {
	/** Protocol revision encoded by the core. */
	uint32_t protocol_version;
	sync_frame_kind_t kind; /**< Recognized frame kind. */
	sync_session_id_t session_id; /**< Nonzero server identity. */
	/** Positive stream ID, unique within the session. */
	uint64_t stream_id;
	uint64_t sequence; /**< Starts at 1; increases for every frame. */
	uint64_t tick; /**< Positive capture tick; equal within one update. */
	/** Exact payload length excluding envelope. */
	uint32_t payload_bytes;
} sync_frame_info_t;

/** Copyable capability for one peer's current outbox head. */
typedef struct sync_outbox_token {
	/** Opaque; only copy returned tokens unchanged. */
	uint64_t opaque[4];
} sync_outbox_token_t;

/** Immutable borrowed outbox frame; no ownership transfer to the caller. */
typedef struct sync_frame_view {
	const void *data; /**< Exact encoded bytes; non-NULL on OK. */
	/** Nonzero complete frame size, including its envelope. */
	size_t size;
	/** Commit capability for this exact head. */
	sync_outbox_token_t token;
	sync_frame_info_t info; /**< Envelope for adapters and diagnostics. */
} sync_frame_view_t;

/** Capture progress, valid even when capture returns PARTIAL or an error. */
typedef struct sync_capture_result {
	/** Consumed tick, or zero if rejected before sampling. */
	uint64_t tick;
	uint32_t peers_queued; /**< Peers retaining a complete update. */
	uint32_t peers_faulted; /**< Peers newly faulted by this capture. */
} sync_capture_result_t;

/** Application progress; earlier published updates survive a later error. */
typedef struct sync_apply_result {
	uint32_t updates_applied; /**< Complete snapshots/deltas published. */
	/** Frames dequeued, including schema/chunks. */
	uint64_t frames_consumed;
	/** Last tick published this call; zero if none. */
	uint64_t last_tick;
} sync_apply_result_t;

/** Side-effect-free copied session/peer status. No internal pointers. */
typedef struct sync_status {
	sync_role_t role; /**< Session role. */
	sync_state_t state; /**< Selected session or peer's state. */
	sync_session_id_t session_id; /**< Zero until a client's manifest. */
	uint64_t stream_id; /**< Zero for server aggregate/unbound client. */
	uint64_t last_tick; /**< Last queued/published tick, zero if none. */
	/** Last queued/processed sequence, zero if none. */
	uint64_t last_sequence;
	size_t queued_bytes; /**< Outbox bytes or unconsumed inbox bytes. */
	/** Client retained logical input, else zero. */
	size_t assembly_bytes;
	size_t memory_bytes; /**< Total core-owned session allocation bytes. */
	/** Session mappings or a peer's known memberships. */
	uint32_t entities;
	/** Server peers, including pending/faulted; else zero. */
	uint32_t peers;
	/** Server aggregate faulted count; else zero. */
	uint32_t faulted_peers;
	/** Sticky fault for selected object; zero if none. */
	sync_error_t fault;
} sync_status_t;

/** Immutable session information for delivery adapter integration. */
typedef struct sync_session_info {
	ecs_world_t *world; /**< Borrowed real world; outlives the session. */
	sync_ids_t ids; /**< Copied declarations belonging to world. */
	sync_role_t role; /**< Server or client. */
	sync_schedule_t schedule; /**< Fixed capture/application schedule. */
	sync_limits_t limits; /**< Copied, normalized finite limits. */
} sync_session_info_t;

/**
 * Copy immutable session information without consuming state or progress.
 * @param[in] session Live session, including faulted/ended sessions.
 * @param[out] out_info Required; complete on OK, zero on failure.
 * @return OK or ARGUMENT for a NULL session/output.
 * Allocates nothing. The returned world is borrowed until session teardown;
 * IDs remain world-local. All common owning-thread rules apply.
 */
sync_result_t sync_get_session_info(const sync_session_t *session,
	sync_session_info_t *out_info);

/**
 * Register declarations and return world-local metadata IDs.
 * @param[in,out] world Live real writable world, borrowed for this call.
 * @param[out] out_ids Required; complete on OK, zero on failure.
 * @return OK, ARGUMENT, SCHEMA (conflicting names), UNSUPPORTED or NOMEM.
 * Idempotent. Registers module/types/phase declarations and required Flecs
 * addons only; no session, systems, capture, I/O or simulation progress.
 * On failure no session exists; compatible partial declarations may remain.
 * IDs live until world finalization. Deleting/redefining them is forbidden.
 * The relation removes pairs when its peer target dies; never their owners.
 */
sync_result_t sync_import(ecs_world_t *world, sync_ids_t *out_ids);

/**
 * Create the world's sole server session and freeze its marked schema.
 * @param[in,out] world Borrowed real world; sync_import already succeeded.
 * @param[in] desc Optional copied options; NULL selects defaults.
 * @param[out] out_session Required; owned handle on OK, NULL on failure.
 * @param[out] out_error Optional; creation diagnostic or zero on OK.
 * @return OK, ARGUMENT, STATE (session exists), SCHEMA, UNSUPPORTED,
 * LIMIT or NOMEM. Failure leaves no active session or session-owned systems.
 * Validates schema before activation; installs private tracking and optional
 * scheduled capture. Opens no peers, sockets or files. send_hz must be zero
 * in manual mode. An explicit nonzero session identity must be unique among
 * unrelated live/recorded sessions; fixed identities are useful for tests.
 * Fresh identity generation must fail visibly if uniqueness cannot be met.
 * Imports/declarations may remain after failure; no gameplay state changes.
 */
sync_result_t sync_server_create(ecs_world_t *world,
	const sync_server_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error);

/**
 * Create the world's sole client session in WAIT_MANIFEST.
 * @param[in,out] world Borrowed real world with Sync already imported.
 * @param[in] desc Optional copied options; NULL selects defaults.
 * @param[out] out_session Required; owned handle on OK, NULL on failure.
 * @param[out] out_error Optional; creation diagnostic or zero on OK.
 * @return Same creation failures as sync_server_create.
 * Freezes the locally marked registry. No replicas exist until a complete
 * snapshot is applied. Automatic mode installs a scheduled apply wrapper.
 * updates_per_progress must be zero in manual mode; no server connection is
 * opened. Creation never adopts existing application entities.
 */
sync_result_t sync_client_create(ecs_world_t *world,
	const sync_client_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error);

/**
 * Destroy a session at a writable boundary; NULL is a no-op.
 * @param[in,out] session Handle to release, or NULL.
 * Adapters must first stop all callbacks/systems and release borrowed state.
 * Server: close peers and remove module-assigned NetworkId values, preserving
 * gameplay entities. Client: delete session-owned replicas including attached
 * local-only data; preserve unrelated entities. Remove tracking/systems and
 * free queues. Imported declarations remain usable for a replacement session.
 * Invalidates the handle and all borrowed views/tokens. No recoverable result.
 * Explicit destruction must precede ecs_fini; adapter integration must also
 * enforce adapter -> session -> world order on any automatic teardown path.
 */
void sync_session_destroy(sync_session_t *session);

/**
 * Create a server recipient in PEER_PENDING; capture publishes its snapshot.
 * @param[in,out] session Active server.
 * @param[out] out_peer Required world-local peer entity; zero on failure.
 * @return OK, ARGUMENT, ROLE, STATE, LIMIT or NOMEM.
 * Allocates a fresh nonzero stream ID; IDs are not reused in this session.
 * No snapshot is taken immediately. Set recipient policy before capture.
 * Peer lives until close/destroy. Game code may attach local data, but must
 * not delete, disable, prefab, reparent or add synchronized data to the peer.
 */
sync_result_t sync_peer_open(sync_session_t *session,
	ecs_entity_t *out_peer);

/**
 * Close an active, pending or faulted peer; not a reliable flush operation.
 * @param[in,out] session Server, including a faulted server.
 * @param[in] peer Nonzero live peer belonging to this session.
 * @return OK, ARGUMENT, ROLE or NOT_FOUND; failure changes nothing.
 * Discards queues, removes all direct recipient pairs targeting peer, then
 * deletes peer and its local data. Gameplay entities remain. Views/tokens for
 * this peer expire. Adapter must separately close the transport association.
 */
sync_result_t sync_peer_close(sync_session_t *session, ecs_entity_t peer);

/**
 * Change an automatic server's capture rate without immediate capture.
 * @param[in,out] session Active automatic server.
 * @param[in] send_hz Finite >0; interval 1/send_hz must be representable >0.
 * @return OK, ARGUMENT, ROLE or STATE; failure preserves prior configuration.
 * Resets accumulated time to zero. Uses simulation time; at most one capture
 * per world progression, retaining fmod(accumulated_time, interval) after a
 * capture. Automatic ticks start at 1 and advance once per actual capture.
 */
sync_result_t sync_set_rate(sync_session_t *session, double send_hz);

/**
 * Sample a manual server and queue one logical update for each healthy peer.
 * @param[in,out] session Active manual server at a writable merge boundary.
 * @param[in] tick 1..UINT64_MAX and greater than every consumed prior tick.
 * @param[out] out_result Optional; reports actual progress, even on failure.
 * @return OK (including zero peers), PARTIAL, ARGUMENT, ROLE, STATE, SCHEMA,
 * LIMIT, NOMEM or RUNTIME. ARGUMENT/ROLE/STATE before work consume no tick.
 * Once sampling begins tick is consumed, even on failure; never retry it.
 * Assigns missing IDs before reading values. IDs survive hidden/ineligible
 * periods and are retired on deletion; no wrapping/reuse. New peers receive
 * manifest+snapshot at tick, ready peers a delta (possibly empty).
 * A peer baseline advances only after its entire update is retained. Queue
 * overflow/reference failure faults that peer without suppressing others;
 * PARTIAL includes per-peer faults retrievable with get_status. Already
 * faulted peers are skipped and do not alone cause PARTIAL on later calls.
 * Global schema/tracking failure faults the session and every healthy peer.
 * Dirty state is preserved in retained records or invalidated by faulting;
 * it is never silently lost. Capturing never sends or commits an outbox.
 */
sync_result_t sync_capture(sync_session_t *session, uint64_t tick,
	sync_capture_result_t *out_result);

/**
 * Borrow a peer's immutable head frame without dequeuing it.
 * @param[in] session Live server.
 * @param[in] peer Live peer from that session.
 * @param[out] out_view Required; zero on EMPTY/error.
 * @return OK, EMPTY, ARGUMENT, ROLE, STATE (faulted), or NOT_FOUND.
 * Repeated peeks return the same head/token until committed. Tail appends,
 * other peer operations and fault transitions do not invalidate an existing
 * view. It expires only on successful head commit, peer close, or destruction.
 * Only one consumer may peek/commit per peer; use explicit copies for a tee.
 * Borrowed memory must not be mutated, freed or passed to a retaining no-copy
 * transport without an independent lifetime mechanism.
 */
sync_result_t sync_outbox_peek(const sync_session_t *session,
	ecs_entity_t peer, sync_frame_view_t *out_view);

/**
 * Release exactly a previously peeked head after reliable adapter acceptance.
 * @param[in,out] session Live server.
 * @param[in] peer Live owning peer.
 * @param[in] token Unmodified token copied from that peer's current head.
 * @return OK, ARGUMENT (stale/foreign token), ROLE, STATE or NOT_FOUND.
 * Failure leaves queues/views unchanged. Success frees the head and expires
 * every view of it. Does not mean remote receipt/application, and does not
 * advance a capture baseline (that happened during enqueue). On send failure
 * retain and retry the same head, or close; never commit a dropped frame.
 */
sync_result_t sync_outbox_commit(sync_session_t *session,
	ecs_entity_t peer, sync_outbox_token_t token);

/**
 * Copy exactly one complete frame to a client's bounded incoming queue.
 * @param[in,out] session Live client, neither faulted nor ended.
 * @param[in] data Non-NULL buffer, valid only for this call.
 * @param[in] size Complete frame bytes; 1..configured max_frame_bytes.
 * @return OK, BACKPRESSURE, ARGUMENT, ROLE, STATE, PROTOCOL, UNSUPPORTED,
 * LIMIT or NOMEM.
 * OK accepts the whole frame and permits immediate release of input memory.
 * Every other result accepts zero bytes. BACKPRESSURE leaves stream state
 * unchanged: drain through apply and retry the same frame before later ones.
 * Basic envelope/length failures fault the client without ECS mutation.
 * Schema, sequencing and logical validation happen during apply. Recoverable
 * allocation failure accepts nothing and faults the client. No partial reads,
 * trailing bytes, frame concatenation, or auto-detection of a new stream.
 * ARGUMENT/ROLE/STATE never change the stream. An oversized nonempty frame
 * returns LIMIT and faults; zero size/NULL data are caller ARGUMENT errors.
 */
sync_result_t sync_receive(sync_session_t *session,
	const void *data, size_t size);

/**
 * Publish up to max_updates complete updates on a manual client.
 * @param[in,out] session Manual client at a writable boundary.
 * @param[in] max_updates 1..UINT32_MAX; snapshot counts as one update.
 * @param[out] out_result Optional actual progress, also on error.
 * @return OK if at least one update was published and no error; EMPTY if no
 * input or assembly exists; NEED_MORE if schema/fragments were consumed or
 * assembly needs input but nothing published; END for an ended stream;
 * ARGUMENT, ROLE, STATE, SCHEMA, UNSUPPORTED, PROTOCOL, SEQUENCE, REFERENCE,
 * LIMIT, NOMEM or RUNTIME on error. No public apply in automatic mode.
 * Consumes available frames into bounded assembly until the update budget,
 * exhaustion of input, or an error. A fragment-only pass can free receive
 * queue space without publishing. The update budget is not a CPU time bound.
 * Validates each entire logical update before any of its ECS mutations.
 * Earlier updates in this call stay applied if a later one fails; no claim of
 * whole-call atomicity. Runtime failure after mutation faults without
 * rollback.
 * Introduced entities are allocated before references resolve; removals occur
 * after referring values are replaced/cleared. Ordinary observers run and may
 * see individual changes; downstream game systems see completed updates.
 * Error faults retain published replicas until explicit reset/destruction.
 */
sync_result_t sync_apply(sync_session_t *session, uint32_t max_updates,
	sync_apply_result_t *out_result);

/**
 * Declare final EOF after the adapter has delivered and drained all frames.
 * @param[in,out] session Live client in either scheduling mode.
 * @return OK on clean EOF (also idempotently when ended), BACKPRESSURE if
 * inbox still contains frames, TRUNCATED if assembly or initial snapshot is
 * incomplete, ARGUMENT, ROLE, or STATE if already faulted.
 * BACKPRESSURE makes no change; drive normal application and retry. Clean
 * finish transitions to ENDED and preserves replicas for replay inspection.
 * TRUNCATED faults without publishing incomplete state. A never-initialized
 * stream is truncated, not an empty valid recording. No world mutation.
 */
sync_result_t sync_client_finish(sync_session_t *session);

/**
 * End/reset the incoming stream at a writable boundary, including after fault.
 * @param[in,out] session Client in any state.
 * @return OK, ARGUMENT or ROLE. Existing valid local cleanup hooks must not
 * fail; fatal Flecs errors are outside recoverable error handling.
 * Stop/discard the old adapter stream first. Deletes all owned replicas and
 * their local-only data, clears mappings/queues/faults, retains frozen schema
 * and scheduling, then returns to WAIT_MANIFEST. Repeated reset is harmless.
 * The next accepted stream must begin with its manifest and full snapshot.
 * Game-created entities remain; old local replica IDs must not be retained.
 */
sync_result_t sync_client_reset(sync_session_t *session);

/**
 * Copy status without consuming errors, frames or capture progress.
 * @param[in] session Any live session, including faulted/ended.
 * @param[in] peer Zero for session status; otherwise a server peer entity.
 * @param[out] out_status Required; zero on failure.
 * @return OK, ARGUMENT, ROLE (peer requested on client), or NOT_FOUND.
 * Server aggregate last_tick is the last consumed capture tick; last_sequence
 * is zero and queued_bytes sums all outboxes. Its state can remain READY with
 * faulted peers: inspect faulted_peers and each affected peer. Control errors
 * such as a stale token do not overwrite the sticky stream fault diagnostic.
 */
sync_result_t sync_get_status(const sync_session_t *session,
	ecs_entity_t peer, sync_status_t *out_status);

/**
 * Resolve a NetworkId within this session to a borrowed local entity ID.
 * @param[in] session Live server/client, including faulted/ended.
 * @param[in] network_id Nonzero identity in the current server namespace.
 * @param[out] out_entity Required; local ID on OK, zero on error.
 * @return OK, ARGUMENT or NOT_FOUND. Does not allocate or adopt entities.
 * Server can return a hidden entity retaining its ID; client can return only
 * a published, still-present replica. Returned ID expires on deletion, client
 * visibility loss/reset, or session destruction; it is never a wire ID.
 */
sync_result_t sync_lookup(const sync_session_t *session,
	uint64_t network_id, ecs_entity_t *out_entity);

/**
 * Decode a complete frame's envelope for transport/file inspection.
 * @param[in] data Read-only complete frame bytes, non-NULL.
 * @param[in] size Buffer bytes, 1..UINT32_MAX.
 * @param[out] out_info Required copied envelope; zero on failure.
 * @return OK, ARGUMENT, PROTOCOL or UNSUPPORTED (protocol version).
 * Pure/reentrant; no world, allocation, retention, payload decoding, schema
 * checks, ordering checks or configured-limit enforcement. OK is not a trust
 * decision. Adapters deliver the original frame unchanged to receive/apply.
 */
sync_result_t sync_frame_inspect(const void *data, size_t size,
	sync_frame_info_t *out_info);

#ifdef __cplusplus
}
#endif
#endif
