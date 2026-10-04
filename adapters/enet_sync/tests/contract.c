#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void invalid_calls(void)
{
	assert(enet_sync_server_listen(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_client_connect(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_poll(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_send(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_service(NULL, 1, 1, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_client_service(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_disconnect(NULL, 1, 0) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_client_disconnect(NULL, 0) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_client_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
}

static void initialized_outputs(void)
{
	enet_sync_status_t status, zero = {0};
	memset(&status, 0xff, sizeof(status));
	assert(enet_sync_server_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
	assert(!memcmp(&status, &zero, sizeof(status)));
	memset(&status, 0xff, sizeof(status));
	assert(enet_sync_client_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
	assert(!memcmp(&status, &zero, sizeof(status)));
	enet_sync_progress_t p, empty = {0};
	memset(&p, 0xff, sizeof(p));
	assert(enet_sync_server_send(NULL, 0, &p) == SYNC_ERR_ARGUMENT);
	assert(!memcmp(&p, &empty, sizeof(p)));
	enet_sync_server_t *server = (void *)1;
	enet_sync_error_t e, success = {0};
	memset(&e, 0xff, sizeof(e));
	assert(enet_sync_server_listen(NULL, &server, &e) == SYNC_ERR_ARGUMENT);
	assert(!server && !memcmp(&e, &success, sizeof(e)));
}

static void null_cleanup(void)
{
	assert(enet_sync_server_close(NULL, NULL) == SYNC_OK);
	assert(enet_sync_client_close(NULL, NULL) == SYNC_OK);
}

static void options(void)
{
	struct world server, client;
	world_open(&server, true, false, 0, 0);
	world_open(&client, false, false, 0, 0);
	enet_sync_server_t *s;
	enet_sync_client_t *c;
	enet_sync_server_desc_t d = {.session = server.session};
	d.options.schedule = (sync_schedule_t)99;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	d.options.schedule = SYNC_MANUAL;
	d.options.events_per_progress = 1;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	d.options.events_per_progress = 0;
	d.options.frames_per_peer = 1;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	d.options.frames_per_peer = 0;
	d.options.transport_limits.max_waiting_bytes = 1;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	d.options.transport_limits.max_waiting_bytes = 0;
	d.session = client.session;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ROLE);
	d.session = server.session;
	d.max_clients = UINT32_MAX;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	d.max_clients = 2;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
	memset(&d, 0xff, sizeof(d));
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_OPENING && status.host);
#ifdef ENET_SYNC_TESTING
	assert(!es_hosts_live);
#endif
	ecs_progress(server.ecs, 0.001f);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_OK);
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_ACTIVE && status.address.port);
	assert(status.schedule == SYNC_MANUAL && !status.connections);
	assert(status.options.transport_limits.max_outgoing_bytes ==
		32u * 1024 * 1024);
	assert(status.options.transport_limits.max_outgoing_packets == 8192);
	assert(status.options.transport_limits.max_waiting_bytes ==
		32u * 1024 * 1024);
	assert(status.max_frame_bytes == 65536);
	assert(enet_sync_server_service(s, 1, 0, NULL) == SYNC_ERR_ARGUMENT);
	assert(enet_sync_server_disconnect(s, 1, 0) == SYNC_ERR_NOT_FOUND);
	enet_sync_client_desc_t cd = {.session = client.session};
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ARGUMENT);
	cd.address = status.address;
	cd.address.ip[0] = 127;
	cd.address.ip[3] = 1;
	cd.session = server.session;
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ROLE);
	cd.session = client.session;
	cd.options.schedule = SYNC_AUTOMATIC;
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_STATE);
	cd.options.schedule = SYNC_MANUAL;
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
	memset(&cd, 0xff, sizeof(cd));
	assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_CONNECTING);
	assert(enet_sync_client_close(c, NULL) == SYNC_OK);
	assert(enet_sync_server_close(s, NULL) == SYNC_OK);
	world_close(&client);
	world_close(&server);
}

static void connections(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	assert(g.connected == 3 && !g.recorded);
	sync_status_t core;
	assert(sync_get_status(g.server.session, 0, &core) == SYNC_OK);
	assert(core.peers == 3 && !core.last_tick && !core.queued_bytes);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_EMPTY);
	game_close(&g);
}

