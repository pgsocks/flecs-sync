# Record and replay with FileSync

[README](../README.md) · [Sync integration](usage.md) · [Testing](testing.md)

FileSync is an optional C11 library in this repository. Include `file_sync.h`
and link `FileSync::file_sync` in a CMake consumer. The target includes its
public headers and links Sync transitively. It opens binary files by filename,
records unchanged Sync frames, and replays them through the ordinary client
receive and apply operations. No network adapter or ENet library is required.

`SYNC_BUILD_FILE_SYNC` defaults to `ON`; configure it `OFF` to omit FileSync.
Set `SYNC_BUILD_ENET_SYNC=OFF` too for only the core. All libraries share the
repository's release and test workflow.
`SYNC_CHECK_CXX=ON` checks both public interfaces from C++17.

## Calling and ownership

All handles use explicit free functions, copied descriptors and diagnostics,
and the owning thread. Bound sessions and worlds must outlive their file
handles. Open, close and update stepping require writable world boundaries.
Close adapters before destroying sessions, then finalize worlds. Do not reset
or destroy a client, or externally close a recording peer, while it is bound.

Opening a writer replaces an existing file. Opening a reader never changes
its file. Neither function retains the pathname or takes ownership of the
session. A writer bound to a server owns a new peer; use its status to obtain
the peer for recipient policy. Closing that writer removes its peer and
recipient pairs while preserving gameplay entities.

Options are zero-initializable. The default is manual service. Frame limits
inherit the bound session's normalized limit; raw handles default to 65,536
bytes. Explicit limits cannot exceed their bound session's limit. Automatic
service defaults to 64 frames per progression. Descriptors are copied at open.

A successful write or outbox commit means a complete record reached the C
stream's buffers. `file_sync_writer_flush` checks `fflush`; neither that
operation nor close promises persistence after power loss. Flush does not
capture state or drain the server outbox.

Close removes systems and releases the handle even if it returns an error.
A healthy bound writer drains the remaining outbox before flush and close.
A faulted writer never retries a partially written record. Reader close does
not finish/reset its client or delete any published replicas. Stop or abandon
the old reader, then reset the client before attaching another stream.

## Record one sampled update

First import Sync, register reflected types and create a server, following the
integration guide. Call this function after game writes merge. Pass a tick
greater than every tick already consumed by that server. The selected entity
must own synchronized types and its references must obey recipient visibility.
Other entities marked for all recipients also participate in this recording.

```c
#include <file_sync.h>

sync_result_t record_sample(ecs_world_t *world, sync_session_t *server,
	const sync_ids_t *ids, ecs_entity_t entity, uint64_t tick,
	const char *path)
{
	file_sync_writer_t *writer = NULL;
	file_sync_writer_desc_t desc = { .server = server };
	sync_result_t result = file_sync_writer_open(path, &desc, &writer,
		NULL);
	if (result != SYNC_OK)
		return result;
	file_sync_status_t status;
	result = file_sync_writer_get_status(writer, &status);
	if (result == SYNC_OK) {
		ecs_add_pair(world, entity, ids->replicate_to, status.peer);
		result = sync_capture(server, tick, NULL);
	}
	if (result == SYNC_OK)
		result = file_sync_writer_pump(writer, UINT32_MAX, NULL);
	if (result == SYNC_OK)
		result = file_sync_writer_flush(writer);
	sync_result_t closed = file_sync_writer_close(writer, NULL);
	return result == SYNC_OK ? closed : result;
}
```

For a continuous recording, keep the writer open, repeat capture and pump
with increasing ticks, and close when recording stops. Capture remains the
server application's responsibility. A fresh peer ensures the file begins
with its manifest and complete initial snapshot, even when recording starts
partway through an existing server session.

## Replay complete updates

Create a compatible manual client first. The client must be fresh, with no
queued input, assembly or replicas. This helper replays until clean EOF and
retains the final client state. Files carry sampled state; they do not rerun
unsynchronized simulation or reproduce every intermediate gameplay event.

