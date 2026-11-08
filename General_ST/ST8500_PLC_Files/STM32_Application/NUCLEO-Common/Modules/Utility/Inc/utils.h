/**
  ******************************************************************************
  * @file    utils.h
  * @author  AMG/IPC Application Team
  * @brief   utility functions
  *          This file provides some general utility functions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <main.h>

/** @defgroup Utility Generic utility macros and functions
  * @{
  */

/* Macros */
#define LOW_BYTE(x)         			((uint8_t)( (x)       & 0xFF))
#define HIGH_BYTE(x)        			((uint8_t)(((x) >> 8) & 0xFF))
#define TAKE_BYTE(x, n) 				((uint8_t)(((x) >> (n * 8)) & 0xFF))
#define TAKE_BYTE_MASK(x, n, mask) 		((uint8_t)(((x) >> (n * 8)) & mask))
#define MASK_AND_SHIFTR(x, mask, shift)	((uint8_t)(((x) & mask) >> shift))

#define ASSEMBLE_U8(b1, b0)				(                              ((b1) << 4) | (b0))
#define ASSEMBLE_U16(b1, b0)			(                              ((b1) << 8) | (b0))
#define ASSEMBLE_U24(b2, b1, b0)		(               ((b2) << 16) | ((b1) << 8) | (b0))
#define ASSEMBLE_U32(b3, b2, b1, b0)	(((b3) << 24) | ((b2) << 16) | ((b1) << 8) | (b0))

#define BIT_SET(x, bit)			(x |=  (1U << bit))
#define BIT_CLEAR(x, bit)		(x &= ~(1U << bit))

#define BIT_IS_SET(x, bit)		(((x) & (1U << bit)) != 0)
#define BIT_IS_CLEAR(x, bit)	(((x) & (1U << bit)) == 0)

#define MASK_IS_SET(x, mask)	(((x) & (mask)) != 0)
#define MASK_IS_CLEAR(x, mask)	(((x) & (mask)) == 0)

#define NUM_OF_ELEM(x)			(sizeof(x)/sizeof(x[0]))

#define SWAP_U16(x) 			((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

/* Definitions */
#define VAR_SIZE_POINTER_OFFSET(field, offset)			 ((uint8_t*)  (((uint8_t*) &(field)) - offset))
#define VAR_SIZE_PAYLOAD_OFFSET(field, offset)			*((uint8_t*)  (((uint8_t*) &(field)) - offset))

#define OS_IS_ACTIVE()					(osKernelGetState() == osKernelRunning)

/* LEDs */
#define LED_SLEEP_GPIO_Port       		LED0_GPIO_Port
#define LED_SLEEP_Pin             		LED0_Pin

#define LED_ONLINE_GPIO_Port       		LED0_GPIO_Port
#define LED_ONLINE_Pin             		LED0_Pin

#define LED_DEVTYPE_GPIO_Port           LED1_GPIO_Port
#define LED_DEVTYPE_Pin                 LED1_Pin

#define LED_DOWNLOAD_ST_GPIO_Port       LED2_GPIO_Port
#define LED_DOWNLOAD_ST_Pin             LED2_Pin

#define LED_KEEPALIVE_GPIO_Port			LED2_GPIO_Port
#define LED_KEEPALIVE_Pin				LED2_Pin

#define LED_TRANSFER_GPIO_Port       	LED3_GPIO_Port
#define LED_TRANSFER_Pin             	LED3_Pin

/* Default task periods */
#if IS_COORD
#define LED_DEVTYPE_SEMIPERIOD			250		/* In ms */
#else
#define LED_DEVTYPE_SEMIPERIOD			500		/* In ms */
#endif

#define LED_DOWNLOAD_ST_SEMIPERIOD		50 		/* In ms */

#define LED_IS_ON(led)			(((led ## _GPIO_Port)->IDR & led ## _Pin) == 0x00u)
#define LED_IS_OFF(led)			(((led ## _GPIO_Port)->IDR & led ## _Pin) != 0x00u)

#define LED_SET_ON(led)     	led ## _GPIO_Port->BRR  =  led ## _Pin
#define LED_SET_OFF(led)    	led ## _GPIO_Port->BSRR =  led ## _Pin

#define LED_SET(led, state)     if (state) LED_SET_ON(led); else LED_SET_OFF(led)
#define LED_TOGGLE(led)     	if LED_IS_ON(led) LED_SET_OFF(led); else LED_SET_ON(led)

#define LED_SET_ALL_ON()		do {					\
									LED_SET_ON(LED0);	\
									LED_SET_ON(LED1);	\
									LED_SET_ON(LED2);	\
									LED_SET_ON(LED3);	\
								} while(0)

#define LED_SET_ALL_OFF()		do {					\
									LED_SET_OFF(LED0);	\
									LED_SET_OFF(LED1);	\
									LED_SET_OFF(LED2);	\
									LED_SET_OFF(LED3);	\
								} while(0)

/* Allocation of strings to display hex values */
#define ALLOC_STATIC_HEX_STRING(str_name, array, array_len) 	char str_name[(array_len)*2 + 1];								\
																utils_convet_array_to_hex_string(str_name, array, array_len)

#define ALLOC_DYNAMIC_HEX_STRING(str_name, array, array_len) 	char *str_name = MEMPOOL_MALLOC((array_len)*2 + 1);				\
																utils_convet_array_to_hex_string(str_name, array, array_len)

#define FREE_DYNAMIC_HEX_STRING(str_name) 						MEMPOOL_FREE(str_name);


/* Public functions */
void	utils_delay_ms(uint32_t time_in_ms);
void 	utils_blink_led(GPIO_TypeDef *GPIO_Port, const uint16_t GPIO_Pin, uint32_t time_in_ms, uint32_t *timestamp);
void 	utils_reverse_array(uint8_t *array, const uint8_t array_size);
char*	utils_convet_array_to_hex_string(char* string, const uint8_t *array, const uint8_t array_size);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

