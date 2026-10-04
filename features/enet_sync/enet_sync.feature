Feature: EnetSync authoritative dedicated-host delivery

@enet_sync
Scenario: invalid_calls
	# Given independent reflected worlds and dedicated ENet transports
	# When calls receive null pointers or invalid scalar inputs
	# Then argument errors leave no resources
	Given the "enet_sync" workflow "invalid_calls"
	When I perform operation 1:
		"""
		assert(enet_sync_server_listen(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		assert(enet_sync_client_connect(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		assert(enet_sync_server_poll(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(enet_sync_server_send(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_service(NULL, 1, 1, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_client_service(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_disconnect(NULL, 1, 0) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_disconnect(NULL, 0) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_client_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: initialized_outputs
	# Given independent reflected worlds and dedicated ENet transports
	# When calls fail before work
	# Then every provided output is initialized to zero
	Given the "enet_sync" workflow "initialized_outputs"
	When I perform operation 1:
		"""
		enet_sync_status_t status, zero = {0};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		memset(&status, 0xff, sizeof(status));
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		assert(enet_sync_server_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(!memcmp(&status, &zero, sizeof(status)));
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		memset(&status, 0xff, sizeof(status));
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_client_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(!memcmp(&status, &zero, sizeof(status)));
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		enet_sync_progress_t p, empty = {0};
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		memset(&p, 0xff, sizeof(p));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_send(NULL, 0, &p) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(!memcmp(&p, &empty, sizeof(p)));
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		enet_sync_server_t *server = (void *)1;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		enet_sync_error_t e, success = {0};
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		memset(&e, 0xff, sizeof(e));
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_server_listen(NULL, &server, &e) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(!server && !memcmp(&e, &success, sizeof(e)));
		"""
	Then its observed values match the contract

@enet_sync
Scenario: null_cleanup
	# Given independent reflected worlds and dedicated ENet transports
	# When null handles are closed
	# Then cleanup succeeds without side effects
	Given the "enet_sync" workflow "null_cleanup"
	When I perform operation 1:
		"""
		assert(enet_sync_server_close(NULL, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		assert(enet_sync_client_close(NULL, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: options
	# Given independent reflected worlds and dedicated ENet transports
	# When copied descriptors use invalid roles modes or limits
	# Then valid options are copied and invalid configurations fail
	Given the "enet_sync" workflow "options"
	When I perform operation 1:
		"""
		struct world server, client;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_open(&client, false, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_server_t *s;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		enet_sync_client_t *c;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_server_desc_t d = {.session = server.session};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		d.options.schedule = (sync_schedule_t)99;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		d.options.schedule = SYNC_MANUAL;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		d.options.events_per_progress = 1;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		d.options.events_per_progress = 0;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		d.options.frames_per_peer = 1;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		d.options.frames_per_peer = 0;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		d.options.transport_limits.max_waiting_bytes = 1;
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		d.options.transport_limits.max_waiting_bytes = 0;
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		d.session = client.session;
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		d.session = server.session;
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		d.max_clients = UINT32_MAX;
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		d.max_clients = 2;
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		memset(&d, 0xff, sizeof(d));
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		assert(status.state == ENET_SYNC_OPENING && status.host);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		assert(!es_hosts_live);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		ecs_progress(server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		assert(enet_sync_server_poll(s, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 33:
		"""
		assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 34:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && status.address.port);
		"""
	Then its observed values match the contract
	When I perform operation 35:
		"""
		assert(status.schedule == SYNC_MANUAL && !status.connections);
		"""
	Then its observed values match the contract
	When I perform operation 36:
		"""
		assert(status.options.transport_limits.max_outgoing_bytes ==
		32u * 1024 * 1024);
		"""
	Then its observed values match the contract
	When I perform operation 37:
		"""
		assert(status.options.transport_limits.max_outgoing_packets == 8192);
		"""
	Then its observed values match the contract
	When I perform operation 38:
		"""
		assert(status.options.transport_limits.max_waiting_bytes ==
		32u * 1024 * 1024);
		"""
	Then its observed values match the contract
	When I perform operation 39:
		"""
		assert(status.max_frame_bytes == 65536);
		"""
	Then its observed values match the contract
	When I perform operation 40:
		"""
		assert(enet_sync_server_service(s, 1, 0, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 41:
		"""
		assert(enet_sync_server_disconnect(s, 1, 0) == SYNC_ERR_NOT_FOUND);
		"""
	Then its observed values match the contract
	When I perform operation 42:
		"""
		enet_sync_client_desc_t cd = {.session = client.session};
		"""
	Then its observed values match the contract
	When I perform operation 43:
		"""
		assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ARGUMENT);
		"""
	Then its observed values match the contract
	When I perform operation 44:
		"""
		cd.address = status.address;
		"""
	Then its observed values match the contract
	When I perform operation 45:
		"""
		cd.address.ip[0] = 127;
		"""
	Then its observed values match the contract
	When I perform operation 46:
		"""
		cd.address.ip[3] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 47:
		"""
		cd.session = server.session;
		"""
	Then its observed values match the contract
	When I perform operation 48:
		"""
		assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ROLE);
		"""
	Then its observed values match the contract
	When I perform operation 49:
		"""
		cd.session = client.session;
		"""
	Then its observed values match the contract
	When I perform operation 50:
		"""
		cd.options.schedule = SYNC_AUTOMATIC;
		"""
	Then its observed values match the contract
	When I perform operation 51:
		"""
		assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 52:
		"""
		cd.options.schedule = SYNC_MANUAL;
		"""
	Then its observed values match the contract
	When I perform operation 53:
		"""
		assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 54:
		"""
		memset(&cd, 0xff, sizeof(cd));
		"""
	Then its observed values match the contract
	When I perform operation 55:
		"""
		assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 56:
		"""
		assert(status.state == ENET_SYNC_CONNECTING);
		"""
	Then its observed values match the contract
	When I perform operation 57:
		"""
		assert(enet_sync_client_close(c, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 58:
		"""
		assert(enet_sync_server_close(s, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 59:
		"""
		world_close(&client);
		"""
	Then its observed values match the contract
	When I perform operation 60:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: connections
	# Given independent reflected worlds and dedicated ENet transports
	# When Aria Bram and the GM connect asynchronously
	# Then callbacks receive pending peers and connection data before capture
	Given the "enet_sync" workflow "connections"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		assert(g.connected == 3 && !g.recorded);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_status_t core;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(sync_get_status(g.server.session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(core.peers == 3 && !core.last_tick && !core.queued_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: snapshot_delta
	# Given independent reflected worlds and dedicated ENet transports
	# When the server captures a snapshot and changes Aria health
	# Then clients publish ordered values, references and private inventories
	Given the "enet_sync" workflow "snapshot_delta"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 3, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct health h = {42};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		game_capture(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		game_settle(&g, 3, 2, false);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(g.recorded > 12);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: fragmented_updates
	# Given independent reflected worlds and dedicated ENet transports
	# When a bounded core stream fragments a large delta
	# Then small inboxes drain without publishing incomplete updates
	Given the "enet_sync" workflow "fragmented_updates"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 1024, 1024, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t entities[300];
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (unsigned i = 0; i < 300; i++) {
		entities[i] = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
		sizeof(h), &h);
		ecs_add_id(g.server.ecs, entities[i],
		g.server.ids.replicate_to_all);
		}
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		for (unsigned i = 0; i < 300; i++) {
		struct health h = {1000 + (int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
		sizeof(h), &h);
		}
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		game_capture(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		game_settle(&g, 1, 2, false);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(g.fragments > 1);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		for (unsigned i = 0; i < 300; i++) {
		ecs_entity_t local = replica(&g, 0, entities[i]);
		const struct health *h = ecs_get_id(
		g.clients[0].ecs, local, g.clients[0].health);
		assert(h && h->value == 1000 + (int32_t)i);
		}
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: packet_fragmentation
	# Given independent reflected worlds and dedicated ENet transports
	# When one reliable packet exceeds the ENet MTU
	# Then all original Sync frame bytes arrive and every replica is published
	Given the "enet_sync" workflow "packet_fragmentation"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 500; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(g.max_packet > 1400);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		sync_status_t s;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(sync_get_status(g.clients[0].session, 0, &s) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(s.entities == 503);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: rpg_trade_visibility
	# Given independent reflected worlds and dedicated ENet transports
	# When Aria trades an item to Bram and Bram reveals it
	# Then inventories retain references and respect recipient visibility
	Given the "enet_sync" workflow "rpg_trade_visibility"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 3, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		struct item item = {g.actors[1], 7};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_set_id(
		g.server.ecs, g.items[0], g.server.item, sizeof(item), &item);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		g.mask[0] = 2 | 4;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		game_policies(&g);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_capture(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		game_settle(&g, 3, 2, false);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		g.mask[0] = 1 | 2 | 4;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		game_policies(&g);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		game_capture(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		game_settle(&g, 3, 3, false);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: reconnect
	# Given independent reflected worlds and dedicated ENet transports
	# When Aria disconnects then closes resets and reconnects
	# Then reset clears old replicas and reconnection publishes a fresh snapshot
	Given the "enet_sync" workflow "reconnect"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 3, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t old = replica(&g, 0, g.actors[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_client_disconnect(g.connections[0], 17) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(ecs_is_alive(g.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		sync_status_t core;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(core.state == SYNC_CLIENT_READY);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		for (unsigned i = 0; i < 5000 && g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(!g.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_client_close(g.connections[0], NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		enet_sync_status_t listener;
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(enet_sync_server_get_status(g.listener, &listener) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		enet_sync_client_desc_t d = {
		.session = g.clients[0].session, .address = listener.address};
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		d.address.ip[0] = 127;
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		d.address.ip[3] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(sync_client_reset(g.clients[0].session) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(!ecs_is_alive(g.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		game_connect(&g, 0);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		for (unsigned i = 0; i < 5000 && !g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		ecs_progress(g.clients[0].ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
		assert(enet_sync_client_service(g.connections[0], 64, NULL) >=
		0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		assert(g.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		struct health h = {23};
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		game_capture(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		game_settle(&g, 3, 2, false);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @enet_sync
Scenario: automatic_phases
	# Given independent reflected worlds and dedicated ENet transports
	# When automatic worlds progress through Sync phases
	# Then automatic phases preserve ordering and use the default budgets
	Given the "enet_sync" workflow "automatic_phases"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, true, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		enet_sync_status_t s;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(enet_sync_server_get_status(g.listener, &s) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(s.options.events_per_progress == 64);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(s.options.frames_per_peer == 64);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_service(g.listener, 1, 1, NULL) ==
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		game_settle(&g, 3, 1, true);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_check(&g, 3);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(g.recorded >= 12);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@loopback @file_sync @enet_sync
Scenario: recording_replay
	# Given independent reflected worlds and dedicated ENet transports
	# When accepted packets are recorded to raw FileSync writers
	# Then Aria Bram and GM replays preserve every value reference and visibility
	# rule
	Given the "enet_sync" workflow "recording_replay"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		const char *paths[] = {"aria.sync", "bram.sync", "gm.sync"};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		for (unsigned i = 0; i < 3; i++)
		assert(file_sync_writer_open(
		paths[i], NULL, &g.writers[i], NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		game_settle(&g, 3, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		struct health h = {8};
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		game_capture(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_settle(&g, 3, 2, false);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(g.recording_error == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		for (unsigned view = 0; view < 3; view++) {
		assert(file_sync_writer_close(g.writers[view], NULL) ==
		SYNC_OK);
		g.writers[view] = NULL;
		struct world replay;
		world_open(&replay, false, false, 0, 0);
		file_sync_reader_t *reader;
		file_sync_reader_desc_t d = {.client = replay.session};
		assert(file_sync_reader_open(paths[view], &d, &reader, NULL) ==
		SYNC_OK);
		sync_result_t r;
		do {
		r = file_sync_reader_step(reader, 1, NULL);
		assert(r >= 0);
		} while (r != SYNC_END);
		struct world live = g.clients[view];
		g.clients[view] = replay;
		
		game_check(&g, 3);
		g.clients[view] = live;
		assert(file_sync_reader_close(reader, NULL) == SYNC_OK);
		world_close(&replay);
		assert(remove(paths[view]) == 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: outgoing_pressure
	# Given independent reflected worlds and dedicated ENet transports
	# When ENet retains the configured number of outgoing packets
	# Then each core head and token remain unchanged until packet release
	Given the "enet_sync" workflow "outgoing_pressure"
	When I perform operation 1:
		"""
		enet_sync_options_t options = {.transport_limits = {.max_requests = 1}};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_open(&g, false, 0, 0, &options, 2);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		es_hold_packets = 1;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(p.frames == 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(!status.retained_packets && status.retained_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		sync_frame_view_t before, after;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(sync_outbox_peek(g.server.session, g.peers[0], &before) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(!p.frames);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(sync_outbox_peek(g.server.session, g.peers[0], &after) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(before.data == after.data && before.size == after.size);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		assert(!memcmp(&before.token, &after.token, sizeof(before.token)));
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		es_release_packets();
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(!status.retained_bytes && !status.retained_packets);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		es_hold_packets = 0;
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		game_settle(&g, 2, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		game_check(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: outgoing_bytes
	# Given independent reflected worlds and dedicated ENet transports
	# When a frame exceeds the retained outgoing byte budget
	# Then the complete unchanged core head remains queued
	Given the "enet_sync" workflow "outgoing_bytes"
	When I perform operation 1:
		"""
		struct world server;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		enet_sync_server_desc_t d = {.session = server.session,
		.options.transport_limits.max_outgoing_bytes = 1};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_server_t *s = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(!s);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		d.options.transport_limits.max_outgoing_bytes = 0;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		d.options.transport_limits.max_packet_bytes = 1;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(!s);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: per_peer_budgets
	# Given independent reflected worlds and dedicated ENet transports
	# When three recipients each have several queued frames
	# Then the independent one-frame budget accepts one frame for each peer
	Given the "enet_sync" workflow "per_peer_budgets"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 3);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_send(g.listener, 1, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(p.frames == 3);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		for (unsigned i = 0; i < 3; i++) {
		sync_frame_view_t v;
		assert(sync_outbox_peek(g.server.session, g.peers[i], &v) ==
		SYNC_OK);
		assert(v.info.sequence == 2);
		}
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		game_settle(&g, 3, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: client_pressure
	# Given independent reflected worlds and dedicated ENet transports
	# When the core inbox rejects the next complete packet
	# Then pending input is retried first while ENet protocol servicing continues
	Given the "enet_sync" workflow "client_pressure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 1024, 1024, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(p.frames);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		enet_sync_status_t before, after;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &before) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(before.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		int services = es_protocol_services;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(!p.frames && !p.events && es_protocol_services > services);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(before.pending_bytes == after.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(!after.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		sync_status_t core;
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		assert(core.entities == 203);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: send_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When ENet rejects one peer send
	# Then the caller destroys its packet and healthy recipients continue
	Given the "enet_sync" workflow "send_failure"
	When I perform operation 1:
		"""
		bool allocation = false;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_open(&g, false, 0, 0, NULL, 2);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		if (allocation)
		es_fail_packet = 1;
		else
		es_fail_send = 1;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(!p.faulted_connections && p.frames == 8);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(p.faulted_connections == 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(g.faults == 1 && !g.peers[0] && g.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && status.connections == 1);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(status.last_connection_error.detail.code ==
		(allocation ? SYNC_ERR_NOMEM : SYNC_ERR_TRANSPORT));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		es_release_packets();
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		ecs_progress(g.clients[1].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(enet_sync_client_service(g.connections[1], 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(sync_apply(g.clients[1].session, 1, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(replica(&g, 1, g.actors[0]));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: packet_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When packet allocation fails for one peer
	# Then only that association closes and other recipients receive snapshots
	Given the "enet_sync" workflow "packet_failure"
	When I perform operation 1:
		"""
		bool allocation = true;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_open(&g, false, 0, 0, NULL, 2);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		if (allocation)
		es_fail_packet = 1;
		else
		es_fail_send = 1;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(!p.faulted_connections && p.frames == 8);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(p.faulted_connections == 1);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(g.faults == 1 && !g.peers[0] && g.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && status.connections == 1);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(status.last_connection_error.detail.code ==
		(allocation ? SYNC_ERR_NOMEM : SYNC_ERR_TRANSPORT));
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		es_release_packets();
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		ecs_progress(g.clients[1].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(enet_sync_client_service(g.connections[1], 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(sync_apply(g.clients[1].session, 1, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(replica(&g, 1, g.actors[0]));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: host_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When ENet host service fails
	# Then the adapter faults and closes every association with copied diagnostics
	Given the "enet_sync" workflow "host_failure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 2);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		es_fail_service = 1;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_poll(g.listener, 64, NULL) ==
		SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(status.state == ENET_SYNC_FAULTED && !status.connections);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(g.faults == 2);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_close(g.listener, NULL) == SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		g.listener = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: open_failures
	# Given independent reflected worlds and dedicated ENet transports
	# When host creation or asynchronous connect setup fails
	# Then no transport handle system or core peer leaks
	Given the "enet_sync" workflow "open_failures"
	When I perform operation 1:
		"""
		struct world server, client;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_open(&client, false, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_server_desc_t d = {.session = server.session};
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		enet_sync_server_t *s;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		es_fail_host = 1;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(status.state == ENET_SYNC_OPENING && !es_hosts_live);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_poll(s, 64, NULL) == SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ecs_progress(server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_server_poll(s, 64, NULL) == SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_close(s, NULL) == SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		enet_sync_client_desc_t cd = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		enet_sync_client_t *c;
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		es_fail_connect = 1;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		ecs_progress(client.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(enet_sync_client_service(c, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		ecs_progress(client.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(enet_sync_client_service(c, 64, NULL) == SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(enet_sync_client_close(c, NULL) == SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		assert(!es_hosts_live);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		world_close(&client);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: malformed_packet
	# Given independent reflected worlds and dedicated ENet transports
	# When a malformed reliable packet arrives after a snapshot
	# Then the client faults while its published replicas survive
	Given the "enet_sync" workflow "malformed_packet"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t old = replica(&g, 0, g.actors[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		const char bad[] = "malformed";
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		es_inject_client(bad, sizeof(bad), 0, ENET_PACKET_FLAG_RELIABLE);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(ecs_is_alive(g.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(status.state == ENET_SYNC_FAULTED && !status.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_client_close(g.connections[0], NULL) ==
		SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		g.connections[0] = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: core_peer_isolation
	# Given independent reflected worlds and dedicated ENet transports
	# When Bram references a secret NPC invisible to his recipient
	# Then only Bram association closes while Aria continues
	Given the "enet_sync" workflow "core_peer_isolation"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 2);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 2, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		struct item value = {g.actors[2], 1};
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_set_id(
		g.server.ecs, g.items[1], g.server.item, sizeof(value), &value);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(sync_capture(g.server.session, 2, NULL) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(g.peers[0] && !g.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(status.last_connection_error.detail.code == SYNC_ERR_REFERENCE);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: retained_cleanup
	# Given independent reflected worlds and dedicated ENet transports
	# When a host is closed with retained outgoing packets
	# Then release callbacks remain valid until host destruction completes
	Given the "enet_sync" workflow "retained_cleanup"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		es_hold_packets = 1;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(!status.retained_packets && status.retained_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(status.retained_packets == 4 && status.retained_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		es_hold_packets = 0;
		"""
	Then its observed values match the contract

@enet_sync
Scenario: server_disconnect
	# Given independent reflected worlds and dedicated ENet transports
	# When the application requests an immediate server disconnect
	# Then the next service reports disconnect and closes the owned peer
	Given the "enet_sync" workflow "server_disconnect"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 2);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		ecs_entity_t peer = g.peers[0];
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(enet_sync_server_disconnect(g.listener, peer, 9) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(!g.disconnected);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(g.disconnected == 1 && !g.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_disconnect(g.listener, peer, 9) ==
		SYNC_ERR_NOT_FOUND);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(!ecs_is_alive(g.server.ecs, peer));
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@file_sync @enet_sync
Scenario: recording_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When the accepted-frame recording writer fails
	# Then network acceptance continues and the snapshot publishes
	Given the "enet_sync" workflow "recording_failure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		file_sync_writer_desc_t d = {.max_frame_bytes = 1};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(file_sync_writer_open("failed.sync", &d, &g.writers[0], NULL) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(g.recording_error < 0 && g.recorded == 4);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(replica(&g, 0, g.actors[0]));
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && !status.fault.detail.code);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(file_sync_writer_close(g.writers[0], NULL) == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		g.writers[0] = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(remove("failed.sync") == 0);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: client_apply_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When core application rejects an accepted duplicate manifest
	# Then the next service closes the transport and preserves published replicas
	Given the "enet_sync" workflow "client_apply_failure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		sync_frame_view_t v;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(sync_outbox_peek(g.server.session, g.peers[0], &v) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		void *copy = malloc(v.size);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(copy);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		memcpy(copy, v.data, v.size);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		size_t size = v.size;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_entity_t old = replica(&g, 0, g.actors[0]);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		es_inject_client(copy, size, 0, ENET_PACKET_FLAG_RELIABLE);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		free(copy);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(status.state == ENET_SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(status.fault.detail.code == SYNC_ERR_SEQUENCE);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(ecs_is_alive(g.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: poll_budget
	# Given independent reflected worlds and dedicated ENet transports
	# When three pending connections have a one-event polling budget
	# Then each call opens only one core peer and invokes only one callback
	Given the "enet_sync" workflow "poll_budget"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 3; i++) {
		world_open(&g.clients[i], false, false, 0, 0);
		game_connect(&g, i);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		for (unsigned i = 0; i < 3; i++) {
		ecs_progress(g.clients[i].ecs, 0.001f);
		enet_sync_client_service(g.connections[i], 64, NULL);
		ecs_progress(g.clients[i].ecs, 0.001f);
		}
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(p.events == 1 && p.connections == 1 && g.connected == 1);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(p.events == 1 && p.connections == 1 && g.connected == 2);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(g.connected == 3);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: client_host_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When client host servicing fails after publication
	# Then the adapter faults while core replicas remain alive
	Given the "enet_sync" workflow "client_host_failure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_entity_t old = replica(&g, 0, g.actors[0]);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		es_fail_service = 1;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_TRANSPORT);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(ecs_is_alive(g.clients[0].ecs, old));
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: unreliable_packet
	# Given independent reflected worlds and dedicated ENet transports
	# When an unreliable packet arrives on the replication channel
	# Then the association faults without accepting any bytes
	Given the "enet_sync" workflow "unreliable_packet"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		const char bytes[] = "invalid channel/flags";
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		es_inject_client(bytes, sizeof(bytes), 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(status.state == ENET_SYNC_FAULTED);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: peer_setup_failure
	# Given independent reflected worlds and dedicated ENet transports
	# When all core peer capacity is already reserved
	# Then the new connection is rejected without faulting the listening host
	Given the "enet_sync" workflow "peer_setup_failure"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		assert(sync_peer_open(g.server.session, &peer) == SYNC_OK);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		world_open(&g.clients[0], false, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_connect(&g, 0);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		enet_sync_client_service(g.connections[0], 64, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(g.faults == 1 && !g.connected && !g.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && !status.connections);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(status.last_connection_error.detail.code == SYNC_ERR_LIMIT);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: client_frame_callback
	# Given independent reflected worlds and dedicated ENet transports
	# When the client accepts a manifest and snapshot
	# Then its callback observes unchanged bytes in sequence with peer zero
	Given the "enet_sync" workflow "client_frame_callback"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_open(&g.clients[0], false, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		uint32_t accepted = 0;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options = {.on_frame = count_frames, .ctx = &accepted}};
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		d.address.ip[0] = 127;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		d.address.ip[3] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		enet_sync_client_service(g.connections[0], 64, NULL);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		game_settle(&g, 1, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(accepted == 4);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: automatic_custom_budget
	# Given independent reflected worlds and dedicated ENet transports
	# When automatic adapters borrow a manual server and automatic client
	# Then capture stays explicit and each send progression accepts at most one
	# frame
	Given the "enet_sync" workflow "automatic_custom_budget"
	When I perform operation 1:
		"""
		struct world server, client;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_open(&client, false, true, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_server_desc_t d = {.session = server.session,
		.options = {.schedule = SYNC_AUTOMATIC,
		.events_per_progress = 1,
		.frames_per_peer = 1}};
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		enet_sync_server_t *listener;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_server_listen(&d, &listener, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		ecs_progress(server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		enet_sync_client_desc_t cd = {.session = client.session,
		.address = status.address,
		.options = {.schedule = SYNC_AUTOMATIC,
		.events_per_progress = 1,
		.frames_per_peer = 1}};
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		cd.address.ip[0] = 127;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		cd.address.ip[3] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		enet_sync_client_t *connection;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_client_connect(&cd, &connection, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		memset(&d, 0xff, sizeof(d));
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		for (unsigned i = 0; i < 4; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
		}
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		sync_status_t core;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		assert(sync_get_status(server.session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(core.peers == 1 && !core.last_tick && !core.queued_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(sync_capture(server.session, 1, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		ecs_progress(server.ecs, 0.01f);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(status.frames == 1 && status.options.frames_per_peer == 1);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		assert(status.options.events_per_progress == 1);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		for (unsigned i = 0; i < 10; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
		}
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		assert(sync_get_status(client.session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		assert(enet_sync_client_close(connection, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		assert(enet_sync_server_close(listener, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		world_close(&client);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: empty_packet
	# Given independent reflected worlds and dedicated ENet transports
	# When an empty reliable packet arrives
	# Then the transport reports a protocol failure instead of a caller error
	Given the "enet_sync" workflow "empty_packet"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		es_inject_client("", 0, 0, ENET_PACKET_FLAG_RELIABLE);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: shared_world_ownership
	Given the "enet_sync" workflow "shared_world_ownership"
	When I perform operation 1:
		"""
		struct world server;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		flecs_enet_ids_t ids, again;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(flecs_enet_import(server.ecs, &again) == FLECS_ENET_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(ids.module == again.module);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_entity_t external = ecs_new(server.ecs), request;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		flecs_enet_host_config_t config = {.listen = true};
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_set_id(
		server.ecs, external, ids.host_config, sizeof(config), &config);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		flecs_enet_request_t open = {.operation = FLECS_ENET_OPEN};
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(flecs_enet_submit(server.ecs, external, &open, NULL, 0,
		&request) == FLECS_ENET_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		enet_sync_server_desc_t desc = {.session = server.session};
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		enet_sync_server_t *a, *b;
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_server_listen(&desc, &a, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_server_listen(&desc, &b, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		ecs_progress(server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(enet_sync_server_poll(a, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		assert(enet_sync_server_poll(b, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		enet_sync_status_t sa, sb;
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(enet_sync_server_get_status(a, &sa) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(enet_sync_server_get_status(b, &sb) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(sa.host != sb.host && sa.host != external);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		const flecs_enet_host_status_t *hs =
		ecs_get_id(server.ecs, sa.host, ids.host_status);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		assert(hs && !hs->pending_requests && !hs->retained_events);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		assert(enet_sync_server_close(a, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 26:
		"""
		assert(!ecs_is_alive(server.ecs, sa.host));
		"""
	Then its observed values match the contract
	When I perform operation 27:
		"""
		assert(ecs_is_alive(server.ecs, sb.host));
		"""
	Then its observed values match the contract
	When I perform operation 28:
		"""
		assert(ecs_is_alive(server.ecs, external) && es_hosts_live == 2);
		"""
	Then its observed values match the contract
	When I perform operation 29:
		"""
		assert(enet_sync_server_close(b, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 30:
		"""
		assert(es_hosts_live == 1);
		"""
	Then its observed values match the contract
	When I perform operation 31:
		"""
		ecs_delete(server.ecs, external);
		"""
	Then its observed values match the contract
	When I perform operation 32:
		"""
		assert(!es_hosts_live);
		"""
	Then its observed values match the contract
	When I perform operation 33:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: host_packet_limit
	Given the "enet_sync" workflow "host_packet_limit"
	When I perform operation 1:
		"""
		enet_sync_options_t options = {
		.transport_limits = {.max_outgoing_packets = 1}};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_open(&g, false, 0, 0, &options, 2);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		es_hold_packets = 1;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(p.frames == 8 && g.recorded == 8);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(status.retained_packets == 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(status.retained_packets == 1);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		es_hold_packets = 0;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		es_release_packets();
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		game_settle(&g, 2, 1, false);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		game_check(&g, 2);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(g.recorded == 8);
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: failed_send_cancellation
	Given the "enet_sync" workflow "failed_send_cancellation"
	When I perform operation 1:
		"""
		enet_sync_options_t options = {.transport_limits = {.max_events = 1}};
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		game_open(&g, false, 0, 0, &options, 2);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		es_fail_send = 1;
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_progress_t p;
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(p.frames == 8 && g.recorded == 8);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(p.faulted_connections == 1 && !g.peers[0] && g.peers[1]);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		flecs_enet_ids_t ids;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(flecs_enet_import(g.server.ecs, &ids) == FLECS_ENET_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		const flecs_enet_host_status_t *h =
		ecs_get_id(g.server.ecs, status.host, ids.host_status);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(h->pending_requests == 5);
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		for (unsigned i = 0; i < 10; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_service(g.listener, 64, 64, NULL) >= 0);
		ecs_progress(g.clients[1].ecs, 0.001f);
		assert(enet_sync_client_service(g.connections[1], 64, NULL) >=
		0);
		assert(sync_apply(g.clients[1].session, 64, NULL) >= 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		h = ecs_get_id(g.server.ecs, status.host, ids.host_status);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(!h->pending_requests && !h->retained_events);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		assert(replica(&g, 1, g.actors[0]));
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(g.recorded == 8);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: same_frame_publication
	Given the "enet_sync" workflow "same_frame_publication"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 0, 0, NULL, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		world_open(&g.clients[0], false, true, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options.schedule = SYNC_AUTOMATIC};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		d.address.ip[0] = 127;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		d.address.ip[3] = 1;
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		for (unsigned i = 0; i < 4; i++) {
		ecs_progress(g.clients[0].ecs, 0.001f);
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(g.peers[0]);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		ecs_entity_t reader = ecs_new(g.clients[0].ecs);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		ecs_add_pair(g.clients[0].ecs, reader, EcsDependsOn, EcsPreUpdate);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		assert(ecs_system_init(g.clients[0].ecs,
		&(ecs_system_desc_t){.entity = reader,
		.callback = published_reader,
		.ctx = &g.clients[0],
		.immediate = true}));
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(replica(&g, 0, g.actors[0]));
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		ecs_delete(g.clients[0].ecs, reader);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: pending_disconnect
	Given the "enet_sync" workflow "pending_disconnect"
	When I perform operation 1:
		"""
		struct world client;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&client, false, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		enet_sync_client_desc_t d = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		enet_sync_client_t *c;
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_client_connect(&d, &c, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_service(c, 64, NULL) == SYNC_EMPTY);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		ecs_progress(client.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		assert(enet_sync_client_service(c, 64, NULL) == SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(status.state == ENET_SYNC_DISCONNECTED);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		assert(enet_sync_client_close(c, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		world_close(&client);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: ordered_receive_disconnect
	Given the "enet_sync" workflow "ordered_receive_disconnect"
	When I perform operation 1:
		"""
		struct game g;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		game_open(&g, false, 1024, 1024, NULL, 1);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health value = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(value),
		&value);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
		}
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		game_capture(&g, 1);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(enet_sync_server_disconnect(g.listener, g.peers[0], 8) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		ecs_progress(g.server.ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 11:
		"""
		assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 12:
		"""
		ecs_progress(g.clients[0].ecs, 0.001f);
		"""
	Then its observed values match the contract
	When I perform operation 13:
		"""
		assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
		"""
	Then its observed values match the contract
	When I perform operation 14:
		"""
		enet_sync_status_t status;
		"""
	Then its observed values match the contract
	When I perform operation 15:
		"""
		assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 16:
		"""
		assert(status.state == ENET_SYNC_ACTIVE && status.pending_bytes);
		"""
	Then its observed values match the contract
	When I perform operation 17:
		"""
		sync_result_t r = SYNC_OK;
		"""
	Then its observed values match the contract
	When I perform operation 18:
		"""
		for (unsigned i = 0; i < 20 && r != SYNC_END; i++) {
		assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
		r = enet_sync_client_service(g.connections[0], 64, NULL);
		assert(r >= 0);
		}
		"""
	Then its observed values match the contract
	When I perform operation 19:
		"""
		assert(r == SYNC_END);
		"""
	Then its observed values match the contract
	When I perform operation 20:
		"""
		assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
		"""
	Then its observed values match the contract
	When I perform operation 21:
		"""
		sync_status_t core;
		"""
	Then its observed values match the contract
	When I perform operation 22:
		"""
		assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
		"""
	Then its observed values match the contract
	When I perform operation 23:
		"""
		assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
		"""
	Then its observed values match the contract
	When I perform operation 24:
		"""
		assert(core.entities == 203);
		"""
	Then its observed values match the contract
	When I perform operation 25:
		"""
		game_close(&g);
		"""
	Then its observed values match the contract

@enet_sync
Scenario: phase_cycle
	Given the "enet_sync" workflow "phase_cycle"
	When I perform operation 1:
		"""
		struct world server;
		"""
	Then its observed values match the contract
	When I perform operation 2:
		"""
		world_open(&server, true, false, 0, 0);
		"""
	Then its observed values match the contract
	When I perform operation 3:
		"""
		flecs_enet_ids_t ids;
		"""
	Then its observed values match the contract
	When I perform operation 4:
		"""
		assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
		"""
	Then its observed values match the contract
	When I perform operation 5:
		"""
		ecs_add_pair(server.ecs, server.ids.send_phase, EcsDependsOn,
		ids.send_phase);
		"""
	Then its observed values match the contract
	When I perform operation 6:
		"""
		enet_sync_server_desc_t desc = {
		.session = server.session, .options.schedule = SYNC_AUTOMATIC};
		"""
	Then its observed values match the contract
	When I perform operation 7:
		"""
		enet_sync_server_t *s = NULL;
		"""
	Then its observed values match the contract
	When I perform operation 8:
		"""
		assert(enet_sync_server_listen(&desc, &s, NULL) == SYNC_ERR_STATE);
		"""
	Then its observed values match the contract
	When I perform operation 9:
		"""
		assert(!s && !es_hosts_live);
		"""
	Then its observed values match the contract
	When I perform operation 10:
		"""
		world_close(&server);
		"""
	Then its observed values match the contract