static void snapshot_delta(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	game_capture(&g, 1);
	game_settle(&g, 3, 1, false);
	game_check(&g, 3);
	struct health h = {42};
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	game_capture(&g, 2);
	game_settle(&g, 3, 2, false);
	game_check(&g, 3);
	assert(g.recorded > 12);
	game_close(&g);
}

static void fragmented_updates(void)
{
	struct game g;
	game_open(&g, false, 1024, 1024, NULL, 1);
	ecs_entity_t entities[300];
	for (unsigned i = 0; i < 300; i++) {
		entities[i] = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
			sizeof(h), &h);
		ecs_add_id(g.server.ecs, entities[i],
			g.server.ids.replicate_to_all);
	}
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	for (unsigned i = 0; i < 300; i++) {
		struct health h = {1000 + (int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
			sizeof(h), &h);
	}
	game_capture(&g, 2);
	game_settle(&g, 1, 2, false);
	assert(g.fragments > 1);
	for (unsigned i = 0; i < 300; i++) {
		ecs_entity_t local = replica(&g, 0, entities[i]);
		const struct health *h = ecs_get_id(
			g.clients[0].ecs, local, g.clients[0].health);
		assert(h && h->value == 1000 + (int32_t)i);
	}
	game_close(&g);
}

static void packet_fragmentation(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	for (unsigned i = 0; i < 500; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	assert(g.max_packet > 1400);
	sync_status_t s;
	assert(sync_get_status(g.clients[0].session, 0, &s) == SYNC_OK);
	assert(s.entities == 503);
	game_close(&g);
}

static void rpg_trade_visibility(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	game_capture(&g, 1);
	game_settle(&g, 3, 1, false);
	game_check(&g, 3);
	/* Aria trades her stack to Bram; GM retains access to both views. */
	struct item item = {g.actors[1], 7};
	ecs_set_id(
		g.server.ecs, g.items[0], g.server.item, sizeof(item), &item);
	g.mask[0] = 2 | 4;
	game_policies(&g);
	game_capture(&g, 2);
	game_settle(&g, 3, 2, false);
	game_check(&g, 3);
	/* Bram reveals this item to Aria; owner is already public. */
	g.mask[0] = 1 | 2 | 4;
	game_policies(&g);
	game_capture(&g, 3);
	game_settle(&g, 3, 3, false);
	game_check(&g, 3);
	game_close(&g);
}

static void reconnect(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	game_capture(&g, 1);
	game_settle(&g, 3, 1, false);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	assert(enet_sync_client_disconnect(g.connections[0], 17) == SYNC_OK);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	sync_status_t core;
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	assert(core.state == SYNC_CLIENT_READY);
	for (unsigned i = 0; i < 5000 && g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
	}
	assert(!g.peers[0]);
	assert(enet_sync_client_close(g.connections[0], NULL) == SYNC_OK);
	enet_sync_status_t listener;
	assert(enet_sync_server_get_status(g.listener, &listener) == SYNC_OK);
	enet_sync_client_desc_t d = {
		.session = g.clients[0].session, .address = listener.address};
	d.address.ip[0] = 127;
	d.address.ip[3] = 1;
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_ERR_STATE);
	assert(sync_client_reset(g.clients[0].session) == SYNC_OK);
	assert(!ecs_is_alive(g.clients[0].ecs, old));
	game_connect(&g, 0);
	for (unsigned i = 0; i < 5000 && !g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		ecs_progress(g.clients[0].ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
		assert(enet_sync_client_service(g.connections[0], 64, NULL) >=
			0);
	}
	assert(g.peers[0]);
	struct health h = {23};
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	game_capture(&g, 2);
	game_settle(&g, 3, 2, false);
	game_check(&g, 3);
	game_close(&g);
}

static void automatic_phases(void)
{
	struct game g;
	game_open(&g, true, 0, 0, NULL, 3);
	enet_sync_status_t s;
	assert(enet_sync_server_get_status(g.listener, &s) == SYNC_OK);
	assert(s.options.events_per_progress == 64);
	assert(s.options.frames_per_peer == 64);
	assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_ERR_STATE);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
	assert(enet_sync_server_service(g.listener, 1, 1, NULL) ==
		SYNC_ERR_STATE);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
	game_settle(&g, 3, 1, true);
	game_check(&g, 3);
	assert(g.recorded >= 12);
	game_close(&g);
}

