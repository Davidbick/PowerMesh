#!/usr/bin/env python3
"""Send one IPv6 UDP echo request over the PowerMesh gateway UART from a laptop."""

import socket
import struct
import sys
import time
from pathlib import Path

import serial

sys.path.insert(0, str(Path(__file__).parent / "RaspberryPi"))
from uart_frame import TYPE_ACK, TYPE_DATA, read_frame, write_frame

SOURCE = "fd00:706d::781d:ff:fe00:0"
DESTINATION = "fd00:706d::781d:ff:fe00:1"
MESSAGE = b"HELLO THROUGH TUN"
SOURCE_PORT = 54000


def checksum(data):
    if len(data) % 2:
        data += b"\x00"

    total = sum(
        int.from_bytes(data[i:i + 2], "big")
        for i in range(0, len(data), 2)
    )
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def make_packet():
    src = socket.inet_pton(socket.AF_INET6, SOURCE)
    dst = socket.inet_pton(socket.AF_INET6, DESTINATION)
    length = 8 + len(MESSAGE)

    udp = struct.pack("!HHHH", SOURCE_PORT, 4242, length, 0)
    pseudo = src + dst + struct.pack("!I3xB", length, 17)
    udp_checksum = checksum(pseudo + udp + MESSAGE)

    udp = struct.pack(
        "!HHHH", SOURCE_PORT, 4242, length, udp_checksum
    ) + MESSAGE

    ipv6 = struct.pack(
        "!IHBB16s16s", 6 << 28, len(udp), 17, 64, src, dst
    )
    return ipv6 + udp


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} COM18")
        return 2

    port = sys.argv[1]
    packet = make_packet()

    with serial.Serial(port, 115200, timeout=0.5) as link:
        link.reset_input_buffer()
        write_frame(link, TYPE_DATA, 1, packet)
        print(f"Sent {len(packet)}-byte IPv6 UDP packet")
        print("Listening for ACK and UDP echo for 60 seconds...")

        deadline = time.monotonic() + 60

        while time.monotonic() < deadline:
            try:
                frame_type, seq, data = read_frame(link)
            except TimeoutError:
                continue
            except ValueError as error:
                print("Invalid UART frame:", error)
                continue

            if frame_type == TYPE_ACK:
                status = data[0] if data else -1
                print(f"UART ACK seq={seq}, status={status}")
                continue

            if frame_type == TYPE_DATA:
                print(f"Received IPv6 frame: {len(data)} bytes")

                if (
                    len(data) >= 48
                    and data[6] == 17
                    and data[8:24] == packet[24:40]
                    and data[24:40] == packet[8:24]
                    and int.from_bytes(data[42:44], "big") == SOURCE_PORT
                    and data[48:] == MESSAGE
                ):
                    print("SUCCESS: UDP echo returned through PLC!")
                    return 0

        print("TIMEOUT: No matching UDP echo received")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
