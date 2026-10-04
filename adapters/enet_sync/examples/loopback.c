/* Minimal explicit-scheduling example: no synchronized gameplay schema. */
#include <enet_sync.h>
#include <stdio.h>

int main(void)
{
	ecs_world_t *server_world = ecs_init(), *client_world = ecs_init();
	sync_ids_t server_ids, client_ids;
	sync_session_t *server = NULL, *client = NULL;
	enet_sync_server_t *listener = NULL;
	enet_sync_client_t *connection = NULL;
	int result = 1;
	if (sync_import(server_world, &server_ids) != SYNC_OK ||
		sync_import(client_world, &client_ids) != SYNC_OK)
		goto cleanup;
	/* Register matching reflection and ids.sync types here, before sessions. */
	if (sync_server_create(server_world, NULL, &server, NULL) != SYNC_OK ||
		sync_client_create(client_world, NULL, &client, NULL) != SYNC_OK)
		goto cleanup;
	enet_sync_server_desc_t listen = { .session = server };
	if (enet_sync_server_listen(&listen, &listener, NULL) != SYNC_OK)
		goto cleanup;
	ecs_progress(server_world, 0.001f);
	if (enet_sync_server_poll(listener, 64, NULL) < 0)
		goto cleanup;
	enet_sync_status_t status;
	enet_sync_server_get_status(listener, &status);
	enet_sync_client_desc_t connect = {
		.session = client, .address = status.address, .data = 1
	};
	connect.address.ip[0] = 127;
	connect.address.ip[3] = 1;
	if (enet_sync_client_connect(&connect, &connection, NULL) != SYNC_OK)
		goto cleanup;
	for (uint64_t tick = 1; tick <= 1000; tick++) {
		ecs_progress(server_world, 0.001f);
		ecs_progress(client_world, 0.001f);
		/* Poll before capture so on_event can assign each new peer's policy. */
		if (enet_sync_server_poll(listener, 64, NULL) < 0 ||
			sync_capture(server, tick, NULL) < 0 ||
			enet_sync_server_send(listener, 64, NULL) < 0 ||
			enet_sync_client_service(connection, 64, NULL) < 0 ||
			sync_apply(client, 1, NULL) < 0)
			goto cleanup;
		sync_status_t core;
		sync_get_status(client, 0, &core);
		if (core.state == SYNC_CLIENT_READY) {
			printf("Initial snapshot published at tick %llu\n",
				(unsigned long long)core.last_tick);
			result = 0;
			break;
		}
	}
cleanup:
	enet_sync_client_close(connection, NULL);
	enet_sync_server_close(listener, NULL);
	sync_session_destroy(client);
	sync_session_destroy(server);
	ecs_fini(client_world);
	ecs_fini(server_world);
	return result;
}
