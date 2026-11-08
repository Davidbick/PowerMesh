/**
  ******************************************************************************
  * @file    host_if_task.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that implements the Host Interface task
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
#include <g3_comm.h>
#include <main.h>
#include <host_if.h>

/** @defgroup g3_hif_uart G3 Host Interface
  * @{
  */

/* External Variables */
extern osMessageQueueId_t 	host_if_queueHandle;
extern osMessageQueueId_t 	g3_queueHandle;

extern osSemaphoreId_t 		semConfirmationHandle;

/* Private variables */


/**
  * @brief This functions parses a Host Interface message, verifying its integrity.
  * @param hif_msg Pointer to the Host Interface message
  * @retval 'true' if the received Host Interface message is valid, 'false' otherwise
  */
static bool host_if_parse_message(host_if_msg_rx_t *hif_msg)
{
	/* Message integrity check */
	bool integrity_ok = false;

	/* Gets SYNC from received message */
	uint16_t sync = HIF_GET_SYNC(hif_msg);

	/* Message validation (sync field) */
	if (sync == HIF_PREAMBLE_FIELD_VALUE)
	{
		/* Gets CRC from received message */
		uint16_t crc_recv = HIF_GET_REP_CRC16(hif_msg);

		/* Calculates CRC on preamble + EC */
		uint16_t crc_calc = CRC16_XMODEM(hif_msg->preamble, sizeof(hif_msg->preamble));

		/* Calculates CRC on payload (EC excluded) */
		crc_calc = crc16_generic(hif_msg->payload_crc, hif_msg->payload_len, crc_calc);

		/* Message validation (CRC field) */
		if (crc_recv == crc_calc)
		{
			/* Gets Error Code from message */
			uint8_t error_code = HIF_GET_REP_EC(hif_msg);

			/* Message validation (Error Code) */
			if (error_code == 0)
			{
				integrity_ok = true;
			}
			else
			{
				PRINT_G3_MSG_CRITICAL("EC: 0x%02X\n", error_code);
			}
		}
		else
		{
			PRINT_G3_MSG_CRITICAL("Invalid CRC16: %04X instead of %04X\n", crc_recv, crc_calc);
		}
	}
	else
	{
		PRINT_G3_MSG_CRITICAL("Invalid SYNC: %04X instead of %04X\n", sync, HIF_PREAMBLE_FIELD_VALUE);
	}

	return integrity_ok;
}

/* Public functions */

/**
 * @brief This functions initializes the HIF task.
 * @param None
 * @retval None
 */
void host_if_task_init()
{
	/* No action */
}

/**
 * @brief This functions executes the HIF task routine.
 * @param None
 * @retval None
 */
void host_if_task_exec()
{
	task_msg_t 			task_msg;
	host_if_msg_rx_t	*hif_msg;

	for(;;)
	{
		/* Reception of messages */
		if (RTOS_GET_MSG(host_if_queueHandle, &task_msg))
		{
			hif_msg = task_msg.data; /* The payload of the message is a Host Interface (HIF) message */

			/* Parses only HIF message received from the Host UART */
			if (task_msg.message_type == HIF_RX_MSG)
			{
				/* Verifies and parses a message from the Host Interface */
				if (host_if_parse_message(hif_msg))
				{
					/* Gets command ID from the message */
					hif_cmd_id_t cmd_id	= HIF_GET_CMD_ID(hif_msg);

					/* Releases confirmation semaphore if the received message is a confirm */
					switch(cmd_id)
					{
					case HIF_HI_HWRESET_CNF:
					//case HIF_HI_BAUDRATE_SET_CNF: /* deprecated, do not use */
					case HIF_HI_SFLASH_CNF:
					case HIF_HI_NVM_CNF:
					case HIF_HI_DBGTOOL_CNF:
					case HIF_HI_RFCONFIGSET_CNF:
					case HIF_HI_RFCONFIGGET_CNF:
					case HIF_HI_HWCONFIG_CNF:
					case HIF_G3LIB_GET_CNF:
					case HIF_G3LIB_SET_CNF:
					case HIF_G3LIB_SWRESET_CNF:
					case HIF_MCPS_DATA_CNF:
					case HIF_ADPM_DISCOVERY_CNF:
					case HIF_ADPM_NTWSTART_CNF:
					case HIF_ADPM_NTWJOIN_CNF:
					case HIF_ADPM_NTWLEAVE_CNF:
					case HIF_ADPM_ROUTEDISCO_CNF:
					case HIF_ADPM_LBP_CNF:
					case HIF_BOOT_SRV_START_CNF:
					case HIF_BOOT_SRV_STOP_CNF:
					case HIF_BOOT_SRV_KICK_CNF:
					case HIF_BOOT_DEV_LEAVE_CNF:
					case HIF_BOOT_DEV_PANSORT_CNF:
					case HIF_BOOT_SRV_SETPSK_CNF:
					case HIF_UDP_DATA_CNF:
					case HIF_UDP_CONN_SET_CNF:
					case HIF_UDP_CONN_GET_CNF:
					case HIF_ICMP_ECHO_CNF:
					/* In case more CNF message handlers are added, add the CNF ID here */
						osSemaphoreRelease(semConfirmationHandle);
						break;
					default:
						break;
					};

					/* Sends the message to the G3 task, without copying it, since the payload is already allocated in a memory pool */
					/* The pool where the Host interface message payload is allocated lives on in the buffer of the G3 message */
					g3_send_message(G3_RX_MSG, cmd_id, hif_msg->payload_crc, hif_msg->payload_len);
				}
				else
				{
					/* Discards the payload of the message */
					MEMPOOL_FREE(hif_msg->payload_crc);
				}

				/* Frees the memory pool that was allocated by the Host Interface reception handler for the received message */
				MEMPOOL_FREE(hif_msg);
			}
			else
			{
				Error_Handler(); /* Unexpected task message type */
			}
		}
	}
}


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
