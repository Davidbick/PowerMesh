# STM32 Bring-Up

This sets up the NUCLEO-F446RE environment.


## What This Does

- Builds STM32 project using **CMake + VSCode**
- Verifies:
  - LED blink
  - UART transmit

## Required Tools

Install the following:

- STM32CubeMX
- arm-none-eabi-gcc
- CMake
- VSCode + Extensions
  - C/C++, CMake Tools, Cortex-Debug
- OpenOCD
- STM32CubeProgrammer

## Build Instructions

From project root:

cmake --preset Debug
cmake --build --preset Debug

## Hardware Test (When Board Arrives)

1. Plug in NUCLEO-F446RE via USB  
2. Start debug in VSCode  
3. Verify:
   - Breakpoint at `main()` works  
   - LED blinks  
   - UART prints over serial terminal  

- Next steps:
  - SPI bring-up  
  - STM32 ↔ ST8500 communication  
