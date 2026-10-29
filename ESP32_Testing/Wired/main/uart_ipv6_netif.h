#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the custom wired IPv6 interface.
// Incoming and outgoing IPv6 packets use PowerMesh UART framing.
esp_err_t uart_ipv6_netif_start(void);

// Validate and deliver one raw IPv6 packet into lwIP.
// The packet contains an IPv6 header, without an Ethernet header.
esp_err_t uart_ipv6_netif_input(
    const uint8_t *packet,
    size_t length
);

#ifdef __cplusplus
}
#endif
