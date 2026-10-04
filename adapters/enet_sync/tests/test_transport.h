#ifndef ES_TEST_TRANSPORT_H
#define ES_TEST_TRANSPORT_H
#include <enet/enet.h>

/* Private deterministic wrappers; ENet sources are never modified. */
extern int es_fail_host, es_fail_connect, es_fail_service;
extern int es_fail_send, es_fail_packet;
extern int es_protocol_services;
extern int es_hosts_live;
extern int es_hold_packets;
ENetHost *es_host_create(const ENetAddress *, size_t, size_t,
	enet_uint32, enet_uint32);
void es_host_destroy(ENetHost *);
int es_host_service(ENetHost *, ENetEvent *, enet_uint32);
void es_host_flush(ENetHost *);
ENetPeer *es_host_connect(ENetHost *, const ENetAddress *, size_t, enet_uint32);
void es_peer_disconnect_now(ENetPeer *, enet_uint32);
int es_peer_send(ENetPeer *, enet_uint8, ENetPacket *);
ENetPacket *es_packet_create(const void *, size_t, enet_uint32);
void es_release_packets(void);
void es_inject_client(const void *, size_t, enet_uint8, enet_uint32);
#endif
