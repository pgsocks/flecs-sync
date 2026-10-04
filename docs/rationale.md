# API rationale

[README](../README.md) · [Architecture](architecture.md) · [API](api.md)

This guide explains the choices behind Sync for contributors and application
authors. It adapts the [design philosophy][philosophy], [module design][design],
and [API design][api-design]. The [public header](../include/sync.h) defines
the exact contract. Design tradeoffs explain that contract; they do not add
compatibility or performance guarantees.

## Separate replication from delivery

Sync knows schemas, entities, visibility, and state transitions. Adapters know
connections or files. Keeping these decisions separate lets a memory or file
test exercise the same replication behavior as the networked application.
Opaque sessions hide indexes, journals, queues, and codec plans from callers.

The cost is owned intermediate storage and bookkeeping. A transport-specific
implementation could avoid some copying, but would couple tests and gameplay
integration to that transport. This separation follows the information-hiding
principle discussed in [Parnas's modularity paper][parnas].

## Put a complete manual interface beneath convenience

The foundation is explicit: create a session, open a peer, capture, transfer
frames, apply, and clean up. Automatic scheduling invokes the same behavior.
It does not replace the schema, ownership, or failure model.

Manual mode defaults to explicit ticks and writable boundaries, making tests
independent of clocks, sockets, and sleeps. The cost is more application
coordination. Automatic mode reduces that work and rejects manual capture or
apply calls that would advance its sessions twice. SQLite's
[convenience wrappers][sqlite] provide a useful comparison for this layering;
the project's "iceberg API" term describes this foundation-first approach.

## Keep game policy in ECS data

Ordinary tags and nonexclusive relationships express synchronized types and
recipient membership. Teams, ownership, distance, and permissions remain
application policies. Component libraries can stay independent of Sync;
application setup adds reflection and synchronization annotations.

World-local ID tables support independently registered worlds without public
mutable module globals. ID-based calls are more verbose than convenient
global component macros, but make the world association explicit.

## Separate identity and representation

Remote identity must survive differences in local entity allocation and
component registration order. A persistent mapping lets clients hold local
entities alongside replicas. [EnTT's continuous loader][entt] is a precedent
for keeping remote and local entity identities distinct.

Reflection describes type structure; it does not define a portable memory
dump. Canonical schema shapes and field encoding avoid dependence on native
padding and pointers. Frozen schemas allow startup validation to remain useful
for the session lifetime. Their cost is an explicit supported-type boundary
and session recreation when schema changes are needed.

## Promise sampled correctness

Flecs change detection can identify dirty table columns without identifying
exactly which row values changed. Sync permits extra assignments and shares
the capture results across recipients. This avoids requiring per-component
network callbacks or exact prior-value comparisons.

Visibility is reconciled separately: a clean value table can still contain
an entity newly visible to a peer. Likewise, references must remain valid
when policy changes without writes. These obligations require membership and
reference bookkeeping even when no gameplay value changes.

Capture ticks identify samples, not individual events. After a stalled frame,
automatic mode captures the current state once; it cannot reconstruct samples
that were never retained. An apply budget limits published updates, not CPU
time: one large update can still be expensive.

## Make ownership and failure observable

Retaining a complete update advances the queued baseline. Accepting a frame
moves delivery responsibility to an adapter. Neither proves that the remote
client applied it. The [end-to-end argument][end-to-end] helps explain why
lower-layer delivery acknowledgement cannot establish application completion.

Dependent deltas cannot be dropped arbitrarily. Finite queue limits make a
stalled stream fail visibly; peer-local failures let other recipients
continue. Client backpressure accepts no bytes and permits retry of the same
frame. Recovery uses a fresh snapshot after the underlying condition is fixed.

Borrowed outbox views, copied receive buffers, and copied status results make
lifetime rules explicit. Reset deletes replicas; clean EOF preserves them for
inspection. Prevalidation protects against publishing malformed updates, but
does not promise rollback after observers or runtime mutations have run.

## Revisit decisions with evidence

One session per world, frozen schemas, reliable ordered deltas, and a generic
codec keep the initial contract focused. Independent replication domains,
live schema evolution, loss-tolerant delivery, or measured codec bottlenecks
could justify extensions. Those extensions need their own explicit contracts.

The default rate and limits are starting configurations, not measured optimal
values. Semantic tests establish behavior; benchmarks must separately measure
memory, latency, throughput, and useful capacity for the intended workload.

[philosophy]: https://chatgpt.com/space/page_562b542bf9c48191988c913f90472b3b
[design]: https://chatgpt.com/space/page_bff89c33d07c8191bfecb3c681db1405
[api-design]: https://chatgpt.com/space/page_bc72b5d6c7b48191a202e24b430c5cb7
[parnas]: https://doi.org/10.1145/361598.361623
[sqlite]: https://www.sqlite.org/cintro.html
[entt]: https://github.com/skypjack/entt/wiki/Entity-Component-System
[end-to-end]: https://web.mit.edu/saltzer/www/publications/endtoend/endtoend.pdf
