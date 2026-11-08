/*****************************************************************************
*   @file    modbus_config.h
*   @author  Alejandro Mera, modified by AMG/IPC Application Team
*   @brief   Configuration file for the modbus library for STM32.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef MODBUS_CONFIG_H_
#define MODBUS_CONFIG_H_

/* Set to 1 the following line to enable support for Modbus RTU USART DMA mode. */
#define ENABLE_MODBUS_USART_DMA 	0

#define T35  			5       // Timer T35 period (in ticks) for end frame detection.
#define MAX_BUFFER  	128	    // Maximum size for the communication buffer in bytes.
#define TIMEOUT_MODBUS 	1000 	// Timeout for master query (in ticks)
#define MAX_M_HANDLERS 	1    	// Maximum number of modbus handlers that can work concurrently
#define MAX_TELEGRAMS 	2     	// Max number of Telegrams in master queue

#endif /* MODBUS_CONFIG_H_ */
