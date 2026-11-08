/**
  ******************************************************************************
  * @file    user_task.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that implements the User task
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include <cmsis_os.h>
#include <assert.h>
#include <debug_print.h>
#include <pin_management.h>
#include <utils.h>
#include <user_g3_common.h>
#include <user_image_transfer.h>
#include <user_mac.h>
#include <user_task.h>
#include <user_terminal.h>

/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_Task
  * @{
  */

/** @addtogroup User_Task_Exported_Code
  * @{
  */

/* Global variables */
uint32_t user_task_operation_counter = 0;

/**
  * @}
  */

/** @addtogroup User_Task_Private_Code
  * @{
  */

/* External Variables */
extern plc_mode_t	working_plc_mode;

extern osMessageQueueId_t user_queueHandle;

/* Private Functions */

/**
 * @brief This functions parses the messages for the user task, when needed by it.
 * @param g3_msg Pointer to the G3 message structure.
 * @retval None
 */
static void user_msg_handler(g3_msg_t *g3_msg)
{
	/* Must point to a memory pool, not to NULL */
	assert(g3_msg != NULL);

	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		/* Forwards the message to the modules that need it */
		if (UserG3_MsgNeeded(g3_msg))
		{
			/* Forwards the message to the User G3 */
			UserG3_MsgHandler(g3_msg);
		}
#if ENABLE_IMAGE_TRANSFER
		if (UserImgTransfer_MsgNeeded(g3_msg))
		{
			/* Forwards the message to the User Image Transfer */
			UserImgTransfer_MsgHandler(g3_msg);
		}
#endif
	}
	else if (working_plc_mode == PLC_MODE_MAC)
	{
		if (UserMac_MsgNeeded(g3_msg))
		{
			UserMac_MsgHandler(g3_msg);
		}
	}
}

/**
  * @}
  */


/** @addtogroup User_Task_Exported_Code
  * @{
  */

/* Public Functions */

/**
  * @brief This is the initialization of the User Task.
  * @param None
  * @retval None
  */
void user_app_init(void)
{
	/* Initialize User Interface Input/Output */
	user_if_init();

	/* Set baudrate for User UART */
	USART_changeBaudrate(&huartUserIf, USERIF_BAUDRATE);

	/* Starts listening for the first byte */
    user_if_rx_start();

	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		/* Initialize User G3 */
		UserG3_Init();

		/* Initialize User Interface FSM */
		UserTerminal_Init();

#if ENABLE_IMAGE_TRANSFER
		/* Initialize User Image Transfer */
		UserImgTransfer_Init();
#endif
	}
	else if (working_plc_mode == PLC_MODE_MAC)
	{
		/* Initialize User MAC */
		UserMac_Init();
	}

	/* Add here more initializations for the User task */
	/* ... */
}

/**
  * @brief This is the main function of the User Task.
  * @param None
  * @retval None
  */
void user_app_exec(void)
{
	g3_msg_t *g3_msg;
	task_msg_t task_msg;

    for(;;)
    {
        /* Check if a message coming from the G3 task has been received in the queue */
    	if (RTOS_GET_MSG(user_queueHandle, &task_msg))
    	{
    		/* Counts how many time this task is executed */
    		user_task_operation_counter++;

    		g3_msg = task_msg.data;

    		switch (task_msg.message_type)
    		{
    		case G3_RX_MSG:
				user_msg_handler(g3_msg);
				break;
    		case USER_MSG:
    			/* Expected, but with no action */
    			break;
    		default:
    			Error_Handler(); /* Unexpected message received in the queue, check task_msg.message_type for more details */
    			break;
    		}
    		
    		if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
    		{
    			/* FSMs executed in BOOT or ADP mode */

    			UserG3_FsmManager();  			/* User G3 (UDP management) */

    			UserTerminal_FsmManager_IPv6(); /* User terminal */
#if ENABLE_IMAGE_TRANSFER
    			UserImgTransfer_FsmManager();	/* User Image transfer */
#endif
    		}
    		else if (working_plc_mode == PLC_MODE_MAC)
    		{
    			/* FSMs executed in MAC mode */

    			UserMac_FsmManager();

    			UserTerminal_FsmManager_MAC();
    		}
    		else
    		{
    			Error_Handler(); /* Unsupported modes */
    		}

    		/* The message and its content in the memory pool can now be freed */
			g3_discard_message(g3_msg);
    	}
    }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
