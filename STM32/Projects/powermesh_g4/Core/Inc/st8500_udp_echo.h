#ifndef ST8500_UDP_ECHO_H
#define ST8500_UDP_ECHO_H

#include "st8500_g3.h"

typedef struct
{
    uint32_t received;
    uint32_t requests;
    uint32_t bad_checksum;
    uint32_t busy;
    uint32_t submitted;
    uint32_t confirmed;
    uint32_t failed;
    bool pending;
} udp_echo_stats_t;

void udp_echo_on_ipv6(
    const uint8_t *packet, uint16_t length, void *context
);

void udp_echo_poll(st8500_g3_t *g3);

const udp_echo_stats_t *udp_echo_get_stats(void);

#endif
