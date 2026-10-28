#ifndef POWERMESH_BOARD_H
#define POWERMESH_BOARD_H

#include "usart.h"

// Raspberry Pi gateway connection
#define POWERMESH_PI_UART       (&huart2)

// Remote STM32 connection
#define POWERMESH_REMOTE_UART   (&huart1)

#define POWERMESH_UART_BAUD     115200U

#endif
