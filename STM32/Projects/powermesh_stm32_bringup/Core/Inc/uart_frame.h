#ifndef UART_FRAME_H
#define UART_FRAME_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define UART_FRAME_VERSION      0x01U
#define UART_FRAME_TYPE_DATA    0x01U
#define UART_FRAME_TYPE_ACK     0x02U

#define UART_FRAME_MAX_PAYLOAD  1280U

typedef struct
{
    uint8_t version;
    uint8_t type;
    uint16_t sequence;
    uint16_t length;
    uint8_t payload[UART_FRAME_MAX_PAYLOAD];
} uart_frame_t;

HAL_StatusTypeDef uart_frame_receive(
    UART_HandleTypeDef *huart,
    uart_frame_t *frame
);

HAL_StatusTypeDef uart_frame_send(
    UART_HandleTypeDef *huart,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
);

#endif