# PowerMesh: PLC Smart Home
PowerMesh is a smart home system that uses power line communication (PLC) instead of Wi-Fi to connect devices, enabling more stable and secure control over existing electrical wiring. It integrates Matter and Home Assistant to support cross-platform device control while exploring IP-based communication over power lines. 

## Project Architecture 
PowerMesh leverages PLC communication to integrate a Matter-based protocol built atop IEEE 1901.2. The user via our protocol can communication with one of our "hubs" to control an LED bulb at the other end of the Powerline.

Our user interacts with Home Assistant which transfers the commands via the ST8500 and STLD1 chips. The commands control the state of the endpoint lightbulbs and has the option to scale to various other receiver hubs connected to the powerlines.



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

## Complete Work: Prototype Design
- We've completed schematic design for our analog and digital board with plans to complete layout before the beginning of next semester.
- We've integrated Home Assistant wirelessly with our RaspberryPi host device with plans to complete wired integration with an LED circuit.
- We've narrowed our software scope to the G3 stack after ST confirmed indirect IEEE 1901.2 compatibility. We'll have ST8500 integration before the beginning of next semester.
