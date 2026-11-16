#include "st8500_g3.h"
#include "powermesh_board.h"

#include <stdio.h>
#include <string.h>

#define G3_SYNC                  0x16U
#define G3_HEADER_SIZE           10U
#define G3_FRAME_OVERHEAD        12U
#define G3_CMD_TIMEOUT_MS        3000U
#define G3_BOOT_WAIT_MS          250U

#define G3_DBGTOOL_REQ           0x14U
#define G3_DBGTOOL_CONF          0x15U
#define G3_LIB_SET_REQ           0x22U
#define G3_LIB_SET_CONF          0x23U
#define G3_LIB_SWRESET_REQ       0x24U
#define G3_LIB_SWRESET_CONF      0x25U
#define G3_ADP_DATA_REQ         0x80U
#define G3_ADP_DATA_CONF        0x81U
#define G3_ADP_DATA_IND         0x82U
#define G3_DATA_TIMEOUT_MS      30000U
#define G3_BOOT_SRV_START_REQ    0xB0U
#define G3_BOOT_SRV_START_CONF   0xB1U
#define G3_BOOT_SRV_STOP_REQ     0xB2U
#define G3_BOOT_SRV_STOP_CONF    0xB3U
#define G3_BOOT_SRV_JOIN_IND     0xB7U
#define G3_BOOT_DEV_START_CONF   0xB9U
#define G3_BOOT_DEV_LEAVE_IND    0xBCU
#define G3_BOOT_SRV_GETPSK_IND   0xC4U
#define G3_BOOT_SRV_SETPSK_REQ   0xC5U
#define G3_BOOT_SRV_SETPSK_CONF  0xC6U
#define G3_ERROR_IND            0xFFU

#define G3_MODE_BOOT            0x03U
#define G3_BAND_CEN_A           0x00U
#define G3_ROLE_DEVICE          0x00U
#define G3_ROLE_COORDINATOR     0x01U
#define G3_PAN_ID               0x781DU
#define G3_DEVICE_SHORT_ADDR    0x0001U
#define G3_PIB_PAN_ID           0x00000050UL
#define G3_PIB_SHORT_ADDR       0x00000053UL
#define G3_PIB_KEY_TABLE        0x00000071UL
#define G3_PIB_ACTIVE_KEY       0x00000022UL
#define G3_PIB_EAP_PSK          0x00000094UL
#define G3_PIB_COORD_ADDR       0x00000008UL
#define G3_PIB_CHANNEL_CONFIG   0x00003202UL
#define G3_CHANNEL_PLC_ONLY     0x00000003UL

static const uint8_t g3_psk[16] = {
    0xABU, 0x10U, 0x34U, 0x11U, 0x45U, 0x11U, 0x1BU, 0xC3U,
    0xC1U, 0x2DU, 0xE8U, 0xFFU, 0x11U, 0x14U, 0x22U, 0x04U
};

static const uint8_t g3_gmk[16] = {
    0xAFU, 0x4DU, 0x6DU, 0xCCU, 0xF1U, 0x4DU, 0xE7U, 0xC1U,
    0xC4U, 0x23U, 0x5EU, 0x6FU, 0xEFU, 0x6CU, 0x15U, 0x1FU
};

static void g3_log(const char *message)
{
#if defined(POWERMESH_ROLE_ENDPOINT)
    char line[100];
    int length = snprintf(line, sizeof(line), "[G3] %s\r\n", message);

    if (length > 0 && (size_t)length < sizeof(line))
    {
        (void)HAL_UART_Transmit(
            POWERMESH_DEBUG_UART,
            (uint8_t *)line,
            (uint16_t)length,
            100U
        );
    }
#else
    (void)message;
#endif
}