```c
#include <file_sync.h>

sync_result_t replay_file(sync_session_t *client, const char *path)
{
	file_sync_reader_t *reader = NULL;
	file_sync_reader_desc_t desc = { .client = client };
	sync_result_t result = file_sync_reader_open(path, &desc, &reader,
		NULL);
	if (result != SYNC_OK)
		return result;
	do {
		file_sync_progress_t progress;
		result = file_sync_reader_step(reader, 1, &progress);
		/* progress.application identifies completed updates and
		 * ticks.
		 */
	} while (result == SYNC_OK);
	sync_result_t closed = file_sync_reader_close(reader, NULL);
	return result == SYNC_END ? closed : result;
}
```

Stepping by one update can read multiple frames. Schema, snapshot chunks and
fragments do not count as published updates. Step first drains queued client
input, then alternates reception and application until its update budget or
EOF. It rejects automatic clients before reading or publishing anything.

`file_sync_reader_pump` instead feeds a bounded number of frames without
application. It works with manual or automatic clients. Backpressure retains
one verified record unchanged, with no reading of subsequent records. Drain
through normal application and retry. Physical EOF becomes pending while
queued input remains; the next pump checks `sync_client_finish` again. Only
successful finish reports a clean replay end. Empty recordings and unfinished
snapshots or updates report truncation, including at record-boundary EOF.

## Inspect raw frames

An unbound reader verifies file records without interacting with a world.
Repeated peeks return the same immutable frame until consume or close. Copy
its bytes if they need to survive either operation. A raw reader's `SYNC_END`
means physical EOF and does not validate logical completeness.

```c
#include <file_sync.h>

sync_result_t inspect_file(const char *path, uint64_t *out_frames)
{
	*out_frames = 0;
	file_sync_reader_t *reader = NULL;
	sync_result_t result = file_sync_reader_open(path, NULL, &reader, NULL);
	if (result != SYNC_OK)
		return result;
	for (;;) {
		file_sync_frame_t frame;
		result = file_sync_reader_peek(reader, &frame);
		if (result != SYNC_OK)
			break;
		/* frame.info contains kind, stream, sequence and sampled
		 * tick.
		 */
		result = file_sync_reader_consume(reader);
		if (result != SYNC_OK)
			break;
		(*out_frames)++;
	}
	sync_result_t closed = file_sync_reader_close(reader, NULL);
	return result == SYNC_END ? closed : result;
}
```

## Record from a network consumer

Only one adapter consumes a peer outbox. To record the same stream, open an
unbound manual writer and pass every complete frame to `file_sync_writer_write`
from the existing consumer before it commits that frame. Begin with a new
stream's manifest, including every frame in order. Starting from a delta does
not produce a standalone recording.

```c
#include <file_sync.h>

sync_result_t record_delivery(file_sync_writer_t *writer,
	const sync_frame_view_t *frame)
{
	return file_sync_writer_write(writer, frame->data, frame->size);
}
```

The caller still controls network acceptance and outbox commit. If recording
fails, stop that recording; do not retry a possibly partially written record.
The network consumer may continue independently according to application
policy. FileSync write never retains the borrowed frame pointer.

## Automatic service

Set `schedule = SYNC_AUTOMATIC` in a bound writer descriptor to install one
system in `sync.Send`. Its server may capture manually or automatically.
Automatic file service never captures or changes the server's rate. Set that
schedule on a bound reader with an automatic client to install a system in
`sync.Receive`. The existing core system in `sync.Apply` publishes updates
before readers in `EcsPreUpdate`. EOF finishing is retried in later progressions
until queued input drains. Custom pipelines must select these phase systems.

Manual transfer and inspection calls reject automatic handles. Status, writer
flush and close remain available on the owning thread. File I/O is synchronous
and may block that thread. The frame budget limits records, not wall time or
logical update size. Playback timing is controlled by application progression;
there is no implicit conversion from sampled ticks to seconds.

## Format and failure diagnostics

Version 1 has a 16-byte header: eight-byte `FILESYNC` magic, little-endian
32-bit version `1`, and four reserved zero bytes. Each record is a little-endian
32-bit length, the unchanged complete Sync frame, and a little-endian IEEE
CRC-32 of the encoded length plus frame bytes. CRC initialization and final
XOR are `0xffffffff`; its reflected polynomial is `0xedb88320`. The container
version is independent of the embedded Sync protocol version.

