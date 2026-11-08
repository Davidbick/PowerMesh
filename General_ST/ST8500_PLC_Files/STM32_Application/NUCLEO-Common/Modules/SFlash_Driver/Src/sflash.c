/*****************************************************************************
 *   @file    sflash.c
 *   @author  AMG/IPC Application Team
 *   @brief   This file contains code that implements the communication with the SPI FLASH.
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
#include <cmsis_os.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <utils.h>
#include <task_comm.h>
#include <sflash.h>
#include <sflash_driver.h>
#include <main.h>


/* External Variables */
extern sflash_task_semaphore_t sflash_task_semaphore[];

extern osMessageQueueId_t sflash_queueHandle;

/* Private Functions */

/**
 * @brief Assigns a semaphore to a task if available, otherwise returns NULL.
 * @param task_id The ID of the task requesting the semaphore.
 * @retval The semaphore handle if it was assigned or already assigned, NULL otherwise (all semaphores are occupied).
 */
static osSemaphoreId_t sflash_task_assign_semaphore(osThreadId_t task_id)
{
	osSemaphoreId_t semaphore = NULL;

	/* Checks if the task has already a semaphore assigned */
	for (int i = 0; i < SFLASH_SEM_NUM; i++)
	{
		if (sflash_task_semaphore[i].task_id == task_id)
		{
			semaphore = sflash_task_semaphore[i].semaphore;
			break;
		}
	}

	/* If no semaphore has been assigned to the calling task yet */
	if (semaphore == NULL)
	{
		/* Looks for the first unassigned semaphore */
		for (int i = 0; i < SFLASH_SEM_NUM; i++)
		{
			if (sflash_task_semaphore[i].task_id == NULL)
			{
				/* Assigns the semaphore to the task */
				sflash_task_semaphore[i].task_id = task_id;
				semaphore = sflash_task_semaphore[i].semaphore;
				PRINT_SFLASH_INFO("Semaphore %X assigned to task %X\n", semaphore, task_id);
				break;
			}
		}
	}

	return semaphore;
}

/* Public Functions */

/**
 * @brief Executes a command on the SPI flash memory.( handle message send to sflash_task )
 * @param sflash_cmd The command to execute.
 * @param address The memory address to operate on.
 * @param data The pointer to the data buffer to read from or write to. In case of ID check, 1 is written at this address if the ID matches, 0 is written otherwise.
 * @param size The number of bytes to read or write.
 * @param timeout The timeout value for the read operation (not used for erase/write). Use 0 for no timeout.
 * @retval The result of the operation (true in case of success)
 */
bool sflash_command(sflash_cmd_t sflash_cmd, uint32_t address, void *data, uint32_t size, uint32_t timeout)
{
	bool result = false;

	if (OS_IS_ACTIVE())
	{
		osSemaphoreId_t semaphore = NULL; /* Meaningful only in case of read operation */

		sflash_msg_t *sflash_msg = MEMPOOL_MALLOC(sizeof(sflash_msg_t));

		sflash_msg->command = sflash_cmd;
		sflash_msg->address = address;

		if (sflash_cmd == SFLASH_CMD_WRITE)
		{
			sflash_msg->data = MEMPOOL_MALLOC(size);
			memcpy(sflash_msg->data, data, size);
		}
		else
		{
			sflash_msg->data = data;
		}

		sflash_msg->size    = size;
		sflash_msg->task_id = osThreadGetId();

#if (DEBUG_SFLASH >= DEBUG_LEVEL_WARNING)
		const char *task_name = osThreadGetName(sflash_msg->task_id);
#endif
		PRINT_SFLASH_INFO("FLASH operation requested by task %s\n", task_name);

		if ((sflash_cmd == SFLASH_CMD_READ) || (sflash_cmd == SFLASH_CMD_GET_ID))
		{
			semaphore = sflash_task_assign_semaphore(sflash_msg->task_id);

			assert(semaphore != NULL);
		}

		result = RTOS_PUT_MSG(sflash_queueHandle, SFLASH_MSG, sflash_msg);

		/* The calling task can acquire this semaphore, in case of a read operation, to synchronize with the completion of the read operation */
		if ((sflash_cmd == SFLASH_CMD_READ) || (sflash_cmd == SFLASH_CMD_GET_ID))
		{
			if (result)
			{
				if (osSemaphoreAcquire(semaphore, timeout) == osOK)
				{
					result = true;
				}
				else
				{
					PRINT_SFLASH_WARNING("Timeout on FLASH semaphore for task %s (%u ms)\n", task_name, timeout);
					result = false;
				}
			}
		}
	}
	else
	{
		switch(sflash_cmd)
		{
		case SFLASH_CMD_BULK_ERASE:
			result = SFLASH_BulkErase();
			break;
		case SFLASH_CMD_ERASE:
			result = SFLASH_Erase(address, size);
			break;
		case SFLASH_CMD_WRITE:
			result = SFLASH_Write(address, (uint8_t*) data, size);
			break;
		case SFLASH_CMD_READ:
			result = SFLASH_Read((uint8_t*) data, address, size);
			break;
		case SFLASH_CMD_GET_ID:
			result = SFLASH_GetDeviceId((uint32_t*) data);
			break;
		default:
			Error_Handler(); /* Handle wrong operation ID */
			break;
		}
	}

	return result;
}
