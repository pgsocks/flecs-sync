# Public API guide

[README](../README.md) · [Integration](usage.md)
· [Architecture](architecture.md)

Use this guide when integrating Sync into a game loop or delivery adapter.
The [header](../include/sync.h) is authoritative for exact declarations,
per-function results, accepted ranges, and output initialization. The
[API design][design] explains the same contracts by workflow.

## Calling and ownership rules

A session borrows a real Flecs world, never a stage. Each world has at most
one active session. Ordinary calls run on its owning thread, do not overlap,
and are nonreentrant. Worker threads must hand owned bytes to that thread.
Only the stateless `sync_frame_inspect` operation is reentrant.

ECS mutators require a writable boundary: no live iterators, readonly mode,
outstanding deferred commands, or concurrent world access. Receive, peek,
commit, finish, rate changes, and getters do not mutate ECS storage, but still
obey the owning-thread and nonreentrancy rules.

Hooks invoked by Sync must not reenter Sync, mutate replication metadata,
delete peers or replicas, or throw across the C ABI. Queue application work
for a later boundary. Do not connect unrelated local entities to replicas or
peers through cascading deletion policies such as `ChildOf`.

| Resource | Ownership and lifetime |
| --- | --- |
| World | Application-owned; outlives its session. |
| Session | Caller destroys; owns replication state and client replicas. |
| Peer | Sync-owned entity; close through `sync_peer_close`. |
| Descriptors | Copied at creation; caller can release them afterward. |
| Outbox view | Borrowed until successful commit, peer close, or destroy. |
| Receive input | Copied on `SYNC_OK`; caller retains its buffer. |
| Status and error | Copied values; no retained internal pointers. |

`sync_get_session_info` copies the borrowed real world, world-local declaration
IDs, role, scheduling mode, and normalized limits without consuming state.
Adapters use this public information to install systems and bound buffers;
there is no need to inspect core internals. The returned world/session lifetime
and owning-thread rules still apply. See [FileSync](file-sync.md) for a concrete
recording/replay adapter.

Required pointers must be valid and non-null; input and output objects must
be mutually disjoint. Except where specified, non-null outputs are zeroed
before fallible work. A detectable argument error is reported, but dangling
handles and violated Flecs invariants remain caller errors. Results do not
use `errno`, guarantee rollback, or convert fatal Flecs failures into errors.

## Import, register, and create

`sync_import(world, &ids)` is idempotent and registers declarations only.
It returns a `sync_ids_t` belonging to that world. Do not reuse those IDs in
another world or delete/redefine the declarations.

| Field | ECS path | Use |
| --- | --- | --- |
| `module` | `sync` | Module namespace. |
| `sync` | `sync.Sync` | Mark a component or tag definition. |
| `network_id` | `sync.NetworkId` | Read `sync_network_id_t.value`. |
| `replicate_to` | `sync.ReplicateTo` | Pair with a server peer. |
| `replicate_to_all` | `sync.ReplicateToAll` | Include all healthy peers. |
| `receive_phase` | `sync.Receive` | Adapter input phase. |
| `apply_phase` | `sync.Apply` | Client publication phase. |
| `capture_phase` | `sync.Capture` | Server sampling phase. |
| `send_phase` | `sync.Send` | Adapter output phase. |

Register reflection and mark every synchronized type before calling
`sync_server_create` or `sync_client_create`. Creation validates and freezes
the schema. A failed creation leaves no active session or session systems;
compatible imported declarations can remain. Import and creation open no I/O.

Pass `NULL` or a zero-initialized descriptor for manual mode and finite
default limits. In manual mode, server `send_hz` and client
`updates_per_progress` must be zero. A client's server-only limit fields are
ignored. Scheduling mode is fixed until session destruction.

A zero server `session_id` requests a fresh identity. Fixed nonzero identities
support reproducible fixtures but must remain unique among unrelated live or
recorded sessions. Fresh identity generation fails visibly if it cannot meet
that requirement.

## Peers, policy, and identity

`sync_peer_open` creates a server recipient in `SYNC_PEER_PENDING` with a fresh
stream ID. Its first snapshot occurs on the next capture, after application
code has set recipient policy. Peers may carry local application data, but
must not be deleted, disabled, reparented, made prefabs, or given synchronized
data by game code.

Use the all-recipients tag or direct recipient pairs on gameplay entities.
Only directly owned registered components and tags participate. Network IDs
and policy markers are metadata and are excluded from gameplay payloads.