Lengths are checked before allocation. Checksum and envelope verification
precede delivery. Every file contains one session/stream identity with exact
contiguous sequences beginning at one. Sync validates the manifest and
logical payloads during application. There is no append mode, seek index,
compression, or footer. CRC detects accidental corruption; it is not an
authentication mechanism.

Use the writer/reader status getters to inspect sticky faults, effective
limits, transfer counters, pending input, and the recording peer. Diagnostic
copies include a Sync result/context/message, record number, file byte offset,
and OS error where available. Earlier publications remain after a later
failure. File errors do not inject a fictitious Sync fault; inspect core
status independently and explicitly reset a client when abandoning its stream.

## RPG behavior specifications

The [RPG features][rpg-features] demonstrate FileSync through 38 tagged
scenarios in five feature files, covering 58 cases after expanding Examples.
Cucumber drives the expected behavior and stable IDs through persistent
private C fixtures against real Flecs worlds and the public Sync/FileSync
APIs. Every Examples row executes, and inventory tables query replayed state.
Native C tests remain separately registered for every case. Cucumber 11.1.1
requires Ruby >=3.2, RubyGems >=3.2.8 and Bundler 2.6.9; use
`SYNC_CUCUMBER=OFF` to run only the native tests. Fixture teardown also runs
after a failed assertion.

Aria and Bram have independent player clients. Mira the merchant and Skulk
the goblin are NPCs. A third client represents the GM's privileged view.
Every view has its own recording and replay world:

| Character | Initial health / mana | Initial private inventory |
| --- | --- | --- |
| Aria | 100 / 30 | 5 HealingPotion, 20 Arrow |
| Bram | 120 / 10 | 3 HealingPotion, 1 IronSword |
| Mira | 40 / 0 | 12 HealingPotion, 40 Arrow |
| Skulk | 35 / 0 | 1 RustyDagger, 7 GoldCoin |

Public actor entities carry individual stats, nested positions, a fixed
attribute array, labels, and combat tags. Separate private detail entities
carry an owner reference, gold, experience, and an owned nullable quest string.
Each inventory is another Flecs entity with an owner reference, eight fixed
stack-reference slots, and a nullable equipped-stack reference. Every stack
is a Flecs entity with quantity, a public item-definition reference, and an
inventory backlink. These cycles exercise allocation before reference
resolution; trades also exercise clearing old slots before removing replicas.

Player details, inventories, and stacks are visible to their owner and the
GM. NPC private entities initially belong only to the GM's view. Revealing an
NPC inventory adds that inventory and its stacks to a selected player view;
the NPC's quest stays private. Public actors never reference hidden private
entities. Local type registration order and entity IDs differ across worlds,
and assertions check reference translation, quantities, visibility, and
network identity through actual replay.

Permissions select the recording peer's stream during capture. A GM file
contains the GM's private view: replay preserves that recorded view. Sharing
that file therefore shares those private details.

The following index connects adapter capabilities to tagged scenarios.
The [feature guide][rpg-features] explains their compiled test names.

