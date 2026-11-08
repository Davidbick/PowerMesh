/******************************************************************************
  * @file    task_routines.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements main task routines functions.
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
#include <cmsis_os.h>
#include <event_groups.h>
#include <main.h>
#include <utils.h>
#include <hi_boot_sap_interface.h>
#include <g3_task.h>
#include <host_if_task.h>
#include <user_task.h>
#include <print_task.h>
#include <sflash_task.h>

/* Definitions */

/* Task synchronization bits */
#define HOST_IF_TASK_BIT 		(1 << 0)
#define PRINT_TASK_BIT 			(1 << 1)
#define SFLASH_TASK_BIT 		(1 << 2)
#define G3_TASK_BIT 			(1 << 3)
#define USER_TASK_BIT 			(1 << 4)

/* Task synchronization bitmask */
#define ALL_TASKS_BITS			(HOST_IF_TASK_BIT | PRINT_TASK_BIT | G3_TASK_BIT | USER_TASK_BIT | SFLASH_TASK_BIT)

/* Custom types */

/* Global variables */

/* External variables */
extern plc_mode_t	working_plc_mode; /* Working mode of the platform (MAC / IPV6_ADP / IPV6_BOOT) */

extern osThreadId_t default_taskHandle;
extern osThreadId_t host_if_taskHandle;
extern osThreadId_t print_taskHandle;
extern osThreadId_t sflash_taskHandle;
extern osThreadId_t g3_taskHandle;
extern osThreadId_t user_taskHandle;

extern osEventFlagsId_t eventSyncHandle;


/* Public functions */
/**
  * @brief  Function implementing the default_task thread.
  * @param  argument: Not used
  * @retval None
  */
void start_default_task(void *argument)
{
	UNUSED(argument);

	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		/* Infinite loop */
		for(;;)
		{
			/* Blinks device type LED */
			LED_TOGGLE(LED_DEVTYPE);

			/* The blinking period determines the device type */
			osDelay(LED_DEVTYPE_SEMIPERIOD);
		}
    }
	else
	{
		/* Not used outside IPV6_BOOT/IPV6_ADP mode, terminated immediately */
		osThreadExit();
	}
}

/**
* @brief Function implementing the hif_task thread.
* @param argument: Not used
* @retval None
*/
void start_host_if_task(void *argument)
{
	UNUSED(argument);

	/* HIF task context initialization */
	host_if_task_init();

	/* Task synchronization */
	osEventFlagsSet(eventSyncHandle, HOST_IF_TASK_BIT);
	osEventFlagsWait(eventSyncHandle, ALL_TASKS_BITS, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

	/* HIF task */
	host_if_task_exec();
}

/**
* @brief Function implementing the print_task thread.
* @param argument: Not used
* @retval None
*/
void start_print_task(void *argument)
{
	UNUSED(argument);

	/* Print application context initialization */
	print_app_init();

	/* Task synchronization */
	osEventFlagsSet(eventSyncHandle, PRINT_TASK_BIT);
	osEventFlagsWait(eventSyncHandle, ALL_TASKS_BITS, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

	/* Print application task */
	print_app_exec();
}

/**
* @brief Function implementing the sflash_task thread.
* @param argument: Not used
* @retval None
*/
void start_sflash_task(void *argument)
{
	UNUSED(argument);

	/* HIF task context initialization */
	sflash_app_init();

	/* Task synchronization */
	osEventFlagsSet(eventSyncHandle, SFLASH_TASK_BIT);
	osEventFlagsWait(eventSyncHandle, ALL_TASKS_BITS, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

	/* HIF task */
	sflash_app_exec();
}

/**
* @brief Function implementing the g3_task thread.
* @param argument: Not used
* @retval None
*/
void start_g3_task(void *argument)
{
	UNUSED(argument);

    /* G3 application context initialization */
    g3_task_init();

    /* Task synchronization */
	osEventFlagsSet(eventSyncHandle, G3_TASK_BIT);
    osEventFlagsWait(eventSyncHandle, ALL_TASKS_BITS, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

    /* G3 application task */
    g3_task_exec();
}

/**
* @brief Function implementing the user_task thread.
* @param argument: Not used
* @retval None
*/
void start_user_task(void *argument)
{
	UNUSED(argument);

	/* User application context initialization */
	user_app_init();

	/* Task synchronization */
	osEventFlagsSet(eventSyncHandle, USER_TASK_BIT);
	osEventFlagsWait(eventSyncHandle, ALL_TASKS_BITS, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

	/* User application task */
	user_app_exec();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
