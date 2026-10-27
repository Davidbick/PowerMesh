#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a raw IPv6 lwIP netif for the UART tunnel.
 *
 * This function must be called after ESP-Matter/lwIP have started.
 * The interface is assigned fe80::2 and fd00:706d::2. The initial
 * implementation is receive-only: lwIP output through this netif is
 * deliberately rejected until the UART transmit path is implemented.
 */
esp_err_t uart_ipv6_netif_start(void);

/**
 * Validate and enqueue one raw IPv6 packet into lwIP.
 *
 * The packet must begin with an IPv6 header; no Ethernet header is expected.
 * The caller retains ownership of packet. This function copies the packet
 * into a lwIP pbuf before handing it to the TCP/IP task.
 */
esp_err_t uart_ipv6_netif_input(const uint8_t *packet, size_t length);

#ifdef __cplusplus
}
#endif
