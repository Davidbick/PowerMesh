# Testing Environment

Clone ESP-IDF 6.0.2 and ESP Matter 1.6.0 in a WSL environment. Please install the necessary dependencies and applications to build and flash the project.

Directory Structure:

~/esp

~/esp/esp-idf

~/esp/esp-matter

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

Building commands (change esp32s3 to whatever ESP32 you are using). Make sure to go to the Wired or Wireless folders and read the README to select the correct ESP32 model being used.
```bash
cd ~/esp/esp-matter/examples/light
idf.py set-target esp32s3
idf.py build
```

To flash, install `usbipd-win` which is used to pass USB devices to WSL. After installation is complete, open PowerShell as an administrator and run `usbipd list`. Look for the ESP32 or USB Serial Device and notice its `BUSID`.

Enter this command while replacing `YOUR-BUSID` with your corresponding ESP32 `BUSID`. It may say that it is already shared, which is fine.
```bash
usbipd bind --busid YOUR-BUSID
```

In a **regular** PowerShell window, enter this command while replacing `YOUR-BUSID` with your corresponding ESP32 `BUSID`.
```bash
usbipd attach --wsl --busid YOUR-BUSID
```

Any PowerShell windows are not needed anymore. Go back to WSL and enter `ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null`. This should display the ESP32 port.

Enter this command while replacing `/dev/ttyUSB0` with your actual port.
```bash
cd "$ESP_MATTER_PATH/examples/light"
idf.py -p /dev/ttyUSB0 flash monitor
```

The ESP32 should now be flashed and the serial monitor should appear. If the ESP32 has not yet been commissioned, go to the Wired or Wireless and scan the QR code using the Home Assistant Companion App. Both Wireless and Wired contain troubleshooting methods for commissioning.

If the ESP32 has already been commissioned by you, it may not be necessary to recommission it. To check, look at the monitor and check if the last printed messages mentions about the ESP32 being broadcasted. Another way to check would be to open Home Assistant dashboard.