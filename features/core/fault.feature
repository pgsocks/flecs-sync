Feature: fault contracts

@core
Scenario: fault_creation_allocation
	# When each server-creation allocation is failed in turn
	# Then Server creation allocation failures leave no active session.
	Given the "core" workflow "fault_creation_allocation"
	When I perform operation 1:
		"""
		creation_allocation(false);
		"""
	Then its observed values match the contract

@core
Scenario: fault_client_creation_allocation
	# When each client-creation allocation is failed in turn
	# Then Client creation allocation failures leave no active session.
	Given the "core" workflow "fault_client_creation_allocation"
	When I perform operation 1:
		"""
		creation_allocation(true);
		"""
	Then its observed values match the contract

@core
Scenario: fault_receive_allocation
	# When allocations required to copy an incoming frame fail
	# Then Receive allocation failures accept no bytes and fault recoverably.
	Given the "core" workflow "fault_receive_allocation"
	When I perform operation 1:
		"""
		bool done = false;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		for (size_t n = 0; n < 4096; n++) {
		struct fixture f;
		fixture_init(&f, NULL, NULL);
		capture(&f, 1);
		struct packets p = take_packets(f.server.session, f.peers[0]);
		CHECK(p.count > 0);
		sync_test_arm(SYNC_TEST_ALLOCATION, n);
		sync_result_t r = sync_receive(f.clients[0].session,
		p.items[0].data, p.items[0].size);
		bool hit = sync_test_hit(SYNC_TEST_ALLOCATION);
		sync_test_clear();
		sync_status_t state = status(f.clients[0].session, 0);
		if (hit) {
		EQ(r, SYNC_ERR_NOMEM);
		EQ(state.state, SYNC_FAULTED);
		CHECK(!state.queued_bytes && !state.entities);
		EQ(state.fault.code, SYNC_ERR_NOMEM);
		} else {
		
		OK(r);
		CHECK(state.queued_bytes == p.items[0].size);
		done = true;
		}
		packets_fini(&p);
		fixture_fini(&f);
		if (done)
		break;
		}
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		CHECK(done);
		"""
	Then its observed values match the contract

@core
Scenario: fault_decode_cleanup
	# When each allocation while decoding an owned string fails in turn
	# Then Decode allocation failures release nontrivial temporary values on reset.
	Given the "core" workflow "fault_decode_cleanup"
	When I perform operation 1:
		"""
		bool reached_success = false;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		for (size_t n = 0; n < 4096; n++) {
		struct fixture f;
		fixture_init(&f, NULL, NULL);
		ecs_entity_t e = spawn(&f.server, 1, 0);
		set_record(&f.server, e, (struct record){ .text = "owned" });
		capture(&f, 1);
		struct packets p = take_packets(f.server.session, f.peers[0]);
		receive_packets(f.clients[0].session, &p);
		sync_test_arm(SYNC_TEST_ALLOCATION, n);
		sync_result_t r = sync_apply(f.clients[0].session, 1, NULL);
		bool hit = sync_test_hit(SYNC_TEST_ALLOCATION);
		sync_test_clear();
		if (hit) {
		EQ(r, SYNC_ERR_NOMEM);
		EQ(status(f.clients[0].session, 0).state, SYNC_FAULTED);
		} else {
		OK(r);
		reached_success = true;
		}
		OK(sync_client_reset(f.clients[0].session));
		packets_fini(&p);
		fixture_fini(&f);
		EQ(live_records, 0);
		if (reached_success)
		break;
		}
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		CHECK(reached_success);
		"""
	Then its observed values match the contract

@core
Scenario: fault_before_publication
	# When publication fails before the first ECS mutation
	# Then Runtime failure before the first mutation faults without a false update.
	Given the "core" workflow "fault_before_publication"
	When I perform operation 1:
		"""
		publication_failure(0);
		"""
	Then its observed values match the contract

@core
Scenario: fault_during_publication
	# When publication fails after one ECS mutation
	# Then Failure after a mutation makes no rollback promise and remains
	# resettable.
	Given the "core" workflow "fault_during_publication"
	When I perform operation 1:
		"""
		publication_failure(1);
		"""
	Then its observed values match the contract

