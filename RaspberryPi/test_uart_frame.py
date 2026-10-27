#!/usr/bin/env python3

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
        "fd00:706d::1"
    ).packed

    header[24:40] = ipaddress.IPv6Address(
        "fd00:706d::2"
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

    print("IPv6 source: fd00:706d::1")
    print("IPv6 destination: fd00:706d::2")

    with serial.Serial(args.port, 115200, timeout=1) as port:
        port.write(frame)
        port.flush()

    print(f"Sent {len(frame)} bytes")
    print(f"IPv6 packet length: {len(packet)} bytes")


if __name__ == "__main__":
    main()

