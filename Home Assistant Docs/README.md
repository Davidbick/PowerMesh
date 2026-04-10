# Home Assistant

Home Assistant (HA) is an open-source software that will act as the hub to connect with the smart devices.

## How it will be implemented

Using the RPI3 of the previous group, a micro SD card housing HA OS will be inserted into the RPI3. The RPI3 will be powered on using a micro USB and an ethernet cable will be connected to the RPI3 and a router. Once everything is connected, powered on, and the configuration is all set, the HA dashboard can be accessed using http://homeassistant.local:8123/ or the HA companion app. Commands will be sent via HA, and the statuses of the smart devices connected will also be sent to HA via the RPI hub. This should be done via using the Matter standard.

## Current bugs

- When initially setting up the HA OS, a white screen with the HA logo appeared after using http://homeassistant.local:8123/. This persisted for more than an hour. The likely cause is that the browser caches are causing some type of issue. It could also be the hardware limitations of the RPI3 running HA OS for the first time. The fix would be using the companion app, or using http://IP_ADDRESS:8123, where IP_ADDRESS is the IP address of the RPI in a web browser. However, after accessing the dashboard for the first time using http://homeassistant.local:8123/ no longer causes a problem.
