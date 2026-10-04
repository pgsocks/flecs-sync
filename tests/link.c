#include <sync.h>

/* Link every public symbol without executing unimplemented behavior. */
static sync_result_t (*volatile ref_sync_import)(
	ecs_world_t *world, sync_ids_t *out_ids) = sync_import;
static sync_result_t (*volatile ref_sync_server_create)(
	ecs_world_t *world,
	const sync_server_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error) = sync_server_create;
static sync_result_t (*volatile ref_sync_client_create)(
	ecs_world_t *world,
	const sync_client_desc_t *desc, sync_session_t **out_session,
	sync_error_t *out_error) = sync_client_create;
static void (*volatile ref_sync_session_destroy)(
	sync_session_t *session) = sync_session_destroy;
static sync_result_t (*volatile ref_sync_peer_open)(
	sync_session_t *session,
	ecs_entity_t *out_peer) = sync_peer_open;
static sync_result_t (*volatile ref_sync_peer_close)(
	sync_session_t *session, ecs_entity_t peer) = sync_peer_close;
static sync_result_t (*volatile ref_sync_set_rate)(
	sync_session_t *session, double send_hz) = sync_set_rate;
static sync_result_t (*volatile ref_sync_capture)(
	sync_session_t *session, uint64_t tick,
	sync_capture_result_t *out_result) = sync_capture;
static sync_result_t (*volatile ref_sync_outbox_peek)(
	const sync_session_t *session,
	ecs_entity_t peer, sync_frame_view_t *out_view) = sync_outbox_peek;
static sync_result_t (*volatile ref_sync_outbox_commit)(
	sync_session_t *session,
	ecs_entity_t peer, sync_outbox_token_t token) = sync_outbox_commit;
static sync_result_t (*volatile ref_sync_receive)(
	sync_session_t *session,
	const void *data, size_t size) = sync_receive;
static sync_result_t (*volatile ref_sync_apply)(
	sync_session_t *session, uint32_t max_updates,
	sync_apply_result_t *out_result) = sync_apply;
static sync_result_t (*volatile ref_sync_client_finish)(
	sync_session_t *session) = sync_client_finish;
static sync_result_t (*volatile ref_sync_client_reset)(
	sync_session_t *session) = sync_client_reset;
static sync_result_t (*volatile ref_sync_get_status)(
	const sync_session_t *session,
	ecs_entity_t peer, sync_status_t *out_status) = sync_get_status;
static sync_result_t (*volatile ref_sync_lookup)(
	const sync_session_t *session,
	uint64_t network_id, ecs_entity_t *out_entity) = sync_lookup;
static sync_result_t (*volatile ref_sync_frame_inspect)(
	const void *data, size_t size,
	sync_frame_info_t *out_info) = sync_frame_inspect;

static sync_result_t (*volatile ref_sync_get_session_info)(
	const sync_session_t *session,
	sync_session_info_t *out_info) = sync_get_session_info;

int main(void)
{
	if (!ref_sync_get_session_info)
		return 1;
	if (!ref_sync_import)
		return 1;
	if (!ref_sync_server_create)
		return 1;
	if (!ref_sync_client_create)
		return 1;
	if (!ref_sync_session_destroy)
		return 1;
	if (!ref_sync_peer_open)
		return 1;
	if (!ref_sync_peer_close)
		return 1;
	if (!ref_sync_set_rate)
		return 1;
	if (!ref_sync_capture)
		return 1;
	if (!ref_sync_outbox_peek)
		return 1;
	if (!ref_sync_outbox_commit)
		return 1;
	if (!ref_sync_receive)
		return 1;
	if (!ref_sync_apply)
		return 1;
	if (!ref_sync_client_finish)
		return 1;
	if (!ref_sync_client_reset)
		return 1;
	if (!ref_sync_get_status)
		return 1;
	if (!ref_sync_lookup)
		return 1;
	if (!ref_sync_frame_inspect)
		return 1;
	return 0;
}
