# Home Assistant

Home Assistant (HA) is an open-source software that will act as the hub to connect with the smart devices.

## How it will be implemented

Using the RPI3 of the previous group, a micro SD card housing HA OS will be inserted into the RPI3. The RPI3 will be powered on using a micro USB and an ethernet cable will be connected to the RPI3 and a router. Once everything is connected, powered on, and the configuration is all set, the HA dashboard can be accessed using http://homeassistant.local:8123/ or the HA companion app. Commands will be sent via HA, and the statuses of the smart devices connected will also be sent to HA via the RPI hub. This should be done via using the Matter standard.

## Current bugs

- When initially setting up the HA OS, a white screen with the HA logo appeared after using http://homeassistant.local:8123/. This persisted for more than an hour. The likely cause is that the browser caches are causing some type of issue. It could also be the hardware limitations of the RPI3 running HA OS for the first time. The fix would be using the companion app, or using http://IP_ADDRESS:8123, where IP_ADDRESS is the IP address of the RPI in a web browser. However, after accessing the dashboard for the first time, using http://homeassistant.local:8123/ no longer causes a problem. I can no longer replicate the bug, but I am assuming if I did a fresh install of HA OS on to the micro SD again it might happen again.

## Work completed

- Able to send a command from HA to a Matter-enabled smart bulb, which changed its color (Matter over Wi-Fi)

## Planned work

- Make RPI3 send the command out through UART instead of Wi-Fi (might have to disable Bluetooth, which might cause a problem for comissioning new Matter-enabled smart devices on to HA)
- Connect the RPI3 and STM32 via SPI so that the RPI3 sends the command to the STM32
- Get a ESP32 S3 dev kit that has enough RAM to store the Matter SDK via esp-matter. This will act as a DIY Matter-enabled smart bulb. Turn off wireless capabilities except Bluetooth (used for comissioning new smart devices). Test if Matter commands can control the RGB LED on it via PWM
