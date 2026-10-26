#include "uart_ipv6_netif.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"

#define TAG "uart_ipv6_netif"
#define UART_IPV6_MAX_PACKET 1280U
#define IPV6_HEADER_LENGTH 40U

/* UDP outer tunnel used for ESP32 -> Pi packets (Pi listens on eth0). */
#define WIFI_TUNNEL_PORT 4243U
#define WIFI_TUNNEL_HEADER_SIZE 8U
#define WIFI_TUNNEL_QUEUE_DEPTH 4U
#define WIFI_TUNNEL_MAGIC "PMU1"

/*
 * Set this to the Pi's eth0 link-local IPv6 address. It is taken from the
 * user's current interface output: ip -6 -o addr show dev eth0 scope link.
 * Override at compile time if the Pi address changes.
 */
#ifndef POWERMESH_PI_LINK_LOCAL_IPV6
#define POWERMESH_PI_LINK_LOCAL_IPV6 "fe80::1ea3:e633:8c63:30b1"
#endif

typedef struct {
    uint16_t length;
    uint8_t packet[UART_IPV6_MAX_PACKET];
} wifi_tunnel_item_t;

static struct netif s_uart_netif;
static bool s_started;
static bool s_tx_task_started;
static SemaphoreHandle_t s_init_done;
static QueueHandle_t s_wifi_tx_queue;
static volatile uint8_t s_wifi_if_index;
static esp_err_t s_init_result = ESP_FAIL;

#if LWIP_IPV6
static uint8_t find_wifi_station_index_on_tcpip_thread(void)
{
    for (struct netif *candidate = netif_list;
         candidate != NULL;
         candidate = candidate->next) {
        if (candidate != &s_uart_netif &&
            candidate->name[0] == 's' && candidate->name[1] == 't') {
            /* netif_get_index() returns the 1-based IPv6 zone/interface index. */
            return netif_get_index(candidate);
        }
    }
    return 0;
}

static void refresh_wifi_station_index(void *arg)
{
    (void)arg;
    s_wifi_if_index = find_wifi_station_index_on_tcpip_thread();
}

/*
 * A small FreeRTOS task encapsulates packets selected for uaN in UDP and
 * sends them over the ESP32's Wi-Fi link to the Pi. Do not call BSD sockets
 * from the lwIP output callback itself: that callback runs in lwIP's context.
 */
