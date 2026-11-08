/**
  ******************************************************************************
  * @file    pin_management.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the ST8500 GPIOs handler functions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PIN_MANAGEMENT_H_
#define PIN_MANAGEMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ST8500_Gpio_Management ST8500 GPIO Management
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <settings.h>

/** @addtogroup ST8500_Gpio_Management
  * @{
  */

/* Definitions */
#define RESET_PULSE_DURATION	5	/* In ms */

/* Custom types */
typedef enum {
  STM32_BOOT_DEFAULT = 0,
  STM32_BOOT_SLEEP
} stm32_boot_mode_t;

typedef enum {
  ST8500_BOOT_FROM_UART = 0,
  ST8500_BOOT_FROM_SFLASH
} st8500_boot_mode_t;

/* Macros */

#define IN_PIN_STR(pin)				{	.Pin   = pin, 						\
										.Mode  = GPIO_MODE_INPUT, 			\
										.Pull  = GPIO_NOPULL,  				\
										.Speed = GPIO_SPEED_FREQ_VERY_HIGH  \
									}

#define EXTI_RISE_FALL_PIN_STR(pin)	{											\
										.Pin   = pin, 							\
										.Mode  = GPIO_MODE_IT_RISING_FALLING, 	\
										.Pull  = GPIO_PULLUP, 					\
										.Speed = GPIO_SPEED_FREQ_VERY_HIGH 		\
									}

#define OUT_PIN_STR(pin)			{										\
										.Pin   = pin, 						\
										.Mode  = GPIO_MODE_OUTPUT_PP, 		\
										.Pull  = GPIO_NOPULL,				\
										.Speed = GPIO_SPEED_FREQ_VERY_HIGH 	\
									}

/* Public functions */

/* Switches for ST8500 boot modes */
st8500_boot_mode_t readBootModeST8500(void);

/* ST8500 reset pin */
void assertResetOnST8500(void);
void deassertResetOnST8500(void);

/* ST8500 BOOT pins */
void setBootPinsAsInput(void);
void setBootPinsAsOutput(void);

/* Switches and configuration */
bool readRfSelect(void);
bool readFCCSelect(void);
bool readMacSelect(void);
bool readModbusMasterSelect(void);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* PIN_MANAGEMENT_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
