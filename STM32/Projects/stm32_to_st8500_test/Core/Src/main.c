/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ST8500_SYNC_BYTE       0x16
#define ST8500_MODE            0xFE

#define ST8500_CMD_MIB_GET     0x21

#define ST8500_MIB_BOOT_VER    0x04

#define ST8500_TIMEOUT_MS      500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static uint16_t ST8500_CRC16_XMODEM(const uint8_t *data, uint16_t length);
static HAL_StatusTypeDef ST8500_ReadBootVersion(uint32_t *bootVersion);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
  /* USER CODE BEGIN 0 */
  /*
  * Calculate CRC-16/XMODEM.
  *
  * ST's ST8500 host interface uses CRC protection on messages.
  * This implementation can later be reused for additional
  * ST8500 commands.
  */
  static uint16_t ST8500_CRC16_XMODEM(const uint8_t *data, uint16_t length)
  {
      uint16_t crc = 0x0000;

      for (uint16_t i = 0; i < length; i++)
      {
          crc ^= ((uint16_t)data[i] << 8);

          for (uint8_t bit = 0; bit < 8; bit++)
          {
              if (crc & 0x8000)
              {
                  crc = (crc << 1) ^ 0x1021;
              }
              else
              {
                  crc <<= 1;
              }
          }
      }

      return crc;
  }


  /*
  * Ask the ST8500 bootloader for its boot version.
  *
  * Returns HAL_OK if a valid ST8500 response is received.
  */
  static HAL_StatusTypeDef ST8500_ReadBootVersion(uint32_t *bootVersion)
  {
      /*
      * MIB_Get request:
      *
      * [0..1]  Sync
      * [2]     Command ID
      * [3..4]  Message length
      * [5]     Mode
      * [6..9]  State/reserved
      * [10]    MIB attribute
      * [11..12] CRC
      */

      uint8_t txBuffer[13] = {0};

      txBuffer[0] = ST8500_SYNC_BYTE;
      txBuffer[1] = ST8500_SYNC_BYTE;

      txBuffer[2] = ST8500_CMD_MIB_GET;

      /* Payload contains one byte: the MIB ID */
      txBuffer[3] = 0x01;
      txBuffer[4] = 0x00;

      txBuffer[5] = ST8500_MODE;

      /* State field for request */
      txBuffer[6] = 0x00;
      txBuffer[7] = 0x00;
      txBuffer[8] = 0x00;
      txBuffer[9] = 0x00;

      /* Request ST8500 bootloader version */
      txBuffer[10] = ST8500_MIB_BOOT_VER;

      uint16_t crc = ST8500_CRC16_XMODEM(txBuffer, 11);

      /* CRC is transmitted little-endian */
      txBuffer[11] = (uint8_t)(crc & 0xFF);
      txBuffer[12] = (uint8_t)((crc >> 8) & 0xFF);

      /*
      * Boot version MIB is four bytes, so the expected
      * complete response is 16 bytes.
      */
      uint8_t rxBuffer[16] = {0};

      HAL_StatusTypeDef status;

      status = HAL_UART_Transmit(
          &huart2,
          txBuffer,
          sizeof(txBuffer),
          ST8500_TIMEOUT_MS
      );

      if (status != HAL_OK)
      {
          return status;
      }

      status = HAL_UART_Receive(
          &huart2,
          rxBuffer,
          sizeof(rxBuffer),
          ST8500_TIMEOUT_MS
      );

      if (status != HAL_OK)
      {
          return status;
      }

      /* Check synchronization bytes */
      if ((rxBuffer[0] != ST8500_SYNC_BYTE) ||
          (rxBuffer[1] != ST8500_SYNC_BYTE))
      {
          return HAL_ERROR;
      }

      /* Response should match our MIB_GET command */
      if (rxBuffer[2] != ST8500_CMD_MIB_GET)
      {
          return HAL_ERROR;
      }

      /*
      * STATE = 0 means ACK.
      * Bytes 6-9 form the four-byte state field.
      */
      if ((rxBuffer[6] != 0x00) ||
          (rxBuffer[7] != 0x00) ||
          (rxBuffer[8] != 0x00) ||
          (rxBuffer[9] != 0x00))
      {
          return HAL_ERROR;
      }

      /*
      * MIB boot version is four bytes.
      * ST8500 protocol uses little-endian fields.
      */
      *bootVersion =
            ((uint32_t)rxBuffer[10])
          | ((uint32_t)rxBuffer[11] << 8)
          | ((uint32_t)rxBuffer[12] << 16)
          | ((uint32_t)rxBuffer[13] << 24);

      return HAL_OK;
  }
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  uint32_t st8500BootVersion = 0;

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  HAL_Delay(100);

  if (ST8500_ReadBootVersion(&st8500BootVersion) == HAL_OK)
  {
      /*
      * ST8500 responded successfully.
      * Turn on the Nucleo LED.
      */
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  }
  else
  {
      /*
      * No valid response.
      * Leave LED off.
      */
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
