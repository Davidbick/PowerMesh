# ESP32 and Framing Testing
Instructions on testing the ESP32 uart code and the custom framing protocol without using the TUN.

Testing proved that the RPI can send data to the ESP32. This was verified was Putty. However, the ESP32 cannot send information to the RPI through UART. Further testing would be needed to see if the ESP32 is actually sending anything through its TX (probably by connecting its TX with RX in a loop). Once it is checked that the ESP32 TX is working properly, the RX of the RPI can be checked. In addition, something else could be using the uart of the RPI for some reason.

### Steps
Wire the TX, RX, and GND of the ESP32 and the RPI.

ESP32 TX (17) --> RPI3 RX

ESP32 RX (18) --> RPI3 TX

Wiring the GND is important so that the data is not corrupted while travelling serially.
ESP32 GND --> RPI3 GND (any GND for both devices can be connected)

Open Putty using the corresponding COM port that the ESP32 is using and set the baud rate to 115200. It should have the defaults of the other bits such as parity.

The file we will be using is `test_uart_frame.py`, which should be located alongside the other uart files for the RPI. `test_uart_ack` could also be used, but its main focus was to see if after the ESP32 received the data, it would send it back to the RPI. This did not happen.

run `python3 /home/powermesh/PowerMesh/RaspberryPi/test_uart_frame.py /dev/serial0`
Sudo might be needed if an error occurs.

Putty should now be receiving information from the ESP32 that the RPI has sent through its TX pin to the ESP32's RX pin. The ESP32 should now log information about receiving the data. If an error occurred about the data, there would be at least a crc error.

The corresponding files that are associated with this uart test are: `uart_frame.c`, `uart_frame.h`, and modifications in `app_main`. Archived versions can be found in ESP32_Testing/Misc/uart_testing/