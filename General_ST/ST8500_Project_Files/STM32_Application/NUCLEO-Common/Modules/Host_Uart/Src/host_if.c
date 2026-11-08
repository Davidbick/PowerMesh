/**
  ******************************************************************************
  * @file    host_if.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that implements the Host Interface handling
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/


/* Inclusions*/
#include <string.h>
#include <crc.h>
#include <utils.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <task_comm.h>
#include <main.h>
#include <host_if.h>

/** @defgroup g3_hif_uart G3 Host Interface
  * @{
  */

/* Custom types */
typedef enum host_if_rx_state_enum
{
	host_if_rx_st_first_byte = 0,
	host_if_rx_st_preamble,
	host_if_rx_st_payload,
	host_if_rx_st_number,
} host_if_rx_state_t;

#pragma pack(push, 1)

/* G3 request message structure */
typedef struct host_if_g3_tx_msg_str
{
    uint16_t    sync;
    uint8_t     cmd_id;
    uint16_t    len;
    uint8_t     mode;
    uint32_t    cnt;
    uint8_t     data[];
} host_if_g3_tx_msg_t;

/* G3 confirm/indication message structure */
typedef struct host_if_g3_rx_msg_str
{
    uint16_t    sync;
    uint8_t     cmd_id;
    uint16_t    len;
    uint8_t     mode;
    uint32_t    cnt;
    uint8_t     ec;
    uint8_t     data[];
} host_if_g3_rx_msg_t;

#pragma pack(pop)

typedef struct host_if_rx_handler_str
{
	host_if_rx_state_t	recv_state;		/* Current reception state */
    host_if_msg_rx_t 	*hif_msg_rx;	/* Contains the three parts of a G3 message (preamble, payload, crc16) */
} host_if_rx_handler_t;

typedef struct host_if_tx_handler_str
{
	host_if_g3_tx_msg_t		*hif_msg_tx;
} host_if_tx_handler_t;

/* External Variables */
extern osMessageQueueId_t	host_if_queueHandle;

extern osSemaphoreId_t 		semHostIfTxCompleteHandle;

/* Private variables */
static host_if_rx_handler_t rx_handler;
static host_if_tx_handler_t tx_handler;

static uint8_t huartHostIf_data;

/* Private functions */

/**
  * @brief This functions initializes the input buffer of the Host Interface.
  * @param None
  * @retval None
  */
static inline void host_if_init_rx_buffer(void)
{
	rx_handler.recv_state = host_if_rx_st_first_byte;

	/* Prepare reception buffer for the first incoming message */
	rx_handler.hif_msg_rx = MEMPOOL_MALLOC(sizeof(host_if_msg_rx_t));
}

/* Public functions */

/**
  * @brief This functions initializes the Host Interface (HIF).
  * @param None
  * @retval None
  */
void host_if_init(void)
{
	/* Initialize TX buffer */
	tx_handler.hif_msg_tx = NULL;

	/* Initialize RX buffer */
	host_if_init_rx_buffer();

	/* Start reception */
	host_if_rx_start();
}

/**
  * @brief This functions starts interrupt reception on the Host Interface.
  * @param None
  * @retval None
  */
inline void host_if_rx_start(void)
{
	/* Starts listening for the first byte */
	HAL_UART_Receive_IT(&huartHostIf, &huartHostIf_data, sizeof(huartHostIf_data));
}

/**
  * @brief This functions stops interrupt reception on the Host Interface.
  * @param None
  * @retval None
  */
inline void host_if_rx_stop(void)
{
	/* Stops listening */
	HAL_UART_Abort_IT(&huartHostIf);
}

/**
  * @brief This functions handles the reception of messages through the HIF.
  * @param None
  * @retval None
  */
void host_if_rx_handler(void)
{
	if (rx_handler.recv_state == host_if_rx_st_first_byte)
	{
		if (huartHostIf_data == HIF_PREAMBLE_BYTE_VALUE)
		{
			/* Sets the first receive byte */
			rx_handler.hif_msg_rx->preamble[0] = huartHostIf_data;

			/* Starts the reception of the rest of the preamble + the EC */
			HAL_UART_Receive_DMA(&huartHostIf, &rx_handler.hif_msg_rx->preamble[1], HIF_REM_PREAMBLE_AND_EC_LEN);

			/* Change reception state */
			rx_handler.recv_state = host_if_rx_st_preamble;
		}
		else
		{
			/* Discards byte */
			HAL_UART_Receive_IT(&huartHostIf, &huartHostIf_data, sizeof(huartHostIf_data));
		}
	}
	else if (rx_handler.recv_state == host_if_rx_st_preamble)
	{
		rx_handler.hif_msg_rx->payload_len	= HIF_GET_MSG_LEN(rx_handler.hif_msg_rx) - HIF_EC_LEN; 	/* Length excludes the EC field (only pure payload) */
		uint16_t remaining_bytes		    = rx_handler.hif_msg_rx->payload_len + HIF_CRC_LEN;	/* Payload (without EC) and CRC16 are still to be received */

		/* This pool will transport the payload after the forwarding. Two extra bytes for the CRC are allocated */
		rx_handler.hif_msg_rx->payload_crc = MEMPOOL_MALLOC(remaining_bytes);

		/* Starts the reception of the rest of the message */
		HAL_UART_Receive_DMA(&huartHostIf, rx_handler.hif_msg_rx->payload_crc, remaining_bytes);

		/* Change reception state */
		rx_handler.recv_state = host_if_rx_st_payload;
	}
    else if (rx_handler.recv_state == host_if_rx_st_payload)
    {
    	/* Put message in the HIF task message queue */
		RTOS_PUT_MSG(host_if_queueHandle, HIF_RX_MSG, rx_handler.hif_msg_rx);

		/* Prepare to receive another message (also reinitializes the state). Does not free the pool because it is needed to preserve the message */
		host_if_init_rx_buffer();

		/* Restart reception */
		host_if_rx_start();
    }
    else
    {
    	Error_Handler(); /* Unexpected reception state */
    }
}

