/******************************************************************************
  * @file    callbacks.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements callback functions.
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
#include "usart.h"
#include "spi.h"
#include <main.h>
#include <modbus.h>
#include <host_if.h>
#include <user_if.h>
#include <g3_app_boot.h>
#include <g3_boot_srv_eap.h>
#include <g3_app_keep_alive.h>
#include <g3_app_last_gasp.h>
#include <user_g3_common.h>
#include <user_image_transfer.h>
#include <user_mac.h>
#include <user_terminal.h>

/* Definitions */
#define DEFAULT_TEST_MSG_NUMBER		10

/* Global variables */

/* External variables */
extern plc_mode_t			working_plc_mode;

extern osSemaphoreId_t		semSPIHandle;

/* Public functions */

/**
  * @brief  Rx Transfer completed callback for UART.
  * @param  huart: UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == huartHostIf.Instance)
	{
		host_if_rx_handler();
	}
	else if (huart->Instance == huartUserIf.Instance)
	{
		user_if_rx_handler();
	}
#if ENABLE_MODBUS
	else if (huart->Instance == huartModbus.Instance)
	{
		Modbus_RxCpltCallback(huart);
	}
#endif
}

/**
  * @brief  Tx Transfer completed callback for UART.
  * @param  huart: UART handle.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == huartHostIf.Instance)
	{
		host_if_tx_handler();
	}
	else if (huart->Instance == huartUserIf.Instance)
	{
		user_if_tx_handler();
	}
#if ENABLE_MODBUS
	else if (huart->Instance == huartModbus.Instance)
	{
		Modbus_TxCpltCallback(huart);
	}
#endif
}

/**
  * @brief  Error callback for UART.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
#if ENABLE_MODBUS_USART_DMA
	if (huart->Instance == huartModbus.Instance)
	{
		Modbus_ErrorCallback(huart);
	}
	else
#else
// gibi debuggone
	//UNUSED(huart);
	if (huart->Instance == huartHostIf.Instance)
	{
		// if blocking error
		if ((huart->ErrorCode & (HAL_UART_ERROR_RTO | HAL_UART_ERROR_ORE)) != 0U)
		{
			/* clear error flag */
			huart->ErrorCode = HAL_UART_ERROR_NONE;
			/* Stop reception (if not already stopped) */
			host_if_rx_stop();
			/* Re-Initializes Host Interface and Restart reception */
			host_if_init();
		}
	}
//gibi debuggone
#endif
}

#if ENABLE_MODBUS_USART_DMA
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == huartModbus.Instance)
	{
		Modbus_RxEventCallback(huart, Size);
	}
}
#endif

/**
  * @brief  Tx and Rx Transfer completed callback.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspiSFlash.Instance)
	{
		/* Unblocks the task that requested the SPI transfer */
		osSemaphoreRelease(semSPIHandle);
	}
}

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
#if defined(STM32G070xx)
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
#else
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
#endif
{
  if (GPIO_Pin == USER_PUSHBUTTON_Pin)
  {
	  HAL_NVIC_DisableIRQ(USER_PUSHBUTTON_EXTI_IRQn);

	  if (working_plc_mode == PLC_MODE_MAC)
	  {
		  UserMac_StartTxCallback(NULL, mac_test_plc_rf, DEFAULT_TEST_MSG_NUMBER); // Start MAC tests
	  }
#if !IS_COORD && ENABLE_LAST_GASP
	  else
	  {
		  g3_app_last_gasp_start();
	  }
#endif
  }
}

/**
  * @brief  SPI error callback.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hspi);
	Error_Handler();
}

#if ENABLE_ICMP_KEEP_ALIVE
/**
  * @brief Callback wrapper function for the kaTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void kaTimerCallback(void *argument)
{
	g3_app_ka_timer_callback(argument);
}
#endif

/**
  * @brief Callback wrapper function for the bootTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void bootTimerCallback(void *argument)
{
#if IS_COORD
#if ENABLE_BOOT_SERVER_ON_HOST
	g3_app_boot_srv_timeoutCallback(argument);
#else
	UNUSED(argument);
#endif
#else
#if ENABLE_BOOT_CLIENT_ON_HOST
	g3_app_boot_clt_timeoutCallback(argument);
#else
	g3_app_boot_dev_timeoutCallback(argument);
#endif
#endif
}

/**
  * @brief Callback wrapper function for the userTimeoutTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void userTimeoutTimerCallback(void *argument)
{
	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		UserTerminal_TimeoutCallback(argument);
	}
	else if (working_plc_mode == PLC_MODE_MAC)
	{
		UserMac_TimeoutCallback(argument);
	}
}

/**
  * @brief Callback wrapper function for the commTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void commTimerCallback(void *argument)
{
	UserG3_TimeoutCallback(argument);
}

#if ENABLE_IMAGE_TRANSFER
/**
  * @brief Callback wrapper function for the transferTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void transferTimerCallback(void *argument)
{
	UserImgTransfer_TimeoutCallback(argument);
}
#endif

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Callback wrapper function for the serverTimer FreeRTOStimer.
  * @param argument Passed argument.
  * @retval None
  */
void serverTimerCallback(void *argument)
{
	g3_boot_srv_eap_timeoutCallback(argument);
}
#endif

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
