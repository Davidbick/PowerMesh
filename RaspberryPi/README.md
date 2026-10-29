# PowerMesh Raspberry Pi gateway

The Pi runs Home Assistant/Matter Server, provides the IPv6 `powermesh0` TUN interface, and exchanges framed IPv6 packets with the STM32F4 over UART. The UART transport uses sync bytes, sequence numbers, CRC32, DATA frames, and link-local ACKs.

## Initial installation

On the Raspberry Pi, as your normal user:

```bash
git clone https://github.com/Davidbick/PowerMesh.git
cd PowerMesh
bash RaspberryPi/install.sh
```

The installer creates `RaspberryPi/.venv`, installs Python dependencies, and installs/enables the `powermesh.service` systemd unit. On every boot the service configures `powermesh0` and starts `tun_uart_bridge.py`.

Check it with:

```bash
sudo systemctl status powermesh
sudo journalctl -u powermesh -f
```

If you have multiple USB serial devices, configure `POWERMESH_SERIAL` in `/etc/default/powermesh` to select the F4 ST-LINK virtual COM device. Otherwise `start_bridge.sh` attempts to find it automatically. To inspect serial devices:

```bash
ls -l /dev/serial/by-id/
ls /dev/ttyACM*
```

## Network check

```bash
ip -6 addr show dev powermesh0
ip -6 route get fd00:706d::2
```

The Pi has `fd00:706d::1/64` on `powermesh0` with MTU 1280. The ESP32 endpoint uses `fd00:706d::2`.

With the complete F4, G4, and ESP32 setup powered, test:

```bash
ping -6 fd00:706d::2
sudo tcpdump -ni powermesh0 'ip6 and udp port 5540'
```

In the service logs, `TUN -> UART` describes outgoing IPv6 packets, `UART ACK` confirms only local frame acceptance, and `UART -> TUN` describes returned IPv6 packets. A returned IPv6 packet or a changed Matter light state confirms end-to-end activity.

## Manual debugging only

Normally you do **not** need to start the bridge manually after boot. If you need to run it interactively, stop the systemd service first so two processes do not open the same UART:

```bash
sudo systemctl stop powermesh
sudo bash RaspberryPi/setup_tun.sh
sudo RaspberryPi/.venv/bin/python -u RaspberryPi/tun_uart_bridge.py /dev/ttyACM0
```

Use the correct serial path for your board. After manual debugging, restart the service with `sudo systemctl start powermesh`.

## Current packet path

```text
Home Assistant / Matter Server
       ↕
IPv6 TUN / Raspberry Pi UART bridge
       ↕
STM32F446RE gateway
       ↕
STM32G474RE bridge
       ↕
ESP32-S3 Matter light
```

The current prototype is wired UART throughout; PLC/ST8500 integration is still pending. Earlier `tun_uart_bridge_hybrid.py` and wireless-return tests are retained only as historical experiments.
