# STM32 firmware projects

The current wired PowerMesh prototype uses two STM32 development boards. Both active projects build with STM32CubeMX-generated CMake projects and share packet framing, queues, routing, and DMA UART transport code from `../Common/`.

| Project | Role | Status |
| --- | --- | --- |
| `powermesh_stm32_bringup/` | STM32F446RE gateway, Raspberry Pi UART to remote UART | Active |
| `powermesh_g4_bridge/` | STM32G474RE bridge, F4 UART to ESP32 UART | Active |
| `stm32_to_st8500_test/` | Earlier standalone STM32F4 / ST8500 bootloader experiment | Historical; not the active transport |

## Build

Install STM32CubeMX, an Arm GCC toolchain, CMake, OpenOCD, and the VS Code Cortex-Debug extension if debugging in VS Code.

From the active project directory:

```bash
cmake --preset Debug
cmake --build build/Debug
```

Use each project's `.ioc` file when changing peripheral configuration. The active projects share code in `STM32/Common/`; neither has an integrated ST8500 modem driver yet.

To run the complete wired test, connect Pi to F4, F4 to G4, and G4 to ESP32 as documented in the platform READMEs. The eventual PLC integration will replace the F4-to-G4 test link with ST8500 modem transport; it is not an SPI link.
