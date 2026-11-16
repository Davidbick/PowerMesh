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
#include "powermesh_board.h"
#include "uart_transport.h"
#include "packet_router.h"
#include <string.h>
#include <stdio.h>

// Network joining diagnostics.
#define POWERMESH_JOIN_MONITOR

// Temporary PHY-only bench test, disabled for G3 network joining.
// #define POWERMESH_PHY_DIRECT_TEST
#define POWERMESH_PHY_TEST_FRAME_COUNT 100U
#define POWERMESH_PHY_TEST_GAP_MS 200U
#if defined(POWERMESH_TRANSPORT_ST8500)
#include "st8500_boot.h"
#include "st8500_g3.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_lpuart1_rx;
DMA_HandleTypeDef hdma_lpuart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

/* USER CODE BEGIN PV */
static uart_transport_t gateway_transport;
static uart_transport_t remote_transport;
static packet_router_t router;

static packet_port_t gateway_port = PACKET_PORT_GATEWAY;
static packet_port_t remote_port = PACKET_PORT_REMOTE;

#if defined(POWERMESH_TRANSPORT_ST8500)
static st8500_g3_t g3;
static uart_frame_t g3_received_frame;
static uint16_t g3_rx_sequence = 0U;
#endif

static uint32_t last_blink_ms = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Adapt the shared router interface to the G4 HAL UART transport.
static bool send_uart_frame(
    void *context,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
)
{
    uart_transport_t *transport = (uart_transport_t *)context;

    return uart_transport_send(
        transport,
        type,
        sequence,
        payload,
        length
    ) == HAL_OK;
}

// Receive complete, CRC-validated frames from either UART.
static void on_uart_frame(const uart_frame_t *frame, void *context)
{
    if (frame == NULL || context == NULL)
    {
        return;
    }

    packet_port_t source = *(packet_port_t *)context;

    (void)packet_router_receive(
        &router,
        source,
        frame
    );
}

#if defined(POWERMESH_TRANSPORT_ST8500)

// Send the IPv6 payload through the G3 adaptation layer.
static bool send_g3_frame(
    void *context,
    uint8_t type,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t length
)
{
    st8500_g3_t *modem = (st8500_g3_t *)context;
    (void)sequence;

    // PowerMesh ACKs are local to each transport.
    if (type == UART_FRAME_TYPE_ACK)
    {
        return true;
    }

    if (type != UART_FRAME_TYPE_DATA)
    {
        return false;
    }

    return st8500_g3_send_ipv6(
        modem,
        payload,
        length
    );
}

// Forward received G3 IPv6 packets into the existing router.
static void on_g3_ipv6(
    const uint8_t *packet,
    uint16_t length,
    void *context
)
{
    (void)context;

    if (packet == NULL || length > UART_FRAME_MAX_PAYLOAD)
    {
        return;
    }

    g3_received_frame.version = UART_FRAME_VERSION;
    g3_received_frame.type = UART_FRAME_TYPE_DATA;
    g3_received_frame.sequence = g3_rx_sequence++;
    g3_received_frame.length = length;

    memcpy(g3_received_frame.payload, packet, length);

#if defined(POWERMESH_ROLE_GATEWAY)
    packet_port_t source = PACKET_PORT_REMOTE;
#else
    packet_port_t source = PACKET_PORT_GATEWAY;
#endif

    (void)packet_router_receive(
        &router,
        source,
        &g3_received_frame
    );
}

#endif
#if defined(POWERMESH_TRANSPORT_ST8500) && defined(POWERMESH_JOIN_MONITOR)

static void print_join_status(void)
{
    const char *status;

    if (g3.state == ST8500_G3_FAILED)
    {
        status = "ERROR";
    }
    else if (g3.coordinator)
    {
        status = g3.joined_devices > 0U ?
            "DEVICE JOINED" :
            (g3.state == ST8500_G3_NETWORK_READY ?
                "NETWORK STARTED, WAITING FOR DEVICE" :
                "WAITING FOR NETWORK START");
    }
    else
    {
        status = g3.state == ST8500_G3_NETWORK_READY ?
            "JOINED NETWORK" : "WAITING TO JOIN";
    }

    char line[160];

    int length = snprintf(
        line,
        sizeof(line),
        "[G3] %s | %s | PAN=0x%04X | Addr=0x%04X | Peers=%lu | CRC=%lu\r\n",
        g3.coordinator ? "GATEWAY" : "ENDPOINT",
        status,
        g3.pan_id,
        g3.short_addr,
        (unsigned long)g3.joined_devices,
        (unsigned long)g3.crc_errors
    );

    if (length > 0 && (size_t)length < sizeof(line) &&
        hlpuart1.gState == HAL_UART_STATE_READY)
    {
        (void)HAL_UART_Transmit(
            &hlpuart1,
            (uint8_t *)line,
            (uint16_t)length,
            100U
        );
    }
}

