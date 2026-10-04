#include "test_transport.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int es_fail_host, es_fail_connect, es_fail_service;
int es_fail_send, es_fail_packet, es_protocol_services, es_hosts_live;
int es_hold_packets;

struct queued {
	ENetEvent event;
	struct queued *next;
};
struct held {
	ENetPeer *peer;
	ENetPacket *packet;
	struct held *next;
};
struct host {
	ENetHost enet;
	ENetPeer **remote;
	struct queued *head, *tail;
	struct held *held;
	struct host *next;
};
static struct host *hosts;
static enet_uint16 next_port = 10000;

static void enqueue(ENetPeer *peer, ENetEventType type, ENetPacket *packet,
	enet_uint32 data, enet_uint8 channel)
{
	struct host *h = (struct host *)peer->host;
	struct queued *q = calloc(1, sizeof(*q));
	assert(q);
	q->event = (ENetEvent){ .type = type, .peer = peer,
		.packet = packet, .data = data, .channelID = channel };
	if (h->tail)
		h->tail->next = q;
	else
		h->head = q;
	h->tail = q;
}

ENetHost *es_host_create(const ENetAddress *address, size_t count,
	size_t channels, enet_uint32 incoming, enet_uint32 outgoing)
{
	(void)channels;
	(void)incoming;
	(void)outgoing;
	if (es_fail_host) {
		es_fail_host = 0;
		return NULL;
	}
	struct host *h = calloc(1, sizeof(*h));
	assert(h);
	h->enet.peers = calloc(count, sizeof(ENetPeer));
	h->remote = calloc(count, sizeof(ENetPeer *));
	assert(h->enet.peers && h->remote);
	h->enet.peerCount = count;
	if (address)
		h->enet.address = *address;
	if (!h->enet.address.port)
		h->enet.address.port = next_port++;
	for (size_t i = 0; i < count; i++) {
		h->enet.peers[i].host = &h->enet;
		h->enet.peers[i].channelCount = channels;
		h->enet.peers[i].incomingPeerID = (enet_uint16)i;
	}
	h->next = hosts;
	hosts = h;
	es_hosts_live++;
	return &h->enet;
}

static void release(struct host *h, ENetPeer *peer)
{
	struct held **link = &h->held;
	while (*link) {
		struct held *item = *link;
		if (peer && item->peer != peer) {
			link = &item->next;
			continue;
		}
		*link = item->next;
		item->packet->referenceCount = 0;
		enet_packet_destroy(item->packet);
		free(item);
	}
}

void es_release_packets(void)
{
	for (struct host *h = hosts; h; h = h->next)
		release(h, NULL);
}

void es_host_destroy(ENetHost *host)
{
	struct host *h = (struct host *)host;
	for (struct host *other = hosts; other; other = other->next) {
		for (size_t i = 0; i < other->enet.peerCount; i++) {
			if (other->remote[i] && other->remote[i]->host == host)
				other->remote[i] = NULL;
		}
	}
	release(h, NULL);
	while (h->head) {
		struct queued *q = h->head;
		h->head = q->next;
		if (q->event.packet)
			enet_packet_destroy(q->event.packet);
		free(q);
	}
	struct host **link = &hosts;
	while (*link != h)
		link = &(*link)->next;
	*link = h->next;
	free(h->enet.peers);
	free(h->remote);
	free(h);
	es_hosts_live--;
}

int es_host_service(ENetHost *host, ENetEvent *event, enet_uint32 timeout)
{
	(void)timeout;
	if (!es_hold_packets)
		release((struct host *)host, NULL);
	if (es_fail_service) {
		es_fail_service = 0;
		return -1;
	}
	es_protocol_services++;
	if (!event) {
		return 0;
	}
	struct host *h = (struct host *)host;
	if (!h->head)
		return 0;
	struct queued *q = h->head;
	h->head = q->next;
	if (!h->head)
		h->tail = NULL;
	*event = q->event;
	free(q);
	return 1;
}

void es_host_flush(ENetHost *host)
{
	if (!es_hold_packets)
		release((struct host *)host, NULL);
}

ENetPeer *es_host_connect(ENetHost *host, const ENetAddress *address,
	size_t channels, enet_uint32 data)
{
	(void)channels;
	if (es_fail_connect) {
		es_fail_connect = 0;
		return NULL;
	}
	struct host *h = (struct host *)host;
	ENetPeer *peer = &host->peers[0];
	peer->address = *address;
	for (struct host *server = hosts; server; server = server->next) {
		if (server == h || server->enet.address.port != address->port)
			continue;
		for (size_t i = 0; i < server->enet.peerCount; i++) {
			ENetPeer *remote = &server->enet.peers[i];
			if (remote->state == ENET_PEER_STATE_CONNECTED)
				continue;
			remote->state = peer->state = ENET_PEER_STATE_CONNECTED;
			remote->address = host->address;
			server->remote[i] = peer;
			h->remote[0] = remote;
			enqueue(remote, ENET_EVENT_TYPE_CONNECT, NULL, data, 0);
			enqueue(peer, ENET_EVENT_TYPE_CONNECT, NULL, 0, 0);
			return peer;
		}
	}
	/* Like real ENet, an unavailable endpoint still connects asynchronously. */
	return peer;
}

void es_peer_disconnect_now(ENetPeer *peer, enet_uint32 data)
{
	struct host *h = (struct host *)peer->host;
	ENetPeer *remote = h->remote[peer->incomingPeerID];
	if (remote) {
		enqueue(remote, ENET_EVENT_TYPE_DISCONNECT, NULL, data, 0);
		struct host *rh = (struct host *)remote->host;
		rh->remote[remote->incomingPeerID] = NULL;
		h->remote[peer->incomingPeerID] = NULL;
	}
	release(h, peer);
	peer->state = ENET_PEER_STATE_DISCONNECTED;
}

int es_peer_send(ENetPeer *peer, enet_uint8 channel, ENetPacket *packet)
{
	if (es_fail_send) {
		es_fail_send = 0;
		return -1;
	}
	struct host *h = (struct host *)peer->host;
	ENetPeer *remote = h->remote[peer->incomingPeerID];
	assert(remote && channel == 0);
	assert(packet->flags == ENET_PACKET_FLAG_RELIABLE);
	ENetPacket *copy = enet_packet_create(packet->data, packet->dataLength,
		packet->flags);
	assert(copy);
	enqueue(remote, ENET_EVENT_TYPE_RECEIVE, copy, 0, channel);
	struct held *item = calloc(1, sizeof(*item));
	assert(item);
	item->peer = peer;
	item->packet = packet;
	item->next = h->held;
	h->held = item;
	packet->referenceCount++;
	return 0;
}

ENetPacket *es_packet_create(const void *data, size_t size, enet_uint32 flags)
{
	if (es_fail_packet) {
		es_fail_packet = 0;
		return NULL;
	}
	return enet_packet_create(data, size, flags);
}

void es_inject_client(const void *data, size_t size, enet_uint8 channel,
	enet_uint32 flags)
{
	assert(hosts);
	ENetPacket *packet = enet_packet_create(data, size, flags);
	assert(packet);
	enqueue(&hosts->enet.peers[0], ENET_EVENT_TYPE_RECEIVE, packet, 0, channel);
}

void es_peer_reset(ENetPeer *peer)
{
	es_peer_disconnect_now(peer, 0);
}
