#include "uart_ipv6_netif.h"

#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"

#define TAG "uart_ipv6_netif"
#define UART_IPV6_MAX_PACKET 1280U
#define IPV6_HEADER_LENGTH 40U

static struct netif s_uart_netif;
static bool s_started;
static SemaphoreHandle_t s_init_done;
static esp_err_t s_init_result = ESP_FAIL;

#if LWIP_IPV4
#include "lwip/ip4_addr.h"

static err_t uart_netif_output_ipv4(struct netif *netif,
                                    struct pbuf *p,
                                    const ip4_addr_t *destination)
{
    (void)netif;
    (void)p;
    (void)destination;
    /* The UART tunnel currently transports IPv6 only. */
    return ERR_IF;
}
#endif

#if LWIP_IPV6
static err_t uart_netif_output_ipv6(struct netif *netif,
                                    struct pbuf *p,
                                    const ip6_addr_t *destination)
{
    (void)netif;
    (void)p;
    (void)destination;
    /* Receive-only for this milestone; add UART TX here later. */
    return ERR_IF;
}
#endif

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
    /* This is a point-to-point virtual link. Multicast packets are delivered
       from UART directly; there is no Ethernet MAC filter on this interface. */
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
    /* These are statically assigned tunnel addresses. For this isolated
       point-to-point link, mark them usable without running DAD on UART. */
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

    /* The ESP-IDF default IPv6 input hook drops packets received on an
       interface without a valid link-local IPv6 address. Configure one and
       the project's ULA address before marking the interface up. */
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

    /* Do not make this receive-only tunnel the system default route. If
       netif_add_noaddr selected it because there was no previous default,
       restore the prior value (including NULL). */
    netif_set_default(previous_default);

    s_started = true;
    s_init_result = ESP_OK;
    ESP_LOGI(TAG,
             "Raw IPv6 lwIP netif ready (ua): fe80::2, fd00:706d::2, MTU=%u; RX only",
             (unsigned)UART_IPV6_MAX_PACKET);
    xSemaphoreGive(s_init_done);
#endif
}

esp_err_t uart_ipv6_netif_start(void)
{
    if (s_started) {
        return ESP_OK;
    }

    if (s_init_done != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_init_done = xSemaphoreCreateBinary();
    if (s_init_done == NULL) {
        return ESP_ERR_NO_MEM;
    }

    err_t lwip_err = tcpip_callback(uart_netif_add_on_tcpip_thread, NULL);
    if (lwip_err != ERR_OK) {
        ESP_LOGE(TAG, "tcpip_callback failed: %d", (int)lwip_err);
        vSemaphoreDelete(s_init_done);
        s_init_done = NULL;
        return ESP_FAIL;
    }

    /* ESP-Matter has already started the TCP/IP stack before this call. */
    if (xSemaphoreTake(s_init_done, pdMS_TO_TICKS(10000)) != pdTRUE) {
        ESP_LOGE(TAG, "Timed out creating UART lwIP netif");
        /* Keep the semaphore alive in case the already-queued callback runs. */
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(s_init_done);
    s_init_done = NULL;
    return s_init_result;
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

    /* netif.input is tcpip_input: it queues the pbuf to the lwIP TCP/IP task.
       On success, ownership of p passes to lwIP. */
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
