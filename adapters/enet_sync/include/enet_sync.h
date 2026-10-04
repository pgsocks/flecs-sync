/**
 * @file enet_sync.h
 * @brief Dedicated ENet delivery of unchanged authoritative Sync frames.
 *
 * C11/C++17, owning thread, nonreentrant. Input/output objects are disjoint;
 * non-NULL outputs are zeroed before validation. Descriptors/addresses are
 * copied; sessions and callback contexts are borrowed until close. The wrapper
 * manages ENet initialization for each session world. Adapters own hosts;
 * external/shared hosts are unsupported. All mutators require a writable world
 * boundary. Close adapters before sessions, then worlds. Bound sessions/peers
 * cannot be externally closed/reset. Client replicas and queues survive
 * disconnect/close; close, explicitly reset Sync, then open a new adapter to
 * reconnect. Disconnect never signals EOF to Sync.
 *
 * Service never captures or applies. Channel zero carries one complete frame
 * per reliable packet. Send commits when the wrapper admits an owned byte copy,
 * which is not receipt/application acknowledgment. Close discards outstanding
 * delivery without waiting. Server owns connection peers; client never owns the
 * session or replicas. V1 has no upstream gameplay, authentication or automatic
 * reconnect. A client adapter must be the sole stream producer for its session;
 * each server peer has one outbox consumer.
 *
 * World progression executes transport requests. Manual service consumes
 * available events and submits work without capture/apply.
 * Lifecycle callbacks occur inside poll/send/service and automatic systems.
 * They may update gameplay ECS data (including recipient policy) but cannot
 * reenter any adapter, call capture/apply, or close/reset bound Sync objects.
 * Recording callbacks receive borrowed accepted bytes only for their call.
 * Their failures are application-owned and independent of delivery.
 * Detectable argument/mode errors leave healthy handles unchanged. Faults are
 * copied, not errno-based. Connection faults close only their association;
 * host service failures fault the handle. Dangling handles are caller bugs.
 */
