import struct
import zlib


SYNC = b"\xA5\x5A"

VERSION = 1

TYPE_DATA = 1
TYPE_ACK = 2

MAX_PAYLOAD = 1280

HEADER_STRUCT = struct.Struct(">BBHH")
CRC_STRUCT = struct.Struct(">I")


def build_frame(frame_type: int, sequence: int, payload: bytes) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(
            f"Payload is {len(payload)} bytes; max is {MAX_PAYLOAD}"
        )

    header = HEADER_STRUCT.pack(
        VERSION,
        frame_type,
        sequence,
        len(payload),
    )

    crc = zlib.crc32(header + payload) & 0xFFFFFFFF

    return (
        SYNC
        + header
        + payload
        + CRC_STRUCT.pack(crc)
    )


def read_exact(port, count: int) -> bytes:
    data = bytearray()

    while len(data) < count:
        chunk = port.read(count - len(data))

        if not chunk:
            raise TimeoutError(
                f"Expected {count} bytes, received {len(data)}"
            )

        data.extend(chunk)

    return bytes(data)


def find_sync(port) -> None:
    previous = None

    while True:
        current = read_exact(port, 1)[0]

        if previous == SYNC[0] and current == SYNC[1]:
            return

        previous = current


def read_frame(port):
    find_sync(port)

    header = read_exact(port, HEADER_STRUCT.size)

    version, frame_type, sequence, length = HEADER_STRUCT.unpack(header)

    if version != VERSION:
        raise ValueError(
            f"Unsupported protocol version: {version}"
        )

    if length > MAX_PAYLOAD:
        raise ValueError(
            f"Invalid payload length: {length}"
        )

    payload = read_exact(port, length)

    received_crc = CRC_STRUCT.unpack(
        read_exact(port, CRC_STRUCT.size)
    )[0]

    calculated_crc = zlib.crc32(
        header + payload
    ) & 0xFFFFFFFF

    if received_crc != calculated_crc:
        raise ValueError(
            f"CRC mismatch: received=0x{received_crc:08X}, "
            f"calculated=0x{calculated_crc:08X}"
        )

    return frame_type, sequence, payload


def write_frame(
    port,
    frame_type: int,
    sequence: int,
    payload: bytes,
) -> None:
    frame = build_frame(
        frame_type,
        sequence,
        payload,
    )

    port.write(frame)
    port.flush()