static void g3_log_number(const char *message, uint32_t value)
{
#if defined(POWERMESH_ROLE_ENDPOINT)
    char line[80];
    int length = snprintf(
        line,
        sizeof(line),
        "%s: %lu",
        message,
        (unsigned long)value
    );

    if (length > 0 && (size_t)length < sizeof(line))
    {
        g3_log(line);
    }
#else
    (void)message;
    (void)value;
#endif
}

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    for (uint8_t i = 0U; i < 4U; i++)
    {
        data[i] = (uint8_t)(value >> (8U * i));
    }
}

static uint16_t crc16_xmodem(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0U;

    for (uint16_t i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = (crc & 0x8000U) != 0U ?
                (uint16_t)((crc << 1) ^ 0x1021U) :
                (uint16_t)(crc << 1);
        }
    }

    return crc;
}

static bool valid_ipv6_packet(const uint8_t *packet, uint16_t length)
{
    if (packet == NULL || length < 40U ||
        length > ST8500_G3_MAX_IPV6_SIZE ||
        (packet[0] >> 4) != 6U)
    {
        return false;
    }

    uint16_t payload_length =
        ((uint16_t)packet[4] << 8) | packet[5];

    return (uint32_t)payload_length + 40U == length;
}

static void handle_adp_data_indication(
    st8500_g3_t *g3,
    const uint8_t *data,
    uint16_t length
)
{
    if (length < 14U)
    {
        g3->rx_invalid++;
        return;
    }

    uint16_t nsdu_length = read_u16(data);
    uint8_t address_mode = data[2];
    uint16_t index = 5U;

    if (address_mode == 2U)
    {
        index += 2U;
    }
    else if (address_mode == 3U)
    {
        index += 8U;
    }
    else if (address_mode != 0U)
    {
        g3->rx_invalid++;
        return;
    }

    // LQI, phase, modulation type, scheme and 3-byte tone map.
    index += 7U;

    if (index >= length)
    {
        g3->rx_invalid++;
        return;
    }

    uint8_t bc0_count = data[index++];

    if (bc0_count > 6U ||
        (uint32_t)index + bc0_count + 1U > length)
    {
        g3->rx_invalid++;
        return;
    }

    // Skip the BC0 list and previous-hop medium.
    index = (uint16_t)(index + bc0_count + 1U);

    if (nsdu_length != (uint16_t)(length - index) ||
        !valid_ipv6_packet(&data[index], nsdu_length))
    {
        g3->rx_invalid++;
        return;
    }

    g3->rx_packets++;

    if (g3->on_ipv6 != NULL)
    {
        g3->on_ipv6(
            &data[index],
            nsdu_length,
            g3->on_ipv6_context
        );
    }
}

static bool send_frame(
    st8500_g3_t *g3,
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length
);

