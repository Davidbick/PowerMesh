#!/usr/bin/env python3
"""PowerMesh hybrid IPv6 bridge: Pi TUN -> ESP32 UART, ESP32 -> Pi via Wi-Fi UDP.

The ESP32 sends outgoing raw IPv6 packets as one UDP datagram with this envelope:
  b"PMU1" + sequence (uint16 big-endian) + packet length (uint16 big-endian) + IPv6 packet

The script validates each envelope and inner IPv6 header before writing the raw
IPv6 packet into the TUN interface. Run as root. Stop the older
 tun_uart_bridge.py first; only one process may own the UART and TUN endpoints.
"""

from __future__ import annotations

import argparse
import fcntl
import ipaddress
import os
import select
import socket
import struct
import sys

import serial

from uart_frame import MAX_PAYLOAD, TYPE_ACK, TYPE_DATA, read_frame, write_frame

TUNSETIFF = 0x400454CA
IFF_TUN = 0x0001
IFF_NO_PI = 0x1000
IPV6_HEADER_LENGTH = 40
WIFI_TUNNEL_PORT = 4243
WIFI_TUNNEL_HEADER_LENGTH = 8
WIFI_TUNNEL_MAGIC = b"PMU1"
ALLOWED_INNER_SOURCES = {
    ipaddress.IPv6Address("fd00:706d::2"),
    ipaddress.IPv6Address("fe80::2"),
}


def open_tun(name: str) -> int:
    fd = os.open("/dev/net/tun", os.O_RDWR)
    ifreq = struct.pack("16sH", name.encode("ascii"), IFF_TUN | IFF_NO_PI)
    try:
        fcntl.ioctl(fd, TUNSETIFF, ifreq)
    except Exception:
        os.close(fd)
        raise
    return fd


def is_ipv6(packet: bytes) -> bool:
    return len(packet) >= IPV6_HEADER_LENGTH and (packet[0] >> 4) == 6


def valid_ipv6_packet(packet: bytes) -> bool:
    if not is_ipv6(packet):
        return False
    declared_length = IPV6_HEADER_LENGTH + int.from_bytes(packet[4:6], "big")
    return declared_length == len(packet) and len(packet) <= MAX_PAYLOAD


def create_return_socket(interface: str, bind_address: str, port: int) -> socket.socket:
    interface_index = socket.if_nametoindex(interface)
    sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)

    # This script runs as root. Bind return datagrams only to the Pi's LAN NIC.
    if not hasattr(socket, "SO_BINDTODEVICE"):
        sock.close()
        raise RuntimeError("This Linux Python build does not expose SO_BINDTODEVICE")
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, interface.encode() + b"\0")

    bind_ip = ipaddress.IPv6Address(bind_address)
    if not bind_ip.is_link_local:
        sock.close()
        raise ValueError("--return-bind must be the Pi LAN interface's link-local IPv6 address")

    sock.bind((str(bind_ip), port, 0, interface_index))
    return sock


