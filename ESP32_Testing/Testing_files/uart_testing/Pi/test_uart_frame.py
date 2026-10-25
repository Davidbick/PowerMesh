#!/usr/bin/env python3

# Used to test the UART functionality of the ESP32 S3 before testing
# it with the TUN. Also tests the our custom framing protocol.

import argparse
import ipaddress
import serial

from uart_frame import TYPE_DATA, build_frame


def make_test_ipv6_packet():
    # Minimal IPv6 header, with no payload.
    header = bytearray(40)

    header[0] = 0x60       # IPv6 version 6
    header[4:6] = b"\x00\x00"  # Payload length = 0
    header[6] = 59          # No Next Header
    header[7] = 64          # Hop limit

    header[8:24] = ipaddress.IPv6Address(
        "fd42:706f:7765::1"
    ).packed

    header[24:40] = ipaddress.IPv6Address(
        "fd42:706f:7765::2"
    ).packed

    return bytes(header)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="UART device, e.g. /dev/ttyUSB0")
    args = parser.parse_args()

    packet = make_test_ipv6_packet()

    frame = build_frame(
        TYPE_DATA,
        1,
        packet,
    )

    with serial.Serial(args.port, 115200, timeout=1) as port:
        port.write(frame)
        port.flush()

    print(f"Sent {len(frame)} bytes")
    print(f"IPv6 packet length: {len(packet)} bytes")


if __name__ == "__main__":
    main()
