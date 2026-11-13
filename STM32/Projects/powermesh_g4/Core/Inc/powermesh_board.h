#ifndef POWERMESH_BOARD_H
#define POWERMESH_BOARD_H

#include "main.h"

#if defined(POWERMESH_ROLE_GATEWAY) == defined(POWERMESH_ROLE_ENDPOINT)
#error "Select exactly one PowerMesh G4 role"
#endif

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef hlpuart1;

// Temporary wired link, later used for ST8500
#define POWERMESH_LINK_UART (&huart3)

#if defined(POWERMESH_ROLE_GATEWAY)

#define POWERMESH_GATEWAY_UART (&hlpuart1)
#define POWERMESH_REMOTE_UART  POWERMESH_LINK_UART

#else

#define POWERMESH_GATEWAY_UART POWERMESH_LINK_UART
#define POWERMESH_REMOTE_UART  (&huart1)
#define POWERMESH_DEBUG_UART   (&hlpuart1)

#endif

#define POWERMESH_UART_BAUD 115200U

#endif
