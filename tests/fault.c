#include "test.h"
#include "private.h"

static void creation_allocation(bool client)
{
	bool reached_success = false;
	for (size_t n = 0; n < 4096; n++) {
		struct world w;
		world_init(&w, false);
		sync_test_arm(SYNC_TEST_ALLOCATION, n);
		sync_error_t error;
		sync_result_t r = client
			? sync_client_create(w.ecs, NULL, &w.session, &error)
			: sync_server_create(w.ecs, NULL, &w.session, &error);
		bool hit = sync_test_hit(SYNC_TEST_ALLOCATION);
		sync_test_clear();
		if (hit) {
			EQ(r, SYNC_ERR_NOMEM);
			CHECK(!w.session);
			EQ(error.code, SYNC_ERR_NOMEM);
			OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
		} else {
			OK(r);
			reached_success = true;
		}
		world_fini(&w);
		if (reached_success)
			break;
	}
	CHECK(reached_success);
}


/* Server creation allocation failures leave no active session. */
TEST(fault_creation_allocation) { creation_allocation(false); }
/* Client creation allocation failures leave no active session. */
TEST(fault_client_creation_allocation) { creation_allocation(true); }

/* Receive allocation failures accept no bytes and fault recoverably. */
TEST(fault_receive_allocation)
{
	bool done = false;
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
			/* An existing buffer need not allocate here. */
			OK(r);
			CHECK(state.queued_bytes == p.items[0].size);
			done = true;
		}
		packets_fini(&p);
		fixture_fini(&f);
		if (done)
			break;
	}
	CHECK(done);
}

/* Decode allocation failures release nontrivial temporary values on reset. */
TEST(fault_decode_cleanup)
{
	bool reached_success = false;
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
	CHECK(reached_success);
}

static void publication_failure(size_t after)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, a);
	set_position(&f.server, a, 3);
	set_position(&f.server, b, 4);
	capture(&f, 2);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	receive_packets(f.clients[0].session, &p);
	sync_test_arm(SYNC_TEST_PUBLICATION, after);
	sync_apply_result_t progress;
	EQ(sync_apply(f.clients[0].session, 1, &progress), SYNC_ERR_RUNTIME);
	CHECK(sync_test_hit(SYNC_TEST_PUBLICATION));
	sync_test_clear();
	CHECK(!progress.updates_applied && !progress.last_tick);
	EQ(status(f.clients[0].session, 0).state, SYNC_FAULTED);
	CHECK(replica(&f.clients[0], id) != 0);
	/* No assertion of rollback or of which entity was mutated first. */
	OK(sync_client_reset(f.clients[0].session));
	expect_absent(&f.clients[0], id);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Runtime failure before the first mutation faults without a false update. */
TEST(fault_before_publication) { publication_failure(0); }
/* Failure after a mutation makes no rollback promise and remains resettable. */
TEST(fault_during_publication) { publication_failure(1); }

/* Frame inspection performs no Sync allocation and does not consume faults. */
TEST(fault_inspection_no_allocation)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	sync_test_arm(SYNC_TEST_ALLOCATION, 0);
	sync_frame_info_t info;
	OK(sync_frame_inspect(p.items[0].data, p.items[0].size, &info));
	CHECK(!sync_test_hit(SYNC_TEST_ALLOCATION));
	sync_test_clear();
	check_info_equal(&info, &p.items[0].info);
	packets_fini(&p);
	fixture_fini(&f);
}

/* Stream identities issue UINT64_MAX once and reject overflow without reuse. */
TEST(fault_stream_exhaustion)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_test_seed(f.server.session, 0, SYNC_TEST_STREAM_ID,
		UINT64_MAX - 1);
	ecs_entity_t peer, failed = UINT64_MAX;
	OK(sync_peer_open(f.server.session, &peer));
	CHECK(status(f.server.session, peer).stream_id == UINT64_MAX);
	EQ(sync_peer_open(f.server.session, &failed), SYNC_ERR_LIMIT);
	CHECK(!failed);
	fixture_fini(&f);
}

/* Exhausted network identity space never wraps or silently loses an entity. */
TEST(fault_network_exhaustion)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_test_seed(f.server.session, 0, SYNC_TEST_NETWORK_ID,
		UINT64_MAX - 1);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	CHECK(network_id(&f.server, e) == UINT64_MAX);
	spawn(&f.server, 2, 0);
	sync_capture_result_t p;
	EQ(sync_capture(f.server.session, 2, &p), SYNC_ERR_LIMIT);
	CHECK(p.tick == 2);
	CHECK(network_id(&f.server, e) == UINT64_MAX);
	fixture_fini(&f);
}

/* A peer sequence overflow faults that peer while healthy peers continue. */
TEST(fault_sequence_exhaustion)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	capture(&f, 1);
	transfer(&f, 0);
	transfer(&f, 1);
	sync_test_seed(f.server.session, f.peers[0],
		SYNC_TEST_SEQUENCE, UINT64_MAX);
	sync_capture_result_t p;
	EQ(sync_capture(f.server.session, 2, &p), SYNC_PARTIAL);
	CHECK(p.peers_faulted == 1 && p.peers_queued == 1);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	transfer(&f, 1);
	EQ(status(f.clients[1].session, 0).last_tick, 2);
	fixture_fini(&f);
}

/* Automatic ticks cannot wrap to zero when their counter is exhausted. */
TEST(fault_automatic_tick_exhaustion)
{
	struct fixture f;
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	fixture_init(&f, &d, NULL);
	sync_test_seed(f.server.session, 0, SYNC_TEST_AUTO_TICK, UINT64_MAX);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	EQ(status(f.server.session, 0).state, SYNC_FAULTED);
	EQ(status(f.server.session, 0).fault.code, SYNC_ERR_LIMIT);
	fixture_fini(&f);
}

/* Capture allocation failures preserve updates or explicitly fault streams. */
TEST(fault_capture_allocation)
{
	bool done = false;
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
			/* Reservation can fail before sampling starts. */
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
	CHECK(done);
}
