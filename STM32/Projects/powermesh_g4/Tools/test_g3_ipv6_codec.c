#include "st8500_g3_codec.h"
#include "st8500_ipv6_diag.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const uint8_t crc_input[] = "123456789";
    assert(st8500_g3_crc16(crc_input, 9U) == 0x31C3U);

    uint8_t request[ST8500_IPV6_DIAG_SIZE];
    uint8_t response[ST8500_IPV6_DIAG_SIZE];

    assert(st8500_ipv6_diag_build(
        request, sizeof(request), 0x781DU, 0U, 1U, false));
    assert(st8500_ipv6_diag_matches(
        request, sizeof(request), 0x781DU, 0U, 1U, false));
    assert(st8500_ipv6_diag_build(
        response, sizeof(response), 0x781DU, 1U, 0U, true));
    assert(st8500_ipv6_diag_matches(
        response, sizeof(response), 0x781DU, 1U, 0U, true));
    assert(!st8500_ipv6_diag_matches(
        request, sizeof(request), 0x781DU, 1U, 0U, true));

    uint8_t frame[ST8500_G3_TX_FRAME_SIZE];
    uint16_t frame_length = 0U;

    assert(st8500_adp_encode_request(
        frame, sizeof(frame), request, sizeof(request),
        7U, &frame_length));
    assert(frame_length == sizeof(request) + 17U);
    assert(frame[2] == 0x80U);
    assert(frame[12] == 7U);
    assert(frame[13] == 1U);
    assert(st8500_g3_crc16(frame, frame_length - 2U) ==
        (uint16_t)(frame[frame_length - 2U] |
        ((uint16_t)frame[frame_length - 1U] << 8)));
    assert(memcmp(&frame[15], request, sizeof(request)) == 0);

    // A short-address previous hop with no BC0 headers.
    uint8_t indication[100] = {0};
    indication[0] = sizeof(request);
    indication[2] = 2U;
    indication[3] = 0x1DU;
    indication[4] = 0x78U;
    memcpy(&indication[16], request, sizeof(request));

    const uint8_t *decoded = NULL;
    uint16_t decoded_length = 0U;
    assert(st8500_adp_decode_indication(
        indication, 80U, &decoded, &decoded_length));
    assert(decoded_length == sizeof(request));
    assert(memcmp(decoded, request, sizeof(request)) == 0);

    assert(!st8500_adp_decode_indication(
        indication, 79U, &decoded, &decoded_length));
    indication[14] = 7U;
    assert(!st8500_adp_decode_indication(
        indication, 80U, &decoded, &decoded_length));
    indication[14] = 0U;
    indication[2] = 1U;
    assert(!st8500_adp_decode_indication(
        indication, 80U, &decoded, &decoded_length));

    uint8_t packet1280[1280] = {0x60};
    packet1280[4] = 0x04U;
    packet1280[5] = 0xD8U;
    assert(st8500_adp_encode_request(
        frame, sizeof(frame), packet1280, sizeof(packet1280),
        9U, &frame_length));
    assert(frame_length == ST8500_G3_TX_FRAME_SIZE);

    puts("G3 codec and IPv6 diagnostic tests passed");
    return 0;
}
