#include <enet_sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ES_BYTES ((size_t)32 * 1024 * 1024)

struct es_base {
	sync_session_t *session;
	sync_session_info_t info;
	enet_sync_status_t status;
	ecs_entity_t host;
	flecs_enet_ids_t ids;
	ecs_query_t *events;
	ecs_query_t *requests;
	ecs_entity_t receive_system;
	ecs_entity_t send_system;
	bool busy;
};

/* Full entity IDs preserve transport generation identity across reconnects. */
struct es_connection {
	ecs_entity_t transport;
	flecs_enet_address_t address;
	ecs_entity_t peer;
	bool closing;
	uint32_t data;
};

struct enet_sync_server {
	struct es_base base;
	struct es_connection *connections;
	uint32_t capacity;
};

struct enet_sync_client {
	struct es_base base;
	ecs_entity_t transport;
	ecs_entity_t pending;
	uint32_t connect_data;
	bool closing;
	uint32_t data;
};

static void zero(void *out, size_t size)
{
	if (out)
		memset(out, 0, size);
}

static enet_sync_error_t diagnostic(sync_result_t code, const char *message,
	ecs_entity_t peer, const flecs_enet_address_t *address)
{
	enet_sync_error_t e = {0};
	e.detail.code = code;
	snprintf(e.detail.message, sizeof(e.detail.message), "%s", message);
	e.peer = peer;
	if (address)
		e.address = *address;
	return e;
}

static void event(struct es_base *b, enet_sync_event_kind_t kind,
	ecs_entity_t peer, flecs_enet_address_t address, uint32_t data,
	const enet_sync_error_t *error)
{
	enet_sync_event_t e = {
		.kind = kind, .peer = peer, .address = address, .data = data};
	if (error)
		e.error = *error;
	if (b->status.options.on_event)
		b->status.options.on_event(&e, b->status.options.ctx);
}

static sync_result_t result(flecs_enet_result_t r)
{
	switch (r) {
	case FLECS_ENET_OK:
		return SYNC_OK;
	case FLECS_ENET_BACKPRESSURE:
		return SYNC_BACKPRESSURE;
	case FLECS_ENET_NOMEM:
		return SYNC_ERR_NOMEM;
	case FLECS_ENET_LIMIT:
		return SYNC_ERR_LIMIT;
	case FLECS_ENET_ARGUMENT:
		return SYNC_ERR_ARGUMENT;
	case FLECS_ENET_STATE:
		return SYNC_ERR_STATE;
	case FLECS_ENET_NOT_FOUND:
		return SYNC_ERR_NOT_FOUND;
	default:
		return SYNC_ERR_TRANSPORT;
	}
}

static void accepted(struct es_base *b, ecs_entity_t peer,
	const flecs_enet_packet_t *packet, enet_sync_progress_t *p)
{
	p->frames++;
	p->bytes += packet->size;
	b->status.frames++;
	b->status.bytes += packet->size;
	if (b->status.options.on_frame)
		b->status.options.on_frame(peer, packet->data, packet->size,
			b->status.options.ctx);
}

static sync_result_t configure(struct es_base *b, sync_session_t *session,
	const enet_sync_options_t *options, sync_role_t role)
{
	if (!session ||
		(options->schedule != SYNC_MANUAL &&
			options->schedule != SYNC_AUTOMATIC))
		return SYNC_ERR_ARGUMENT;
	if (options->schedule == SYNC_MANUAL &&
		(options->events_per_progress || options->frames_per_peer))
		return SYNC_ERR_ARGUMENT;
	sync_result_t r = sync_get_session_info(session, &b->info);
	if (r != SYNC_OK)
		return r;
	if (b->info.role != role)
		return SYNC_ERR_ROLE;
	sync_status_t s;
	r = sync_get_status(session, 0, &s);
	if (r != SYNC_OK)
		return r;
	if ((role == SYNC_ROLE_SERVER && s.state != SYNC_SERVER_READY) ||
		(role == SYNC_ROLE_CLIENT &&
			(s.state != SYNC_CLIENT_WAIT_MANIFEST ||
				s.queued_bytes || s.assembly_bytes)))
		return SYNC_ERR_STATE;
	if (role == SYNC_ROLE_CLIENT && options->schedule == SYNC_AUTOMATIC &&
		b->info.schedule != SYNC_AUTOMATIC)
		return SYNC_ERR_STATE;
	b->session = session;
	b->status.schedule = options->schedule;
	b->status.max_frame_bytes = b->info.limits.max_frame_bytes;
	b->status.options = *options;
	enet_sync_options_t *o = &b->status.options;
	flecs_enet_limits_t *l = &o->transport_limits;
	size_t bound = b->status.max_frame_bytes;
	if ((l->max_packet_bytes && l->max_packet_bytes != bound) ||
		bound > ES_BYTES ||
		(l->max_event_bytes && l->max_event_bytes < bound) ||
		(l->max_outgoing_bytes && l->max_outgoing_bytes < bound) ||
		(l->max_waiting_bytes && l->max_waiting_bytes < bound))
		return SYNC_ERR_LIMIT;
	l->max_packet_bytes = bound;
	if (o->schedule == SYNC_AUTOMATIC) {
		if (!o->events_per_progress)
			o->events_per_progress = 64;
		if (!o->frames_per_peer)
			o->frames_per_peer = 64;
	}
	return SYNC_OK;
}