#ifdef ES_FILE_SYNC
static void recording_replay(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	const char *paths[] = {"aria.sync", "bram.sync", "gm.sync"};
	for (unsigned i = 0; i < 3; i++)
		assert(file_sync_writer_open(
			       paths[i], NULL, &g.writers[i], NULL) == SYNC_OK);
	game_capture(&g, 1);
	game_settle(&g, 3, 1, false);
	struct health h = {8};
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	game_capture(&g, 2);
	game_settle(&g, 3, 2, false);
	assert(g.recording_error == SYNC_OK);
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
		/* Compare every replayed value/reference/visibility to
		 * authority. */
		game_check(&g, 3);
		g.clients[view] = live;
		assert(file_sync_reader_close(reader, NULL) == SYNC_OK);
		world_close(&replay);
		assert(remove(paths[view]) == 0);
	}
	game_close(&g);
}

static void recording_failure(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	file_sync_writer_desc_t d = {.max_frame_bytes = 1};
	assert(file_sync_writer_open("failed.sync", &d, &g.writers[0], NULL) ==
		SYNC_OK);
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	assert(g.recording_error < 0 && g.recorded == 4);
	assert(replica(&g, 0, g.actors[0]));
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_ACTIVE && !status.fault.detail.code);
	assert(file_sync_writer_close(g.writers[0], NULL) == SYNC_ERR_LIMIT);
	g.writers[0] = NULL;
	assert(remove("failed.sync") == 0);
	game_close(&g);
}

#endif

#ifdef ENET_SYNC_TESTING
static void outgoing_pressure(void)
{
	enet_sync_options_t options = {.transport_limits = {.max_requests = 1}};
	struct game g;
	game_open(&g, false, 0, 0, &options, 2);
	es_hold_packets = 1;
	game_capture(&g, 1);
	enet_sync_progress_t p;
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
	assert(p.frames == 1);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(!status.retained_packets && status.retained_bytes);
	sync_frame_view_t before, after;
	assert(sync_outbox_peek(g.server.session, g.peers[0], &before) ==
		SYNC_OK);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
	assert(!p.frames);
	assert(sync_outbox_peek(g.server.session, g.peers[0], &after) ==
		SYNC_OK);
	assert(before.data == after.data && before.size == after.size);
	assert(!memcmp(&before.token, &after.token, sizeof(before.token)));
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	es_release_packets();
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(!status.retained_bytes && !status.retained_packets);
	es_hold_packets = 0;
	game_settle(&g, 2, 1, false);
	game_check(&g, 2);
	game_close(&g);
}

static void outgoing_bytes(void)
{
	struct world server;
	world_open(&server, true, false, 0, 0);
	enet_sync_server_desc_t d = {.session = server.session,
		.options.transport_limits.max_outgoing_bytes = 1};
	enet_sync_server_t *s = NULL;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	assert(!s);
	d.options.transport_limits.max_outgoing_bytes = 0;
	d.options.transport_limits.max_packet_bytes = 1;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	assert(!s);
	world_close(&server);
}

