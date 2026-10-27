#include "uart_ipv6_udp_test.h"

#include <errno.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define TAG "uart_ipv6_udp_test"
#define UDP_TEST_PORT 4242
#define UDP_RX_BUFFER_SIZE 256

static void udp_rx_task(void *arg)
{
    (void)arg;

    const int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed, errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in6 local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(UDP_TEST_PORT);

    if (bind(sock, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "bind() port %u failed, errno=%d",
                 (unsigned)UDP_TEST_PORT, errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Listening for IPv6 UDP datagrams on port %u",
             (unsigned)UDP_TEST_PORT);

    while (true) {
        uint8_t rx_buffer[UDP_RX_BUFFER_SIZE];
        struct sockaddr_in6 source_addr;
        socklen_t source_len = sizeof(source_addr);
        memset(&source_addr, 0, sizeof(source_addr));

        const int received = recvfrom(
            sock,
            rx_buffer,
            sizeof(rx_buffer),
            0,
            (struct sockaddr *)&source_addr,
            &source_len);

        if (received < 0) {
            ESP_LOGW(TAG, "recvfrom() failed, errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ESP_LOGI(TAG,
                 "Received UDP datagram: %d bytes, source port %u, scope=%u",
                 received,
                 (unsigned)ntohs(source_addr.sin6_port),
                 (unsigned)source_addr.sin6_scope_id);
        ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, (size_t)received, ESP_LOG_INFO);

        if (received == 13 && memcmp(rx_buffer, "HELLO FROM PI", 13) == 0) {
            ESP_LOGI(TAG, "UDP TEST PASSED: received HELLO FROM PI");
        }

        /*
         * Echo to the sender. If the packet arrived through uaN and the
         * sender's address is link-local, lwIP's scope ID keeps the send on
         * uaN; uart_netif_output_ipv6 then hands this unicast packet to Wi-Fi.
         */
        const int sent = sendto(
            sock,
            rx_buffer,
            (size_t)received,
            0,
            (const struct sockaddr *)&source_addr,
            source_len);

        if (sent < 0) {
            ESP_LOGE(TAG, "UDP echo sendto() failed, errno=%d", errno);
        } else {
            ESP_LOGI(TAG,
                     "UDP echo submitted: %d bytes to source port %u",
                     sent,
                     (unsigned)ntohs(source_addr.sin6_port));
        }
    }
}

esp_err_t uart_ipv6_udp_test_start(void)
{
    const BaseType_t created = xTaskCreate(
        udp_rx_task,
        "uart_ipv6_udp_rx",
        4096,
        NULL,
        5,
        NULL);

    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