static void remove_systems(struct es_base *b)
{
	if (b->receive_system)
		ecs_delete(b->info.world, b->receive_system);
	if (b->send_system)
		ecs_delete(b->info.world, b->send_system);
}

static sync_result_t install(struct es_base *b, ecs_entity_t *id,
	ecs_entity_t phase, ecs_iter_action_t callback, void *ctx)
{
	*id = ecs_new(b->info.world);
	ecs_add_pair(b->info.world, *id, EcsDependsOn, phase);
	if (!ecs_system_init(b->info.world,
		    &(ecs_system_desc_t){.entity = *id,
			    .callback = callback,
			    .ctx = ctx,
			    .immediate = true,
			    .query.terms = {{.id = EcsWildcard,
				    .src.id = EcsIsEntity,
				    .inout = EcsInOut}}}))
		return SYNC_ERR_RUNTIME;
	return SYNC_OK;
}

/* Cancel only unfinished requests for this generation-aware association. */
static void cancel_peer(struct es_base *b, ecs_entity_t peer)
{
	for (;;) {
		ecs_entity_t found = 0;
		ecs_iter_t it = ecs_query_iter(b->info.world, b->requests);
		while (ecs_query_next(&it)) {
			const flecs_enet_request_t *r =
				ecs_field(&it, flecs_enet_request_t, 0);
			for (int32_t i = 0; i < it.count; i++) {
				if (r[i].peer == peer) {
					found = it.entities[i];
					break;
				}
			}
			if (found) {
				ecs_iter_fini(&it);
				break;
			}
		}
		if (!found)
			break;
		ecs_delete(b->info.world, found);
	}
}

static void reset_peer(struct es_base *b, ecs_entity_t peer)
{
	if (!peer)
		return;
	cancel_peer(b, peer);
	ecs_entity_t request;
	flecs_enet_disconnect(
		b->info.world, b->host, peer, FLECS_ENET_RESET, 0, 0, &request);
}

static void server_end(enet_sync_server_t *s, struct es_connection *c,
	uint32_t data, const enet_sync_error_t *error, enet_sync_progress_t *p)
{
	struct es_base *b = &s->base;
	enet_sync_event_kind_t kind = ENET_SYNC_DISCONNECTED_EVENT;
	cancel_peer(b, c->transport);
	if (error) {
		kind = ENET_SYNC_CONNECTION_FAULT;
		b->status.last_connection_error = *error;
		p->faulted_connections++;
		reset_peer(b, c->transport);
	}
	event(b, kind, c->peer, c->address, data, error);
	if (c->peer) {
		sync_peer_close(b->session, c->peer);
		b->status.connections--;
	}
	c->peer = 0;
	c->transport = 0;
	c->closing = false;
	p->disconnections++;
}

static sync_result_t server_fault(
	enet_sync_server_t *s, sync_result_t code, enet_sync_progress_t *p)
{
	struct es_base *b = &s->base;
	b->status.fault = diagnostic(code, "transport host failed", 0, NULL);
	b->status.state = ENET_SYNC_FAULTED;
	for (uint32_t i = 0; i < s->capacity; i++) {
		struct es_connection *c = &s->connections[i];
		if (c->peer) {
			enet_sync_error_t e = b->status.fault;
			e.peer = c->peer;
			e.address = c->address;
			server_end(s, c, 0, &e, p);
		}
	}
	if (!p->faulted_connections)
		event(b, ENET_SYNC_CONNECTION_FAULT, 0, b->status.address, 0,
			&b->status.fault);
	return code;
}

