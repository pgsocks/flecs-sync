# EnetSync

EnetSync is the optional transport adapter in `adapters/enet_sync`. Link
`EnetSync::enet_sync` and include `enet_sync.h`. It borrows Sync sessions,
owns dedicated Flecs ENet host entities, and sends complete unchanged Sync frames using
reliable channel zero. Sync owns reflection, schema validation, recipient
visibility, capture, decoding, and publication. [Flecs ENet](https://github.com/pgsocks/flecs-enet), pinned to
`8b00bf290c7eb5d17c3b6b5858b7086405a224d0`, supplies ENet transitively
through CPM when `SYNC_BUILD_ENET_SYNC=ON` (the default).

V1 supports authoritative server-to-client replication. Gameplay input,
authentication, shared external hosts, and automatic reconnect belong to the
application. The [public header](../adapters/enet_sync/include/enet_sync.h)
is the exact API contract.

## Manual integration

The wrapper manages ENet initialization per world. Register reflected types
and mark their definitions with `ids.sync` before creating server/client
sessions. The [core integration guide](usage.md) explains that setup.

```c
static void connection_event(const enet_sync_event_t *event, void *context)
{
	struct game *game = context;
	if (event->kind != ENET_SYNC_CONNECTED)
		return;
	/* Application maps connection data to a player before the first capture. */
	ecs_entity_t player = find_player(game, event->data);
	ecs_add_pair(game->world, player, game->ids.replicate_to, event->peer);
}

enet_sync_server_desc_t desc = {
	.session = server,
	.address = { .port = 7777 },
	.options = { .on_event = connection_event, .ctx = game }
};
enet_sync_server_t *transport = NULL;
enet_sync_error_t error;
sync_result_t result = enet_sync_server_listen(&desc, &transport, &error);
/* Handle result before continuing; diagnostic strings are copied. */
```

Listen returns `ENET_SYNC_OPENING` after admitting an OPEN request. Progress
the session world, then service the adapter to observe execution. Port zero
requests an ephemeral port; status reports the bound address after execution.
Addresses are copied numeric IPv4 endpoints, with no blocking DNS lookup.

```c
flecs_enet_address_t address = { .ip = {127, 0, 0, 1}, .port = 7777 };
enet_sync_client_desc_t desc = {
	.session = client,
	.address = address,
	.data = player_connection_data
};
enet_sync_client_t *connection = NULL;
sync_result_t result = enet_sync_client_connect(&desc, &connection, &error);
```

Connect is asynchronous: successful open means `ENET_SYNC_CONNECTING`, not
that a remote server has accepted the connection. Progress the session world, consume available events, and
use the lifecycle callback/status to observe `ENET_SYNC_ACTIVE` or failure.
Connection data is application metadata, not authenticated identity.

At writable owning-thread boundaries, independently schedule these steps:

1. `enet_sync_server_poll(transport, 64, &progress)` establishes peers and
   invokes callbacks. Set recipient policy here before capture.
2. Merge gameplay writes, then `sync_capture(server, tick, &capture)`.
3. `enet_sync_server_send(transport, 64, &progress)` drains each recipient's
   queue with its own frame budget into wrapper-owned requests.
4. Progress the server and client worlds to execute transport work.
   `enet_sync_client_service(connection, 64, &progress)` accepts frames.
5. `sync_apply(client, 1, &application)` publishes complete updates.

Service calls never capture or apply. `enet_sync_server_service` composes poll
and send and cannot replace the capture between those operations when a new
snapshot is wanted immediately. World progression executes nonblocking ENet work;
manual adapter calls only submit requests and consume retained events. Budgets count events or complete outgoing frames, not
CPU time. Progress survives partial failures and pressure.

The [compiling loopback example](../adapters/enet_sync/examples/loopback.c)
shows full setup, explicit scheduling, and cleanup. With tests enabled, run
`build/adapters/enet_sync/tests/enet_sync_example` on a socket-capable machine.

## Automatic integration

Set `options.schedule = SYNC_AUTOMATIC`. Server transport may borrow a manual
or automatic server; client transport requires an automatic client. The
adapter polls in `sync.Receive` and sends in `sync.Send`. Core systems capture
in `sync.Capture` and apply in `sync.Apply`. Manual sessions still require
explicit capture even if their server adapter sends automatically.

```text
EcsOnLoad -> flecs_enet.Receive -> sync.Receive -> sync.Apply -> EcsPreUpdate
EcsPostUpdate -> sync.Capture -> sync.Send -> flecs_enet.Send
```

Received frames can publish before gameplay readers in the same progression.
Asynchronous send results are consumed on subsequent adapter servicing.

Zero automatic budgets normalize to 64 events per progression and 64 outgoing
frames per peer. Set `events_per_progress` and `frames_per_peer` for finite
custom budgets. Manual descriptors require both fields to be zero; explicit
manual calls take their own positive budgets. Manual transfer calls on an
automatic adapter return `SYNC_ERR_STATE` before transport or core work.
Disconnect, status, and close remain available in either schedule.

## Ownership, pressure, and failure

The application owns gameplay entities, worlds, and Sync sessions. The wrapper
balances ENet initialization at world cleanup. The adapter owns its host entity,
systems, requests, events, and generation-aware transport associations. Status
exposes `host` for inspection using the wrapper's world-local IDs. Each server
connection owns a fresh Sync peer. Multiple adapters and unrelated wrapper hosts
can coexist in the same world; each adapter consumes only its own host's events.
A client adapter is the sole stream producer for its borrowed client session.

Outgoing admission copies a frame into an immutable wrapper request. Successful
admission commits the core head and invokes recording with that owned copy.
Execution occurs later in `flecs_enet.Send`; an execution failure faults the
association and cancels its remaining requests. Admission and execution do not
promise remote receipt or application. Admission pressure preserves the exact
core head and token.

`options.transport_limits` uses the wrapper's finite host-wide capacities.
Zero fields keep wrapper defaults: 1024 requests, 1024 events, 8192 outgoing
packets, 32 MiB event/outgoing/waiting bytes, and 64 dispatched events per
progression. Outgoing bytes include pending requests and ENet-owned packets;
packet counts include accepted ENet packets. These capacities are shared across
all recipients on a host; adapter frame budgets remain per recipient.

The packet bound derives from the Sync session. An explicit `max_packet_bytes`
must equal it, and event/outgoing/waiting byte capacities must fit it. ENet
waiting bytes are per native peer and separate from retained receive events.
Core inbox and assembly bounds remain independent.

When `sync_receive` returns `SYNC_BACKPRESSURE`, EnetSync retains that exact
receive event and retries it before any later event, including disconnect.
World progression continues wrapper protocol service under its own capacities.
Drain Sync through apply/progression to resume. Completed requests and consumed
events are deleted, returning their storage and capacity to the wrapper.

A server peer send/setup/protocol/core failure reports a copied lifecycle
error, closes its ENet association and owned core peer, and returns
`SYNC_PARTIAL`; healthy recipients continue. `last_connection_error` copies
the most recent isolated failure. Host service failure faults the adapter and
closes associations. Client core/transport failures fault the adapter, retaining
published core state. Sticky faults are available in status and returned by
close. Argument, role, scheduling, and state control errors preserve healthy
handles. Inspect Sync status separately for publication errors.

Lifecycle callbacks run within poll/send/service, on the owning thread. They
may update application ECS data, including recipient policy. They cannot
reenter adapters, capture/apply, or close/reset bound objects. A disconnect
callback sees the server peer before it is removed; copy any needed IDs/data
before returning. Do not retain its peer after the callback. Close invokes no
callbacks.

## Recording accepted frames

Open one raw FileSync writer per server recipient. In `on_frame`, call
`file_sync_writer_write(writer_for_peer(peer), data, size)` and save any error
in application-owned recording status. Writers need a frame limit compatible
with the core session. Callback bytes are borrowed only for that call; never
retain or destroy them. The callback has no return value and a recording
failure does not cancel a network send or roll back its committed core head.
Client adapters also support accepted-frame callbacks, with peer zero.

## Disconnect, reconnect, and close

Disconnect admits a request for immediate transport release. World progression
executes it; subsequent service reports lifecycle notification and removes the
owned server core peer. Close
also discards outstanding packets and removes systems/owned peers; it never
waits for acknowledgments. Closing a client never finishes/resets Sync or
deletes replicas. The core may retain queued input and an incomplete update,
so transport disconnect is not clean EOF.

To reconnect a client, close the old adapter, explicitly call
`sync_client_reset`, and open a new adapter. Reset deletes old replicas and
returns Sync to `WAIT_MANIFEST`. The new server peer has a new stream and gets
a fresh manifest/full snapshot on capture. Do not attach another stream to a
client holding the old stream's state.

Always tear down adapter, then session, then world. Do not close or
reset bound sessions/peers externally. Opening, connection servicing, and
closing require writable world boundaries and the owning thread.

## Verification

Tagged [Gherkin specifications](../features/enet_sync/enet_sync.feature)
execute through persistent deterministic and production UDP fixtures, and
map one-to-one to native contract case IDs. CTest individually registers each
case. Private instrumentation uses the wrapper’s native test boundary to emulate
ENet acceptance/release and inject failures without modifying dependencies. Real loopback cases use the production adapter/ENet.
Both suites include Aria, Bram, the GM, NPC privacy, inventory references,
trading, visibility changes, and reconnection snapshots. When FileSync is
also enabled, accepted-frame recordings replay through its public API.

Cucumber requires Ruby >=3.2, RubyGems >=3.2.8 and Bundler 2.6.9.
`SYNC_CUCUMBER` defaults to `ON`; disabling it retains all native tests.

```sh
gem install bundler -v 2.6.9
BUNDLE_FROZEN=true bundle _2.6.9_ install
cmake -S . -B build -DBUILD_TESTING=ON -DSYNC_CHECK_CXX=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
ctest --test-dir build -L loopback --output-on-failure
ctest --test-dir build -R '^sync\.cucumber_enet_sync$' -V
# Socket-restricted environments can run the other checks separately:
ctest --test-dir build -LE loopback --output-on-failure

cmake -S . -B build-core -DSYNC_BUILD_ENET_SYNC=OFF \
	-DSYNC_BUILD_FILE_SYNC=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure

cmake -S . -B build-library -DBUILD_TESTING=OFF
cmake --build build-library --parallel
```

Real loopback validation requires UDP socket creation. Loopback cases remain
required and are not silently skipped when a sandbox denies sockets. Passing
only the deterministic suite does not verify the ENet network path. The
[testing guide](testing.md) also gives sanitizer commands; run them with both
adapters enabled and rerun loopback in a socket-capable local/CI environment.