static void per_peer_budgets(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 3);
	game_capture(&g, 1);
	enet_sync_progress_t p;
	assert(enet_sync_server_send(g.listener, 1, &p) == SYNC_OK);
	assert(p.frames == 3);
	for (unsigned i = 0; i < 3; i++) {
		sync_frame_view_t v;
		assert(sync_outbox_peek(g.server.session, g.peers[i], &v) ==
			SYNC_OK);
		assert(v.info.sequence == 2);
	}
	game_settle(&g, 3, 1, false);
	game_close(&g);
}

static void client_pressure(void)
{
	struct game g;
	game_open(&g, false, 1024, 1024, NULL, 1);
	for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	game_capture(&g, 1);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	enet_sync_progress_t p;
	ecs_progress(g.server.ecs, 0.001f);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
	assert(p.frames);
	enet_sync_status_t before, after;
	assert(enet_sync_client_get_status(g.connections[0], &before) ==
		SYNC_OK);
	assert(before.pending_bytes);
	int services = es_protocol_services;
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
	assert(!p.frames && !p.events && es_protocol_services > services);
	assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
	assert(before.pending_bytes == after.pending_bytes);
	game_settle(&g, 1, 1, false);
	assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
	assert(!after.pending_bytes);
	sync_status_t core;
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	assert(core.entities == 203);
	game_close(&g);
}

static void peer_failure(bool allocation)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 2);
	game_capture(&g, 1);
	if (allocation)
		es_fail_packet = 1;
	else
		es_fail_send = 1;
	enet_sync_progress_t p;
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	assert(!p.faulted_connections && p.frames == 8);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
	assert(p.faulted_connections == 1);
	assert(g.faults == 1 && !g.peers[0] && g.peers[1]);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_ACTIVE && status.connections == 1);
	assert(status.last_connection_error.detail.code ==
		(allocation ? SYNC_ERR_NOMEM : SYNC_ERR_TRANSPORT));
	es_release_packets();
	ecs_progress(g.clients[1].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[1], 64, NULL) == SYNC_OK);
	assert(sync_apply(g.clients[1].session, 1, NULL) == SYNC_OK);
	assert(replica(&g, 1, g.actors[0]));
	game_close(&g);
}
static void send_failure(void)
{
	peer_failure(false);
}
static void packet_failure(void)
{
	peer_failure(true);
}

static void host_failure(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 2);
	es_fail_service = 1;
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, NULL) ==
		SYNC_ERR_TRANSPORT);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_FAULTED && !status.connections);
	assert(g.faults == 2);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
	assert(enet_sync_server_close(g.listener, NULL) == SYNC_ERR_TRANSPORT);
	g.listener = NULL;
	game_close(&g);
}

static void open_failures(void)
{
	struct world server, client;
	world_open(&server, true, false, 0, 0);
	world_open(&client, false, false, 0, 0);
	enet_sync_server_desc_t d = {.session = server.session};
	enet_sync_server_t *s;
	es_fail_host = 1;
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_OPENING && !es_hosts_live);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_EMPTY);
	ecs_progress(server.ecs, 0.001f);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_ERR_TRANSPORT);
	assert(enet_sync_server_close(s, NULL) == SYNC_ERR_TRANSPORT);
	enet_sync_client_desc_t cd = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
	enet_sync_client_t *c;
	es_fail_connect = 1;
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
	ecs_progress(client.ecs, 0.001f);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_OK);
	ecs_progress(client.ecs, 0.001f);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_ERR_TRANSPORT);
	assert(enet_sync_client_close(c, NULL) == SYNC_ERR_TRANSPORT);
	assert(!es_hosts_live);
	world_close(&client);
	world_close(&server);
}

static void malformed_packet(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	const char bad[] = "malformed";
	es_inject_client(bad, sizeof(bad), 0, ENET_PACKET_FLAG_RELIABLE);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_PROTOCOL);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	enet_sync_status_t status;
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	assert(status.state == ENET_SYNC_FAULTED && !status.pending_bytes);
	assert(enet_sync_client_close(g.connections[0], NULL) ==
		SYNC_ERR_PROTOCOL);
	g.connections[0] = NULL;
	game_close(&g);
}