#endif

#if defined(POWERMESH_TRANSPORT_ST8500) && \
    (defined(POWERMESH_PHY_TEST_TX) || defined(POWERMESH_PHY_TEST_RX)) && \
    !defined(POWERMESH_PHY_DIRECT_TEST)

static void print_phy_status(void)
{
    st8500_phy_stats_t stats;

    
  if (!st8500_g3_phy_test_stats(&g3, &stats))
    {
      char line[180];

      int length = snprintf(
          line,
          sizeof(line),
          "[PHY] Stats failed | Reply=%u | Len=%u | "
          "Error=0x%02X | Timeouts=%lu | CRC=%lu | UART=%lu\r\n",
          g3.reply_received ? 1U : 0U,
          g3.reply_length,
          g3.reply_error,
          (unsigned long)g3.command_timeouts,
          (unsigned long)g3.crc_errors,
          (unsigned long)g3.uart_errors
      );

      if (length > 0 && (size_t)length < sizeof(line))
      {
          (void)HAL_UART_Transmit(
              &hlpuart1,
              (uint8_t *)line,
              (uint16_t)length,
              100U
          );
      }

      char bytes[100];
      int pos = snprintf(bytes, sizeof(bytes), "[PHY] Raw:");

      for (uint16_t i = 0; i < g3.reply_length && i < 13U; i++)
      {
          pos += snprintf(
              bytes + pos,
              sizeof(bytes) - pos,
              " %02X",
              g3.reply[i]
          );
      }

      pos += snprintf(bytes + pos, sizeof(bytes) - pos, "\r\n");

      if (pos > 0 && (size_t)pos < sizeof(bytes))
      {
          HAL_UART_Transmit(
              &hlpuart1, (uint8_t *)bytes, (uint16_t)pos, 100U
          );
      }

      return;
  }

#if defined(POWERMESH_PHY_TEST_TX)
    const char *role = "TX";
#else
    const char *role = "RX";
#endif

    char line[180];

    int length = snprintf(
        line,
        sizeof(line),
        "[PHY] %s | Packets=%lu | Bytes=%lu | Duration=%lums"
        " | Indications=%lu | Matches=%lu | HostCRC=%lu\r\n",
        role,
        (unsigned long)stats.packets,
        (unsigned long)stats.bytes,
        (unsigned long)stats.duration_ms,
        (unsigned long)g3.phy_rx_indications,
        (unsigned long)g3.phy_rx_matches,
        (unsigned long)g3.crc_errors
    );

    if (length > 0 && (size_t)length < sizeof(line))
    {
        (void)HAL_UART_Transmit(
            &hlpuart1, (uint8_t *)line, (uint16_t)length, 100U);
    }
}

#endif

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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */

#if defined(POWERMESH_TRANSPORT_WIRED)
  if (POWERMESH_GATEWAY_UART->hdmarx == NULL ||
      POWERMESH_GATEWAY_UART->hdmatx == NULL ||
      POWERMESH_REMOTE_UART->hdmarx == NULL ||
      POWERMESH_REMOTE_UART->hdmatx == NULL)
  {
      Error_Handler();
  }

  if (!packet_router_init(
          &router,
          send_uart_frame,
          &gateway_transport,
          send_uart_frame,
          &remote_transport))
  {
      Error_Handler();
  }

  if (uart_transport_start(
          &gateway_transport,
          POWERMESH_GATEWAY_UART,
          on_uart_frame,
          &gateway_port) != HAL_OK)
  {
      Error_Handler();
  }

  if (uart_transport_start(
          &remote_transport,
          POWERMESH_REMOTE_UART,
          on_uart_frame,
          &remote_port) != HAL_OK)
  {
      Error_Handler();
  }

