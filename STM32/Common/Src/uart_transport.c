#include "uart_transport.h"

#include <string.h>

#define UART_TRANSPORT_MAX_PORTS 2U

static uart_transport_t *registered_ports[UART_TRANSPORT_MAX_PORTS];

static uart_transport_t *find_transport(UART_HandleTypeDef *huart)
{
    for (uint8_t i = 0U; i < UART_TRANSPORT_MAX_PORTS; i++)
    {
        if (registered_ports[i] != NULL &&
            registered_ports[i]->huart == huart)
        {
            return registered_ports[i];
        }
    }

    return NULL;
}

static void service_tx(uart_transport_t *transport)
{
    if (transport->tx_active != 0U &&
        transport->tx_complete != 0U)
    {
        transport->tx_complete = 0U;
        transport->tx_active = 0U;

        transport->tx_head = (uint8_t)(
            (transport->tx_head + 1U) %
            UART_TRANSPORT_TX_QUEUE_DEPTH
        );

        transport->tx_count--;
    }

    if (transport->tx_active != 0U ||
        transport->tx_count == 0U)
    {
        return;
    }

    uart_transport_tx_item_t *item =
        &transport->tx_queue[transport->tx_head];

    transport->tx_complete = 0U;
    transport->tx_active = 1U;

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(
        transport->huart,
        item->bytes,
        item->length
    );

    if (status != HAL_OK)
    {
        transport->tx_active = 0U;

        if (status == HAL_ERROR)
        {
            transport->tx_errors++;
        }

        // Keep the frame queued and retry during a later poll.
    }
}

HAL_StatusTypeDef uart_transport_start(
    uart_transport_t *transport,
    UART_HandleTypeDef *huart,
    uart_transport_frame_callback_t on_frame,
    void *context
)
{
    if (transport == NULL || huart == NULL ||
        huart->hdmarx == NULL || huart->hdmatx == NULL)
    {
        return HAL_ERROR;
    }

    uint8_t slot = UART_TRANSPORT_MAX_PORTS;

    for (uint8_t i = 0U; i < UART_TRANSPORT_MAX_PORTS; i++)
    {
        if (registered_ports[i] == NULL)
        {
            slot = i;
            break;
        }
    }

    if (slot == UART_TRANSPORT_MAX_PORTS)
    {
        return HAL_ERROR;
    }

    memset(transport, 0, sizeof(*transport));

    transport->huart = huart;
    transport->on_frame = on_frame;
    transport->context = context;

    uart_frame_parser_init(&transport->parser);
    registered_ports[slot] = transport;

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(
        huart,
        transport->rx_dma,
        UART_TRANSPORT_RX_DMA_SIZE
    );

    if (status != HAL_OK)
    {
        registered_ports[slot] = NULL;
        return status;
    }

    // Reception uses a circular DMA buffer.
    // The main loop polls its write position.
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);

    return HAL_OK;
}

HAL_StatusTypeDef uart_transport_send(
    uart_transport_t *transport,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
)
{
    if (transport == NULL || transport->huart == NULL)
    {
        return HAL_ERROR;
    }

    if (transport->tx_count == UART_TRANSPORT_TX_QUEUE_DEPTH)
    {
        return HAL_BUSY;
    }

    uart_transport_tx_item_t *item =
        &transport->tx_queue[transport->tx_tail];

    size_t frame_size = uart_frame_encode(
        type,
        sequence,
        payload,
        length,
        item->bytes,
        sizeof(item->bytes)
    );

    if (frame_size == 0U)
    {
        return HAL_ERROR;
    }

    item->length = (uint16_t)frame_size;

    transport->tx_tail = (uint8_t)(
        (transport->tx_tail + 1U) %
        UART_TRANSPORT_TX_QUEUE_DEPTH
    );

    transport->tx_count++;

    return HAL_OK;
}

void uart_transport_poll(uart_transport_t *transport)
{
    if (transport == NULL || transport->huart == NULL)
    {
        return;
    }

    service_tx(transport);

    if (transport->rx_restart_requested != 0U)
    {
        if (HAL_UART_AbortReceive(transport->huart) != HAL_OK)
        {
            return;
        }

        transport->rx_read_pos = 0U;
        uart_frame_parser_init(&transport->parser);
        transport->rx_restart_requested = 0U;

        if (HAL_UART_Receive_DMA(
                transport->huart,
                transport->rx_dma,
                UART_TRANSPORT_RX_DMA_SIZE) != HAL_OK)
        {
            transport->rx_restart_requested = 1U;
            return;
        }

        __HAL_DMA_DISABLE_IT(
            transport->huart->hdmarx,
            DMA_IT_HT
        );
    }

    // Consume all bytes available in the circular RX buffer.
    uint16_t write_pos = (uint16_t)(
        UART_TRANSPORT_RX_DMA_SIZE -
        __HAL_DMA_GET_COUNTER(transport->huart->hdmarx)
    );

    if (write_pos == UART_TRANSPORT_RX_DMA_SIZE)
    {
        write_pos = 0U;
    }

    while (transport->rx_read_pos != write_pos)
    {
        uint8_t byte =
            transport->rx_dma[transport->rx_read_pos];

        transport->rx_read_pos = (uint16_t)(
            (transport->rx_read_pos + 1U) %
            UART_TRANSPORT_RX_DMA_SIZE
        );

        if (uart_frame_parser_push(&transport->parser, byte))
        {
            if (transport->on_frame != NULL)
            {
                transport->on_frame(
                    &transport->parser.frame,
                    transport->context
                );
            }

            service_tx(transport);
        }
    }

    service_tx(transport);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_transport_t *transport = find_transport(huart);

    if (transport != NULL)
    {
        transport->tx_complete = 1U;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uart_transport_t *transport = find_transport(huart);

    if (transport != NULL)
    {
        transport->rx_errors++;
        transport->rx_restart_requested = 1U;
    }
}
