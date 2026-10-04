#include <enet_sync.h>
int main(void)
{
	enet_sync_status_t status;
	return enet_sync_server_close(NULL, NULL) != SYNC_OK ||
		enet_sync_client_close(NULL, NULL) != SYNC_OK ||
		enet_sync_server_get_status(NULL, &status) != SYNC_ERR_ARGUMENT ||
		enet_sync_client_service(NULL, 1, NULL) != SYNC_ERR_ARGUMENT;
}
