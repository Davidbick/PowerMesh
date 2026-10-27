#!/usr/bin/env python3
"""Send a valid IPv6/UDP test datagram through the existing UART frame protocol."""

import argparse
import ipaddress
import struct
import time

import serial
from uart_frame import TYPE_DATA, build_frame

SOURCE_IP = ipaddress.IPv6Address("fd00:706d::1")
DEST_IP = ipaddress.IPv6Address("fd00:706d::2")
SOURCE_PORT = 49152
DEST_PORT = 4242
PAYLOAD = b"HELLO FROM PI"


def internet_checksum(data: bytes) -> int:
    """Return the standard one's-complement Internet checksum."""
    if len(data) & 1:
        data += b"\x00"
    total = sum(struct.unpack(f"!{len(data) // 2}H", data))
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def make_ipv6_udp_packet() -> bytes:
    udp_length = 8 + len(PAYLOAD)
    udp_without_checksum = struct.pack(
        "!HHHH", SOURCE_PORT, DEST_PORT, udp_length, 0
    ) + PAYLOAD

    # IPv6 UDP checksum includes the IPv6 pseudo-header.
    pseudo_header = (
        SOURCE_IP.packed
        + DEST_IP.packed
        + struct.pack("!I3xB", udp_length, 17)  # UDP next-header = 17
    )
    udp_checksum = internet_checksum(pseudo_header + udp_without_checksum)
    if udp_checksum == 0:
        udp_checksum = 0xFFFF

    udp_datagram = struct.pack(
        "!HHHH", SOURCE_PORT, DEST_PORT, udp_length, udp_checksum
    ) + PAYLOAD

    ipv6_header = bytearray(40)
    ipv6_header[0] = 0x60
    ipv6_header[4:6] = struct.pack("!H", len(udp_datagram))
    ipv6_header[6] = 17  # UDP
    ipv6_header[7] = 64  # hop limit
    ipv6_header[8:24] = SOURCE_IP.packed
    ipv6_header[24:40] = DEST_IP.packed

    return bytes(ipv6_header) + udp_datagram


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="UART device, e.g. /dev/serial0")
    parser.add_argument("--sequence", type=int, default=2)
    args = parser.parse_args()

    packet = make_ipv6_udp_packet()
    frame = build_frame(TYPE_DATA, args.sequence, packet)

    print(f"UART port: {args.port} at 115200 baud")
    print(f"IPv6 source:      {SOURCE_IP}")
    print(f"IPv6 destination: {DEST_IP}")
    print(f"UDP destination:  port {DEST_PORT}")
    print(f"UDP payload:      {PAYLOAD.decode()!r}")
    print(f"IPv6 packet size: {len(packet)} bytes; UART frame size: {len(frame)} bytes")

    with serial.Serial(args.port, 115200, timeout=1) as port:
        written = port.write(frame)
        port.flush()
        if written != len(frame):
            raise RuntimeError(f"Short UART write: wrote {written} of {len(frame)} bytes")

    print("Sent UDP test frame. Check the ESP32 monitor for UDP TEST PASSED.")
    time.sleep(0.05)


if __name__ == "__main__":
    main()
