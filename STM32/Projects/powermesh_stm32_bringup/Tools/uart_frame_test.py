import argparse
import struct
import time
import zlib

import serial


SYNC = b"\xA5\x5A"
VERSION = 1
TYPE_DATA = 1
TYPE_ACK = 2
MAX_PAYLOAD = 1280


def build_frame(frame_type, sequence, payload):
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("Payload is too large")

    header = struct.pack(
        ">BBHH",
        VERSION,
        frame_type,
        sequence,
        len(payload),
    )

    crc = zlib.crc32(header + payload) & 0xFFFFFFFF

    return SYNC + header + payload + struct.pack(">I", crc)


def read_exact(port, count):
    data = port.read(count)

    if len(data) != count:
        raise TimeoutError(
            f"Expected {count} bytes, received {len(data)}"
        )

    return data


def find_sync(port):
    previous = None

    while True:
        current = read_exact(port, 1)[0]

        if previous == SYNC[0] and current == SYNC[1]:
            return

        previous = current


def read_frame(port):
    find_sync(port)

    header = read_exact(port, 6)
    version, frame_type, sequence, length = struct.unpack(
        ">BBHH",
        header,
    )

    if length > MAX_PAYLOAD:
        raise ValueError(f"Invalid payload length: {length}")

    payload = read_exact(port, length)
    received_crc = struct.unpack(">I", read_exact(port, 4))[0]
    calculated_crc = zlib.crc32(header + payload) & 0xFFFFFFFF

    if received_crc != calculated_crc:
        raise ValueError("CRC check failed")

    return version, frame_type, sequence, payload


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="For example, COM6")
    args = parser.parse_args()

    sequence = 1
    payload = b"PowerMesh framed UART works"

    with serial.Serial(
        args.port,
        baudrate=115200,
        timeout=2,
    ) as port:
        time.sleep(0.2)
        port.reset_input_buffer()

        packet = build_frame(TYPE_DATA, sequence, payload)

        port.write(packet)
        port.flush()

        print(f"Sent DATA: sequence={sequence}")
        print(f"Payload: {payload.decode()}")

        version, frame_type, reply_sequence, reply = read_frame(port)

        if version != VERSION:
            raise ValueError("Unexpected protocol version")

        if frame_type != TYPE_ACK:
            raise ValueError("Expected an ACK")

        if reply_sequence != sequence:
            raise ValueError("ACK sequence does not match")

        if reply != b"\x00":
            raise ValueError("STM32 returned an error status")

        print(f"Received ACK: sequence={reply_sequence}")
        print("PASS")


if __name__ == "__main__":
    main()