static void core_peer_isolation(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 2);
	game_capture(&g, 1);
	game_settle(&g, 2, 1, false);
	/* Bram's item references a secret NPC invisible in his view. */
	struct item value = {g.actors[2], 1};
	ecs_set_id(
		g.server.ecs, g.items[1], g.server.item, sizeof(value), &value);
	assert(sync_capture(g.server.session, 2, NULL) == SYNC_PARTIAL);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_PARTIAL);
	assert(g.peers[0] && !g.peers[1]);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.last_connection_error.detail.code == SYNC_ERR_REFERENCE);
	ecs_progress(g.server.ecs, 0.001f);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_OK);
	game_close(&g);
}

static void retained_cleanup(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	es_hold_packets = 1;
	game_capture(&g, 1);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(!status.retained_packets && status.retained_bytes);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.retained_packets == 4 && status.retained_bytes);
	game_close(&g);
	es_hold_packets = 0;
}

static void server_disconnect(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 2);
	ecs_entity_t peer = g.peers[0];
	assert(enet_sync_server_disconnect(g.listener, peer, 9) == SYNC_OK);
	assert(!g.disconnected);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	assert(g.disconnected == 1 && !g.peers[0]);
	assert(enet_sync_server_disconnect(g.listener, peer, 9) ==
		SYNC_ERR_NOT_FOUND);
	assert(!ecs_is_alive(g.server.ecs, peer));
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
	game_close(&g);
}

static void client_apply_failure(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	game_capture(&g, 1);
	sync_frame_view_t v;
	assert(sync_outbox_peek(g.server.session, g.peers[0], &v) == SYNC_OK);
	void *copy = malloc(v.size);
	assert(copy);
	memcpy(copy, v.data, v.size);
	size_t size = v.size;
	game_settle(&g, 1, 1, false);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	es_inject_client(copy, size, 0, ENET_PACKET_FLAG_RELIABLE);
	ecs_progress(g.clients[0].ecs, 0.001f);
	free(copy);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_ERR_SEQUENCE);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_SEQUENCE);
	enet_sync_status_t status;
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	assert(status.state == ENET_SYNC_FAULTED);
	assert(status.fault.detail.code == SYNC_ERR_SEQUENCE);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	game_close(&g);
}

static void poll_budget(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 0);
	for (unsigned i = 0; i < 3; i++) {
		world_open(&g.clients[i], false, false, 0, 0);
		game_connect(&g, i);
	}
	enet_sync_progress_t p;
	for (unsigned i = 0; i < 3; i++) {
		ecs_progress(g.clients[i].ecs, 0.001f);
		enet_sync_client_service(g.connections[i], 64, NULL);
		ecs_progress(g.clients[i].ecs, 0.001f);
	}
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	assert(p.events == 1 && p.connections == 1 && g.connected == 1);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	assert(p.events == 1 && p.connections == 1 && g.connected == 2);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	assert(g.connected == 3);
	game_close(&g);
}

static void client_host_failure(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	es_fail_service = 1;
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_TRANSPORT);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
	game_close(&g);
}

static void unreliable_packet(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	const char bytes[] = "invalid channel/flags";
	es_inject_client(bytes, sizeof(bytes), 0, 0);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
	enet_sync_status_t status;
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	assert(status.state == ENET_SYNC_FAULTED);
	game_close(&g);
}

static void peer_setup_failure(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 0);
	/* Other application recipients consume all core peer capacity. */
	for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		assert(sync_peer_open(g.server.session, &peer) == SYNC_OK);
	}
	world_open(&g.clients[0], false, false, 0, 0);
	game_connect(&g, 0);
	ecs_progress(g.clients[0].ecs, 0.001f);
	enet_sync_client_service(g.connections[0], 64, NULL);
	ecs_progress(g.clients[0].ecs, 0.001f);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_PARTIAL);
	assert(g.faults == 1 && !g.connected && !g.peers[0]);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_ACTIVE && !status.connections);
	assert(status.last_connection_error.detail.code == SYNC_ERR_LIMIT);
	game_close(&g);
}

