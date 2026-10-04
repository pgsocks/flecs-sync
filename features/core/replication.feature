Feature: replication contracts

@core
Scenario: replication_two_recipients
	# When public and peer-private entities are captured and delivered
	# Then Snapshots route public and private entities to independent client
	# worlds.
	Given the "core" workflow "replication_two_recipients"
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
		ecs_entity_t public = spawn(&f.server, 10, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t private[2];
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		for (unsigned i = 0; i < 2; i++)
		private[i] = spawn(&f.server, (float)i, f.peers[i]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
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
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_tracked_writes
	# When set and modified operations change one row of a shared table
	# Then Tracked writes in a shared table reach both peers without losing rows.
	Given the "core" workflow "replication_tracked_writes"
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
		ecs_entity_t a = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t b = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		set_position(&f.server, a, 3);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 3);
		expect_position(&f.clients[i], network_id(&f.server, b), 2);
		}
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		struct position *p = ecs_get_mut_id(f.server.ecs, a, f.server.position);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(p != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		p->x = 4;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		p->y = 5;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		ecs_modified_id(f.server.ecs, a, f.server.position);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		capture(&f, 3);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		for (unsigned i = 0; i < 2; i++) {
		transfer(&f, i);
		expect_position(&f.clients[i], network_id(&f.server, a), 4);
		}
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_writable_query
	# When an inout query writes values before the next capture
	# Then Writable iteration reports changes after the iterator has completed.
	Given the "core" workflow "replication_writable_query"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_query_t *q = ecs_query_init(f.server.ecs, &(ecs_query_desc_t){
		.terms = {{ .id = f.server.position, .inout = EcsInOut }}
		});
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(q != NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_iter_t it = ecs_query_iter(f.server.ecs, q);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		while (ecs_query_next(&it)) {
		struct position *p = ecs_field(&it, struct position, 0);
		for (int32_t i = 0; i < it.count; i++) {
		p[i].x = 8;
		p[i].y = 9;
		}
		}
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_query_fini(q);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 8);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_visibility_entry
	# When a clean entity gains another recipient policy pair
	# Then Visibility entry sends full state even after its value table is clean.
	Given the "core" workflow "replication_visibility_entry"
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
		ecs_entity_t e = spawn(&f.server, 5, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		expect_position(&f.clients[1], id, 5);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_exit_reentry
	# When an entity becomes hidden and then visible again
	# Then Exit deletes the replica; reentry rebuilds it under the same network ID.
	Given the "core" workflow "replication_exit_reentry"
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
		ecs_entity_t e = spawn(&f.server, 5, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t old = replica(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		expect_absent(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!ecs_is_alive(f.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(replica(&f.server, id) == e);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		capture(&f, 3);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		CHECK(network_id(&f.server, e) == id);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(replica(&f.clients[0], id) != old);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		expect_position(&f.clients[0], id, 5);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_policy_union
	# When direct recipient pairs and public policy are removed in turn
	# Then Recipient pairs form a union with the public policy tag.
	Given the "core" workflow "replication_policy_union"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_remove_pair(f.server.ecs, e, f.server.ids.replicate_to, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_remove_id(f.server.ecs, e, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		capture(&f, 3);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		expect_absent(&f.clients[0], network_id(&f.server, e));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		expect_position(&f.clients[1], network_id(&f.server, e), 1);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_excluded_entities
	# When excluded entities and inherited data are captured
	# Then Prefabs, disabled entities, inherited values and absent policy stay out.
	Given the "core" workflow "replication_excluded_entities"
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
		ecs_entity_t prefab = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_add_id(f.server.ecs, prefab, EcsPrefab);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t disabled = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_add_id(f.server.ecs, disabled, EcsDisabled);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t hidden = spawn(&f.server, 3, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_remove_id(f.server.ecs, hidden, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_entity_t inherited = ecs_new(f.server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_add_pair(f.server.ecs, inherited, EcsIsA, prefab);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_add_id(f.server.ecs, inherited, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(status(f.clients[0].session, 0).entities, 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_tag_and_pair_exclusion
	# When a tag-only entity also owns an arbitrary gameplay pair
	# Then Tag-only entities replicate while arbitrary pairs and policy do not.
	Given the "core" workflow "replication_tag_and_pair_exclusion"
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
		ecs_entity_t e = ecs_new(f.server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_add_id(f.server.ecs, e, f.server.tag);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_id(f.server.ecs, e, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_add_pair(f.server.ecs, e, f.server.tag, f.server.local);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(ecs_has_id(f.clients[0].ecs, r, f.clients[0].tag));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!ecs_has_id(f.clients[0].ecs, r,
		f.clients[0].ids.replicate_to_all));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(!ecs_has_pair(f.clients[0].ecs, r,
		f.clients[0].tag, EcsWildcard));
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_coalescing
	# When components are removed and readded between samples
	# Then Add/remove/delete mutations coalesce to final sampled membership.
	Given the "core" workflow "replication_coalescing"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_remove_id(f.server.ecs, e, f.server.position);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		set_position(&f.server, e, 4);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_add_id(f.server.ecs, e, f.server.tag);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_remove_id(f.server.ecs, e, f.server.tag);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_entity_t transient = spawn(&f.server, 8, 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ecs_delete(f.server.ecs, transient);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		expect_position(&f.clients[0], id, 4);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(!ecs_has_id(f.clients[0].ecs,
		replica(&f.clients[0], id), f.clients[0].tag));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(status(f.clients[0].session, 0).entities, 1);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		ecs_remove_id(f.server.ecs, e, f.server.position);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		capture(&f, 3);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		expect_absent(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_native_id_reuse
	# When a deleted server entity slot is reused by a new entity
	# Then Deleting and recycling a Flecs slot cannot recycle its network identity.
	Given the "core" workflow "replication_native_id_reuse"
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
		ecs_entity_t old = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint64_t id = network_id(&f.server, old);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_delete(f.server.ecs, old);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t replacement = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK((uint32_t)old == (uint32_t)replacement);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(old != replacement);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		uint64_t new_id = network_id(&f.server, replacement);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(new_id != id);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		expect_absent(&f.server, id);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		expect_absent(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		expect_position(&f.clients[0], new_id, 2);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_reference_mapping
	# When new entities reference each other and later themselves
	# Then Forward, cyclic, self and null references resolve to client-local IDs.
	Given the "core" workflow "replication_reference_mapping"
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
		ecs_entity_t a = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t b = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_reference(&f.server, a, b);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		set_reference(&f.server, b, a);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_entity_t rb = replica(&f.clients[0], network_id(&f.server, b));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		CHECK(ref && *ref == rb);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(ref && *ref == ra);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		set_reference(&f.server, a, a);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		set_reference(&f.server, b, 0);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		ref = ecs_get_id(f.clients[0].ecs, ra, f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		CHECK(ref && *ref == ra);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		ref = ecs_get_id(f.clients[0].ecs, rb, f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		CHECK(ref && *ref == 0);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_hidden_reference_fault
	# When a referenced target loses visibility for one peer
	# Then Hiding a target without writing its reference faults only that
	# recipient.
	Given the "core" workflow "replication_hidden_reference_fault"
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
		ecs_entity_t a = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t b = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_reference(&f.server, a, b);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_remove_id(f.server.ecs, b, f.server.ids.replicate_to_all);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_add_pair(f.server.ecs, b, f.server.ids.replicate_to, f.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		sync_capture_result_t progress;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		EQ(sync_capture(f.server.session, 2, &progress), SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(progress.tick == 2 && progress.peers_faulted == 1);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(progress.peers_queued, 1);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		sync_status_t fault = status(f.server.session, f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(fault.state, SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		EQ(fault.fault.code, SYNC_ERR_REFERENCE);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		CHECK(fault.fault.peer == f.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		EQ(status(f.server.session, 0).state, SYNC_SERVER_READY);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		EQ(status(f.server.session, 0).faulted_peers, 1);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		expect_position(&f.clients[0], network_id(&f.server, b), 2);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		OK(sync_capture(f.server.session, 3, &progress));
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		EQ(progress.peers_faulted, 0);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		EQ(progress.peers_queued, 1);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_reference_removal
	# When a reference is cleared in the sample deleting its target
	# Then Clearing references in the same sample permits the target to depart.
	Given the "core" workflow "replication_reference_removal"
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
		ecs_entity_t a = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_entity_t b = spawn(&f.server, 2, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_reference(&f.server, a, b);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		uint64_t id = network_id(&f.server, b);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		set_reference(&f.server, a, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_delete(f.server.ecs, b);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		expect_absent(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_entity_t ra = replica(&f.clients[0], network_id(&f.server, a));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		const ecs_entity_t *ref = ecs_get_id(f.clients[0].ecs,
		ra, f.clients[0].reference);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(ref && !*ref);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_snapshot_delta_order
	# When a value changes before its snapshot has been delivered
	# Then Mutations queued after a snapshot are applied without a sampling gap.
	Given the "core" workflow "replication_snapshot_delta_order"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		set_position(&f.server, e, 9);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		expect_position(&f.clients[0], network_id(&f.server, e), 9);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).last_tick, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_tick_boundaries
	# When capture receives zero, repeated, older and maximum ticks
	# Then Zero and repeated ticks reject before sampling; UINT64_MAX is legal.
	Given the "core" workflow "replication_tick_boundaries"
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
		sync_capture_result_t r = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_capture(f.server.session, 0, &r), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(!r.tick && !r.peers_queued && !r.peers_faulted);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		capture(&f, 7);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(sync_capture(f.server.session, 7, &r), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(!r.tick);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_capture(f.server.session, 6, &r), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(!r.tick);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		EQ(status(f.server.session, 0).last_tick, 7);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		capture(&f, UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		EQ(sync_capture(f.server.session, UINT64_MAX, &r), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!r.tick);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_no_peers
	# When a server captures without recipients
	# Then Capture with no peers still consumes its tick successfully.
	Given the "core" workflow "replication_no_peers"
	When I perform operation 1:
		"""
		struct world w;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_init(&w, false);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		OK(sync_server_create(w.ecs, NULL, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_capture_result_t r;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_capture(w.session, 1, &r));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(r.tick == 1 && !r.peers_queued && !r.peers_faulted);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: replication_local_data_lifetime
	# When a replica with local data is updated and then reset
	# Then Client-local data survives value updates but is removed with its
	# replica.
	Given the "core" workflow "replication_local_data_lifetime"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint64_t id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t r = replica(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_entity_t local_tag = ecs_new(f.clients[0].ecs);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_add_id(f.clients[0].ecs, r, local_tag);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		set_position(&f.server, e, 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		capture(&f, 2);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(ecs_has_id(f.clients[0].ecs, r, local_tag));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		OK(sync_client_reset(f.clients[0].session));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(!ecs_is_alive(f.clients[0].ecs, r));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		CHECK(ecs_is_alive(f.clients[0].ecs, local_tag));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		expect_absent(&f.clients[0], id);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_lookup_arguments
	# When lookup receives zero, unknown IDs or a null output
	# Then Lookup rejects zero and null output without adopting unknown identities.
	Given the "core" workflow "replication_lookup_arguments"
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
		ecs_entity_t e = UINT64_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		EQ(sync_lookup(f.server.session, 0, &e), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		CHECK(!e);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_lookup(f.server.session, 1, NULL), SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		expect_absent(&f.server, UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		expect_absent(&f.clients[0], UINT64_MAX);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: replication_session_teardown
	# When sessions owning peers and replicas are destroyed
	# Then Destruction removes module IDs and replicas while gameplay survives.
	Given the "core" workflow "replication_session_teardown"
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
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		capture(&f, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_entity_t r = replica(&f.clients[0], network_id(&f.server, e));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_session_destroy(f.server.session);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		f.server.session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(ecs_is_alive(f.server.ecs, e));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(!ecs_has_id(f.server.ecs, e, f.server.ids.network_id));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(!ecs_is_alive(f.server.ecs, f.peers[0]));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_session_destroy(f.clients[0].session);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		f.clients[0].session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(!ecs_is_alive(f.clients[0].ecs, r));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		CHECK(ecs_is_alive(f.clients[0].ecs, f.clients[0].local));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		OK(sync_server_create(f.server.ecs, NULL, &f.server.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
