#include "unimplemented.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdatomic.h>
#include <assert.h>
#include <float.h>
#include <limits.h>
#if defined(__linux__)
#include <sys/random.h>
#include <errno.h>
#endif

/* Session ownership is process local; world-local metadata IDs never escape
 * through this list. Calls are confined to the world's owning thread.
 */
static sync_session_t *sessions;
static atomic_flag registry_lock = ATOMIC_FLAG_INIT;

static void registry_enter(void)
{
	while (atomic_flag_test_and_set_explicit(&registry_lock,
		memory_order_acquire))
		;
}

static void registry_leave(void)
{
	atomic_flag_clear_explicit(&registry_lock, memory_order_release);
}

static bool writable(ecs_world_t *world)
{
	return world && ecs_get_world(world) == world &&
		!(ecs_world_get_flags(world) & EcsWorldReadonly) && !ecs_is_deferred(world);
}

static ecs_entity_t declaration(ecs_world_t *world, const char *name)
{
	return ecs_entity_init(world, &(ecs_entity_desc_t){ .name = name });
}

sync_result_t sync_import(ecs_world_t *world, sync_ids_t *out_ids)
{
	/*
	 * Validate writable real world; zero outputs; check namespace
	 * conflicts.
	 * Register compatible declarations and internal phase dependencies
	 * only.
	 * Return world-local IDs; leave no systems or session on failure.
	 */
	if (out_ids)
		memset(out_ids, 0, sizeof(*out_ids));
	if (!out_ids || !writable(world))
		return SYNC_ERR_ARGUMENT;
	ecs_entity_t existing = ecs_lookup(world, "sync.NetworkId");
	if (existing) {
		const EcsComponent *c = ecs_get(world, existing, EcsComponent);
		if (!c || c->size != sizeof(sync_network_id_t) ||
			c->alignment != _Alignof(sync_network_id_t))
			return SYNC_ERR_SCHEMA;
	}
	sync_ids_t ids = {0};
	ids.module = declaration(world, "sync");
	ecs_add_id(world, ids.module, EcsModule);
	ids.sync = declaration(world, "sync.Sync");
	ids.network_id = declaration(world, "sync.NetworkId");
	if (!existing) {
		ecs_component_desc_t desc = { .entity = ids.network_id };
		desc.type.size = sizeof(sync_network_id_t);
		desc.type.alignment = _Alignof(sync_network_id_t);
		ecs_component_init(world, &desc);
	}
	ids.replicate_to = declaration(world, "sync.ReplicateTo");
	ecs_add_pair(world, ids.replicate_to, EcsOnDeleteTarget, EcsRemove);
	ids.replicate_to_all = declaration(world, "sync.ReplicateToAll");
	ids.receive_phase = declaration(world, "sync.Receive");
	ids.apply_phase = declaration(world, "sync.Apply");
	ids.capture_phase = declaration(world, "sync.Capture");
	ids.send_phase = declaration(world, "sync.Send");
	ecs_entity_t phases[] = {ids.receive_phase, ids.apply_phase,
		ids.capture_phase, ids.send_phase};
	for (unsigned i = 0; i < 4; i++)
		ecs_add_id(world, phases[i], EcsPhase);
	ecs_add_pair(world, ids.apply_phase, EcsDependsOn, ids.receive_phase);
	ecs_add_pair(world, ids.send_phase, EcsDependsOn, ids.capture_phase);
	*out_ids = ids;
	return SYNC_OK;
}

static sync_result_t normalize(sync_limits_t *l, sync_role_t role)
{
#define DEFAULT(field, n) if (!l->field) l->field = n
	DEFAULT(max_types, 1024);
	DEFAULT(max_schema_bytes, 32768);
	DEFAULT(max_frame_bytes, 65536);
	DEFAULT(max_update_bytes, 16 * 1024 * 1024);
	DEFAULT(max_entities, 100000);
	DEFAULT(max_peers, 64);
	DEFAULT(max_string_bytes, 65535);
	DEFAULT(max_elements, 4096);
	DEFAULT(max_depth, 32);
	DEFAULT(max_fragments, 8192);
	DEFAULT(max_pending_bytes, 32 * 1024 * 1024);
	DEFAULT(max_receive_bytes, 32 * 1024 * 1024);
	DEFAULT(max_memory_bytes, 256 * 1024 * 1024);
#undef DEFAULT
	if (l->max_frame_bytes <= SYNC_HEADER + 8 ||
		l->max_schema_bytes > l->max_frame_bytes - SYNC_HEADER ||
		(role == SYNC_ROLE_SERVER &&
			l->max_pending_bytes < l->max_frame_bytes) ||
		(role == SYNC_ROLE_CLIENT &&
			l->max_receive_bytes < l->max_frame_bytes))
		return SYNC_ERR_ARGUMENT;
	return SYNC_OK;
}

