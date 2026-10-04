# Integrate Sync

[README](../README.md) · [API](api.md) · [Testing](testing.md)

This guide takes a C application from reflected component registration to a
replica in a separate world. It assumes the [build prerequisites](../README.md)
and ordinary Flecs component knowledge. The example uses manual sessions and
in-memory delivery, so it requires no sockets, files, sleeps, or adapters.

## Replicate a component between worlds

Save the complete C block below as `sync_example.c` outside the source tree.
Compile it as C11 with `include/` and the Flecs headers on the include path;
link against Sync and the same Flecs build used by Sync. The
[testing guide](testing.md#check-documentation-examples) also shows how to
check its syntax independently.

`register_position` uses world-local component IDs and explicitly registers
native member offsets with [Flecs Meta][meta]. Each world registers its own
type before session creation. A network ID, rather than the server's native
entity ID, is used to find the resulting client replica.

`transfer` is the single delivery consumer. Backpressure causes it to apply
queued input and retry the same outbox head. It commits only after receive
has copied the complete frame. Errors stop the example and lead to cleanup;
an accepted frame must not be delivered again if its subsequent commit fails.

```c
#include <sync.h>
#include <stdio.h>

struct position {
	float x;
	float y;
};

static ecs_entity_t register_position(ecs_world_t *world,
	const sync_ids_t *ids)
{
	ecs_entity_t entity = ecs_entity_init(world, &(ecs_entity_desc_t){
		.name = "Position"
	});
	if (!entity)
		return 0;

	ecs_entity_t type = ecs_component_init(world,
		&(ecs_component_desc_t){
			.entity = entity,
			.type.size = sizeof(struct position),
			.type.alignment = _Alignof(struct position)
		});
	if (!type)
		return 0;

	ecs_entity_t reflected = ecs_struct_init(world,
		&(ecs_struct_desc_t){
			.entity = type,
			.members = {
				{
					.name = "x", .type = ecs_id(ecs_f32_t),
					.offset = offsetof(struct position, x),
					.use_offset = true
				},
				{
					.name = "y", .type = ecs_id(ecs_f32_t),
					.offset = offsetof(struct position, y),
					.use_offset = true
				}
			}
		});
	if (!reflected)
		return 0;

	ecs_add_id(world, type, ids->sync);
	return type;
}

static sync_result_t transfer(sync_session_t *server, ecs_entity_t peer,
	sync_session_t *client)
{
	for (;;) {
		sync_frame_view_t frame;
		sync_result_t result = sync_outbox_peek(server, peer, &frame);

		if (result == SYNC_EMPTY)
			break;
		if (result != SYNC_OK)
			return result;

		result = sync_receive(client, frame.data, frame.size);
		if (result == SYNC_BACKPRESSURE) {
			sync_apply_result_t progress;

			result = sync_apply(client, UINT32_MAX, &progress);
			if (result < 0)
				return result;
			if (!progress.frames_consumed)
				return SYNC_BACKPRESSURE;
			continue;
		}
		if (result != SYNC_OK)
			return result;

		result = sync_outbox_commit(server, peer, frame.token);
		if (result != SYNC_OK)
			return result;
	}

	for (;;) {
		sync_result_t result = sync_apply(client, UINT32_MAX, NULL);

		if (result == SYNC_EMPTY || result == SYNC_NEED_MORE)
			return SYNC_OK;
		if (result != SYNC_OK)
			return result;
	}
}

static int check_result(sync_result_t result, const char *operation)
{
	if (result == SYNC_OK)
		return 1;
	fprintf(stderr, "%s returned %d\n", operation, (int)result);
	return 0;
}

int main(void)
{
	ecs_world_t *server_world = ecs_init();
	ecs_world_t *client_world = ecs_init();
	sync_session_t *server = NULL;
	sync_session_t *client = NULL;
	int exit_code = 1;

	if (!server_world || !client_world)
		goto out;

	sync_ids_t server_ids;
	sync_ids_t client_ids;

	if (!check_result(sync_import(server_world, &server_ids), "import"))
		goto out;
	if (!check_result(sync_import(client_world, &client_ids), "import"))
		goto out;

	ecs_entity_t server_type = register_position(server_world, &server_ids);
	ecs_entity_t client_type = register_position(client_world, &client_ids);

	if (!server_type || !client_type) {
		fprintf(stderr, "Position registration failed\n");
		goto out;
	}

	sync_error_t error;

	if (!check_result(sync_server_create(server_world, NULL, &server,
		&error), "server create")) {
		fprintf(stderr, "%s\n", error.message);
		goto out;
	}
	if (!check_result(sync_client_create(client_world, NULL, &client,
		&error), "client create")) {
		fprintf(stderr, "%s\n", error.message);
		goto out;
	}

	ecs_entity_t peer;

	if (!check_result(sync_peer_open(server, &peer), "peer open"))
		goto out;

	ecs_entity_t entity = ecs_new(server_world);

	ecs_add_id(server_world, entity,
		ecs_pair(server_ids.replicate_to, peer));
	for (uint64_t tick = 1; tick <= 2; tick++) {
		struct position value = { .x = (float)tick * 10, .y = 5 };

		ecs_set_id(server_world, entity, server_type, sizeof(value),
			&value);
		sync_result_t result = sync_capture(server, tick, NULL);

		if (!check_result(result, "capture")) {
			sync_status_t status;

			if (sync_get_status(server, peer, &status) == SYNC_OK)
				fprintf(stderr, "%s\n", status.fault.message);
			goto out;
		}
		if (!check_result(transfer(server, peer, client), "transfer"))
			goto out;

		const sync_network_id_t *identity = ecs_get_id(server_world,
			entity, server_ids.network_id);
		if (!identity)
			goto out;

		ecs_entity_t replica;

		result = sync_lookup(client, identity->value, &replica);
		if (!check_result(result, "lookup"))
			goto out;

		const struct position *copy = ecs_get_id(client_world, replica,
			client_type);
		if (!copy || copy->x != value.x || copy->y != value.y) {
			fprintf(stderr, "Replica Position differs\n");
			goto out;
		}
	}

	if (!check_result(sync_client_finish(client), "finish"))
		goto out;
	puts("Snapshot and delta reproduced Position");
	exit_code = 0;
out:
	/* No adapter remains active; destruction also closes the peer. */
	sync_session_destroy(client);
	sync_session_destroy(server);
	if (client_world)
		ecs_fini(client_world);
	if (server_world)
		ecs_fini(server_world);
	return exit_code;
}
```

The expected result is exit status zero and the message
`Snapshot and delta reproduced Position`. Tick one supplies the initial
snapshot; tick two changes the value through a tracked Flecs operation.
Finishing the client preserves its final replicas until session destruction.

The pump returning success means available frames were transferred and
processed, not that an incomplete logical update is complete. Lookup and EOF
validation supply the example's end-state checks. If backpressure cannot make
progress, the helper returns instead of spinning; a larger application can
retain the stream and resume after investigating its queue configuration.

## Select recipients and report writes

The example's direct pair restricts the entity to one peer. For a public
entity, add `server_ids.replicate_to_all` with `ecs_add_id`. Additional direct
pairs can select more peers. Removing one pair does not hide an entity while
the all-recipients tag still applies.

Change policy and values before capture, after finishing iterators and merging
deferred commands. Use `ecs_set_id` as above or call `ecs_modified_id` after
writing through a component pointer. Sync cannot detect unreported writes.

Keep referenced entities visible to every recipient of the referring entity.
If a target becomes hidden, clear or replace the reference before the same
capture. The core faults invalid recipient views rather than exposing hidden
targets. Inspect peer status when capture returns `SYNC_PARTIAL`; healthy
recipients can still have queued updates.

## Opt into automatic scheduling

For an ordinary game loop, set `schedule` to `SYNC_AUTOMATIC` in each creation
descriptor. A zero `send_hz` selects 20 Hz; a zero `updates_per_progress`
selects one client update per progression. Set explicit positive values to
change those defaults. This choice replaces manual capture/apply driving;
the example's manual `transfer` helper must not drive an automatic client.

Run adapter receive work in `ids.receive_phase` and output work in
`ids.send_phase`. Let the core apply and capture systems run at their writable
boundaries. Complete server writers by `EcsPostUpdate` and run client readers
after apply. Custom pipelines need the [documented phase order](api.md).
Automatic scheduling does not open a transport or deliver frames by itself.

## Handle disconnect and shutdown

Stop the old incoming adapter, discard its retained input, and reset the
client at a writable boundary. Reset deletes replicas and their local-only
data, while keeping schema and scheduling. On the server, close the old peer
and open a fresh one, then restore direct recipient policy for the new peer.
Its next capture starts a new stream with a complete snapshot.

For orderly shutdown, stop adapters and release their borrowed state first.
Destroy both sessions before finalizing their worlds. Do not directly delete
peer entities or rely on cascading relationships from unrelated local data
to replicas. See the [ownership rules](api.md#calling-and-ownership-rules)
before adding hooks or local-only data to replicated entities.

[meta]: https://www.flecs.dev/flecs/group__c__addons__meta.html