static void count_frames(
	ecs_entity_t peer, const void *data, size_t size, void *ctx)
{
	uint32_t *count = ctx;
	assert(!peer);
	sync_frame_info_t info;
	assert(sync_frame_inspect(data, size, &info) == SYNC_OK);
	assert(info.sequence == (uint64_t)*count + 1);
	(*count)++;
}

static void client_frame_callback(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 0);
	world_open(&g.clients[0], false, false, 0, 0);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	uint32_t accepted = 0;
	enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options = {.on_frame = count_frames, .ctx = &accepted}};
	d.address.ip[0] = 127;
	d.address.ip[3] = 1;
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
	ecs_progress(g.clients[0].ecs, 0.001f);
	enet_sync_client_service(g.connections[0], 64, NULL);
	ecs_progress(g.clients[0].ecs, 0.001f);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	game_capture(&g, 1);
	game_settle(&g, 1, 1, false);
	assert(accepted == 4);
	game_close(&g);
}

static void automatic_custom_budget(void)
{
	struct world server, client;
	world_open(&server, true, false, 0, 0);
	world_open(&client, false, true, 0, 0);
	enet_sync_server_desc_t d = {.session = server.session,
		.options = {.schedule = SYNC_AUTOMATIC,
			.events_per_progress = 1,
			.frames_per_peer = 1}};
	enet_sync_server_t *listener;
	assert(enet_sync_server_listen(&d, &listener, NULL) == SYNC_OK);
	enet_sync_status_t status;
	ecs_progress(server.ecs, 0.001f);
	assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
	enet_sync_client_desc_t cd = {.session = client.session,
		.address = status.address,
		.options = {.schedule = SYNC_AUTOMATIC,
			.events_per_progress = 1,
			.frames_per_peer = 1}};
	cd.address.ip[0] = 127;
	cd.address.ip[3] = 1;
	enet_sync_client_t *connection;
	assert(enet_sync_client_connect(&cd, &connection, NULL) == SYNC_OK);
	memset(&d, 0xff, sizeof(d));
	for (unsigned i = 0; i < 4; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
	}
	sync_status_t core;
	assert(sync_get_status(server.session, 0, &core) == SYNC_OK);
	assert(core.peers == 1 && !core.last_tick && !core.queued_bytes);
	/* Automatic transport never captures a manual core session. */
	assert(sync_capture(server.session, 1, NULL) == SYNC_OK);
	ecs_progress(server.ecs, 0.01f);
	assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
	assert(status.frames == 1 && status.options.frames_per_peer == 1);
	assert(status.options.events_per_progress == 1);
	for (unsigned i = 0; i < 10; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
	}
	assert(sync_get_status(client.session, 0, &core) == SYNC_OK);
	assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
	assert(enet_sync_client_close(connection, NULL) == SYNC_OK);
	assert(enet_sync_server_close(listener, NULL) == SYNC_OK);
	world_close(&client);
	world_close(&server);
}

static void empty_packet(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 1);
	es_inject_client("", 0, 0, ENET_PACKET_FLAG_RELIABLE);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
	game_close(&g);
}