#if defined(POWERMESH_ROLE_ENDPOINT)
  static uint8_t boot_message[] = "PowerMesh G4 ready\r\n";

  (void)HAL_UART_Transmit(
      POWERMESH_DEBUG_UART,
      boot_message,
      sizeof(boot_message) - 1U,
      100U
  );
#endif
#endif

  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

#if defined(POWERMESH_TRANSPORT_ST8500)
  if (!st8500_boot_load(POWERMESH_LINK_UART))
  {
      while (1)
      {
          (void)BSP_LED_Toggle(LED_GREEN);
          HAL_Delay(150U);
      }
  }

#if defined(POWERMESH_PHY_TEST_TX) || defined(POWERMESH_PHY_TEST_RX)

#if defined(POWERMESH_PHY_DIRECT_TEST)
  if (!st8500_g3_start_phy(&g3, POWERMESH_LINK_UART))
  {
      Error_Handler();
  }

#if defined(POWERMESH_PHY_TEST_TX)

  bool tx_ready = st8500_g3_phy_set_tx(&g3);

  if (!tx_ready)
  {
      static uint8_t message[] = "[PHY TX] Failed to enter TX state\r\n";
      (void)HAL_UART_Transmit(
          &hlpuart1, message, sizeof(message) - 1U, 100U
      );
      Error_Handler();
  }

  static uint8_t start_message[] =
      "[PHY TX] Ready | Sending 100 frames after 2 seconds\r\n";
  (void)HAL_UART_Transmit(
      &hlpuart1, start_message, sizeof(start_message) - 1U, 100U
  );

  HAL_Delay(2000U);

  uint32_t successful = 0U;

  for (uint32_t frame = 1U; frame <= POWERMESH_PHY_TEST_FRAME_COUNT; frame++)
  {
      bool host_ok = st8500_g3_phy_send_once(&g3);

      bool phy_ok = host_ok && g3.reply_received &&
          g3.reply_length == 5U && g3.reply[0] == 0U;

      if (phy_ok)
      {
          successful++;
      }

      char line[220];
      int length = snprintf(
          line,
          sizeof(line),
          "[PHY TX] Frame=%lu | HostOK=%u | Reply=%u | Len=%u | "
          "HostErr=0x%02X | Status=0x%02X | "
          "PowerStart=%u | PowerEnd=%u | Thermal=0x%02X\r\n",
          (unsigned long)frame,
          host_ok ? 1U : 0U,
          g3.reply_received ? 1U : 0U,
          g3.reply_length,
          g3.reply_received ? g3.reply_error : 0xFFU,
          g3.reply_received && g3.reply_length >= 1U ?
              g3.reply[0] : 0xFFU,
          g3.reply_received && g3.reply_length >= 3U ?
              g3.reply[2] : 0U,
          g3.reply_received && g3.reply_length >= 4U ?
              g3.reply[3] : 0U,
          g3.reply_received && g3.reply_length >= 5U ?
              g3.reply[4] : 0xFFU
      );

      if (length > 0 && (size_t)length < sizeof(line))
      {
          (void)HAL_UART_Transmit(
              &hlpuart1, (uint8_t *)line, (uint16_t)length, 100U
          );
      }

      // Keep the 200 ms gap; thermal flags are reported, not a stop condition.
      if (frame < POWERMESH_PHY_TEST_FRAME_COUNT)
      {
          HAL_Delay(POWERMESH_PHY_TEST_GAP_MS);
      }
  }

  char summary[112];
  int summary_length = snprintf(
      summary,
      sizeof(summary),
      "[PHY TX] Complete | Success=%lu | Failed=%lu | HostCRC=%lu\r\n",
      (unsigned long)successful,
      (unsigned long)(POWERMESH_PHY_TEST_FRAME_COUNT - successful),
      (unsigned long)g3.crc_errors
  );

  if (summary_length > 0 && (size_t)summary_length < sizeof(summary))
  {
      (void)HAL_UART_Transmit(
          &hlpuart1, (uint8_t *)summary, (uint16_t)summary_length, 100U
      );
  }

#else

  bool rx_ready = st8500_g3_phy_set_rx(&g3);
  char line[150];
  int length = snprintf(
      line,
      sizeof(line),
      "[PHY RX] RXReady=%u | Reply=%u | Status=0x%02X | Listening\r\n",
      rx_ready ? 1U : 0U,
      g3.reply_received ? 1U : 0U,
      g3.reply_received && g3.reply_length >= 1U ?
          g3.reply[0] : 0xFFU
  );

  if (length > 0 && (size_t)length < sizeof(line))
  {
      (void)HAL_UART_Transmit(
          &hlpuart1, (uint8_t *)line, (uint16_t)length, 100U
      );
  }

  if (!rx_ready)
  {
      Error_Handler();
  }

