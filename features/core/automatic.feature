Feature: automatic contracts

@core
Scenario: automatic_manual_rejected
	# When manual capture and apply are called on automatic sessions
	# Then Automatic sessions reject public manual operations without progress.
	Given the "core" workflow "automatic_manual_rejected"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_init(&f, &s, &c);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_capture_result_t cr = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(sync_capture(f.server.session, 1, &cr), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(!cr.tick && !cr.peers_queued && !cr.peers_faulted);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		sync_apply_result_t ar = {99, 99, 99};
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(sync_apply(f.clients[0].session, 1, &ar), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(!ar.updates_applied && !ar.frames_consumed && !ar.last_tick);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_default_rate
	# When simulation advances by twenty then forty milliseconds
	# Then Default capture rate follows simulation time and starts with tick one.
	Given the "core" workflow "automatic_default_rate"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.02f));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(status(f.server.session, 0).last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.04f));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_stall_remainder
	# When a long simulation step is followed by two short steps
	# Then A stalled progress captures once and retains only the interval
	# remainder.
	Given the "core" workflow "automatic_stall_remainder"
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
		CHECK(ecs_progress(f.server.ecs, 1.0625f));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.03125f));
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.03125f));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.server.session, 0).last_tick, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_rate_reset
	# When the rate changes after half of the old interval has elapsed
	# Then Rate changes clear accumulated time and do not immediately capture.
	Given the "core" workflow "automatic_rate_reset"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC, .send_hz = 4 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		fixture_init(&f, &d, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_set_rate(f.server.session, 8));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		EQ(status(f.server.session, 0).last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.0625f));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(status(f.server.session, 0).last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.0625f));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_invalid_rate
	# When invalid rates are supplied with time already accumulated
	# Then Invalid rate changes preserve the previous interval and accumulator.
	Given the "core" workflow "automatic_invalid_rate"
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
		CHECK(ecs_progress(f.server.ecs, 0.0625f));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		double invalid[] = {0, -1, INFINITY, NAN, DBL_TRUE_MIN};
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
		EQ(sync_set_rate(f.server.session, invalid[i]),
		SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.0625f));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_rate_on_manual
	# When a manual server receives a rate-change request
	# Then Manual server rates cannot be changed through the automatic interface.
	Given the "core" workflow "automatic_rate_on_manual"
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
		EQ(sync_set_rate(f.server.session, 20), SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_default_budget
	# When three updates await a default automatic client
	# Then Default automatic application publishes one complete update per
	# progress.
	Given the "core" workflow "automatic_default_budget"
	When I perform operation 1:
		"""
		client_budget(0);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_custom_budget
	# When three updates await an automatic client with budget two
	# Then Explicit automatic budgets allow exactly the requested update count.
	Given the "core" workflow "automatic_custom_budget"
	When I perform operation 1:
		"""
		client_budget(2);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_phase_order
	# When writers, delivery systems and client readers run in phases
	# Then Writers precede capture; receive precedes apply; readers see full state.
	Given the "core" workflow "automatic_phase_order"
	When I perform operation 1:
		"""
		struct fixture f;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		sync_server_desc_t s = { .schedule = SYNC_AUTOMATIC, .send_hz = 8 };
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		sync_client_desc_t c = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		fixture_init(&f, &s, &c);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t e = spawn(&f.server, 1, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, EcsPostUpdate),
		.query.terms = {{ .id = f.server.position, .inout = EcsOut }},
		.callback = writer
		});
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		struct schedule_context ctx = { .f = &f };
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_system_init(f.server.ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.server, f.server.ids.send_phase),
		.callback = send_system, .ctx = &ctx
		});
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0],
		f.clients[0].ids.receive_phase),
		.callback = receive_system, .ctx = &ctx
		});
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_system_init(f.clients[0].ecs, &(ecs_system_desc_t){
		.entity = system_entity(&f.clients[0], EcsPreUpdate),
		.callback = reader, .ctx = &ctx
		});
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ctx.id = network_id(&f.server, e);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(ecs_progress(f.clients[0].ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		CHECK(ctx.received && ctx.read);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_cycle_rejected
	# When automatic setup would close a phase dependency cycle
	# Then Automatic setup rejects dependencies that would create a phase cycle.
	Given the "core" workflow "automatic_cycle_rejected"
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
		ecs_add_pair(w.ecs, EcsPostUpdate, EcsDependsOn, w.ids.capture_phase);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		sync_result_t r = sync_server_create(w.ecs, &d, &w.session, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(r < 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		CHECK(w.session == NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_teardown
	# When the world progresses after its automatic session is destroyed
	# Then Destroyed automatic sessions leave no core systems running on progress.
	Given the "core" workflow "automatic_teardown"
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
		int32_t count = ecs_count_id(w.ecs, EcsSystem);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_server_desc_t d = { .schedule = SYNC_AUTOMATIC };
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		OK(sync_server_create(w.ecs, &d, &w.session, NULL));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(ecs_count_id(w.ecs, EcsSystem) > count);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_session_destroy(w.session);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		w.session = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(ecs_count_id(w.ecs, EcsSystem), count);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		CHECK(ecs_progress(w.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		world_fini(&w);
		"""
	Then its observed values match the contract

@core
Scenario: automatic_custom_pipeline
	# When a custom pipeline first excludes and then includes Sync
	# Then Custom pipelines must select Sync systems and preserve phase ordering.
	Given the "core" workflow "automatic_custom_pipeline"
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
		ecs_entity_t unrelated = ecs_new(f.server.ecs);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t omitted = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query.terms = {
		{ .id = EcsSystem }, { .id = unrelated }
		} });
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		CHECK(omitted != 0);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_set_pipeline(f.server.ecs, omitted);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		EQ(status(f.server.session, 0).last_tick, 0);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_entity_t included = ecs_pipeline_init(f.server.ecs,
		&(ecs_pipeline_desc_t){ .query = {
		.terms = {
		{ .id = EcsSystem },
		{ .id = EcsPhase, .src.id = EcsCascade,
		.trav = EcsDependsOn }
		}, .order_by_callback = system_order
		} });
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		CHECK(included != 0);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ecs_set_pipeline(f.server.ecs, included);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		CHECK(ecs_progress(f.server.ecs, 0.125f));
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		EQ(status(f.server.session, 0).last_tick, 1);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		transfer(&f, 0);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		EQ(status(f.clients[0].session, 0).state, SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		fixture_fini(&f);
		"""
	Then its observed values match the contract
