#include "st8500_g3_codec.h"

#include <string.h>

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

uint16_t st8500_g3_crc16(const uint8_t *data, uint16_t length)
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

bool st8500_g3_valid_ipv6(const uint8_t *packet, uint16_t length)
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

bool st8500_adp_encode_request(
    uint8_t *frame, uint16_t capacity,
    const uint8_t *packet, uint16_t length,
    uint8_t handle, uint16_t *frame_length
)
{
    if (frame == NULL || frame_length == NULL ||
        !st8500_g3_valid_ipv6(packet, length) ||
        (uint32_t)length + 17U > capacity)
    {
        return false;
    }

    uint16_t total = (uint16_t)(length + 17U);
    memset(frame, 0, 15U);

    frame[0] = 0x16U;
    frame[1] = 0x16U;
    frame[2] = 0x80U;
    write_u16(&frame[3], (uint16_t)(length + 5U));

    // NsduLength, NsduHandle, DiscoverRoute and QoS.
    write_u16(&frame[10], length);
    frame[12] = handle;
    frame[13] = 1U;
    frame[14] = 0U;

    memcpy(&frame[15], packet, length);
    write_u16(&frame[total - 2U],
              st8500_g3_crc16(frame, total - 2U));

    *frame_length = total;
    return true;
}

bool st8500_adp_decode_indication(
    const uint8_t *data, uint16_t length,
    const uint8_t **packet, uint16_t *packet_length
)
{
    if (data == NULL || packet == NULL ||
        packet_length == NULL || length < 14U)
    {
        return false;
    }

    uint16_t nsdu_length = read_u16(data);
    uint16_t index = 2U;
    uint8_t address_mode = data[index++];

    if (address_mode != 0U && address_mode != 2U &&
        address_mode != 3U)
    {
        return false;
    }

    // Previous-hop PAN ID and optional short or extended address.
    index = (uint16_t)(index + 2U);
    if (address_mode == 2U)
    {
        index = (uint16_t)(index + 2U);
    }
    else if (address_mode == 3U)
    {
        index = (uint16_t)(index + 8U);
    }

    // LQI, phase, modulation type, scheme, and 3-byte tone map.
    if ((uint32_t)index + 8U > length)
    {
        return false;
    }

    index = (uint16_t)(index + 7U);
    uint8_t bc0_count = data[index++];

    if (bc0_count > 6U ||
        (uint32_t)index + bc0_count + 1U > length)
    {
        return false;
    }

    // Variable-length BC0 list and previous-hop media.
    index = (uint16_t)(index + bc0_count + 1U);

    if (nsdu_length != (uint16_t)(length - index) ||
        !st8500_g3_valid_ipv6(&data[index], nsdu_length))
    {
        return false;
    }

    *packet = &data[index];
    *packet_length = nsdu_length;
    return true;
}
