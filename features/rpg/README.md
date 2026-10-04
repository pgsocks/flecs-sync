# RPG behavior specifications

These executable Cucumber Gherkin files describe FileSync through an RPG with
independent Aria, Bram, and GM clients, NPC secrets, and inventories whose
item stacks are Flecs entities. Each scenario has a stable ID; retain that ID
when renaming or clarifying the scenario.

The Cucumber runner drives persistent private C fixtures through individual
setup, action and observation commands. Native C acceptance tests remain
available through CTest. Both use real public Sync/FileSync APIs and compare
each client's recorded and replayed state with the specified outcome.

| Feature | Scenario IDs | C test source | Cases |
| --- | --- | --- | --- |
| [Recording][record] | `RPG-RECORD-001`–`008` | [C tests][c-record] | 8 |
| [Gameplay][game] | `RPG-GAME-001`–`009` | [C tests][c-game] | 10 |
| [Replay][replay] | `RPG-REPLAY-001`–`006` | [C tests][c-replay] | 6 |
| [Automatic service][auto] | `RPG-AUTO-001`–`005` | [C tests][c-auto] | 5 |
| [Failures][fail] | `RPG-FAIL-001`–`010` | [C tests][c-fail] | 29 |

CTest names begin with `file_sync.` followed by the lowercase scenario ID
with underscores: `@RPG-GAME-001` becomes `file_sync.rpg_game_001`. Scenario
Outline examples add a suffix; the null and empty quest notes are
`file_sync.rpg_game_009_null` and `file_sync.rpg_game_009_empty`. Each row of
[the case registry](../../adapters/file_sync/tests/rpg/cases.def) identifies its C test function
and example input. There are 38 scenario IDs and 58 individually runnable
cases. Keep every scenario and example covered when editing the suite.

Every case creates fresh server/client worlds and isolated recording files.
All cases verify cleanup, including lifecycle hooks for owned strings and
survival of unrelated client-local entities. Failure cases additionally use
private test-only I/O/allocation controls; the production library is unchanged.

See the [RPG model and capability index][model] and [test commands][tests].

[record]: recording.feature
[game]: gameplay.feature
[replay]: replay.feature
[auto]: automatic.feature
[fail]: failures.feature
[c-record]: ../../adapters/file_sync/tests/rpg/recording.c
[c-game]: ../../adapters/file_sync/tests/rpg/gameplay.c
[c-replay]: ../../adapters/file_sync/tests/rpg/replay.c
[c-auto]: ../../adapters/file_sync/tests/rpg/automatic.c
[c-fail]: ../../adapters/file_sync/tests/rpg/failures.c
[model]: ../../docs/file-sync.md#rpg-behavior-specifications
[tests]: ../../docs/testing.md#running-the-rpg-tests

Run `BUNDLE_FROZEN=true bundle _2.6.9_ install` from the Sync root before testing. Ruby >=3.2
and RubyGems >=3.2.8 are required. With tests, FileSync and Cucumber enabled,
`ctest --test-dir build -R '^sync\.cucumber_rpg$' -V` executes all 58 cases,
including every Scenario Outline row. Use `-DSYNC_CUCUMBER=OFF` for only
the native runners.
