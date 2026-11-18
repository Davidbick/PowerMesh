#include "st8500_udp_echo.h"

#include <string.h>

#define UDP_ECHO_PORT 4242U
#define RETRY_DELAY_MS 1000U

static const uint8_t endpoint_address[16] = {
    0xFE, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x78, 0x1D, 0x00, 0xFF,
    0xFE, 0x00, 0x00, 0x01
};

static uint8_t reply[ST8500_G3_MAX_IPV6_SIZE];
static uint16_t reply_length;

static udp_echo_stats_t stats;

static bool awaiting_confirm;
static bool retry_backoff;
static uint32_t last_failure_ms;
static uint32_t request_count;
static uint32_t confirmed_before;
static uint8_t request_handle;


static uint16_t read_u16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | data[1];
}


static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)value;
}


static uint32_t sum_words(
    const uint8_t *data, uint16_t length, uint32_t sum
)
{
    uint16_t i = 0U;

    while (i + 1U < length)
    {
        sum += read_u16(&data[i]);
        i += 2U;
    }

    if (i < length)
    {
        sum += (uint16_t)data[i] << 8;
    }

    return sum;
}


static uint16_t udp_checksum(
    const uint8_t *packet, uint16_t udp_length
)
{
    uint32_t sum = sum_words(&packet[8], 32U, 0U);

    // IPv6 pseudo-header length and next-header value.
    sum += udp_length;
    sum += 17U;

    sum = sum_words(&packet[40], udp_length, sum);

    while (sum >> 16)
    {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }

    return (uint16_t)~sum;
}


void udp_echo_on_ipv6(
    const uint8_t *packet, uint16_t length, void *context
)
{
    (void)context;

    stats.received++;

    if (!st8500_g3_valid_ipv6(packet, length) ||
        length < 48U || packet[6] != 17U ||
        memcmp(&packet[24], endpoint_address, 16U) != 0)
    {
        return;
    }

    uint16_t udp_length = read_u16(&packet[44]);

    if (udp_length != length - 40U ||
        udp_length < 8U ||
        read_u16(&packet[42]) != UDP_ECHO_PORT)
    {
        return;
    }

    stats.requests++;

    // A zero UDP checksum is invalid for this IPv6 test.
    if (read_u16(&packet[46]) == 0U ||
        udp_checksum(packet, udp_length) != 0U)
    {
        stats.bad_checksum++;
        return;
    }

    if (stats.pending)
    {
        stats.busy++;
        return;
    }

    memcpy(reply, packet, length);
    reply_length = length;

    // Swap IPv6 source and destination addresses.
    for (uint8_t i = 0U; i < 16U; i++)
    {
        uint8_t temp = reply[8U + i];
        reply[8U + i] = reply[24U + i];
        reply[24U + i] = temp;
    }

    // Swap UDP source and destination ports.
    for (uint8_t i = 0U; i < 2U; i++)
    {
        uint8_t temp = reply[40U + i];
        reply[40U + i] = reply[42U + i];
        reply[42U + i] = temp;
    }

    // Recalculate the UDP checksum for the reply.
    reply[46] = 0U;
    reply[47] = 0U;

    uint16_t value = udp_checksum(reply, udp_length);
    write_u16(&reply[46], value == 0U ? 0xFFFFU : value);

    stats.pending = true;
    awaiting_confirm = false;
    retry_backoff = false;
}


void udp_echo_poll(st8500_g3_t *g3)
{
    if (g3 == NULL || !stats.pending)
    {
        return;
    }

    uint32_t now = HAL_GetTick();

    if (awaiting_confirm)
    {
        // Detect a modem reset or unexpected TX state change.
        if (g3->tx_requested != request_count ||
            g3->tx_handle != request_handle)
        {
            awaiting_confirm = false;
            retry_backoff = true;
            last_failure_ms = now;
            stats.failed++;
            return;
        }

        if (g3->tx_pending)
        {
            return;
        }

        awaiting_confirm = false;

        if (g3->tx_confirmed != confirmed_before)
        {
            stats.confirmed++;
            stats.pending = false;
            return;
        }

        stats.failed++;
        retry_backoff = true;
        last_failure_ms = now;
        return;
    }

    if (retry_backoff)
    {
        if ((uint32_t)(now - last_failure_ms) <
            RETRY_DELAY_MS)
        {
            return;
        }

        retry_backoff = false;
    }

    if (!st8500_g3_send_ipv6(g3, reply, reply_length))
    {
        return;
    }

    request_count = g3->tx_requested;
    request_handle = g3->tx_handle;
    confirmed_before = g3->tx_confirmed;
    awaiting_confirm = true;

    stats.submitted++;
}


const udp_echo_stats_t *udp_echo_get_stats(void)
{
    return &stats;
}