`sync_lookup` translates a nonzero network ID into a borrowed local entity ID.
The server can find hidden entities retaining their identity; clients can
find only published replicas still present. Visibility loss, deletion, reset,
or session destruction can invalidate a returned client ID. Zero network ID
represents a null reference and is not a valid lookup argument.

`sync_peer_close` discards the peer's queue and local data, removes recipient
pairs, and deletes the peer. Gameplay entities remain. It can close faulted
peers and is not a flush or delivery acknowledgement. The adapter separately
closes its connection.

## Capture, transfer, and apply

`sync_capture` samples a manual server after writes merge. Supply a positive
tick greater than every previously consumed tick. Once sampling starts, the
tick is consumed even if capture fails. Pre-work argument, role, or state
rejection consumes no tick; inspect `sync_capture_result_t.tick`.

New peers receive a manifest and snapshot; ready peers receive a delta,
possibly empty. A peer's membership baseline advances only when its entire
update is retained. `peers_queued` and `peers_faulted` report actual progress,
including partial results. Already faulted peers are skipped and do not alone
make subsequent captures return `SYNC_PARTIAL`.

`sync_outbox_peek` borrows the current immutable head and its opaque token.
Repeated peeks return that head until it is committed. Tail appends, work on
other peers, and fault transitions leave an existing view valid. A faulted
peer rejects new delivery operations; close it to release retained storage.

`sync_outbox_commit` accepts the unchanged token for that peer's exact head.
Commit only after the adapter takes responsibility for the bytes. Success
releases the head and invalidates its views. A stale or foreign token cannot
release another frame. Commit does not mean remote receipt or application.
Never hand borrowed storage to a retaining no-copy transport without an
independent lifetime mechanism.

`sync_receive` accepts exactly one complete frame. Only `SYNC_OK` copies its
bytes. `SYNC_BACKPRESSURE` accepts nothing and leaves the stream unchanged:
drain through apply, then retry that same frame before sending later frames.
Do not concatenate frames, deliver partial frames, or silently drop input.
Envelope failures fault the client; logical validation occurs during apply.

`sync_apply` processes a manual client with `max_updates` from 1 through
`UINT32_MAX`. A snapshot counts as one update. It can consume manifest or
fragment frames without publishing anything. Check both `frames_consumed`
and `updates_applied`; `last_tick` identifies the last update published by
the call. The update budget does not bound CPU time.

The first applied frame must be a manifest with sequence one. Complete
logical updates are validated before mutations begin. Earlier updates from
the same call remain published if a later update fails. Observers see ordinary
Flecs mutations; runtime failures can leave partial mutations without rollback.

## Automatic scheduling

Set `.schedule = SYNC_AUTOMATIC` when creating a session. The server defaults
to 20 Hz when `send_hz` is zero; the client defaults to one complete update
per progression when `updates_per_progress` is zero. Manual capture/apply
calls on automatic sessions are rejected.

The standard pipeline orders phases as follows:

```text
EcsOnLoad -> sync.Receive -> sync.Apply -> EcsPreUpdate
EcsPostUpdate -> sync.Capture -> sync.Send -> EcsPreStore
```

Synchronized writers finish by `EcsPostUpdate`; client readers run after apply.
Automatic setup installs single-threaded systems with writable boundaries and
the surrounding phase dependencies. Import alone declares phases and their
internal pair order. Custom pipelines must include these phases in the right
order or use manual integration. Cyclic ordering must fail creation.

Capture follows simulation time and runs at most once per progression. A
stall captures current state once and retains the fractional interval
remainder. Automatic ticks begin at one and advance per actual capture.
`sync_set_rate` accepts only a finite positive rate with a representable
positive reciprocal, resets the accumulator, and does not capture immediately.
It is valid only on an active automatic server.

## Schema and limits

Supported values are fixed-width integers, booleans, IEEE-754 binary32 and
binary64 floats, supported fixed-width enums/bitmasks, reflected structs,
fixed arrays, strings, and entity references. Native lifecycle operations
must initialize, copy/move, and destroy nontrivial values correctly.

Platform-sized integers such as `size_t`, unmanaged pointers, opaque
resources, and unsupported dynamic containers fail schema validation.
Arbitrary gameplay pairs, inherited values, per-component visibility,
prediction, and upstream replication are outside this contract. Changing
marked types or reflected wire shapes requires a new session.

Each zero limit selects the following finite default, never unlimited:

