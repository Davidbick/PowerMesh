# Historical F4 / ST8500 experiment

This is an early, standalone test of an STM32F4 sending a boot-version MIB command to an ST8500. It is **not** part of the current bidirectional Matter/IPv6 packet path.

The implementation uses hardcoded bootloader request/response fields and has not been validated here against the complete ST8500 host protocol. Do not reuse it as a production ST8500 host driver without checking the official ST documentation and validating response length, CRC, startup sequence, and error handling.

The active prototype uses `../powermesh_stm32_bringup/` (F4), `../powermesh_g4_bridge/` (G4), and the common framing/forwarding code in `../../Common/`.
