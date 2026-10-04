# Sync

Sync is a C11 Flecs module for authoritative server-to-client state
replication. It captures reflected components and tags, selects the entities
each recipient can see, and publishes complete updates into client worlds.
Applications supply ordinary Flecs reflection and recipient policy; they do
not need a network encoder for each component.

This repository contains the replication core and the optional FileSync and
EnetSync adapters. Sync owns schema validation, network identities,
change collection, serialization, queues, and client application. The core
opens no sockets or files. FileSync delivers its frames through recording
files; EnetSync delivers them through dedicated ENet hosts. An in-memory
adapter exercises the same core without either adapter.

The guides assume familiarity with C and Flecs entities, components, and
reflection. Start with the [integration example](docs/usage.md) to replicate
a component between two independent worlds.

## Build and test

For a local command-line build, install these prerequisites:

- A C11 compiler and a working platform development toolchain.
- CMake 3.20 or newer, including CTest.
- Make or Ninja, and Git for dependency retrieval.
- Network access for the CPM bootstrap and uncached dependency sources.
- For Cucumber tests: Ruby >=3.2, RubyGems >=3.2.8 and Bundler 2.6.9.

The API baseline is Flecs 4.1.1. CMake obtains dependencies through
[CPM][cpm]; no system Flecs installation or ENet installation is required.
Keep Flecs module, Meta, system, and pipeline support enabled. A C++17
compiler is needed only for the optional header inclusion check.

Run these commands from the directory containing this README. Configuration
creates an out-of-source build in `build/`; the next commands compile it and
run its registered tests. See the [CMake CLI][cmake] and [CTest CLI][ctest]
for the command options.

```sh
gem install bundler -v 2.6.9
BUNDLE_FROZEN=true bundle _2.6.9_ install
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

A successful run discovers tests, exits with status zero, and reports no
failed tests. A run reporting no tests does not validate Sync. The
[testing guide](docs/testing.md) explains the behavioral coverage and how to
diagnose build or test failures.

FileSync builds by default as the separate `FileSync::file_sync` target.
EnetSync builds by default as `EnetSync::enet_sync`. Configure with
`-DSYNC_BUILD_FILE_SYNC=OFF -DSYNC_BUILD_ENET_SYNC=OFF` for only the core.
The Flecs ENet wrapper and its ENet dependency are fetched only when the
adapter is enabled. Sync shares the wrapper’s Flecs target. See
[EnetSync](docs/enet-sync.md) for connection lifetimes, scheduling, pressure,
recording callbacks, and the required socket-capable loopback checks.
See [FileSync](docs/file-sync.md) for recording, replay and inspection.
Its [RPG behavior specifications](docs/file-sync.md#rpg-behavior-specifications)
demonstrate player and GM views, NPC secrets, and entity-based inventories.

Cucumber 11.1.1 is locked in the test-only `Gemfile.lock`.
`SYNC_CUCUMBER=ON` is the default when tests are built; missing tooling
leaves failing tests registered with installation diagnostics. Use
`-DSYNC_CUCUMBER=OFF` to run only the native tests. Feature files live under
[`features/`](features), grouped by core, FileSync, RPG and EnetSync.

The CPM 0.43.1 bootstrap uses ENet's SHA-256 pin. Use `CPM_PATH` for a
directory containing the pinned `CPM.cmake`, or `CPM_SOURCE_CACHE` for a
shared cache. The bootstrap checks the pinned file against the release URL;
a matching cached file can avoid downloading it. Standard
`CPM_flecs_SOURCE`, `CPM_flecs_enet_SOURCE` and `CPM_enet_SOURCE` overrides select local sources.

To build libraries without tests, configure a separate directory with
`-DBUILD_TESTING=OFF`, then build that directory. This skips the test suite.
With a multi-configuration generator, pass `--config Debug` to the build
command and `-C Debug` to CTest instead of selecting `CMAKE_BUILD_TYPE`.

## Use Sync

1. Create a Flecs world and call `sync_import` to obtain world-local IDs.
2. Register component reflection and mark synchronized types with `ids.sync`.
3. Create one server or client session for that world.
4. Open server peers and mark runtime entities for their recipients.
5. Capture after writes merge, deliver queued frames, and apply on clients.
6. Stop adapters, destroy sessions, and then finalize the worlds.

Include `sync.h` from `include/` in consumers and link Sync together with its
Flecs dependency. The public interface is C and is includable from C++17.
Manual sessions are the default. Automatic sessions schedule the same core
operations through Flecs phases.

Replication carries sampled state. It does not deliver every intermediate
mutation as a gameplay event. Visibility is per entity; the initial contract
excludes prediction, upstream replication, arbitrary gameplay pairs,
inherited values, and runtime schema changes.

## Documentation

| Guide | Purpose |
| --- | --- |
| [Integration](docs/usage.md) | Register types and exchange frames safely. |
| [FileSync](docs/file-sync.md) | Record, replay and inspect Sync streams. |
| [EnetSync](docs/enet-sync.md) | Deliver Sync frames through dedicated hosts. |
| [API guide](docs/api.md) | Lifetimes, defaults, scheduling, and recovery. |
| [Architecture](docs/architecture.md) | Capture, schema, and adapters. |
| [Rationale](docs/rationale.md) | Design decisions and their tradeoffs. |
| [Testing](docs/testing.md) | Build workflow and behavioral acceptance. |
| [Public header](include/sync.h) | Exact declarations and call contracts. |

The public header is authoritative for parameters, results, and ownership.
Read [CONTRIBUTING.md](CONTRIBUTING.md) before changing the project. Behavioral
changes belong in the feature specifications and tests as well as the API
documentation.

[cmake]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[ctest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[cpm]: https://github.com/cpm-cmake/CPM.cmake
