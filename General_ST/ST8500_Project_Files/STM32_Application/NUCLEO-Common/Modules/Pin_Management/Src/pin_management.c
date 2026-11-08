/**
  ******************************************************************************
  * @file    pin_management.c
  * @author  AMG/IPC Application Team
  * @brief   Handler functions for the ST8500 GPIOs.
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
#include <spi.h>
#include <usart.h>
#include <main.h>
#include <utils.h>

#include <pin_management.h>

/** @addtogroup ST8500_Gpio_Management
  * @{
  */

/* Public functions */

/**
 * @brief  This function reads the ST8500 boot mode.
 * @param  None
 * @return ST8500 boot mode, boot from UART (0), or boot from SFLASH attached to ST8500 (1).
 */
st8500_boot_mode_t readBootModeST8500(void)
{
	/* LOW GPIO LEVEL  -> 0 -> Boot from UART */
	/* HIGH GPIO LEVEL -> 1 -> Boot from FLASH */

	return (st8500_boot_mode_t) HAL_GPIO_ReadPin(BOOT1_IN_GPIO_Port, BOOT1_IN_Pin);
}

/**
 * @brief  This function asserts the ST8500 reset pin.
 * @param  None
 * @retval None
 */
void assertResetOnST8500(void)
{
	/* Active low */
	HAL_GPIO_WritePin(ST8500_RESET_N_GPIO_Port, ST8500_RESET_N_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  This function deasserts the ST8500 reset pin.
 * @param  None
 * @retval None
 */
void deassertResetOnST8500(void)
{
	/* Active low */
	HAL_GPIO_WritePin(ST8500_RESET_N_GPIO_Port, ST8500_RESET_N_Pin, GPIO_PIN_SET);
}

/**
 * @brief  This function sets all BOOT pins of the ST8500 as inputs.
 *         This needs to be done after ST8500 power-up/reset.
 * @param  None
 * @retval None
 */
void setBootPinsAsInput(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = IN_PIN_STR(BOOT0_Pin);

	HAL_GPIO_Init(BOOT0_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin   = BOOT2_Pin;
	HAL_GPIO_Init(BOOT2_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  This function sets all BOOT pins of the ST8500 as outputs, and sets their value.
 *         This needs to be done before ST8500 power-up/reset.
 * @retval None
 */
void setBootPinsAsOutput(void)
{
	/* Always 0 */
	HAL_GPIO_WritePin(BOOT2_GPIO_Port, BOOT2_Pin, GPIO_PIN_RESET);

	/* BOOT1 is controlled via HW, and can only be read */

	/* Always 0 */
	HAL_GPIO_WritePin(BOOT0_GPIO_Port, BOOT0_Pin, GPIO_PIN_RESET);

	GPIO_InitTypeDef GPIO_InitStruct = OUT_PIN_STR(BOOT0_Pin);

	HAL_GPIO_Init(BOOT0_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin   = BOOT2_Pin;
	HAL_GPIO_Init(BOOT2_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  This function reads the MAC_SELECT pin.
 * @param  None
 * @return true, if MAC_SELECT is enabled, false otherwise.
 */
bool readRfSelect(void)
{
	/* LOW GPIO LEVEL  -> 0 -> 915 module selected */
	/* HIGH GPIO LEVEL -> 1 -> 868 module selected */

	return (HAL_GPIO_ReadPin(RF_MODULE_SEL_GPIO_Port, RF_MODULE_SEL_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief  This function reads the selected bandplan (CEN-A/FCC).
 * @param  None
 * @return STM32 boot mode, default (0), or in sleep mode (1).
 */
bool readFCCSelect(void)
{
	/* SWITCH UP   -> LOW GPIO LEVEL  -> 0 -> FCC */
	/* SWITCH DOWN -> HIGH GPIO LEVEL -> 1 -> CENA  */

	return (HAL_GPIO_ReadPin(BANDPLAN_SEL_GPIO_Port, BANDPLAN_SEL_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief  This function reads the MAC_SELECT pin.
 * @param  None
 * @return true, if MAC_SELECT is enabled, false otherwise.
 */
bool readMacSelect(void)
{
	/* LOW GPIO LEVEL  -> 0 -> MAC mode */
	/* HIGH GPIO LEVEL -> 1 -> Normal mode */

	return (HAL_GPIO_ReadPin(MAC_MODE_SEL_GPIO_Port, MAC_MODE_SEL_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief  This function reads the MAC_SELECT pin.
 * @param  None
 * @return true, if MAC_SELECT is enabled, false otherwise.
 */
bool readModbusMasterSelect(void)
{
	/* LOW GPIO LEVEL  -> 0 -> Slave mode */
	/* HIGH GPIO LEVEL -> 1 -> Master mode */

	return (HAL_GPIO_ReadPin(MODBUS_MASTER_SEL_GPIO_Port, MODBUS_MASTER_SEL_Pin) == GPIO_PIN_SET);
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
