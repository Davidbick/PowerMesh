/**
  ******************************************************************************
  * @file    print_task.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that implements the Print task
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
#include <stdbool.h>
#include <cmsis_os.h>
#include <stream_buffer.h>
#include <user_if.h>
#include <print_task.h>

/** @addtogroup Print_App
  * @{
  */

/** @addtogroup Print_Task
  * @{
  */

/** @addtogroup Print_Task_Private_Code
  * @{
  */

/* Private variables */
static bool 	print_task_is_busy;
static uint8_t  printBuffer[USERIF_PRINT_MAX_SIZE];

/**
  * @}
  */

/** @addtogroup Print_Task_Exported_Code
  * @{
  */

/* External variables */
extern StreamBufferHandle_t	printStreamBufferHandle;

extern osSemaphoreId_t 		semStartPrintHandle;
extern osSemaphoreId_t 		semUserIfTxCompleteHandle;

/**
  * @brief Returns true when the Print task is busy.
  * @param None
  * @retval True if there is still data to print, false otherwise
  */
bool print_app_is_busy()
{
	/* In case the variable is not up to date, the stream buffer is checked */
	return (print_task_is_busy || (xStreamBufferBytesAvailable(printStreamBufferHandle) > 0));
}

/**
  * @brief This is the initialization of the Print task.
  * @param None
  * @retval None
  */
void print_app_init()
{
	print_task_is_busy = false;
	memset(printBuffer, 0, sizeof(printBuffer));
}

/**
  * @brief This is the main function of the Print task.
  * @param None
  * @retval None
  */
void print_app_exec()
{
	uint32_t size;

    for(;;)
    {
    	/* Waits for transfer start */
        osSemaphoreAcquire(semStartPrintHandle,  osWaitForever);

        /* Busy flag set */
        print_task_is_busy = true;

        /* Repeats until no more data is available */
    	while (xStreamBufferBytesAvailable(printStreamBufferHandle) > 0)
    	{
    		/* Extracts data */
    		size = xStreamBufferReceive(printStreamBufferHandle, printBuffer, sizeof(printBuffer), osWaitForever);

    		/* Prints data */
			HAL_UART_Transmit_DMA(&huartUserIf, printBuffer, size);

			/* Waits for transfer completion */
			osSemaphoreAcquire(semUserIfTxCompleteHandle, osWaitForever);
    	}

    	/* Busy flag reset */
    	print_task_is_busy = false;
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
