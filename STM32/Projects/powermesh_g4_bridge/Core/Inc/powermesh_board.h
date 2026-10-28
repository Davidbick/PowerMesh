#ifndef POWERMESH_BOARD_H
#define POWERMESH_BOARD_H

#include "main.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef hlpuart1;

// Temporary F4 link, eventually replaced by the ST8500 transport
#define POWERMESH_LINK_UART    (&huart3)

// ESP32 endpoint connection
#define POWERMESH_ESP32_UART   (&huart1)

// ST-LINK virtual COM port
#define POWERMESH_DEBUG_UART   (&hlpuart1)

#define POWERMESH_UART_BAUD    115200U

#endif
