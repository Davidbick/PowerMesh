#include "packet_queue.h"

#include <string.h>

void packet_queue_init(packet_queue_t *queue)
{
    if (queue != NULL)
    {
        memset(queue, 0, sizeof(*queue));
    }
}

bool packet_queue_push(packet_queue_t *queue, const uart_frame_t *frame)
{
    if (queue == NULL || frame == NULL ||
        frame->type != UART_FRAME_TYPE_DATA ||
        frame->length > UART_FRAME_MAX_PAYLOAD ||
        queue->count >= PACKET_QUEUE_DEPTH)
    {
        return false;
    }

    packet_queue_item_t *item = &queue->items[queue->tail];
    item->sequence = frame->sequence;
    item->length = frame->length;

    if (frame->length > 0U)
    {
        memcpy(item->payload, frame->payload, frame->length);
    }

    queue->tail = (uint8_t)((queue->tail + 1U) % PACKET_QUEUE_DEPTH);
    queue->count++;
    return true;
}

const packet_queue_item_t *packet_queue_peek(const packet_queue_t *queue)
{
    if (queue == NULL || queue->count == 0U)
    {
        return NULL;
    }

    return &queue->items[queue->head];
}

void packet_queue_pop(packet_queue_t *queue)
{
    if (queue == NULL || queue->count == 0U)
    {
        return;
    }

    queue->head = (uint8_t)((queue->head + 1U) % PACKET_QUEUE_DEPTH);
    queue->count--;
}
