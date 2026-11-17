#ifndef ST8500_G3_CODEC_H
#define ST8500_G3_CODEC_H

#include <stdbool.h>
#include <stdint.h>

#define ST8500_G3_MAX_IPV6_SIZE 1280U
#define ST8500_G3_TX_FRAME_SIZE (ST8500_G3_MAX_IPV6_SIZE + 17U)

uint16_t st8500_g3_crc16(const uint8_t *data, uint16_t length);
bool st8500_g3_valid_ipv6(const uint8_t *packet, uint16_t length);
bool st8500_adp_encode_request(
    uint8_t *frame, uint16_t capacity,
    const uint8_t *packet, uint16_t length,
    uint8_t handle, uint16_t *frame_length
);
bool st8500_adp_decode_indication(
    const uint8_t *data, uint16_t length,
    const uint8_t **packet, uint16_t *packet_length
);

#endif