static sync_result_t client_end(enet_sync_client_t *c, sync_result_t code,
	const char *message, uint32_t data, enet_sync_progress_t *p)
{
	struct es_base *b = &c->base;
	enet_sync_event_kind_t kind = ENET_SYNC_DISCONNECTED_EVENT;
	if (code < 0) {
		b->status.fault =
			diagnostic(code, message, 0, &b->status.address);
		sync_status_t core;
		if (sync_get_status(b->session, 0, &core) == SYNC_OK &&
			core.fault.code)
			b->status.fault.detail = core.fault;
		b->status.state = ENET_SYNC_FAULTED;
		kind = ENET_SYNC_CONNECTION_FAULT;
		p->faulted_connections++;
		reset_peer(b, c->transport);
	} else {
		b->status.state = ENET_SYNC_DISCONNECTED;
	}
	if (c->pending) {
		ecs_delete(b->info.world, c->pending);
		c->pending = 0;
	}
	b->status.connections = 0;
	c->closing = false;
	p->disconnections++;
	event(b, kind, 0, b->status.address, data,
		code < 0 ? &b->status.fault : NULL);
	return code < 0 ? code : SYNC_END;
}

/* Event snapshots survive structural changes; packet storage lasts to consume.
 */
static ecs_entity_t next_event(struct es_base *b, flecs_enet_event_t *out)
{
	ecs_entity_t found = 0;
	uint64_t sequence = UINT64_MAX;
	ecs_iter_t it = ecs_query_iter(b->info.world, b->events);
	while (ecs_query_next(&it)) {
		const flecs_enet_event_t *e =
			ecs_field(&it, flecs_enet_event_t, 0);
		for (int32_t i = 0; i < it.count; i++) {
			if (!found || e[i].sequence < sequence) {
				sequence = e[i].sequence;
				found = it.entities[i];
				*out = e[i];
			}
		}
	}
	return found;
}

static void consume(
	struct es_base *b, ecs_entity_t id, const flecs_enet_event_t *e)
{
	if (e->request && ecs_is_alive(b->info.world, e->request))
		ecs_delete(b->info.world, e->request);
	flecs_enet_consume(b->info.world, b->host, id);
}

static sync_result_t host_fault(struct es_base *b)
{
	const flecs_enet_host_status_t *h =
		ecs_get_id(b->info.world, b->host, b->ids.host_status);
	return h && h->state == FLECS_ENET_HOST_FAULTED ? result(h->fault)
							: SYNC_OK;
}

static sync_result_t poll_server(
	enet_sync_server_t *s, uint32_t budget, enet_sync_progress_t *p)
{
	struct es_base *b = &s->base;
	sync_result_t r = host_fault(b);
	if (r < 0)
		return server_fault(s, r, p);
	bool partial = false;
	for (uint32_t n = 0; n < budget; n++) {
		flecs_enet_event_t e;
		ecs_entity_t id = next_event(b, &e);
		if (!id)
			break;
		p->events++;
		struct es_connection *c = NULL;
		for (uint32_t i = 0; i < s->capacity; i++) {
			if (s->connections[i].transport == e.peer && e.peer) {
				c = &s->connections[i];
				break;
			}
		}
		if (e.kind == FLECS_ENET_EVENT_RESULT) {
			const flecs_enet_operation_result_t *v =
				ecs_get_id(b->info.world, id, b->ids.result);
			flecs_enet_operation_result_t vcopy = *v;
			consume(b, id, &e);
			if (vcopy.operation == FLECS_ENET_OPEN) {
				if (vcopy.code != FLECS_ENET_OK)
					return server_fault(
						s, result(vcopy.code), p);
				b->status.state = ENET_SYNC_ACTIVE;
			} else if (vcopy.code != FLECS_ENET_OK && c) {
				enet_sync_error_t err =
					diagnostic(result(vcopy.code),
						"transport request failed",
						c->peer, &c->address);
				server_end(s, c, 0, &err, p);
				partial = true;
			}
			continue;
		}
		if (e.kind == FLECS_ENET_EVENT_CONNECT) {
			for (uint32_t i = 0; i < s->capacity; i++) {
				if (!s->connections[i].transport) {
					c = &s->connections[i];
					break;
				}
			}
			if (!c) {
				consume(b, id, &e);
				return server_fault(s, SYNC_ERR_LIMIT, p);
			}
			c->transport = e.peer;
			const flecs_enet_peer_status_t *ps = ecs_get_id(
				b->info.world, e.peer, b->ids.peer_status);
			c->address = ps->address;
			r = sync_peer_open(b->session, &c->peer);
			if (r != SYNC_OK) {
				enet_sync_error_t err =
					diagnostic(r, "Sync peer setup failed",
						0, &c->address);
				server_end(s, c, e.data, &err, p);
				partial = true;
			} else {
				b->status.connections++;
				p->connections++;
				event(b, ENET_SYNC_CONNECTED, c->peer,
					c->address, e.data, NULL);
			}
		} else if (c && e.kind == FLECS_ENET_EVENT_DISCONNECT) {
			server_end(s, c, e.data, NULL, p);
		} else if (c && e.kind == FLECS_ENET_EVENT_RECEIVE) {
			enet_sync_error_t err = diagnostic(SYNC_ERR_PROTOCOL,
				"upstream packets are unsupported", c->peer,
				&c->address);
			server_end(s, c, 0, &err, p);
			partial = true;
		}
		consume(b, id, &e);
	}
	return partial ? SYNC_PARTIAL : p->events ? SYNC_OK : SYNC_EMPTY;
}

