#include "uart_frame.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "uart_frame"

#define UART_RX_BUFFER_SIZE 4096
#define UART_READ_CHUNK_SIZE 256

#define FRAME_SYNC_1 0xA5
#define FRAME_SYNC_2 0x5A
#define FRAME_VERSION 1

#define FRAME_HEADER_SIZE 6
#define FRAME_CRC_SIZE 4

#define UART_FRAME_OVERHEAD (2 + FRAME_HEADER_SIZE + FRAME_CRC_SIZE)

static uart_port_t s_uart_num = UART_NUM_1;
static SemaphoreHandle_t s_uart_tx_mutex = NULL;

typedef enum {
    PARSER_SYNC_1,
    PARSER_SYNC_2,
    PARSER_HEADER,
    PARSER_PAYLOAD,
    PARSER_CRC
} parser_state_t;

typedef struct {
    parser_state_t state;

    uint8_t header[FRAME_HEADER_SIZE];
    size_t header_pos;

    uint8_t payload[UART_FRAME_MAX_PAYLOAD];
    size_t payload_pos;
    uint16_t payload_length;

    uint8_t crc_bytes[FRAME_CRC_SIZE];
    size_t crc_pos;

    uint8_t frame_type;
    uint16_t sequence;

    uart_frame_callback_t callback;
    void *context;

    uart_frame_stats_t stats;
} frame_parser_t;

static frame_parser_t s_parser;

/*
 * Standard reflected CRC-32, matching zlib.crc32().
 * Polynomial: 0xEDB88320
 */
static uint32_t frame_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (unsigned bit = 0; bit < 8; bit++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

static uint16_t read_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static void parser_reset(frame_parser_t *p)
{
    p->state = PARSER_SYNC_1;
    p->header_pos = 0;
    p->payload_pos = 0;
    p->payload_length = 0;
    p->crc_pos = 0;
}

static void parser_process_byte(frame_parser_t *p, uint8_t byte)
{
    switch (p->state) {
    case PARSER_SYNC_1:
        if (byte == FRAME_SYNC_1) {
            p->state = PARSER_SYNC_2;
        }
        break;

    case PARSER_SYNC_2:
        if (byte == FRAME_SYNC_2) {
            p->header_pos = 0;
            p->state = PARSER_HEADER;
        } else if (byte == FRAME_SYNC_1) {
            p->state = PARSER_SYNC_2;
        } else {
            p->state = PARSER_SYNC_1;
        }
        break;

    case PARSER_HEADER:
        p->header[p->header_pos++] = byte;

        if (p->header_pos == FRAME_HEADER_SIZE) {
            const uint8_t version = p->header[0];

            p->frame_type = p->header[1];
            p->sequence = read_be16(&p->header[2]);
            p->payload_length = read_be16(&p->header[4]);

            if (version != FRAME_VERSION ||
                (p->frame_type != UART_FRAME_TYPE_DATA &&
                 p->frame_type != UART_FRAME_TYPE_ACK)) {
                p->stats.invalid_headers++;
                ESP_LOGW(TAG, "Invalid header: version=%u type=%u", version, p->frame_type);
                parser_reset(p);
                break;
            }

            if (p->payload_length > UART_FRAME_MAX_PAYLOAD) {
                p->stats.invalid_lengths++;
                ESP_LOGW(TAG, "Invalid payload length: %u", p->payload_length);
                parser_reset(p);
                break;
            }

            p->payload_pos = 0;
            p->crc_pos = 0;
            p->state = (p->payload_length == 0) ? PARSER_CRC : PARSER_PAYLOAD;
        }
        break;

    case PARSER_PAYLOAD:
        p->payload[p->payload_pos++] = byte;

        if (p->payload_pos == p->payload_length) {
            p->state = PARSER_CRC;
            p->crc_pos = 0;
        }
        break;

    case PARSER_CRC:
        p->crc_bytes[p->crc_pos++] = byte;

        if (p->crc_pos == FRAME_CRC_SIZE) {
            uint32_t received_crc = read_be32(p->crc_bytes);

            // Compute CRC over contiguous Header + Payload blocks
            uint32_t crc = 0xFFFFFFFF;
            crc = frame_crc32(p->header, FRAME_HEADER_SIZE);
            
            // Re-running standard chunked CRC calculation:
            uint32_t running_crc = 0xFFFFFFFF;
            for (size_t i = 0; i < FRAME_HEADER_SIZE; i++) {
                running_crc ^= p->header[i];
                for (unsigned bit = 0; bit < 8; bit++) {
                    running_crc = (running_crc & 1) ? ((running_crc >> 1) ^ 0xEDB88320) : (running_crc >> 1);
                }
            }
            for (size_t i = 0; i < p->payload_length; i++) {
                running_crc ^= p->payload[i];
                for (unsigned bit = 0; bit < 8; bit++) {
                    running_crc = (running_crc & 1) ? ((running_crc >> 1) ^ 0xEDB88320) : (running_crc >> 1);
                }
            }
            running_crc ^= 0xFFFFFFFF;

            if (running_crc != received_crc) {
                p->stats.crc_errors++;
                ESP_LOGW(TAG, "CRC error: seq=%u received=%08lx calculated=%08lx",
                         p->sequence, (unsigned long)received_crc, (unsigned long)running_crc);
                parser_reset(p);
                break;
            }

            p->stats.frames_ok++;

            if (p->callback != NULL) {
                p->callback(p->frame_type, p->sequence, p->payload, p->payload_length, p->context);
            }

            parser_reset(p);
        }
        break;
    }
}

static void uart_rx_task(void *arg)
{
    uint8_t buffer[UART_READ_CHUNK_SIZE];

    while (true) {
        int count = uart_read_bytes(
            (uart_port_t)(intptr_t)arg,
            buffer,
            sizeof(buffer),
            pdMS_TO_TICKS(100)
        );

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                parser_process_byte(&s_parser, buffer[i]);
            }
        }
    }
}

