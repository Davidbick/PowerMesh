/**
  ******************************************************************************
  * @file    g3_comm.c
  * @author  AMG/IPC Application Team
  * @brief   G3 Communication implementation.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

/* Inclusions */
#include <string.h>
#include <g3_comm.h>
#include <mem_pool.h>
#include <main.h>

/* External Variables */
extern osMessageQueueId_t g3_queueHandle;

/* Public Functions */

/**
  * @brief Function that builds a G3 message, without copying the payload, and sends it to the G3 message queue.
  * @param msg_type The type of message
  * @param msg_id The command ID of the message
  * @param payload_pool Pointer to the memory pool containing payload of the message
  * @param payload_len Length of the payload, in bytes
  * @retval None
  * @Note In order to avoid unnecessary copies of data:
  * 		1) allocate a memory pool of the size of the payload to send
  * 		2) fill payload with the target data
  * 		3) call this function passing the memory pool as "payload_pool". The memory pool address will be sent without copying the payload.
  */
void g3_send_message(msg_type_t msg_type, hif_cmd_id_t msg_id, void *payload_pool, uint16_t payload_len)
{
	/* Allocate new G3 message */
	g3_msg_t *g3_msg = MEMPOOL_MALLOC(sizeof(g3_msg_t));

	/* Set message ID, length and payload pointer */
	g3_msg->command_id  = msg_id;
	g3_msg->size        = payload_len;
	g3_msg->payload     = payload_pool;

	/* Put message in the G3 task message queue */
	RTOS_PUT_MSG(g3_queueHandle, msg_type, g3_msg);
}

/**
  * @brief Function that builds a G3 message, copying or passing the payload, and sends it to the G3 message queue.
  * @param msg_type The type of message
  * @param msg_id The command ID of the message
  * @param payload Pointer to the payload of the message (that is copied to a new memory pool)
  * @param payload_len Length of the payload, in bytes
  * @retval None
  * @Note Less efficient than calling directly "g3_send_message" with a already filled memory pool (unnecessary copy)
  */
void g3_copy_and_send_message(msg_type_t msg_type, hif_cmd_id_t msg_id, void *payload, uint16_t payload_len)
{
	void *payload_pool;

	/* Allocate and copies the payload, if present. Set buffer to NULL, otherwise */
	if (payload_len > 0)
	{
		payload_pool = MEMPOOL_MALLOC(payload_len);  	/* Allocates the message payload in a new memory pool */
		memcpy(payload_pool, payload, payload_len);		/* Copies the payload to the allocated memory */
	}
	else
	{
		payload_pool = NULL; /* Message with empty payload */
	}

	/* Put message in the G3 task message queue */
	g3_send_message(msg_type, msg_id, payload_pool, payload_len);
}

/**
  * @brief Function that discards a G3 message, freeing the memory pools allocated for it and its payload.
  * @param g3_msg The G3 message to discard
  * @retval None
  */
void g3_discard_message(g3_msg_t *g3_msg)
{
	if (g3_msg != NULL)
	{
		if (g3_msg->payload != NULL)
		{
			MEMPOOL_FREE(g3_msg->payload); /* Free memory pool used for the payload */
		}

		MEMPOOL_FREE(g3_msg); /* Free memory pool used for the message */
	}
}