static sync_result_t send_server(
	enet_sync_server_t *s, uint32_t budget, enet_sync_progress_t *p)
{
	struct es_base *b = &s->base;
	sync_result_t fault = host_fault(b);
	if (fault < 0)
		return server_fault(s, fault, p);
	if (b->status.state == ENET_SYNC_OPENING)
		return SYNC_EMPTY;
	bool pressure = false, partial = false;
	for (uint32_t i = 0; i < s->capacity; i++) {
		struct es_connection *c = &s->connections[i];
		if (!c->peer || c->closing)
			continue;
		for (uint32_t j = 0; j < budget; j++) {
			sync_frame_view_t v;
			sync_result_t r =
				sync_outbox_peek(b->session, c->peer, &v);
			if (r == SYNC_EMPTY)
				break;
			ecs_entity_t request = 0;
			if (r == SYNC_OK)
				r = result(flecs_enet_send(b->info.world,
					b->host, c->transport, 0,
					FLECS_ENET_RELIABLE, v.data, v.size, 0,
					&request));
			if (r == SYNC_BACKPRESSURE) {
				pressure = true;
				break;
			}
			if (r == SYNC_OK)
				r = sync_outbox_commit(
					b->session, c->peer, v.token);
			if (r != SYNC_OK) {
				enet_sync_error_t e = diagnostic(r,
					"connection send/core failure", c->peer,
					&c->address);
				sync_status_t core;
				if (sync_get_status(b->session, c->peer,
					    &core) == SYNC_OK &&
					core.fault.code)
					e.detail = core.fault;
				server_end(s, c, 0, &e, p);
				partial = true;
				break;
			}
			const flecs_enet_packet_t *packet = ecs_get_id(
				b->info.world, request, b->ids.packet);
			flecs_enet_packet_t copy = *packet;
			accepted(b, c->peer, &copy, p);
		}
	}
	return partial	    ? SYNC_PARTIAL
		: pressure  ? SYNC_BACKPRESSURE
		: p->frames ? SYNC_OK
			    : SYNC_EMPTY;
}

