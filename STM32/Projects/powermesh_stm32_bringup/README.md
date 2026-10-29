# PowerMesh STM32 UART Bring-Up

The STM32F446RE forwards framed IPv6 packets between the Raspberry Pi and the downstream STM32G474RE without interpreting Matter commands.

Both UART connections use framed packets with a payload length, sequence number, and CRC validation.

## UART Configuration

USART2 (Pi connection) and USART1 (G4 connection) both use:

```text
Baud:       115200
Data bits:  8
Parity:     None
Stop bits:  1
Flow ctrl:  None
```

USART2 uses PA2/PA3 through the ST-LINK USB virtual COM port. USART1 uses PA9 (TX) and PA10 (RX) for the G4 link.

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

## Forwarding

Both UARTs use DMA reception and queued transmission. Valid DATA frames are forwarded in either direction. ACK frames indicate local queue acceptance, not delivery to the final ESP32 endpoint.

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

This test checks Pi-to-F4 framing and the local ACK. To verify downstream forwarding, test IPv6 connectivity with the G4 and ESP32 connected.