static void handle_frame(st8500_g3_t *g3)
{
    const uint8_t *frame = g3->frame;
    uint8_t command = frame[2];
    uint16_t payload_length = read_u16(&frame[3]);
    uint8_t error = payload_length != 0U ? frame[10] : 0xFFU;
    const uint8_t *payload = &frame[11];
    uint16_t data_length = payload_length != 0U ?
        payload_length - 1U : 0U;

    if (command == G3_BOOT_SRV_START_CONF && g3->coordinator &&
        error == 0U && data_length >= 1U)
    {
        if (payload[0] == 0U)
        {
            g3->pan_id = G3_PAN_ID;
            g3->short_addr = 0U;
            g3->state = ST8500_G3_NETWORK_READY;
            g3_log("Coordinator network started");
        }
        else
        {
            g3->state = ST8500_G3_FAILED;
            g3_log_number("Coordinator start status", payload[0]);
        }
    }
    else if (command == G3_BOOT_DEV_START_CONF && !g3->coordinator &&
             error == 0U && data_length >= 5U)
    {
        if (payload[0] == 0U)
        {
            g3->short_addr = read_u16(&payload[1]);
            g3->pan_id = read_u16(&payload[3]);
            g3->state = ST8500_G3_NETWORK_READY;

            g3_log("Device joined G3 network");
            g3_log_number("PAN ID", g3->pan_id);
            g3_log_number("Short address", g3->short_addr);
        }
        else
        {
            g3_log_number("Device join status", payload[0]);
        }
    }
    else if (command == G3_BOOT_SRV_JOIN_IND && g3->coordinator &&
             error == 0U && data_length >= 10U)
    {
        g3->last_joined_addr = read_u16(&payload[8]);
        g3->joined_devices++;

        g3_log_number(
            "Device joined, short address",
            g3->last_joined_addr
        );
    }
    else if (command == G3_BOOT_SRV_GETPSK_IND && g3->coordinator &&
             error == 0U && data_length >= 9U)
    {
        uint8_t response[26];

        memcpy(response, payload, 8U);
        memcpy(&response[8], g3_psk, sizeof(g3_psk));
        write_u16(&response[24], G3_DEVICE_SHORT_ADDR);

        if (send_frame(
                g3,
                G3_BOOT_SRV_SETPSK_REQ,
                response,
                sizeof(response)))
        {
            g3_log("Responded to device PSK request");
        }
        else
        {
            g3_log("Failed to send device PSK");
        }
    }
    else if (command == G3_BOOT_SRV_SETPSK_CONF && g3->coordinator &&
             error == 0U && data_length >= 1U && payload[0] != 0U)
    {
        g3_log_number("PSK confirmation error", payload[0]);
    }
    else if (command == G3_BOOT_DEV_LEAVE_IND && !g3->coordinator)
    {
        g3->state = ST8500_G3_WAITING_FOR_NETWORK;
        g3_log("Device left G3 network");
    }
    else if (command == G3_ADP_DATA_CONF && g3->tx_pending)
    {
        if (error != 0U)
        {
            g3->tx_pending = false;
            g3->last_tx_status = error;
            g3->tx_failed++;
            g3_log_number("G3 data interface error", error);
        }
        else if (data_length >= 3U &&
                 payload[1] == g3->tx_handle)
        {
            g3->tx_pending = false;
            g3->last_tx_status = payload[0];

            if (payload[0] == 0U)
            {
                g3->tx_confirmed++;
            }
            else
            {
                g3->tx_failed++;
                g3_log_number("G3 data TX failed", payload[0]);
            }
        }
    }
    else if (command == G3_ADP_DATA_IND)
    {
        if (error == 0U)
        {
            handle_adp_data_indication(
                g3,
                payload,
                data_length
            );
        }
        else
        {
            g3->rx_invalid++;
        }
    }
    else if (command == G3_ERROR_IND)
    {
        g3_log_number("G3 error indication", error);
    }

    if (g3->waiting_reply && command == g3->expected_reply)
    {
        g3->reply_error = error;
        g3->reply_length = data_length > sizeof(g3->reply) ?
            sizeof(g3->reply) : data_length;

        memcpy(g3->reply, payload, g3->reply_length);
        g3->reply_received = true;
    }
}

static void receive_byte(st8500_g3_t *g3, uint8_t byte)
{
    if (g3->frame_length == 0U && byte != G3_SYNC)
    {
        return;
    }

    if (g3->frame_length == 1U && byte != G3_SYNC)
    {
        g3->frame_length = 0U;
        return;
    }

    g3->frame[g3->frame_length++] = byte;

    if (g3->frame_length == G3_HEADER_SIZE)
    {
        uint16_t payload_length = read_u16(&g3->frame[3]);
        uint32_t total = G3_FRAME_OVERHEAD + (uint32_t)payload_length;

        if (total > ST8500_G3_FRAME_SIZE ||
            payload_length == 0U || g3->frame[5] != 0U)
        {
            g3->frame_length = 0U;
            g3->expected_length = 0U;
            return;
        }

        g3->expected_length = (uint16_t)total;
    }

    if (g3->expected_length != 0U &&
        g3->frame_length == g3->expected_length)
    {
        uint16_t calculated = crc16_xmodem(
            g3->frame,
            (uint16_t)(g3->frame_length - 2U)
        );

        uint16_t received = read_u16(
            &g3->frame[g3->frame_length - 2U]
        );

        if (calculated == received)
        {
            handle_frame(g3);
        }
        else
        {
            g3->crc_errors++;
        }

        g3->frame_length = 0U;
        g3->expected_length = 0U;
    }
}

