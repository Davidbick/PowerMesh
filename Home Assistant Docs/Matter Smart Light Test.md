# Matter Smart Light Test

This contains information about controlling a commercial Matter-enabled smart light bulb via Home Assistant (HA) which is on Raspberry PI OS.
This is for testing current configuration of HA and familarizing using HA.

### Smart Light used for Testing

We are using the Linkind Wi-Fi Smart Light Bulb A19 RGBTW that supports Matter.

## Commissioning the Smart Light
1) Factory reset the light bulb. Each brand has their own different ways to factory reset their devices. For our current light, we need to power on and off the light bulb several times. Each toggle should be between 0.5 to 1.5 seconds. The light bulb will shift through many colors before it is ready to pair.
2) Download the Home Assistant Companion App. Configure the app however you want so that it works.
3) Turn on Wi-Fi and Bluetooth.
4) On the app, click add a device, and make sure that Matter was selected for the device type.
5) Scan the QR code.
6) It should automatically commission now. To use the smart light, go on the app and change the state of the light. These changes should be physically shone on the light and on the app.