#ifndef ENET_SYNC_H
#define ENET_SYNC_H
#include <sync.h>
#include <flecs_enet.h>
#ifdef __cplusplus
extern "C" {
#endif

/** Owned dedicated listening transport; borrows server session. */
typedef struct enet_sync_server enet_sync_server_t;
/** Owned dedicated connecting transport; borrows fresh client session. */
typedef struct enet_sync_client enet_sync_client_t;

/** Adapter state; disconnect retains core client state. */
typedef enum enet_sync_state {
	ENET_SYNC_CONNECTING = 1, /**< Client connection pending. */
	ENET_SYNC_ACTIVE = 2, /**< Listening server or connected client. */
	ENET_SYNC_DISCONNECTED = 3, /**< Close, reset core, then reopen. */
	ENET_SYNC_FAULTED = 4, /**< Sticky failure; close required. */
	ENET_SYNC_OPENING = 5 /**< Server setup pending. */
} enet_sync_state_t;

/** Copied lifecycle type. */
typedef enum enet_sync_event_kind {
	ENET_SYNC_CONNECTED = 1, /**< Core peer exists before callback. */
	ENET_SYNC_DISCONNECTED_EVENT = 2, /**< Peer closed after callback. */
	ENET_SYNC_CONNECTION_FAULT = 3 /**< Peer closed after callback. */
} enet_sync_event_kind_t;

/** Bounded diagnostic; detail.code zero denotes no failure. */
typedef struct enet_sync_error {
	sync_error_t detail; /**< Adapter diagnostic or copied core fault. */
	ecs_entity_t peer; /**< Core peer; zero for clients/host failures. */
	flecs_enet_address_t address; /**< Remote address, zero if unknown. */
} enet_sync_error_t;

/** Borrowed callback event; copy if needed after callback. */
typedef struct enet_sync_event {
	enet_sync_event_kind_t kind; /**< Connection, disconnect or fault. */
	ecs_entity_t peer; /**< Core peer; zero on client/setup rejection. */
	flecs_enet_address_t address; /**< Remote endpoint. */
	uint32_t data; /**< ENet connection/disconnection data. */
	enet_sync_error_t error; /**< Nonzero only on fault. */
} enet_sync_event_t;

/** Callback obeying the file-level lifecycle/nonreentrancy contract. */
typedef void (*enet_sync_event_fn)(const enet_sync_event_t *event, void *ctx);
/** Recording tee: server admission or successful client receive.
 * Server peer identifies the stream; client peer is zero. Return no result:
 * recording diagnostics belong to the application, never network delivery.
 * Bytes are borrowed only for the callback, including admitted request copies.
 */
typedef void (*enet_sync_frame_fn)(ecs_entity_t peer, const void *data,
	size_t size, void *ctx);

/** Copied finite options; zero fields select defaults except manual budgets. */
typedef struct enet_sync_options {
	sync_schedule_t schedule; /**< MANUAL=0 default, AUTOMATIC=1. */
	uint32_t events_per_progress; /**< Auto: 0=64; manual requires 0. */
	uint32_t frames_per_peer; /**< Auto: 0=64; manual requires 0. */
	/** Host limits; zero fields select wrapper defaults. The packet bound
	 * derives from Sync; explicit max_packet_bytes must match. Capacities
	 * for event/outgoing/waiting bytes must fit that bound. */
	flecs_enet_limits_t transport_limits;
	enet_sync_event_fn on_event; /**< Optional lifecycle callback. */
	enet_sync_frame_fn on_frame; /**< Optional recording callback. */
	void *ctx; /**< Borrowed context for both callbacks. */
} enet_sync_options_t;

/** Required copied listening descriptor. */
typedef struct enet_sync_server_desc {
	sync_session_t *session; /**< Required active borrowed server. */
	flecs_enet_address_t address; /**< Listen address; port zero allowed. */
	uint32_t max_clients; /**< 0=32; maximum 4095. */
	enet_sync_options_t options; /**< Copied options. */
} enet_sync_server_desc_t;

/** Required copied asynchronous connection descriptor. */
typedef struct enet_sync_client_desc {
	sync_session_t *session; /**< Required fresh WAIT_MANIFEST client. */
	flecs_enet_address_t address; /**< Nonzero IPv4 endpoint and port. */
	uint32_t data; /**< ENet connection data sent to server callback. */
	enet_sync_options_t options; /**< Copied options. */
} enet_sync_client_desc_t;

/** Actual call progress, preserved on pressure/partial/error. */
typedef struct enet_sync_progress {
	uint64_t events; /**< Consumed wrapper events, including results. */
	uint64_t frames; /**< Frames admitted by wrapper or Sync this call. */
	uint64_t bytes; /**< Complete frame bytes admitted this call. */
	uint32_t connections; /**< Connections established this call. */
	uint32_t disconnections; /**< Associations closed this call. */
	uint32_t faulted_connections; /**< Isolated connection failures. */
} enet_sync_progress_t;

/** Side-effect-free copied status. No transport/core frame pointers. */
typedef struct enet_sync_status {
	enet_sync_state_t state; /**< Current adapter state. */
	ecs_entity_t host; /**< Owned wrapper host in the session world. */
	sync_schedule_t schedule; /**< Immutable scheduling mode. */
	flecs_enet_address_t address; /**< Bound server or remote client. */
	uint32_t connections; /**< Server associations or client (0/1). */
	uint32_t max_frame_bytes; /**< Inherited core frame bound. */
	enet_sync_options_t options; /**< Copied effective configuration. */
	uint64_t frames; /**< Lifetime admitted frame count. */
	uint64_t bytes; /**< Lifetime admitted complete frame bytes. */
	size_t retained_bytes; /**< Host pending plus ENet-owned bytes. */
	uint64_t retained_packets; /**< Host ENet-owned packet count. */
	size_t pending_bytes; /**< Exact pressured client event bytes. */
	enet_sync_error_t fault; /**< Sticky host/client fault or zero. */
	enet_sync_error_t last_connection_error; /**< Last isolated fault. */
} enet_sync_status_t;

/** Listen with a dedicated host. Required desc/output; optional diagnostic.
 * Returns OK, ARGUMENT, ROLE, STATE, LIMIT, NOMEM, TRANSPORT or RUNTIME.
 * Admission failure leaves output NULL, no systems/peers. Success is OPENING;
 * execution failures arrive through status/callbacks. Auto installs
 * Receive/Send systems; borrowed server may use either schedule. Never
 * captures.
 */
sync_result_t enet_sync_server_listen(const enet_sync_server_desc_t *desc,
	enet_sync_server_t **out_server, enet_sync_error_t *out_error);
/** Connect asynchronously with one channel and one peer. Same open results.
 * Client must have no queued/assembled input; auto requires an auto client.
 * Output is CONNECTING with OPEN pending; on_event reports eventual connection
 * or failure after progression.
 */
sync_result_t enet_sync_client_connect(const enet_sync_client_desc_t *desc,
	enet_sync_client_t **out_client, enet_sync_error_t *out_error);
/** Poll manual server, max_events>0; consumes available wrapper events.
 * Opens peers before callbacks and rejects client packets. Progression
 * services protocol; polling alone executes no native transport work.
 * Returns OK on progress, EMPTY if none, PARTIAL on isolated peer failure,
 * or ARGUMENT/STATE/TRANSPORT. Progress optional, initialized on every return.
 */
sync_result_t enet_sync_server_poll(enet_sync_server_t *server,
	uint32_t max_events, enet_sync_progress_t *out);
/** Send manual server outboxes, max_frames_per_peer>0 independently per peer.
 * Returns OK on progress, EMPTY, BACKPRESSURE (head unchanged), PARTIAL for
 * isolated peer faults, ARGUMENT/STATE/TRANSPORT. Submits work, never captures.
 */
sync_result_t enet_sync_server_send(enet_sync_server_t *server,
	uint32_t max_frames_per_peer, enet_sync_progress_t *out);
/** Compose poll then send; same positive budgets, optional combined progress.
 * Poll host failure stops send; isolated faults allow healthy peers to send.
 */
sync_result_t enet_sync_server_service(enet_sync_server_t *server,
	uint32_t max_events, uint32_t max_frames_per_peer,
	enet_sync_progress_t *out);
/** Receive manual client, max_events>0; consumes available wrapper events.
 * Retry the retained receive event before dispatching later events. On
 * pressure, world progression continues wrapper protocol service. Returns OK,
 * EMPTY, BACKPRESSURE, END for disconnect (not core EOF), ARGUMENT/STATE or
 * sticky receive/host error. Never applies. Progress optional, initialized on
 * every return.
 */
sync_result_t enet_sync_client_service(enet_sync_client_t *client,
	uint32_t max_events, enet_sync_progress_t *out);
/** Request immediate transport disconnect; callback/core peer removal on next
 * service. Required live handle/peer; returns OK, ARGUMENT, STATE, NOT_FOUND,
 * BACKPRESSURE, NOMEM or LIMIT. Admission pressure leaves the association live.
 * Execution requires progression. No acknowledgment wait. Available in both
 * schedules. Client idempotent
 * after disconnect; no finish/reset. Data delivered to local callback only.
 */
sync_result_t enet_sync_server_disconnect(
	enet_sync_server_t *server, ecs_entity_t peer, uint32_t data);
/** Request client disconnect under the preceding contract. */
sync_result_t enet_sync_client_disconnect(
	enet_sync_client_t *client, uint32_t data);
/** Copy status including faulted/disconnected handles. Required handle/output.
 * Returns OK or ARGUMENT/STATE (callback reentry); allocates nothing.
 */
sync_result_t enet_sync_server_get_status(
	const enet_sync_server_t *server, enet_sync_status_t *out);
/** Copy client status under the preceding contract. */
sync_result_t enet_sync_client_get_status(
	const enet_sync_client_t *client, enet_sync_status_t *out);
/** Close NULL is OK; otherwise stop systems, discard packets, destroy host and
 * owned peers. No callbacks/drain/capture/apply. Handle invalid on return.
 * Returns sticky host fault or OK; optional diagnostic initialized. Client
 * session/replicas survive. Calling close from a callback is a caller bug.
 */
sync_result_t enet_sync_server_close(
	enet_sync_server_t *server, enet_sync_error_t *out_error);
/** Close client under the preceding contract; discards pending packet. */
sync_result_t enet_sync_client_close(
	enet_sync_client_t *client, enet_sync_error_t *out_error);
#ifdef __cplusplus
}
#endif
#endif
