#include "test.h"

/* Snapshots route public and private entities to independent client worlds. */
TEST(replication_two_recipients)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t public = spawn(&f.server, 10, 0);
	ecs_entity_t private[2];
	for (unsigned i = 0; i < 2; i++)
		private[i] = spawn(&f.server, (float)i, f.peers[i]);
	capture(&f, 1);
	for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, public),
			10);
		expect_position(&f.clients[i],
			network_id(&f.server, private[i]), (float)i);
		expect_absent(&f.clients[i],
			network_id(&f.server, private[1 - i]));
		EQ(status(f.clients[i].session, 0).entities, 2);
	}
	fixture_fini(&f);
}

/* Tracked writes in a shared table reach both peers without losing rows. */
TEST(replication_tracked_writes)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	capture(&f, 1);
	transfer(&f, 0);
	transfer(&f, 1);
	set_position(&f.server, a, 3);
	capture(&f, 2);
	for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 3);
		expect_position(&f.clients[i], network_id(&f.server, b), 2);
	}
	struct position *p = ecs_get_mut_id(f.server.ecs, a, f.server.position);
	CHECK(p != NULL);
	p->x = 4;
	p->y = 5;
	ecs_modified_id(f.server.ecs, a, f.server.position);
	capture(&f, 3);
	for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 4);
	}
	fixture_fini(&f);
}

/* Writable iteration reports changes after the iterator has completed. */
TEST(replication_writable_query)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	ecs_query_t *q = ecs_query_init(f.server.ecs, &(ecs_query_desc_t){
		.terms = {{ .id = f.server.position, .inout = EcsInOut }}
	});
	CHECK(q != NULL);
	ecs_iter_t it = ecs_query_iter(f.server.ecs, q);
	while (ecs_query_next(&it)) {
		struct position *p = ecs_field(&it, struct position, 0);
		for (int32_t i = 0; i < it.count; i++) {
			p[i].x = 8;
			p[i].y = 9;
		}
	}
	ecs_query_fini(q);
	capture(&f, 2);
	transfer(&f, 0);
	expect_position(&f.clients[0], network_id(&f.server, e), 8);
	fixture_fini(&f);
}

/* Visibility entry sends full state even after its value table is clean. */
TEST(replication_visibility_entry)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 5, f.peers[0]);
	capture(&f, 1);
	transfer(&f, 0);
	transfer(&f, 1);
	uint64_t id = network_id(&f.server, e);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
	capture(&f, 2);
	transfer(&f, 1);
	expect_position(&f.clients[1], id, 5);
	fixture_fini(&f);
}

/* Exit deletes the replica; reentry rebuilds it under the same network ID. */
TEST(replication_exit_reentry)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 5, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, e);
	ecs_entity_t old = replica(&f.clients[0], id);
	ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	capture(&f, 2);
	transfer(&f, 0);
	expect_absent(&f.clients[0], id);
	CHECK(!ecs_is_alive(f.clients[0].ecs, old));
	CHECK(replica(&f.server, id) == e);
	ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	capture(&f, 3);
	transfer(&f, 0);
	CHECK(network_id(&f.server, e) == id);
	CHECK(replica(&f.clients[0], id) != old);
	expect_position(&f.clients[0], id, 5);
	fixture_fini(&f);
}

/* Recipient pairs form a union with the public policy tag. */
TEST(replication_policy_union)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
	capture(&f, 1);
	transfer(&f, 0);
	transfer(&f, 1);
	ecs_remove_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
	capture(&f, 2);
	transfer(&f, 0);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	capture(&f, 3);
	transfer(&f, 0);
	transfer(&f, 1);
	expect_absent(&f.clients[0], network_id(&f.server, e));
	expect_position(&f.clients[1], network_id(&f.server, e), 1);
	fixture_fini(&f);
}

/* Prefabs, disabled entities, inherited values and absent policy stay out. */
TEST(replication_excluded_entities)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t prefab = spawn(&f.server, 1, 0);
	ecs_add_id(f.server.ecs, prefab, EcsPrefab);
	ecs_entity_t disabled = spawn(&f.server, 2, 0);
	ecs_add_id(f.server.ecs, disabled, EcsDisabled);
	ecs_entity_t hidden = spawn(&f.server, 3, 0);
	ecs_remove_id(f.server.ecs, hidden, f.server.ids.replicate_to_all);
	ecs_entity_t inherited = ecs_new(f.server.ecs);
	ecs_add_pair(f.server.ecs, inherited, EcsIsA, prefab);
	ecs_add_id(f.server.ecs, inherited, f.server.ids.replicate_to_all);
	capture(&f, 1);
	transfer(&f, 0);
	EQ(status(f.clients[0].session, 0).entities, 0);
	fixture_fini(&f);
}