static sync_result_t service_client(
	enet_sync_client_t *c, uint32_t budget, enet_sync_progress_t *p)
{
	struct es_base *b = &c->base;
	sync_status_t core;
	sync_result_t r = sync_get_status(b->session, 0, &core);
	if (r != SYNC_OK || core.state == SYNC_FAULTED)
		return client_end(c, r != SYNC_OK ? r : core.fault.code,
			"borrowed Sync client faulted", 0, p);
	r = host_fault(b);
	if (r < 0)
		return client_end(c, r, "transport host failed", 0, p);
	for (uint32_t n = 0; n < budget; n++) {
		flecs_enet_event_t e;
		ecs_entity_t id = next_event(b, &e);
		if (!id)
			break;
		if (e.kind == FLECS_ENET_EVENT_RECEIVE) {
			const flecs_enet_packet_t *packet =
				ecs_get_id(b->info.world, id, b->ids.packet);
			flecs_enet_packet_t copy = *packet;
			if (!copy.size || e.channel ||
				copy.delivery != FLECS_ENET_RELIABLE) {
				consume(b, id, &e);
				return client_end(c, SYNC_ERR_PROTOCOL,
					"expected nonempty reliable channel "
					"zero",
					0, p);
			}
			r = sync_receive(b->session, copy.data, copy.size);
			if (r == SYNC_BACKPRESSURE) {
				c->pending = id;
				return r;
			}
			c->pending = 0;
			if (r != SYNC_OK) {
				consume(b, id, &e);
				return client_end(
					c, r, "Sync rejected frame", 0, p);
			}
			accepted(b, 0, &copy, p);
		} else if (e.kind == FLECS_ENET_EVENT_CONNECT) {
			c->transport = e.peer;
			b->status.state = ENET_SYNC_ACTIVE;
			b->status.connections = 1;
			p->connections++;
			event(b, ENET_SYNC_CONNECTED, 0, b->status.address,
				e.data, NULL);
		} else if (e.kind == FLECS_ENET_EVENT_DISCONNECT) {
			consume(b, id, &e);
			p->events++;
			return client_end(c, SYNC_OK, NULL,
				c->closing ? c->data : e.data, p);
		} else if (e.kind == FLECS_ENET_EVENT_RESULT) {
			const flecs_enet_operation_result_t *v =
				ecs_get_id(b->info.world, id, b->ids.result);
			flecs_enet_operation_result_t vcopy = *v;
			consume(b, id, &e);
			p->events++;
			if (c->closing && vcopy.operation == FLECS_ENET_CLOSE)
				return client_end(c, result(vcopy.code),
					"transport close failed", c->data, p);
			if (c->closing && !c->transport)
				continue;
			if (vcopy.code != FLECS_ENET_OK)
				return client_end(c, result(vcopy.code),
					"transport request failed", 0, p);
			if (vcopy.operation == FLECS_ENET_OPEN) {
				flecs_enet_request_t d = {
					.operation = FLECS_ENET_CONNECT,
					.address = b->status.address,
					.channels = 1,
					.data = c->connect_data};
				ecs_entity_t request;
				r = result(flecs_enet_submit(b->info.world,
					b->host, &d, NULL, 0, &request));
				if (r != SYNC_OK)
					return client_end(c, r,
						"connect admission failed", 0,
						p);
			} else if (vcopy.operation == FLECS_ENET_CONNECT) {
				c->transport = vcopy.peer;
			}
			continue;
		}
		p->events++;
		consume(b, id, &e);
	}
	return p->events || p->frames ? SYNC_OK : SYNC_EMPTY;
}

static bool depends(ecs_world_t *world, ecs_entity_t from, ecs_entity_t target,
	unsigned depth)
{
	if (from == target || depth > 128)
		return true;
	for (int32_t i = 0;; i++) {
		ecs_entity_t next =
			ecs_get_target(world, from, EcsDependsOn, i);
		if (!next)
			return false;
		if (depends(world, next, target, depth + 1))
			return true;
	}
}

static sync_result_t open_host(struct es_base *b, bool listen,
	flecs_enet_address_t address, uint32_t capacity)
{
	sync_result_t r = result(flecs_enet_import(b->info.world, &b->ids));
	if (r != SYNC_OK)
		return r;
	if (depends(b->info.world, b->ids.receive_phase,
		    b->info.ids.receive_phase, 0) ||
		depends(b->info.world, b->info.ids.send_phase,
			b->ids.send_phase, 0) ||
		depends(b->info.world, b->info.ids.apply_phase, EcsPreUpdate,
			0) ||
		depends(b->info.world, EcsPostUpdate, b->info.ids.capture_phase,
			0))
		return SYNC_ERR_STATE;
	ecs_add_pair(b->info.world, EcsPreUpdate, EcsDependsOn,
		b->info.ids.apply_phase);
	ecs_add_pair(b->info.world, b->info.ids.capture_phase, EcsDependsOn,
		EcsPostUpdate);
	ecs_add_pair(b->info.world, b->info.ids.receive_phase, EcsDependsOn,
		b->ids.receive_phase);
	ecs_add_pair(b->info.world, b->ids.send_phase, EcsDependsOn,
		b->info.ids.send_phase);
	b->host = ecs_new(b->info.world);
	b->status.host = b->host;
	flecs_enet_host_config_t config = {.listen = listen,
		.address = address,
		.peer_limit = capacity,
		.channel_limit = 1,
		.limits = b->status.options.transport_limits};
	ecs_set_id(b->info.world, b->host, b->ids.host_config, sizeof(config),
		&config);
	b->events = ecs_query_init(b->info.world,
		&(ecs_query_desc_t){
			.terms = {{.id = b->ids.event},
				{.id = ecs_pair(EcsChildOf, b->host)}}});
	b->requests = ecs_query_init(b->info.world,
		&(ecs_query_desc_t){
			.terms = {{.id = b->ids.request},
				{.id = ecs_pair(EcsChildOf, b->host)}}});
	if (!b->events || !b->requests)
		return SYNC_ERR_NOMEM;
	flecs_enet_request_t d = {.operation = FLECS_ENET_OPEN};
	ecs_entity_t request;
	return result(flecs_enet_submit(
		b->info.world, b->host, &d, NULL, 0, &request));
}

