import logging
import os
import sys
import time
from typing import Union
from serial import SerialException
from abstract_device import AbstractDevice
from global_defs import Protocol, Platform, SpiFlashOperation

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\MM_framework")

from MM_framework.mm_protocol import MMinterface, StPlatform  # noqa: E402
from MM_framework.mm_defs import Modulations, SpiFlashOp  # noqa: E402


class MMDevice(AbstractDevice):
    modulation = Modulations.BPSK_COD
    level = 21

    def __init__(self, port, platform=Platform.ST8500, lg=logging, log_level_prot=logging.ERROR,
                 log_level_if=logging.ERROR):
        super().__init__(protocol=Protocol.MM, lg=lg)

        try:
            mm_platform = StPlatform(platform)
            self.protocol_if = MMinterface(port, mm_platform, log_level_prot=log_level_prot, log_level_if=log_level_if)
            self.modulation = Modulations.BPSK_COD
            self.level = 21
            self.init_fail = False
        except SerialException:
            self.log.error("Warning, device was already initialized")

    def __str__(self):
        return " MM Node"

    def __del__(self):
        self.close()

    def close(self):
        self.protocol_if.close()

    def wait_startup(self):
        return self.protocol_if.wait_startup()

    def reset(self, confirm_expected):
        return self.protocol_if.reset(confirm_expected)

    def get_baudrate(self):
        """
        Returns the current baudrate in use in BOOT
        :return: The current used baudrate
        """
        return self.protocol_if.get_baudrate()

    def version(self):
        return self.protocol_if.version()

    def check_protocol(self):
        """
        Detects if BOOT protocol is currently running on the platform
        :return: True, it the protocol matches, the BOOT version, None
        """
        # Delay Needed for the FW to recover from previous request for other protocols
        time.sleep(0.5)  # TODO: FIND CAUSE OF THIS NECESSARY DELAY

        version_1, version_2 = self.version()

        if self.protocol_if.platform == StPlatform.ST8500:
            if version_1 is not None and version_2 is not None:
                success = True
            else:
                success = False
                if version_1 is None:
                    self.log.error("Could not get PE version")

                if version_2 is None:
                    self.log.error("Could not get RTE version")
        else:
            if version_1 is not None:
                success = True
            else:
                success = False
                if version_1 is None:
                    self.log.error("Could not get FW release")

        return success, version_1, version_2

    def set_configuration(self, band, channel_mode=None):
        self.protocol_if.default_configuration(address=None)

    def send_phy_data(self, array, level=0, modulation=Modulations.BPSK_COD):
        if self.modulation != modulation or self.level != level:
            self.modulation = modulation
            self.level = level
            self.protocol_if.set_level_and_modulation(tx_gain=self.level, tx_modulation=self.modulation)

        return self.protocol_if.raw_data(payload=array)

    def wait_phy_data(self, timeout=1):
        return self.protocol_if.received_msg_handler(timeout=timeout)

    def get_phy_data(self):
        return self.protocol_if.received_msg_handler(timeout=0)

    def get_thermal_status(self):
        max_t, min_t = self.protocol_if.get_temperature()

        return self.parse_thermal_status(max_t, min_t)

    @staticmethod
    def parse_thermal_status(max_t: int, min_t: int):

        if max_t == 0:
            max_t_str = "T max < 70 °C"
        elif max_t == 1:
            max_t_str = "T max < 100 °C"
        elif max_t == 2:
            max_t_str = "T max < 125 °C"
        elif max_t == 3:
            max_t_str = "T max > 125 °C"
        else:
            max_t_str = "invalid ({})".format(max_t)

        if min_t == 0:
            min_t_str = "T min < 70 °C"
        elif min_t == 1:
            min_t_str = "T min < 100 °C"
        elif min_t == 2:
            min_t_str = "T min < 125 °C"
        elif min_t == 3:
            min_t_str = "T min > 125 °C"
        else:
            min_t_str = "invalid ({})".format(min_t)

        return max_t_str, min_t_str

    def sflash_request(self, operation: SpiFlashOperation, address=0, data=None, size=0) -> Union[bool, list]:

        if data is None:
            data = []

        if operation == SpiFlashOperation.erase:
            mm_operation = SpiFlashOp.erase
        elif operation == SpiFlashOperation.read:
            mm_operation = SpiFlashOp.read
        elif operation == SpiFlashOperation.write:
            mm_operation = SpiFlashOp.write
        else:
            raise Exception("Invalid operation")

        return self.protocol_if.sflash_request(mm_operation, address, data, len(data))
