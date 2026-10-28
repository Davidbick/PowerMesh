#include "uart_frame.h"

#include <string.h>

#define FRAME_SYNC_1 0xA5U
#define FRAME_SYNC_2 0x5AU
#define FRAME_HEADER_SIZE 6U
#define FRAME_CRC_SIZE 4U

static uint16_t read_be16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | data[1];
}

static uint32_t read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)value;
}

static void write_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static uint32_t crc32_add_byte(uint32_t crc, uint8_t byte)
{
    crc ^= byte;

    for (uint8_t bit = 0; bit < 8U; bit++)
    {
        crc = (crc & 1U) ?
            ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
    }

    return crc;
}

static void parser_reset(uart_frame_parser_t *parser)
{
    parser->state = UART_PARSER_SYNC_1;
    parser->header_index = 0U;
    parser->payload_index = 0U;
    parser->crc_index = 0U;
    parser->running_crc = 0xFFFFFFFFU;
}

void uart_frame_parser_init(uart_frame_parser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    parser_reset(parser);
}

bool uart_frame_parser_push(
    uart_frame_parser_t *parser,
    uint8_t byte
)
{
    if (parser == NULL)
    {
        return false;
    }

    switch (parser->state)
    {
        case UART_PARSER_SYNC_1:
            if (byte == FRAME_SYNC_1)
            {
                parser->state = UART_PARSER_SYNC_2;
            }
            break;

        case UART_PARSER_SYNC_2:
            if (byte == FRAME_SYNC_2)
            {
                parser->state = UART_PARSER_HEADER;
                parser->header_index = 0U;
                parser->running_crc = 0xFFFFFFFFU;
            }
            else if (byte != FRAME_SYNC_1)
            {
                parser_reset(parser);
            }
            break;

        case UART_PARSER_HEADER:
            parser->header[parser->header_index++] = byte;
            parser->running_crc = crc32_add_byte(
                parser->running_crc, byte
            );

            if (parser->header_index == FRAME_HEADER_SIZE)
            {
                parser->frame.version = parser->header[0];
                parser->frame.type = parser->header[1];
                parser->frame.sequence = read_be16(
                    &parser->header[2]
                );
                parser->frame.length = read_be16(
                    &parser->header[4]
                );

                if (parser->frame.version != UART_FRAME_VERSION ||
                    (parser->frame.type != UART_FRAME_TYPE_DATA &&
                     parser->frame.type != UART_FRAME_TYPE_ACK) ||
                    parser->frame.length > UART_FRAME_MAX_PAYLOAD)
                {
                    parser_reset(parser);
                    break;
                }

                parser->payload_index = 0U;
                parser->crc_index = 0U;
                parser->state = (parser->frame.length == 0U) ?
                    UART_PARSER_CRC : UART_PARSER_PAYLOAD;
            }
            break;

        case UART_PARSER_PAYLOAD:
            parser->frame.payload[parser->payload_index++] = byte;
            parser->running_crc = crc32_add_byte(
                parser->running_crc, byte
            );

            if (parser->payload_index == parser->frame.length)
            {
                parser->crc_index = 0U;
                parser->state = UART_PARSER_CRC;
            }
            break;

        case UART_PARSER_CRC:
            parser->crc_bytes[parser->crc_index++] = byte;

            if (parser->crc_index == FRAME_CRC_SIZE)
            {
                bool valid = read_be32(parser->crc_bytes) ==
                    (parser->running_crc ^ 0xFFFFFFFFU);

                parser_reset(parser);
                return valid;
            }
            break;
    }

    return false;
}

size_t uart_frame_encode(
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length,
    uint8_t *output,
    size_t output_capacity
)
{
    size_t frame_size = UART_FRAME_OVERHEAD + (size_t)length;

    if (output == NULL || frame_size > output_capacity ||
        length > UART_FRAME_MAX_PAYLOAD ||
        (length > 0U && payload == NULL) ||
        (type != UART_FRAME_TYPE_DATA &&
         type != UART_FRAME_TYPE_ACK))
    {
        return 0U;
    }

    output[0] = FRAME_SYNC_1;
    output[1] = FRAME_SYNC_2;
    output[2] = UART_FRAME_VERSION;
    output[3] = type;
    write_be16(&output[4], sequence);
    write_be16(&output[6], length);

    if (length > 0U)
    {
        memcpy(&output[8], payload, length);
    }

    uint32_t crc = 0xFFFFFFFFU;

    for (size_t i = 2U; i < 8U + length; i++)
    {
        crc = crc32_add_byte(crc, output[i]);
    }

    write_be32(
        &output[8U + length],
        crc ^ 0xFFFFFFFFU
    );

    return frame_size;
}
