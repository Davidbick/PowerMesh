# ESP32 + RPI Final Testing Before Matter Integration

Passing this test and the previous test should mean that the ESP32 and RPI is correctly configured to attepmt wired Matter integration.

This directory should contain all of the files need for testing after completing uart_testing and is separated between the ESP32 and RPI.

### Running Files on the RPI
To run files on the RPI (Replace {COMMAND} with the corresponding .py file that you want to run on the RPI):

`sudo python3 /home/powermesh/PowerMesh/RaspberryPi/{COMMAND}] /dev/serial0`

All files should run if all of the .c and .h files are on the ESP32. Make sure to use the correct app_main.cpp. The corresponding app_main.cpp should be in the ESP32 directory.

After running the file, it should say something about sending data. This should be true for all RPI files for testing.

### ESP32 Logging
The ESP32 should log information. If there are errors, they will be apparent such as crc errors. If there are no error, it should show something like this:

```
app_main: Received DATA: seq=2 len=61
uart_ipv6_netif: Queued 61-byte IPv6 packet to lwIP
app_main: Queued DATA seq=2 into lwIP
uart_ipv6_udp_test: Received UDP datagram: 13 bytes, source port 49152
uart_ipv6_udp_test: UDP TEST PASSED: received HELLO FROM PI
```

This is just an example and the actual log could be something different, but it should be similar to this.

### Final Test Instructions
This test involves different commands than the other tests because we will now be using the TUN.

Configure the TUN:
```
cd /home/powermesh/PowerMesh
sudo bash RaspberryPi/setup_tun.sh
```

Activate the TUN:
```
sudo python3 -u RaspberryPi/tun_uart_bridge.py /dev/serial0 --tun powermesh0
```

SSH into the RPI again so there are now two terminals

On the other terminal that did not activate the TUN, run:
```
sudo python3 - <<'PY'
import socket

source = "fd00:706d::1"
destination = "fd00:706d::2"
payload = b"HELLO FROM PI"
port = 4242

with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM) as s:
    s.bind((source, 0, 0, 0))
    sent = s.sendto(payload, (destination, port, 0, 0))
    print(f"Sent {sent} UDP payload bytes to [{destination}]:{port}")
    print(f"Local socket: {s.getsockname()}")
PY
```

The terminal that is running the TUN should display something like this:

```
TUN -> UART: seq=1, 61 bytes
```

The ESP32 should log something like this:

```
app_main: Received DATA: seq=1 len=61
uart_ipv6_netif: Queued 61-byte IPv6 packet to lwIP
app_main: Queued DATA seq=1 into lwIP
uart_ipv6_udp_test: Received UDP datagram: 13 bytes, source port ...
uart_ipv6_udp_test: UDP TEST PASSED: received HELLO FROM PI
```

If everything displayed correctly, Matter integration with the custom network interface can proceed.