static void wifi_udp_tunnel_tx_task(void *arg)
{
    (void)arg;

    int sock = -1;
    while (sock < 0) {
        sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            ESP_LOGE(TAG, "UDP tunnel socket() failed, errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    struct sockaddr_in6 peer;
    memset(&peer, 0, sizeof(peer));
    peer.sin6_family = AF_INET6;
    peer.sin6_port = htons(WIFI_TUNNEL_PORT);
    if (inet_pton(AF_INET6, POWERMESH_PI_LINK_LOCAL_IPV6, &peer.sin6_addr) != 1) {
        ESP_LOGE(TAG, "Invalid Pi link-local IPv6 address: %s",
                 POWERMESH_PI_LINK_LOCAL_IPV6);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    uint16_t sequence = 1;
    uint8_t datagram[WIFI_TUNNEL_HEADER_SIZE + UART_IPV6_MAX_PACKET];

    ESP_LOGI(TAG,
             "Wi-Fi UDP return-tunnel worker started: Pi=[%s]:%u",
             POWERMESH_PI_LINK_LOCAL_IPV6,
             (unsigned)WIFI_TUNNEL_PORT);

    while (true) {
        if (s_wifi_if_index == 0) {
            /* Retry from lwIP's TCP/IP thread if the station netif appeared late. */
            (void)tcpip_callback(refresh_wifi_station_index, NULL);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        peer.sin6_scope_id = s_wifi_if_index;

        wifi_tunnel_item_t item;
        if (xQueueReceive(s_wifi_tx_queue, &item, pdMS_TO_TICKS(500)) != pdTRUE) {
            continue;
        }

        memcpy(datagram, WIFI_TUNNEL_MAGIC, 4);
        datagram[4] = (uint8_t)(sequence >> 8);
        datagram[5] = (uint8_t)(sequence & 0xFF);
        datagram[6] = (uint8_t)(item.length >> 8);
        datagram[7] = (uint8_t)(item.length & 0xFF);
        memcpy(&datagram[WIFI_TUNNEL_HEADER_SIZE], item.packet, item.length);

        const size_t datagram_length = WIFI_TUNNEL_HEADER_SIZE + item.length;
        bool sent_ok = false;
        int last_errno = 0;
        for (unsigned attempt = 0; attempt < 3; attempt++) {
            const int sent = sendto(
                sock,
                datagram,
                datagram_length,
                0,
                (const struct sockaddr *)&peer,
                sizeof(peer));

            if (sent == (int)datagram_length) {
                sent_ok = true;
                break;
            }

            last_errno = errno;
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (sent_ok) {
            ESP_LOGD(TAG,
                     "Wi-Fi UDP tunnel -> Pi: seq=%u, inner IPv6 length=%u",
                     (unsigned)sequence,
                     (unsigned)item.length);
            sequence++;
            if (sequence == 0) {
                sequence = 1;
            }
        } else {
            ESP_LOGE(TAG,
                     "Wi-Fi UDP tunnel send failed after retries, errno=%d; dropped %u-byte packet",
                     last_errno,
                     (unsigned)item.length);
        }
    }
}
#endif /* LWIP_IPV6 */

#if LWIP_IPV4
#include "lwip/ip4_addr.h"

static err_t uart_netif_output_ipv4(struct netif *netif,
                                    struct pbuf *p,
                                    const ip4_addr_t *destination)
{
    (void)netif;
    (void)p;
    (void)destination;
    /* PowerMesh transports raw IPv6 packets only. */
    return ERR_IF;
}
#endif

#if LWIP_IPV6
static err_t uart_netif_output_ipv6(struct netif *netif,
                                    struct pbuf *p,
                                    const ip6_addr_t *destination)
{
    (void)netif;
    (void)destination;

    if (p == NULL || s_wifi_tx_queue == NULL) {
        return ERR_IF;
    }

    if (p->tot_len < IPV6_HEADER_LENGTH || p->tot_len > UART_IPV6_MAX_PACKET) {
        ESP_LOGW(TAG, "Rejecting outbound IPv6 packet with invalid size %u",
                 (unsigned)p->tot_len);
        return ERR_BUF;
    }

    /*
     * The raw packet is copied to a bounded queue so the lwIP TCP/IP task
     * never calls BSD socket APIs or waits for Wi-Fi. The worker wraps it in
     * a small UDP envelope and sends it to the Pi. The Pi unwraps the packet
     * and writes it to powermesh0, preserving the inner IPv6 addresses.
     *
     * Multicast output is not supported in this first hybrid-tunnel stage;
     * CHIP Minimal mDNS is configured separately to advertise on Wi-Fi.
     */
    if (destination != NULL && ip6_addr_ismulticast(destination)) {
        ESP_LOGD(TAG, "Rejecting multicast output from ua%u in this test stage",
                 (unsigned)s_uart_netif.num);
        return ERR_RTE;
    }

    wifi_tunnel_item_t item;
    item.length = p->tot_len;
    const u16_t copied = pbuf_copy_partial(p, item.packet, item.length, 0);
    if (copied != item.length) {
        ESP_LOGW(TAG, "pbuf_copy_partial copied %u of %u outbound bytes",
                 (unsigned)copied, (unsigned)item.length);
        return ERR_BUF;
    }

    if ((item.packet[0] >> 4) != 6) {
        ESP_LOGW(TAG, "Rejecting non-IPv6 packet from ua%u", (unsigned)s_uart_netif.num);
        return ERR_VAL;
    }

    if (xQueueSend(s_wifi_tx_queue, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Wi-Fi return queue full; dropping %u-byte packet",
                 (unsigned)item.length);
        return ERR_MEM;
    }

    ESP_LOGD(TAG, "Queued %u-byte IPv6 packet for Wi-Fi UDP return tunnel",
             (unsigned)item.length);
    return ERR_OK;
}
#endif /* LWIP_IPV6 */

static err_t uart_netif_init(struct netif *netif)
{
    netif->name[0] = 'u';
    netif->name[1] = 'a';
    netif->mtu = UART_IPV6_MAX_PACKET;

#if LWIP_IPV4
    netif->output = uart_netif_output_ipv4;
#endif

#if LWIP_IPV6
    netif->output_ip6 = uart_netif_output_ipv6;
#if LWIP_IPV6_MLD
    /* Address membership can be tracked; mDNS will not bind to uaN. */
    netif->flags |= NETIF_FLAG_MLD6;
#endif
#endif

    return ERR_OK;
}

#if LWIP_IPV6
static bool set_static_ipv6_address(struct netif *netif,
                                    s8_t slot,
                                    const char *address_text)
{
    ip6_addr_t address;

    if (ip6addr_aton(address_text, &address) == 0) {
        ESP_LOGE(TAG, "Invalid hard-coded IPv6 address: %s", address_text);
        return false;
    }

    netif_ip6_addr_set(netif, slot, &address);
    /* The isolated tunnel addresses are configured statically for this test. */
    netif_ip6_addr_set_state(netif, slot, IP6_ADDR_PREFERRED);
    return true;
}
#endif

static void uart_netif_add_on_tcpip_thread(void *arg)
{
    (void)arg;

#if !LWIP_IPV6
    s_init_result = ESP_ERR_NOT_SUPPORTED;
    xSemaphoreGive(s_init_done);
    return;
#else
    struct netif *previous_default = netif_default;

    if (netif_add_noaddr(&s_uart_netif, NULL, uart_netif_init, tcpip_input) == NULL) {
        ESP_LOGE(TAG, "netif_add_noaddr failed");
        s_init_result = ESP_FAIL;
        xSemaphoreGive(s_init_done);
        return;
    }

    if (!set_static_ipv6_address(&s_uart_netif, 0, "fe80::2") ||
        !set_static_ipv6_address(&s_uart_netif, 1, "fd00:706d::2")) {
        netif_remove(&s_uart_netif);
        netif_set_default(previous_default);
        s_init_result = ESP_FAIL;
        xSemaphoreGive(s_init_done);
        return;
    }

    netif_set_up(&s_uart_netif);
    netif_set_link_up(&s_uart_netif);

    /* Preserve the existing system default (normally Wi-Fi). */
    netif_set_default(previous_default);

    s_wifi_if_index = find_wifi_station_index_on_tcpip_thread();
    if (s_wifi_if_index == 0) {
        ESP_LOGW(TAG, "Wi-Fi station netif (stN) not found yet; tunnel sender will retry lookup");
    } else {
        ESP_LOGI(TAG, "Wi-Fi UDP return path will use station interface index %u",
                 (unsigned)s_wifi_if_index);
    }

    s_started = true;
    s_init_result = ESP_OK;
    ESP_LOGI(TAG,
             "Raw IPv6 lwIP netif ready (ua%u): fe80::2, fd00:706d::2, MTU=%u; "
             "RX via UART, unicast TX via UDP/Wi-Fi to Pi port %u",
             (unsigned)s_uart_netif.num,
             (unsigned)UART_IPV6_MAX_PACKET,
             (unsigned)WIFI_TUNNEL_PORT);
    xSemaphoreGive(s_init_done);
#endif
}

static esp_err_t start_wifi_tx_task(void)
{
#if !LWIP_IPV6
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_tx_task_started) {
        return ESP_OK;
    }

    if (s_wifi_tx_queue == NULL) {
        s_wifi_tx_queue = xQueueCreate(WIFI_TUNNEL_QUEUE_DEPTH, sizeof(wifi_tunnel_item_t));
        if (s_wifi_tx_queue == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    const BaseType_t created = xTaskCreate(
        wifi_udp_tunnel_tx_task,
        "wifi_ip6_tunnel_tx",
        6144,
        NULL,
        5,
        NULL);

    if (created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    s_tx_task_started = true;
    return ESP_OK;
#endif
}

esp_err_t uart_ipv6_netif_start(void)
{
    if (s_started) {
        return start_wifi_tx_task();
    }

    if (s_init_done != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_wifi_tx_queue == NULL) {
        s_wifi_tx_queue = xQueueCreate(WIFI_TUNNEL_QUEUE_DEPTH, sizeof(wifi_tunnel_item_t));
        if (s_wifi_tx_queue == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_init_done = xSemaphoreCreateBinary();
    if (s_init_done == NULL) {
        vQueueDelete(s_wifi_tx_queue);
        s_wifi_tx_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    err_t lwip_err = tcpip_callback(uart_netif_add_on_tcpip_thread, NULL);
    if (lwip_err != ERR_OK) {
        ESP_LOGE(TAG, "tcpip_callback failed: %d", (int)lwip_err);
        vSemaphoreDelete(s_init_done);
        s_init_done = NULL;
        vQueueDelete(s_wifi_tx_queue);
        s_wifi_tx_queue = NULL;
        return ESP_FAIL;
    }

    if (xSemaphoreTake(s_init_done, pdMS_TO_TICKS(10000)) != pdTRUE) {
        ESP_LOGE(TAG, "Timed out creating UART lwIP netif");
        /* Callback may still run; keep its semaphore and queue alive. */
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(s_init_done);
    s_init_done = NULL;

    if (s_init_result != ESP_OK) {
        vQueueDelete(s_wifi_tx_queue);
        s_wifi_tx_queue = NULL;
        return s_init_result;
    }

    return start_wifi_tx_task();
}

esp_err_t uart_ipv6_netif_input(const uint8_t *packet, size_t length)
{
#if !LWIP_IPV6
    (void)packet;
    (void)length;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_started) {
        return ESP_ERR_INVALID_STATE;
    }

    if (packet == NULL || length < IPV6_HEADER_LENGTH ||
        length > UART_IPV6_MAX_PACKET) {
        return ESP_ERR_INVALID_SIZE;
    }

    if ((packet[0] >> 4) != 6) {
        ESP_LOGW(TAG, "Dropping non-IPv6 UART payload");
        return ESP_ERR_INVALID_ARG;
    }

    const uint16_t ipv6_payload_length =
        ((uint16_t)packet[4] << 8) | (uint16_t)packet[5];
    if (length != IPV6_HEADER_LENGTH + (size_t)ipv6_payload_length) {
        ESP_LOGW(TAG,
                 "Dropping IPv6 packet with inconsistent length: frame=%u IPv6-header=%u",
                 (unsigned)length,
                 (unsigned)(IPV6_HEADER_LENGTH + ipv6_payload_length));
        return ESP_ERR_INVALID_SIZE;
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)length, PBUF_RAM);
    if (p == NULL) {
        ESP_LOGW(TAG, "pbuf_alloc failed for %u-byte IPv6 packet", (unsigned)length);
        return ESP_ERR_NO_MEM;
    }

    if (pbuf_take(p, packet, (u16_t)length) != ERR_OK) {
        pbuf_free(p);
        return ESP_FAIL;
    }

    /* tcpip_input queues the pbuf; on success lwIP owns it. */
    err_t input_err = s_uart_netif.input(p, &s_uart_netif);
    if (input_err != ERR_OK) {
        pbuf_free(p);
        ESP_LOGW(TAG, "lwIP rejected packet enqueue: %d", (int)input_err);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Queued %u-byte IPv6 packet to lwIP", (unsigned)length);
    return ESP_OK;
#endif
}
