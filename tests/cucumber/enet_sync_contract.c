/* Private procedural script; derived from native contracts. */
#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

static void invalid_calls(void)
{
	cucumber_wait(1, 726577496u);
	assert(enet_sync_server_listen(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(2, 2850421148u);
	assert(enet_sync_client_connect(NULL, NULL, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(3, 194705399u);
	assert(enet_sync_server_poll(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(4, 2309272271u);
	assert(enet_sync_server_send(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(5, 1175805425u);
	assert(enet_sync_server_service(NULL, 1, 1, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(6, 2942189242u);
	assert(enet_sync_client_service(NULL, 1, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 1622223849u);
	assert(enet_sync_server_disconnect(NULL, 1, 0) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(8, 1758530765u);
	assert(enet_sync_client_disconnect(NULL, 0) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 1081472500u);
	assert(enet_sync_server_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(10, 3401506301u);
	assert(enet_sync_client_get_status(NULL, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();
}

static void initialized_outputs(void)
{
	cucumber_wait(1, 1273772002u);
	enet_sync_status_t status, zero = {0};
	cucumber_reply();

	cucumber_wait(2, 3203340565u);
	memset(&status, 0xff, sizeof(status));
	cucumber_reply();

	cucumber_wait(3, 311667823u);
	assert(enet_sync_server_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(4, 3809620498u);
	assert(!memcmp(&status, &zero, sizeof(status)));
	cucumber_reply();

	cucumber_wait(5, 3203340565u);
	memset(&status, 0xff, sizeof(status));
	cucumber_reply();

	cucumber_wait(6, 1286152952u);
	assert(enet_sync_client_get_status(NULL, &status) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(7, 3809620498u);
	assert(!memcmp(&status, &zero, sizeof(status)));
	cucumber_reply();

	cucumber_wait(8, 791835075u);
	enet_sync_progress_t p, empty = {0};
	cucumber_reply();

	cucumber_wait(9, 3145053677u);
	memset(&p, 0xff, sizeof(p));
	cucumber_reply();

	cucumber_wait(10, 819273760u);
	assert(enet_sync_server_send(NULL, 0, &p) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(11, 2675875936u);
	assert(!memcmp(&p, &empty, sizeof(p)));
	cucumber_reply();

	cucumber_wait(12, 1639374150u);
	enet_sync_server_t *server = (void *)1;
	cucumber_reply();

	cucumber_wait(13, 146501751u);
	enet_sync_error_t e, success = {0};
	cucumber_reply();

	cucumber_wait(14, 402549114u);
	memset(&e, 0xff, sizeof(e));
	cucumber_reply();

	cucumber_wait(15, 3495458422u);
	assert(enet_sync_server_listen(NULL, &server, &e) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(16, 1147289483u);
	assert(!server && !memcmp(&e, &success, sizeof(e)));
	cucumber_reply();
}

static void null_cleanup(void)
{
	cucumber_wait(1, 292434861u);
	assert(enet_sync_server_close(NULL, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(2, 2925731062u);
	assert(enet_sync_client_close(NULL, NULL) == SYNC_OK);
	cucumber_reply();
}

static void options(void)
{
	cucumber_wait(1, 1131742053u);
	struct world server, client;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 1721062718u);
	world_open(&client, false, false, 0, 0);
	cucumber_reply();

	cucumber_wait(4, 2466451462u);
	enet_sync_server_t *s;
	cucumber_reply();

	cucumber_wait(5, 3660446953u);
	enet_sync_client_t *c;
	cucumber_reply();

	cucumber_wait(6, 3062199521u);
	enet_sync_server_desc_t d = {.session = server.session};
	cucumber_reply();

	cucumber_wait(7, 821243795u);
	d.options.schedule = (sync_schedule_t)99;
	cucumber_reply();

	cucumber_wait(8, 582973595u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(9, 69439286u);
	d.options.schedule = SYNC_MANUAL;
	cucumber_reply();

	cucumber_wait(10, 936169348u);
	d.options.events_per_progress = 1;
	cucumber_reply();

	cucumber_wait(11, 582973595u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(12, 785907397u);
	d.options.events_per_progress = 0;
	cucumber_reply();

	cucumber_wait(13, 2314309444u);
	d.options.frames_per_peer = 1;
	cucumber_reply();

	cucumber_wait(14, 582973595u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(15, 2431303173u);
	d.options.frames_per_peer = 0;
	cucumber_reply();

	cucumber_wait(16, 1235891749u);
	d.options.transport_limits.max_waiting_bytes = 1;
	cucumber_reply();

	cucumber_wait(17, 4175274420u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(18, 1353779044u);
	d.options.transport_limits.max_waiting_bytes = 0;
	cucumber_reply();

	cucumber_wait(19, 2628867596u);
	d.session = client.session;
	cucumber_reply();

	cucumber_wait(20, 3732394873u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(21, 1894967157u);
	d.session = server.session;
	cucumber_reply();

	cucumber_wait(22, 3985047311u);
	d.max_clients = UINT32_MAX;
	cucumber_reply();

	cucumber_wait(23, 582973595u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(24, 297769168u);
	d.max_clients = 2;
	cucumber_reply();

	cucumber_wait(25, 1394033897u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(26, 3391436121u);
	memset(&d, 0xff, sizeof(d));
	cucumber_reply();

	cucumber_wait(27, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(28, 479560238u);
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(29, 3084640159u);
	assert(status.state == ENET_SYNC_OPENING && status.host);
	cucumber_reply();

	cucumber_wait(30, 2851874964u);
	#ifdef ENET_SYNC_TESTING
	assert(!es_hosts_live);
	cucumber_reply();

	cucumber_wait(31, 3231567973u);
	#endif
	ecs_progress(server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(32, 2268719381u);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(33, 479560238u);
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(34, 401527643u);
	assert(status.state == ENET_SYNC_ACTIVE && status.address.port);
	cucumber_reply();

	cucumber_wait(35, 988748962u);
	assert(status.schedule == SYNC_MANUAL && !status.connections);
	cucumber_reply();

	cucumber_wait(36, 3280020524u);
	assert(status.options.transport_limits.max_outgoing_bytes ==
		32u * 1024 * 1024);
	cucumber_reply();

	cucumber_wait(37, 2360678708u);
	assert(status.options.transport_limits.max_outgoing_packets == 8192);
	cucumber_reply();

	cucumber_wait(38, 1841341308u);
	assert(status.options.transport_limits.max_waiting_bytes ==
		32u * 1024 * 1024);
	cucumber_reply();

	cucumber_wait(39, 2329420449u);
	assert(status.max_frame_bytes == 65536);
	cucumber_reply();

	cucumber_wait(40, 435096765u);
	assert(enet_sync_server_service(s, 1, 0, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(41, 397457489u);
	assert(enet_sync_server_disconnect(s, 1, 0) == SYNC_ERR_NOT_FOUND);
	cucumber_reply();

	cucumber_wait(42, 3311045822u);
	enet_sync_client_desc_t cd = {.session = client.session};
	cucumber_reply();

	cucumber_wait(43, 3698320230u);
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ARGUMENT);
	cucumber_reply();

	cucumber_wait(44, 4267738701u);
	cd.address = status.address;
	cucumber_reply();

	cucumber_wait(45, 1410176404u);
	cd.address.ip[0] = 127;
	cucumber_reply();

	cucumber_wait(46, 966718982u);
	cd.address.ip[3] = 1;
	cucumber_reply();

	cucumber_wait(47, 3018203624u);
	cd.session = server.session;
	cucumber_reply();

	cucumber_wait(48, 570093566u);
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_ROLE);
	cucumber_reply();

	cucumber_wait(49, 1604685969u);
	cd.session = client.session;
	cucumber_reply();

	cucumber_wait(50, 2128820094u);
	cd.options.schedule = SYNC_AUTOMATIC;
	cucumber_reply();

	cucumber_wait(51, 3745999981u);
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(52, 2401009409u);
	cd.options.schedule = SYNC_MANUAL;
	cucumber_reply();

	cucumber_wait(53, 2727475719u);
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(54, 2965773521u);
	memset(&cd, 0xff, sizeof(cd));
	cucumber_reply();

	cucumber_wait(55, 3219758469u);
	assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(56, 183101191u);
	assert(status.state == ENET_SYNC_CONNECTING);
	cucumber_reply();

	cucumber_wait(57, 3307654665u);
	assert(enet_sync_client_close(c, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(58, 3604076941u);
	assert(enet_sync_server_close(s, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(59, 4033515195u);
	world_close(&client);
	cucumber_reply();

	cucumber_wait(60, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

static void connections(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 1645657544u);
	assert(g.connected == 3 && !g.recorded);
	cucumber_reply();

	cucumber_wait(4, 3490566933u);
	sync_status_t core;
	cucumber_reply();

	cucumber_wait(5, 2700706759u);
	assert(sync_get_status(g.server.session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 2775189848u);
	assert(core.peers == 3 && !core.last_tick && !core.queued_bytes);
	cucumber_reply();

	cucumber_wait(7, 3137295437u);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(8, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void snapshot_delta(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 2569141450u);
	game_settle(&g, 3, 1, false);
	cucumber_reply();

	cucumber_wait(5, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(6, 2835001373u);
	struct health h = {42};
	cucumber_reply();

	cucumber_wait(7, 1625275989u);
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	cucumber_reply();

	cucumber_wait(8, 4191767704u);
	game_capture(&g, 2);
	cucumber_reply();

	cucumber_wait(9, 1914062793u);
	game_settle(&g, 3, 2, false);
	cucumber_reply();

	cucumber_wait(10, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(11, 3573599077u);
	assert(g.recorded > 12);
	cucumber_reply();

	cucumber_wait(12, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void fragmented_updates(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2460615309u);
	game_open(&g, false, 1024, 1024, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 1763331948u);
	ecs_entity_t entities[300];
	cucumber_reply();

	cucumber_wait(4, 2479718072u);
	for (unsigned i = 0; i < 300; i++) {
		entities[i] = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
			sizeof(h), &h);
		ecs_add_id(g.server.ecs, entities[i],
			g.server.ids.replicate_to_all);
	}
	cucumber_reply();

	cucumber_wait(5, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(6, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(7, 1011718703u);
	for (unsigned i = 0; i < 300; i++) {
		struct health h = {1000 + (int32_t)i};
		ecs_set_id(g.server.ecs, entities[i], g.server.health,
			sizeof(h), &h);
	}
	cucumber_reply();

	cucumber_wait(8, 4191767704u);
	game_capture(&g, 2);
	cucumber_reply();

	cucumber_wait(9, 306901634u);
	game_settle(&g, 1, 2, false);
	cucumber_reply();

	cucumber_wait(10, 1287759636u);
	assert(g.fragments > 1);
	cucumber_reply();

	cucumber_wait(11, 1060630969u);
	for (unsigned i = 0; i < 300; i++) {
		ecs_entity_t local = replica(&g, 0, entities[i]);
		const struct health *h = ecs_get_id(
			g.clients[0].ecs, local, g.clients[0].health);
		assert(h && h->value == 1000 + (int32_t)i);
	}
	cucumber_reply();

	cucumber_wait(12, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void packet_fragmentation(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 2622687770u);
	for (unsigned i = 0; i < 500; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(6, 4213402519u);
	assert(g.max_packet > 1400);
	cucumber_reply();

	cucumber_wait(7, 3412547943u);
	sync_status_t s;
	cucumber_reply();

	cucumber_wait(8, 2438241161u);
	assert(sync_get_status(g.clients[0].session, 0, &s) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 976088230u);
	assert(s.entities == 503);
	cucumber_reply();

	cucumber_wait(10, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void rpg_trade_visibility(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 2569141450u);
	game_settle(&g, 3, 1, false);
	cucumber_reply();

	cucumber_wait(5, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(6, 3111796985u);
	/* Aria trades her stack to Bram; GM retains access to both views. */
	struct item item = {g.actors[1], 7};
	cucumber_reply();

	cucumber_wait(7, 2916057283u);
	ecs_set_id(
		g.server.ecs, g.items[0], g.server.item, sizeof(item), &item);
	cucumber_reply();

	cucumber_wait(8, 1536748131u);
	g.mask[0] = 2 | 4;
	cucumber_reply();

	cucumber_wait(9, 13720512u);
	game_policies(&g);
	cucumber_reply();

	cucumber_wait(10, 4191767704u);
	game_capture(&g, 2);
	cucumber_reply();

	cucumber_wait(11, 1914062793u);
	game_settle(&g, 3, 2, false);
	cucumber_reply();

	cucumber_wait(12, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(13, 3495487347u);
	/* Bram reveals this item to Aria; owner is already public. */
	g.mask[0] = 1 | 2 | 4;
	cucumber_reply();

	cucumber_wait(14, 13720512u);
	game_policies(&g);
	cucumber_reply();

	cucumber_wait(15, 4162528943u);
	game_capture(&g, 3);
	cucumber_reply();

	cucumber_wait(16, 2647928055u);
	game_settle(&g, 3, 3, false);
	cucumber_reply();

	cucumber_wait(17, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(18, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void reconnect(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 2569141450u);
	game_settle(&g, 3, 1, false);
	cucumber_reply();

	cucumber_wait(5, 3901996999u);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	cucumber_reply();

	cucumber_wait(6, 2099571513u);
	assert(enet_sync_client_disconnect(g.connections[0], 17) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(8, 3371596223u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
	cucumber_reply();

	cucumber_wait(9, 4022773515u);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(10, 3490566933u);
	sync_status_t core;
	cucumber_reply();

	cucumber_wait(11, 2679528859u);
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 3077016113u);
	assert(core.state == SYNC_CLIENT_READY);
	cucumber_reply();

	cucumber_wait(13, 1100061275u);
	for (unsigned i = 0; i < 5000 && g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
	}
	cucumber_reply();

	cucumber_wait(14, 1454302474u);
	assert(!g.peers[0]);
	cucumber_reply();

	cucumber_wait(15, 3988417082u);
	assert(enet_sync_client_close(g.connections[0], NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 1773162592u);
	enet_sync_status_t listener;
	cucumber_reply();

	cucumber_wait(17, 3215688099u);
	assert(enet_sync_server_get_status(g.listener, &listener) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(18, 4138425408u);
	enet_sync_client_desc_t d = {
		.session = g.clients[0].session, .address = listener.address};
	cucumber_reply();

	cucumber_wait(19, 4283313317u);
	d.address.ip[0] = 127;
	cucumber_reply();

	cucumber_wait(20, 1870152394u);
	d.address.ip[3] = 1;
	cucumber_reply();

	cucumber_wait(21, 2423566266u);
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(22, 2319992072u);
	assert(sync_client_reset(g.clients[0].session) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(23, 2687040780u);
	assert(!ecs_is_alive(g.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(24, 1143544494u);
	game_connect(&g, 0);
	cucumber_reply();

	cucumber_wait(25, 1695110725u);
	for (unsigned i = 0; i < 5000 && !g.peers[0]; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		ecs_progress(g.clients[0].ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
		assert(enet_sync_client_service(g.connections[0], 64, NULL) >=
			0);
	}
	cucumber_reply();

	cucumber_wait(26, 2915839356u);
	assert(g.peers[0]);
	cucumber_reply();

	cucumber_wait(27, 2354284022u);
	struct health h = {23};
	cucumber_reply();

	cucumber_wait(28, 1625275989u);
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	cucumber_reply();

	cucumber_wait(29, 4191767704u);
	game_capture(&g, 2);
	cucumber_reply();

	cucumber_wait(30, 1914062793u);
	game_settle(&g, 3, 2, false);
	cucumber_reply();

	cucumber_wait(31, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(32, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void automatic_phases(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 3550638348u);
	game_open(&g, true, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 2776438875u);
	enet_sync_status_t s;
	cucumber_reply();

	cucumber_wait(4, 2041254639u);
	assert(enet_sync_server_get_status(g.listener, &s) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 3695224581u);
	assert(s.options.events_per_progress == 64);
	cucumber_reply();

	cucumber_wait(6, 4226557950u);
	assert(s.options.frames_per_peer == 64);
	cucumber_reply();

	cucumber_wait(7, 1072470698u);
	assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(8, 1509656811u);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(9, 4245781265u);
	assert(enet_sync_server_service(g.listener, 1, 1, NULL) ==
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(10, 775800849u);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(11, 3562469450u);
	game_settle(&g, 3, 1, true);
	cucumber_reply();

	cucumber_wait(12, 436135525u);
	game_check(&g, 3);
	cucumber_reply();

	cucumber_wait(13, 3338579822u);
	assert(g.recorded >= 12);
	cucumber_reply();

	cucumber_wait(14, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

#ifdef ES_FILE_SYNC
static void recording_replay(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 278324363u);
	const char *paths[] = {"aria.sync", "bram.sync", "gm.sync"};
	cucumber_reply();

	cucumber_wait(4, 1143154556u);
	for (unsigned i = 0; i < 3; i++)
		assert(file_sync_writer_open(
			       paths[i], NULL, &g.writers[i], NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(6, 2569141450u);
	game_settle(&g, 3, 1, false);
	cucumber_reply();

	cucumber_wait(7, 1933670075u);
	struct health h = {8};
	cucumber_reply();

	cucumber_wait(8, 1625275989u);
	ecs_set_id(g.server.ecs, g.actors[0], g.server.health, sizeof(h), &h);
	cucumber_reply();

	cucumber_wait(9, 4191767704u);
	game_capture(&g, 2);
	cucumber_reply();

	cucumber_wait(10, 1914062793u);
	game_settle(&g, 3, 2, false);
	cucumber_reply();

	cucumber_wait(11, 1211651830u);
	assert(g.recording_error == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 2546613084u);
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
	cucumber_reply();

	cucumber_wait(13, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void recording_failure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 874142835u);
	file_sync_writer_desc_t d = {.max_frame_bytes = 1};
	cucumber_reply();

	cucumber_wait(4, 3993767246u);
	assert(file_sync_writer_open("failed.sync", &d, &g.writers[0], NULL) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(6, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(7, 2535966255u);
	assert(g.recording_error < 0 && g.recorded == 4);
	cucumber_reply();

	cucumber_wait(8, 1051964499u);
	assert(replica(&g, 0, g.actors[0]));
	cucumber_reply();

	cucumber_wait(9, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(10, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 1311815507u);
	assert(status.state == ENET_SYNC_ACTIVE && !status.fault.detail.code);
	cucumber_reply();

	cucumber_wait(12, 2712904220u);
	assert(file_sync_writer_close(g.writers[0], NULL) == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(13, 183661498u);
	g.writers[0] = NULL;
	cucumber_reply();

	cucumber_wait(14, 1982804189u);
	assert(remove("failed.sync") == 0);
	cucumber_reply();

	cucumber_wait(15, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

#endif

#ifdef ENET_SYNC_TESTING
static void outgoing_pressure(void)
{
	cucumber_wait(1, 1051095699u);
	enet_sync_options_t options = {.transport_limits = {.max_requests = 1}};
	cucumber_reply();

	cucumber_wait(2, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(3, 1044430701u);
	game_open(&g, false, 0, 0, &options, 2);
	cucumber_reply();

	cucumber_wait(4, 4235920092u);
	es_hold_packets = 1;
	cucumber_reply();

	cucumber_wait(5, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 168697446u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(8, 2647658753u);
	assert(p.frames == 1);
	cucumber_reply();

	cucumber_wait(9, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(10, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 1775893324u);
	assert(!status.retained_packets && status.retained_bytes);
	cucumber_reply();

	cucumber_wait(12, 1095148412u);
	sync_frame_view_t before, after;
	cucumber_reply();

	cucumber_wait(13, 3833846815u);
	assert(sync_outbox_peek(g.server.session, g.peers[0], &before) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 168697446u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(15, 735655936u);
	assert(!p.frames);
	cucumber_reply();

	cucumber_wait(16, 1077061977u);
	assert(sync_outbox_peek(g.server.session, g.peers[0], &after) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 3126373146u);
	assert(before.data == after.data && before.size == after.size);
	cucumber_reply();

	cucumber_wait(18, 624274245u);
	assert(!memcmp(&before.token, &after.token, sizeof(before.token)));
	cucumber_reply();

	cucumber_wait(19, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(20, 4020987439u);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 3216034083u);
	es_release_packets();
	cucumber_reply();

	cucumber_wait(22, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(23, 2946337000u);
	assert(!status.retained_bytes && !status.retained_packets);
	cucumber_reply();

	cucumber_wait(24, 3848287133u);
	es_hold_packets = 0;
	cucumber_reply();

	cucumber_wait(25, 1152853327u);
	game_settle(&g, 2, 1, false);
	cucumber_reply();

	cucumber_wait(26, 406621266u);
	game_check(&g, 2);
	cucumber_reply();

	cucumber_wait(27, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void outgoing_bytes(void)
{
	cucumber_wait(1, 1371526465u);
	struct world server;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 3944719455u);
	enet_sync_server_desc_t d = {.session = server.session,
		.options.transport_limits.max_outgoing_bytes = 1};
	cucumber_reply();

	cucumber_wait(4, 1973531821u);
	enet_sync_server_t *s = NULL;
	cucumber_reply();

	cucumber_wait(5, 4175274420u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(6, 386340301u);
	assert(!s);
	cucumber_reply();

	cucumber_wait(7, 952296668u);
	d.options.transport_limits.max_outgoing_bytes = 0;
	cucumber_reply();

	cucumber_wait(8, 320205182u);
	d.options.transport_limits.max_packet_bytes = 1;
	cucumber_reply();

	cucumber_wait(9, 4175274420u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(10, 386340301u);
	assert(!s);
	cucumber_reply();

	cucumber_wait(11, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

static void per_peer_budgets(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2192248417u);
	game_open(&g, false, 0, 0, NULL, 3);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(5, 1401555708u);
	assert(enet_sync_server_send(g.listener, 1, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 2656355695u);
	assert(p.frames == 3);
	cucumber_reply();

	cucumber_wait(7, 556957812u);
	for (unsigned i = 0; i < 3; i++) {
		sync_frame_view_t v;
		assert(sync_outbox_peek(g.server.session, g.peers[i], &v) ==
			SYNC_OK);
		assert(v.info.sequence == 2);
	}
	cucumber_reply();

	cucumber_wait(8, 2569141450u);
	game_settle(&g, 3, 1, false);
	cucumber_reply();

	cucumber_wait(9, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void client_pressure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2460615309u);
	game_open(&g, false, 1024, 1024, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 3610557938u);
	for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health h = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(h), &h);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 3774932118u);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(8, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(9, 2457936135u);
	assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(10, 3387382335u);
	assert(p.frames);
	cucumber_reply();

	cucumber_wait(11, 3584742444u);
	enet_sync_status_t before, after;
	cucumber_reply();

	cucumber_wait(12, 2311912505u);
	assert(enet_sync_client_get_status(g.connections[0], &before) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(13, 2673285734u);
	assert(before.pending_bytes);
	cucumber_reply();

	cucumber_wait(14, 2403839001u);
	int services = es_protocol_services;
	cucumber_reply();

	cucumber_wait(15, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(16, 2457936135u);
	assert(enet_sync_client_service(g.connections[0], 64, &p) ==
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(17, 647953091u);
	assert(!p.frames && !p.events && es_protocol_services > services);
	cucumber_reply();

	cucumber_wait(18, 3539362892u);
	assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(19, 4265246446u);
	assert(before.pending_bytes == after.pending_bytes);
	cucumber_reply();

	cucumber_wait(20, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(21, 3539362892u);
	assert(enet_sync_client_get_status(g.connections[0], &after) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(22, 1063486262u);
	assert(!after.pending_bytes);
	cucumber_reply();

	cucumber_wait(23, 3490566933u);
	sync_status_t core;
	cucumber_reply();

	cucumber_wait(24, 2679528859u);
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(25, 3643929918u);
	assert(core.entities == 203);
	cucumber_reply();

	cucumber_wait(26, 3449731933u);
	game_close(&g);
	cucumber_reply();
}


static void send_failure(void) {
	cucumber_wait(1, 869850777u);
	bool allocation = false;
	cucumber_reply();

	cucumber_wait(2, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(3, 2204722262u);
	game_open(&g, false, 0, 0, NULL, 2);
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 71385138u);
	if (allocation)
		es_fail_packet = 1;
	else
		es_fail_send = 1;
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 4224704032u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 2106279071u);
	assert(!p.faulted_connections && p.frames == 8);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 555685309u);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(11, 1673837744u);
	assert(p.faulted_connections == 1);
	cucumber_reply();

	cucumber_wait(12, 1406801805u);
	assert(g.faults == 1 && !g.peers[0] && g.peers[1]);
	cucumber_reply();

	cucumber_wait(13, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(14, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 906024755u);
	assert(status.state == ENET_SYNC_ACTIVE && status.connections == 1);
	cucumber_reply();

	cucumber_wait(16, 3113253050u);
	assert(status.last_connection_error.detail.code ==
		(allocation ? SYNC_ERR_NOMEM : SYNC_ERR_TRANSPORT));
	cucumber_reply();

	cucumber_wait(17, 3216034083u);
	es_release_packets();
	cucumber_reply();

	cucumber_wait(18, 3308316443u);
	ecs_progress(g.clients[1].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(19, 2210639765u);
	assert(enet_sync_client_service(g.connections[1], 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(20, 3853983782u);
	assert(sync_apply(g.clients[1].session, 1, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 3105193744u);
	assert(replica(&g, 1, g.actors[0]));
	cucumber_reply();

	cucumber_wait(22, 3449731933u);
	game_close(&g);
	cucumber_reply();
}
static void packet_failure(void) {
	cucumber_wait(1, 261710136u);
	bool allocation = true;
	cucumber_reply();

	cucumber_wait(2, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(3, 2204722262u);
	game_open(&g, false, 0, 0, NULL, 2);
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 71385138u);
	if (allocation)
		es_fail_packet = 1;
	else
		es_fail_send = 1;
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 4224704032u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 2106279071u);
	assert(!p.faulted_connections && p.frames == 8);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 555685309u);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(11, 1673837744u);
	assert(p.faulted_connections == 1);
	cucumber_reply();

	cucumber_wait(12, 1406801805u);
	assert(g.faults == 1 && !g.peers[0] && g.peers[1]);
	cucumber_reply();

	cucumber_wait(13, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(14, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 906024755u);
	assert(status.state == ENET_SYNC_ACTIVE && status.connections == 1);
	cucumber_reply();

	cucumber_wait(16, 3113253050u);
	assert(status.last_connection_error.detail.code ==
		(allocation ? SYNC_ERR_NOMEM : SYNC_ERR_TRANSPORT));
	cucumber_reply();

	cucumber_wait(17, 3216034083u);
	es_release_packets();
	cucumber_reply();

	cucumber_wait(18, 3308316443u);
	ecs_progress(g.clients[1].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(19, 2210639765u);
	assert(enet_sync_client_service(g.connections[1], 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(20, 3853983782u);
	assert(sync_apply(g.clients[1].session, 1, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 3105193744u);
	assert(replica(&g, 1, g.actors[0]));
	cucumber_reply();

	cucumber_wait(22, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void host_failure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2204722262u);
	game_open(&g, false, 0, 0, NULL, 2);
	cucumber_reply();

	cucumber_wait(3, 4026405975u);
	es_fail_service = 1;
	cucumber_reply();

	cucumber_wait(4, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(5, 4228744155u);
	assert(enet_sync_server_poll(g.listener, 64, NULL) ==
		SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(6, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(7, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 2472672235u);
	assert(status.state == ENET_SYNC_FAULTED && !status.connections);
	cucumber_reply();

	cucumber_wait(9, 1898285636u);
	assert(g.faults == 2);
	cucumber_reply();

	cucumber_wait(10, 1509656811u);
	assert(enet_sync_server_send(g.listener, 1, NULL) == SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(11, 388552811u);
	assert(enet_sync_server_close(g.listener, NULL) == SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(12, 2883907288u);
	g.listener = NULL;
	cucumber_reply();

	cucumber_wait(13, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void open_failures(void)
{
	cucumber_wait(1, 1131742053u);
	struct world server, client;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 1721062718u);
	world_open(&client, false, false, 0, 0);
	cucumber_reply();

	cucumber_wait(4, 3062199521u);
	enet_sync_server_desc_t d = {.session = server.session};
	cucumber_reply();

	cucumber_wait(5, 2466451462u);
	enet_sync_server_t *s;
	cucumber_reply();

	cucumber_wait(6, 1301516632u);
	es_fail_host = 1;
	cucumber_reply();

	cucumber_wait(7, 1394033897u);
	assert(enet_sync_server_listen(&d, &s, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(9, 479560238u);
	assert(enet_sync_server_get_status(s, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 162596743u);
	assert(status.state == ENET_SYNC_OPENING && !es_hosts_live);
	cucumber_reply();

	cucumber_wait(11, 1184613099u);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(12, 3231567973u);
	ecs_progress(server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(13, 3333100409u);
	assert(enet_sync_server_poll(s, 64, NULL) == SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(14, 3589165563u);
	assert(enet_sync_server_close(s, NULL) == SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(15, 3209080251u);
	enet_sync_client_desc_t cd = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
	cucumber_reply();

	cucumber_wait(16, 3660446953u);
	enet_sync_client_t *c;
	cucumber_reply();

	cucumber_wait(17, 3035030609u);
	es_fail_connect = 1;
	cucumber_reply();

	cucumber_wait(18, 2727475719u);
	assert(enet_sync_client_connect(&cd, &c, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(19, 294057053u);
	ecs_progress(client.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(20, 2195552254u);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 294057053u);
	ecs_progress(client.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(22, 854020419u);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(23, 3607565804u);
	assert(enet_sync_client_close(c, NULL) == SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(24, 2851874964u);
	assert(!es_hosts_live);
	cucumber_reply();

	cucumber_wait(25, 4033515195u);
	world_close(&client);
	cucumber_reply();

	cucumber_wait(26, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

static void malformed_packet(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(5, 3901996999u);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	cucumber_reply();

	cucumber_wait(6, 3519009084u);
	const char bad[] = "malformed";
	cucumber_reply();

	cucumber_wait(7, 885922891u);
	es_inject_client(bad, sizeof(bad), 0, ENET_PACKET_FLAG_RELIABLE);
	cucumber_reply();

	cucumber_wait(8, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(9, 531797596u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(10, 4022773515u);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(11, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(12, 274326467u);
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(13, 84588569u);
	assert(status.state == ENET_SYNC_FAULTED && !status.pending_bytes);
	cucumber_reply();

	cucumber_wait(14, 1213720721u);
	assert(enet_sync_client_close(g.connections[0], NULL) ==
		SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(15, 1363732335u);
	g.connections[0] = NULL;
	cucumber_reply();

	cucumber_wait(16, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void core_peer_isolation(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2204722262u);
	game_open(&g, false, 0, 0, NULL, 2);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 1152853327u);
	game_settle(&g, 2, 1, false);
	cucumber_reply();

	cucumber_wait(5, 1098822350u);
	/* Bram's item references a secret NPC invisible in his view. */
	struct item value = {g.actors[2], 1};
	cucumber_reply();

	cucumber_wait(6, 1284140658u);
	ecs_set_id(
		g.server.ecs, g.items[1], g.server.item, sizeof(value), &value);
	cucumber_reply();

	cucumber_wait(7, 642381725u);
	assert(sync_capture(g.server.session, 2, NULL) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(8, 2314116690u);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(9, 2552423142u);
	assert(g.peers[0] && !g.peers[1]);
	cucumber_reply();

	cucumber_wait(10, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(11, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 2633327803u);
	assert(status.last_connection_error.detail.code == SYNC_ERR_REFERENCE);
	cucumber_reply();

	cucumber_wait(13, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(14, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(15, 1692272898u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 3544559829u);
	assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void retained_cleanup(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 4235920092u);
	es_hold_packets = 1;
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 3774932118u);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(7, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 1775893324u);
	assert(!status.retained_packets && status.retained_bytes);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 3906951058u);
	assert(status.retained_packets == 4 && status.retained_bytes);
	cucumber_reply();

	cucumber_wait(12, 3449731933u);
	game_close(&g);
	cucumber_reply();

	cucumber_wait(13, 3848287133u);
	es_hold_packets = 0;
	cucumber_reply();
}

static void server_disconnect(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2204722262u);
	game_open(&g, false, 0, 0, NULL, 2);
	cucumber_reply();

	cucumber_wait(3, 3222595111u);
	ecs_entity_t peer = g.peers[0];
	cucumber_reply();

	cucumber_wait(4, 1506350698u);
	assert(enet_sync_server_disconnect(g.listener, peer, 9) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(5, 2059013028u);
	assert(!g.disconnected);
	cucumber_reply();

	cucumber_wait(6, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(7, 4020987439u);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 3613358927u);
	assert(g.disconnected == 1 && !g.peers[0]);
	cucumber_reply();

	cucumber_wait(9, 3972506147u);
	assert(enet_sync_server_disconnect(g.listener, peer, 9) ==
		SYNC_ERR_NOT_FOUND);
	cucumber_reply();

	cucumber_wait(10, 3499010348u);
	assert(!ecs_is_alive(g.server.ecs, peer));
	cucumber_reply();

	cucumber_wait(11, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(12, 3371596223u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_END);
	cucumber_reply();

	cucumber_wait(13, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void client_apply_failure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 458352243u);
	sync_frame_view_t v;
	cucumber_reply();

	cucumber_wait(5, 1634868346u);
	assert(sync_outbox_peek(g.server.session, g.peers[0], &v) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 182763253u);
	void *copy = malloc(v.size);
	cucumber_reply();

	cucumber_wait(7, 2240214717u);
	assert(copy);
	cucumber_reply();

	cucumber_wait(8, 1839269881u);
	memcpy(copy, v.data, v.size);
	cucumber_reply();

	cucumber_wait(9, 4024719016u);
	size_t size = v.size;
	cucumber_reply();

	cucumber_wait(10, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(11, 3901996999u);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	cucumber_reply();

	cucumber_wait(12, 1096838932u);
	es_inject_client(copy, size, 0, ENET_PACKET_FLAG_RELIABLE);
	cucumber_reply();

	cucumber_wait(13, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(14, 61324569u);
	free(copy);
	cucumber_reply();

	cucumber_wait(15, 1692272898u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 559706837u);
	assert(sync_apply(g.clients[0].session, 1, NULL) == SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(17, 4016982442u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(18, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(19, 274326467u);
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(20, 3760844810u);
	assert(status.state == ENET_SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(21, 1808482341u);
	assert(status.fault.detail.code == SYNC_ERR_SEQUENCE);
	cucumber_reply();

	cucumber_wait(22, 4022773515u);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(23, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void poll_budget(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2163060792u);
	game_open(&g, false, 0, 0, NULL, 0);
	cucumber_reply();

	cucumber_wait(3, 3275426390u);
	for (unsigned i = 0; i < 3; i++) {
		world_open(&g.clients[i], false, false, 0, 0);
		game_connect(&g, i);
	}
	cucumber_reply();

	cucumber_wait(4, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(5, 2941575972u);
	for (unsigned i = 0; i < 3; i++) {
		ecs_progress(g.clients[i].ecs, 0.001f);
		enet_sync_client_service(g.connections[i], 64, NULL);
		ecs_progress(g.clients[i].ecs, 0.001f);
	}
	cucumber_reply();

	cucumber_wait(6, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(7, 2625886819u);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 2559722969u);
	assert(p.events == 1 && p.connections == 1 && g.connected == 1);
	cucumber_reply();

	cucumber_wait(9, 2625886819u);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 2597649280u);
	assert(p.events == 1 && p.connections == 1 && g.connected == 2);
	cucumber_reply();

	cucumber_wait(11, 2625886819u);
	assert(enet_sync_server_poll(g.listener, 1, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 3864992182u);
	assert(g.connected == 3);
	cucumber_reply();

	cucumber_wait(13, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void client_host_failure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(4, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(5, 3901996999u);
	ecs_entity_t old = replica(&g, 0, g.actors[0]);
	cucumber_reply();

	cucumber_wait(6, 4026405975u);
	es_fail_service = 1;
	cucumber_reply();

	cucumber_wait(7, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(8, 4071661872u);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_TRANSPORT);
	cucumber_reply();

	cucumber_wait(9, 4022773515u);
	assert(ecs_is_alive(g.clients[0].ecs, old));
	cucumber_reply();

	cucumber_wait(10, 775800849u);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(11, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void unreliable_packet(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 215121542u);
	const char bytes[] = "invalid channel/flags";
	cucumber_reply();

	cucumber_wait(4, 669487327u);
	es_inject_client(bytes, sizeof(bytes), 0, 0);
	cucumber_reply();

	cucumber_wait(5, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(6, 784378045u);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(7, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(8, 274326467u);
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(9, 3760844810u);
	assert(status.state == ENET_SYNC_FAULTED);
	cucumber_reply();

	cucumber_wait(10, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void peer_setup_failure(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2163060792u);
	game_open(&g, false, 0, 0, NULL, 0);
	cucumber_reply();

	cucumber_wait(3, 3262093660u);
	/* Other application recipients consume all core peer capacity. */
	for (unsigned i = 0; i < 64; i++) {
		ecs_entity_t peer;
		assert(sync_peer_open(g.server.session, &peer) == SYNC_OK);
	}
	cucumber_reply();

	cucumber_wait(4, 1533216090u);
	world_open(&g.clients[0], false, false, 0, 0);
	cucumber_reply();

	cucumber_wait(5, 1143544494u);
	game_connect(&g, 0);
	cucumber_reply();

	cucumber_wait(6, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(7, 3043044981u);
	enet_sync_client_service(g.connections[0], 64, NULL);
	cucumber_reply();

	cucumber_wait(8, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 1550764370u);
	assert(enet_sync_server_poll(g.listener, 1, NULL) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(11, 1281791374u);
	assert(g.faults == 1 && !g.connected && !g.peers[0]);
	cucumber_reply();

	cucumber_wait(12, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(13, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 3020409100u);
	assert(status.state == ENET_SYNC_ACTIVE && !status.connections);
	cucumber_reply();

	cucumber_wait(15, 50766849u);
	assert(status.last_connection_error.detail.code == SYNC_ERR_LIMIT);
	cucumber_reply();

	cucumber_wait(16, 3449731933u);
	game_close(&g);
	cucumber_reply();
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
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2163060792u);
	game_open(&g, false, 0, 0, NULL, 0);
	cucumber_reply();

	cucumber_wait(3, 1533216090u);
	world_open(&g.clients[0], false, false, 0, 0);
	cucumber_reply();

	cucumber_wait(4, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(5, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 1879327897u);
	uint32_t accepted = 0;
	cucumber_reply();

	cucumber_wait(7, 4060782620u);
	enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options = {.on_frame = count_frames, .ctx = &accepted}};
	cucumber_reply();

	cucumber_wait(8, 4283313317u);
	d.address.ip[0] = 127;
	cucumber_reply();

	cucumber_wait(9, 1870152394u);
	d.address.ip[3] = 1;
	cucumber_reply();

	cucumber_wait(10, 2491430640u);
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(11, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(12, 3043044981u);
	enet_sync_client_service(g.connections[0], 64, NULL);
	cucumber_reply();

	cucumber_wait(13, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(14, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(15, 4020987439u);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 1692272898u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(17, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(18, 4185737601u);
	game_settle(&g, 1, 1, false);
	cucumber_reply();

	cucumber_wait(19, 936586908u);
	assert(accepted == 4);
	cucumber_reply();

	cucumber_wait(20, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void automatic_custom_budget(void)
{
	cucumber_wait(1, 1131742053u);
	struct world server, client;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 649055109u);
	world_open(&client, false, true, 0, 0);
	cucumber_reply();

	cucumber_wait(4, 4175033026u);
	enet_sync_server_desc_t d = {.session = server.session,
		.options = {.schedule = SYNC_AUTOMATIC,
			.events_per_progress = 1,
			.frames_per_peer = 1}};
	cucumber_reply();

	cucumber_wait(5, 1413718953u);
	enet_sync_server_t *listener;
	cucumber_reply();

	cucumber_wait(6, 2595075558u);
	assert(enet_sync_server_listen(&d, &listener, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(8, 3231567973u);
	ecs_progress(server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(9, 2753543043u);
	assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 116819889u);
	enet_sync_client_desc_t cd = {.session = client.session,
		.address = status.address,
		.options = {.schedule = SYNC_AUTOMATIC,
			.events_per_progress = 1,
			.frames_per_peer = 1}};
	cucumber_reply();

	cucumber_wait(11, 1410176404u);
	cd.address.ip[0] = 127;
	cucumber_reply();

	cucumber_wait(12, 966718982u);
	cd.address.ip[3] = 1;
	cucumber_reply();

	cucumber_wait(13, 818170019u);
	enet_sync_client_t *connection;
	cucumber_reply();

	cucumber_wait(14, 1267393031u);
	assert(enet_sync_client_connect(&cd, &connection, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 3391436121u);
	memset(&d, 0xff, sizeof(d));
	cucumber_reply();

	cucumber_wait(16, 3392357256u);
	for (unsigned i = 0; i < 4; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
	}
	cucumber_reply();

	cucumber_wait(17, 3490566933u);
	sync_status_t core;
	cucumber_reply();

	cucumber_wait(18, 142535624u);
	assert(sync_get_status(server.session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(19, 4047141276u);
	assert(core.peers == 1 && !core.last_tick && !core.queued_bytes);
	cucumber_reply();

	cucumber_wait(20, 3698660953u);
	/* Automatic transport never captures a manual core session. */
	assert(sync_capture(server.session, 1, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 2109851056u);
	ecs_progress(server.ecs, 0.01f);
	cucumber_reply();

	cucumber_wait(22, 2753543043u);
	assert(enet_sync_server_get_status(listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(23, 3579976966u);
	assert(status.frames == 1 && status.options.frames_per_peer == 1);
	cucumber_reply();

	cucumber_wait(24, 4039120493u);
	assert(status.options.events_per_progress == 1);
	cucumber_reply();

	cucumber_wait(25, 3711256331u);
	for (unsigned i = 0; i < 10; i++) {
		ecs_progress(server.ecs, 0.01f);
		ecs_progress(client.ecs, 0.01f);
	}
	cucumber_reply();

	cucumber_wait(26, 4101533339u);
	assert(sync_get_status(client.session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(27, 3669781790u);
	assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
	cucumber_reply();

	cucumber_wait(28, 1928080094u);
	assert(enet_sync_client_close(connection, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(29, 4169040895u);
	assert(enet_sync_server_close(listener, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(30, 4033515195u);
	world_close(&client);
	cucumber_reply();

	cucumber_wait(31, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

static void empty_packet(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2167396879u);
	game_open(&g, false, 0, 0, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 1362767934u);
	es_inject_client("", 0, 0, ENET_PACKET_FLAG_RELIABLE);
	cucumber_reply();

	cucumber_wait(4, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(5, 784378045u);
	assert(enet_sync_client_service(g.connections[0], 1, NULL) ==
		SYNC_ERR_PROTOCOL);
	cucumber_reply();

	cucumber_wait(6, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void shared_world_ownership(void)
{
	cucumber_wait(1, 1371526465u);
	struct world server;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 3732995308u);
	flecs_enet_ids_t ids, again;
	cucumber_reply();

	cucumber_wait(4, 8892903u);
	assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
	cucumber_reply();

	cucumber_wait(5, 946175483u);
	assert(flecs_enet_import(server.ecs, &again) == FLECS_ENET_OK);
	cucumber_reply();

	cucumber_wait(6, 1296943817u);
	assert(ids.module == again.module);
	cucumber_reply();

	cucumber_wait(7, 1056523738u);
	ecs_entity_t external = ecs_new(server.ecs), request;
	cucumber_reply();

	cucumber_wait(8, 3964159900u);
	flecs_enet_host_config_t config = {.listen = true};
	cucumber_reply();

	cucumber_wait(9, 592939932u);
	ecs_set_id(
		server.ecs, external, ids.host_config, sizeof(config), &config);
	cucumber_reply();

	cucumber_wait(10, 1964232786u);
	flecs_enet_request_t open = {.operation = FLECS_ENET_OPEN};
	cucumber_reply();

	cucumber_wait(11, 2605530210u);
	assert(flecs_enet_submit(server.ecs, external, &open, NULL, 0,
		       &request) == FLECS_ENET_OK);
	cucumber_reply();

	cucumber_wait(12, 1532824725u);
	enet_sync_server_desc_t desc = {.session = server.session};
	cucumber_reply();

	cucumber_wait(13, 3050624646u);
	enet_sync_server_t *a, *b;
	cucumber_reply();

	cucumber_wait(14, 3920220187u);
	assert(enet_sync_server_listen(&desc, &a, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 2461487864u);
	assert(enet_sync_server_listen(&desc, &b, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 3231567973u);
	ecs_progress(server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(17, 1330178086u);
	assert(enet_sync_server_poll(a, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(18, 2057657973u);
	assert(enet_sync_server_poll(b, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(19, 712291770u);
	enet_sync_status_t sa, sb;
	cucumber_reply();

	cucumber_wait(20, 3142469166u);
	assert(enet_sync_server_get_status(a, &sa) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(21, 94003613u);
	assert(enet_sync_server_get_status(b, &sb) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(22, 1598376954u);
	assert(sa.host != sb.host && sa.host != external);
	cucumber_reply();

	cucumber_wait(23, 2224738686u);
	const flecs_enet_host_status_t *hs =
		ecs_get_id(server.ecs, sa.host, ids.host_status);
	cucumber_reply();

	cucumber_wait(24, 2961400593u);
	assert(hs && !hs->pending_requests && !hs->retained_events);
	cucumber_reply();

	cucumber_wait(25, 2045889052u);
	assert(enet_sync_server_close(a, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(26, 4035300164u);
	assert(!ecs_is_alive(server.ecs, sa.host));
	cucumber_reply();

	cucumber_wait(27, 265534255u);
	assert(ecs_is_alive(server.ecs, sb.host));
	cucumber_reply();

	cucumber_wait(28, 2508922912u);
	assert(ecs_is_alive(server.ecs, external) && es_hosts_live == 2);
	cucumber_reply();

	cucumber_wait(29, 49236223u);
	assert(enet_sync_server_close(b, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(30, 3247076929u);
	assert(es_hosts_live == 1);
	cucumber_reply();

	cucumber_wait(31, 760321177u);
	ecs_delete(server.ecs, external);
	cucumber_reply();

	cucumber_wait(32, 2851874964u);
	assert(!es_hosts_live);
	cucumber_reply();

	cucumber_wait(33, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

static void host_packet_limit(void)
{
	cucumber_wait(1, 2776145998u);
	enet_sync_options_t options = {
		.transport_limits = {.max_outgoing_packets = 1}};
	cucumber_reply();

	cucumber_wait(2, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(3, 1044430701u);
	game_open(&g, false, 0, 0, &options, 2);
	cucumber_reply();

	cucumber_wait(4, 4235920092u);
	es_hold_packets = 1;
	cucumber_reply();

	cucumber_wait(5, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 4224704032u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 1840502171u);
	assert(p.frames == 8 && g.recorded == 8);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(11, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 2639315302u);
	assert(status.retained_packets == 1);
	cucumber_reply();

	cucumber_wait(13, 2612025637u);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(15, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 2639315302u);
	assert(status.retained_packets == 1);
	cucumber_reply();

	cucumber_wait(17, 3848287133u);
	es_hold_packets = 0;
	cucumber_reply();

	cucumber_wait(18, 3216034083u);
	es_release_packets();
	cucumber_reply();

	cucumber_wait(19, 1152853327u);
	game_settle(&g, 2, 1, false);
	cucumber_reply();

	cucumber_wait(20, 406621266u);
	game_check(&g, 2);
	cucumber_reply();

	cucumber_wait(21, 785801172u);
	assert(g.recorded == 8);
	cucumber_reply();

	cucumber_wait(22, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void failed_send_cancellation(void)
{
	cucumber_wait(1, 1893431941u);
	enet_sync_options_t options = {.transport_limits = {.max_events = 1}};
	cucumber_reply();

	cucumber_wait(2, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(3, 1044430701u);
	game_open(&g, false, 0, 0, &options, 2);
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 4145655465u);
	es_fail_send = 1;
	cucumber_reply();

	cucumber_wait(6, 1683871321u);
	enet_sync_progress_t p;
	cucumber_reply();

	cucumber_wait(7, 4224704032u);
	assert(enet_sync_server_send(g.listener, 64, &p) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 1840502171u);
	assert(p.frames == 8 && g.recorded == 8);
	cucumber_reply();

	cucumber_wait(9, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 555685309u);
	assert(enet_sync_server_poll(g.listener, 64, &p) == SYNC_PARTIAL);
	cucumber_reply();

	cucumber_wait(11, 2145678696u);
	assert(p.faulted_connections == 1 && !g.peers[0] && g.peers[1]);
	cucumber_reply();

	cucumber_wait(12, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(13, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 3732475618u);
	flecs_enet_ids_t ids;
	cucumber_reply();

	cucumber_wait(15, 4080013663u);
	assert(flecs_enet_import(g.server.ecs, &ids) == FLECS_ENET_OK);
	cucumber_reply();

	cucumber_wait(16, 2604991923u);
	const flecs_enet_host_status_t *h =
		ecs_get_id(g.server.ecs, status.host, ids.host_status);
	cucumber_reply();

	cucumber_wait(17, 1603343203u);
	/* Four healthy sends and one local reset remain; failed peer sends
	 * gone. */
	assert(h->pending_requests == 5);
	cucumber_reply();

	cucumber_wait(18, 371155890u);
	for (unsigned i = 0; i < 10; i++) {
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_service(g.listener, 64, 64, NULL) >= 0);
		ecs_progress(g.clients[1].ecs, 0.001f);
		assert(enet_sync_client_service(g.connections[1], 64, NULL) >=
			0);
		assert(sync_apply(g.clients[1].session, 64, NULL) >= 0);
	}
	cucumber_reply();

	cucumber_wait(19, 1725839632u);
	h = ecs_get_id(g.server.ecs, status.host, ids.host_status);
	cucumber_reply();

	cucumber_wait(20, 4021366687u);
	assert(!h->pending_requests && !h->retained_events);
	cucumber_reply();

	cucumber_wait(21, 3105193744u);
	assert(replica(&g, 1, g.actors[0]));
	cucumber_reply();

	cucumber_wait(22, 785801172u);
	assert(g.recorded == 8);
	cucumber_reply();

	cucumber_wait(23, 3449731933u);
	game_close(&g);
	cucumber_reply();
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
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2163060792u);
	game_open(&g, false, 0, 0, NULL, 0);
	cucumber_reply();

	cucumber_wait(3, 141195488u);
	world_open(&g.clients[0], false, true, 0, 0);
	cucumber_reply();

	cucumber_wait(4, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(5, 2951109199u);
	assert(enet_sync_server_get_status(g.listener, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 17107995u);
	enet_sync_client_desc_t d = {.session = g.clients[0].session,
		.address = status.address,
		.data = 1,
		.options.schedule = SYNC_AUTOMATIC};
	cucumber_reply();

	cucumber_wait(7, 4283313317u);
	d.address.ip[0] = 127;
	cucumber_reply();

	cucumber_wait(8, 1870152394u);
	d.address.ip[3] = 1;
	cucumber_reply();

	cucumber_wait(9, 2491430640u);
	assert(enet_sync_client_connect(&d, &g.connections[0], NULL) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 3267042037u);
	for (unsigned i = 0; i < 4; i++) {
		ecs_progress(g.clients[0].ecs, 0.001f);
		ecs_progress(g.server.ecs, 0.001f);
		assert(enet_sync_server_poll(g.listener, 64, NULL) >= 0);
	}
	cucumber_reply();

	cucumber_wait(11, 2915839356u);
	assert(g.peers[0]);
	cucumber_reply();

	cucumber_wait(12, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(13, 3774932118u);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(14, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(15, 2164870413u);
	ecs_entity_t reader = ecs_new(g.clients[0].ecs);
	cucumber_reply();

	cucumber_wait(16, 2200089655u);
	ecs_add_pair(g.clients[0].ecs, reader, EcsDependsOn, EcsPreUpdate);
	cucumber_reply();

	cucumber_wait(17, 1507072858u);
	assert(ecs_system_init(g.clients[0].ecs,
		&(ecs_system_desc_t){.entity = reader,
			.callback = published_reader,
			.ctx = &g.clients[0],
			.immediate = true}));
	cucumber_reply();

	cucumber_wait(18, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(19, 1051964499u);
	assert(replica(&g, 0, g.actors[0]));
	cucumber_reply();

	cucumber_wait(20, 1145663691u);
	ecs_delete(g.clients[0].ecs, reader);
	cucumber_reply();

	cucumber_wait(21, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void pending_disconnect(void)
{
	cucumber_wait(1, 519900866u);
	struct world client;
	cucumber_reply();

	cucumber_wait(2, 1721062718u);
	world_open(&client, false, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 261159098u);
	enet_sync_client_desc_t d = {.session = client.session,
		.address = {.ip = {127, 0, 0, 1}, .port = 9999}};
	cucumber_reply();

	cucumber_wait(4, 3660446953u);
	enet_sync_client_t *c;
	cucumber_reply();

	cucumber_wait(5, 3641592031u);
	assert(enet_sync_client_connect(&d, &c, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 379197637u);
	assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(7, 379197637u);
	assert(enet_sync_client_disconnect(c, 7) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(8, 2877914202u);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_EMPTY);
	cucumber_reply();

	cucumber_wait(9, 294057053u);
	ecs_progress(client.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(10, 2082194678u);
	assert(enet_sync_client_service(c, 64, NULL) == SYNC_END);
	cucumber_reply();

	cucumber_wait(11, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(12, 3219758469u);
	assert(enet_sync_client_get_status(c, &status) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(13, 545649310u);
	assert(status.state == ENET_SYNC_DISCONNECTED);
	cucumber_reply();

	cucumber_wait(14, 3307654665u);
	assert(enet_sync_client_close(c, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(15, 4033515195u);
	world_close(&client);
	cucumber_reply();
}

static void ordered_receive_disconnect(void)
{
	cucumber_wait(1, 1460412304u);
	struct game g;
	cucumber_reply();

	cucumber_wait(2, 2460615309u);
	game_open(&g, false, 1024, 1024, NULL, 1);
	cucumber_reply();

	cucumber_wait(3, 1923651516u);
	for (unsigned i = 0; i < 200; i++) {
		ecs_entity_t e = ecs_new(g.server.ecs);
		struct health value = {(int32_t)i};
		ecs_set_id(g.server.ecs, e, g.server.health, sizeof(value),
			&value);
		ecs_add_id(g.server.ecs, e, g.server.ids.replicate_to_all);
	}
	cucumber_reply();

	cucumber_wait(4, 4221563585u);
	game_capture(&g, 1);
	cucumber_reply();

	cucumber_wait(5, 3774932118u);
	assert(enet_sync_server_send(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(6, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(7, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(8, 2555500059u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(9, 2764254197u);
	assert(enet_sync_server_disconnect(g.listener, g.peers[0], 8) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(10, 3118159817u);
	ecs_progress(g.server.ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(11, 4020987439u);
	assert(enet_sync_server_poll(g.listener, 64, NULL) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(12, 1800959626u);
	ecs_progress(g.clients[0].ecs, 0.001f);
	cucumber_reply();

	cucumber_wait(13, 2555500059u);
	assert(enet_sync_client_service(g.connections[0], 64, NULL) ==
		SYNC_BACKPRESSURE);
	cucumber_reply();

	cucumber_wait(14, 3392634173u);
	enet_sync_status_t status;
	cucumber_reply();

	cucumber_wait(15, 274326467u);
	assert(enet_sync_client_get_status(g.connections[0], &status) ==
		SYNC_OK);
	cucumber_reply();

	cucumber_wait(16, 874742671u);
	assert(status.state == ENET_SYNC_ACTIVE && status.pending_bytes);
	cucumber_reply();

	cucumber_wait(17, 133210207u);
	sync_result_t r = SYNC_OK;
	cucumber_reply();

	cucumber_wait(18, 519556723u);
	for (unsigned i = 0; i < 20 && r != SYNC_END; i++) {
		assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
		r = enet_sync_client_service(g.connections[0], 64, NULL);
		assert(r >= 0);
	}
	cucumber_reply();

	cucumber_wait(19, 1514876193u);
	assert(r == SYNC_END);
	cucumber_reply();

	cucumber_wait(20, 2086311147u);
	assert(sync_apply(g.clients[0].session, 64, NULL) >= 0);
	cucumber_reply();

	cucumber_wait(21, 3490566933u);
	sync_status_t core;
	cucumber_reply();

	cucumber_wait(22, 2679528859u);
	assert(sync_get_status(g.clients[0].session, 0, &core) == SYNC_OK);
	cucumber_reply();

	cucumber_wait(23, 3669781790u);
	assert(core.state == SYNC_CLIENT_READY && core.last_tick == 1);
	cucumber_reply();

	cucumber_wait(24, 3643929918u);
	assert(core.entities == 203);
	cucumber_reply();

	cucumber_wait(25, 3449731933u);
	game_close(&g);
	cucumber_reply();
}

static void phase_cycle(void)
{
	cucumber_wait(1, 1371526465u);
	struct world server;
	cucumber_reply();

	cucumber_wait(2, 645226379u);
	world_open(&server, true, false, 0, 0);
	cucumber_reply();

	cucumber_wait(3, 3732475618u);
	flecs_enet_ids_t ids;
	cucumber_reply();

	cucumber_wait(4, 8892903u);
	assert(flecs_enet_import(server.ecs, &ids) == FLECS_ENET_OK);
	cucumber_reply();

	cucumber_wait(5, 2835771325u);
	ecs_add_pair(server.ecs, server.ids.send_phase, EcsDependsOn,
		ids.send_phase);
	cucumber_reply();

	cucumber_wait(6, 3342191973u);
	enet_sync_server_desc_t desc = {
		.session = server.session, .options.schedule = SYNC_AUTOMATIC};
	cucumber_reply();

	cucumber_wait(7, 1973531821u);
	enet_sync_server_t *s = NULL;
	cucumber_reply();

	cucumber_wait(8, 478067478u);
	assert(enet_sync_server_listen(&desc, &s, NULL) == SYNC_ERR_STATE);
	cucumber_reply();

	cucumber_wait(9, 611655544u);
	assert(!s && !es_hosts_live);
	cucumber_reply();

	cucumber_wait(10, 2224299770u);
	world_close(&server);
	cucumber_reply();
}

#endif

#define CASE(name) {#name, name},
struct script { const char *name; void (*operations)(void); };
static const struct script scripts[] = {
CASE(invalid_calls)
CASE(initialized_outputs)
CASE(null_cleanup)
CASE(options)
CASE(connections)
CASE(snapshot_delta)
CASE(fragmented_updates)
CASE(packet_fragmentation)
CASE(rpg_trade_visibility)
CASE(reconnect)
CASE(automatic_phases)
#ifdef ES_FILE_SYNC
CASE(recording_replay)
CASE(recording_failure)
#endif
#ifdef ENET_SYNC_TESTING
CASE(outgoing_pressure)
CASE(outgoing_bytes)
CASE(per_peer_budgets)
CASE(client_pressure)
CASE(send_failure)
CASE(packet_failure)
CASE(host_failure)
CASE(open_failures)
CASE(malformed_packet)
CASE(core_peer_isolation)
CASE(retained_cleanup)
CASE(server_disconnect)
CASE(client_apply_failure)
CASE(poll_budget)
CASE(client_host_failure)
CASE(unreliable_packet)
CASE(peer_setup_failure)
CASE(client_frame_callback)
CASE(automatic_custom_budget)
CASE(empty_packet)
CASE(shared_world_ownership)
CASE(host_packet_limit)
CASE(failed_send_cancellation)
CASE(same_frame_publication)
CASE(pending_disconnect)
CASE(ordered_receive_disconnect)
CASE(phase_cycle)
#endif

};
#undef CASE

int main(void)
{
	char name[128];
	cucumber_begin(name, sizeof(name));
	for (unsigned i = 0; i < sizeof(scripts) / sizeof(*scripts); i++) {
		if (strcmp(name, scripts[i].name))
			continue;
		scripts[i].operations();
		cucumber_done();
		return 0;
	}
	fprintf(stderr, "unknown workflow: %s\n", name);
	return 1;
}
