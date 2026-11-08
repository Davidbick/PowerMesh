/*****************************************************************************
*   @file    modbus_callbacks.c
*   @author  Alejandro Mera, modified by AMG/IPC Application Team
*   @brief   This code manages modbus callbacks.
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
#include "cmsis_os.h"
#include "task.h"
#include "main.h"
#include "modbus.h"
#include "settings.h"

#if ENABLE_MODBUS

/**
 * @brief
 * This is the callback for HAL interrupts of UART TX used by Modbus library.
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @param huart UART HAL handler
 */

void Modbus_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Modbus RTU TX callback BEGIN */
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	for (int i = 0; i < numberHandlers; i++)
	{
		if (mHandlers[i]->port == huart)
		{
			// notify the end of TX
			xTaskNotifyFromISR(mHandlers[i]->modbus_taskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
			break;
		}
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief
 * This is the callback for HAL interrupt of UART RX
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @param huart UART HAL handler
 */
void Modbus_RxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	for (int i = 0; i < numberHandlers; i++)
	{
		if (mHandlers[i]->port == huart)
		{
			if (mHandlers[i]->xTypeHW == USART_HW)
			{
				RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
				HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
				xTimerResetFromISR(mHandlers[i]->TimerT35, &xHigherPriorityTaskWoken);
			}
			break;
		}
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#if ENABLE_MODBUS_USART_DMA
/*
 * @brief
 * DMA requires to handle callbacks for special communication modes of the HAL
 * It also has to handle eventual errors including extra steps that are not automatically
 * handled by the HAL
 * @param huart UART HAL handle
 */

void Modbus_ErrorCallback(UART_HandleTypeDef *huart)
{
	for (int i = 0; i < numberHandlers; i++)
	{
		if (mHandlers[i]->port == huart)
		{
			if (mHandlers[i]->xTypeHW == USART_HW_DMA)
			{
				while (HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
				{
					HAL_UART_DMAStop(mHandlers[i]->port);
				}

				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt
			}

			break;
		}
	}
}

/*
 * @brief
 * DMA requires to handle callbacks for special communication modes of the HAL
 * It also has to handle eventual errors including extra steps that are not automatically
 * handled by the HAL
 * @param huart UART HAL handle
 * @param Size Bytes received
 */
void Modbus_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	for (int i = 0; i < numberHandlers; i++ )
	{
		if (mHandlers[i]->port == huart  )
		{
			if (mHandlers[i]->xTypeHW == USART_HW_DMA)
			{
				if (Size) //check if we have received any byte
				{
					mHandlers[i]->xBufferRX.u8available = Size;
					mHandlers[i]->xBufferRX.overflow = false;

					while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
					{
						HAL_UART_DMAStop(mHandlers[i]->port);
					}

					__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

					xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0 , eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
				}
			}

			break;
		}
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#endif
#endif