/* Tag-only entities replicate while arbitrary pairs and policy do not. */
TEST(replication_tag_and_pair_exclusion)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = ecs_new(f.server.ecs);
	ecs_add_id(f.server.ecs, e, f.server.tag);
	ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	ecs_add_pair(f.server.ecs, e, f.server.tag, f.server.local);
	capture(&f, 1);
	transfer(&f, 0);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	CHECK(ecs_has_id(f.clients[0].ecs, r, f.clients[0].tag));
	CHECK(!ecs_has_id(f.clients[0].ecs, r,
		f.clients[0].ids.replicate_to_all));
	CHECK(!ecs_has_pair(f.clients[0].ecs, r,
		f.clients[0].tag, EcsWildcard));
	fixture_fini(&f);
}

/* Add/remove/delete mutations coalesce to final sampled membership. */
TEST(replication_coalescing)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, e);
	ecs_remove_id(f.server.ecs, e, f.server.position);
	set_position(&f.server, e, 4);
	ecs_add_id(f.server.ecs, e, f.server.tag);
	ecs_remove_id(f.server.ecs, e, f.server.tag);
	ecs_entity_t transient = spawn(&f.server, 8, 0);
	ecs_delete(f.server.ecs, transient);
	capture(&f, 2);
	transfer(&f, 0);
	expect_position(&f.clients[0], id, 4);
	CHECK(!ecs_has_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].tag));
	EQ(status(f.clients[0].session, 0).entities, 1);
	ecs_remove_id(f.server.ecs, e, f.server.position);
	capture(&f, 3);
	transfer(&f, 0);
	expect_absent(&f.clients[0], id);
	fixture_fini(&f);
}

/* Deleting and recycling a Flecs slot cannot recycle its network identity. */
TEST(replication_native_id_reuse)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t old = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, old);
	ecs_delete(f.server.ecs, old);
	ecs_entity_t replacement = spawn(&f.server, 2, 0);
	CHECK((uint32_t)old == (uint32_t)replacement);
	CHECK(old != replacement);
	capture(&f, 2);
	transfer(&f, 0);
	uint64_t new_id = network_id(&f.server, replacement);
	CHECK(new_id != id);
	expect_absent(&f.server, id);
	expect_absent(&f.clients[0], id);
	expect_position(&f.clients[0], new_id, 2);
	fixture_fini(&f);
}

static void set_reference(struct world *w, ecs_entity_t e, ecs_entity_t target)
{
	ecs_set_id(w->ecs, e, w->reference, sizeof(target), &target);
}

/* Forward, cyclic, self and null references resolve to client-local IDs. */
TEST(replication_reference_mapping)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	set_reference(&f.server, a, b);
	set_reference(&f.server, b, a);
	capture(&f, 1);
	transfer(&f, 0);
	ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
	ecs_entity_t rb = replica(&f.clients[0], network_id(&f.server, b));
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
	CHECK(ref && *ref == rb);
	ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
	CHECK(ref && *ref == ra);
	set_reference(&f.server, a, a);
	set_reference(&f.server, b, 0);
	capture(&f, 2);
	transfer(&f, 0);
	ref = ecs_get_id(f.clients[0].ecs, ra, f.clients[0].reference);
	CHECK(ref && *ref == ra);
	ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
	CHECK(ref && *ref == 0);
	fixture_fini(&f);
}

/* Hiding a target without writing its reference faults only that recipient. */
TEST(replication_hidden_reference_fault)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	set_reference(&f.server, a, b);
	capture(&f, 1);
	transfer(&f, 0);
	transfer(&f, 1);
	ecs_remove_id(f.server.ecs, b, f.server.ids.replicate_to_all);
	ecs_add_pair(f.server.ecs, b, f.server.ids.replicate_to, f.peers[0]);
	sync_capture_result_t progress;
	EQ(sync_capture(f.server.session, 2, &progress), SYNC_PARTIAL);
	CHECK(progress.tick == 2 && progress.peers_faulted == 1);
	EQ(progress.peers_queued, 1);
	sync_status_t fault = status(f.server.session, f.peers[1]);
	EQ(fault.state, SYNC_FAULTED);
	EQ(fault.fault.code, SYNC_ERR_REFERENCE);
	CHECK(fault.fault.peer == f.peers[1]);
	EQ(status(f.server.session, 0).state, SYNC_SERVER_READY);
	EQ(status(f.server.session, 0).faulted_peers, 1);
	transfer(&f, 0);
	expect_position(&f.clients[0], network_id(&f.server, b), 2);
	OK(sync_capture(f.server.session, 3, &progress));
	EQ(progress.peers_faulted, 0);
	EQ(progress.peers_queued, 1);
	fixture_fini(&f);
}