esp_err_t uart_frame_uart_start(int uart_num, int tx_gpio, int rx_gpio)
{
    s_uart_num = (uart_port_t)uart_num;

    if (s_uart_tx_mutex == NULL) {
        s_uart_tx_mutex = xSemaphoreCreateMutex();
        if (s_uart_tx_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    const uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t err = uart_driver_install((uart_port_t)uart_num, UART_RX_BUFFER_SIZE, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config((uart_port_t)uart_num, &config);
    if (err != ESP_OK) return err;

    err = uart_set_pin((uart_port_t)uart_num, tx_gpio, rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "UART%d initialized at 115200 baud", uart_num);
    return ESP_OK;
}

esp_err_t uart_frame_parser_start(uart_frame_callback_t callback, void *context)
{
    memset(&s_parser, 0, sizeof(s_parser));

    s_parser.callback = callback;
    s_parser.context = context;

    parser_reset(&s_parser);

    BaseType_t created = xTaskCreate(
        uart_rx_task,
        "uart_rx",
        4096,
        (void *)(intptr_t)s_uart_num,
        10,
        NULL
    );

    if (created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void uart_frame_get_stats(uart_frame_stats_t *stats)
{
    if (stats != NULL) {
        *stats = s_parser.stats;
    }
}

esp_err_t uart_frame_send(uint8_t type, uint16_t seq, const uint8_t *payload, uint16_t len)
{
    if (len > UART_FRAME_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (len > 0 && payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_uart_tx_mutex == NULL || xSemaphoreTake(s_uart_tx_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    uint8_t frame[UART_FRAME_OVERHEAD + UART_FRAME_MAX_PAYLOAD];

    frame[0] = FRAME_SYNC_1;
    frame[1] = FRAME_SYNC_2;

    frame[2] = FRAME_VERSION;
    frame[3] = type;
    frame[4] = (uint8_t)(seq >> 8);
    frame[5] = (uint8_t)(seq & 0xFF);
    frame[6] = (uint8_t)(len >> 8);
    frame[7] = (uint8_t)(len & 0xFF);

    if (len > 0) {
        memcpy(&frame[8], payload, len);
    }

    // CRC covers version + type + sequence + length + payload.
    uint32_t crc = frame_crc32(&frame[2], 6 + len);

    size_t crc_offset = 8 + len;

    frame[crc_offset]     = (uint8_t)(crc >> 24);
    frame[crc_offset + 1] = (uint8_t)(crc >> 16);
    frame[crc_offset + 2] = (uint8_t)(crc >> 8);
    frame[crc_offset + 3] = (uint8_t)(crc);

    size_t frame_len = UART_FRAME_OVERHEAD + len;

    int written = uart_write_bytes(s_uart_num, (const char *)frame, frame_len);

    xSemaphoreGive(s_uart_tx_mutex);

    if (written != (int)frame_len) {
        return ESP_FAIL;
    }

    return ESP_OK;
}