#endif

#else

#if defined(POWERMESH_PHY_TEST_TX)
  const bool phy_transmit = true;
#else
  const bool phy_transmit = false;
#endif

  if (!st8500_g3_start_phy(&g3, POWERMESH_LINK_UART) ||
      !st8500_g3_phy_test_start(&g3, phy_transmit))
  {
      Error_Handler();
  }

#endif

#else

#if defined(POWERMESH_ROLE_GATEWAY)
  const bool coordinator = true;
#else
  const bool coordinator = false;
#endif

  if (!st8500_g3_start(
          &g3,
          POWERMESH_LINK_UART,
          coordinator))
  {
      while (1)
      {
          (void)BSP_LED_Toggle(LED_GREEN);
          HAL_Delay(150U);
      }
  }

  uart_transport_t *local_transport;
  UART_HandleTypeDef *local_uart;
  packet_port_t *local_port;
  bool router_ready;

#if defined(POWERMESH_ROLE_GATEWAY)
  local_transport = &gateway_transport;
  local_uart = POWERMESH_GATEWAY_UART;
  local_port = &gateway_port;

  router_ready = packet_router_init(
      &router,
      send_uart_frame,
      &gateway_transport,
      send_g3_frame,
      &g3
  );
#else
  local_transport = &remote_transport;
  local_uart = POWERMESH_REMOTE_UART;
  local_port = &remote_port;

  router_ready = packet_router_init(
      &router,
      send_g3_frame,
      &g3,
      send_uart_frame,
      &remote_transport
  );
#endif

  if (!router_ready)
  {
      Error_Handler();
  }

  // Clear UART data received during ST8500 firmware loading.
  __HAL_UART_CLEAR_OREFLAG(local_uart);
  __HAL_UART_SEND_REQ(local_uart, UART_RXDATA_FLUSH_REQUEST);

  if (uart_transport_start(
          local_transport,
          local_uart,
          on_uart_frame,
          local_port) != HAL_OK)
  {
      Error_Handler();
  }

  st8500_g3_set_receive_callback(
      &g3,
      on_g3_ipv6,
      NULL
  );
#endif
#endif

  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if defined(POWERMESH_TRANSPORT_WIRED)
    uart_transport_poll(&gateway_transport);
    uart_transport_poll(&remote_transport);

    packet_router_poll(&router);
#endif

#if defined(POWERMESH_TRANSPORT_ST8500)
    st8500_g3_poll(&g3);