def parse_tunnel_datagram(data: bytes) -> tuple[int, bytes] | None:
    if len(data) < WIFI_TUNNEL_HEADER_LENGTH:
        return None
    if data[:4] != WIFI_TUNNEL_MAGIC:
        return None

    sequence = struct.unpack("!H", data[4:6])[0]
    packet_length = struct.unpack("!H", data[6:8])[0]
    if packet_length < IPV6_HEADER_LENGTH or packet_length > MAX_PAYLOAD:
        return None
    if len(data) != WIFI_TUNNEL_HEADER_LENGTH + packet_length:
        return None

    packet = data[WIFI_TUNNEL_HEADER_LENGTH:]
    if not valid_ipv6_packet(packet):
        return None

    source = ipaddress.IPv6Address(packet[8:24])
    if source not in ALLOWED_INNER_SOURCES:
        print(f"DROP Wi-Fi UDP: unexpected inner source {source}")
        return None

    return sequence, packet


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="Pi UART device, e.g. /dev/serial0")
    parser.add_argument("--tun", default="powermesh0", help="TUN interface name")
    parser.add_argument("--baud", type=int, default=115200, help="UART baud rate")
    parser.add_argument("--return-interface", default="eth0", help="Pi LAN interface receiving ESP UDP packets")
    parser.add_argument(
        "--return-bind",
        required=True,
        help="Pi LAN link-local IPv6 address, e.g. fe80::1ea3:e633:8c63:30b1",
    )
    parser.add_argument(
        "--return-peer",
        required=True,
        help="ESP32 Wi-Fi link-local IPv6 address, e.g. fe80::eaf6:aff:fea8:9f58",
    )
    parser.add_argument("--return-port", type=int, default=WIFI_TUNNEL_PORT, help="UDP return-tunnel port")
    args = parser.parse_args()

    try:
        expected_peer = ipaddress.IPv6Address(args.return_peer)
        tun_fd = open_tun(args.tun)
        port = serial.Serial(args.port, baudrate=args.baud, timeout=1.0)
        return_sock = create_return_socket(args.return_interface, args.return_bind, args.return_port)
    except (OSError, ValueError, RuntimeError, serial.SerialException) as exc:
        print(f"ERROR: startup failed: {exc}", file=sys.stderr)
        try:
            os.close(tun_fd)
        except (UnboundLocalError, OSError):
            pass
        return 2

    port.reset_input_buffer()
    sequence = 1

    print("PowerMesh hybrid bridge started:")
    print(f"  TUN:          {args.tun}")
    print(f"  UART:         {args.port} at {args.baud} baud")
    print(f"  Wi-Fi return: [{args.return_bind}%{args.return_interface}]:{args.return_port}")
    print(f"  Expected ESP: {expected_peer}")
    print("  Inner packets are restricted to sources fd00:706d::2 and fe80::2.")

    try:
        while True:
            if not os.path.exists(args.port):
                raise serial.SerialException(f"serial device disappeared: {args.port}")

            readable, _, _ = select.select(
                [tun_fd, port.fileno(), return_sock], [], [], 1.0
            )

            # Pi host/controller -> TUN -> UART -> ESP32.
            if tun_fd in readable:
                packet = os.read(tun_fd, MAX_PAYLOAD)
                if not valid_ipv6_packet(packet):
                    print(f"DROP TUN: invalid/non-IPv6 packet ({len(packet)} bytes)")
                    continue

                write_frame(port, TYPE_DATA, sequence, packet)
                print(f"TUN -> UART: seq={sequence}, {len(packet)} IPv6 bytes")
                sequence = (sequence + 1) & 0xFFFF or 1

            # Optional future ESP32 UART -> TUN receive path; not required for Wi-Fi return.
            if port.fileno() in readable:
                try:
                    frame_type, rx_sequence, payload = read_frame(port)
                except (TimeoutError, ValueError) as error:
                    print(f"UART frame error: {error}")
                    continue

                if frame_type == TYPE_ACK:
                    status = payload[0] if payload else None
                    print(f"UART ACK: seq={rx_sequence}, status={status}")
                    continue

                if frame_type != TYPE_DATA or not valid_ipv6_packet(payload):
                    print(f"DROP UART: invalid/unsupported frame type={frame_type}, length={len(payload)}")
                    continue

                os.write(tun_fd, payload)
                print(f"UART -> TUN: seq={rx_sequence}, {len(payload)} IPv6 bytes")

            # ESP32 -> Wi-Fi UDP envelope -> raw IPv6 -> TUN.
            if return_sock in readable:
                datagram, peer = return_sock.recvfrom(WIFI_TUNNEL_HEADER_LENGTH + MAX_PAYLOAD + 1)
                try:
                    peer_ip = ipaddress.IPv6Address(peer[0].split("%", 1)[0])
                except ipaddress.AddressValueError:
                    print(f"DROP Wi-Fi UDP: invalid peer address {peer[0]!r}")
                    continue

                if peer_ip != expected_peer:
                    print(f"DROP Wi-Fi UDP: packet from unexpected peer {peer_ip}")
                    continue

                parsed = parse_tunnel_datagram(datagram)
                if parsed is None:
                    print(f"DROP Wi-Fi UDP: invalid PMU1 envelope ({len(datagram)} bytes)")
                    continue

                rx_sequence, packet = parsed
                written = os.write(tun_fd, packet)
                if written != len(packet):
                    print(f"ERROR Wi-Fi UDP -> TUN: short write {written}/{len(packet)}")
                    continue
                print(f"Wi-Fi UDP -> TUN: seq={rx_sequence}, {len(packet)} inner IPv6 bytes")

    except KeyboardInterrupt:
        print("\nStopping PowerMesh hybrid bridge.")
    finally:
        return_sock.close()
        port.close()
        os.close(tun_fd)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
