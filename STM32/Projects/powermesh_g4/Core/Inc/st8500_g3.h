#ifndef ST8500_G3_H
#define ST8500_G3_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

#define ST8500_G3_RX_DMA_SIZE 2048U
#define ST8500_G3_FRAME_SIZE 1664U
#define ST8500_G3_MAX_IPV6_SIZE 1280U
#define ST8500_G3_TX_FRAME_SIZE (ST8500_G3_MAX_IPV6_SIZE + 17U)

typedef void (*st8500_g3_ipv6_callback_t)(
    const uint8_t *packet,
    uint16_t length,
    void *context
);

typedef enum
{
    ST8500_G3_STOPPED = 0,
    ST8500_G3_CONFIGURING,
    ST8500_G3_WAITING_FOR_NETWORK,
    ST8500_G3_NETWORK_READY,
    ST8500_G3_PHY_READY,
    ST8500_G3_FAILED
} st8500_g3_state_t;

typedef struct
{
    uint32_t packets;
    uint32_t bytes;
    uint32_t duration_ms;
} st8500_phy_stats_t;

typedef enum
{
    ST8500_PHY_RX_NO_FRAME = 0,
    ST8500_PHY_RX_HOST_ERROR,
    ST8500_PHY_RX_SHORT_HEADER,
    ST8500_PHY_RX_BAD_LENGTH,
    ST8500_PHY_RX_PATTERN_MISMATCH,
    ST8500_PHY_RX_MATCH
} st8500_phy_rx_check_t;

typedef struct
{
    uint8_t host_error;
    uint16_t data_length;
    uint16_t payload_length;
    uint8_t header_bytes[8];
    uint8_t header_bytes_length;
    uint8_t first_bytes[16];
    uint8_t first_bytes_length;
    uint16_t first_mismatch;
    st8500_phy_rx_check_t check;
} st8500_phy_rx_debug_t;

typedef struct
{
    UART_HandleTypeDef *uart;
    st8500_g3_state_t state;
    bool coordinator;
    bool phy_test;
    uint32_t phy_rx_indications;
    uint32_t phy_rx_matches;
    st8500_phy_rx_debug_t phy_rx_debug;
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

    st8500_g3_ipv6_callback_t on_ipv6;
    void *on_ipv6_context;

    uint8_t tx_frame[ST8500_G3_TX_FRAME_SIZE];
    uint8_t tx_handle;
    uint8_t next_handle;
    uint8_t last_tx_status;
    bool tx_pending;
    uint32_t tx_started_ms;

    uint32_t tx_requested;
    uint32_t tx_confirmed;
    uint32_t tx_failed;
    uint32_t tx_timeouts;
    uint32_t rx_packets;
    uint32_t rx_invalid;
} st8500_g3_t;

bool st8500_g3_start(
    st8500_g3_t *g3,
    UART_HandleTypeDef *uart,
    bool coordinator
);

void st8500_g3_poll(st8500_g3_t *g3);

void st8500_g3_set_receive_callback(
    st8500_g3_t *g3,
    st8500_g3_ipv6_callback_t callback,
    void *context
);

bool st8500_g3_send_ipv6(
    st8500_g3_t *g3,
    const uint8_t *packet,
    uint16_t length
);

bool st8500_g3_start_phy(
    st8500_g3_t *g3,
    UART_HandleTypeDef *uart
);

bool st8500_g3_phy_test_start(
    st8500_g3_t *g3,
    bool transmit
);

bool st8500_g3_phy_test_stats(
    st8500_g3_t *g3,
    st8500_phy_stats_t *stats
);

bool st8500_g3_phy_send_once(st8500_g3_t *g3);

bool st8500_g3_phy_set_tx(st8500_g3_t *g3);
bool st8500_g3_phy_set_rx(st8500_g3_t *g3);

#endif