| Capability | Scenario |
| --- | --- |
| Bound recording and visibility | [Public and private views][rpg-1] |
| Fresh snapshot after late start | [Mid-adventure recording][rpg-2] |
| Raw writer and byte preservation | [Exact delivered stream][rpg-3] |
| Borrowed peek and consume | [Raw inspection][rpg-4] |
| Copied options and pure getters | [Options and status][rpg-5] |
| Flush without capture or drainage | [Flush semantics][rpg-6] |
| Writer close and peer ownership | [Drain and release][rpg-7] |
| Replacement files | [Replace a file][rpg-8] |
| Translated and nullable references | [Equipment][rpg-9] |
| Visibility changes and identity | [Reveal and revoke][rpg-10] |
| Sampled state and tags | [Combat][rpg-11] |
| Owned null and empty strings | [Quest notes][rpg-12] |
| Complete-update budgets | [Snapshots and deltas][rpg-13] |
| Frame budgets without publication | [Frame pumping][rpg-14] |
| Retained pending frame and retry | [Backpressure][rpg-15] |
| Fragment assembly | [Fragmented quest][rpg-16] |
| Pending EOF and clean finish | [EOF drainage][rpg-17] |
| Reader ownership and client reset | [Close and reset][rpg-18] |
| Receive / Apply / Capture / Send | [Phase ordering][rpg-19] |
| Automatic budgets and manual core | [Automatic recording budget][rpg-20] |
| Automatic replay frame budgets | [Automatic replay budget][rpg-33] |
| Manual input to automatic client | [Automatic application][rpg-21] |
| Mode rejection and system cleanup | [Automatic handles][rpg-22] |
| Header, length and CRC validation | [Invalid records][rpg-23] |
| Logical truncation | [Unfinished updates][rpg-24] |
| Identity and sequence validation | [Stream ordering][rpg-25] |
| Preserved earlier publications | [Later failure][rpg-26] |
| Core schema diagnostics | [Incompatible stats][rpg-27] |
| Per-peer reference faults | [Hidden referenced stack][rpg-28] |
| Commit after complete write | [Partial writes][rpg-29] |
| Sticky I/O/allocation diagnostics | [Operation faults][rpg-30] |
| Failed-open ownership | [Open failures][rpg-31] |
| Arguments, roles and limits | [Invalid calls][rpg-32] |

Inventory trades, consumption, splitting, merging, looting, and whole-stack
transfers are additional gameplay examples in [gameplay.feature][gameplay].
The existing lower-level adapter tests remain in place.
See [Running the RPG tests](testing.md#running-the-rpg-tests) for
commands and individual case selection.

[rpg-features]: ../features/rpg/README.md
[recording]: ../features/rpg/recording.feature
[gameplay]: ../features/rpg/gameplay.feature
[replay]: ../features/rpg/replay.feature
[automatic]: ../features/rpg/automatic.feature
[failures]: ../features/rpg/failures.feature

[rpg-1]: ../features/rpg/recording.feature#L5
[rpg-2]: ../features/rpg/recording.feature#L126
[rpg-3]: ../features/rpg/recording.feature#L199
[rpg-4]: ../features/rpg/recording.feature#L248
[rpg-5]: ../features/rpg/recording.feature#L291
[rpg-6]: ../features/rpg/recording.feature#L322
[rpg-7]: ../features/rpg/recording.feature#L370
[rpg-8]: ../features/rpg/recording.feature#L419
[rpg-9]: ../features/rpg/gameplay.feature#L319
[rpg-10]: ../features/rpg/gameplay.feature#L457
[rpg-11]: ../features/rpg/gameplay.feature#L768
[rpg-12]: ../features/rpg/gameplay.feature#L840
[rpg-13]: ../features/rpg/replay.feature#L5
[rpg-14]: ../features/rpg/replay.feature#L77
[rpg-15]: ../features/rpg/replay.feature#L150
[rpg-16]: ../features/rpg/replay.feature#L292
[rpg-17]: ../features/rpg/replay.feature#L375
[rpg-18]: ../features/rpg/replay.feature#L445
[rpg-19]: ../features/rpg/automatic.feature#L5
[rpg-20]: ../features/rpg/automatic.feature#L98
[rpg-21]: ../features/rpg/automatic.feature#L177
[rpg-22]: ../features/rpg/automatic.feature#L372
[rpg-23]: ../features/rpg/failures.feature#L5
[rpg-24]: ../features/rpg/failures.feature#L97
[rpg-25]: ../features/rpg/failures.feature#L154
[rpg-26]: ../features/rpg/failures.feature#L206
[rpg-27]: ../features/rpg/failures.feature#L262
[rpg-28]: ../features/rpg/failures.feature#L306
[rpg-29]: ../features/rpg/failures.feature#L434
[rpg-30]: ../features/rpg/failures.feature#L522
[rpg-31]: ../features/rpg/failures.feature#L617
[rpg-32]: ../features/rpg/failures.feature#L648
[rpg-33]: ../features/rpg/automatic.feature#L273