static void destroy_host(struct es_base *b)
{
	if (b->events)
		ecs_query_fini(b->events);
	if (b->requests)
		ecs_query_fini(b->requests);
	if (b->host)
		ecs_delete(b->info.world, b->host);
}

static void server_receive_system(ecs_iter_t *it)
{
	enet_sync_server_t *s = it->ctx;
	if (s->base.status.state == ENET_SYNC_FAULTED)
		return;
	enet_sync_progress_t p = {0};
	bool deferred = ecs_is_deferred(it->world);
	if (deferred)
		ecs_defer_suspend(it->world);
	s->base.busy = true;
	poll_server(s, s->base.status.options.events_per_progress, &p);
	s->base.busy = false;
	if (deferred)
		ecs_defer_resume(it->world);
}

static void server_send_system(ecs_iter_t *it)
{
	enet_sync_server_t *s = it->ctx;
	if (s->base.status.state != ENET_SYNC_ACTIVE)
		return;
	enet_sync_progress_t p = {0};
	bool deferred = ecs_is_deferred(it->world);
	if (deferred)
		ecs_defer_suspend(it->world);
	s->base.busy = true;
	send_server(s, s->base.status.options.frames_per_peer, &p);
	s->base.busy = false;
	if (deferred)
		ecs_defer_resume(it->world);
}

static void client_receive_system(ecs_iter_t *it)
{
	enet_sync_client_t *c = it->ctx;
	if (c->base.status.state != ENET_SYNC_ACTIVE &&
		c->base.status.state != ENET_SYNC_CONNECTING)
		return;
	enet_sync_progress_t p = {0};
	bool deferred = ecs_is_deferred(it->world);
	if (deferred)
		ecs_defer_suspend(it->world);
	c->base.busy = true;
	service_client(c, c->base.status.options.events_per_progress, &p);
	c->base.busy = false;
	if (deferred)
		ecs_defer_resume(it->world);
}

sync_result_t enet_sync_server_listen(const enet_sync_server_desc_t *desc,
	enet_sync_server_t **out_server, enet_sync_error_t *out_error)
{
	zero(out_server, sizeof(*out_server));
	zero(out_error, sizeof(*out_error));
	if (!desc || !out_server)
		return SYNC_ERR_ARGUMENT;
	uint32_t capacity = desc->max_clients ? desc->max_clients : 32;
	if (capacity > 4095)
		return SYNC_ERR_ARGUMENT;
	enet_sync_server_t *s = calloc(1, sizeof(*s));
	if (!s)
		return SYNC_ERR_NOMEM;
	struct es_base *b = &s->base;
	sync_result_t r =
		configure(b, desc->session, &desc->options, SYNC_ROLE_SERVER);
	if (r != SYNC_OK)
		goto fail;
	s->capacity = capacity;
	s->connections = calloc(capacity, sizeof(*s->connections));
	if (!s->connections) {
		r = SYNC_ERR_NOMEM;
		goto fail;
	}
	r = open_host(b, true, desc->address, capacity);
	if (r != SYNC_OK)
		goto fail;
	b->status.address = desc->address;
	b->status.state = ENET_SYNC_OPENING;
	if (b->status.schedule == SYNC_AUTOMATIC) {
		r = install(b, &b->receive_system, b->info.ids.receive_phase,
			server_receive_system, s);
		if (r == SYNC_OK)
			r = install(b, &b->send_system, b->info.ids.send_phase,
				server_send_system, s);
		if (r != SYNC_OK)
			goto fail;
	}
	*out_server = s;
	return SYNC_OK;
fail:
	if (out_error)
		*out_error = diagnostic(r, "server listen failed", 0, NULL);
	remove_systems(b);
	destroy_host(b);
	free(s->connections);
	free(s);
	return r;
}

