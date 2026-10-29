# ESP32 Matter endpoint

The active endpoint for the wired PowerMesh prototype is `Wired/` (ESP32-S3). `Wireless/` is the earlier Wi-Fi Matter-light baseline, and `Testing_files/` contains pre-Matter UART experiments.

The project uses ESP-IDF 6.0.2 and ESP-Matter 1.6.0. Install both toolchains in your development environment (WSL is supported), then activate the ESP-IDF and ESP-Matter environments as required by your SDK installation.

## Build the active wired endpoint

```bash
cd $HOME/PowerMesh/ESP32_Testing/Wired
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace the checkout and serial port paths for your machine. See [Wired/README.md](Wired/README.md) for the external CHIP mDNS patch, commissioning, and the Pi/STM32 wiring instructions.

`managed_components/` is downloaded by ESP-IDF's component manager during configuration/build. It is intentionally not committed; `dependencies.lock` and the project manifests remain versioned to preserve the selected dependency versions. A fresh checkout needs access to the Espressif component registry.

To pass a USB serial device into WSL on Windows, use `usbipd` to bind and attach the ESP32 USB device, then identify it with `ls -l /dev/ttyUSB* /dev/ttyACM*`.