void st8500_g3_poll(st8500_g3_t *g3)
{
    if (g3 == NULL || g3->uart == NULL)
    {
        return;
    }

    if (g3->uart->RxState != HAL_UART_STATE_BUSY_RX)
    {
        g3->uart_errors++;

        if (HAL_UART_AbortReceive(g3->uart) != HAL_OK)
        {
            return;
        }

        g3->rx_read_pos = 0U;
        g3->frame_length = 0U;
        g3->expected_length = 0U;

        if (HAL_UART_Receive_DMA(
                g3->uart,
                g3->rx_dma,
                sizeof(g3->rx_dma)) != HAL_OK)
        {
            return;
        }

        __HAL_DMA_DISABLE_IT(g3->uart->hdmarx, DMA_IT_HT);
    }

    uint16_t write_pos = (uint16_t)(
        ST8500_G3_RX_DMA_SIZE -
        __HAL_DMA_GET_COUNTER(g3->uart->hdmarx)
    );

    if (write_pos == ST8500_G3_RX_DMA_SIZE)
    {
        write_pos = 0U;
    }

    while (g3->rx_read_pos != write_pos)
    {
        uint8_t byte = g3->rx_dma[g3->rx_read_pos];

        g3->rx_read_pos = (uint16_t)(
            (g3->rx_read_pos + 1U) % ST8500_G3_RX_DMA_SIZE
        );

        receive_byte(g3, byte);
    }

    if (g3->tx_pending &&
        (uint32_t)(HAL_GetTick() - g3->tx_started_ms) >=
            G3_DATA_TIMEOUT_MS)
    {
        g3->tx_pending = false;
        g3->tx_timeouts++;
        g3_log("G3 data confirmation timeout");
    }
}

static bool send_frame(
    st8500_g3_t *g3,
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    uint8_t frame[80] = {G3_SYNC, G3_SYNC};
    uint16_t length = (uint16_t)(G3_FRAME_OVERHEAD + payload_length);

    if (length > sizeof(frame) ||
        (payload_length != 0U && payload == NULL))
    {
        return false;
    }

    frame[2] = command;
    write_u16(&frame[3], payload_length);

    if (payload_length != 0U)
    {
        memcpy(&frame[G3_HEADER_SIZE], payload, payload_length);
    }

    uint16_t crc = crc16_xmodem(frame, length - 2U);
    write_u16(&frame[length - 2U], crc);

    return HAL_UART_Transmit(
        g3->uart,
        frame,
        length,
        1000U
    ) == HAL_OK;
}

static bool send_request(
    st8500_g3_t *g3,
    uint8_t command,
    uint8_t confirmation,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    g3->expected_reply = confirmation;
    g3->reply_received = false;
    g3->reply_length = 0U;
    g3->waiting_reply = true;

    if (!send_frame(g3, command, payload, payload_length))
    {
        g3->waiting_reply = false;
        g3_log_number("G3 UART TX failed, command", command);
        return false;
    }

    uint32_t started = HAL_GetTick();

    while ((uint32_t)(HAL_GetTick() - started) < G3_CMD_TIMEOUT_MS)
    {
        st8500_g3_poll(g3);

        if (g3->reply_received)
        {
            g3->waiting_reply = false;

            if (g3->reply_error != 0U)
            {
                g3_log_number("G3 command error", g3->reply_error);
                return false;
            }

            return true;
        }

        HAL_Delay(1U);
    }

    g3->waiting_reply = false;
    g3->command_timeouts++;
    g3_log_number("G3 command timeout", command);
    return false;
}

