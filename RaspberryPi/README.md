# PowerMesh Raspberry Pi Bridge

This directory contains the Raspberry Pi side of the PowerMesh transport.

It bridges Linux IPv6 traffic from a TUN interface to the STM32 gateway using the PowerMesh UART framing protocol.

## Setup

Clone the repository on the Raspberry Pi:

```bash
cd ~
git clone https://github.com/Davidbick/PowerMesh.git
cd PowerMesh
```

Install the required Python support:

```bash
sudo apt update
sudo apt install -y python3-venv
```

Create the virtual environment and install dependencies:

```bash
python3 -m venv RaspberryPi/.venv

RaspberryPi/.venv/bin/pip install \
    -r RaspberryPi/requirements.txt
```

## Connect the STM32

Connect the STM32/Nucleo to the Pi over USB.

Find its serial port:

```bash
ls -l /dev/serial/by-id/
```

or:

```bash
ls /dev/ttyACM*
```

If serial permissions fail:

```bash
sudo usermod -aG dialout "$USER"
```

Then log out and back in or reboot.

## Create the TUN Interface

Run the TUN setup script as root:

```bash
sudo bash RaspberryPi/setup_tun.sh
```

This creates:

```text
Interface:  powermesh0
Pi address: fd00:706d::1/64
MTU:        1280
```

Verify:

```bash
ip -6 addr show dev powermesh0
```

and:

```bash
ip -6 route get fd00:706d::2
```

## Run the Bridge

Start the TUN/UART bridge:

```bash
RaspberryPi/.venv/bin/python \
    RaspberryPi/tun_uart_bridge.py \
    /dev/ttyACM0
```

Leave this running.

## Test

From another terminal on the Pi:

```bash
ping -6 fd00:706d::2
```

With the F4, G4, and ESP32 powered and connected, the ESP32 should reply over the full wired path.

The bridge should show:

```text
TUN -> UART: seq=1, <bytes> bytes
UART ACK: seq=1, status=0
UART -> TUN: seq=1, <bytes> bytes
```

The UART ACK confirms local frame acceptance; the returned IPv6 packet confirms end-to-end communication.

## Normal Use After Reboot

After rebooting the Pi:

```bash
cd ~/PowerMesh

sudo bash RaspberryPi/setup_tun.sh

RaspberryPi/.venv/bin/python \
    RaspberryPi/tun_uart_bridge.py \
    /dev/ttyACM0
```

## Updating

Pull the latest PowerMesh code:

```bash
cd ~/PowerMesh
git pull --ff-only
```

## Current Status

PowerMesh currently supports:

```text
Home Assistant / Matter Server
        ↕
Linux IPv6 / powermesh0 TUN
        ↕
Python TUN-UART bridge
        ↕
STM32F446RE (Pi-side UART forwarding)
        ↕
STM32G474RE (endpoint-side UART forwarding)
        ↕
ESP32-S3 (wired IPv6 / Matter light)
