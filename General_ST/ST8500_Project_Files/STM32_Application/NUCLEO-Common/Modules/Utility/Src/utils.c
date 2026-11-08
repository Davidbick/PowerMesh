/**
  ******************************************************************************
  * @file    utils.c
  * @author  AMG/IPC Application Team
  * @brief   This file provides some general utility functions.
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
#include <stdio.h>
#include <math.h>
#include <cmsis_os.h>
#include <assert.h>
#include <utils.h>
#include <main.h>

/** @addgroup Utility
  * @{
  */

/* Public functions */

/**
  * @brief  Function that handles a OS/task delay, depending on the OS status.
  * @param  time_in_ms Number of milliseconds to wait.
  * @retval None
  * @note Do not call this function if the system tick is disabled
  */
void utils_delay_ms(uint32_t time_in_ms)
{
	/* Conversion from ms to ticks */
	uint32_t ticks = pdMS_TO_TICKS(time_in_ms);

	/* Guarantees a minimum wait of 1 tick */
	if (ticks == 0)
	{
		ticks = 1;
	}

	if (osKernelGetState() == osKernelRunning)
	{
		/* If inside FreeRTOS, delays the current task */
		osDelay(ticks);
	}
	else
	{
		/* otherwise, performs "busy wait" */
		HAL_Delay(ticks);
	}
}

/**
  * @brief  Function that blinks a LED connected to a GPIO, if called periodically.
  * @param  [out]GPIO_Port Port of the GPIO to toggle.
  * @param  [in] GPIO_Pin Pin of the GPIO to toggle.
  * @param  [in] toggle_period Period of the toggle activity (in ms).
  * @param  [out] timestamp Pointer to the time stamp of the last toggle (in ms).
  * @retval None
  */
void utils_blink_led(GPIO_TypeDef *GPIO_Port, const uint16_t GPIO_Pin, const uint32_t toggle_period, uint32_t *timestamp)
{
	if (toggle_period > 0)
	{
		if ((HAL_GetTick() - *timestamp) > pdMS_TO_TICKS(toggle_period))
		{
			HAL_GPIO_TogglePin(GPIO_Port, GPIO_Pin);
			*timestamp = HAL_GetTick();
		}
	}
}

/**
  * @brief  Function that converts an array of bytes to a string of hexadecimal values.
  * @param  [out] string Destination buffer where the converted hex string is stored.
  * @param  [in] array Source buffer of the bytes to convert to HEX format.
  * @param  [in] array_size Number of bytes in the source buffer.
  * @retval Pointer to the destination buffer
  */
void utils_reverse_array(uint8_t *array, const uint8_t array_size)
{
    uint8_t temp;
    const uint8_t n_half = array_size / 2;

    for (uint8_t i = 0; i < n_half; i++)
    {
        //swap first, last elements
        temp = array[i];
        array[i] = array[array_size - i - 1];
        array[array_size - i - 1] = temp;
    }
}

/**
  * @brief  Function that converts an array of bytes to a string of hexadecimal values.
  * @param  [out] string Destination buffer where the converted hex string is stored.
  * @param  [in] array Source buffer of the bytes to convert to HEX format.
  * @param  [in] array_size Number of bytes in the source buffer.
  * @retval Pointer to the destination buffer
  */
char* utils_convet_array_to_hex_string(char* string, const uint8_t *array, const uint8_t array_size)
{
	for (int32_t i = 0; i < array_size; i++)
	{
		snprintf(&string[2*i], 3, "%02X", array[i]);
	}

	return string;
}

/**
  * @}
  */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

