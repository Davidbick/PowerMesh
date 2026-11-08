/**
  *****************************************************************************
  *   @file    sflash_task.c
  *   @author  AMG/IPC Application Team
  *   @brief   This file contains code that implements the sflash task.
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
#include <debug_print.h>
#include <mem_pool.h>
#include <task_comm.h>
#include <main.h>
#include <sflash_driver.h>
#include <sflash.h>
#include <sflash_task.h>

/* Definitions */
#define SEM_MAX_COUNT				1	/* Maximum value for each semaphore (binary) */
#define SEM_INITIAL_COUNT			0	/* Starting value for each semaphore */

/* External Variables */
extern osMessageQueueId_t sflash_queueHandle;

/* Global Variables */
sflash_task_semaphore_t sflash_task_semaphore[SFLASH_SEM_NUM]; /* table with task-semaphore assignments */

/* Private Variables */
static bool sflash_task_is_busy = false;

static StaticSemaphore_t sflash_task_semaphore_control_block[SFLASH_SEM_NUM];
static const osSemaphoreAttr_t sflash_task_semaphore_attributes[SFLASH_SEM_NUM] = {{
	  .name = "SFLASHsem1",
	  .cb_mem = &sflash_task_semaphore_control_block[0],
	  .cb_size = sizeof(sflash_task_semaphore_control_block[0]),
	},
	{
	  .name = "SFLASHsem2",
	  .cb_mem = &sflash_task_semaphore_control_block[1],
	  .cb_size = sizeof(sflash_task_semaphore_control_block[1]),
	}
};


/**
  * @brief Returns true when the SFLASH task is busy.
  * @param None
  * @retval True if there are operations to complete for the SFLASH, false otherwise
  */
bool sflash_app_is_busy()
{
	/* In case the variable is not up to date, the command queue is checked */
	return (sflash_task_is_busy || (RTOS_MSG_AVAILABLE(sflash_queueHandle) > 0));
}

/**
 * @brief This is the initialization of the SFLASH task.
 * @param None
 * @retval None
 */
void sflash_app_init(void)
{
	/* Initialize binary semaphores for tasks that require reading from flash memory */
	for (int i = 0; i < SFLASH_SEM_NUM; i++)
	{
		sflash_task_semaphore[i].task_id = NULL;
		sflash_task_semaphore[i].semaphore = osSemaphoreNew(SEM_MAX_COUNT, SEM_INITIAL_COUNT, &sflash_task_semaphore_attributes[i]);

		assert(sflash_task_semaphore[i].semaphore != NULL);
	}
}

/**
 * @brief This is the main function of the SFLASH task.
 * @param None
 * @retval None
 */
void sflash_app_exec(void)
{
	task_msg_t task_msg;
	sflash_msg_t* sflash_msg;

	for(;;)
	{
		/* Reception of messages */
		if (RTOS_GET_MSG(sflash_queueHandle, &task_msg))
		{
	        /* Busy flag set */
			sflash_task_is_busy = true;
#if (DEBUG_SFLASH >= DEBUG_LEVEL_INFO)
			uint32_t timestamp = HAL_GetTick();
#endif
			sflash_msg = task_msg.data; /* The payload of the message is a SFLASH message */

			if (task_msg.message_type == SFLASH_MSG)
			{
				osSemaphoreId_t semaphore = NULL;

				/* Read and Get ID use semaphores */
				if ((sflash_msg->command == SFLASH_CMD_READ) || (sflash_msg->command == SFLASH_CMD_GET_ID))
				{
					/* Gets the semaphore assigned to the task specified in the SFLASH message */
					for (int i = 0; i < SFLASH_SEM_NUM; i++)
					{
						if (sflash_task_semaphore[i].task_id == sflash_msg->task_id)
						{
							semaphore = sflash_task_semaphore[i].semaphore;
							break;
						}
					}

					/* Handles the situation where the calling task did not acquire the semaphore (makes sure that the semaphore is busy) */
					osSemaphoreAcquire(semaphore, 0);
				}

				switch(sflash_msg->command)
				{
				case SFLASH_CMD_BULK_ERASE:
					SFLASH_BulkErase();
					break;
				case SFLASH_CMD_ERASE:
					SFLASH_Erase(sflash_msg->address, sflash_msg->size);
					break;
				case SFLASH_CMD_WRITE:
					SFLASH_Write(sflash_msg->address, sflash_msg->data, sflash_msg->size);
					break;
				case SFLASH_CMD_READ:
					SFLASH_Read(sflash_msg->data, sflash_msg->address, sflash_msg->size);
					break;
				case SFLASH_CMD_GET_ID:
					SFLASH_GetDeviceId(sflash_msg->data);
					break;
				default:
					Error_Handler(); /* Handle wrong operation ID */
					break;
				}

				if (sflash_msg->command == SFLASH_CMD_WRITE)
				{
					MEMPOOL_FREE(sflash_msg->data);
				}
				else if ((sflash_msg->command == SFLASH_CMD_READ) || (sflash_msg->command == SFLASH_CMD_GET_ID))
				{
					/* Used to unblock the calling task that waits for the read data */
					osSemaphoreRelease(semaphore);
				}
			}
			else
			{
				Error_Handler(); /* Unexpected message type */
			}

			/* Free the memory pool used to allocate the SFLASH command message */
			MEMPOOL_FREE(sflash_msg);

#if (DEBUG_SFLASH >= DEBUG_LEVEL_INFO)
			PRINT_SFLASH_INFO("SFLASH operation completed in %u ms\n", HAL_GetTick() - timestamp);
#endif
	    	/* Busy flag reset */
			sflash_task_is_busy = false;
		}
	}
}
