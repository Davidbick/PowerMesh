#ifndef ST8500_G3_H
#define ST8500_G3_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

#define ST8500_G3_RX_DMA_SIZE 2048U
#define ST8500_G3_FRAME_SIZE 1664U

typedef enum
{
    ST8500_G3_STOPPED = 0,
    ST8500_G3_CONFIGURING,
    ST8500_G3_WAITING_FOR_NETWORK,
    ST8500_G3_NETWORK_READY,
    ST8500_G3_FAILED
} st8500_g3_state_t;

typedef struct
{
    UART_HandleTypeDef *uart;
    st8500_g3_state_t state;
    bool coordinator;
    uint16_t pan_id;
    uint16_t short_addr;
    uint16_t last_joined_addr;
    uint32_t joined_devices;
    uint32_t crc_errors;
    uint32_t uart_errors;
    uint32_t command_timeouts;

    uint8_t rx_dma[ST8500_G3_RX_DMA_SIZE];
    uint16_t rx_read_pos;
    uint8_t frame[ST8500_G3_FRAME_SIZE];
    uint16_t frame_length;
    uint16_t expected_length;

    uint8_t expected_reply;
    bool waiting_reply;
    bool reply_received;
    uint8_t reply_error;
    uint8_t reply[64];
    uint16_t reply_length;
} st8500_g3_t;

bool st8500_g3_start(
    st8500_g3_t *g3,
    UART_HandleTypeDef *uart,
    bool coordinator
);

void st8500_g3_poll(st8500_g3_t *g3);

#endif
