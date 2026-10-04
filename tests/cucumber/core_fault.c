/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "private.h"
#include "protocol.h"

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
TEST(fault_creation_allocation) {
	cucumber_wait(1, 3283347106u);
	creation_allocation(false);
	cucumber_reply();
}
/* Client creation allocation failures leave no active session. */
TEST(fault_client_creation_allocation) {
	cucumber_wait(1, 1133743028u);
	creation_allocation(true);
	cucumber_reply();
}

/* Receive allocation failures accept no bytes and fault recoverably. */
TEST(fault_receive_allocation)
{
	cucumber_wait(1, 1262149825u);
	bool done = false;
	cucumber_reply();

	cucumber_wait(2, 1908649486u);
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
	cucumber_reply();

	cucumber_wait(3, 2938458951u);
	CHECK(done);
	cucumber_reply();
}

/* Decode allocation failures release nontrivial temporary values on reset. */
TEST(fault_decode_cleanup)
{
	cucumber_wait(1, 1884560267u);
	bool reached_success = false;
	cucumber_reply();

	cucumber_wait(2, 1268673369u);
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
	cucumber_reply();

	cucumber_wait(3, 436292567u);
	CHECK(reached_success);
	cucumber_reply();
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
TEST(fault_before_publication) {
	cucumber_wait(1, 1357650572u);
	publication_failure(0);
	cucumber_reply();
}
/* Failure after a mutation makes no rollback promise and remains resettable. */
TEST(fault_during_publication) {
	cucumber_wait(1, 1361998011u);
	publication_failure(1);
	cucumber_reply();
}

/* Frame inspection performs no Sync allocation and does not consume faults. */
TEST(fault_inspection_no_allocation)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 4190491061u);
	struct packets p = take_packets(f.server.session, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 4280401617u);
	sync_test_arm(SYNC_TEST_ALLOCATION, 0);
	cucumber_reply();

	cucumber_wait(6, 393334897u);
	sync_frame_info_t info;
	cucumber_reply();

	cucumber_wait(7, 2500512470u);
	OK(sync_frame_inspect(p.items[0].data, p.items[0].size, &info));
	cucumber_reply();

	cucumber_wait(8, 3607408028u);
	CHECK(!sync_test_hit(SYNC_TEST_ALLOCATION));
	cucumber_reply();

	cucumber_wait(9, 1776369140u);
	sync_test_clear();
	cucumber_reply();

	cucumber_wait(10, 1094710164u);
	check_info_equal(&info, &p.items[0].info);
	cucumber_reply();

	cucumber_wait(11, 1885348443u);
	packets_fini(&p);
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Stream identities issue UINT64_MAX once and reject overflow without reuse. */
TEST(fault_stream_exhaustion)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2752433468u);
	sync_test_seed(f.server.session, 0, SYNC_TEST_STREAM_ID,
		UINT64_MAX - 1);
	cucumber_reply();

	cucumber_wait(4, 1215218635u);
	ecs_entity_t peer, failed = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(5, 3798078875u);
	OK(sync_peer_open(f.server.session, &peer));
	cucumber_reply();

	cucumber_wait(6, 2944994607u);
	CHECK(status(f.server.session, peer).stream_id == UINT64_MAX);
	cucumber_reply();

	cucumber_wait(7, 3808253066u);
	EQ(sync_peer_open(f.server.session, &failed), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(8, 1536479042u);
	CHECK(!failed);
	cucumber_reply();

	cucumber_wait(9, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Exhausted network identity space never wraps or silently loses an entity. */
TEST(fault_network_exhaustion)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 3217266890u);
	sync_test_seed(f.server.session, 0, SYNC_TEST_NETWORK_ID,
		UINT64_MAX - 1);
	cucumber_reply();

	cucumber_wait(4, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 933750960u);
	CHECK(network_id(&f.server, e) == UINT64_MAX);
	cucumber_reply();

	cucumber_wait(7, 3875599360u);
	spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(8, 2907244225u);
	sync_capture_result_t p;
	cucumber_reply();

	cucumber_wait(9, 1310295491u);
	EQ(sync_capture(f.server.session, 2, &p), SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(10, 1091358573u);
	CHECK(p.tick == 2);
	cucumber_reply();

	cucumber_wait(11, 933750960u);
	CHECK(network_id(&f.server, e) == UINT64_MAX);
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* A peer sequence overflow faults that peer while healthy peers continue. */
TEST(fault_sequence_exhaustion)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(4, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(5, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 211844715u);
	sync_test_seed(f.server.session, f.peers[0],
		SYNC_TEST_SEQUENCE, UINT64_MAX);
	cucumber_reply();

	cucumber_wait(7, 2907244225u);
	sync_capture_result_t p;
	cucumber_reply();

	cucumber_wait(8, 514245797u);
	EQ(sync_capture(f.server.session, 2, &p), SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(9, 3124697474u);
	CHECK(p.peers_faulted == 1 && p.peers_queued == 1);
	cucumber_reply();

	cucumber_wait(10, 2960022781u);
	EQ(status(f.server.session, f.peers[0]).fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(11, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(12, 2323807114u);
	EQ(status(f.clients[1].session, 0).last_tick, 2);
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Automatic ticks cannot wrap to zero when their counter is exhausted. */
TEST(fault_automatic_tick_exhaustion)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 1030511650u);
	sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
	cucumber_reply();

	cucumber_wait(3, 1089821575u);
	fixture_init(&f, &d, NULL);
	cucumber_reply();

	cucumber_wait(4, 2989753250u);
	sync_test_seed(f.server.session, 0, SYNC_TEST_AUTO_TICK, UINT64_MAX);
	cucumber_reply();

	cucumber_wait(5, 1169400985u);
	CHECK(ecs_progress(f.server.ecs, 0.125f));
	cucumber_reply();

	cucumber_wait(6, 1116331269u);
	EQ(status(f.server.session, 0).state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(7, 980664347u);
	EQ(status(f.server.session, 0).fault.code, SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(8, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Capture allocation failures preserve updates or explicitly fault streams. */
TEST(fault_capture_allocation)
{
	cucumber_wait(1, 1262149825u);
	bool done = false;
	cucumber_reply();

	cucumber_wait(2, 4125155954u);
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
	cucumber_reply();

	cucumber_wait(3, 2938458951u);
	CHECK(done);
	cucumber_reply();
}