/* Clearing references in the same sample permits the target to depart. */
TEST(replication_reference_removal)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	set_reference(&f.server, a, b);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, b);
	set_reference(&f.server, a, 0);
	ecs_delete(f.server.ecs, b);
	capture(&f, 2);
	transfer(&f, 0);
	expect_absent(&f.clients[0], id);
	ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
	CHECK(ref && !*ref);
	fixture_fini(&f);
}

/* Mutations queued after a snapshot are applied without a sampling gap. */
TEST(replication_snapshot_delta_order)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	set_position(&f.server, e, 9);
	capture(&f, 2);
	transfer(&f, 0);
	expect_position(&f.clients[0], network_id(&f.server, e), 9);
	EQ(status(f.clients[0].session, 0).last_tick, 2);
	fixture_fini(&f);
}

/* Zero and repeated ticks reject before sampling; UINT64_MAX is legal. */
TEST(replication_tick_boundaries)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	sync_capture_result_t r = {99, 99, 99};
	EQ(sync_capture(f.server.session, 0, &r), SYNC_ERR_ARGUMENT);
	CHECK(!r.tick && !r.peers_queued && !r.peers_faulted);
	capture(&f, 7);
	EQ(sync_capture(f.server.session, 7, &r), SYNC_ERR_ARGUMENT);
	CHECK(!r.tick);
	EQ(sync_capture(f.server.session, 6, &r), SYNC_ERR_ARGUMENT);
	CHECK(!r.tick);
	EQ(status(f.server.session, 0).last_tick, 7);
	capture(&f, UINT64_MAX);
	EQ(sync_capture(f.server.session, UINT64_MAX, &r), SYNC_ERR_ARGUMENT);
	CHECK(!r.tick);
	fixture_fini(&f);
}

/* Capture with no peers still consumes its tick successfully. */
TEST(replication_no_peers)
{
	struct world w;
	world_init(&w, false);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	sync_capture_result_t r;
	OK(sync_capture(w.session, 1, &r));
	CHECK(r.tick == 1 && !r.peers_queued && !r.peers_faulted);
	world_fini(&w);
}

/* Client-local data survives value updates but is removed with its replica. */
TEST(replication_local_data_lifetime)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	uint64_t id = network_id(&f.server, e);
	ecs_entity_t r = replica(&f.clients[0], id);
	ecs_entity_t local_tag = ecs_new(f.clients[0].ecs);
	ecs_add_id(f.clients[0].ecs, r, local_tag);
	set_position(&f.server, e, 2);
	capture(&f, 2);
	transfer(&f, 0);
	CHECK(ecs_has_id(f.clients[0].ecs, r, local_tag));
	OK(sync_client_reset(f.clients[0].session));
	CHECK(!ecs_is_alive(f.clients[0].ecs, r));
	CHECK(ecs_is_alive(f.clients[0].ecs, local_tag));
	expect_absent(&f.clients[0], id);
	fixture_fini(&f);
}

/* Lookup rejects zero and null output without adopting unknown identities. */
TEST(replication_lookup_arguments)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = UINT64_MAX;
	EQ(sync_lookup(f.server.session, 0, &e), SYNC_ERR_ARGUMENT);
	CHECK(!e);
	EQ(sync_lookup(f.server.session, 1, NULL), SYNC_ERR_ARGUMENT);
	expect_absent(&f.server, UINT64_MAX);
	expect_absent(&f.clients[0], UINT64_MAX);
	fixture_fini(&f);
}

/* Destruction removes module IDs and replicas while gameplay survives. */
TEST(replication_session_teardown)
{
	struct fixture f;
	fixture_init(&f, NULL, NULL);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	capture(&f, 1);
	transfer(&f, 0);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	sync_session_destroy(f.server.session);
	f.server.session = NULL;
	CHECK(ecs_is_alive(f.server.ecs, e));
	CHECK(!ecs_has_id(f.server.ecs, e, f.server.ids.network_id));
	CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
	sync_session_destroy(f.clients[0].session);
	f.clients[0].session = NULL;
	CHECK(!ecs_is_alive(f.clients[0].ecs, r));
	CHECK(ecs_is_alive(f.clients[0].ecs, f.clients[0].local));
	OK(sync_server_create(f.server.ecs, NULL, &f.server.session, NULL));
	fixture_fini(&f);
}
