#ifndef ST8500_BOOT_H
#define ST8500_BOOT_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

typedef enum
{
    ST8500_BOOT_IDLE = 0,
    ST8500_BOOT_RESET,
    ST8500_BOOT_PROBE,
    ST8500_BOOT_BAUD,
    ST8500_BOOT_RTE_HEADER,
    ST8500_BOOT_RTE_DATA,
    ST8500_BOOT_RTE_START,
    ST8500_BOOT_PE_HEADER,
    ST8500_BOOT_PE_DATA,
    ST8500_BOOT_PE_START,
    ST8500_BOOT_IMAGES_STARTED
} st8500_boot_stage_t;

extern volatile st8500_boot_stage_t st8500_boot_stage;
extern volatile uint32_t st8500_boot_bytes_written;
extern volatile uint32_t st8500_boot_attempt;

bool st8500_boot_load(UART_HandleTypeDef *uart);

#endif
