#include "packet_router.h"

#include <string.h>

static bool valid_port(packet_port_t port)
{
    return port == PACKET_PORT_GATEWAY || port == PACKET_PORT_REMOTE;
}

bool packet_router_init(
    packet_router_t *router,
    packet_router_send_fn gateway_send,
    void *gateway_context,
    packet_router_send_fn remote_send,
    void *remote_context
)
{
    if (router == NULL || gateway_send == NULL || remote_send == NULL)
    {
        return false;
    }

    memset(router, 0, sizeof(*router));
    router->ports[PACKET_PORT_GATEWAY].send = gateway_send;
    router->ports[PACKET_PORT_GATEWAY].context = gateway_context;
    router->ports[PACKET_PORT_REMOTE].send = remote_send;
    router->ports[PACKET_PORT_REMOTE].context = remote_context;

    packet_queue_init(&router->outgoing[PACKET_PORT_GATEWAY]);
    packet_queue_init(&router->outgoing[PACKET_PORT_REMOTE]);
    return true;
}

bool packet_router_receive(
    packet_router_t *router,
    packet_port_t source,
    const uart_frame_t *frame
)
{
    if (router == NULL || frame == NULL || !valid_port(source))
    {
        return false;
    }

    if (frame->type == UART_FRAME_TYPE_ACK)
    {
        // ACKs are link-local and must not be forwarded as IPv6 data.
        router->ack_received[source]++;
        return false;
    }

    if (frame->type != UART_FRAME_TYPE_DATA ||
        frame->length > UART_FRAME_MAX_PAYLOAD)
    {
        return false;
    }

    router->received[source]++;

    packet_port_t destination = (source == PACKET_PORT_GATEWAY) ?
        PACKET_PORT_REMOTE : PACKET_PORT_GATEWAY;

    bool accepted = packet_queue_push(&router->outgoing[destination], frame);
    uint8_t status = accepted ? PACKET_ACK_ACCEPTED : PACKET_ACK_QUEUE_FULL;

    if (!accepted)
    {
        router->dropped[source]++;
    }

    // This ACK confirms queue acceptance, not remote delivery.
    if (!router->ports[source].send(
            router->ports[source].context,
            UART_FRAME_TYPE_ACK,
            frame->sequence,
            &status,
            sizeof(status)))
    {
        router->ack_send_failures[source]++;
    }

    return accepted;
}

void packet_router_poll(packet_router_t *router)
{
    if (router == NULL)
    {
        return;
    }

    for (uint8_t port = 0U; port < PACKET_PORT_COUNT; port++)
    {
        packet_queue_t *queue = &router->outgoing[port];
        const packet_queue_item_t *item = packet_queue_peek(queue);

        if (item == NULL)
        {
            continue;
        }

        // Retry on the next poll if the UART TX queue is full.
        if (router->ports[port].send(
                router->ports[port].context,
                UART_FRAME_TYPE_DATA,
                item->sequence,
                item->payload,
                item->length))
        {
            router->forwarded[port]++;
            packet_queue_pop(queue);
        }
    }
}
