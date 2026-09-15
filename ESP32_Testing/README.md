# ESP32 Testing

Used to test the Matter transport to see if a Matter endpoint receives the payload correctly.

Currently only supports ESP32-S3 DevKitC-1 v1.1 with the WROOM2 N32R16V

## Current Progress
1. Creates the Matter endpoint
2. Has commissioning
3. Has different functionalites for the Matter device, such as turning in on, changing colors, etc.
4. State of the light should be reflected

## To-do
### Operation with Wi-Fi (To be completed first)
1. Verify that commissioning works properly (must connect to Home Assistant)
2. Verify that endpoint can be seen
3. Verify that updating the main program doesn't ruin the commissioning if it is already done
4. Verify that features such as turning on/off light and changing colors work
5. Verify if the state change is reflected on the Home Assistant card

### Without Wi-Fi (WIRED)
1. Enable UART functionality
2. De-frame the custom frame of the IPv6 packets
3. Have Matter take in the de-framed packets
4. Go through all functionality tests (1-5) from Operation with Wi-Fi, except this time it is wired

### Tests Completed
1. None