static bool rate_valid(double rate)
{
	return isfinite(rate) && rate > 0 && isfinite(1 / rate) &&
		1 / rate > 0;
}

/* Check the existing graph before introducing automatic ordering edges. */
static bool depends(ecs_world_t *w, ecs_entity_t from, ecs_entity_t target,
	unsigned depth)
{
	if (from == target)
		return true;
	if (depth > 128)
		return true;
	for (int32_t i = 0; ; i++) {
		ecs_entity_t next = ecs_get_target(w, from, EcsDependsOn, i);
		if (!next)
			return false;
		if (depends(w, next, target, depth + 1))
			return true;
	}
}

static void scheduled(ecs_iter_t *it)
{
	sync_session_t *s = it->ctx;
	if (s->state == SYNC_FAULTED || s->state == SYNC_CLIENT_ENDED)
		return;
	if (s->role == SYNC_ROLE_CLIENT) {
		sync_apply_core(s, s->budget, NULL);
		return;
	}
	s->accumulator += it->delta_time;
	if (s->accumulator < s->interval)
		return;
	s->accumulator = fmod(s->accumulator, s->interval);
	if (s->auto_counter == UINT64_MAX) {
		sync_fault(s, SYNC_ERR_LIMIT);
		for (struct sync_peer *p = s->peers; p; p = p->next) {
			if (p->state == SYNC_FAULTED)
				continue;
			p->state = SYNC_FAULTED;
			sync_diagnostic(&p->fault, SYNC_ERR_LIMIT,
				p->entity, s->tick, p->sequence);
		}
		return;
	}
	sync_capture_core(s, ++s->auto_counter, NULL);
}

