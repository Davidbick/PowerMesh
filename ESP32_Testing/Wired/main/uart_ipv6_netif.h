#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the raw IPv6 lwIP netif for the UART tunnel.
 *
 * Incoming raw IPv6 packets arrive over UART. Outgoing unicast IPv6 packets
 * selected for this interface are copied to a FreeRTOS queue, encapsulated in
 * UDP over the ESP32 Wi-Fi connection, and sent to the Pi. The Pi's hybrid
 * bridge decapsulates them and injects them into powermesh0. The inner IPv6
 * packet addresses are preserved. Multicast output from uaN is not supported
 * in this initial test stage.
 */
esp_err_t uart_ipv6_netif_start(void);

/**
 * Validate and enqueue one raw IPv6 packet into lwIP. The packet begins with
 * an IPv6 header (no Ethernet header). This function copies it into a lwIP
 * pbuf and queues it for the TCP/IP task.
 */
esp_err_t uart_ipv6_netif_input(const uint8_t *packet, size_t length);

#ifdef __cplusplus
}
#endif
