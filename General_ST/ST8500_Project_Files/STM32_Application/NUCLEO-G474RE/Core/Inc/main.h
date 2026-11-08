/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <assert.h>
#include <settings.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SD_CS_GPIO_Port SDC_CS_N_GPIO_Port
#define huartHostIf huart4
#define huartModbus huart1
#define hspiSFlash hspi2
#define SD_CS_Pin SDC_CS_N_Pin
#define huartUserIf huart2
#define SD_SPI_HANDLE hspi2
#define htimSys htim6
#define USER_PUSHBUTTON_Pin GPIO_PIN_13
#define USER_PUSHBUTTON_GPIO_Port GPIOC
#define USER_PUSHBUTTON_EXTI_IRQn EXTI15_10_IRQn
#define ST8500_RESET_N_Pin GPIO_PIN_2
#define ST8500_RESET_N_GPIO_Port GPIOC
#define RF_MODULE_SEL_Pin GPIO_PIN_3
#define RF_MODULE_SEL_GPIO_Port GPIOC
#define MODBUS_DE_Pin GPIO_PIN_1
#define MODBUS_DE_GPIO_Port GPIOA
#define UIF_TX_Pin GPIO_PIN_2
#define UIF_TX_GPIO_Port GPIOA
#define UIF_RX_Pin GPIO_PIN_3
#define UIF_RX_GPIO_Port GPIOA
#define LPMODE_N_Pin GPIO_PIN_4
#define LPMODE_N_GPIO_Port GPIOA
#define GPIO0_06_Pin GPIO_PIN_6
#define GPIO0_06_GPIO_Port GPIOA
#define GPIO0_07_Pin GPIO_PIN_7
#define GPIO0_07_GPIO_Port GPIOA
#define MODBUS_TX_Pin GPIO_PIN_4
#define MODBUS_TX_GPIO_Port GPIOC
#define MODBUS_RX_Pin GPIO_PIN_5
#define MODBUS_RX_GPIO_Port GPIOC
#define BOOT1_IN_Pin GPIO_PIN_1
#define BOOT1_IN_GPIO_Port GPIOB
#define STM32_SYNC_Pin GPIO_PIN_2
#define STM32_SYNC_GPIO_Port GPIOB
#define SDC_DETECT_Pin GPIO_PIN_11
#define SDC_DETECT_GPIO_Port GPIOB
#define SDC_CS_N_Pin GPIO_PIN_12
#define SDC_CS_N_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_6
#define LED3_GPIO_Port GPIOC
#define MAC_MODE_SEL_Pin GPIO_PIN_7
#define MAC_MODE_SEL_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_8
#define LED2_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_9
#define LED1_GPIO_Port GPIOC
#define SFLASH_CS_N_Pin GPIO_PIN_8
#define SFLASH_CS_N_GPIO_Port GPIOA
#define HIF_TX_Pin GPIO_PIN_10
#define HIF_TX_GPIO_Port GPIOC
#define HIF_RX_Pin GPIO_PIN_11
#define HIF_RX_GPIO_Port GPIOC
#define BANDPLAN_SEL_Pin GPIO_PIN_12
#define BANDPLAN_SEL_GPIO_Port GPIOC
#define LED0_Pin GPIO_PIN_2
#define LED0_GPIO_Port GPIOD
#define MODBUS_MASTER_SEL_Pin GPIO_PIN_3
#define MODBUS_MASTER_SEL_GPIO_Port GPIOB
#define BOOT2_Pin GPIO_PIN_5
#define BOOT2_GPIO_Port GPIOB
#define BOOT0_Pin GPIO_PIN_7
#define BOOT0_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