#if defined(POWERMESH_PHY_TEST_TX) || defined(POWERMESH_PHY_TEST_RX)
#if defined(POWERMESH_PHY_DIRECT_TEST) && defined(POWERMESH_PHY_TEST_RX)

    static uint32_t last_rx_report_ms = 0U;
    static uint32_t last_rx_indications = 0U;
    uint32_t tick = HAL_GetTick();

    bool new_frame = g3.phy_rx_indications != last_rx_indications;

    if (new_frame || (uint32_t)(tick - last_rx_report_ms) >= 5000U)
    {
        last_rx_indications = g3.phy_rx_indications;
        last_rx_report_ms = tick;

        char line[160];
        int length = snprintf(
            line,
            sizeof(line),
            "[PHY RX] Indications=%lu | Matches=%lu | "
            "HostCRC=%lu | UART=%lu\r\n",
            (unsigned long)g3.phy_rx_indications,
            (unsigned long)g3.phy_rx_matches,
            (unsigned long)g3.crc_errors,
            (unsigned long)g3.uart_errors
        );

        if (length > 0 && (size_t)length < sizeof(line))
        {
            (void)HAL_UART_Transmit(
                &hlpuart1, (uint8_t *)line, (uint16_t)length, 100U
            );
        }

        if (new_frame)
        {
            const st8500_phy_rx_debug_t *debug = &g3.phy_rx_debug;
            const char *check = "Unknown";

            switch (debug->check)
            {
                case ST8500_PHY_RX_HOST_ERROR: check = "HostError"; break;
                case ST8500_PHY_RX_SHORT_HEADER: check = "ShortHeader"; break;
                case ST8500_PHY_RX_BAD_LENGTH: check = "BadLength"; break;
                case ST8500_PHY_RX_PATTERN_MISMATCH: check = "Mismatch"; break;
                case ST8500_PHY_RX_MATCH: check = "Expected120Match"; break;
                default: break;
            }

            char details[160];
            int count = snprintf(
                details, sizeof(details),
                "[PHY RX DBG] HostErr=0x%02X | DataLen=%u | "
                "PayloadLen=%u | Check=%s | FirstDiff=%u | ExtraBytes=%u\r\n",
                debug->host_error,
                debug->data_length,
                debug->payload_length,
                check,
                debug->first_mismatch,
                debug->extra_bytes
            );

            if (count > 0 && (size_t)count < sizeof(details))
            {
                (void)HAL_UART_Transmit(
                    &hlpuart1, (uint8_t *)details, (uint16_t)count, 100U
                );
            }

            if (debug->extra_bytes == 1U)
            {
                char trailing[80];
                int trailing_length = snprintf(
                    trailing, sizeof(trailing),
                    "[PHY RX DBG] ExtraByte[120]=0x%02X\r\n",
                    debug->extra_byte
                );

                if (trailing_length > 0 &&
                    (size_t)trailing_length < sizeof(trailing))
                {
                    (void)HAL_UART_Transmit(
                        &hlpuart1, (uint8_t *)trailing,
                        (uint16_t)trailing_length, 100U
                    );
                }
            }

            char bytes[128];
            int pos = snprintf(bytes, sizeof(bytes), "[PHY RX DBG] Header8:");

            for (uint8_t i = 0U; i < debug->header_bytes_length &&
                 pos > 0 && (size_t)pos < sizeof(bytes); i++)
            {
                pos += snprintf(
                    &bytes[pos], sizeof(bytes) - (size_t)pos,
                    " %02X", debug->header_bytes[i]
                );
            }

            if (pos > 0 && (size_t)pos < sizeof(bytes))
            {
                pos += snprintf(
                    &bytes[pos], sizeof(bytes) - (size_t)pos,
                    " | First16:"
                );
            }

            for (uint8_t i = 0U; i < debug->first_bytes_length &&
                 pos > 0 && (size_t)pos < sizeof(bytes); i++)
            {
                pos += snprintf(
                    &bytes[pos], sizeof(bytes) - (size_t)pos,
                    " %02X", debug->first_bytes[i]
                );
            }

            if (pos > 0 && (size_t)pos < sizeof(bytes))
            {
                pos += snprintf(
                    &bytes[pos], sizeof(bytes) - (size_t)pos, "\r\n"
                );
            }

            if (pos > 0 && (size_t)pos < sizeof(bytes))
            {
                (void)HAL_UART_Transmit(
                    &hlpuart1, (uint8_t *)bytes, (uint16_t)pos, 100U
                );
            }
        }
    }

#elif !defined(POWERMESH_PHY_DIRECT_TEST)

    static uint32_t last_phy_report_ms = 0U;
    uint32_t phy_tick = HAL_GetTick();

    if ((uint32_t)(phy_tick - last_phy_report_ms) >= 5000U)
    {
        last_phy_report_ms = phy_tick;
        print_phy_status();
    }

#endif

#else
    uart_transport_poll(local_transport);
    packet_router_poll(&router);

#if defined(POWERMESH_JOIN_MONITOR)
    static uint32_t last_report_ms = 0U;
    static uint32_t last_peers = 0xFFFFFFFFU;
    static st8500_g3_state_t last_state = ST8500_G3_STOPPED;

    uint32_t tick = HAL_GetTick();

    if (g3.state != last_state ||
        g3.joined_devices != last_peers ||
        (uint32_t)(tick - last_report_ms) >= 5000U)
    {
        print_join_status();

        last_report_ms = tick;
        last_state = g3.state;
        last_peers = g3.joined_devices;
    }
#endif
#endif
#endif

    // Toggle the status LED every 500 ms.
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_blink_ms) >= 500U)
    {
        last_blink_ms = now;
        (void)BSP_LED_Toggle(LED_GREEN);
    }
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ST8500_RESET_N_GPIO_Port, ST8500_RESET_N_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : ST8500_RESET_N_Pin */
  GPIO_InitStruct.Pin = ST8500_RESET_N_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ST8500_RESET_N_GPIO_Port, &GPIO_InitStruct);

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
