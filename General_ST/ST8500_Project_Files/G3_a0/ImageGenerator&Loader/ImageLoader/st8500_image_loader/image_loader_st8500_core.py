import configparser
import logging
import os
import serial
import sys
import time
from enum import IntEnum
from loadercommon import LoaderCommon
from version import StImageLoaderVersion

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\protocol_abstraction_layer")

from protocol_abstraction_layer.global_defs import Protocol  # noqa: E402


class LoaderOp(IntEnum):
    check = 0  # Detect running protocol
    upload = 1  # Write PE and RTE images in RAM memory
    program = 2  # Write PE and RTE images in SPI FLASH memory
    reset = 3  # Reset platform
    erase = 4  # Erase sectors 0-5
    erase_nvm = 5  # Erase sectors 28-31
    erase_full = 6  # Erase sectors 0-31
    invalid = 0xFF

    def __str__(self):
        if self == self.check:
            return "Check Protocol"
        elif self == self.upload:
            return "Upload to RAM"
        elif self == self.program:
            return "Program FLASH"
        elif self == self.reset:
            return "Reset"
        elif self == self.erase:
            return "Erase FLASH"
        elif self == self.erase_nvm:
            return "Erase NVM"
        elif self == self.erase_full:
            return "Full FLASH erase"


class StImageLoader:

    ini_filename = "image_loader_st8500_settings.ini"

    def __init__(self, com_port: str, program_baudrate: int, lg=logging):
        """
        Creates a StImageLoader object
        :param com_port: The serial COM port to use
        :param program_baudrate: The baudrate to use for uploading/programming operations
        :param lg: Logger to use for the log prints (leave empty for the root logger)
        """
        self.com_port = com_port
        self.program_baudrate = program_baudrate
        self.log = lg

        self.aborted = [False]  # a list is used to pass the reference to the Loader Common class

    @staticmethod
    def version():
        """
        Returns the standard Image Loader version
        :return: The Image Loader version in Major.Minor.Patch format
        """
        return "{}.{}.{}".format(StImageLoaderVersion.IMAGE_LOADER_VERSION_MAJOR,
                                 StImageLoaderVersion.IMAGE_LOADER_VERSION_MINOR,
                                 StImageLoaderVersion.IMAGE_LOADER_VERSION_PATCH)

    @staticmethod
    def version_extended():
        """
        Returns the extended Image Loader version
        return: The Image Loader version in Major.Minor.Patch.Release format
        """
        return "{}.{}.{}.{}".format(StImageLoaderVersion.IMAGE_LOADER_VERSION_MAJOR,
                                    StImageLoaderVersion.IMAGE_LOADER_VERSION_MINOR,
                                    StImageLoaderVersion.IMAGE_LOADER_VERSION_PATCH,
                                    StImageLoaderVersion.IMAGE_LOADER_VERSION_RELEASE)

    def try_file(self, file):
        """
        Check if a given file exists
        :param file: File to check
        :return: True if the file exists, False otherwise
        """
        ret = False

        if file is not None:
            try:
                fp = open(file, "rb")
                fp.close()
                ret = True
            except (FileNotFoundError, IOError):
                self.log.error("File '{}' not found".format(file))

        return ret

    def test_com_port(self, com_name, silent=False):
        """
        Tests a COM port availability
        :param com_name: COM port name (string) to check
        :return: The COM port name, if available, or None, if not
        """
        try:
            ret = com_name
            ser = serial.Serial(com_name)
            ser.close()
            if not silent:
                self.log.info("Selected " + com_name + " port")
        except serial.SerialException:
            ret = None
            self.log.error("Cannot open " + com_name + " port")

        return ret

    def run(self, operation: LoaderOp, pe_file="", rte_file="", protocol=None, baudrate=None,
            detect_ck=None, step_ck=None, progress_ck=None, spi_loader=None):
        """
        Executes the selected operation on the platform
        :param operation: The operation to perform on the platform (see LoaderOp class)
        :param pe_file: Path to the PE image, must be specified for program and upload operations
        :param rte_file: Path to the RTE image, can be specified in program and upload operations
        :param protocol: Current protocol running on the platform, use None if unknown (optional)
        :param baudrate: Current baudrate set on the platform, use None if unknown (optional)
        :param detect_ck: Protocol/baudrate detection callback, use None if not used (optional)
        :param step_ck: Step change callback, use None if not used (optional)
        :param progress_ck: Progress callback, use None if not used (optional)
        :param spi_loader: Path to the 'ST8500_SPI_Loader.img' file, required for programming, use None for
        default path: imgs/ST8500_SPI_Loader.img (optional)
        """
        # Starts counting time elapsed
        start_ts = time.time()
        success = False

        # Parse all arguments
        config = configparser.ConfigParser()

        # Read the configuration file
        if config.read(self.ini_filename):
            config.sections()
            scan_all_baudrates = int(config["SETTINGS"]["scan_all_baudrates"])
            protocol_log_level = int(config["LOGGING"]["protocol_level"])
            interface_log_level = int(config["LOGGING"]["interface_level"])
        else:
            self.log.warning("Configuration file not found, using default configuration")
            scan_all_baudrates = 0
            protocol_log_level = logging.ERROR
            interface_log_level = logging.ERROR

        # Common class for the image loader
        lc = LoaderCommon(protocol=protocol, baudrate=baudrate, spi_loader_g3=spi_loader, abort_ref=self.aborted,
                          lg=self.log)

        lc.set_protocol_log_level(protocol_log_level)
        lc.set_interface_log_level(interface_log_level)

        self.log.info("Log level of protocol layer: {}".format(logging.getLevelName(protocol_log_level)))
        self.log.info("Log level of interface layer: {}".format(logging.getLevelName(interface_log_level)))

        # Step and progress callback
        lc.set_detect_callback(detect_ck)
        lc.set_step_callback(step_ck)
        lc.set_progress_callback(progress_ck)

        pe_present = False
        program_protocol = None

        can_continue = True

        # Check SPI Loader (only for Programming)
        if operation == LoaderOp.program:
            if self.try_file(lc.image_loader_g3):
                program_protocol = Protocol.G3
            elif self.try_file(lc.image_loader_mm):
                program_protocol = Protocol.MM
                self.log.warning("Using {}".format(lc.image_loader_mm))
            else:
                self.log.error("Could not find '{}'".format(lc.image_loader_g3))
                can_continue = False

        if can_continue:
            # Check PE image (only for Upload and Programming)
            if operation == LoaderOp.upload or operation == LoaderOp.program:
                pe_present = self.try_file(pe_file)
                rte_present = self.try_file(rte_file)

                if pe_present:
                    pe_name = pe_file.split("/")[-1]
                    self.log.info("PE Image  : " + pe_name)
                else:
                    self.log.error("PE image is required")
                    can_continue = False

                if rte_present:
                    rte_name = rte_file.split("/")[-1]
                    self.log.info("RTE image : " + rte_name)
                else:
                    rte_file = None
                    self.log.info("No RTE image")

        if can_continue:
            # COM port and baudrate
            if self.test_com_port(self.com_port) != self.com_port:
                can_continue = False

        if can_continue:
            # Command to execute
            self.log.info("Executing command: {}".format(operation))
            if operation == LoaderOp.check:
                protocol, pe_ver, rte_ver = lc.detect_protocol(self.com_port, scan_all_baudrates)
                if protocol is not None:
                    success = True
            elif operation == LoaderOp.reset:
                if protocol is None:
                    protocol, pe_ver, rte_ver = lc.detect_protocol(self.com_port)
                if protocol is not None:
                    success = lc.reset(self.com_port, True)
                if not success:
                    protocol, pe_ver, rte_ver = lc.detect_protocol(self.com_port)
                    if protocol is not None:
                        success = True
            elif operation == LoaderOp.erase:
                if protocol is None:
                    protocol, ver_pe, ver_rte = lc.detect_protocol(self.com_port)
                if protocol != Protocol.BOOT:
                    success = lc.spi_flash_erase(self.com_port)
                else:
                    self.log.warning("Cannot erase in BOOT")
            elif operation == LoaderOp.erase_nvm:
                if protocol is None:
                    protocol, ver_pe, ver_rte = lc.detect_protocol(self.com_port)
                if protocol != Protocol.BOOT:
                    success = lc.spi_flash_erase_nvm(self.com_port)
                else:
                    self.log.warning("Cannot erase in BOOT")
            elif operation == LoaderOp.erase_full:
                if protocol is None:
                    protocol, ver_pe, ver_rte = lc.detect_protocol(self.com_port)
                if protocol != Protocol.BOOT:
                    success = lc.spi_flash_erase_full(self.com_port)
                else:
                    self.log.warning("Cannot erase in BOOT")
            elif operation == LoaderOp.upload:
                if pe_present:
                    if protocol is None:
                        protocol, ver_pe, ver_rte = lc.detect_protocol(self.com_port)
                    if protocol == Protocol.BOOT:
                        success = lc.upload_to_ram(self.com_port, pe_file, rte_file, self.program_baudrate)
                    else:
                        self.log.warning("BOOT mode required, please erase the SPI Flash memory, "
                                         "reset the ST8500 and then try again.")
                else:
                    self.log.error("Files missing: cannot proceed")
            elif operation == LoaderOp.program:
                if pe_present and (program_protocol is not None):
                    if protocol is None:
                        protocol, ver_pe, ver_rte = lc.detect_protocol(self.com_port)
                    if protocol is not None:
                        if protocol == Protocol.BOOT or protocol == Protocol.G3 or protocol == Protocol.MM:
                            success = lc.program_spi_flash(self.com_port, pe_file, rte_file, program_protocol,
                                                           self.program_baudrate)
                        else:
                            self.log.warning("Programming not supported for {} protocol".format(protocol))
                else:
                    self.log.error("Files missing, cannot proceed")
            else:
                self.log.error("Unknown command")

        elapsed_time = round(time.time() - start_ts, 3)

        self.log.info("Operation completed in {} s".format(elapsed_time))

        return success
