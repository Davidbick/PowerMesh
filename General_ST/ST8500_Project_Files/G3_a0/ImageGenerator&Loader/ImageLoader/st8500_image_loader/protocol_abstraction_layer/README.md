## Package Content

This package contains the Protocol Abstraction Layer, a framework that unifies G3, PRIME, MM and ST8500 bootloader protocols under the same interface.
With this framework, all protocol can be accessed and controlled with the API.
	
This software is compatible with ST8500.
A direct serial communication between the PC and the ST8500 is required (if using an evaluation kit, make sure it is running in "passthrough mode").
	
## Info

The GenericDevice instantiate one of the following, depending on the selected protocol:
	- G3 device
	- PRIME device
	- MM device
	- BOOT device (ST8500 bootloader)

All of these are derivated from AbstractDevice.

Common methods:
    - __init__
    - close
    - wait_startup
    - reset
    - get_baudrate
    - set_baudrate
    - info
    - version
    - check_protocol
    - set_configuration
    - sflash_request

## Usage

Import the "generic_device" module and use "GenericDevice" to create a controller for a given protocol.