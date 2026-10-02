#!/usr/bin/env python3

import argparse
import fcntl
import os
import select
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

    parser.add_argument(
        "port",
        help=(
            "STM32 serial device, for example "
            "/dev/ttyACM0 or /dev/serial/by-id/..."
        ),
    )

    parser.add_argument(
        "--tun",
        default="powermesh0",
        help="TUN interface name",
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="UART baud rate",
    )

    args = parser.parse_args()

    tun_fd = open_tun(args.tun)

    port = serial.Serial(
        args.port,
        baudrate=args.baud,
        timeout=1.0,
    )

    port.reset_input_buffer()

    sequence = 1

    print(
        f"PowerMesh bridge started:"
        f"\n  TUN:  {args.tun}"
        f"\n  UART: {args.port}"
        f"\n  Baud: {args.baud}"
    )

    try:
        while True:
            readable, _, _ = select.select(
                [
                    tun_fd,
                    port.fileno(),
                ],
                [],
                [],
            )

            #
            # Linux -> TUN -> UART -> STM32
            #
            if tun_fd in readable:
                packet = os.read(
                    tun_fd,
                    MAX_PAYLOAD,
                )

                if not is_ipv6(packet):
                    print(
                        f"DROP TUN: non-IPv6 packet "
                        f"({len(packet)} bytes)"
                    )
                    continue

                write_frame(
                    port,
                    TYPE_DATA,
                    sequence,
                    packet,
                )

                print(
                    f"TUN -> UART: "
                    f"seq={sequence}, "
                    f"{len(packet)} bytes"
                )

                sequence = (sequence + 1) & 0xFFFF

                if sequence == 0:
                    sequence = 1

            if port.fileno() in readable:
                try:
                    frame_type, rx_sequence, payload = read_frame(
                        port
                    )

                except (TimeoutError, ValueError) as error:
                    print(
                        f"UART frame error: {error}"
                    )
                    continue

                if frame_type == TYPE_ACK:
                    status = (
                        payload[0]
                        if payload
                        else None
                    )

                    print(
                        f"UART ACK: "
                        f"seq={rx_sequence}, "
                        f"status={status}"
                    )

                    continue

                if frame_type != TYPE_DATA:
                    print(
                        f"UART: unsupported frame type "
                        f"{frame_type}"
                    )
                    continue

                if not is_ipv6(payload):
                    print(
                        f"DROP UART: DATA payload is "
                        f"not IPv6 ({len(payload)} bytes)"
                    )
                    continue

                os.write(
                    tun_fd,
                    payload,
                )

                print(
                    f"UART -> TUN: "
                    f"seq={rx_sequence}, "
                    f"{len(payload)} bytes"
                )

    except KeyboardInterrupt:
        print("\nStopping PowerMesh bridge.")

    finally:
        port.close()
        os.close(tun_fd)


if __name__ == "__main__":
    main()