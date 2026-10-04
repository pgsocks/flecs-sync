/* Private procedural script; derived from native contracts. */
#include "test.h"
#include "protocol.h"

/* Snapshots route public and private entities to independent client worlds. */
TEST(replication_two_recipients)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 3731208083u);
	ecs_entity_t public = spawn(&f.server, 10, 0);
	cucumber_reply();

	cucumber_wait(4, 2133199032u);
	ecs_entity_t private[2];
	cucumber_reply();

	cucumber_wait(5, 2891265565u);
	for (unsigned i = 0; i < 2; i++)
		private[i] = spawn(&f.server, (float)i, f.peers[i]);
	cucumber_reply();

	cucumber_wait(6, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 2911432188u);
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
	cucumber_reply();

	cucumber_wait(8, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Tracked writes in a shared table reach both peers without losing rows. */
TEST(replication_tracked_writes)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1962381748u);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1106759385u);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(5, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(6, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(7, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(8, 4046807215u);
	set_position(&f.server, a, 3);
	cucumber_reply();

	cucumber_wait(9, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(10, 1436235832u);
	for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 3);
		expect_position(&f.clients[i], network_id(&f.server, b), 2);
	}
	cucumber_reply();

	cucumber_wait(11, 3342994067u);
	struct position *p = ecs_get_mut_id(f.server.ecs, a, f.server.position);
	cucumber_reply();

	cucumber_wait(12, 2007843856u);
	CHECK(p != NULL);
	cucumber_reply();

	cucumber_wait(13, 2585305620u);
	p->x = 4;
	cucumber_reply();

	cucumber_wait(14, 1214202096u);
	p->y = 5;
	cucumber_reply();

	cucumber_wait(15, 2321165375u);
	ecs_modified_id(f.server.ecs, a, f.server.position);
	cucumber_reply();

	cucumber_wait(16, 1291268845u);
	capture(&f, 3);
	cucumber_reply();

	cucumber_wait(17, 104033037u);
	for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 4);
	}
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Writable iteration reports changes after the iterator has completed. */
TEST(replication_writable_query)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 4141310276u);
	ecs_query_t *q = ecs_query_init(f.server.ecs, &(ecs_query_desc_t){
		.terms = {{ .id = f.server.position, .inout = EcsInOut }}
	});
	cucumber_reply();

	cucumber_wait(7, 3055786960u);
	CHECK(q != NULL);
	cucumber_reply();

	cucumber_wait(8, 317092755u);
	ecs_iter_t it = ecs_query_iter(f.server.ecs, q);
	cucumber_reply();

	cucumber_wait(9, 2533490985u);
	while (ecs_query_next(&it)) {
		struct position *p = ecs_field(&it, struct position, 0);
		for (int32_t i = 0; i < it.count; i++) {
			p[i].x = 8;
			p[i].y = 9;
		}
	}
	cucumber_reply();

	cucumber_wait(10, 1658269482u);
	ecs_query_fini(q);
	cucumber_reply();

	cucumber_wait(11, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 3679398294u);
	expect_position(&f.clients[0], network_id(&f.server, e), 8);
	cucumber_reply();

	cucumber_wait(14, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Visibility entry sends full state even after its value table is clean. */
TEST(replication_visibility_entry)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1505052690u);
	ecs_entity_t e = spawn(&f.server, 5, f.peers[0]);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(8, 1973082421u);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
	cucumber_reply();

	cucumber_wait(9, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(10, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(11, 2994724440u);
	expect_position(&f.clients[1], id, 5);
	cucumber_reply();

	cucumber_wait(12, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Exit deletes the replica; reentry rebuilds it under the same network ID. */
TEST(replication_exit_reentry)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2384967911u);
	ecs_entity_t e = spawn(&f.server, 5, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(7, 1432765860u);
	ecs_entity_t old = replica(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(8, 936362882u);
	ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(9, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(10, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(11, 1901023605u);
	expect_absent(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(12, 130567596u);
	CHECK(!ecs_is_alive(f.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(13, 1289379738u);
	CHECK(replica(&f.server, id) == e);
	cucumber_reply();

	cucumber_wait(14, 3183778016u);
	ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(15, 1291268845u);
	capture(&f, 3);
	cucumber_reply();

	cucumber_wait(16, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(17, 3199284908u);
	CHECK(network_id(&f.server, e) == id);
	cucumber_reply();

	cucumber_wait(18, 2115093378u);
	CHECK(replica(&f.clients[0], id) != old);
	cucumber_reply();

	cucumber_wait(19, 1945174424u);
	expect_position(&f.clients[0], id, 5);
	cucumber_reply();

	cucumber_wait(20, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Recipient pairs form a union with the public policy tag. */
TEST(replication_policy_union)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 3441868368u);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
	cucumber_reply();

	cucumber_wait(5, 1973082421u);
	ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
	cucumber_reply();

	cucumber_wait(6, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(8, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(9, 2571050076u);
	ecs_remove_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
	cucumber_reply();

	cucumber_wait(10, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(11, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(12, 3567129113u);
	expect_position(&f.clients[0], network_id(&f.server, e), 1);
	cucumber_reply();

	cucumber_wait(13, 936362882u);
	ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(14, 1291268845u);
	capture(&f, 3);
	cucumber_reply();

	cucumber_wait(15, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(16, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(17, 4103346256u);
	expect_absent(&f.clients[0], network_id(&f.server, e));
	cucumber_reply();

	cucumber_wait(18, 3798762218u);
	expect_position(&f.clients[1], network_id(&f.server, e), 1);
	cucumber_reply();

	cucumber_wait(19, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Prefabs, disabled entities, inherited values and absent policy stay out. */
TEST(replication_excluded_entities)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 3699322791u);
	ecs_entity_t prefab = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 3090781533u);
	ecs_add_id(f.server.ecs, prefab, EcsPrefab);
	cucumber_reply();

	cucumber_wait(5, 42607562u);
	ecs_entity_t disabled = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(6, 98084237u);
	ecs_add_id(f.server.ecs, disabled, EcsDisabled);
	cucumber_reply();

	cucumber_wait(7, 3135782401u);
	ecs_entity_t hidden = spawn(&f.server, 3, 0);
	cucumber_reply();

	cucumber_wait(8, 758738585u);
	ecs_remove_id(f.server.ecs, hidden, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(9, 2859392042u);
	ecs_entity_t inherited = ecs_new(f.server.ecs);
	cucumber_reply();

	cucumber_wait(10, 2540183241u);
	ecs_add_pair(f.server.ecs, inherited, EcsIsA, prefab);
	cucumber_reply();

	cucumber_wait(11, 2840331062u);
	ecs_add_id(f.server.ecs, inherited, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(12, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(13, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(14, 583471985u);
	EQ(status(f.clients[0].session, 0).entities, 0);
	cucumber_reply();

	cucumber_wait(15, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Tag-only entities replicate while arbitrary pairs and policy do not. */
TEST(replication_tag_and_pair_exclusion)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1880906126u);
	ecs_entity_t e = ecs_new(f.server.ecs);
	cucumber_reply();

	cucumber_wait(4, 1769413767u);
	ecs_add_id(f.server.ecs, e, f.server.tag);
	cucumber_reply();

	cucumber_wait(5, 3183778016u);
	ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(6, 1908506144u);
	ecs_add_pair(f.server.ecs, e, f.server.tag, f.server.local);
	cucumber_reply();

	cucumber_wait(7, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(8, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(9, 3826206884u);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	cucumber_reply();

	cucumber_wait(10, 3232257462u);
	CHECK(ecs_has_id(f.clients[0].ecs, r, f.clients[0].tag));
	cucumber_reply();

	cucumber_wait(11, 1332185602u);
	CHECK(!ecs_has_id(f.clients[0].ecs, r,
		f.clients[0].ids.replicate_to_all));
	cucumber_reply();

	cucumber_wait(12, 2191976087u);
	CHECK(!ecs_has_pair(f.clients[0].ecs, r,
		f.clients[0].tag, EcsWildcard));
	cucumber_reply();

	cucumber_wait(13, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Add/remove/delete mutations coalesce to final sampled membership. */
TEST(replication_coalescing)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(7, 1765259416u);
	ecs_remove_id(f.server.ecs, e, f.server.position);
	cucumber_reply();

	cucumber_wait(8, 1877685308u);
	set_position(&f.server, e, 4);
	cucumber_reply();

	cucumber_wait(9, 1769413767u);
	ecs_add_id(f.server.ecs, e, f.server.tag);
	cucumber_reply();

	cucumber_wait(10, 2036335689u);
	ecs_remove_id(f.server.ecs, e, f.server.tag);
	cucumber_reply();

	cucumber_wait(11, 2743998536u);
	ecs_entity_t transient = spawn(&f.server, 8, 0);
	cucumber_reply();

	cucumber_wait(12, 3919757733u);
	ecs_delete(f.server.ecs, transient);
	cucumber_reply();

	cucumber_wait(13, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(14, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(15, 1915972527u);
	expect_position(&f.clients[0], id, 4);
	cucumber_reply();

	cucumber_wait(16, 1494544910u);
	CHECK(!ecs_has_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].tag));
	cucumber_reply();

	cucumber_wait(17, 587561286u);
	EQ(status(f.clients[0].session, 0).entities, 1);
	cucumber_reply();

	cucumber_wait(18, 1765259416u);
	ecs_remove_id(f.server.ecs, e, f.server.position);
	cucumber_reply();

	cucumber_wait(19, 1291268845u);
	capture(&f, 3);
	cucumber_reply();

	cucumber_wait(20, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(21, 1901023605u);
	expect_absent(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(22, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Deleting and recycling a Flecs slot cannot recycle its network identity. */
TEST(replication_native_id_reuse)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2763299836u);
	ecs_entity_t old = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 596260149u);
	uint64_t id = network_id(&f.server, old);
	cucumber_reply();

	cucumber_wait(7, 1343304691u);
	ecs_delete(f.server.ecs, old);
	cucumber_reply();

	cucumber_wait(8, 2103595095u);
	ecs_entity_t replacement = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(9, 737740980u);
	CHECK((uint32_t)old == (uint32_t)replacement);
	cucumber_reply();

	cucumber_wait(10, 519706753u);
	CHECK(old != replacement);
	cucumber_reply();

	cucumber_wait(11, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 132884762u);
	uint64_t new_id = network_id(&f.server, replacement);
	cucumber_reply();

	cucumber_wait(14, 1681411209u);
	CHECK(new_id != id);
	cucumber_reply();

	cucumber_wait(15, 3336462118u);
	expect_absent(&f.server, id);
	cucumber_reply();

	cucumber_wait(16, 1901023605u);
	expect_absent(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(17, 682923996u);
	expect_position(&f.clients[0], new_id, 2);
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

static void set_reference(struct world *w, ecs_entity_t e, ecs_entity_t target)
{
	ecs_set_id(w->ecs, e, w->reference, sizeof(target), &target);
}

/* Forward, cyclic, self and null references resolve to client-local IDs. */
TEST(replication_reference_mapping)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1962381748u);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1106759385u);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(5, 2891386649u);
	set_reference(&f.server, a, b);
	cucumber_reply();

	cucumber_wait(6, 679855086u);
	set_reference(&f.server, b, a);
	cucumber_reply();

	cucumber_wait(7, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(8, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(9, 4005897680u);
	ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
	cucumber_reply();

	cucumber_wait(10, 2132368125u);
	ecs_entity_t rb = replica(&f.clients[0], network_id(&f.server, b));
	cucumber_reply();

	cucumber_wait(11, 3727529367u);
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(12, 1230310763u);
	CHECK(ref && *ref == rb);
	cucumber_reply();

	cucumber_wait(13, 2525632940u);
	ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(14, 1259582258u);
	CHECK(ref && *ref == ra);
	cucumber_reply();

	cucumber_wait(15, 2920395072u);
	set_reference(&f.server, a, a);
	cucumber_reply();

	cucumber_wait(16, 1174112105u);
	set_reference(&f.server, b, 0);
	cucumber_reply();

	cucumber_wait(17, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(18, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(19, 622736495u);
	ref = ecs_get_id(f.clients[0].ecs, ra, f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(20, 1259582258u);
	CHECK(ref && *ref == ra);
	cucumber_reply();

	cucumber_wait(21, 2525632940u);
	ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(22, 2685069523u);
	CHECK(ref && *ref == 0);
	cucumber_reply();

	cucumber_wait(23, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Hiding a target without writing its reference faults only that recipient. */
TEST(replication_hidden_reference_fault)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1962381748u);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1106759385u);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(5, 2891386649u);
	set_reference(&f.server, a, b);
	cucumber_reply();

	cucumber_wait(6, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(8, 2188067086u);
	transfer(&f, 1);
	cucumber_reply();

	cucumber_wait(9, 3359667200u);
	ecs_remove_id(f.server.ecs, b, f.server.ids.replicate_to_all);
	cucumber_reply();

	cucumber_wait(10, 476589144u);
	ecs_add_pair(f.server.ecs, b, f.server.ids.replicate_to, f.peers[0]);
	cucumber_reply();

	cucumber_wait(11, 4179896338u);
	sync_capture_result_t progress;
	cucumber_reply();

	cucumber_wait(12, 3491848255u);
	EQ(sync_capture(f.server.session, 2, &progress), SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(13, 751370838u);
	CHECK(progress.tick == 2 && progress.peers_faulted == 1);
	cucumber_reply();

	cucumber_wait(14, 192008880u);
	EQ(progress.peers_queued, 1);
	cucumber_reply();

	cucumber_wait(15, 3848971567u);
	sync_status_t fault = status(f.server.session, f.peers[1]);
	cucumber_reply();

	cucumber_wait(16, 3797551683u);
	EQ(fault.state, SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(17, 3845482731u);
	EQ(fault.fault.code, SYNC_ERR_REFERENCE);
	cucumber_reply();

	cucumber_wait(18, 1113539928u);
	CHECK(fault.fault.peer == f.peers[1]);
	cucumber_reply();

	cucumber_wait(19, 4120937702u);
	EQ(status(f.server.session, 0).state, SYNC_SERVER_READY);
	cucumber_reply();

	cucumber_wait(20, 4089356648u);
	EQ(status(f.server.session, 0).faulted_peers, 1);
	cucumber_reply();

	cucumber_wait(21, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(22, 327127758u);
	expect_position(&f.clients[0], network_id(&f.server, b), 2);
	cucumber_reply();

	cucumber_wait(23, 1235051796u);
	OK(sync_capture(f.server.session, 3, &progress));
	cucumber_reply();

	cucumber_wait(24, 1513784792u);
	EQ(progress.peers_faulted, 0);
	cucumber_reply();

	cucumber_wait(25, 192008880u);
	EQ(progress.peers_queued, 1);
	cucumber_reply();

	cucumber_wait(26, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Clearing references in the same sample permits the target to depart. */
TEST(replication_reference_removal)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 1962381748u);
	ecs_entity_t a = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1106759385u);
	ecs_entity_t b = spawn(&f.server, 2, 0);
	cucumber_reply();

	cucumber_wait(5, 2891386649u);
	set_reference(&f.server, a, b);
	cucumber_reply();

	cucumber_wait(6, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(7, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(8, 4210621122u);
	uint64_t id = network_id(&f.server, b);
	cucumber_reply();

	cucumber_wait(9, 3278894535u);
	set_reference(&f.server, a, 0);
	cucumber_reply();

	cucumber_wait(10, 1483866668u);
	ecs_delete(f.server.ecs, b);
	cucumber_reply();

	cucumber_wait(11, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 1901023605u);
	expect_absent(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(14, 4005897680u);
	ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
	cucumber_reply();

	cucumber_wait(15, 3727529367u);
	const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
	cucumber_reply();

	cucumber_wait(16, 1486836892u);
	CHECK(ref && !*ref);
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Mutations queued after a snapshot are applied without a sampling gap. */
TEST(replication_snapshot_delta_order)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 1731438447u);
	set_position(&f.server, e, 9);
	cucumber_reply();

	cucumber_wait(6, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(7, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(8, 3666690977u);
	expect_position(&f.clients[0], network_id(&f.server, e), 9);
	cucumber_reply();

	cucumber_wait(9, 3917849262u);
	EQ(status(f.clients[0].session, 0).last_tick, 2);
	cucumber_reply();

	cucumber_wait(10, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Zero and repeated ticks reject before sampling; UINT64_MAX is legal. */
TEST(replication_tick_boundaries)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 2416248520u);
	sync_capture_result_t r = {99, 99, 99};
	cucumber_reply();

	cucumber_wait(4, 778962076u);
	EQ(sync_capture(f.server.session, 0, &r), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 31064418u);
	CHECK(!r.tick && !r.peers_queued && !r.peers_faulted);
	cucumber_reply();

	cucumber_wait(6, 1274976817u);
	capture(&f, 7);
	cucumber_reply();

	cucumber_wait(7, 4240130586u);
	EQ(sync_capture(f.server.session, 7, &r), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(8, 1220113367u);
	CHECK(!r.tick);
	cucumber_reply();

	cucumber_wait(9, 2453100891u);
	EQ(sync_capture(f.server.session, 6, &r), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(10, 1220113367u);
	CHECK(!r.tick);
	cucumber_reply();

	cucumber_wait(11, 3905219979u);
	EQ(status(f.server.session, 0).last_tick, 7);
	cucumber_reply();

	cucumber_wait(12, 1841373914u);
	capture(&f, UINT64_MAX);
	cucumber_reply();

	cucumber_wait(13, 1347566613u);
	EQ(sync_capture(f.server.session, UINT64_MAX, &r), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(14, 1220113367u);
	CHECK(!r.tick);
	cucumber_reply();

	cucumber_wait(15, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Capture with no peers still consumes its tick successfully. */
TEST(replication_no_peers)
{
	cucumber_wait(1, 2099926429u);
	struct world w;
	cucumber_reply();

	cucumber_wait(2, 3087103547u);
	world_init(&w, false);
	cucumber_reply();

	cucumber_wait(3, 2787170963u);
	OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
	cucumber_reply();

	cucumber_wait(4, 2675926083u);
	sync_capture_result_t r;
	cucumber_reply();

	cucumber_wait(5, 825498325u);
	OK(sync_capture(w.session, 1, &r));
	cucumber_reply();

	cucumber_wait(6, 841877803u);
	CHECK(r.tick == 1 && !r.peers_queued && !r.peers_faulted);
	cucumber_reply();

	cucumber_wait(7, 194087710u);
	world_fini(&w);
	cucumber_reply();
}

/* Client-local data survives value updates but is removed with its replica. */
TEST(replication_local_data_lifetime)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 4290243655u);
	uint64_t id = network_id(&f.server, e);
	cucumber_reply();

	cucumber_wait(7, 2793458385u);
	ecs_entity_t r = replica(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(8, 3281705794u);
	ecs_entity_t local_tag = ecs_new(f.clients[0].ecs);
	cucumber_reply();

	cucumber_wait(9, 1667771162u);
	ecs_add_id(f.clients[0].ecs, r, local_tag);
	cucumber_reply();

	cucumber_wait(10, 1801865358u);
	set_position(&f.server, e, 2);
	cucumber_reply();

	cucumber_wait(11, 1295341786u);
	capture(&f, 2);
	cucumber_reply();

	cucumber_wait(12, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(13, 4123145226u);
	CHECK(ecs_has_id(f.clients[0].ecs, r, local_tag));
	cucumber_reply();

	cucumber_wait(14, 443638422u);
	OK(sync_client_reset(f.clients[0].session));
	cucumber_reply();

	cucumber_wait(15, 2226107912u);
	CHECK(!ecs_is_alive(f.clients[0].ecs, r));
	cucumber_reply();

	cucumber_wait(16, 3047381719u);
	CHECK(ecs_is_alive(f.clients[0].ecs, local_tag));
	cucumber_reply();

	cucumber_wait(17, 1901023605u);
	expect_absent(&f.clients[0], id);
	cucumber_reply();

	cucumber_wait(18, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Lookup rejects zero and null output without adopting unknown identities. */
TEST(replication_lookup_arguments)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 4017309184u);
	ecs_entity_t e = UINT64_MAX;
	cucumber_reply();

	cucumber_wait(4, 1682939751u);
	EQ(sync_lookup(f.server.session, 0, &e), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 3491954578u);
	CHECK(!e);
	cucumber_reply();

	cucumber_wait(6, 3542237076u);
	EQ(sync_lookup(f.server.session, 1, NULL), SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 3681709589u);
	expect_absent(&f.server, UINT64_MAX);
	cucumber_reply();

	cucumber_wait(8, 607776911u);
	expect_absent(&f.clients[0], UINT64_MAX);
	cucumber_reply();

	cucumber_wait(9, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}

/* Destruction removes module IDs and replicas while gameplay survives. */
TEST(replication_session_teardown)
{
	cucumber_wait(1, 36100305u);
	struct fixture f;
	cucumber_reply();

	cucumber_wait(2, 3238342537u);
	fixture_init(&f, NULL, NULL);
	cucumber_reply();

	cucumber_wait(3, 364313329u);
	ecs_entity_t e = spawn(&f.server, 1, 0);
	cucumber_reply();

	cucumber_wait(4, 1332995715u);
	capture(&f, 1);
	cucumber_reply();

	cucumber_wait(5, 2208901945u);
	transfer(&f, 0);
	cucumber_reply();

	cucumber_wait(6, 3826206884u);
	ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
	cucumber_reply();

	cucumber_wait(7, 2795851496u);
	sync_session_destroy(f.server.session);
	cucumber_reply();

	cucumber_wait(8, 276200796u);
	f.server.session = NULL;
	cucumber_reply();

	cucumber_wait(9, 801986897u);
	CHECK(ecs_is_alive(f.server.ecs, e));
	cucumber_reply();

	cucumber_wait(10, 2922842868u);
	CHECK(!ecs_has_id(f.server.ecs, e, f.server.ids.network_id));
	cucumber_reply();

	cucumber_wait(11, 2503171111u);
	CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
	cucumber_reply();

	cucumber_wait(12, 2778804864u);
	sync_session_destroy(f.clients[0].session);
	cucumber_reply();

	cucumber_wait(13, 970535443u);
	f.clients[0].session = NULL;
	cucumber_reply();

	cucumber_wait(14, 2226107912u);
	CHECK(!ecs_is_alive(f.clients[0].ecs, r));
	cucumber_reply();

	cucumber_wait(15, 2694942283u);
	CHECK(ecs_is_alive(f.clients[0].ecs, f.clients[0].local));
	cucumber_reply();

	cucumber_wait(16, 2427681867u);
	OK(sync_server_create(f.server.ecs, NULL, &f.server.session, NULL));
	cucumber_reply();

	cucumber_wait(17, 477425722u);
	fixture_fini(&f);
	cucumber_reply();
}
