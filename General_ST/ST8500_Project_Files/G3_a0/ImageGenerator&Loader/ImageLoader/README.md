## Package Content

This package contains the ST8500 image loader, a software based on python that features:
	- Protocol detection (G3/PRIME/MM/BOOT...)
	- Image upload to RAM memory
	- SPI FLASH memory erase (main memory and NVM memory)
	- SPI FLASH memory programming
	- Platform reset
	
This software is compatible with ST8500.
A direct serial communication between the PC and the ST8500 is required (if using an evaluation kit, make sure it is running in "passthrough mode").
	
## Info

Two script versions are available for this software:
	- GUI (image_loader_st8500_gui.py)
	- Terminal (image_loader_st8500.py)
	
Both version rely on the image_loader_st8500_core.py module, where the main class is implemented.	

## Requirements:
The python scripts require Python 3.11 (or more recent) installed in the system
In order to run the python scripts, the python packages listed in "requirements.txt" must be installed

## Usage

The GUI version can be launched by double-clicking on "start_gui.vbs". Its usage is explained internally.

The terminal version usage is displayed by launching the command "python image_loader_st8500.py --help" inside its folder "st8500_image_loader"
Some batch files are present with predefined commands:
	- ST8500_get_protocol.bat
	- ST8500_reset.bat
	- ST8500_FLASH_erase.bat
	- ST8500_FLASH_erase_full.bat
	- ST8500_FLASH_erase_nvm.bat
	- ST8500_G3_Hybrid_PE_RTE_upload.bat
	- ST8500_G3_Hybrid_PE_RTE_write_to_RAM.bat