static void shared_world_ownership(void)
{
	struct world server;
	world_open(&server, true, false, 0, 0);
	flecs_enet_ids_t ids, again;
	assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
	assert(flecs_enet_import(server.ecs, &again) == FLECS_ENET_OK);
	assert(ids.module == again.module);
	ecs_entity_t external = ecs_new(server.ecs), request;
	flecs_enet_host_config_t config = {.listen = true};
	ecs_set_id(
		server.ecs, external, ids.host_config, sizeof(config), &config);
	flecs_enet_request_t open = {.operation = FLECS_ENET_OPEN};
	assert(flecs_enet_submit(server.ecs, external, &open, NULL, 0,
		       &request) == FLECS_ENET_OK);
	enet_sync_server_desc_t desc = {.session = server.session};
	enet_sync_server_t *a, *b;
	assert(enet_sync_server_listen(&desc, &a, NULL) == SYNC_OK);
	assert(enet_sync_server_listen(&desc, &b, NULL) == SYNC_OK);
	ecs_progress(server.ecs, 0.001f);
	assert(enet_sync_server_poll(a, 64, NULL) == SYNC_OK);
	assert(enet_sync_server_poll(b, 64, NULL) == SYNC_OK);
	enet_sync_status_t sa, sb;
	assert(enet_sync_server_get_status(a, &sa) == SYNC_OK);
	assert(enet_sync_server_get_status(b, &sb) == SYNC_OK);
	assert(sa.host != sb.host && sa.host != external);
	const flecs_enet_host_status_t *hs =
		ecs_get_id(server.ecs, sa.host, ids.host_status);
	assert(hs && !hs->pending_requests && !hs->retained_events);
	assert(enet_sync_server_close(a, NULL) == SYNC_OK);
	assert(!ecs_is_alive(server.ecs, sa.host));
	assert(ecs_is_alive(server.ecs, sb.host));
	assert(ecs_is_alive(server.ecs, external) && es_hosts_live == 2);
	assert(enet_sync_server_close(b, NULL) == SYNC_OK);
	assert(es_hosts_live == 1);
	ecs_delete(server.ecs, external);
	assert(!es_hosts_live);
	world_close(&server);
}

static void host_packet_limit(void)
{
	enet_sync_options_t options = {
		.transport_limits = {.max_outgoing_packets = 1}};
	struct game g;
	game_open(&g, false, 0, 0, &options, 2);
	es_hold_packets = 1;
	game_capture(&g, 1);
	enet_sync_progress_t p;
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	assert(p.frames == 8 && g.recorded == 8);
	ecs_progress(g.server.ecs, 0.001f);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.retained_packets == 1);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_OK);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	assert(status.retained_packets == 1);
	es_hold_packets = 0;
	es_release_packets();
	game_settle(&g, 2, 1, false);
	game_check(&g, 2);
	assert(g.recorded == 8);
	game_close(&g);
}

static void failed_send_cancellation(void)
{
	enet_sync_options_t options = {.transport_limits = {.max_events = 1}};
	struct game g;
	game_open(&g, false, 0, 0, &options, 2);
	game_capture(&g, 1);
	es_fail_send = 1;
	enet_sync_progress_t p;
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	assert(p.frames == 8 && g.recorded == 8);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
	assert(p.faulted_connections == 1 && !g.peers[0] && g.peers[1]);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	flecs_enet_ids_t ids;
	assert(flecs_enet_import(g.server.ecs, &ids) == FLECS_ENET_OK);
	const flecs_enet_host_status_t *h =
		ecs_get_id(g.server.ecs, status.host, ids.host_status);
	/* Four healthy sends and one local reset remain; failed peer sends
	 * gone. */
	assert(h->pending_requests == 5);
	for (unsigned i = 0; i < 10; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_service(g.listener, 64, 64, NULL) >= 0);
		ecs_progress(g.clients[1].ecs, 0.001f);
		assert(enet_sync_client_service(g.connections[1], 64, NULL) >=
			0);
		assert(sync_apply(g.clients[1].session, 64, NULL) >= 0);
	}
	h = ecs_get_id(g.server.ecs, status.host, ids.host_status);
	assert(!h->pending_requests && !h->retained_events);
	assert(replica(&g, 1, g.actors[0]));
	assert(g.recorded == 8);
	game_close(&g);
}

