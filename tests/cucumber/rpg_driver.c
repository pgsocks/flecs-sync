#include "test.h"
#include "private.h"
#include "protocol.h"
struct script {
	const char *name, *example;
	bool (*operations)(struct rpg_context *, const char *, char *, size_t);
};
#define CASE(id, example, function) {#id, example, function},
static const struct script scripts[] = {
CASE(rpg_record_001, "", recording_views)
CASE(rpg_record_002, "", recording_late_start)
CASE(rpg_record_003, "", recording_delivery)
CASE(rpg_record_004, "", recording_inspection)
CASE(rpg_record_005, "", recording_options)
CASE(rpg_record_006, "", recording_flush)
CASE(rpg_record_007, "", recording_close)
CASE(rpg_record_008, "", recording_replace)
CASE(rpg_game_001, "", gameplay_trade)
CASE(rpg_game_002, "", gameplay_potion)
CASE(rpg_game_003, "", gameplay_merge)
CASE(rpg_game_004, "", gameplay_equipment)
CASE(rpg_game_005, "", gameplay_visibility)
CASE(rpg_game_006, "", gameplay_loot)
CASE(rpg_game_007, "", gameplay_identity)
CASE(rpg_game_008, "", gameplay_combat)
CASE(rpg_game_009_null, "null", gameplay_quest)
CASE(rpg_game_009_empty, "empty", gameplay_quest)
CASE(rpg_replay_001, "", replay_updates)
CASE(rpg_replay_002, "", replay_frames)
CASE(rpg_replay_003, "", replay_backpressure)
CASE(rpg_replay_004, "", replay_fragments)
CASE(rpg_replay_005, "", replay_eof)
CASE(rpg_replay_006, "", replay_reset)
CASE(rpg_auto_001, "", automatic_phases)
CASE(rpg_auto_002, "", automatic_recording_budget)
CASE(rpg_auto_003, "", automatic_application)
CASE(rpg_auto_004, "", automatic_replay_budget)
CASE(rpg_auto_005, "", automatic_cleanup)
#ifdef FILE_SYNC_TESTING
CASE(rpg_fail_001_checksum, "checksum", failure_records)
CASE(rpg_fail_001_zero_length, "zero-length", failure_records)
CASE(rpg_fail_001_oversized_length, "oversized-length", failure_records)
CASE(rpg_fail_001_partial_length, "partial-length", failure_records)
CASE(rpg_fail_001_partial_frame, "partial-frame", failure_records)
CASE(rpg_fail_001_partial_checksum, "partial-checksum", failure_records)
CASE(rpg_fail_001_bad_magic, "bad-magic", failure_records)
CASE(rpg_fail_001_reserved_header, "reserved-header", failure_records)
CASE(rpg_fail_001_future_version, "future-version", failure_records)
CASE(rpg_fail_001_short_header, "short-header", failure_records)
CASE(rpg_fail_002_header_only, "header-only", failure_eof)
CASE(rpg_fail_002_unfinished_snapshot, "unfinished-snapshot", failure_eof)
CASE(rpg_fail_002_unfinished_delta, "unfinished-delta", failure_eof)
CASE(rpg_fail_003_duplicate_first, "duplicate-first", failure_sequence)
CASE(rpg_fail_003_missing_first, "missing-first", failure_sequence)
CASE(rpg_fail_003_foreign_stream, "foreign-stream", failure_sequence)
CASE(rpg_fail_003_skipped_record, "skipped-record", failure_sequence)
CASE(rpg_fail_004, "", failure_later)
CASE(rpg_fail_005, "", failure_schema)
CASE(rpg_fail_006, "", failure_reference)
CASE(rpg_fail_007_length, "0", failure_write)
CASE(rpg_fail_007_frame, "1", failure_write)
CASE(rpg_fail_007_checksum, "2", failure_write)
CASE(rpg_fail_008_read, "read", failure_operation)
CASE(rpg_fail_008_flush, "flush", failure_operation)
CASE(rpg_fail_008_close, "close", failure_operation)
CASE(rpg_fail_008_allocate, "allocate", failure_operation)
CASE(rpg_fail_009, "", failure_open)
CASE(rpg_fail_010, "", failure_arguments)
#endif

};
#undef CASE
static struct rpg_context *active;
static const char *active_example;

static void cleanup(void)
{
	if (!active)
		return;
	file_sync_test_clear();
	if (!rpg_destroy(active))
		fprintf(stderr, "RPG failure cleanup failed\n");
	active = NULL;
}

static bool query(const char *command)
{
	if (!active)
		return false;
	if (!strcmp(command, "example")) {
		printf("ok example=%s\n", active_example);
		return true;
	}
	if (!strcmp(command, "result")) {
		printf("ok result=%d\n", active->result);
		return true;
	}
	unsigned view, actor, item, stacks;
	int consumed = 0;
	if (sscanf(command, "quantity %u %u %u%n",
		&view, &actor, &item, &consumed) == 3 &&
		!command[consumed] && view < RPG_VIEWS && actor < RPG_ACTORS &&
		item < RPG_ITEMS) {
		int64_t value = rpg_quantity(active, view, actor, item, &stacks);
		printf("ok quantity=%lld stacks=%u\n", (long long)value, stacks);
		return true;
	}
	if (!strcmp(command, "quest")) {
		ecs_entity_t e = rpg_replica(active, RPG_ARIA,
			active->details[RPG_ARIA]);
		if (!e)
			return false;
		const struct rpg_details *d = ecs_get_id(
			active->clients[RPG_ARIA].ecs, e,
			active->clients[RPG_ARIA].types[RPG_DETAILS]);
		if (!d)
			return false;
		printf("ok null=%u length=%zu\n", !d->quest,
			d->quest ? strlen(d->quest) : 0);
		return true;
	}
	return false;
}

int main(void)
{
	char name[128];
	struct rpg_context context = {0};
	char error[1024] = {0};

	cucumber_begin(name, sizeof(name));
	cucumber_queries(query);
	if (atexit(cleanup))
		return 1;
	for (unsigned i = 0; i < sizeof(scripts) / sizeof(*scripts); i++) {
		if (strcmp(name, scripts[i].name))
			continue;
		file_sync_test_clear();
		cucumber_wait(0, 1259650751u);
		active = &context;
		active_example = scripts[i].example;
		CHECK(rpg_initialize(&context));
		cucumber_reply();
		bool result = scripts[i].operations(&context,
			scripts[i].example, error, sizeof(error));
		cucumber_wait(9999, 472882565u);
		file_sync_test_clear();
		active = NULL;
		CHECK(rpg_destroy(&context));
		cucumber_reply();
		if (!result) {
			fprintf(stderr, "%s\n", error);
			return 1;
		}
		cucumber_done();
		return 0;
	}
	fprintf(stderr, "unknown workflow: %s\n", name);
	return 1;
}
