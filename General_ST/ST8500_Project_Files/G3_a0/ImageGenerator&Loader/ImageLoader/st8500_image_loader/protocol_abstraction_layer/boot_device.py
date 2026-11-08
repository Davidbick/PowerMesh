import logging
import os
import sys
import time
from serial import SerialException
from abstract_device import AbstractDevice
from global_defs import Protocol

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\BOOT_framework")

from BOOT_framework.boot_protocol import BootInterface


class BootDevice(AbstractDevice):

    def __init__(self, port, baudrate=None, lg=logging, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):
        super().__init__(protocol=Protocol.BOOT, lg=lg)

        try:
            self.protocol_if = BootInterface(port_name=port, baudrate=baudrate, log_level_prot=log_level_prot,
                                             log_level_if=log_level_if)
            self.init_fail = False
        except SerialException:
            self.log.error("Warning, device was already initialized")

    def __str__(self):
        return "BOOT Node"

    def __del__(self):
        self.close()

    def close(self):
        self.protocol_if.close()

    def wait_startup(self):
        time.sleep(0.5)
        return False

    def reset(self, confirm_expected):
        return self.protocol_if.reset(confirm_expected)

    def get_baudrate(self):
        """
        Returns the current baudrate in use in BOOT
        :return: The current used baudrate
        """
        return self.protocol_if.get_baudrate()

    def set_baudrate(self, baudrate):
        """
        Sets the baudrate to use in BOOT
        :param baudrate: Baudrate value to use
        :return: Nothing
        """
        self.protocol_if.set_baudrate(baudrate)

    def change_baudrate(self, baudrate):
        """
        Changes the baudrate to use in BOOT
        :param baudrate: Baudrate value to use
        :return: Result of the operation
        """
        return self.protocol_if.change_baudrate(baudrate)

    def check_protocol(self):
        """
        Detects if BOOT protocol is currently running on the platform
        :return: True, it the protocol matches, the BOOT version, None
        """
        boot_version = self.version()

        if boot_version is not None:
            success = True
        else:
            success = False

        return success, boot_version, None

    def initialize_image(self, image_header):
        return self.protocol_if.initialize_image(image_header)

    def write_image_data(self, image_data):
        return self.protocol_if.write_image_data(image_data)

    def start_image(self):
        return self.protocol_if.start_image()