static sync_result_t create(ecs_world_t *world, sync_role_t role,
	sync_schedule_t schedule, sync_limits_t limits, double rate,
	uint32_t budget, sync_session_id_t identity,
	sync_session_t **out, sync_error_t *error)
{
	if (out)
		*out = NULL;
	if (error)
		memset(error, 0, sizeof(*error));
	sync_result_t result = SYNC_OK;
	if (!out || !writable(world)) {
		result = SYNC_ERR_ARGUMENT;
		goto fail;
	}
	registry_enter();
	for (sync_session_t *s = sessions; s; s = s->next) {
		if (s->world == world) {
			result = SYNC_ERR_STATE;
			break;
		}
	}
	registry_leave();
	if (result)
		goto fail;
	if (CHAR_BIT != 8 || sizeof(float) != 4 || sizeof(double) != 8 ||
		FLT_RADIX != 2 || FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53) {
		result = SYNC_ERR_UNSUPPORTED;
		goto fail;
	}
	if ((schedule != SYNC_MANUAL && schedule != SYNC_AUTOMATIC) ||
		(role == SYNC_ROLE_SERVER &&
			((schedule == SYNC_MANUAL && rate != 0) ||
			(schedule == SYNC_AUTOMATIC && rate != 0 &&
				!rate_valid(rate)))) ||
		(role == SYNC_ROLE_CLIENT && schedule == SYNC_MANUAL && budget)) {
		result = SYNC_ERR_ARGUMENT;
		goto fail;
	}
	result = normalize(&limits, role);
	if (result)
		goto fail;
	if (!ecs_lookup(world, "sync.Sync")) {
		result = SYNC_ERR_SCHEMA;
		goto fail;
	}
	if (sizeof(sync_session_t) > limits.max_memory_bytes) {
		result = SYNC_ERR_LIMIT;
		goto fail;
	}
	if (sync_test_fail(0)) {
		result = SYNC_ERR_NOMEM;
		goto fail;
	}
	sync_session_t *s = calloc(1, sizeof(*s));
	if (!s) {
		result = SYNC_ERR_NOMEM;
		goto fail;
	}
	s->world = world;
	s->role = role;
	s->schedule = schedule;
	s->limits = limits;
	s->memory = sizeof(*s);
	s->identity = identity;
	s->state = role == SYNC_ROLE_SERVER ? SYNC_SERVER_READY
		: SYNC_CLIENT_WAIT_MANIFEST;
	s->interval = 1 / (rate ? rate : 20);
	s->budget = budget ? budget : 1;
	result = sync_import(world, &s->ids);
	if (!result)
		result = sync_schema(s);
	if (!result && role == SYNC_ROLE_SERVER &&
		!memcmp(identity.bytes, (uint8_t[16]){0}, 16)) {
		/* Obtain OS entropy without opening a transport or a file. */
		#if defined(__linux__)
		size_t filled = 0;
		while (filled < 16) {
			ssize_t n = getrandom(s->identity.bytes + filled,
				16 - filled, 0);
			if (n < 0 && errno == EINTR)
				continue;
			if (n <= 0) {
				result = SYNC_ERR_UNSUPPORTED;
				break;
			}
			filled += (size_t)n;
		}
#else
		result = SYNC_ERR_UNSUPPORTED;
#endif
	}
	if (!result && schedule == SYNC_AUTOMATIC) {
		if (depends(world, EcsPostUpdate, s->ids.capture_phase, 0) ||
			depends(world, s->ids.apply_phase, EcsPreUpdate, 0)) {
			result = SYNC_ERR_STATE;
		} else {
			ecs_add_pair(world, s->ids.capture_phase, EcsDependsOn,
				EcsPostUpdate);
			ecs_add_pair(world, s->ids.receive_phase, EcsDependsOn,
				EcsOnLoad);
			ecs_add_pair(world, EcsPreUpdate, EcsDependsOn,
				s->ids.apply_phase);
			s->system = ecs_new(world);
			ecs_entity_t phase = role == SYNC_ROLE_SERVER
				? s->ids.capture_phase : s->ids.apply_phase;
			ecs_add_pair(world, s->system, EcsDependsOn, phase);
			if (!ecs_system_init(world, &(ecs_system_desc_t){
				.entity = s->system, .callback = scheduled,
				.ctx = s, .immediate = true }))
				result = SYNC_ERR_RUNTIME;
		}
	}
	if (result) {
		sync_session_destroy(s);
		goto fail;
	}
	registry_enter();
	s->next = sessions;
	sessions = s;
	registry_leave();
	*out = s;
	return SYNC_OK;
fail:
	if (error)
		sync_diagnostic(error, result, 0, 0, 0);
	return result;
}

sync_result_t sync_server_create(ecs_world_t *world,
	const sync_server_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error)
{
	/*
	 * Zero outputs; validate world, sole session and copied options.
	 * Normalize limits; freeze and validate marked reflection and hooks.
	 * Allocate schema/tracking; obtain identity; optionally install
	 * scheduling.
	 * Activate only on success; unwind owned resources and systems on
	 * failure.
	 */
	sync_server_desc_t d = desc ? *desc : (sync_server_desc_t){0};
	return create(world, SYNC_ROLE_SERVER, d.schedule, d.limits,
		d.send_hz, 0, d.session_id, out_session, out_error);
}

sync_result_t sync_client_create(ecs_world_t *world,
	const sync_client_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error)
{
	/*
	 * Zero outputs; validate options, world and sole-session ownership.
	 * Freeze local schema; allocate bounded input, assembly and mapping
	 * state.
	 * Install optional apply wrapper; activate in WAIT_MANIFEST on success.
	 * Unwind failure without adopting gameplay entities.
	 */
	sync_client_desc_t d = desc ? *desc : (sync_client_desc_t){0};
	return create(world, SYNC_ROLE_CLIENT, d.schedule, d.limits,
		0, d.updates_per_progress, (sync_session_id_t){0},
		out_session, out_error);
}

