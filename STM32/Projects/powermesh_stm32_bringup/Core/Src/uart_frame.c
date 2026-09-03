#include "uart_frame.h"

#define UART_FRAME_SYNC_1       0xA5U
#define UART_FRAME_SYNC_2       0x5AU
#define UART_FRAME_HEADER_SIZE  6U

static uint16_t read_be16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) |
           ((uint16_t)data[1]);
}

static uint32_t read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           ((uint32_t)data[3]);
}

static void write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)value;
}

static void write_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static uint32_t crc32_update(
    uint32_t crc,
    const uint8_t *data,
    uint16_t length
)
{
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

HAL_StatusTypeDef uart_frame_receive(
    UART_HandleTypeDef *huart,
    uart_frame_t *frame
)
{
    uint8_t byte;
    uint8_t sync_state = 0;
    uint8_t header[UART_FRAME_HEADER_SIZE];
    uint8_t received_crc_bytes[4];

    while (1)
    {
        /*
         * Search the UART stream for the two sync bytes.
         */
        while (sync_state < 2U)
        {
            HAL_StatusTypeDef status = HAL_UART_Receive(
                huart,
                &byte,
                1,
                HAL_MAX_DELAY
            );

            if (status != HAL_OK)
            {
                return status;
            }

            if (sync_state == 0U)
            {
                if (byte == UART_FRAME_SYNC_1)
                {
                    sync_state = 1U;
                }
            }
            else
            {
                if (byte == UART_FRAME_SYNC_2)
                {
                    sync_state = 2U;
                }
                else if (byte != UART_FRAME_SYNC_1)
                {
                    sync_state = 0U;
                }
            }
        }

        sync_state = 0U;

        HAL_StatusTypeDef status = HAL_UART_Receive(
            huart,
            header,
            UART_FRAME_HEADER_SIZE,
            HAL_MAX_DELAY
        );

        if (status != HAL_OK)
        {
            return status;
        }

        frame->version = header[0];
        frame->type = header[1];
        frame->sequence = read_be16(&header[2]);
        frame->length = read_be16(&header[4]);

        if (frame->length > UART_FRAME_MAX_PAYLOAD)
        {
            continue;
        }

        if (frame->length > 0U)
        {
            status = HAL_UART_Receive(
                huart,
                frame->payload,
                frame->length,
                HAL_MAX_DELAY
            );

            if (status != HAL_OK)
            {
                return status;
            }
        }

        status = HAL_UART_Receive(
            huart,
            received_crc_bytes,
            sizeof(received_crc_bytes),
            HAL_MAX_DELAY
        );

        if (status != HAL_OK)
        {
            return status;
        }

        uint32_t calculated_crc = 0xFFFFFFFFU;

        calculated_crc = crc32_update(
            calculated_crc,
            header,
            UART_FRAME_HEADER_SIZE
        );

        calculated_crc = crc32_update(
            calculated_crc,
            frame->payload,
            frame->length
        );

        calculated_crc ^= 0xFFFFFFFFU;

        uint32_t received_crc = read_be32(received_crc_bytes);

        if ((frame->version == UART_FRAME_VERSION) &&
            (calculated_crc == received_crc))
        {
            return HAL_OK;
        }

        /*
         * Bad version or CRC: discard the frame and search again.
         */
    }
}

HAL_StatusTypeDef uart_frame_send(
    UART_HandleTypeDef *huart,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
)
{
    if (length > UART_FRAME_MAX_PAYLOAD)
    {
        return HAL_ERROR;
    }

    if ((length > 0U) && (payload == NULL))
    {
        return HAL_ERROR;
    }

    const uint8_t sync[2] = {
        UART_FRAME_SYNC_1,
        UART_FRAME_SYNC_2
    };

    uint8_t header[UART_FRAME_HEADER_SIZE];

    header[0] = UART_FRAME_VERSION;
    header[1] = type;
    write_be16(&header[2], sequence);
    write_be16(&header[4], length);

    uint32_t crc = 0xFFFFFFFFU;

    crc = crc32_update(crc, header, sizeof(header));
    crc = crc32_update(crc, payload, length);
    crc ^= 0xFFFFFFFFU;

    uint8_t crc_bytes[4];
    write_be32(crc_bytes, crc);

    HAL_StatusTypeDef status;

    status = HAL_UART_Transmit(
        huart,
        (uint8_t *)sync,
        sizeof(sync),
        HAL_MAX_DELAY
    );

    if (status != HAL_OK)
    {
        return status;
    }

    status = HAL_UART_Transmit(
        huart,
        header,
        sizeof(header),
        HAL_MAX_DELAY
    );

    if (status != HAL_OK)
    {
        return status;
    }

    if (length > 0U)
    {
        status = HAL_UART_Transmit(
            huart,
            (uint8_t *)payload,
            length,
            HAL_MAX_DELAY
        );

        if (status != HAL_OK)
        {
            return status;
        }
    }

    return HAL_UART_Transmit(
        huart,
        crc_bytes,
        sizeof(crc_bytes),
        HAL_MAX_DELAY
    );
}