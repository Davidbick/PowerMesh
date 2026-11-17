#include "st8500_ipv6_diag.h"

#include <string.h>

static const uint8_t marker[] = "PMESH_G3_IPV6_01";

_Static_assert(sizeof(marker) - 1U == 16U,
               "Diagnostic marker must be 16 bytes");

// ST G3 link-local scheme based on PAN ID and short address.
static void set_address(
    uint8_t *address, uint16_t pan_id, uint16_t short_addr
)
{
    memset(address, 0, 16U);
    address[0] = 0xFEU;
    address[1] = 0x80U;
    address[8] = (uint8_t)(pan_id >> 8);
    address[9] = (uint8_t)pan_id;
    address[11] = 0xFFU;
    address[12] = 0xFEU;
    address[14] = (uint8_t)(short_addr >> 8);
    address[15] = (uint8_t)short_addr;
}

// Checksum includes the IPv6 pseudo-header.
static uint16_t checksum(const uint8_t *packet)
{
    uint32_t sum = 24U + 58U;

    for (uint16_t i = 8U; i < ST8500_IPV6_DIAG_SIZE; i += 2U)
    {
        sum += ((uint16_t)packet[i] << 8) | packet[i + 1U];
    }

    while ((sum >> 16) != 0U)
    {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

bool st8500_ipv6_diag_build(
    uint8_t *packet, uint16_t capacity, uint16_t pan_id,
    uint16_t source, uint16_t destination, bool reply
)
{
    if (packet == NULL || capacity < ST8500_IPV6_DIAG_SIZE)
    {
        return false;
    }

    memset(packet, 0, ST8500_IPV6_DIAG_SIZE);
    packet[0] = 0x60U;
    packet[5] = 24U;
    packet[6] = 58U;
    packet[7] = 64U;

    set_address(&packet[8], pan_id, source);
    set_address(&packet[24], pan_id, destination);

    packet[40] = reply ? 129U : 128U;
    packet[44] = 0x50U;
    packet[45] = 0x4DU;
    packet[46] = 0U;
    packet[47] = 1U;
    memcpy(&packet[48], marker, sizeof(marker) - 1U);

    uint16_t value = checksum(packet);
    packet[42] = (uint8_t)(value >> 8);
    packet[43] = (uint8_t)value;
    return true;
}

bool st8500_ipv6_diag_matches(
    const uint8_t *packet, uint16_t length, uint16_t pan_id,
    uint16_t source, uint16_t destination, bool reply
)
{
    uint8_t expected[ST8500_IPV6_DIAG_SIZE];

    return packet != NULL &&
        length == ST8500_IPV6_DIAG_SIZE &&
        st8500_ipv6_diag_build(
            expected, sizeof(expected),
            pan_id, source, destination, reply) &&
        memcmp(packet, expected, sizeof(expected)) == 0;
}