sync_result_t enet_sync_client_connect(const enet_sync_client_desc_t *desc,
	enet_sync_client_t **out_client, enet_sync_error_t *out_error)
{
	zero(out_client, sizeof(*out_client));
	zero(out_error, sizeof(*out_error));
	if (!desc || !out_client ||
		!(desc->address.ip[0] || desc->address.ip[1] ||
			desc->address.ip[2] || desc->address.ip[3]) ||
		!desc->address.port ||
		(desc->address.ip[0] == 255 && desc->address.ip[1] == 255 &&
			desc->address.ip[2] == 255 &&
			desc->address.ip[3] == 255))
		return SYNC_ERR_ARGUMENT;
	enet_sync_client_t *c = calloc(1, sizeof(*c));
	if (!c)
		return SYNC_ERR_NOMEM;
	struct es_base *b = &c->base;
	sync_result_t r =
		configure(b, desc->session, &desc->options, SYNC_ROLE_CLIENT);
	if (r != SYNC_OK)
		goto fail;
	r = open_host(b, false, (flecs_enet_address_t){0}, 1);
	if (r != SYNC_OK)
		goto fail;
	b->status.address = desc->address;
	c->connect_data = desc->data;
	b->status.state = ENET_SYNC_CONNECTING;
	if (b->status.schedule == SYNC_AUTOMATIC) {
		r = install(b, &b->receive_system, b->info.ids.receive_phase,
			client_receive_system, c);
		if (r != SYNC_OK)
			goto fail;
	}
	*out_client = c;
	return SYNC_OK;
fail:
	if (out_error)
		*out_error = diagnostic(
			r, "client connect failed", 0, &desc->address);
	remove_systems(b);
	destroy_host(b);
	free(c);
	return r;
}

static sync_result_t manual(const struct es_base *b)
{
	if (b->busy || ecs_is_deferred(b->info.world) ||
		(ecs_world_get_flags(b->info.world) & EcsWorldReadonly) ||
		b->status.schedule != SYNC_MANUAL ||
		b->status.state == ENET_SYNC_FAULTED)
		return SYNC_ERR_STATE;
	return SYNC_OK;
}

sync_result_t enet_sync_server_poll(
	enet_sync_server_t *s, uint32_t max_events, enet_sync_progress_t *out)
{
	zero(out, sizeof(*out));
	if (!s || !max_events)
		return SYNC_ERR_ARGUMENT;
	sync_result_t r = manual(&s->base);
	if (r != SYNC_OK)
		return r;
	enet_sync_progress_t p = {0};
	s->base.busy = true;
	r = poll_server(s, max_events, &p);
	s->base.busy = false;
	if (out)
		*out = p;
	return r;
}

sync_result_t enet_sync_server_send(enet_sync_server_t *s,
	uint32_t max_frames_per_peer, enet_sync_progress_t *out)
{
	zero(out, sizeof(*out));
	if (!s || !max_frames_per_peer)
		return SYNC_ERR_ARGUMENT;
	sync_result_t r = manual(&s->base);
	if (r != SYNC_OK)
		return r;
	enet_sync_progress_t p = {0};
	s->base.busy = true;
	r = send_server(s, max_frames_per_peer, &p);
	s->base.busy = false;
	if (out)
		*out = p;
	return r;
}

sync_result_t enet_sync_server_service(enet_sync_server_t *s,
	uint32_t max_events, uint32_t max_frames_per_peer,
	enet_sync_progress_t *out)
{
	zero(out, sizeof(*out));
	if (!s || !max_events || !max_frames_per_peer)
		return SYNC_ERR_ARGUMENT;
	if (manual(&s->base) != SYNC_OK)
		return SYNC_ERR_STATE;
	enet_sync_progress_t poll, send;
	sync_result_t r = enet_sync_server_poll(s, max_events, &poll);
	if (out)
		*out = poll;
	if (r < 0)
		return r;
	sync_result_t t = enet_sync_server_send(s, max_frames_per_peer, &send);
	if (out) {
		out->events += send.events;
		out->frames += send.frames;
		out->bytes += send.bytes;
		out->connections += send.connections;
		out->disconnections += send.disconnections;
		out->faulted_connections += send.faulted_connections;
	}
	if (t < 0)
		return t;
	if (r == SYNC_PARTIAL || t == SYNC_PARTIAL)
		return SYNC_PARTIAL;
	if (t == SYNC_BACKPRESSURE)
		return t;
	return r == SYNC_OK || t == SYNC_OK ? SYNC_OK : SYNC_EMPTY;
}

sync_result_t enet_sync_client_service(
	enet_sync_client_t *c, uint32_t max_events, enet_sync_progress_t *out)
{
	zero(out, sizeof(*out));
	if (!c || !max_events)
		return SYNC_ERR_ARGUMENT;
	sync_result_t r = manual(&c->base);
	if (r != SYNC_OK)
		return r;
	if (c->base.status.state == ENET_SYNC_DISCONNECTED)
		return SYNC_END;
	enet_sync_progress_t p = {0};
	c->base.busy = true;
	r = service_client(c, max_events, &p);
	c->base.busy = false;
	if (out)
		*out = p;
	return r;
}

