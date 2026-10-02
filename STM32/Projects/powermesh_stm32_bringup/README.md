# PowerMesh STM32 UART Bring-Up

This STM32 project provides the initial PowerMesh UART transport used between the Raspberry Pi gateway and the STM32.

It implements a simple framed UART protocol with payload length, sequence number, and CRC checking.

## UART Configuration

The project uses USART2 with:

```text
Baud:       115200
Data bits:  8
Parity:     None
Stop bits:  1
Flow ctrl:  None
```

On the Nucleo board, USART2 is available through the ST-Link USB virtual COM port.

## PowerMesh Frame

A PowerMesh UART frame contains:

```text
+------+---------+------+----------+--------+---------+-------+
| Sync | Version | Type | Sequence | Length | Payload | CRC32 |
+------+---------+------+----------+--------+---------+-------+
```

Sync bytes:

```text
A5 5A
```

Current frame types:

```text
DATA = 1
ACK  = 2
```

Maximum payload:

```text
1280 bytes
```

The payload is treated as raw data by the UART layer. For the PowerMesh IP transport, a `DATA` payload can contain one raw IPv6 packet.

## Build

From:

```bash
cd STM32/Projects/powermesh_stm32_bringup
```

Configure if needed:

```bash
cmake --preset Debug
```

Build:

```bash
cmake --build build/Debug
```

The resulting firmware image can then be flashed to the STM32.

## Test the UART Link

Connect the Nucleo board to the host computer over USB.

Run the UART test from the PowerMesh repository root:

```bash
python3 \
    STM32/Projects/powermesh_stm32_bringup/Tools/uart_frame_test.py \
    /dev/ttyACM0
```

Expected result:

```text
Sent DATA: sequence=1
Received ACK: sequence=1
PASS
```

This verifies that framed, bidirectional UART communication is working.

The `ACK` confirms receipt of the PowerMesh UART frame. It is not a response to the contents of the payload itself.
