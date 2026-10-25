#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_FRAME_MAX_PAYLOAD 1280

#define UART_FRAME_TYPE_DATA 1
#define UART_FRAME_TYPE_ACK  2

typedef void (*uart_frame_callback_t)(
    uint8_t frame_type,
    uint16_t sequence,
    const uint8_t *payload,
    size_t length,
    void *context
);

typedef struct {
    uint32_t frames_ok;
    uint32_t crc_errors;
    uint32_t invalid_headers;
    uint32_t invalid_lengths;
} uart_frame_stats_t;

esp_err_t uart_frame_uart_start(
    int uart_num,
    int tx_gpio,
    int rx_gpio
);

esp_err_t uart_frame_parser_start(
    uart_frame_callback_t callback,
    void *context
);

void uart_frame_get_stats(uart_frame_stats_t *stats);

esp_err_t uart_frame_send(
    uint8_t type,
    uint16_t seq,
    const uint8_t *payload,
    uint16_t len
);

#ifdef __cplusplus
}
#endif
