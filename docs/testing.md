# Build and test Sync

[README](../README.md) · [Integration](usage.md) · [API](api.md)

This guide is for contributors testing the replication core. It assumes a
local checkout and the compiler, CMake, build tool, and Git prerequisites in
the README. The default workflow uses a Debug build and in-memory delivery.
ENet and FileSync are not required for the core suite.

## Configure, build, and run

Run from the Sync repository root. [CMake][cmake] creates `build/` and obtains
dependencies through [CPM][cpm]. First-time configuration needs network access
for the pinned CPM bootstrap and uncached dependencies. CTest comes with
CMake. The default Cucumber groups additionally require Ruby >=3.2,
RubyGems >=3.2.8 and Bundler 2.6.9. Install the frozen tooling first:

```sh
gem install bundler -v 2.6.9
BUNDLE_FROZEN=true bundle _2.6.9_ install
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The final command must discover tests and exit successfully with zero failed
tests. [CTest][ctest] prints failing test output with `--output-on-failure`.
To see the registered tests without running them, use:

```sh
ctest --test-dir build -N
```

After editing code, rebuild before rerunning CTest. For multi-configuration
generators, build with `--config Debug` and run CTest with `-C Debug`.
Keep separate build directories when changing compiler or generator.

For a library-only build, configure with `-DBUILD_TESTING=OFF` in a separate
directory. Skipping tests is useful for consumers but does not satisfy the
contributor validation requirements in [CONTRIBUTING.md](../CONTRIBUTING.md).

An optional CPM source cache avoids repeated dependency downloads. Configure
its location using `CPM_SOURCE_CACHE` as described in the CPM documentation.
Keep that cache outside the checkout if multiple builds share it. Removing
`build/` discards only that build's generated artifacts; keep any separately
configured shared cache unless you deliberately want to refetch dependencies.

## Core behavioral coverage

Use one server world and two independent client worlds. Deliberately vary
component registration order and local entity allocation. Register reflected
Position, Velocity, Health, a tag, and an entity-reference component. Include
public entities and entities private to each peer, plus unrelated client-local
entities that must survive replica cleanup.

Drive manual sessions with explicit ticks and an in-memory adapter. Assertions
compare synchronized values, tags, membership, and translated references per
recipient. Raw Flecs IDs need not match between worlds. Follow the project's
BDD process: express changed intended behavior in feature specifications and
automate the observable outcomes.

| Scenario | Required observation |
| --- | --- |
| Import without session | Declarations only; no capture or I/O. |
| Initial snapshot | Each client receives only its visible state. |
| Different registration order | Schema mapping ignores native type IDs. |
| Tracked writes | Set, modified, and writable iteration reach clients. |
| Dirty table, unchanged row | Extra assignments preserve correct state. |
| Two recipients | Shared query consumption loses neither peer's update. |
| Visibility entry without write | Full state arrives from a clean table. |
| Exit and reentry | Replica is rebuilt with the same network identity. |
| Add/remove/delete coalescing | Final sampled membership is correct. |
| Final synchronized type removed | Entity leaves the recipient view. |
| Reused Flecs entity slot | Replacement has a fresh network identity. |
| Forward references | Newly introduced targets resolve locally. |
| Hidden reference target | Only the affected peer faults; no exposure. |
| Snapshot followed by mutation | Ordered deltas leave no sampling gap. |
| Fragmented update | No incomplete update reaches downstream systems. |
| Small receive queue | Fragment consumption frees space before publish. |
| Backpressure | Zero bytes accepted; unchanged head retried in order. |
| Stalled recipient | Limits fault that peer; healthy peers continue. |
| Stale or foreign token | Commit fails without releasing another frame. |
| Borrowed head lifetime | Appends and other peers do not invalidate it. |
| Bad schema or sequence | Invalid logical update is not published. |
| Later update fails in apply | Earlier published updates remain applied. |
| Runtime publication failure | Client faults without a rollback claim. |
| Clean EOF | Ended client retains final replicas for inspection. |
| Empty or incomplete EOF | Finish reports truncation. |
| Reset after disconnect | Owned replicas vanish; unrelated entities stay. |
| Session destruction | Systems and storage released before world fini. |

Also exercise invalid role/state calls, zero and non-increasing ticks, finite
limit normalization, and unsupported reflected types. Test null versus empty
strings, nested structs, arrays, tags, enum bits, and entity references.
Validate lifecycle operations for temporary nontrivial values, including
cleanup on decode failure. Recoverable allocation failures return diagnostics;
tests must not assume fatal Flecs aborts become recoverable results.

Automatic-mode tests use explicit simulation deltas with `ecs_progress`, not
sleeps. Check phase ordering, manual-operation rejection, default rate and
budget, at-most-one capture after a stall, and accumulator reset on rate
changes. Check that cyclic phase setup fails and that custom pipelines must
include the Sync phases. These tests complement the manual foundation.

## Reproducible fixtures

Use fixed nonzero session identities only for isolated fixtures. Fix schema,
options, ticks, peer-open order, runtime entity creation order, and capture
history. These inputs affect wire IDs and canonical output. Equivalent final
world state with different dirty-table histories need not yield identical
delta bytes.

Byte comparisons supplement semantic assertions; they do not replace them.
Use `sync_frame_inspect` for envelope diagnostics, and use receive/apply to
validate actual stream behavior. Do not cast bytes to public C structs or
assume enum values are wire discriminants. Where applicable, test
interoperability with the previous release as required by CONTRIBUTING.md.

## Companion adapter verification

The [architecture](architecture.md#adapter-obligations) defines the shared
frame contract. FileSync's integration and fault tests are registered in this
repository when `SYNC_BUILD_FILE_SYNC=ON` (the default). Run just those tests
with `ctest --test-dir build -R 'file_sync\.' --output-on-failure`. Configure
a separate build with both adapter options `OFF` for core independence.
EnetSync is also registered here when `SYNC_BUILD_ENET_SYNC=ON` (the default).
Disable both adapters to verify a core-only build. Run EnetSync's native
contracts with `ctest --test-dir build -L enet_sync -LE loopback` and its
production loopback cases with `ctest --test-dir build -L loopback`.
UDP socket creation is required for loopback. Sandbox denial remains a failed
environment check; the required tests are not skipped. See the
[EnetSync guide](enet-sync.md#verification) for build and lifecycle details.

FileSync tests record a fresh virtual peer, check flush/close, replay through
the normal client path, and compare visible state. Cover partial writes,
checksum failures, partial records, logical truncation at record-boundary EOF,
backpressure, and separate frame/update stepping. File-only tests run without
linking ENet. They also cover automatic Receive/Send phase ordering, service
budgets, path replacement, copied options, peer/session ownership and cleanup.
Private test-only wrappers inject allocation, read, short-write, flush and
close failures without affecting fixture I/O. C++17 inclusion/linking checks
are enabled for both libraries with `SYNC_CHECK_CXX=ON`.

EnetSync loopback tests cover connection lifecycle, reliable ordered framing,
send acceptance, receive ownership, independent service frequency, bounded
transport backlog, disconnect, and fresh-snapshot reconnection. Core or file
tests alone do not establish that network servicing works.

## Running the RPG tests

The [RPG Gherkin features](../features/rpg/README.md) run through Cucumber
and retain their stable scenario IDs and outline examples. Native C tests
remain individually runnable through CTest and exercise the same public APIs.
Their [model and capability index](file-sync.md#rpg-behavior-specifications)
describe the players, NPCs, private inventory entities, and replay assertions.
Build with `BUILD_TESTING=ON` and `SYNC_BUILD_FILE_SYNC=ON`, then run:

```sh
ctest --test-dir build -R 'file_sync\.rpg_' --output-on-failure
ctest --test-dir build -R '^file_sync\.rpg_game_001$' -V
```

CTest registers 58 RPG cases covering all 38 scenario IDs, with a separate
case for each Scenario Outline example. For example, `@RPG-GAME-001` maps to
`file_sync.rpg_game_001`; `@RPG-GAME-009` has separate `_null` and `_empty`
cases. The [case registry](../adapters/file_sync/tests/rpg/cases.def) maps each
case name and example value to its C test function. Keep expectations in the
features and compiled tests consistent when changing behavior.

Each case gets new worlds and files, with fixture teardown after success or
failure. CTest assigns a separate working directory to every case, so parallel
runs do not share recordings. Tests create and remove `Aria.bin`, `Bram.bin`,
and `GM.bin` in those directories.

`file_sync_rpg_tests` runs ordinary cases. `file_sync_rpg_fault_tests` also
links the existing test-only I/O/allocation controls for `failures.feature`.
These controls never alter fixture file operations or the production library.
Both executables accept exactly one case name and reject unknown selections.
Use CTest for individual cases to get the isolated working directory shown
above. All tests use simulation deltas or explicit capture ticks; no sleeps
or wall-clock timing are involved.

## Procedural Cucumber groups

`SYNC_CUCUMBER` defaults to `ON`. Ruby and Bundler discovery happens only
when both tests and Cucumber are enabled. Library-only and
`-DSYNC_CUCUMBER=OFF` configurations need no Ruby tools. Missing tools or a
lockfile/version mismatch fail the registered groups with installation
instructions; they never turn those groups into passing or skipped tests.

Features live beneath `features/core`, `features/file_sync`, `features/rpg`
and `features/enet_sync`. Filter the five groups through CTest:

```sh
ctest --test-dir build -R '^sync\.cucumber_core$' -V
ctest --test-dir build -R '^sync\.cucumber_file_sync$' -V
ctest --test-dir build -R '^sync\.cucumber_rpg$' -V
ctest --test-dir build -R '^sync\.cucumber_enet_sync$' -V
ctest --test-dir build -R '^sync\.cucumber_loopback$' -V
ctest --test-dir build -L cucumber --output-on-failure
```

Core, FileSync, RPG and EnetSync each have a persistent private C fixture.
A workflow selects its operation scope; it performs no complete native test
on selection. Each feature operation requests one statement or bounded loop
of fixture setup, API calls or observations. Replies include actual numeric
values and API results. Operation descriptions are bound to their commands
by a checksum, so changing an action without updating its driver fails.
State queries repeat observations without advancing the workflow. Protocol
checks cover malformed commands, ordering, state retention and teardown.
All fixture interfaces and failure controls are private to testing.

Ruby starts one fixture process per scenario, drains diagnostics separately
from protocol stdout, and gives each command and cleanup five seconds.
Every scenario gets an isolated temporary recording directory, removed even
when an assertion fails. Crashes, undefined steps, mismatched observations
and cleanup failures propagate as test failures. Group timeouts are 120
seconds. Frozen Bundler execution uses the checked-in Cucumber 11.1.1 lock.

All 33 EnetSync scenarios run against deterministic transport. The eight
`@loopback` workflows additionally run against production ENet and require
working UDP sockets. With FileSync disabled, only `@file_sync` recording
integration scenarios are excluded. FileSync and RPG groups follow the
FileSync build option; both ENet groups follow the EnetSync build option.

CPM 0.43.1 uses the same bootstrap and SHA-256 pin as ENet. `CPM_PATH`
selects a directory containing `CPM.cmake`; `CPM_SOURCE_CACHE` selects a
shared cache, including `cpm/CPM_0.43.1.cmake`. Environment variants of both
variables are supported. The bootstrap verifies the file against the release
URL; a matching cached file avoids refetching it. Package-source overrides
such as `CPM_flecs_SOURCE` and `CPM_enet_SOURCE` avoid dependency retrieval.
A first uncached configuration requires network access to GitHub.

## Check documentation examples

This optional check verifies declarations and caller syntax without linking.
Use a C11 compiler, a C++17 compiler for the header check, and the real
[Flecs 4.1.1 distribution header][flecs-header]. Do not substitute dependency
stubs. Put `flecs.h` in a temporary include directory and save the complete
C block from [Integration](usage.md) as `sync_example.c` there.

From the repository root, set `FLECS_INCLUDE_DIR` to that directory's absolute
path. Both commands should exit zero without diagnostics:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsyntax-only \
	-I"$FLECS_INCLUDE_DIR" -Iinclude "$FLECS_INCLUDE_DIR/sync_example.c"
c++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only \
	-I"$FLECS_INCLUDE_DIR" -Iinclude -x c++ include/sync.h
```

