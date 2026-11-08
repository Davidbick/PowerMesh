/**
  ******************************************************************************
  * @file    modbus_task.h
  * @author  AMG/IPC Application Team
  * @brief   This file contains declarations for the ModBus Task.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef MODBUS_TASK_H_
#define MODBUS_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Modbus_Task Print Task
  * @{
  */

/**
  * @}
  */

/** @defgroup Modbus_Task_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/** @defgroup Modbus_Task_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <user_if.h>

#define MODBUS_MASTER_ID	0	/* For master only */
#define MODBUS_SLAVE_ID		1 	/* For slave only */

typedef enum modbus_role_enum
{
	modbus_slave,
	modbus_master
} modbus_role_t;

/* Public Functions */
void UserModbus_init(modbus_role_t role);
void UserModbus_start(void);
void UserModbus_deinit(void);
void UserModbus_exec(void);

#ifdef __cplusplus
}
#endif

#endif /* PRINT_TASK_H_ */
