#ifndef ST8500_IPV6_DIAG_H
#define ST8500_IPV6_DIAG_H

#include <stdbool.h>
#include <stdint.h>

#define ST8500_IPV6_DIAG_SIZE 64U

bool st8500_ipv6_diag_build(
    uint8_t *packet, uint16_t capacity, uint16_t pan_id,
    uint16_t source, uint16_t destination, bool reply
);
bool st8500_ipv6_diag_matches(
    const uint8_t *packet, uint16_t length, uint16_t pan_id,
    uint16_t source, uint16_t destination, bool reply
);

#endif
