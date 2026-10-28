#ifndef PACKET_ROUTER_H
#define PACKET_ROUTER_H

#include <stdbool.h>
#include <stdint.h>

#include "packet_queue.h"

typedef enum
{
    PACKET_PORT_GATEWAY = 0,
    PACKET_PORT_REMOTE = 1,
    PACKET_PORT_COUNT = 2
} packet_port_t;

#define PACKET_ACK_ACCEPTED 0U
#define PACKET_ACK_QUEUE_FULL 1U

typedef bool (*packet_router_send_fn)(
    void *context,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
);

typedef struct
{
    packet_router_send_fn send;
    void *context;
} packet_router_port_t;

typedef struct
{
    packet_router_port_t ports[PACKET_PORT_COUNT];
    packet_queue_t outgoing[PACKET_PORT_COUNT];

    uint32_t received[PACKET_PORT_COUNT];
    uint32_t forwarded[PACKET_PORT_COUNT];
    uint32_t dropped[PACKET_PORT_COUNT];
    uint32_t ack_received[PACKET_PORT_COUNT];
    uint32_t ack_send_failures[PACKET_PORT_COUNT];
} packet_router_t;

bool packet_router_init(
    packet_router_t *router,
    packet_router_send_fn gateway_send,
    void *gateway_context,
    packet_router_send_fn remote_send,
    void *remote_context
);

// Returns true if a DATA packet was accepted for forwarding.
bool packet_router_receive(
    packet_router_t *router,
    packet_port_t source,
    const uart_frame_t *frame
);

void packet_router_poll(packet_router_t *router);

#endif