Syntax checking does not establish linking, runtime behavior, observer safety,
memory recovery, or performance. Run the behavioral suite for those runtime
contracts; measure performance separately. Delete the temporary header and
example when finished, without removing shared dependency caches.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| Compiler not found | Install a toolchain; configure a new build directory. |
| Dependency download fails | Check network access or populate the CPM cache. |
| Wrong generator/compiler cache | Start a separate clean build directory. |
| No tests discovered | Configure with `BUILD_TESTING=ON`, then rebuild. |
| Test executable missing | Build first; match the selected configuration. |
| Missing reflection or phases | Restore required Flecs addon configuration. |
| Schema mismatch | Match qualified names, marked types, and wire shapes. |
| Client never publishes | Check capture, policy, delivery, and apply. |
| Backpressure repeats | Drain input; inspect queue and assembly limits. |
| Peer faults after policy change | Check reference closure in its new view. |

For stream failures, inspect return codes and `sync_get_status`, including
individual peers after `SYNC_PARTIAL`. Keep the offending tick, sequence, and
field diagnostic. Correct the cause before resetting or reopening a stream;
never skip a dependent frame to make a failing test continue.

[cmake]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[ctest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[cpm]: https://github.com/cpm-cmake/CPM.cmake
[flecs-header]:
https://github.com/SanderMertens/flecs/blob/v4.1.1/distr/flecs.h
