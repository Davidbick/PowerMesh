#include "st8500_boot.h"
#include "powermesh_board.h"

#include <string.h>

#define ST8500_BOOT_BAUD        9600U
#define ST8500_UPLOAD_BAUD      115200U
#define ST8500_CHUNK_SIZE       1024U
#define ST8500_HEADER_SIZE      10U
#define ST8500_FRAME_OVERHEAD   12U
#define ST8500_TIMEOUT_MS       1500U
#define ST8500_MAX_ATTEMPTS     3U

#define ST8500_COM_INIT         0x01U
#define ST8500_IMG_INIT         0x11U
#define ST8500_IMG_WRITE        0x12U
#define ST8500_IMG_START        0x13U
#define ST8500_MIB_GET          0x21U
#define ST8500_MIB_VERSION      0x04U

#define ST8500_RTE_TYPE         0x0000A3A3U
#define ST8500_PE_TYPE          0x0000F1F1U

extern const uint8_t st8500_rte_start[];
extern const uint8_t st8500_rte_end[];
extern const uint8_t st8500_pe_start[];
extern const uint8_t st8500_pe_end[];

volatile st8500_boot_stage_t st8500_boot_stage = ST8500_BOOT_IDLE;
volatile uint32_t st8500_boot_bytes_written = 0U;
volatile uint32_t st8500_boot_attempt = 0U;