@core
Scenario: fault_inspection_no_allocation
	# When inspection runs with an allocation failure armed
	# Then Frame inspection performs no Sync allocation and does not consume
	# faults.
	Given the "core" workflow "fault_inspection_no_allocation"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		struct packets p = take_packets(f.server.session, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_test_arm(SYNC_TEST_ALLOCATION, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		sync_frame_info_t info;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		OK(sync_frame_inspect(p.items[0].data, p.items[0].size, &info));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!sync_test_hit(SYNC_TEST_ALLOCATION));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		sync_test_clear();
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		check_info_equal(&info, &p.items[0].info);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		packets_fini(&p);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: fault_stream_exhaustion
	# When peer creation reaches the last representable stream identity
	# Then Stream identities issue UINT64_MAX once and reject overflow without
	# reuse.
	Given the "core" workflow "fault_stream_exhaustion"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_test_seed(f.server.session, 0, SYNC_TEST_STREAM_ID,
		UINT64_MAX - 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t peer, failed = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_peer_open(f.server.session, &peer));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(status(f.server.session, peer).stream_id == UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_peer_open(f.server.session, &failed), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!failed);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: fault_network_exhaustion
	# When capture reaches the last representable network identity
	# Then Exhausted network identity space never wraps or silently loses an
	# entity.
	Given the "core" workflow "fault_network_exhaustion"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_test_seed(f.server.session, 0, SYNC_TEST_NETWORK_ID,
		UINT64_MAX - 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(network_id(&f.server, e) == UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		sync_capture_result_t p;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_capture(f.server.session, 2, &p), SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(p.tick == 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(network_id(&f.server, e) == UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: fault_sequence_exhaustion
	# When one ready peer exhausts its frame sequence
	# Then A peer sequence overflow faults that peer while healthy peers continue.
	Given the "core" workflow "fault_sequence_exhaustion"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		fixture_init(&f, NULL, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		sync_test_seed(f.server.session, f.peers[0],
		SYNC_TEST_SEQUENCE, UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_capture_result_t p;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(sync_capture(f.server.session, 2, &p), SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(p.peers_faulted == 1 && p.peers_queued == 1);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(status(f.clients[1].session, 0).last_tick, 2);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: fault_automatic_tick_exhaustion
	# When automatic progress exhausts its tick counter
	# Then Automatic ticks cannot wrap to zero when their counter is exhausted.
	Given the "core" workflow "fault_automatic_tick_exhaustion"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_test_seed(f.server.session, 0, SYNC_TEST_AUTO_TICK, UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(status(f.server.session, 0).state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(status(f.server.session, 0).fault.code, SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: fault_capture_allocation
	# When allocations fail while retaining a changed sample
	# Then Capture allocation failures preserve updates or explicitly fault
	# streams.
	Given the "core" workflow "fault_capture_allocation"
	When I perform operation 1:
		"""
		bool done = false;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		for (size_t n = 0; n < 4096; n++) {
		struct fixture f;
		fixture_init(&f, NULL, NULL);
		ecs_entity_t e = spawn(&f.server, 1, 0);
		capture(&f, 1);
		transfer(&f, 0);
		transfer(&f, 1);
		set_record(&f.server, e,
		(struct record){ .text = "new value" });
		set_position(&f.server, e, 2);
		sync_test_arm(SYNC_TEST_ALLOCATION, n);
		sync_capture_result_t p;
		sync_result_t r = sync_capture(f.server.session, 2, &p);
		bool hit = sync_test_hit(SYNC_TEST_ALLOCATION);
		sync_test_clear();
		if (hit) {
		CHECK(r == SYNC_ERR_NOMEM || r == SYNC_PARTIAL);
		
		CHECK(p.tick == 0 || p.tick == 2);
		} else {
		OK(r);
		CHECK(p.tick == 2);
		done = true;
		}
		for (unsigned i = 0; i < 2; i++) {
		sync_status_t peer = status(f.server.session,
		f.peers[i]);
		if (peer.state == SYNC_FAULTED) {
		CHECK(peer.fault.code == SYNC_ERR_NOMEM);
		continue;
		}
		transfer(&f, i);
		}
		if (status(f.server.session, 0).state != SYNC_FAULTED) {
		OK(sync_capture(f.server.session, 3, NULL));
		for (unsigned i = 0; i < 2; i++) {
		if (status(f.server.session, f.peers[i]).state
		== SYNC_FAULTED)
		continue;
		transfer(&f, i);
		expect_position(&f.clients[i],
		network_id(&f.server, e), 2);
		}
		}
		fixture_fini(&f);
		if (done)
		break;
		}
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		CHECK(done);
		"""
	Then its observed values match the contract
