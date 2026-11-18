"""Test bidirectional G3 IPv6 forwarding without the ESP32."""

import argparse
import struct
import sys
import time
from pathlib import Path

import serial

sys.path.insert(0, str(Path(__file__).parent / "RaspberryPi"))
from uart_frame import TYPE_ACK, TYPE_DATA, read_frame, write_frame

PAN_ID = 0x781D
MARKER = b"PMESH_G3_IPV6_01"


def make_address(short_addr):
    address = bytearray(16)
    address[0:2] = b"\xfe\x80"
    address[8:10] = PAN_ID.to_bytes(2, "big")
    address[11:13] = b"\xff\xfe"
    address[14:16] = short_addr.to_bytes(2, "big")
    return bytes(address)


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


def make_packet(source, destination, reply=False):
    src = make_address(source)
    dst = make_address(destination)

    # Match the existing STM32 diagnostic exactly.
    icmp_type = 129 if reply else 128
    icmp = struct.pack(
        "!BBHHH", icmp_type, 0, 0, 0x504D, 1
    ) + MARKER

    pseudo = src + dst + struct.pack("!I3xB", len(icmp), 58)
    value = checksum(pseudo + icmp)

    icmp = icmp[:2] + struct.pack("!H", value) + icmp[4:]

    ipv6 = struct.pack(
        "!IHBB16s16s",
        6 << 28, len(icmp), 58, 64, src, dst
    )

    return ipv6 + icmp


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="Gateway COM port")
    parser.add_argument(
        "--endpoint", type=lambda s: int(s, 0), default=1,
        help="Joined endpoint G3 short address, default 0x0001"
    )
    parser.add_argument("--timeout", type=float, default=60)
    args = parser.parse_args()

    if not 0 <= args.endpoint <= 0xFFFF:
        parser.error("Endpoint short address must be 0x0000 to 0xFFFF")

    request = make_packet(0, args.endpoint)
    expected_reply = make_packet(args.endpoint, 0, reply=True)

    print("Gateway short address: 0x0000")
    print(f"Endpoint short address: 0x{args.endpoint:04X}")
    print(f"Sending {len(request)}-byte diagnostic IPv6 packet")

    with serial.Serial(args.port, 115200, timeout=0.5) as link:
        link.reset_input_buffer()
        write_frame(link, TYPE_DATA, 1, request)

        print(f"Listening for {args.timeout:g} seconds...")

        deadline = time.monotonic() + args.timeout

        while time.monotonic() < deadline:
            try:
                frame_type, seq, data = read_frame(link)
            except TimeoutError:
                continue
            except ValueError as error:
                print("UART frame error:", error)
                continue

            if frame_type == TYPE_ACK:
                status = data[0] if data else -1
                print(f"Gateway ACK seq={seq}, status={status}")

                if status == 1:
                    print("Gateway forwarding queue is full")
                continue

            if frame_type == TYPE_DATA:
                print(f"Received IPv6 frame: {len(data)} bytes")

                if data == expected_reply:
                    print("SUCCESS: Exact diagnostic echo received!")
                    print("Bidirectional GatewayPLC forwarding PASS")
                    return 0

                print("Packet did not match expected diagnostic echo")
                print("Received:", data.hex(" "))

        print("TIMEOUT: No matching diagnostic echo received")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