static uint8_t tx_frame[ST8500_CHUNK_SIZE + ST8500_FRAME_OVERHEAD];

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint16_t crc16_xmodem(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0U;

    for (uint16_t i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static bool set_baudrate(UART_HandleTypeDef *uart, uint32_t baudrate)
{
    if (HAL_UART_DeInit(uart) != HAL_OK)
    {
        return false;
    }

    uart->Init.BaudRate = baudrate;

    return HAL_UART_Init(uart) == HAL_OK;
}

static bool send_command(
    UART_HandleTypeDef *uart,
    uint8_t command,
    const uint8_t *data,
    uint16_t data_length,
    uint16_t payload_length,
    uint16_t expected_length
)
{
    if (payload_length > ST8500_CHUNK_SIZE ||
        data_length > payload_length ||
        expected_length > 4U ||
        (data_length != 0U && data == NULL))
    {
        return false;
    }

    tx_frame[0] = 0x16U;
    tx_frame[1] = 0x16U;
    tx_frame[2] = command;
    tx_frame[3] = (uint8_t)payload_length;
    tx_frame[4] = (uint8_t)(payload_length >> 8);
    tx_frame[5] = 0xFEU;

    memset(&tx_frame[6], 0, 4U);

    if (data_length != 0U)
    {
        memcpy(&tx_frame[ST8500_HEADER_SIZE], data, data_length);
    }

    if (payload_length > data_length)
    {
        memset(
            &tx_frame[ST8500_HEADER_SIZE + data_length],
            0,
            payload_length - data_length
        );
    }

    uint16_t frame_length =
        ST8500_HEADER_SIZE + payload_length;

    uint16_t crc = crc16_xmodem(tx_frame, frame_length);

    tx_frame[frame_length] = (uint8_t)crc;
    tx_frame[frame_length + 1U] = (uint8_t)(crc >> 8);

    frame_length += 2U;

    for (uint8_t attempt = 0U; attempt < 3U; attempt++)
    {
        uint8_t response[16];

        if (HAL_UART_Transmit(
                uart,
                tx_frame,
                frame_length,
                ST8500_TIMEOUT_MS) != HAL_OK)
        {
            return false;
        }

        if (HAL_UART_Receive(
                uart,
                response,
                ST8500_HEADER_SIZE,
                ST8500_TIMEOUT_MS) != HAL_OK)
        {
            return false;
        }

        uint16_t response_length =
            (uint16_t)response[3] |
            ((uint16_t)response[4] << 8);

        if (response[0] != 0x16U ||
            response[1] != 0x16U ||
            response[2] != command ||
            response[5] != 0xFEU ||
            response_length != expected_length)
        {
            return false;
        }

        uint16_t total_length =
            ST8500_HEADER_SIZE + response_length + 2U;

        if (HAL_UART_Receive(
                uart,
                &response[ST8500_HEADER_SIZE],
                response_length + 2U,
                ST8500_TIMEOUT_MS) != HAL_OK)
        {
            return false;
        }

        uint16_t received_crc =
            (uint16_t)response[total_length - 2U] |
            ((uint16_t)response[total_length - 1U] << 8);

        uint16_t calculated_crc =
            crc16_xmodem(response, total_length - 2U);

        if (received_crc != calculated_crc)
        {
            return false;
        }

        if (response[6] == 0x00U &&
            response[7] == 0x00U &&
            response[8] == 0x00U &&
            response[9] == 0x00U)
        {
            return true;
        }

        if (response[6] != 0x01U)
        {
            return false;
        }
    }

    return false;
}

static bool load_image(
    UART_HandleTypeDef *uart,
    const uint8_t *start,
    const uint8_t *end,
    uint32_t expected_type,
    bool rte
)
{
    if ((uintptr_t)end <= (uintptr_t)start)
    {
        return false;
    }

    size_t image_size =
        (size_t)((uintptr_t)end - (uintptr_t)start);

    if (image_size < 88U ||
        read_u32(start) != expected_type)
    {
        return false;
    }

    uint32_t sections = read_u32(start + 0x54U);

    if (sections == 0U || sections > 24U)
    {
        return false;
    }

    size_t header_length = 88U + sections * 8U;

    if ((sections & 1U) == 0U)
    {
        header_length += 8U;
    }

    if (header_length >= image_size ||
        (header_length & 15U) != 0U)
    {
        return false;
    }

    st8500_boot_stage = rte ?
        ST8500_BOOT_RTE_HEADER : ST8500_BOOT_PE_HEADER;

    if (!send_command(
            uart,
            ST8500_IMG_INIT,
            start,
            (uint16_t)header_length,
            (uint16_t)header_length,
            0U))
    {
        return false;
    }

    st8500_boot_stage = rte ?
        ST8500_BOOT_RTE_DATA : ST8500_BOOT_PE_DATA;

    for (size_t offset = header_length;
         offset < image_size;)
    {
        size_t remaining = image_size - offset;

        uint16_t actual_length = (uint16_t)(
            remaining > ST8500_CHUNK_SIZE ?
            ST8500_CHUNK_SIZE : remaining
        );

        uint16_t padded_length =
            (uint16_t)((actual_length + 15U) & ~15U);

        if (!send_command(
                uart,
                ST8500_IMG_WRITE,
                start + offset,
                actual_length,
                padded_length,
                0U))
        {
            return false;
        }

        offset += actual_length;
        st8500_boot_bytes_written += actual_length;
    }

    st8500_boot_stage = rte ?
        ST8500_BOOT_RTE_START : ST8500_BOOT_PE_START;

    return send_command(
        uart,
        ST8500_IMG_START,
        NULL,
        0U,
        0U,
        0U
    );
}

static bool boot_once(UART_HandleTypeDef *uart)
{
    st8500_boot_stage = ST8500_BOOT_RESET;
    st8500_boot_bytes_written = 0U;

    if (!set_baudrate(uart, ST8500_BOOT_BAUD))
    {
        return false;
    }

    HAL_GPIO_WritePin(
        ST8500_RESET_N_GPIO_Port,
        ST8500_RESET_N_Pin,
        GPIO_PIN_RESET
    );

    HAL_Delay(20U);

    HAL_GPIO_WritePin(
        ST8500_RESET_N_GPIO_Port,
        ST8500_RESET_N_Pin,
        GPIO_PIN_SET
    );

    HAL_Delay(150U);

    st8500_boot_stage = ST8500_BOOT_PROBE;

    uint8_t mib = ST8500_MIB_VERSION;

    if (!send_command(
            uart,
            ST8500_MIB_GET,
            &mib,
            1U,
            1U,
            4U))
    {
        return false;
    }

    st8500_boot_stage = ST8500_BOOT_BAUD;

    const uint32_t baudrate = ST8500_UPLOAD_BAUD;

    uint8_t config[6] = {
        (uint8_t)baudrate,
        (uint8_t)(baudrate >> 8),
        (uint8_t)(baudrate >> 16),
        (uint8_t)(baudrate >> 24),
        0U,
        1U
    };

    if (!send_command(
            uart,
            ST8500_COM_INIT,
            config,
            sizeof(config),
            sizeof(config),
            0U))
    {
        return false;
    }

    if (!set_baudrate(uart, ST8500_UPLOAD_BAUD))
    {
        return false;
    }

    if (!load_image(
            uart,
            st8500_rte_start,
            st8500_rte_end,
            ST8500_RTE_TYPE,
            true))
    {
        return false;
    }

    if (!load_image(
            uart,
            st8500_pe_start,
            st8500_pe_end,
            ST8500_PE_TYPE,
            false))
    {
        return false;
    }

    st8500_boot_stage = ST8500_BOOT_IMAGES_STARTED;

    return true;
}

bool st8500_boot_load(UART_HandleTypeDef *uart)
{
    if (uart == NULL)
    {
        return false;
    }

    for (uint32_t i = 0U; i < ST8500_MAX_ATTEMPTS; i++)
    {
        st8500_boot_attempt = i + 1U;

        if (boot_once(uart))
        {
            return true;
        }

        HAL_Delay(200U);
    }

    return false;
}
