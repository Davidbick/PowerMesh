# ESP32 Testing (Wired)
Used to test the Matter transport to see if a Matter endpoint receives a wired payload correctly. Currently being tested on a ESP32S3.

Using ESP IDF 6.0.2 and ESP Matter 1.6.0 in a WSL environment. Please install the necessary dependencies and applications to build and flash the project.

This is the QR code needed for commissioning:

![alt text](image.png)

## Current Issues
1. Pressing the reset button or holding the boot button for commissioning seems buggy. It is best to use the serial monitor and repeatedly close/open it for commissioning or for testing BLE.

To close monitor (hold Ctrl and press the right bracket ]):
```bash
Ctrl + ]
```

To reopen monitor (replace `/dev/ttyUSB0` with your actual port):
```bash
cd "$ESP_MATTER_PATH/examples/light"
idf.py -p /dev/ttyUSB0 flash monitor
```

In addition, use a nRF Connect to check if the BLE is being broadcasted while opening/closing the serial monitor. The ESP32 should be repeatedly broadcasted.

### Matter Command Processing
The Matter controller (Raspberry Pi) will send the ESP32 customed framed IPv6 packets over a linux network tunnel addressed to the ESP32. When it reaches the ESP32, the packets must be deframed and processed. 

### sdkconfig Changes
There are no changes currently compared to the wireless version.

### Wiring
On the front silkscreen, 17 is TX while 18 will be RX. Connect these pins to the corresponding pins of the STM32 (if testing full PLC) or Raspberry Pi (if testing if code works).

### Building and Flashing
Activating the ESP-IDF version
```bash
cd ~/esp/esp-idf
source ./export.sh
```

Setting Matter path:
```bash
cd ~/esp/esp-matter
source ./export.sh
```

Building commands. Make sure to go to the Wired or Wireless folders and read the README to select the correct ESP32 model being used.
```bash
cd ~/esp/esp-matter/examples/plc_light_endpoint
idf.py set-target esp32s3
idf.py build
```

## To-do
### Without Wi-Fi (WIRED)
1. Enable UART functionality (RX/TX)
2. De-frame the custom frame of the IPv6 packets
3. Have Matter take in the de-framed packets from the STM32
4. Check functionality (of sending) by testing with Home Assistant. 

## Current Progress (Wired)
1. Creates the Matter endpoint
2. Has commissioning
3. Has different functionalities such as turning it on/off, changing colors, etc.