sync_result_t enet_sync_server_disconnect(
	enet_sync_server_t *s, ecs_entity_t peer, uint32_t data)
{
	if (!s || !peer)
		return SYNC_ERR_ARGUMENT;
	if (s->base.busy || s->base.status.state == ENET_SYNC_FAULTED)
		return SYNC_ERR_STATE;
	for (uint32_t i = 0; i < s->capacity; i++) {
		struct es_connection *c = &s->connections[i];
		if (c->peer == peer) {
			if (c->closing)
				return SYNC_OK;
			c->closing = true;
			c->data = data;
			ecs_entity_t request;
			sync_result_t r = result(flecs_enet_disconnect(
				s->base.info.world, s->base.host, c->transport,
				FLECS_ENET_DISCONNECT_NOW, data, 0, &request));
			if (r != SYNC_OK) {
				c->closing = false;
				return r;
			}
			return SYNC_OK;
		}
	}
	return SYNC_ERR_NOT_FOUND;
}

sync_result_t enet_sync_client_disconnect(enet_sync_client_t *c, uint32_t data)
{
	if (!c)
		return SYNC_ERR_ARGUMENT;
	if (c->base.busy || c->base.status.state == ENET_SYNC_FAULTED)
		return SYNC_ERR_STATE;
	if (c->closing || c->base.status.state == ENET_SYNC_DISCONNECTED)
		return SYNC_OK;
	c->closing = true;
	c->data = data;
	ecs_entity_t request;
	sync_result_t r;
	if (!c->transport)
		r = result(flecs_enet_close(
			c->base.info.world, c->base.host, 0, &request));
	else
		r = result(flecs_enet_disconnect(c->base.info.world,
			c->base.host, c->transport, FLECS_ENET_DISCONNECT_NOW,
			data, 0, &request));
	if (r != SYNC_OK) {
		c->closing = false;
		return r;
	}
	return SYNC_OK;
}

sync_result_t enet_sync_server_get_status(
	const enet_sync_server_t *s, enet_sync_status_t *out)
{
	zero(out, sizeof(*out));
	if (!s || !out)
		return SYNC_ERR_ARGUMENT;
	if (s->base.busy)
		return SYNC_ERR_STATE;
	*out = s->base.status;
	const flecs_enet_host_status_t *h = ecs_get_id(
		s->base.info.world, s->base.host, s->base.ids.host_status);
	if (h) {
		out->address = h->address;
		out->options.transport_limits = h->active.limits;
		out->retained_bytes = h->outgoing_bytes;
		out->retained_packets = h->outgoing_packets;
	}
	return SYNC_OK;
}

sync_result_t enet_sync_client_get_status(
	const enet_sync_client_t *c, enet_sync_status_t *out)
{
	zero(out, sizeof(*out));
	if (!c || !out)
		return SYNC_ERR_ARGUMENT;
	if (c->base.busy)
		return SYNC_ERR_STATE;
	*out = c->base.status;
	const flecs_enet_host_status_t *h = ecs_get_id(
		c->base.info.world, c->base.host, c->base.ids.host_status);
	if (h) {
		out->options.transport_limits = h->active.limits;
		out->retained_bytes = h->outgoing_bytes;
		out->retained_packets = h->outgoing_packets;
	}
	const flecs_enet_packet_t *packet = c->pending
		? ecs_get_id(c->base.info.world, c->pending, c->base.ids.packet)
		: NULL;
	out->pending_bytes = packet ? packet->size : 0;
	return SYNC_OK;
}

sync_result_t enet_sync_server_close(
	enet_sync_server_t *s, enet_sync_error_t *out_error)
{
	zero(out_error, sizeof(*out_error));
	if (!s)
		return SYNC_OK;
	struct es_base *b = &s->base;
	sync_result_t r = b->status.fault.detail.code;
	if (out_error)
		*out_error = b->status.fault;
	remove_systems(b);
	/* Host deletion cancels wrapper work and releases all transport
	 * children. */
	destroy_host(b);
	for (uint32_t i = 0; i < s->capacity; i++) {
		if (s->connections[i].peer)
			sync_peer_close(b->session, s->connections[i].peer);
	}
	free(s->connections);
	free(s);
	return r;
}

sync_result_t enet_sync_client_close(
	enet_sync_client_t *c, enet_sync_error_t *out_error)
{
	zero(out_error, sizeof(*out_error));
	if (!c)
		return SYNC_OK;
	sync_result_t r = c->base.status.fault.detail.code;
	if (out_error)
		*out_error = c->base.status.fault;
	remove_systems(&c->base);
	destroy_host(&c->base);
	free(c);
	return r;
}
