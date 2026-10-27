# ESP32 Testing (Wired)
Used to test the Matter transport to see if a Matter endpoint receives a wired Matter payload correctly. Currently being tested on a ESP32S3.

Using ESP IDF 6.0.2 and ESP Matter 1.6.0 in a WSL environment. Please install the necessary dependencies and applications to build and flash the project.

This is the QR code needed for commissioning:

![alt text](image.png)

## Misc
### Current Issues
1. Pressing the reset button or holding the boot button for commissioning seems buggy. It is best to use the serial monitor and repeatedly close/open it for commissioning or for testing BLE.

In addition, use a nRF Connect to check if the BLE is being broadcasted while opening/closing the serial monitor. The ESP32 should be repeatedly broadcasted.

The ESP32 does not change state when the color red is selected on Home Assistant. It is unknown how much colors are effected.

### Matter Command Processing
The Matter controller (Raspberry Pi) will send the ESP32 customed framed IPv6 packets over a linux network tunnel addressed to the ESP32. The ESP32 will be broadcasting itself via mDNS. When it reaches the ESP32, the packets are deframed and processed. The ESP32 changes states after processing the Matter payload, and sends a callback to the RPI saying that its state has successfully changes.

## ESP32 Build + Flashing
### sdkconfig + CMake Changes
There are no changes currently compared to the wireless version. Make sure that the CMake is using your correct ESP32 model though.

### CHIP patch
Connectedhomeip (CHIP) is the official open source repo for Matter. For the mDNS to work correcly for the ESP32, we will have to patch a file used by the build called `AddressPolicy_DefaultImpl.cpp`. Included in the Wired file is `apply_minimal_mdns_tunnel_patch_v2` which will perform the patch for us.

Find where the file is:
```
cd ~/esp/esp-matter/examples/plc_light_endpoint

SRC="$(realpath build/esp-idf/chip/../../../../../connectedhomeip/connectedhomeip/config/esp32/third_party/connectedhomeip/src/lib/dnssd/minimal_mdns/AddressPolicy_DefaultImpl.cpp)"

echo "$SRC"
ls -l "$SRC"
```

Apply the patch:
```
python3 apply_minimal_mdns_tunnel_patch_v2.py "$SRC"
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
cd ~/esp/esp-matter/examples/plc_light_endpoint
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
cd "$ESP_MATTER_PATH/examples/light"
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

### Setting up TUN
Set up the TUN:
```
sudo bash /home/powermesh/PowerMesh/RaspberryPi/setup_tun.sh
```

Run the hybrid bridge:
```
sudo python3 -u /home/powermesh/PowerMesh/RaspberryPi/tun_uart_bridge_hybrid.py \
  /dev/serial0 --tun powermesh0 \
  --return-interface eth0 \
  --return-bind fe80::1ea3:e633:8c63:30b1 \
  --return-peer fe80::eaf6:aff:fea8:9f58
```

### Open Home Assistant Dashboard
Get the address of the RPI. This should be known because it is the same address that is used to SSH to the RPI. Enter this into a browser and replace ADDRESS with your RPI address, which could be IPv4 or IPv6.
```
http://ADDRESS:8123
```

If the ESP32 has not been commissioned yet, then commission it.

Afterwards, you can now change the state of the ESP32 using Home Assistant, and the transmission (RPI --> ESP32) should be wired. The return (ESP32 --> RPI) should be wireless. To test, simply remove the wiring and the ESP32 should not change states.

The Matter traffic can also be seen by entering this in another SSH terminal of the RPI:
```
sudo tcpdump -ni powermesh0 -tttt -vv \
  'ip6 and udp port 5540'
```