#include "uart_ipv6_netif.h"
#include "uart_frame.h"

#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"

#define TAG "uart_ipv6_netif"
#define UART_IPV6_MAX_PACKET 1280U
#define IPV6_HEADER_LENGTH 40U
#define UART_TX_QUEUE_DEPTH 6U

typedef struct {
    uint16_t length;
    uint8_t packet[UART_IPV6_MAX_PACKET];
} uart_ipv6_item_t;

static struct netif s_uart_netif;
static QueueHandle_t s_uart_tx_queue;
static SemaphoreHandle_t s_init_done;

static bool s_started;
static bool s_tx_task_started;
static esp_err_t s_init_result = ESP_FAIL;

// Send outbound IPv6 packets using the existing PowerMesh framing protocol.
static void uart_ipv6_tx_task(void *arg)
{
    (void)arg;

    uart_ipv6_item_t item;
    uint16_t sequence = 1;

    ESP_LOGI(TAG, "Wired IPv6 UART TX task started");

    while (true) {
        if (xQueueReceive(s_uart_tx_queue, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        esp_err_t err = uart_frame_send(
            UART_FRAME_TYPE_DATA,
            sequence,
            item.packet,
            item.length
        );

        if (err != ESP_OK) {
            ESP_LOGE(TAG,
                     "UART TX failed: seq=%u len=%u error=%s",
                     (unsigned)sequence,
                     (unsigned)item.length,
                     esp_err_to_name(err));
            continue;
        }

        ESP_LOGI(TAG,
                 "IPv6 -> UART: seq=%u len=%u",
                 (unsigned)sequence,
                 (unsigned)item.length);

        sequence++;
        if (sequence == 0) {
            sequence = 1;
        }
    }
}

#if LWIP_IPV4
#include "lwip/ip4_addr.h"

static err_t uart_netif_output_ipv4(
    struct netif *netif,
    struct pbuf *p,
    const ip4_addr_t *destination)
{
    (void)netif;
    (void)p;
    (void)destination;

    // PowerMesh currently transports IPv6 only.
    return ERR_IF;
}
#endif

#if LWIP_IPV6
static err_t uart_netif_output_ipv6(
    struct netif *netif,
    struct pbuf *p,
    const ip6_addr_t *destination)
{
    (void)netif;
    (void)destination;

    if (p == NULL || s_uart_tx_queue == NULL) {
        return ERR_IF;
    }

    if (p->tot_len < IPV6_HEADER_LENGTH ||
        p->tot_len > UART_IPV6_MAX_PACKET) {
        ESP_LOGW(TAG,
                 "Invalid outbound IPv6 packet length=%u",
                 (unsigned)p->tot_len);
        return ERR_BUF;
    }

    // Copy the packet because lwIP owns the original pbuf.
    // UART transmission runs in a separate FreeRTOS task.
    uart_ipv6_item_t item;
    item.length = (uint16_t)p->tot_len;

    u16_t copied = pbuf_copy_partial(
        p,
        item.packet,
        item.length,
        0
    );

    if (copied != item.length) {
        ESP_LOGW(TAG, "Failed to copy outbound IPv6 packet");
        return ERR_BUF;
    }

    if ((item.packet[0] >> 4) != 6) {
        ESP_LOGW(TAG, "Outbound packet is not IPv6");
        return ERR_VAL;
    }

    // Allow both unicast and multicast IPv6.
    // Matter DNS-SD discovery uses multicast.
    if (xQueueSend(s_uart_tx_queue, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "UART TX queue full");
        return ERR_MEM;
    }

    return ERR_OK;
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
    netif->flags |= NETIF_FLAG_MLD6;
#endif
#endif

    return ERR_OK;
}

#if LWIP_IPV6
static bool set_static_ipv6_address(
    struct netif *netif,
    s8_t slot,
    const char *address_text)
{
    ip6_addr_t address;

    if (ip6addr_aton(address_text, &address) == 0) {
        ESP_LOGE(TAG, "Invalid IPv6 address: %s", address_text);
        return false;
    }

    netif_ip6_addr_set(netif, slot, &address);
    netif_ip6_addr_set_state(netif, slot, IP6_ADDR_PREFERRED);

    return true;
}
#endif

static void uart_netif_add_on_tcpip_thread(void *arg)
{
    (void)arg;

#if !LWIP_IPV6
    s_init_result = ESP_ERR_NOT_SUPPORTED;
#else
    struct netif *previous_default = netif_default;

    if (netif_add_noaddr(
            &s_uart_netif,
            NULL,
            uart_netif_init,
            tcpip_input) == NULL) {
        ESP_LOGE(TAG, "Failed to create UART IPv6 netif");
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

    // This test uses UART as its only operational IP network.
    netif_set_default(&s_uart_netif);

    s_started = true;
    s_init_result = ESP_OK;

    ESP_LOGI(TAG,
             "Wired IPv6 ready: ua%u, fe80::2, fd00:706d::2, MTU=%u",
             (unsigned)s_uart_netif.num,
             (unsigned)UART_IPV6_MAX_PACKET);
#endif

    xSemaphoreGive(s_init_done);
}

static esp_err_t start_uart_tx_task(void)
{
    if (s_tx_task_started) {
        return ESP_OK;
    }

    BaseType_t created = xTaskCreate(
        uart_ipv6_tx_task,
        "uart_ipv6_tx",
        6144,
        NULL,
        5,
        NULL
    );

    if (created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    s_tx_task_started = true;
    return ESP_OK;
}

esp_err_t uart_ipv6_netif_start(void)
{
#if !LWIP_IPV6
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_started) {
        return start_uart_tx_task();
    }

    if (s_init_done != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_uart_tx_queue = xQueueCreate(
        UART_TX_QUEUE_DEPTH,
        sizeof(uart_ipv6_item_t)
    );

    if (s_uart_tx_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_init_done = xSemaphoreCreateBinary();

    if (s_init_done == NULL) {
        vQueueDelete(s_uart_tx_queue);
        s_uart_tx_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    err_t err = tcpip_callback(
        uart_netif_add_on_tcpip_thread,
        NULL
    );

    if (err != ERR_OK) {
        vSemaphoreDelete(s_init_done);
        s_init_done = NULL;
        vQueueDelete(s_uart_tx_queue);
        s_uart_tx_queue = NULL;
        return ESP_FAIL;
    }

    if (xSemaphoreTake(
            s_init_done,
            pdMS_TO_TICKS(10000)) != pdTRUE) {
        ESP_LOGE(TAG, "Timed out creating UART IPv6 netif");
        return ESP_ERR_TIMEOUT;
    }

    vSemaphoreDelete(s_init_done);
    s_init_done = NULL;

    if (s_init_result != ESP_OK) {
        vQueueDelete(s_uart_tx_queue);
        s_uart_tx_queue = NULL;
        return s_init_result;
    }

    return start_uart_tx_task();
#endif
}

esp_err_t uart_ipv6_netif_input(
    const uint8_t *packet,
    size_t length)
{
#if !LWIP_IPV6
    (void)packet;
    (void)length;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_started) {
        return ESP_ERR_INVALID_STATE;
    }

    if (packet == NULL ||
        length < IPV6_HEADER_LENGTH ||
        length > UART_IPV6_MAX_PACKET) {
        return ESP_ERR_INVALID_SIZE;
    }

    if ((packet[0] >> 4) != 6) {
        ESP_LOGW(TAG, "Received non-IPv6 UART payload");
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t ipv6_payload_length =
        ((uint16_t)packet[4] << 8) | packet[5];

    if (length != IPV6_HEADER_LENGTH + ipv6_payload_length) {
        ESP_LOGW(TAG, "Received IPv6 packet length mismatch");
        return ESP_ERR_INVALID_SIZE;
    }

    struct pbuf *p = pbuf_alloc(
        PBUF_RAW,
        (u16_t)length,
        PBUF_RAM
    );

    if (p == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (pbuf_take(p, packet, (u16_t)length) != ERR_OK) {
        pbuf_free(p);
        return ESP_FAIL;
    }

    err_t input_err = s_uart_netif.input(p, &s_uart_netif);

    if (input_err != ERR_OK) {
        pbuf_free(p);
        ESP_LOGW(TAG, "lwIP rejected UART IPv6 input");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART -> IPv6: %u bytes", (unsigned)length);
    return ESP_OK;
#endif
}