static void published_reader(ecs_iter_t *it)
{
	struct world *client = it->ctx;
	sync_status_t status;
	assert(sync_get_status(client->session, 0, &status) == SYNC_OK);
	assert(status.state == SYNC_CLIENT_READY && status.last_tick == 1);
}

static void same_frame_publication(void)
{
	struct game g;
	game_open(&g, false, 0, 0, NULL, 0);
	world_open(&g.clients[0], false, true, 0, 0);
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options.schedule = SYNC_AUTOMATIC};
	d.address.ip[0] = 127;
	d.address.ip[3] = 1;
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
	for (unsigned i = 0; i < 4; i++) {
		ecs_progress(g.clients[0].ecs, 0.001f);
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
	}
	assert(g.peers[0]);
	game_capture(&g, 1);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	ecs_progress(g.server.ecs, 0.001f);
	ecs_entity_t reader = ecs_new(g.clients[0].ecs);
	ecs_add_pair(g.clients[0].ecs, reader, EcsDependsOn, EcsPreUpdate);
	assert(ecs_system_init(g.clients[0].ecs,
		&(ecs_system_desc_t){.entity = reader,
			.callback = published_reader,
			.ctx = &g.clients[0],
			.immediate = true}));
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(replica(&g, 0, g.actors[0]));
	ecs_delete(g.clients[0].ecs, reader);
	game_close(&g);
}

static void pending_disconnect(void)
{
	struct world client;
	world_open(&client, false, false, 0, 0);
	enet_sync_client_desc_t d = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
	enet_sync_client_t *c;
	assert(enet_sync_client_connect(&d, &c, NULL) == SYNC_OK);
	assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
	assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_EMPTY);
	ecs_progress(client.ecs, 0.001f);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_END);
	enet_sync_status_t status;
	assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
	assert(status.state == ENET_SYNC_DISCONNECTED);
	assert(enet_sync_client_close(c, NULL) == SYNC_OK);
	world_close(&client);
}

static void ordered_receive_disconnect(void)
{
	struct game g;
	game_open(&g, false, 1024, 1024, NULL, 1);
	for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health value = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(value),
			&value);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	game_capture(&g, 1);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	ecs_progress(g.server.ecs, 0.001f);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
	assert(enet_sync_server_disconnect(g.listener, g.peers[0], 8) ==
		SYNC_OK);
	ecs_progress(g.server.ecs, 0.001f);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	ecs_progress(g.clients[0].ecs, 0.001f);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
	enet_sync_status_t status;
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	assert(status.state == ENET_SYNC_ACTIVE && status.pending_bytes);
	sync_result_t r = SYNC_OK;
	for (unsigned i = 0; i < 20 && r != SYNC_END; i++) {
		assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
		r = enet_sync_client_service(g.connections[0], 64, NULL);
		assert(r >= 0);
	}
	assert(r == SYNC_END);
	assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
	sync_status_t core;
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
	assert(core.entities == 203);
	game_close(&g);
}

static void phase_cycle(void)
{
	struct world server;
	world_open(&server, true, false, 0, 0);
	flecs_enet_ids_t ids;
	assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
	ecs_add_pair(server.ecs, server.ids.send_phase, EcsDependsOn,
		ids.send_phase);
	enet_sync_server_desc_t desc = {
		.session = server.session, .options.schedule = SYNC_AUTOMATIC};
	enet_sync_server_t *s = NULL;
	assert(enet_sync_server_listen(&desc, &s, NULL) == SYNC_ERR_STATE);
	assert(!s && !es_hosts_live);
	world_close(&server);
}

#endif

#define CASE(name) {#name, name},
struct test_case {
	const char *name;
	void (*run)(void);
};
static const struct test_case cases[] = {
#include "cases.def"
};
#undef CASE

int main(int argc, char **argv)
{
	assert(argc == 2);

	for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		if (strcmp(argv[1], cases[i].name))
			continue;
		cases[i].run();

		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", argv[1]);
	return 1;
}