/**
  * @brief This functions handles the transmission completion of the HIF.
  * @param None
  * @retval None
  */
void host_if_tx_handler(void)
{
	MEMPOOL_FREE(tx_handler.hif_msg_tx);

	/* Unblocks the Print task after the data has been sent on User Interface UART */
	osSemaphoreRelease(semHostIfTxCompleteHandle);
}

/**
  * @brief This functions handles the transmission of a G3 message through the Host Interface.
  * @param cmd_id Command ID of the message to send.
  * @param payload Pointer to the payload of the message to send.
  * @param payload_len Length of the payload to send.
  * @return Number of bytes sent (0 in case of error).
  */
uint32_t host_if_send_message(uint8_t cmd_id, void *payload, uint16_t payload_len)
{
    crc16_t crc;
    uint16_t msg_len = 0;
    
    /* Waits for the previous message to be sent */
    osSemaphoreAcquire(semHostIfTxCompleteHandle, osWaitForever);

    assert(tx_handler.hif_msg_tx == NULL);

	/* Total message length */
	msg_len = sizeof(host_if_g3_tx_msg_t) + payload_len + sizeof(crc);

	tx_handler.hif_msg_tx = MEMPOOL_MALLOC(msg_len);

	assert(tx_handler.hif_msg_tx != NULL);

	tx_handler.hif_msg_tx->sync   = HIF_PREAMBLE_FIELD_VALUE;
	tx_handler.hif_msg_tx->cmd_id = cmd_id;
	tx_handler.hif_msg_tx->len    = payload_len;
	tx_handler.hif_msg_tx->mode   = 0U;
	tx_handler.hif_msg_tx->cnt    = 0U;

	if ((payload != NULL) && (payload_len > 0))
	{
		memcpy(tx_handler.hif_msg_tx->data, payload, payload_len);
	}

	/* Calculates CRC16 on preamble+payload */
	crc = CRC16_XMODEM(tx_handler.hif_msg_tx, sizeof(*tx_handler.hif_msg_tx) + payload_len);

	/* Inserts the CRC16 in little endian after the payload */
	tx_handler.hif_msg_tx->data[payload_len]     = LOW_BYTE(crc);
	tx_handler.hif_msg_tx->data[payload_len + 1] = HIGH_BYTE(crc);

#if (DEBUG_G3_MSG >= DEBUG_LEVEL_FULL)

	/* Due to the large amount of data to print, the print is split in multiple lines, handled separately */
	const uint32_t max_bytes_per_line = 64;
	uint32_t printed_bytes = 0;
	uint32_t line = 0;

	while (printed_bytes < tx_handler.hif_msg_tx->len)
	{
		uint32_t next_print;

		if (tx_handler.hif_msg_tx->len > max_bytes_per_line)
		{
			next_print = max_bytes_per_line;
		}
		else
		{
			next_print = tx_handler.hif_msg_tx->len;
		}

		ALLOC_DYNAMIC_HEX_STRING(msg_str, &tx_handler.hif_msg_tx->data[line*max_bytes_per_line], next_print);
		if (line == 0)
		{
			PRINT_G3_MSG_INFO("TX:\t%s\n", msg_str);
		}
		else
		{
			PRINT_G3_MSG_INFO("TX%u:\t%s\n", line+1, msg_str);
		}

		FREE_DYNAMIC_HEX_STRING(msg_str);

		line++;
		printed_bytes += next_print;
	}
#endif

	if (HAL_UART_Transmit_DMA(&huartHostIf, (uint8_t*) tx_handler.hif_msg_tx, msg_len) != HAL_OK)
	{
		Error_Handler();
	}

	PRINT_G3_MSG_INFO("Sent -> %s (0x%X), %u bytes\n", translateG3cmd(cmd_id), cmd_id, msg_len);

    return msg_len;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
