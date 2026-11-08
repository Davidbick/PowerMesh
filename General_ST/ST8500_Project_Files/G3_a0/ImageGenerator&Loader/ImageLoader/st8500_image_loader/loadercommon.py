import logging
import libscrc
import os
import sys
from enum import IntEnum
import time

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\protocol_abstraction_layer")

from protocol_abstraction_layer.boot_device import BootDevice  # noqa: E402
from protocol_abstraction_layer.generic_device import GenericDevice  # noqa: E402
from protocol_abstraction_layer.global_defs import Protocol, SpiFlashOperation  # noqa: E402


class ImageHeader(IntEnum):
    image_type_position = 0x00,
    validity_status_position = 0x04,
    lib_version_position = 0x08,
    fw_version_position = 0x0C,
    fw_crc32_position = 0x10
    section_address_position = 0x50,
    section_size_position = 0x54,
    global_header_size = 0x58,
    section_header_size = 0x8,
    max_chunk_size = 0x400


class ImageType(IntEnum):
    PE = 0x0000F1F1
    RTE = 0x0000A3A3

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_

    def __str__(self):
        return self.name


class ValidityStatus(IntEnum):
    not_validated = 0xFFFFFFFE
    primary = 0xFFFFFFFC
    secondary = 0xFFFFFFF8
    Invalid = 0xFFFFFFF0

    def __str__(self):
        return self.name


class SpiFlashDefine(IntEnum):
    spi_flash_page_size = 256
    spi_flash_pages_in_sector_cnt = 256
    spi_flash_sectors_cnt = 32
    spi_flash_sector_size = spi_flash_page_size * spi_flash_pages_in_sector_cnt
    spi_flash_size = spi_flash_sectors_cnt * spi_flash_sector_size

    def __str__(self):
        return self.name


class OperationStep:
    check = "Checking protocol"
    erase = "Erasing SPI FLASH"
    erase_nvm = "Erasing NVM"
    erase_full = "Erasing entire SPI FLASH"
    reset = "Resetting"
    change_baudrate = "Setting baudrate"
    upload_pe = "Uploading PE Image"
    upload_rte = "Uploading RTE Image"
    program_pe = "Programming PE Image"
    program_rte = "Programming RTE Image"