static bool set_attribute(
    st8500_g3_t *g3,
    uint32_t id,
    uint16_t index,
    const uint8_t *value,
    uint16_t length
)
{
    uint8_t payload[24] = {0};

    if (length > 16U || (length != 0U && value == NULL))
    {
        return false;
    }

    write_u32(&payload[0], id);
    write_u16(&payload[4], index);
    write_u16(&payload[6], length);

    if (length != 0U)
    {
        memcpy(&payload[8], value, length);
    }

    if (!send_request(
            g3,
            G3_LIB_SET_REQ,
            G3_LIB_SET_CONF,
            payload,
            (uint16_t)(8U + length)) ||
        g3->reply_length < 7U ||
        g3->reply[0] != 0U ||
        memcmp(&g3->reply[1], payload, 6U) != 0)
    {
        g3_log_number("Failed to set G3 attribute", id);
        return false;
    }

    return true;
}

static bool configure_network(st8500_g3_t *g3)
{
    uint8_t pan_id[2];
    uint8_t coord_addr[2] = {0U, 0U};
    uint8_t channel[4];
    uint8_t active_key = 0U;

    write_u16(pan_id, G3_PAN_ID);
    write_u32(channel, G3_CHANNEL_PLC_ONLY);

    if (g3->coordinator)
    {
        if (!set_attribute(
                g3, G3_PIB_PAN_ID, 0U,
                pan_id, sizeof(pan_id)) ||
            !set_attribute(
                g3, G3_PIB_SHORT_ADDR, 0U,
                coord_addr, sizeof(coord_addr)) ||
            !set_attribute(
                g3, G3_PIB_KEY_TABLE, 0U,
                g3_gmk, sizeof(g3_gmk)) ||
            !set_attribute(
                g3, G3_PIB_ACTIVE_KEY, 0U,
                &active_key, 1U))
        {
            return false;
        }
    }
    else
    {
        if (!set_attribute(
                g3, G3_PIB_EAP_PSK, 0U,
                g3_psk, sizeof(g3_psk)) ||
            !set_attribute(
                g3, G3_PIB_COORD_ADDR, 0U,
                coord_addr, sizeof(coord_addr)))
        {
            return false;
        }
    }

    return set_attribute(
        g3,
        G3_PIB_CHANNEL_CONFIG,
        0U,
        channel,
        sizeof(channel)
    );
}

static bool start_coordinator(st8500_g3_t *g3)
{
    if (!send_request(
            g3,
            G3_BOOT_SRV_STOP_REQ,
            G3_BOOT_SRV_STOP_CONF,
            NULL,
            0U) ||
        g3->reply_length < 1U || g3->reply[0] != 0U)
    {
        g3_log("Failed to stop G3 boot server");
        return false;
    }

    g3->state = ST8500_G3_WAITING_FOR_NETWORK;

    uint8_t start[5] = {0U, 0U, 0U, 0U, 0U};
    write_u16(&start[2], G3_PAN_ID);

    if (!send_frame(
            g3,
            G3_BOOT_SRV_START_REQ,
            start,
            sizeof(start)))
    {
        g3_log("Failed to start G3 boot server");
        return false;
    }

    g3_log("G3 coordinator start requested");
    return true;
}

