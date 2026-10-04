@failures
Feature: Adventure recordings report failures without inventing game state

@RPG-FAIL-001 @rpg
Scenario Outline: Invalid records are rejected before RPG publication
	# When the campaign records its initial update
	# And Aria's recording closes
	# And Aria's recording is damaged with <damage>
	# And Aria attempts replay
	# Then the file result is <result>
	# And Aria's replay has no character replicas
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<damage>"
	When I perform operation 1:
		"""
		const char *damages[] = {"checksum", "zero-length", "oversized-length",
		"partial-length", "partial-frame", "partial-checksum",
		"bad-magic", "reserved-header", "future-version",
		"short-header"};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		const sync_result_t results[] = {SYNC_ERR_PROTOCOL, SYNC_ERR_PROTOCOL,
		SYNC_ERR_LIMIT, SYNC_ERR_TRUNCATED, SYNC_ERR_TRUNCATED,
		SYNC_ERR_TRUNCATED, SYNC_ERR_PROTOCOL, SYNC_ERR_PROTOCOL,
		SYNC_ERR_UNSUPPORTED, SYNC_ERR_TRUNCATED};
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_result_t expected = SYNC_OK;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (unsigned i = 0; i < sizeof(damages) / sizeof(*damages); i++)
		if (!strcmp(example, damages[i]))
		expected = results[i];
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(expected < 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(damage_file(example));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(attempt_replay(c, expected, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	Then the example file result is "<result>"
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| damage           | result                | workflow |
		| checksum         | SYNC_ERR_PROTOCOL     | rpg_fail_001_checksum |
		| zero-length      | SYNC_ERR_PROTOCOL     | rpg_fail_001_zero_length |
		| oversized-length | SYNC_ERR_LIMIT        | rpg_fail_001_oversized_length |
		| partial-length   | SYNC_ERR_TRUNCATED    | rpg_fail_001_partial_length |
		| partial-frame    | SYNC_ERR_TRUNCATED    | rpg_fail_001_partial_frame |
		| partial-checksum | SYNC_ERR_TRUNCATED    | rpg_fail_001_partial_checksum |
		| bad-magic        | SYNC_ERR_PROTOCOL     | rpg_fail_001_bad_magic |
		| reserved-header  | SYNC_ERR_PROTOCOL     | rpg_fail_001_reserved_header |
		| future-version   | SYNC_ERR_UNSUPPORTED  | rpg_fail_001_future_version |
		| short-header     | SYNC_ERR_TRUNCATED    | rpg_fail_001_short_header |

@RPG-FAIL-002 @rpg
Scenario Outline: Physical EOF does not prove logical completion
	# When the campaign records its initial update
	# And Aria receives a quest note of 65535 characters
	# And the campaign records another update
	# And Aria's recording closes
	# And Aria's recording ends at <boundary>
	# And Aria attempts replay
	# Then the file result is SYNC_ERR_TRUNCATED
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<boundary>"
	When I perform operation 1:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(rpg_large_quest(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(truncate_recording(c, example, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(attempt_replay(c, SYNC_ERR_TRUNCATED, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| boundary             | workflow |
		| header-only          | rpg_fail_002_header_only |
		| unfinished-snapshot  | rpg_fail_002_unfinished_snapshot |
		| unfinished-delta     | rpg_fail_002_unfinished_delta |

@RPG-FAIL-003 @rpg
Scenario Outline: Stream identity and ordering remain strict
	# When the campaign records its initial update
	# And Aria's recording closes
	# And Bram's recording closes
	# And Aria's recording is damaged with <damage>
	# And Aria attempts replay
	# Then the file result is SYNC_ERR_SEQUENCE
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<damage>"
	When I perform operation 1:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_writer_close(c, RPG_BRAM), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(damage_file(example));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(attempt_replay(c, SYNC_ERR_SEQUENCE, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| damage           | workflow |
		| duplicate-first  | rpg_fail_003_duplicate_first |
		| missing-first    | rpg_fail_003_missing_first |
		| foreign-stream   | rpg_fail_003_foreign_stream |
		| skipped-record   | rpg_fail_003_skipped_record |

@RPG-FAIL-004 @rpg
Scenario: A later failure preserves previously published adventure state
	# When the campaign records its initial update
	# And Aria's recording closes
	# And Aria's recording is damaged with a-late-duplicate
	# And Aria attempts replay
	# Then the file result is SYNC_ERR_SEQUENCE
	# And Aria has published 1 update before the failure
	# And Aria sees Aria with health 100 at tick 1
	Given the "rpg" workflow "rpg_fail_004"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_writer_close(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(damage_file("a-late-duplicate"));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(attempt_replay(c, SYNC_ERR_SEQUENCE, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		NEED(c->progress.application.updates_applied == 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		NEED(rpg_expect_health(c, RPG_ARIA, RPG_ARIA, 100, 1, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-FAIL-005 @rpg
Scenario: Incompatible local RPG stats reject the manifest
	# Given Aria's client has an incompatible character schema
	# When the campaign records its initial update
	# And Aria attempts replay
	# Then the file result is SYNC_ERR_SCHEMA
	# And Aria's replay has no character replicas
	Given the "rpg" workflow "rpg_fail_005"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(rpg_client_recreate(c, RPG_ARIA, true));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(attempt_replay(c, SYNC_ERR_SCHEMA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		NEED(rpg_expect_no_replicas(c, RPG_ARIA, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-FAIL-006 @rpg
Scenario: Hiding a referenced stack faults only the affected recording
	# When the campaign records its initial update
	# And Aria's potion stack becomes hidden without clearing its slot
	# And the campaign captures the invalid update
	# Then Aria's recording faults with SYNC_ERR_REFERENCE
	# And the GM recording continues with valid inventory references
	Given the "rpg" workflow "rpg_fail_006"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t e = rpg_find_stack(c, RPG_ARIA, RPG_POTION);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		NEED(e);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		rpg_policy(c, e, 1u << RPG_GM);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		RESULT(rpg_capture(c, false), SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		RESULT(file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress),
		SYNC_ERR_REFERENCE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_status_t status = rpg_writer_status(c, RPG_ARIA);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(status.state == FILE_SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(status.fault.detail.code, SYNC_ERR_REFERENCE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		sync_status_t peer;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		RESULT(sync_get_status(c->server.session, c->peers[RPG_ARIA], &peer),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		NEED(peer.state == SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		RESULT(peer.fault.code, SYNC_ERR_REFERENCE);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		RESULT(sync_get_status(c->server.session, 0, &peer), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		NEED(peer.state == SYNC_SERVER_READY && peer.faulted_peers == 1);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		RESULT(file_sync_writer_pump(
		c->writers[RPG_GM], UINT32_MAX, &c->progress),
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		RESULT(file_sync_writer_flush(c->writers[RPG_GM]), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		RESULT(rpg_replay(c, RPG_GM, 2), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		NEED(c->progress.application.last_tick == 2 && rpg_references(c));
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		NEED(rpg_replica(c, RPG_GM, e));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-FAIL-007 @rpg
Scenario Outline: Partial writes do not commit the queued adventure frame
	# When a campaign update is captured without servicing recordings
	# And a short write is injected after <writes> writes
	# And Aria services one recording frame
	# Then the file result is SYNC_ERR_IO
	# And Aria's queued frame remains uncommitted
	# And the first fault survives closure without a retry
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<writes>"
	When I perform operation 1:
		"""
		NEED(!strcmp(example, "0") || !strcmp(example, "1") ||
		!strcmp(example, "2"));
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(rpg_capture_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		file_sync_test_arm(FILE_SYNC_TEST_WRITE, (size_t)(example[0] - '0'));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		c->result =
		file_sync_writer_pump(c->writers[RPG_ARIA], 1, &c->progress);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		bool hit = file_sync_test_hit(FILE_SYNC_TEST_WRITE);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		c->first_fault = rpg_writer_status(c, RPG_ARIA).fault;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		NEED(hit && !c->progress.frames && !c->progress.bytes);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(c->first_fault.record == 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(c->result, SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(rpg_expect_queued(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		NEED(expect_first_fault(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| writes  | workflow |
		| 0       | rpg_fail_007_length |
		| 1       | rpg_fail_007_frame |
		| 2       | rpg_fail_007_checksum |

@RPG-FAIL-008 @rpg
Scenario Outline: File operations preserve diagnostics and cleanup
	# When the campaign records its initial update
	# And a failure is injected into <operation>
	# And Aria performs the failing <operation>
	# Then the file result is <result>
	# And the first fault survives closure without a retry
	Given the "rpg" workflow "<workflow>"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	Then the fixture example is "<operation>"
	When I perform operation 1:
		"""
		RESULT(rpg_capture(c, true), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		int fault = operation(example);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(fault >= 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		if (fault == FILE_SYNC_TEST_READ || fault == FILE_SYNC_TEST_ALLOC)
		RESULT(rpg_reader_open(c, RPG_ARIA), SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		file_sync_test_arm((enum file_sync_test_fault)fault, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		if (fault == FILE_SYNC_TEST_READ || fault == FILE_SYNC_TEST_ALLOC) {
		c->result = file_sync_reader_step(
		c->readers[RPG_ARIA], 1, &c->progress);
		c->first_fault = rpg_reader_status(c, RPG_ARIA).fault;
		} else if (fault == FILE_SYNC_TEST_FLUSH) {
		c->result = file_sync_writer_flush(c->writers[RPG_ARIA]);
		c->first_fault = rpg_writer_status(c, RPG_ARIA).fault;
		} else {
		c->result = rpg_writer_close(c, RPG_ARIA);
		c->first_fault = c->error;
		c->close_released = true;
		}
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		bool hit = file_sync_test_hit((enum file_sync_test_fault)fault);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		file_sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		NEED(hit);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		RESULT(c->result,
		fault == FILE_SYNC_TEST_ALLOC ? SYNC_ERR_NOMEM : SYNC_ERR_IO);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		NEED(expect_first_fault(c, error, size));
		"""
	Then its observed values match the contract
	Then the example file result is "<result>"
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
	Examples:
		| operation | result          | workflow |
		| read      | SYNC_ERR_IO     | rpg_fail_008_read |
		| flush     | SYNC_ERR_IO     | rpg_fail_008_flush |
		| close     | SYNC_ERR_IO     | rpg_fail_008_close |
		| allocate  | SYNC_ERR_NOMEM  | rpg_fail_008_allocate |

@RPG-FAIL-009 @rpg
Scenario: Open failures leave the RPG client unchanged
	# Then nonexistent recording paths fail without retaining handles
	# And allocation failure during recording open releases resources
	Given the "rpg" workflow "rpg_fail_009"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(missing_paths(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(open_allocation(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract

@RPG-FAIL-010 @rpg
Scenario: Invalid file calls preserve initialized outputs and healthy state
	# Then invalid file arguments are rejected with initialized outputs
	# And invalid file roles schedules and limits retain no resources
	Given the "rpg" workflow "rpg_fail_010"
	When I perform operation 0:
		"""
		rpg_initialize(&context);
		"""
	Then its observed values match the contract
	When I perform operation 1:
		"""
		(void)example;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		NEED(invalid_arguments(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		NEED(invalid_options(c, error, size));
		"""
	Then its observed values match the contract
	When I perform operation 9999:
		"""
		rpg_destroy(&context);
		"""
	Then its observed values match the contract
