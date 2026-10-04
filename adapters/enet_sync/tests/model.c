#include "model.h"
#include <stdio.h>
#include <string.h>

static ecs_entity_t type(struct world *w, const char *name, size_t size,
	size_t alignment, const ecs_member_t *members, unsigned count)
{
	ecs_entity_t e = ecs_entity_init(w->ecs,
		&(ecs_entity_desc_t){ .name = name });
	ecs_component_desc_t component = { .entity = e };
	component.type.size = (ecs_size_t)size;
	component.type.alignment = (ecs_size_t)alignment;
	assert(ecs_component_init(w->ecs, &component) == e);
	ecs_struct_desc_t structure = { .entity = e };
	memcpy(structure.members, members, count * sizeof(*members));
	assert(ecs_struct_init(w->ecs, &structure) == e);
	ecs_add_id(w->ecs, e, w->ids.sync);
	return e;
}

void world_open(struct world *w, bool server, bool automatic,
	uint32_t frame_bytes, size_t receive_bytes)
{
	memset(w, 0, sizeof(*w));
	w->ecs = ecs_init();
	assert(sync_import(w->ecs, &w->ids) == SYNC_OK);
	ecs_member_t health[] = {
		{ .name = "value", .type = ecs_id(ecs_i32_t) }
	};
	w->health = type(w, "Health", sizeof(struct health),
		_Alignof(struct health), health, 1);
	ecs_member_t item[] = {
		{ .name = "owner", .type = ecs_id(ecs_entity_t) },
		{ .name = "quantity", .type = ecs_id(ecs_i32_t) }
	};
	w->item = type(w, "Item", sizeof(struct item),
		_Alignof(struct item), item, 2);
	sync_limits_t limits = { .max_frame_bytes = frame_bytes,
		.max_receive_bytes = receive_bytes };
	if (frame_bytes)
		limits.max_schema_bytes = frame_bytes - 64;
	sync_schedule_t schedule = automatic ? SYNC_AUTOMATIC : SYNC_MANUAL;
	sync_error_t error;
	sync_result_t r;
	if (server) {
		sync_server_desc_t d = { .schedule = schedule, .limits = limits };
		r = sync_server_create(w->ecs, &d, &w->session, &error);
	} else {
		sync_client_desc_t d = { .schedule = schedule, .limits = limits };
		r = sync_client_create(w->ecs, &d, &w->session, &error);
	}
	if (r != SYNC_OK)
		fprintf(stderr, "session: %d %s\n", r, error.message);
	assert(r == SYNC_OK);
}

void world_close(struct world *w)
{
	sync_session_destroy(w->session);
	assert(ecs_fini(w->ecs) == 0);
}

void game_policies(struct game *g)
{
	for (unsigned actor = 0; actor < 3; actor++) {
		for (unsigned view = 0; view < 3; view++) {
			if (!g->peers[view])
				continue;
			if (actor < 2 || view == 2)
				ecs_add_pair(g->server.ecs, g->actors[actor],
					g->server.ids.replicate_to, g->peers[view]);
			if (g->mask[actor] & (1u << view))
				ecs_add_pair(g->server.ecs, g->items[actor],
					g->server.ids.replicate_to, g->peers[view]);
			else
				ecs_remove_pair(g->server.ecs, g->items[actor],
					g->server.ids.replicate_to, g->peers[view]);
		}
	}
}

static void lifecycle(const enet_sync_event_t *e, void *ctx)
{
	struct game *g = ctx;
	if (e->kind == ENET_SYNC_CONNECTED) {
		assert(e->data >= 1 && e->data <= 3 && e->peer);
		g->peers[e->data - 1] = e->peer;
		g->connected++;
		sync_status_t core;
		assert(sync_get_status(g->server.session, e->peer, &core) ==
			SYNC_OK);
		assert(core.state == SYNC_PEER_PENDING && !core.queued_bytes);
		if (g->check_reentry) {
			assert(enet_sync_server_send(g->listener, 1, NULL) ==
				SYNC_ERR_STATE);
		}
		game_policies(g);
	} else {
		if (e->kind == ENET_SYNC_CONNECTION_FAULT) {
			assert(e->error.detail.code < 0);
			g->faults++;
		}
		g->disconnected++;
		for (unsigned i = 0; i < 3; i++) {
			if (g->peers[i] == e->peer)
				g->peers[i] = 0;
		}
	}
}

