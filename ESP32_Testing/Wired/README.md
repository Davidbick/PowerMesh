# ESP32 Testing (Wired)
This is the active ESP32-S3 Matter light endpoint for the fully wired Pi -> STM32F4 -> STM32G4 -> ESP32 prototype. It receives IPv6 packets through a custom framed UART interface and returns IPv6 packets over UART.

Using ESP IDF 6.0.2 and ESP Matter 1.6.0 in a WSL environment. Please install the necessary dependencies and applications to build and flash the project.

This is the QR code needed for commissioning:

![alt text](image.png)

## Misc
### Current Issues
1. Pressing the reset button or holding the boot button for commissioning seems buggy. It is best to use the serial monitor and repeatedly close/open it for commissioning or for testing BLE.

In addition, use a nRF Connect to check if the BLE is being broadcasted while opening/closing the serial monitor. The ESP32 should be repeatedly broadcasted.

The ESP32 does not change state when the color red is selected on Home Assistant. It is unknown how much colors are effected.

### Matter Command Processing
Home Assistant/Matter Server sends IPv6 packets through the Pi TUN and UART bridge, then through the F4 and G4. The ESP32 deframes them and passes the IPv6 payload into lwIP/Matter. State updates and responses follow the wired UART path in the opposite direction. Matter discovery uses a temporary CHIP mDNS address-policy patch described below.

## ESP32 Build + Flashing
### CMake Changes
This project's `CMakeLists.txt`, `sdkconfig`, and `dependencies.lock` are committed. The downloaded `managed_components/` directory is intentionally ignored; ESP-IDF restores components during the build. Use the ESP32-S3 target for the tested setup.

### CHIP patch
Connectedhomeip (CHIP) is the Matter implementation. The current test setup needs an address-policy change in the **external ESP-Matter/CHIP SDK source file** `AddressPolicy_DefaultImpl.cpp`. The versioned `apply_minimal_mdns_tunnel_patch_v2.py` script applies it. Do not confuse this external SDK patch with files in `managed_components/`. It is specific to the working prototype and should not be applied to unrelated projects.

The following locates the SDK source in the original copied-example workspace. If building directly from this repository, use its own build directory and adjust the path to the active SDK checkout:
```
cd ~/esp/esp-matter/examples/plc_light_endpoint

SRC="$(realpath build/esp-idf/chip/../../../../../connectedhomeip/connectedhomeip/config/esp32/third_party/connectedhomeip/src/lib/dnssd/minimal_mdns/AddressPolicy_DefaultImpl.cpp)"

echo "$SRC"
ls -l "$SRC"
```

Apply the patch:
```
python3 $HOME/PowerMesh/ESP32_Testing/Wired/apply_minimal_mdns_tunnel_patch_v2.py "$SRC"
```


### Building
Activating the ESP-IDF version:
```
cd ~/esp/esp-idf
source ./export.sh
```

Setting Matter path:
```
cd ~/esp/esp-matter
source ./export.sh
```

Setting ESP32 target and building:
```
cd $HOME/PowerMesh/ESP32_Testing/Wired
idf.py set-target esp32s3
idf.py build
```

### Flashing
To flash, install `usbipd-win` which is used to pass USB devices to WSL. After installation is complete, open PowerShell as an administrator and run `usbipd list`. Look for the ESP32 or USB Serial Device and notice its `BUSID`.

Enter this command while replacing `YOUR-BUSID` with your corresponding ESP32 `BUSID`. It may say that it is already shared, which is fine.
```
usbipd bind --busid YOUR-BUSID
```

In a **regular** PowerShell window, enter this command while replacing `YOUR-BUSID` with your corresponding ESP32 `BUSID`.
```
usbipd attach --wsl --busid YOUR-BUSID
```

Any PowerShell windows are not needed anymore. Go back to WSL and enter `ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null`. This should display the ESP32 port.

Enter this command while replacing `/dev/ttyUSB0` with your actual port.
```
cd $HOME/PowerMesh/ESP32_Testing/Wired
idf.py -p /dev/ttyUSB0 flash monitor
```

This should successfully flash the ESP32 and open the monitor.

## Raspberry Pi Configuration
The RPI should be connected to a router via ethernet. It could also be connected to a laptop if the laptop can act similarly to a router for the RPI. After flashing the ESP32, follow the wiring guide below to successfully wire UART between the devices.

If tcpdump is not installed yet, add it. It is useful for debugging.
```
sudo apt update && sudo apt install tcpdump -y
```

### Wiring
On the front silkscreen, 17 is TX while 18 will be RX. Connect these pins to the corresponding pins of the STM32 (if testing full PLC) or Raspberry Pi (if testing wired Matter only). G is ground for the ESP32. Make sure to connect ground for the devices that are communicating to each other to a shared ground.

### Pi TUN and bridge
On the Pi, run `bash RaspberryPi/install.sh` once from the PowerMesh root. It installs the service that sets up `powermesh0` and runs the fully wired UART bridge at startup. Check the bridge with:
```
sudo systemctl status powermesh
sudo journalctl -u powermesh -f
```

For manual diagnostics and serial device selection, see [RaspberryPi/README.md](../../RaspberryPi/README.md). The older `tun_uart_bridge_hybrid.py` was for a Wi-Fi return-path experiment; it is not used by the working all-wired demo.

### Open Home Assistant Dashboard
Get the address of the RPI. This should be known because it is the same address that is used to SSH to the RPI. Enter this into a browser and replace ADDRESS with your RPI address, which could be IPv4 or IPv6.
```
http://ADDRESS:8123
```

If the ESP32 has not been commissioned yet, then commission it.

Afterwards, you can now change the state of the ESP32 using Home Assistant, and the transmission (RPI --> ESP32) and (ESP32 --> RPI) should be wired. To test, simply remove the wiring and the ESP32 should not change states.

The Matter traffic can also be seen by entering this in another SSH terminal of the RPI:
```
sudo tcpdump -ni powermesh0 -tttt -vv \
  'ip6 and udp port 5540'
```
