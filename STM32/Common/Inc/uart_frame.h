#ifndef UART_FRAME_H
#define UART_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UART_FRAME_VERSION       0x01U
#define UART_FRAME_TYPE_DATA     0x01U
#define UART_FRAME_TYPE_ACK      0x02U
#define UART_FRAME_MAX_PAYLOAD   1280U
#define UART_FRAME_OVERHEAD      12U
#define UART_FRAME_MAX_SIZE      (UART_FRAME_MAX_PAYLOAD + UART_FRAME_OVERHEAD)

typedef struct
{
    uint8_t version;
    uint8_t type;
    uint16_t sequence;
    uint16_t length;
    uint8_t payload[UART_FRAME_MAX_PAYLOAD];
} uart_frame_t;

typedef enum
{
    UART_PARSER_SYNC_1,
    UART_PARSER_SYNC_2,
    UART_PARSER_HEADER,
    UART_PARSER_PAYLOAD,
    UART_PARSER_CRC
} uart_frame_parser_state_t;

typedef struct
{
    uart_frame_t frame;
    uart_frame_parser_state_t state;
    uint8_t header[6];
    uint8_t crc_bytes[4];
    uint8_t header_index;
    uint8_t crc_index;
    uint16_t payload_index;
    uint32_t running_crc;
} uart_frame_parser_t;

void uart_frame_parser_init(uart_frame_parser_t *parser);

bool uart_frame_parser_push(
    uart_frame_parser_t *parser,
    uint8_t byte
);

size_t uart_frame_encode(
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length,
    uint8_t *output,
    size_t output_capacity
);

#endif