| `sync_limits_t` field | Default |
| --- | --- |
| `max_types` | 1024 synchronized types |
| `max_schema_bytes` | 32768 bytes |
| `max_frame_bytes` | 65536 bytes, including envelope |
| `max_update_bytes` | 16 MiB per complete snapshot or delta |
| `max_entities` | 100000 live mappings per session |
| `max_peers` | 64 peers, including faulted peers |
| `max_string_bytes` | 65535 bytes, excluding terminating NUL |
| `max_elements` | 4096 elements per fixed array |
| `max_depth` | 32 nested reflected types |
| `max_fragments` | 8192 frames per snapshot or delta |
| `max_pending_bytes` | 32 MiB of outbox bytes per peer |
| `max_receive_bytes` | 32 MiB of queued input |
| `max_memory_bytes` | 256 MiB of core-owned allocations |

These are independent maxima, not reservations. Schema bytes must fit the
manifest payload capacity of a frame. Pending and receive queue limits must
each accommodate a maximum-size frame. Logical assembly has its own bound,
so `max_update_bytes` need not fit `max_receive_bytes`.

The memory cap includes plans, indexes, buffer capacity, accounting, and
temporary values; shared allocations count once. It excludes Flecs/application
storage and adapter queues. Faulted peers still occupy resources until closed.
IDs and counters never wrap; exhaustion is a visible limit failure.

## Results, diagnostics, and recovery

Nonnegative results are distinct outcomes, not interchangeable success codes.

| Result | Meaning |
| --- | --- |
| `SYNC_OK` | Requested work succeeded. |
| `SYNC_EMPTY` | No available work or input. |
| `SYNC_NEED_MORE` | More input is needed before publication. |
| `SYNC_BACKPRESSURE` | Retry after draining; no input accepted. |
| `SYNC_END` | Client stream was finished and is drained. |
| `SYNC_PARTIAL` | Capture completed; one or more peers faulted. |

For `sync_apply`, `SYNC_NEED_MORE` can accompany consumed frames or waiting
assembly; it is not itself proof of progress. Negative results identify
argument, role, state, schema, unsupported type, protocol, sequence,
reference, limit, allocation, truncation, runtime, or lookup failures.
`SYNC_ERR_IO` and `SYNC_ERR_TRANSPORT` are reserved for adapters.

`sync_get_status(session, 0, &status)` copies session status. On a server,
pass a peer to inspect its sticky fault. Diagnostics can identify the type,
field, identity, tick, and sequence; human messages are not a parsing API.
Control mistakes such as stale tokens do not overwrite sticky stream faults.
Server aggregate `last_tick` is the last consumed tick, `last_sequence` is
zero, and `queued_bytes` sums outboxes. A server can remain ready while peers
are faulted; inspect `faulted_peers` and individual peers.

| Failure | Recovery |
| --- | --- |
| Invalid control call | Correct the call; existing progress remains. |
| Full client inbox | Apply, retain input, and retry the same frame. |
| Peer outbox or reference failure | Fix cause; close peer and open anew. |
| Global schema/tracking failure | Correct cause and recreate session. |
| Client stream or publication fault | Stop old input, reset, resnapshot. |

`sync_frame_inspect` validates only a complete frame's envelope. It performs
no allocation, schema or sequence checking, payload decoding, or configured
limit enforcement. Its success is not permission to trust the payload.

## EOF, reset, and destruction

After delivering all file records, drain the client and call
`sync_client_finish`. Pending queued frames return `SYNC_BACKPRESSURE`;
drain normally and retry. A missing initial snapshot or incomplete update
returns `SYNC_ERR_TRUNCATED`. Clean finish enters `SYNC_CLIENT_ENDED`, retains
replicas, and is idempotent. An empty, never-initialized stream is truncated.

For disconnect or recovery, stop the adapter and discard old input first.
At a writable boundary, `sync_client_reset` deletes owned replicas and their
local-only data, clears mappings, queues, and faults, and returns to
`SYNC_CLIENT_WAIT_MANIFEST`. It retains schema and scheduling. Old replica IDs
must not be reused by application code; a new stream needs a fresh snapshot.

Stop adapters before `sync_session_destroy`, then call `ecs_fini` on the world.
Server destruction closes peers and removes assigned network IDs while
preserving gameplay entities. Client destruction removes its replicas while
preserving unrelated entities. Session systems are removed; compatible
declarations and phase dependencies can remain. Destroying `NULL` is a no-op.

[design]: https://chatgpt.com/space/page_bc72b5d6c7b48191a202e24b430c5cb7
