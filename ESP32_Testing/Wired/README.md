# ESP32 Testing (Wired)
This is the active ESP32-S3 Matter light endpoint. The original Pi -> STM32F4 -> STM32G4 -> ESP32 wired prototype is being integrated with the STM32G4 -> ST8500 G3 PLC -> STM32G4 path. The ESP32 still receives and returns complete IPv6 packets through the same framed UART interface. The G3 PLC path has been verified end to end for link-local unicast UDP echo; Matter discovery and commands are not yet verified.

Using ESP IDF 6.0.2 and ESP Matter 1.6.0 in a WSL environment. Please install the necessary dependencies and applications to build and flash the project.

This is the QR code needed for commissioning:

![alt text](image.png)

## Misc
### Current Issues
1. Pressing the reset button or holding the boot button for commissioning seems buggy. It is best to use the serial monitor and repeatedly close/open it for commissioning or for testing BLE.

In addition, use a nRF Connect to check if the BLE is being broadcasted while opening/closing the serial monitor. The ESP32 should be repeatedly broadcasted.

The ESP32 does not change state when the color red is selected on Home Assistant. It is unknown how much colors are effected.

### Matter Command Processing
Home Assistant/Matter Server sends IPv6 packets through the Pi TUN, gateway STM32G4, both ST8500 modems and endpoint STM32G4. The ESP32 deframes the UART data and passes the IPv6 payload into lwIP/Matter. State updates and responses follow the same path in reverse. The proven G3 PLC endpoint address is `fe80::781d:ff:fe00:1`. The CHIP mDNS patch below advertises that link-local address and permits mDNS over the UART `uaN` interface. Unicast UDP has been verified over G3; mDNS multicast and Matter commands still need testing.

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

If the previous mDNS patch was installed, restore the active SDK source from its original backup before applying the updated link-local patch. Preserve the old patched source for rollback. **Do not overwrite a `.powermesh.bak` with already-patched code.**

```bash
# Run after git pull, with SRC set to the active CHIP SDK file as shown above.
if [[ ! -f "$SRC.powermesh.bak" ]]; then
  echo "ERROR: original CHIP source backup missing. Stop and inspect SRC."
else
  cp "$SRC" "$SRC.before_linklocal" &&
  cp "$SRC.powermesh.bak" "$SRC" &&
  mv "$SRC.powermesh.bak" "$SRC.original.before_linklocal" &&
  python3 "$HOME/PowerMesh/ESP32_Testing/Wired/apply_minimal_mdns_tunnel_patch_v2.py" "$SRC" &&
  grep -n -E 'fe80::781d|TunnelAddressIterator|strncmp\(name, "ua"' "$SRC"
fi
```

The grep should find the link-local address and `TunnelAddressIterator`, but **no** `strncmp(name, "ua"` exclusion. Only run this migration when the original backup matches the active SDK source. If no original backup exists, do not apply the script over an unknown patched source. The script creates a new `.powermesh.bak` from the restored CHIP source.

Build and flash normally without erasing NVS or factory-resetting the ESP32 to preserve Matter commissioning.

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
On the ESP32-S3 front silkscreen, GPIO17 is TX and GPIO18 is RX. For the G3 PLC build, connect ESP32 TX GPIO17 to endpoint STM32G4 USART1 RX PA10 and ESP32 RX GPIO18 to STM32G4 USART1 TX PA9. Connect grounds together. The G4 USART3 PC10/PC11 interface connects to the endpoint ST8500, not directly to the ESP32.

### Pi TUN and bridge
On the Pi, run `bash RaspberryPi/install.sh` once from the PowerMesh root. It installs the service that sets up `powermesh0` and runs the fully wired UART bridge at startup. Check the bridge with:
```
sudo systemctl status powermesh
sudo journalctl -u powermesh -f
```

For manual diagnostics and serial device selection, see [RaspberryPi/README.md](../../RaspberryPi/README.md). The older `tun_uart_bridge_hybrid.py` was for a Wi-Fi return-path experiment; it is not used by the G3 implementation.

### Open Home Assistant Dashboard
Get the address of the RPI. This should be known because it is the same address that is used to SSH to the RPI. Enter this into a browser and replace ADDRESS with your RPI address, which could be IPv4 or IPv6.
```
http://ADDRESS:8123
```

If the ESP32 has not been commissioned yet, then commission it.

After the G3 join and IPv6 tests pass, run `RaspberryPi/test_tun_udp_echo.py` and try changing the light state through Home Assistant. The G3 PLC forwarding path must be verified on hardware before treating this as a working end-to-end demo.

The Matter discovery and control traffic can be checked in another Pi terminal:
```
sudo tcpdump -ni powermesh0 -tttt -vv \
  'ip6 and (udp port 5353 or udp port 5540)'
```

Link-local IPv6 requires the correct interface scope (`powermesh0`). If the ESP32 transmits multicast mDNS but nothing arrives on the Pi, verify G3 multicast forwarding before changing Matter commissioning.