class LoaderCommon:

    image_loader_folder = "imgs"
    image_loader_g3 = image_loader_folder + "\\ST8500_SPI_Loader.img"
    image_loader_mm = image_loader_folder + "\\ST8500_SPI_Loader_old.img"

    rte_sectors = [0, 1]
    pe_sectors = [2, 3, 4, 5]
    nvm_sectors = [28, 29, 30, 31]
    all_sectors = list(range(0, 32))

    detect_callback = None
    step_callback = None
    progress_callback = None
    log = None
    log_level_prot = logging.ERROR
    log_level_if = logging.ERROR

    current_protocol = None
    current_baudrate = None

    abort_ref = [False]

    def __init__(self, protocol=None, baudrate=None, spi_loader_g3=None, spi_loader_mm=None, abort_ref=None,
                 lg=logging):

        self.current_protocol = protocol
        self.current_baudrate = baudrate

        if spi_loader_g3 is not None:
            self.image_loader_g3 = spi_loader_g3

        if spi_loader_mm is not None:
            self.image_loader_mm = spi_loader_mm

        if abort_ref is not None:
            self.abort_ref = abort_ref

        self.log = lg

    def set_detect_callback(self, function):
        self.detect_callback = function

    def set_step_callback(self, function):
        self.step_callback = function

    def set_progress_callback(self, function):
        self.progress_callback = function

    def set_protocol_log_level(self, log_level=logging.ERROR):
        self.log_level_prot = log_level

    def set_interface_log_level(self, log_level=logging.ERROR):
        self.log_level_if = log_level

    def wait_device(self, port, expected_protocol=None):
        if expected_protocol is not None:
            protocol = expected_protocol
        else:
            protocol = self.current_protocol

        self.current_baudrate = None  # At startup, the default baudrate is expected

        self.log.info("Waiting device...")
        device = GenericDevice(port, protocol, self.current_baudrate, lg=self.log,
                               log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
        ret = device.wait_startup()

        if ret:
            self.current_protocol = device.protocol
            self.current_baudrate = device.get_baudrate()

            if self.detect_callback is not None:
                self.detect_callback(self.current_protocol, self.current_baudrate)

        device.close()

    def detect_protocol(self, port, all_baudrates=False):
        """
        Executes the procedure used to detect which protocol is currently running on the ST8500
        :param port: Name of the COM port to use
        :param all_baudrates: Determines if all baudrates must be checked for G3 and BOOT
        :return: The running protocol, the PE version, the RTE version
        """
        protocol = None
        baudrate = None
        version_pe = ""
        version_rte = ""

        if self.step_callback is not None:
            self.step_callback(OperationStep.check)

        if self.progress_callback is not None:
            self.progress_callback(0)

        if self.current_protocol is not None:
            self.log.info("Checking previous protocol first")
            device = GenericDevice(port, self.current_protocol, self.current_baudrate, lg=self.log,
                                   log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
            protocol, baudrate, version_pe, version_rte = device.check_protocol()
            device.close()

        if protocol is None:
            # Protocols to check
            protocol_list = [Protocol.BOOT, Protocol.G3, Protocol.PRIME_1_4, Protocol.MM]

            # Baudrate tried for BOOT/G3
            if all_baudrates:
                self.log.info("Scanning all baudrates")
                baudrate_list = [None, 9600, 115200, 230400, 460800, 921600]
            else:
                baudrate_list = [None]

            checklist = []

            # Insert all protocol/baudrates combinations in the checklist
            for br in baudrate_list:
                for prot in protocol_list:
                    if prot != Protocol.BOOT and prot != Protocol.G3 and br is not None:
                        continue
                    else:
                        checklist.append([prot, br])

            i = 0
            for protocol_to_check, baudrate_to_check in checklist:
                device = GenericDevice(port, protocol_to_check, baudrate_to_check, lg=self.log,
                                       log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
                protocol, baudrate, version_pe, version_rte = device.check_protocol()
                device.close()

                if protocol == Protocol.PRIME_1_4:
                    protocol = device.protocol

                i += 1

                if self.progress_callback is not None:
                    progress = (i * 100)/len(checklist)
                    self.progress_callback(progress)

                if self.abort_ref[0]:
                    self.log.warning("Operation aborted")
                    break

                if protocol is not None:
                    break

        if self.progress_callback is not None:
            self.progress_callback(100)

        if protocol is not None:
            self.current_protocol = protocol
            self.current_baudrate = baudrate

            if protocol != Protocol.BOOT:
                self.log.info("Running protocol: {}, @{} bps, platform FW: PE v{}, RTE v{}".format(
                    self.current_protocol, self.current_baudrate, version_pe, version_rte))
            else:
                self.log.info("Running protocol: {}, @{} bps".format(self.current_protocol, self.current_baudrate))
        else:
            self.current_protocol = None
            self.current_baudrate = None
            self.log.error("Could not get protocol, check serial connection and reset the ST8500")

        if self.detect_callback is not None:
            self.detect_callback(self.current_protocol, self.current_baudrate)

        return protocol, version_pe, version_rte

    def change_baudrate(self, port, baudrate: int):
        """
        Changes the host interface serial baudrate
        :param port: Name of the COM port to use
        :param baudrate: Baudrate to set
        :return: True if successful, False otherwise
        """

        if self.step_callback is not None:
            self.step_callback(OperationStep.change_baudrate)

        if self.progress_callback is not None:
            self.progress_callback(0)

        self.log.info("Changing baudrate to {} bps".format(baudrate))
        device = GenericDevice(port, self.current_protocol, self.current_baudrate, lg=self.log,
                               log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
        ret = device.change_baudrate(baudrate)
        device.close()

        if self.progress_callback is not None:
            self.progress_callback(100)

        if ret:
            self.current_baudrate = baudrate

            if self.detect_callback is not None:
                self.detect_callback(self.current_protocol, self.current_baudrate)
        else:
            self.log.error("Cannot change baudrate")

        return ret

    def reset(self, port, expected_confirm):
        """
        Resets the device
        :param port: Name of the COM port to use
        :param expected_confirm: True if the confirmation is expected (protocol is not changing), False otherwise
        :return: The result of the operation
        """
        if self.step_callback is not None:
            self.step_callback(OperationStep.reset)

        if self.progress_callback is not None:
            self.progress_callback(0)

        self.log.info("Resetting platform...")
        device = GenericDevice(port, self.current_protocol, self.current_baudrate, lg=self.log,
                               log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
        ret = device.reset(expected_confirm)
        device.close()

        if (not ret) or (not expected_confirm):
            self.current_protocol = None  # No answer means that the protocol may have changed

        self.current_baudrate = None  # Baudrate is reset to the default value

        if self.progress_callback is not None:
            self.progress_callback(100)

        return ret

    def spi_flash_erase_sectors(self, port: str, sectors: list):
        """
        Erases a specific list of SPI FLASH sectors
        :param port: Name of the COM port to use
        :param sectors: List of sectors to erase
        :return: True if successful, False otherwise
        """
        success = True
        self.log.info("Erasing SPI FLASH...")
        device = GenericDevice(port, self.current_protocol, self.current_baudrate, lg=self.log,
                               log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)
        count = 0

        if self.progress_callback is not None:
            self.progress_callback(0)

        for sector in sectors:
            self.log.info("Erasing sector {}...".format(sector))
            address = sector * SpiFlashDefine.spi_flash_sector_size
            erase_ret = device.sflash_request(SpiFlashOperation.erase, address)
            if not erase_ret:
                success = False
                self.log.error("Error erasing sector {}".format(sector))
            count += 1

            if self.progress_callback is not None:
                progress = 100 * count / len(sectors)
                self.progress_callback(progress)

            if self.abort_ref[0]:
                self.log.warning("Operation aborted")
                break

        device.close()

        return success

    def spi_flash_erase(self, port, erase_pe=True, erase_rte=True):
        """
        Erases the PE and/or the RTE sectors
        :param port: Name of the COM port to use
        :param erase_pe: Should the PE image be erased?
        :param erase_rte: Should the PE image be erased?
        :return: True if successful, False otherwise
        """

        if self.step_callback is not None:
            self.step_callback(OperationStep.erase)

        sector_to_erase_list = []

        if erase_rte:
            sector_to_erase_list.extend(self.rte_sectors)

        if erase_pe:
            sector_to_erase_list.extend(self.pe_sectors)

        return self.spi_flash_erase_sectors(port, sector_to_erase_list)

    def spi_flash_erase_nvm(self, port):
        """
        Erases the NVM sectors
        :param port: Name of the COM port to use
        :return: True if successful, False otherwise
        """

        if self.step_callback is not None:
            self.step_callback(OperationStep.erase_nvm)

        return self.spi_flash_erase_sectors(port, self.nvm_sectors)

    def spi_flash_erase_full(self, port):
        """
        Erases the entire SPI FLASH memory
        :param port: Name of the COM port to use
        :return: True if successful, returns False otherwise
        """

        if self.step_callback is not None:
            self.step_callback(OperationStep.erase_full)

        return self.spi_flash_erase_sectors(port, self.all_sectors)

    @staticmethod
    def parse_header_field(header: list, position: int) -> int:
        value = 0
        for i in range(0, 4):
            value |= ((header[position + i] & 0xFF) << (8 * i))

        return value

    def read_image_header(self, fp):
        header_size = ImageHeader.global_header_size  # For single image use-case

        self.log.debug("Reading global header...")
        header = list(fp.read(header_size))

        # The section number is at the end of the global header
        section_number = self.parse_header_field(header, ImageHeader.section_size_position)

        # Get Section header
        section_header_len = section_number * ImageHeader.section_header_size

        if section_number % 2 == 0:
            section_header_len = section_header_len + ImageHeader.section_header_size

        self.log.debug("Reading section header...")
        section_header = list(fp.read(section_header_len))
        header.extend(section_header)

        return header

    def check_image(self, file_name: str, file_type: ImageType):  # -> tuple[bool, int, int]
        success = True
        crc32_read = 0
        crc32_calculated = 0

        try:
            with open(file_name, "rb") as f:

                # Read image header
                header = self.read_image_header(f)
                img_data = f.read()
                f.close()

                # Parse Image type
                image_type = self.parse_header_field(header, ImageHeader.image_type_position)

                if not ImageType.has_value(image_type):
                    success = False
                    self.log.warning(f"Invalid image, image type unknown: {image_type} image")
                elif image_type != file_type:
                    image_type = ImageType(image_type)
                    success = False
                    self.log.warning(f"Incompatible image, selected a {image_type} image "
                                     f"instead of a {file_type} image")

                # Parse FW version
                fw_version = self.parse_header_field(header, ImageHeader.fw_version_position)

                if file_type == ImageType.PE:
                    lib_version = self.parse_header_field(header, ImageHeader.lib_version_position)
                    self.log.info("FW version: 0x{:02X} - 0x{:02X}".format(fw_version, lib_version))
                else:
                    self.log.info("FW version: 0x{:02X}".format(fw_version))

                # Parse the crc32 inside the image header
                crc32_read = self.parse_header_field(header, ImageHeader.fw_crc32_position)

                # Compute the crc32 of the image's payload
                crc32_calculated = libscrc.hacker32(img_data, poly=0x4C11DB7, init=0x00, xorout=0x00, refin=False,
                                                    refout=False)

                if crc32_read == crc32_calculated:
                    self.log.info("CRC32 check valid: 0x{:02X}".format(crc32_read))
                else:
                    success = False
                    self.log.warning("CRC32 check invalid: read 0x{:02X}, calculated 0x{:02X}".format(crc32_read,
                                                                                                      crc32_calculated))
        except (FileNotFoundError, IOError):
            self.log.error(f"File {file_name} not found")

        return success, crc32_read, crc32_calculated

    def load_image(self, device: BootDevice, file_name: str):
        """
        Uploads a PE or RTE image file to the ST8500 RAM memory
        :param device: device in bootloader mode
        :param file_name: file name of the image to upload to the ST8500 RAM
        :return: True if the operation is successful, returns false otherwise
        """
        success = False

        chunk_size = ImageHeader.max_chunk_size
        try:
            with open(file_name, "rb") as f:
                total_size = os.fstat(f.fileno()).st_size

                # Read image header
                header = self.read_image_header(f)

                # Initialize current N bytes read
                current_size = len(header)

                self.log.info("Loading image header...")
                success = device.initialize_image(header)

                if success:
                    self.log.info("Uploading image in RAM...")
                    payload = list(f.read(chunk_size))

                    while len(payload) != 0:
                        success = device.write_image_data(payload)

                        if success:
                            current_size += len(payload)

                            if self.progress_callback is not None:
                                progress = round(current_size * 100 / total_size, 1)
                                self.progress_callback(progress)

                            if self.abort_ref[0]:
                                self.log.warning("Operation aborted")
                                break

                            payload = list(f.read(chunk_size))
                        else:
                            self.log.error("ERROR: Failed to load image chunk")
                            break

                    if current_size == total_size:
                        self.log.info("Upload complete")
                        success = device.start_image()
                        if success:
                            self.log.info("Image started successfully")
                        else:
                            self.log.error("Image start failed")
                    else:
                        self.log.warning("Upload incomplete")
                else:
                    self.log.error("ERROR: Failed to load image header")
                f.close()
        except (FileNotFoundError, IOError):
            self.log.error(f"File {file_name} not found")

        return success

    def upload_to_ram(self, port, pe_file, rte_file, upload_baudrate):
        """
        Sequence used to upload the PE and RTE image to the ST8500 RAM memory
        :param port: Name of the COM port to use
        :param upload_baudrate: baudrate to set for the upload (will replace the default one of 9600 bps)
        :param pe_file: name of the PE image file to upload
        :param rte_file: name of the RTE image file to upload
        :return: Result of the operation
        """
        success = False

        if rte_file is not None:
            self.log.info("Uploading RTE, PE images to RAM")
        else:
            self.log.info("Uploading PE image to RAM")

        # Must start in BOOT
        if self.current_protocol != Protocol.BOOT:
            self.detect_protocol(port)

        if self.current_protocol != Protocol.BOOT:
            self.log.warning("BOOT protocol required")
        else:
            device = BootDevice(port, baudrate=self.current_baudrate, lg=self.log)

            if self.step_callback is not None:
                self.step_callback(OperationStep.change_baudrate)

            if self.progress_callback is not None:
                self.progress_callback(0)

            self.current_baudrate = device.get_baudrate()
            ret = device.change_baudrate(upload_baudrate)

            if self.progress_callback is not None:
                self.progress_callback(100)

            if ret:
                self.current_baudrate = upload_baudrate

                if self.detect_callback is not None:
                    self.detect_callback(self.current_protocol, self.current_baudrate)

                success_rte = False
                success_pe = False

                # Load RTE File
                if rte_file is not None:

                    if self.step_callback is not None:
                        self.step_callback(OperationStep.upload_rte)

                    if self.load_image(device, rte_file):
                        success_rte = True
                        self.log.info("RTE image file Loaded: " + rte_file)
                    else:
                        self.log.error("Failed to load RTE image file: " + rte_file)
                else:
                    success_rte = True
                    self.log.warning("RTE image load skipped")

                if self.step_callback is not None:
                    self.step_callback(OperationStep.upload_pe)

                # Load PE File
                if self.load_image(device, pe_file):
                    success_pe = True
                    self.log.info("PE image file loaded: '{}'".format(pe_file))
                else:
                    self.log.error("Failed to load PE image file: " + pe_file)

                success = success_pe and success_rte
            else:
                self.log.error("Could not change baudrate")

            device.close()

        return success

    def write_image(self, port, file_name, start_sector):
        """
        Writes an image file in a specific SPI flash memory sector in G3
        :param port: Name of the COM port to use
        :param file_name: File to write
        :param start_sector: Initial sector to write
        :return: True if successful, False otherwise
        """
        success = False

        if self.current_protocol == Protocol.G3 or self.current_protocol == Protocol.MM:
            if self.current_protocol == Protocol.G3:
                chunk_size = 256  # Maximum payload size for the write operation
            else:
                chunk_size = 128

            self.log.info("Writing SPI FLASH...")
            device = GenericDevice(port, self.current_protocol, self.current_baudrate, lg=self.log,
                                   log_level_prot=self.log_level_prot, log_level_if=self.log_level_if)

            address = start_sector * SpiFlashDefine.spi_flash_sector_size
            with open(file_name, "rb") as f:
                total_size = os.fstat(f.fileno()).st_size
                current_size = 0
                payload = list(f.read(chunk_size))
                self.log.info("Writing file '{}' ({} bytes) to sector {}".format(f.name, total_size, start_sector))
                while len(payload) != 0:
                    if (address % SpiFlashDefine.spi_flash_sector_size) == 0:

                        erase_ret = device.sflash_request(SpiFlashOperation.erase, address)
                        if not erase_ret:
                            sector = int(address / SpiFlashDefine.spi_flash_sector_size)
                            self.log.error("Error in erase operation of sector {}".format(sector))
                            break

                    write_ret = device.sflash_request(SpiFlashOperation.write, address, payload)

                    if write_ret:
                        current_size += len(payload)
                        address += len(payload)
                        payload = list(f.read(chunk_size))

                        if self.progress_callback is not None:
                            progress = round(current_size * 100 / total_size, 1)
                            self.progress_callback(progress)

                        if self.abort_ref[0]:
                            self.log.warning("Operation aborted")
                            break
                    else:
                        self.log.error("Error in write operation (address=0x%X, size=%u)" % (address, len(payload)))
                        break

                if current_size == total_size:
                    success = True
                else:
                    success = False
                    self.log.warning("Write incomplete")

                f.close()

            device.close()
        else:
            self.log.error("Unsupported protocol for '{}'".format(self.write_image.__name__))

        return success

    def program_pe_rte(self, port, pe_image=None, rte_image=None):
        """
        :param port: Name of the COM port to use
        :param pe_image: PE image file to write
        :param rte_image: RTE image file to write
        Writes the PE and RTE images in the SPI flash memory in G3
        :return: 3: both images are written, 2: only the RTE is written, 1: only the PE is written
        """
        self.log.info("Programming SPI Flash Memory")
        ret = 0
        if pe_image is not None:
            sector = 2

            if self.step_callback is not None:
                self.step_callback(OperationStep.program_pe)

            if self.progress_callback is not None:
                self.progress_callback(0)

            if self.write_image(port, pe_image, sector):
                ret += 1
        if rte_image is not None:
            sector = 0

            if self.step_callback is not None:
                self.step_callback(OperationStep.program_rte)

            if self.progress_callback is not None:
                self.progress_callback(0)

            if self.write_image(port, rte_image, sector):
                ret += 2

        return ret

    def program_spi_flash(self, port: str, pe_file: str, rte_file: str, program_protocol: Protocol,
                          program_baudrate: int):
        """
        :param port: Name of the COM port to use
        :param pe_file: name of the PE image file to program
        :param rte_file: name of the RTE image file to program
        :param program_protocol: Protocol used to do the SPI flash programming (will use the proper ST8500 SPI Loader)
        :param program_baudrate: baudrate to set for the upload/programming
        Sequence used to program the PE and RTE image to the ST8500 SPI flash memory
        :return: Protocol, PE version and RTE version strings.
        """
        success = False

        # SPI loader image name for ST8500
        if program_protocol == Protocol.G3:
            image_loader_pe = self.image_loader_g3
        elif program_protocol == Protocol.MM:
            image_loader_pe = self.image_loader_mm
        else:
            raise Exception("unsupported protocol")

        # Load SPI loader image
        try:
            fp = open(file=image_loader_pe, mode="r")
            fp.close()
        except FileNotFoundError:
            raise Exception("Could not find SPI Loader image file")

        # Load PE image
        if pe_file is not None:
            try:
                fp = open(file=pe_file, mode="r")
                fp.close()
            except FileNotFoundError:
                raise Exception("Could not find PE image file")

        # Load RTE image
        if rte_file is not None:
            try:
                fp = open(file=rte_file, mode="r")
                fp.close()
            except FileNotFoundError:
                raise Exception("Could not find RTE image file")

        result_ok = False

        if self.current_protocol is not None:
            if self.current_protocol == Protocol.BOOT:
                # No need to erase the flash memory if in BOOT
                result_ok = True
            else:
                # Only the PE is erased to have the BOOT mode after reset
                result_ok = self.spi_flash_erase(port=port, erase_pe=True, erase_rte=False)

                if result_ok:
                    self.reset(port, False)
                    # Necessary to wait the Bootloader to start
                    self.wait_device(port, Protocol.BOOT)

        if result_ok:
            result_ok = self.upload_to_ram(port, image_loader_pe, None, program_baudrate)

            if result_ok:
                self.current_protocol = program_protocol  # Sets the current protocol to the one of the image in RAM

                self.wait_device(port)
                result_ok = self.change_baudrate(port, program_baudrate)

                if result_ok:
                    # After SPI loader in RAM has been booted, programs the SPI Flash
                    result_ok = self.program_pe_rte(port=port, pe_image=pe_file, rte_image=rte_file)

                    if result_ok:
                        self.reset(port, False)
                        time.sleep(2)

                        self.current_protocol, ver_pe, ver_rte = self.detect_protocol(port)

                        if self.current_protocol is not None:
                            success = True

        else:
            self.log.error("Cannot proceed due to errors")

        return success
