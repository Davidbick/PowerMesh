#!/usr/bin/env python3

import argparse
import ipaddress
import socket
import sys

DESTINATION_IP = "fd00:706d::2"
DESTINATION_PORT = 4242
PAYLOAD = b"HELLO FROM PI"


def main():
    parser = argparse.ArgumentParser(
        description="Test Pi TUN -> UART -> ESP32, Wi-Fi return path"
    )
    parser.add_argument(
        "--source",
        required=True,
        help="Pi Wi-Fi IPv6 address (not fe80::)"
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=5.0,
        help="Seconds to wait for the UDP echo (default: 5)"
    )
    args = parser.parse_args()

    source_ip = ipaddress.IPv6Address(args.source)

    if source_ip.is_link_local:
        parser.error(
            "Use the Pi's non-link-local Wi-Fi IPv6 address, "
            "not an fe80:: address."
        )

    with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM) as sock:
        sock.settimeout(args.timeout)

        # Force the request's IPv6 source address to be the Pi's
        # Wi-Fi address, even though the destination routes over TUN.
        sock.bind((str(source_ip), 0, 0, 0))

        print(f"Local socket: {sock.getsockname()}")
        print(
            f"Sending {PAYLOAD!r} to "
            f"[{DESTINATION_IP}]:{DESTINATION_PORT}"
        )

        try:
            sent = sock.sendto(
                PAYLOAD,
                (DESTINATION_IP, DESTINATION_PORT, 0, 0)
            )
            print(f"Sent {sent} UDP payload bytes")

            data, peer = sock.recvfrom(1024)

        except socket.timeout:
            print(
                "FAILED: no UDP echo received before timeout",
                file=sys.stderr
            )
            sys.exit(1)

        except OSError as exc:
            print(f"FAILED: {exc}", file=sys.stderr)
            sys.exit(1)

        print(f"Received {data!r} from [{peer[0]}]:{peer[1]}")

        if data != PAYLOAD:
            print("FAILED: returned payload does not match", file=sys.stderr)
            sys.exit(1)

        print("SUCCESS: wireless UDP return path verified")


if __name__ == "__main__":
    main()
