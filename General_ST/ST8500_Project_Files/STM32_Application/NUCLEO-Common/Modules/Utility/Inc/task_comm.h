/**
  ******************************************************************************
  * @file    task_comm.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the task communication.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef TASK_COMM_H_
#define TASK_COMM_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <cmsis_os.h>
#include <settings.h>
#include <hif_g3_common.h>

/* Constant values */
#define NO_WAIT         	0U
#define MIN_WAIT        	1U
#define WAIT_FOREVER    	osWaitForever

#define DEFAULT_MSG_PRIO	0U

/* Macros for task communication */
#define RTOS_PUT_MSG(queueID, message_type, data) 					taskCommPut(queueID, message_type, (void*) data, DEFAULT_MSG_PRIO, NO_WAIT) /* Non-blocking function */
#define RTOS_PUT_MSG_TIMEOUT(queueID, message_type, data, timeout) 	taskCommPut(queueID, message_type, (void*) data, DEFAULT_MSG_PRIO, timeout)	/* Blocking function until a message is sent or the timeout is reached */
#define RTOS_GET_MSG(queueID, msg) 									taskCommGet(queueID, msg, NULL, WAIT_FOREVER) 								/* Blocking function until a message is received */
#define RTOS_GET_MSG_TIMEOUT(queueID, msg, timeout) 				taskCommGet(queueID, msg, NULL, timeout)      								/* Blocking function until a message is received or the timeout is reached */
#define RTOS_MSG_AVAILABLE(queueID)									(osMessageQueueGetCount(queueID) > 0)										/* Returns true if a message is available */

/* Boot communication */
#if IS_COORD
#if ENABLE_BOOT_SERVER_ON_HOST
#define BOOT_SERVER_MSG_TYPE			BOOT_SRV_MSG	/* Sends the message to Boot Server application instead of the ST8500 */
#else
#define BOOT_SERVER_MSG_TYPE			HIF_TX_MSG		/* Sends the message through the serial to the ST8500 */
#endif
#else
#if ENABLE_BOOT_CLIENT_ON_HOST
#define BOOT_CLIENT_MSG_TYPE			BOOT_CLT_MSG	/* Sends the message to Boot Client application instead of the ST8500 */
#else
#define BOOT_CLIENT_MSG_TYPE			HIF_TX_MSG		/* Sends the message through the serial to the ST8500 */
#endif
#endif

typedef enum msg_type_enum
{
   HIF_TX_MSG = 0,	/* Messages to be sent through the Host Interface (payload only) */
   HIF_RX_MSG,		/* Messages received through the Host Interface (preamble + payload + crc) */
   G3_RX_MSG,		/* Messages to be processed by the G3 task (payload only) */
   BOOT_SRV_MSG,	/* Messages reserved for the Boot Server module */
   BOOT_REKEY_MSG,	/* Messages reserved for the Boot Server module (re-keying) */
   BOOT_CLT_MSG,	/* Messages reserved for the Boot Client module */
   KA_MSG,			/* Messages reserved for the Boot module */
   LAST_GASP_MSG,	/* Messages reserved for the Last Gasp module */
   USER_MSG,		/* Messages reserved for the user application */
   SFLASH_MSG,		/* Messages reserved for the SFlash task */
   MSG_TYPE_CNT
/* New entries can be added here to implement another type of message */
} msg_type_t;

/* Task Message Structure */
typedef struct task_msg_str
{
   msg_type_t message_type;
   void       *data;
} task_msg_t;


/* Public Functions */
bool taskCommGet(osMessageQueueId_t queueID, void *msg_ptr, uint8_t* msg_prio, uint32_t timeout);
bool taskCommPut(osMessageQueueId_t queueID, msg_type_t message_type, void * const data, uint8_t msg_prio, uint32_t timeout);

#endif /* TASK_COMM_H_ */