void sync_session_destroy(sync_session_t *session)
{
	/*
	 * Accept NULL; otherwise require stopped adapters and writable
	 * boundary.
	 * Close server peers/remove assigned IDs, or delete owned client
	 * replicas.
	 * Remove private systems/tracking and free queues/storage and handle.
	 * Preserve imported declarations and unrelated gameplay entities.
	 */
	if (!session)
		return;
	sync_session_t *s = session;
	while (s->peers)
		sync_peer_close(s, s->peers->entity);
	if (s->system)
		ecs_delete(s->world, s->system);
	sync_maps_fini(s, s->role == SYNC_ROLE_CLIENT);
	sync_queue_fini(s, &s->head, &s->tail);
	sync_buffer_fini(s, &s->assembly);
	sync_buffer_fini(s, &s->manifest);
	sync_free(s, s->wire_types);
	for (uint32_t i = 0; i < s->type_count; i++) {
		if (s->types[i].query)
			ecs_query_fini(s->types[i].query);
		sync_buffer_fini(s, &s->types[i].shape);
		sync_free(s, s->types[i].name);
	}
	sync_free(s, s->types);
	registry_enter();
	for (sync_session_t **p = &sessions; *p; p = &(*p)->next) {
		if (*p == s) {
			*p = s->next;
			break;
		}
	}
	registry_leave();
	assert(s->memory == sizeof(*s));
	free(s);
}

sync_result_t sync_get_status(const sync_session_t *session,
	ecs_entity_t peer, sync_status_t *out_status)
{
	/*
	 * Zero output; validate session and optional peer ownership/role.
	 * Copy status/diagnostics without consuming progress or errors.
	 * Aggregate server queues/faults; expose no borrowed internal pointers.
	 */
	if (out_status)
		memset(out_status, 0, sizeof(*out_status));
	if (!session || !out_status)
		return SYNC_ERR_ARGUMENT;
	const sync_session_t *s = session;
	if (peer && s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	struct sync_peer *p = peer ? sync_find_peer(s, peer) : NULL;
	if (peer && !p)
		return SYNC_ERR_NOT_FOUND;
	out_status->role = s->role;
	out_status->state = p ? p->state : s->state;
	out_status->session_id = s->identity;
	out_status->stream_id = p ? p->stream : s->stream;
	out_status->last_tick = p ? p->tick : s->tick;
	out_status->last_sequence = p ? p->sequence : s->sequence;
	out_status->memory_bytes = s->memory;
	out_status->entities = p ? p->count : s->map_count;
	out_status->fault = p ? p->fault : s->fault;
	out_status->queued_bytes = p ? p->queued : s->queued;
	out_status->assembly_bytes = s->assembly.size;
	if (!peer && s->role == SYNC_ROLE_SERVER) {
		out_status->peers = s->peer_count;
		for (p = s->peers; p; p = p->next) {
			out_status->queued_bytes += p->queued;
			out_status->faulted_peers += p->state == SYNC_FAULTED;
		}
	}
	return SYNC_OK;
}

sync_result_t sync_lookup(const sync_session_t *session,
	uint64_t network_id, ecs_entity_t *out_entity)
{
	/*
	 * Zero output; reject zero identity; inspect current mapping.
	 * Return hidden server entities or published client replicas; never
	 * allocate.
	 * NOT_FOUND for retired mappings; also permit faulted/ended sessions.
	 */
	if (out_entity)
		*out_entity = 0;
	if (!session || !out_entity || !network_id)
		return SYNC_ERR_ARGUMENT;
	struct sync_mapping *m = sync_find_map(session, network_id);
	if (!m)
		return SYNC_ERR_NOT_FOUND;
	*out_entity = m->entity;
	return SYNC_OK;
}

sync_result_t sync_set_rate(sync_session_t *s, double send_hz)
{
	/*
	 * Validate automatic server, finite positive rate and reciprocal.
	 * Copy rate/reset accumulator without capture; preserve on failure.
	 */
	if (!s)
		return SYNC_ERR_ARGUMENT;
	if (s->role != SYNC_ROLE_SERVER)
		return SYNC_ERR_ROLE;
	if (s->state == SYNC_FAULTED || s->schedule != SYNC_AUTOMATIC)
		return SYNC_ERR_STATE;
	if (!rate_valid(send_hz))
		return SYNC_ERR_ARGUMENT;
	s->interval = 1 / send_hz;
	s->accumulator = 0;
	return SYNC_OK;
}

sync_result_t sync_get_session_info(const sync_session_t *session,
	sync_session_info_t *out_info)
{
	/* Zero output, validate pointers and copy immutable fields. */
	if (out_info)
		memset(out_info, 0, sizeof(*out_info));
	if (!session || !out_info)
		return SYNC_ERR_ARGUMENT;
	*out_info = (sync_session_info_t){
		.world = session->world, .ids = session->ids,
		.role = session->role, .schedule = session->schedule,
		.limits = session->limits
	};
	return SYNC_OK;
}