bool st8500_g3_start(
    st8500_g3_t *g3,
    UART_HandleTypeDef *uart,
    bool coordinator
)
{
    if (g3 == NULL || uart == NULL || uart->hdmarx == NULL)
    {
        return false;
    }

    memset(g3, 0, sizeof(*g3));

    g3->uart = uart;
    g3->coordinator = coordinator;
    g3->state = ST8500_G3_CONFIGURING;
    g3->pan_id = 0xFFFFU;
    g3->short_addr = 0xFFFFU;

    g3_log("Starting G3 host interface");

    __HAL_UART_CLEAR_OREFLAG(uart);
    __HAL_UART_SEND_REQ(uart, UART_RXDATA_FLUSH_REQUEST);

    if (HAL_UART_Receive_DMA(
            uart,
            g3->rx_dma,
            sizeof(g3->rx_dma)) != HAL_OK)
    {
        goto failed;
    }

    __HAL_DMA_DISABLE_IT(uart->hdmarx, DMA_IT_HT);
    HAL_Delay(G3_BOOT_WAIT_MS);

    uint8_t info_req[5] = {0};
    bool detected = false;

    for (uint8_t i = 0U; i < 3U; i++)
    {
        if (send_request(
                g3,
                G3_DBGTOOL_REQ,
                G3_DBGTOOL_CONF,
                info_req,
                sizeof(info_req)))
        {
            detected = true;
            break;
        }

        HAL_Delay(200U);
    }

    if (!detected || g3->reply_length < 3U)
    {
        g3_log("G3 runtime not responding");
        goto failed;
    }

    g3_log("G3 runtime detected");

    uint8_t reset[3] = {
        G3_BAND_CEN_A,
        coordinator ? G3_ROLE_COORDINATOR : G3_ROLE_DEVICE,
        G3_MODE_BOOT
    };

    g3->state = ST8500_G3_WAITING_FOR_NETWORK;

    if (!send_request(
            g3,
            G3_LIB_SWRESET_REQ,
            G3_LIB_SWRESET_CONF,
            reset,
            sizeof(reset)) ||
        g3->reply_length < 4U ||
        g3->reply[0] != 0U)
    {
        g3_log("G3 SWRESET failed");
        goto failed;
    }

    if (!configure_network(g3))
    {
        g3_log("G3 network configuration failed");
        goto failed;
    }

    if (coordinator && !start_coordinator(g3))
    {
        goto failed;
    }

    g3_log(coordinator ? "Coordinator configured" : "Device configured");
    return true;

failed:
    g3->state = ST8500_G3_FAILED;
    return false;
}

void st8500_g3_set_receive_callback(
    st8500_g3_t *g3,
    st8500_g3_ipv6_callback_t callback,
    void *context
)
{
    if (g3 == NULL)
    {
        return;
    }

    g3->on_ipv6 = callback;
    g3->on_ipv6_context = context;
}

bool st8500_g3_send_ipv6(
    st8500_g3_t *g3,
    const uint8_t *packet,
    uint16_t length
)
{
    if (g3 == NULL || g3->uart == NULL ||
        g3->state != ST8500_G3_NETWORK_READY ||
        g3->tx_pending || g3->waiting_reply ||
        !valid_ipv6_packet(packet, length))
    {
        return false;
    }

    // Do not transmit before a device joins the coordinator.
    if (g3->coordinator && g3->joined_devices == 0U)
    {
        return false;
    }

    if (g3->uart->gState != HAL_UART_STATE_READY)
    {
        return false;
    }

    uint8_t *frame = g3->tx_frame;
    uint16_t payload_length = (uint16_t)(length + 5U);
    uint16_t frame_length = (uint16_t)(length + 17U);
    uint8_t handle = g3->next_handle++;

    memset(frame, 0, G3_HEADER_SIZE);

    frame[0] = G3_SYNC;
    frame[1] = G3_SYNC;
    frame[2] = G3_ADP_DATA_REQ;

    write_u16(&frame[3], payload_length);

    // ADPD-DATA.Request fields.
    write_u16(&frame[10], length);
    frame[12] = handle;
    frame[13] = 1U;
    frame[14] = 0U;

    memcpy(&frame[15], packet, length);

    uint16_t crc = crc16_xmodem(
        frame,
        (uint16_t)(frame_length - 2U)
    );

    write_u16(&frame[frame_length - 2U], crc);

    if (HAL_UART_Transmit_DMA(
            g3->uart,
            frame,
            frame_length) != HAL_OK)
    {
        return false;
    }

    g3->tx_handle = handle;
    g3->tx_started_ms = HAL_GetTick();
    g3->tx_pending = true;
    g3->tx_requested++;

    return true;
}
