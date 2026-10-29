#!/usr/bin/env python3
# Exercise the wired Pi TUN -> STM32 chain -> ESP32 lwIP -> UART return path.

import argparse
import ipaddress
import socket
import sys

SOURCE_IPV6 = "fd00:706d::1"
DESTINATION_IPV6 = "fd00:706d::2"
DESTINATION_PORT = 4242
PAYLOAD = b"HELLO THROUGH TUN"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default=SOURCE_IPV6, help=f"Pi TUN IPv6 source (default {SOURCE_IPV6})")
    parser.add_argument("--destination", default=DESTINATION_IPV6, help=f"ESP32 tunnel IPv6 address (default {DESTINATION_IPV6})")
    parser.add_argument("--port", type=int, default=DESTINATION_PORT, help="ESP32 UDP test port")
    parser.add_argument("--timeout", type=float, default=8.0, help="Seconds to wait for echo")
    args = parser.parse_args()

    source = ipaddress.IPv6Address(args.source)
    destination = ipaddress.IPv6Address(args.destination)
    if source != ipaddress.IPv6Address(SOURCE_IPV6):
        print("NOTE: using a non-default source address; ensure it is assigned locally.")

    try:
        with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP) as sock:
            sock.settimeout(args.timeout)
            sock.bind((str(source), 0, 0, 0))
            local = sock.getsockname()
            print(f"Local UDP socket: [{local[0]}]:{local[1]}")
            print(f"Sending {PAYLOAD!r} to [{destination}]:{args.port}")
            sent = sock.sendto(PAYLOAD, (str(destination), args.port, 0, 0))
            print(f"Sent {sent} UDP payload bytes")
            response, peer = sock.recvfrom(2048)
    except socket.timeout:
        print("FAIL: timed out waiting for the tunneled UDP echo", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1

    print(f"Received {response!r} from [{peer[0]}]:{peer[1]}")
    if response != PAYLOAD:
        print("FAIL: payload mismatch", file=sys.stderr)
        return 1
    print("SUCCESS: UDP reply returned over wired UART and the Pi TUN")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
