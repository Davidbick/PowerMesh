#!/usr/bin/env python3

import argparse
import fcntl
import os
import secrets
import select
import time
import struct

import serial

from uart_frame import (
    MAX_PAYLOAD,
    TYPE_ACK,
    TYPE_DATA,
    read_frame,
    write_frame,
)


# Linux TUN definitions
TUNSETIFF = 0x400454CA
IFF_TUN = 0x0001
IFF_NO_PI = 0x1000


def open_tun(name: str) -> int:
    fd = os.open(
        "/dev/net/tun",
        os.O_RDWR,
    )

    ifreq = struct.pack(
        "16sH",
        name.encode("ascii"),
        IFF_TUN | IFF_NO_PI,
    )

    fcntl.ioctl(
        fd,
        TUNSETIFF,
        ifreq,
    )

    return fd


def is_ipv6(packet: bytes) -> bool:
    if not packet:
        return False

    return (packet[0] >> 4) == 6


def main():
    parser = argparse.ArgumentParser(
        description="PowerMesh TUN <-> UART bridge"
    )
    parser.add_argument("port", help="STM32 serial device")
    parser.add_argument("--tun", default="powermesh0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    tun_fd = open_tun(args.tun)
    port = serial.Serial(args.port, args.baud, timeout=1.0)
    port.reset_input_buffer()

    # One outstanding packet until ST8500 confirms transmission.
    sequence = secrets.randbelow(65535) + 1
    pending = None
    queued = False
    next_action = 0.0

    print(f"PowerMesh bridge: {args.tun} <-> {args.port}")
    print("G3 flow control enabled")

    try:
        while True:
            # Don't drain Linux TUN while the PLC link is busy.
            inputs = [port.fileno()]
            if pending is None:
                inputs.append(tun_fd)

            readable, _, _ = select.select(
                inputs, [], [], 0.1
            )

            if not os.path.exists(args.port):
                raise serial.SerialException(
                    f"STM32 disconnected: {args.port}"
                )

            # STM32 -> Pi. Process ACKs and incoming IPv6.
            if port.fileno() in readable:
                try:
                    frame_type, rx_sequence, payload = read_frame(
                        port
                    )
                except (TimeoutError, ValueError) as error:
                    print(f"UART frame error: {error}")
                    continue

                if frame_type == TYPE_ACK:
                    if pending is None or rx_sequence != sequence:
                        continue

                    status = payload[0] if len(payload) == 1 else None

                    if status == 0:
                        # Accepted by STM32, not yet sent over PLC.
                        queued = True
                        next_action = time.monotonic() + 45.0
                        print(f"STM32 queued: seq={sequence}")

                    elif status == 1:
                        # STM32 queue full. Retain the same packet.
                        queued = False
                        next_action = time.monotonic() + 0.5
                        print(f"Queue full: retrying seq={sequence}")

                    elif status == 2:
                        # ST8500 confirmed successful G3 transmission.
                        print(f"G3 TX confirmed: seq={sequence}")
                        pending = None
                        queued = False
                        sequence = (sequence % 65535) + 1

                    else:
                        print(f"Unexpected ACK status: {status}")

                elif frame_type == TYPE_DATA:
                    if is_ipv6(payload):
                        os.write(tun_fd, payload)
                        print(
                            f"UART -> TUN: seq={rx_sequence}, "
                            f"{len(payload)} bytes"
                        )
                    else:
                        print("DROP UART: invalid IPv6 packet")

            # Pi -> STM32. Read only when no packet is outstanding.
            if pending is None and tun_fd in readable:
                packet = os.read(tun_fd, MAX_PAYLOAD)

                if is_ipv6(packet):
                    pending = packet
                    queued = False
                    next_action = time.monotonic()
                else:
                    print("DROP TUN: non-IPv6 packet")

            # Send, retry after queue-full, or probe missing ACKs.
            if pending is not None:
                now = time.monotonic()

                if now >= next_action:
                    if queued:
                        print(
                            f"Waiting for G3 confirmation, "
                            f"probing seq={sequence}"
                        )

                    write_frame(
                        port, TYPE_DATA, sequence, pending
                    )

                    print(
                        f"TUN -> UART: seq={sequence}, "
                        f"{len(pending)} bytes"
                    )

                    # A missing queue ACK is retried quickly.
                    # An accepted packet is only re-probed after 45s.
                    next_action = now + (45.0 if queued else 2.0)

    except KeyboardInterrupt:
        print("\nStopping PowerMesh bridge.")

    finally:
        port.close()
        os.close(tun_fd)

if __name__ == "__main__":
    main()
