import logging
import os
import sys
import time
from serial import SerialException
from global_defs import Protocol, ChannelMode, PrimeChannel, St8500Defs, SpiFlashOperation
from abstract_device import AbstractDevice


# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\prime_validation_framework1_4\\src")

from prime_if import PrimeIf
from prime_validation_framework1_4.src.protocol import Modulation, SpecPibs, CommonVendSpecPibs  # noqa: E402
import prime_validation_framework1_4.src.servicenodeemb as prime_net  # noqa: E402
from prime_validation_framework1_4.src.basenode import BaseNode


class PrimeDevice(AbstractDevice):
    default_baudrate = 57600
    protocol = None

    app_version_len = 16

    prime_1_4_mask = 0x40

    def __init__(self, port, lg=logging, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):
        super().__init__(protocol=self.protocol, lg=lg)

        try:
            self.protocol_if = PrimeIf(port)

            self.protocol = self._get_prime_version()

            if self.protocol == Protocol.PRIME_1_4 or self.protocol == Protocol.PRIME_1_3_6:
                self.init_fail = self.protocol_if.InitFailure
            else:
                self.init_fail = True

        except SerialException:
            self.log.error("Warning, device was already initialized")

        # Get logger
        self.log_prot = logging.getLogger(prime_net.__name__)

        # Disable propagation
        self.log_prot.propagate = False

        # Only the first time, to avoid duplicate handlers
        if len(self.log.handlers) == 0:
            # Define logging format
            logger_fmt = "%(asctime)s %(levelname)s %(filename)s - %(funcName)s(): %(message)s"
            logger_datefmt = "%H:%M:%S"
            logger_formatter = logging.Formatter(fmt=logger_fmt, datefmt=logger_datefmt)

            # Console Logger handler
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setFormatter(logger_formatter)

            # Use defined handler
            self.log_prot.addHandler(console_handler)

        # Set logger level
        self.log_prot.setLevel(log_level_prot)

        self.log_prot.info("Initialize the PRIME interface with {}".format(port))

    def __str__(self):
        if self.protocol == Protocol.PRIME_1_4:
            version = "1.4"
        else:
            version = "1.3.6"
        return "PRIME " + version + " Service Node"

    def __del__(self):
        self.close()

    def close(self):
        self.protocol_if.close()

    def _get_prime_version(self):
        product_id = self.protocol_if.GetPib(pib=self.protocol_if.Pibs.AppProductId)

        if isinstance(product_id, int):
            if (product_id & self.prime_1_4_mask) != 0:
                protocol = Protocol.PRIME_1_4
            else:
                protocol = Protocol.PRIME_1_3_6
        else:
            protocol = None

        return protocol

    def is_prime_1_4(self):
        if self.protocol == Protocol.PRIME_1_4:
            ret = True
        else:
            ret = False

        return ret

    def get_baudrate(self):
        return self.default_baudrate

    def set_baudrate(self, baudrate):
        pass

    def info(self):
        return self.protocol_if.NodeInfo()

    def version(self):
        app_fw_ver_pib = self.protocol_if.GetPib(self.protocol_if.Pibs.AppFwVersion)

        if len(app_fw_ver_pib) == self.app_version_len:
            version_pe = version_rte = ""

            for ascii_value in app_fw_ver_pib[3:7]:
                version_pe += chr(ascii_value)

            for ascii_value in app_fw_ver_pib[12:16]:
                version_rte += chr(ascii_value)

        else:
            version_pe = None
            version_rte = None

        return version_pe, version_rte

    def check_protocol(self):
        """
        Detects if G3 protocol is currently running on the platform
        :return: True, it the protocol matches, the PE version, the RTE version
        """
        version_pe, version_rte = self.version()

        if version_pe is not None:
            success = True
        else:
            success = False

        return success, version_pe, version_rte

    def reset(self, expect_confirm):
        self.protocol_if.SwReset()

    def set_configuration(self, channel=PrimeChannel.Channel1, channel_mode=ChannelMode.PLC):

        ret = self.set_phy_mode(channel_mode)

        if ret:
            ret = self.set_channel(channel)

        return ret

    def set_phy_mode(self, channel_mode: ChannelMode):
        success = False
        read_back = self.protocol_if.PrimeConfig.PHY + 1
        counter = 0

        while read_back != self.protocol_if.PrimeConfig.PHY:
            self.log.info("Setting PHY mode...")
            mode = self.protocol_if.PrimeConfig.PHY
            self.protocol_if.SetPib(prime_net.ServiceNodeEmb.Pibs.primeConfig, [mode])
            time.sleep(0.5)  # Necessary delay

            self.log.info("Checking PHY mode...")
            read_back = self.protocol_if.GetPib(prime_net.ServiceNodeEmb.Pibs.primeConfig)
            if read_back == self.protocol_if.PrimeConfig.PHY:
                success = True
            else:
                self.log.error("Error configuring primeConfig, mode is {} instead of {}".format(read_back, mode))
                counter += 1

                if counter > 3:
                    self.log.error("Cannot set PHY mode")

        return success

    def set_channel(self, channel=PrimeChannel.Channel1):

        success = False

        if self.protocol == Protocol.PRIME_1_4:
            channel = channel
        else:
            channel = PrimeChannel.Channel1

        ch_mask = channel
        # Must be called after SetPrimeConfigPhy
        self.log.info("Setting Channel Map to 0x%X...".format(ch_mask))
        self.protocol_if.SetPib(CommonVendSpecPibs.phyChannelMap, [ch_mask])
        read_back = self.protocol_if.GetPib(CommonVendSpecPibs.phyChannelMap)
        if read_back == ch_mask:
            success = True
        else:
            self.log.error("Error configuring phyChannelMap, {} instead of {}".format(read_back, ch_mask))

        return success

    def send_phy_data(self, array, level=0, modulation=Modulation.DBPSKCC):
        return self.protocol_if.PHY_SendData(array, level, modulation, timer=0, timeout=3)

    def wait_phy_data(self, timeout=1):
        return self.protocol_if.PHY_WaitData(timeout)

    def get_phy_data(self):
        return self.protocol_if.PHY_GetData()

    def get_ld_vcc(self):
        return self.protocol_if.GetPib(CommonVendSpecPibs.phyLDVcc)

    # LD voltage supply to be used for TXPower management.
    # phyLDVcc = 10, 12, 15 or 18
    def set_ld_vcc(self, value):
        if value in St8500Defs.ld_vcc_accepted_values:
            self.protocol_if.SetPib(CommonVendSpecPibs.phyLDVcc, [value])
            return True
        else:
            self.log.error("Invalid LD VCC value %d" % value)
            return False

    # MOL (max output level) decrease in dB, according to the following formula: TxPower (dB) = MOL(dB) - PHY_MOLoffset
    # phyMOLOffset = 0, 1, 2,…, 21
    def set_mol_offset(self, value):
        if 0 <= value <= 21:
            self.protocol_if.SetPib(CommonVendSpecPibs.phyMolOffset, [value])
            return True
        else:
            self.log.error("Invalid MOL offset value %d" % value)
            return False

    def get_mol_offset(self):
        return self.protocol_if.GetPib(CommonVendSpecPibs.phyMolOffset)

    def get_thermal_counter(self):
        return self.protocol_if.GetPib(CommonVendSpecPibs.phyThermCounter)

    def set_start_rx(self, start):
        self.protocol_if.SetPib(CommonVendSpecPibs.phyStartRxTest, [start])
        return True

    def set_start_tx(self, test_value):
        self.protocol_if.SetPib(CommonVendSpecPibs.phyStartTxTest, [test_value])
        return True

    def reset_stats(self):
        self.protocol_if.SetPib(CommonVendSpecPibs.phyCountersReset, [1])
        return True

    def get_stats_rx(self):
        phy_stats_crc_incorrect_count = self.protocol_if.GetPib(SpecPibs.phyStatsCRCIncorrectCount)
        phy_stats_crc_fail_count = self.protocol_if.GetPib(SpecPibs.phyStatsCRCFailCount)
        phy_stats_rx_drop_count = self.protocol_if.GetPib(SpecPibs.phyStatsRxDropCount)
        phy_stats_rx_total_count = self.protocol_if.GetPib(SpecPibs.phyStatsRxTotalCount)
        phy_stats_blk_avg_evm = self.protocol_if.GetPib(SpecPibs.phyStatsBlkAvgEvm)
        return phy_stats_crc_incorrect_count, phy_stats_crc_fail_count, phy_stats_rx_drop_count, \
            phy_stats_rx_total_count, phy_stats_blk_avg_evm

    def get_stats_tx(self):
        phy_stats_tx_drop_count = self.protocol_if.GetPib(SpecPibs.phyStatsTxDropCount)
        return phy_stats_tx_drop_count

    def sflash_request(self, operation: SpiFlashOperation, address=0, data=None, size=0):
        if operation == SpiFlashOperation.erase:
            self.protocol_if.RfuEvents