static void record(ecs_entity_t peer, const void *data, size_t size, void *ctx)
{
	struct game *g = ctx;
	sync_frame_info_t info;
	assert(sync_frame_inspect(data, size, &info) == SYNC_OK);
	g->recorded++;
	if (info.kind == SYNC_FRAME_FRAGMENT)
		g->fragments++;
	if (size > g->max_packet)
		g->max_packet = size;
#ifdef ES_FILE_SYNC
	for (unsigned i = 0; i < 3; i++) {
		if (g->peers[i] == peer && g->writers[i]) {
			sync_result_t r = file_sync_writer_write(g->writers[i],
				data, size);
			if (r != SYNC_OK)
				g->recording_error = r;
		}
	}
#else
	(void)peer;
#endif
}

void game_connect(struct game *g, unsigned view)
{
	enet_sync_status_t status;
	assert(enet_sync_server_get_status(g->listener, &status) == SYNC_OK);
	flecs_enet_address_t address = status.address;
	address.ip[0] = 127;
	address.ip[3] = 1;
	enet_sync_client_desc_t d = { .session = g->clients[view].session,
		.address = address, .data = view + 1 };
	if (status.schedule == SYNC_AUTOMATIC)
		d.options.schedule = SYNC_AUTOMATIC;
	enet_sync_error_t error;
	sync_result_t r = enet_sync_client_connect(&d, &g->connections[view],
		&error);
	if (r != SYNC_OK)
		fprintf(stderr, "connect: %d %s\n", r, error.detail.message);
	assert(r == SYNC_OK);
}

void game_open(struct game *g, bool automatic, uint32_t frame_bytes,
	size_t receive_bytes, const enet_sync_options_t *options, unsigned clients)
{
	memset(g, 0, sizeof(*g));
	g->check_reentry = true;
	world_open(&g->server, true, automatic, frame_bytes, 0);
	const char *names[] = {"Aria", "Bram", "NPC"};
	for (unsigned i = 0; i < 3; i++) {
		g->actors[i] = ecs_entity_init(g->server.ecs,
			&(ecs_entity_desc_t){ .name = names[i] });
		struct health h = {100 + (int32_t)i};
		ecs_set_id(g->server.ecs, g->actors[i], g->server.health,
			sizeof(h), &h);
		g->items[i] = ecs_new(g->server.ecs);
		struct item item = { g->actors[i], 10 + (int32_t)i };
		ecs_set_id(g->server.ecs, g->items[i], g->server.item,
			sizeof(item), &item);
		g->mask[i] = (1u << i) | 4;
	}
	enet_sync_server_desc_t d = { .session = g->server.session };
	if (options)
		d.options = *options;
	if (automatic)
		d.options.schedule = SYNC_AUTOMATIC;
	d.options.on_event = lifecycle;
	d.options.on_frame = record;
	d.options.ctx = g;
	enet_sync_error_t error;
	sync_result_t r = enet_sync_server_listen(&d, &g->listener, &error);
	if (r != SYNC_OK)
		fprintf(stderr, "listen: %d %s\n", r, error.detail.message);
	assert(r == SYNC_OK);
	ecs_progress(g->server.ecs, 0.001f);
	if (!automatic)
		assert(enet_sync_server_poll(g->listener, 64, NULL) >= 0);
	for (unsigned i = 0; i < clients; i++) {
		world_open(&g->clients[i], false, automatic, frame_bytes,
			receive_bytes);
		game_connect(g, i);
	}
	for (unsigned step = 0; step < 5000; step++) {
		if (automatic) {
			ecs_progress(g->server.ecs, 0.001f);
			for (unsigned i = 0; i < clients; i++)
				ecs_progress(g->clients[i].ecs, 0.001f);
		} else {
			ecs_progress(g->server.ecs, 0.001f);
			for (unsigned i = 0; i < clients; i++)
				ecs_progress(g->clients[i].ecs, 0.001f);
			assert(enet_sync_server_poll(g->listener, 64, NULL) >= 0);
			for (unsigned i = 0; i < clients; i++)
				assert(enet_sync_client_service(g->connections[i],
					64, NULL) >= 0);
		}
		bool ready = true;
		for (unsigned i = 0; i < clients; i++) {
			enet_sync_status_t s;
			assert(enet_sync_client_get_status(g->connections[i], &s) ==
				SYNC_OK);
			if (s.state != ENET_SYNC_ACTIVE || !g->peers[i])
				ready = false;
		}
		if (ready)
			return;
	}
	assert(!"connection timeout");
}

