# PowerMesh: PLC Smart Home
PowerMesh is a smart-home research project exploring Matter and Home Assistant over G3-PLC using the ST8500 modem and STLD1 analog front end. The current working prototype uses wired UART connections between development boards to validate bidirectional IPv6/Matter communication before PLC hardware is inserted. 

## Current Working Architecture

```text
Home Assistant / Matter Server (Raspberry Pi)
              |
      IPv6 TUN (powermesh0)
              |
        UART bridge (Pi)
              |
      STM32F446RE (F4)
              |
       Wired UART link
              |
      STM32G474RE (G4)
              |
       Wired UART link
              |
       ESP32-S3 Matter light
```

Packets travel in **both directions** through the wired chain. The STM32 firmware handles framing, CRC validation, buffering, and forwarding; the ESP32 runs the Matter endpoint. This setup does **not** yet include ST8500, STLD1, the analog PLC front end, or the power-line medium.

For active instructions, see [RaspberryPi](RaspberryPi/README.md), [STM32 projects](STM32/Projects/README.md), and [ESP32 wired endpoint](ESP32_Testing/Wired/README.md). Earlier wireless and ST8500 experiments are kept for reference, not as the active packet path.

## Task Designation:

1) PCB Design : Bryan, Daniel
- **Goal:** Contribute to PCB design of the PLC hub on the analog and digital boards to create a complete board design with both aspects.

2) Perf Board, ST IC Prototyping: Logan, David
- **Goal:**  Have code functionality with ST8500 and STLD1 chips using the STM32F4 so to have groundwork for software integration on PCB.

3) Home Assistant Setup: Steven
- **Goal:**  Have the Home Assistant platform working with STM32 and temporary endpoint (RPI4) to seemlessly integrate into design.

## Completed Work: Pre-Alpha Build

- Principal component research. We've narrowed our options to the ST8500 and STLD1 chips that powered a ST PLC development board.
- We've started PCB design on the hub infrastructure. Mostly on the digital side of the board with the STM32F4, ST8500, and STLD1.
- Started STM32 bring-up by writing and testing basic code for UART and peripherals without having the final hardware. Also planned out how the STM32 will communicate with the ST8500.

## Completed Work: Design Prototype
- We've completed schematic design for our analog and digital board with plans to complete layout before the beginning of next semester.
- We've integrated Home Assistant wirelessly with our RaspberryPi host device with plans to complete wired integration with an LED circuit.
- We've narrowed our software scope to the G3 stack after discussions with ST about IEEE 1901.2 compatibility. ST8500 integration remains a future stage.

## Completed Work: Beta Build
- Home Assistant TUN interface and Raspberry Pi UART bridge
- End-to-end bidirectional wired Matter communication through F4, G4, and ESP32-S3
- PCB digital and analog designs (PLC hardware integration still pending)

## Build and Test

1. Prepare the Raspberry Pi bridge with `bash RaspberryPi/install.sh` once (run as your normal Pi user).
2. Build and flash the F4 and G4 firmware under `STM32/Projects/`.
3. Build and flash `ESP32_Testing/Wired/`, then commission the Matter light.
4. Verify both directions using the TUN interface, Pi bridge service logs, and the Matter light state in Home Assistant.

See the component-specific READMEs for wiring, build dependencies, and diagnostics. The Pi installer configures a systemd service to start the TUN and UART bridge automatically after boot.

## Remaining Work
- Integrate and validate the ST8500 modem interface and analog PLC front end.
- Test the actual power-line transport; the present working chain is wired UART only.
- Continue end-to-end testing as the PLC transport replaces the temporary UART link.
