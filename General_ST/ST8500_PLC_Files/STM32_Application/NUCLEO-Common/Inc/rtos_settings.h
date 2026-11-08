/**
  ******************************************************************************
  * @file    rtos_settings.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the RTOS settings.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef RTOS_SETTINGS_H_
#define RTOS_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Stack sizes in words (4 bytes each) */
#define G3_TASK_STACK_SIZE				384
#define USER_TASK_STACK_SIZE			320
#define PRINT_TASK_STACK_SIZE			80

#define HOST_IF_TASK_STACK_SIZE			192

#define SFLASH_TASK_STACK_SIZE			192

/* Maximum number of elements in each queue */
#define G3_QUEUE_LENGTH					8
#define USER_QUEUE_LENGTH				8
#define HOST_IF_QUEUE_LENGTH			16
#define SFLASH_QUEUE_LENGTH				8

/* Size of each queue element, in bytes */
#define G3_QUEUE_SIZE					sizeof(task_msg_t)
#define USER_QUEUE_SIZE					sizeof(task_msg_t)
#define HOST_IF_QUEUE_SIZE				sizeof(task_msg_t)
#define SFLASH_QUEUE_SIZE				sizeof(task_msg_t)

/* Counting semaphore values */
#define CONFIRMATION_SEMAPHORE_COUNT	2

#ifdef __cplusplus
}
#endif

#endif /* RTOS_SETTINGS_H_ */
