#ifndef UART_TRANSPORT_H
#define UART_TRANSPORT_H

#include <stdint.h>

#if defined(STM32F446xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32G474xx)
#include "stm32g4xx_hal.h"
#else
#error "Unsupported STM32 target for UART transport"
#endif
#include "uart_frame.h"

#define UART_TRANSPORT_RX_DMA_SIZE       2048U
#define UART_TRANSPORT_TX_QUEUE_DEPTH   4U

typedef void (*uart_transport_frame_callback_t)(
    const uart_frame_t *frame,
    void *context
);

typedef struct
{
    uint16_t length;
    uint8_t bytes[UART_FRAME_MAX_SIZE];
} uart_transport_tx_item_t;

typedef struct
{
    UART_HandleTypeDef *huart;
    uart_transport_frame_callback_t on_frame;
    void *context;

    uart_frame_parser_t parser;
    uint8_t rx_dma[UART_TRANSPORT_RX_DMA_SIZE];
    uint16_t rx_read_pos;
    volatile uint8_t rx_restart_requested;

    uart_transport_tx_item_t tx_queue[UART_TRANSPORT_TX_QUEUE_DEPTH];
    uint8_t tx_head;
    uint8_t tx_tail;
    uint8_t tx_count;
    uint8_t tx_active;
    volatile uint8_t tx_complete;

    uint32_t rx_errors;
    uint32_t tx_errors;
} uart_transport_t;

HAL_StatusTypeDef uart_transport_start(
    uart_transport_t *transport,
    UART_HandleTypeDef *huart,
    uart_transport_frame_callback_t on_frame,
    void *context
);

void uart_transport_poll(uart_transport_t *transport);

HAL_StatusTypeDef uart_transport_send(
    uart_transport_t *transport,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
);

#endif
