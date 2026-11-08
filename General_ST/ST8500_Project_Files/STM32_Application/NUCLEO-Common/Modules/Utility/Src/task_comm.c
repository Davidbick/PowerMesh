/**
  ******************************************************************************
  * @file    task_comm.c
  * @author  AMG/IPC Application Team
  * @brief   Task Communication implementation.
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
#include <task_comm.h>

/**
  * @brief  Function that puts a message in the given queue.
  * @param  QueueId Queue in which the message should be put.
  * @param  message_type Type of the message (must be one of the "msg_type_t" values).
  * @param  msg_buffer Pointer to the message buffer.
  * @param  WaitTime Waiting time on OS side.
  * @retval Boolean that indicates success.
  */
bool taskCommPut(osMessageQueueId_t queueID, msg_type_t message_type, void * const data, uint8_t msg_prio, uint32_t timeout)
{
	osStatus_t result;
	task_msg_t msg;

	msg.message_type	= message_type;
	msg.data			= data;

	result = osMessageQueuePut(queueID, &msg, msg_prio, timeout);

	assert(result == osOK); /* If result != osOk, there has been an issue; result value must be checked to understand the error */

	return (result == osOK);
}


/**
  * @brief  Function that checks if a message is received in the given queue.
  * @param  QueueId Queue from which the message should be got.
  * @param  Buff Ptr to the message to send.
  * @param  WaitTime Waiting time on OS side.
  * @retval Boolean that indicates success.
  */
bool taskCommGet(osMessageQueueId_t queueID, void *msg_ptr, uint8_t* msg_prio, uint32_t timeout)
{
	osStatus_t result = osMessageQueueGet(queueID, msg_ptr, msg_prio, timeout);

    return (result == osOK);
}
