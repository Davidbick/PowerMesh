import argparse
import sys
import time
from pathlib import Path

import serial

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "RaspberryPi"))
from uart_frame import TYPE_ACK, TYPE_DATA, read_frame, write_frame


def test_direction(source, destination, sequence, payload, label):
    write_frame(source, TYPE_DATA, sequence, payload)

    frame_type, ack_sequence, ack_payload = read_frame(source)
    if (frame_type, ack_sequence, ack_payload) != (
        TYPE_ACK, sequence, b"\x00"
    ):
        raise AssertionError(f"{label}: invalid ACK")

    frame_type, received_sequence, received_payload = read_frame(destination)
    if (frame_type, received_sequence, received_payload) != (
        TYPE_DATA, sequence, payload
    ):
        raise AssertionError(f"{label}: forwarded DATA mismatch")

    print(f"PASS {label}: seq={sequence}, payload={len(payload)} bytes")


def main():
    parser = argparse.ArgumentParser(
        description="Test both F4 UARTs without the G4"
    )
    parser.add_argument("pi_port", help="F4 ST-LINK virtual COM device")
    parser.add_argument("remote_port", help="3.3V USB-UART on F4 USART1")
    args = parser.parse_args()

    if args.pi_port == args.remote_port:
        parser.error("Specify two different serial ports")

    with serial.Serial(args.pi_port, 115200, timeout=5) as pi, \
         serial.Serial(args.remote_port, 115200, timeout=5) as remote:
        time.sleep(0.2)
        pi.reset_input_buffer()
        remote.reset_input_buffer()

        test_direction(pi, remote, 101, b"Pi to remote", "Pi -> remote")
        test_direction(remote, pi, 102, b"Remote to Pi", "Remote -> Pi")
        test_direction(pi, remote, 103, bytes(range(256)) * 5, "Pi -> remote")
        test_direction(remote, pi, 104, bytes(range(256)) * 5, "Remote -> Pi")

    print("PASS: bidirectional F4 forwarding verified")


if __name__ == "__main__":
    main()
