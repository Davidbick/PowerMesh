#!/usr/bin/env python3

import ipaddress
import struct
import time
import zlib

import serial


SYNC = b"\xA5\x5A"
VERSION = 1
TYPE_DATA = 1
TYPE_ACK = 2

PORT = "/dev/serial0"
BAUD = 115200
TIMEOUT = 3

HEADER = struct.Struct(">BBHH")


def build_frame(frame_type, seq, payload=b""):
    header = HEADER.pack(
        VERSION,
        frame_type,
        seq,
        len(payload),
    )

    crc = zlib.crc32(header + payload) & 0xFFFFFFFF

    return (
        SYNC
        + header
        + payload
        + struct.pack(">I", crc)
    )


def read_exact(ser, count):
    """Read exactly count bytes, or fail on timeout."""
    data = bytearray()

    while len(data) < count:
        chunk = ser.read(count - len(data))

        if not chunk:
            raise TimeoutError(
                f"Timed out: expected {count} bytes, "
                f"received {len(data)}"
            )

        data.extend(chunk)

    return bytes(data)


def read_frame(ser):
    """Find sync, read one frame, and validate its CRC."""
    sync_window = bytearray()

    while True:
        byte = read_exact(ser, 1)[0]

        sync_window.append(byte)

        if len(sync_window) > 2:
            del sync_window[0]

        if bytes(sync_window) == SYNC:
            break

    # Version, type, sequence, payload length
    header = read_exact(ser, HEADER.size)

    version, frame_type, seq, length = HEADER.unpack(header)

    if version != VERSION:
        raise ValueError(f"Unsupported version: {version}")

    if length > 1280:
        raise ValueError(f"Invalid payload length: {length}")

    payload = read_exact(ser, length)
    received_crc = struct.unpack(">I", read_exact(ser, 4))[0]

    expected_crc = zlib.crc32(header + payload) & 0xFFFFFFFF

    if received_crc != expected_crc:
        raise ValueError(
            f"CRC mismatch: received {received_crc:08X}, "
            f"expected {expected_crc:08X}"
        )

    return frame_type, seq, payload


def make_test_ipv6_packet():
    # Minimal 40-byte IPv6 packet with no upper-layer payload.
    src = ipaddress.IPv6Address("fd42:706f:7765::1")
    dst = ipaddress.IPv6Address("fd42:706f:7765::2")

    ipv6_header = struct.pack(
        ">IHBB",
        0x60000000,  # Version 6, traffic class and flow label
        0,           # Payload length
        59,          # Next header: No Next Header
        64,          # Hop limit
    )

    return ipv6_header + src.packed + dst.packed


def main():
    seq = 1
    payload = make_test_ipv6_packet()
    frame = build_frame(TYPE_DATA, seq, payload)

    print(f"Opening {PORT} at {BAUD} baud...")

    with serial.Serial(
        PORT,
        BAUD,
        timeout=TIMEOUT,
    ) as ser:
        time.sleep(0.2)

        # Discard old input so a stale ACK cannot pass the test.
        ser.reset_input_buffer()

        print(f"Sending DATA: seq={seq}, IPv6 length={len(payload)}")
        ser.write(frame)
        ser.flush()

        try:
            frame_type, ack_seq, ack_payload = read_frame(ser)
        except (TimeoutError, ValueError) as exc:
            print(f"FAILED: {exc}")
            return

        if frame_type != TYPE_ACK:
            print(f"FAILED: expected ACK type 2, got {frame_type}")
            return

        if ack_seq != seq:
            print(f"FAILED: expected sequence {seq}, got {ack_seq}")
            return

        if ack_payload:
            print(f"FAILED: expected empty ACK, got {len(ack_payload)} bytes")
            return

        print(f"SUCCESS: valid ACK received for sequence {ack_seq}")


if __name__ == "__main__":
    main()
