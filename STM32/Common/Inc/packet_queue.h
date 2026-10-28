#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

#include "uart_frame.h"

#define PACKET_QUEUE_DEPTH 2U

typedef struct
{
    uint16_t sequence;
    uint16_t length;
    uint8_t payload[UART_FRAME_MAX_PAYLOAD];
} packet_queue_item_t;

typedef struct
{
    packet_queue_item_t items[PACKET_QUEUE_DEPTH];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} packet_queue_t;

void packet_queue_init(packet_queue_t *queue);
bool packet_queue_push(packet_queue_t *queue, const uart_frame_t *frame);
const packet_queue_item_t *packet_queue_peek(const packet_queue_t *queue);
void packet_queue_pop(packet_queue_t *queue);

#endif
