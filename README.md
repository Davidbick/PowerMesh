# PowerMesh Smart Home

Task Designation:

1) PCB Design : Bryan, Daniel
- Goal: Have progress on the PCB design aspects of the project so that we can present either a complete schematic or a thorough prototype so that we can order it first thing 2nd semester.,
- I'm planning on having an STM32 board and a PLC board with the ST8500 and the STLD1 chips that will house the coupling circuit.,
- I think splitting both boards would be a good idea so Daniel can likely work on the PLC board and I'll join once I finish the STM32 board which should be easier.,

2) Perf board setup: Logan
- Start communication between the 3 st chips using the PERF boards robert bought/already has.,
- Connect to the STM devboards and try to drive the ST8500 and the STLD1. While we won't be connected to the powerlines we can test functionality of the chips and start designing aspects of the coupling circuit potentially.,
- Goal: Have basic code functionality with the stm chips so that we can re-use that code on the final design with the PCB or in a robust breadboard testing.,

3) Home Assistant Setup: Steven
- Goal: Have the Home Assistant platform already working but with STM32 so we can just swap in the code once we get the PLC stuff working.,
- Specific Goals: Connect STM32 to Rpi4 with any serial communication (not sure if this will be the final method), Have home assistant interact with the bulb already MAYBE( might be harder than expected but we have bulbs to use).,
- This also allows someone to get substantial work done with the STM32 Nucleo and the Rpi4 that the last team left.,
- Dr. Del Rocco mentioned Home Assistant and I figured we could start work on this so we can have something substantial to show in the presentation.
- The benefit with on working with this, is once the nucleo arrives you should have everything to work on this.