void game_close(struct game *g)
{
	for (unsigned i = 0; i < 3; i++) {
		if (!g->clients[i].ecs)
			continue;
		enet_sync_client_close(g->connections[i], NULL);
		world_close(&g->clients[i]);
#ifdef ES_FILE_SYNC
		if (g->writers[i])
			assert(file_sync_writer_close(g->writers[i], NULL) == SYNC_OK);
#endif
	}
	enet_sync_server_close(g->listener, NULL);
	world_close(&g->server);
#ifdef ENET_SYNC_TESTING
	assert(es_hosts_live == 0);
#endif
}

void game_capture(struct game *g, uint64_t tick)
{
	assert(sync_capture(g->server.session, tick, NULL) == SYNC_OK);
}

void game_settle(struct game *g, unsigned clients, uint64_t tick,
	bool automatic)
{
	for (unsigned step = 0; step < 5000; step++) {
		if (automatic) {
			ecs_progress(g->server.ecs, 0.06f);
			for (unsigned i = 0; i < clients; i++)
				ecs_progress(g->clients[i].ecs, 0.06f);
		} else {
			ecs_progress(g->server.ecs, 0.001f);
			for (unsigned i = 0; i < clients; i++)
				ecs_progress(g->clients[i].ecs, 0.001f);
			assert(enet_sync_server_service(g->listener, 64, 64, NULL) >= 0);
			for (unsigned i = 0; i < clients; i++) {
				assert(enet_sync_client_service(g->connections[i],
					64, NULL) >= 0);
				assert(sync_apply(g->clients[i].session, 64, NULL) >= 0);
			}
		}
		bool ready = true;
		for (unsigned i = 0; i < clients; i++) {
			sync_status_t s;
			assert(sync_get_status(g->clients[i].session, 0, &s) ==
				SYNC_OK);
			if (s.last_tick < tick)
				ready = false;
		}
		if (ready)
			return;
	}
	assert(!"replication timeout");
}

ecs_entity_t replica(struct game *g, unsigned view, ecs_entity_t entity)
{
	const sync_network_id_t *id = ecs_get_id(g->server.ecs, entity,
		g->server.ids.network_id);
	if (!id)
		return 0;
	ecs_entity_t local = 0;
	sync_result_t r = sync_lookup(g->clients[view].session, id->value, &local);
	assert(r == SYNC_OK || r == SYNC_ERR_NOT_FOUND);
	return local;
}

void game_check(struct game *g, unsigned clients)
{
	for (unsigned view = 0; view < clients; view++) {
		for (unsigned actor = 0; actor < 3; actor++) {
			ecs_entity_t local = replica(g, view, g->actors[actor]);
			assert((local != 0) == (actor < 2 || view == 2));
			if (local) {
				const struct health *actual = ecs_get_id(
					g->clients[view].ecs, local,
					g->clients[view].health);
				const struct health *expected = ecs_get_id(
					g->server.ecs, g->actors[actor],
					g->server.health);
				assert(actual && actual->value == expected->value);
			}
			local = replica(g, view, g->items[actor]);
			assert((local != 0) == !!(g->mask[actor] & (1u << view)));
			if (!local)
				continue;
			const struct item *actual = ecs_get_id(g->clients[view].ecs,
				local, g->clients[view].item);
			const struct item *expected = ecs_get_id(g->server.ecs,
				g->items[actor], g->server.item);
			assert(actual && actual->quantity == expected->quantity);
			assert(actual->owner == replica(g, view, expected->owner));
		}
	}
}
