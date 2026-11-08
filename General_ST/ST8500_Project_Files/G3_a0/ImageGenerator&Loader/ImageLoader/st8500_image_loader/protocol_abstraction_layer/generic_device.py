import time
from global_defs import Protocol, NodeMode, ChannelMode, Platform, SpiFlashOperation
import logging

from g3_device import G3Device
from prime_device import PrimeDevice
from mm_device import MMDevice
from boot_device import BootDevice


class GenericDevice:
    port = None
    name = None
    protocol = None
    plan = None
    device = None
    init_done = False

    def __init__(self, port: str, protocol: Protocol, baudrate=None, platform=Platform.ST8500, name="Default",
                 lg=logging, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):

        self.port = port
        self.name = name
        self.protocol = protocol
        self.log = lg

        if baudrate is not None:
            baudrate_str = str(baudrate)
        else:
            baudrate_str = "default"

        if log_level_prot <= logging.INFO:
            self.log.info("Device {}, protocol {}, baudrate {}".format(name, protocol, baudrate_str))

        if self.protocol == Protocol.G3:
            self.device = G3Device(port=self.port, baudrate=baudrate, lg=self.log, log_level_prot=log_level_prot,
                                   log_level_if=log_level_if)
        elif self.protocol == Protocol.PRIME_1_4 or self.protocol == Protocol.PRIME_1_3_6:
            self.device = PrimeDevice(port=self.port, lg=self.log, log_level_prot=log_level_prot,
                                      log_level_if=log_level_if)
            self.protocol = self.device.protocol
        elif self.protocol == Protocol.MM:
            self.device = MMDevice(port=self.port, platform=platform, lg=self.log, log_level_prot=log_level_prot,
                                   log_level_if=log_level_if)
        elif self.protocol == Protocol.BOOT:
            self.device = BootDevice(port=self.port, baudrate=baudrate, lg=self.log, log_level_prot=log_level_prot,
                                     log_level_if=log_level_if)
        else:
            self.log.error("Unsupported protocol: {}".format(self.protocol))

        self.init_done = True

    def __str__(self):
        if self.device is not None:
            return str(self.device)
        else:
            return "no device"

    def init(self, band):
        if not self.init_done:
            self.log.info("Initializing device {} with {}, port {}".format(self.protocol.name, self.name, self.port))
            if self.protocol == Protocol.BOOT:
                pass
            elif self.protocol == Protocol.G3:
                self.device.set_configuration(band)
            elif self.protocol == Protocol.PRIME_1_4 or self.protocol == Protocol.PRIME_1_3_6:
                self.device.set_configuration(band)
            elif self.protocol == Protocol.MM:
                self.device.set_configuration(band)
            else:
                self.log.error("Device on port " + self.port + " already initialized")
            self.init_done = True

            if self.device.init_fail:
                self.log.error("Device " + str(self.device) + " - initialization FAIL")
            else:
                self.log.info("Device " + str(self.device) + " - initialization OK")
        else:
            self.log.error("Device on port " + self.port + " already initialized")

    def close(self):
        self.device.close()

    def wait_startup(self):
        self.device.wait_startup()

    def reset(self, expected_confirm):
        return self.device.reset(expected_confirm)

    def get_baudrate(self):
        return self.device.get_baudrate()

    def set_baudrate(self, baudrate):
        return self.device.set_baudrate(baudrate)

    def change_baudrate(self, baudrate):
        return self.device.change_baudrate(baudrate)

    def check_protocol(self):  # -> tuple[Protocol, int, str, str]

        found, version_pe, version_rte = self.device.check_protocol()

        if found:
            protocol = self.protocol
            baudrate = self.device.get_baudrate()
        else:
            protocol = None
            baudrate = None

        return protocol, baudrate, version_pe, version_rte

    def test_bench_config(self, plan, role=NodeMode.Receiver, channel=ChannelMode.PLC):
        ret = True

        self.init(plan)

        time.sleep(1)

        if isinstance(self.device, G3Device) or isinstance(self.device, PrimeDevice):
            self.log.info("Setting phy mode ({}), role: {}".format(channel.name, role.name))

            if not self.device.set_channel(channel):
                ret = False

        if isinstance(self.device, G3Device):
            # Configure device role...if RX or TX since in G3 must be configured...
            if not self.set_role(role):
                ret = False

        return ret

    def set_channel(self, channel: ChannelMode):
        return self.device.set_channel(channel)

    def send_phy_data(self, array, level, modulation):
        return self.device.send_phy_data(array, level, modulation)

    def get_sn_array(self):
        if isinstance(self.device, G3Device):
            return self.device.get_sn_array()
        else:
            self.log.warning("{} is a G3 only feature".format(self.get_sn_array.__name__))

    def wait_phy_data(self, timeout=1):
        return self.device.wait_phy_data(timeout)

    def get_phy_data(self):
        return self.device.get_phy_data()

    def node_info(self):
        return self.device.info()

    def node_version(self):
        return self.device.version()

    def set_role(self, role: NodeMode):
        if isinstance(self.device, G3Device):
            return self.device.set_role(role)
        else:
            self.log.warning("{} is a G3 only feature".format(self.set_role.__name__))

    def set_ld_vcc(self, value):
        return self.device.set_ld_vcc(value)

    def get_ld_vcc(self):
        return self.device.get_ld_vcc()

    def set_mol_offset(self, value):
        if isinstance(self.device, PrimeDevice):
            return self.device.set_mol_offset(value)
        else:
            self.log.warning("{} is a PRIME only feature".format(self.set_mol_offset.__name__))

    def get_mol_offset(self):
        if isinstance(self.device, PrimeDevice):
            return self.device.get_mol_offset()
        else:
            self.log.warning("{} is a PRIME only feature".format(self.get_mol_offset.__name__))

    def get_thermal_status(self):
        if isinstance(self.device, G3Device) or isinstance(self.device, MMDevice):
            return self.device.get_thermal_status()
        else:
            self.log.warning("{} is a G3/MM only feature".format(self.get_thermal_status.__name__))

    def get_thermal_counter(self):
        if isinstance(self.device, PrimeDevice):
            return self.device.get_thermal_counter()
        else:
            self.log.warning("{} is a PRIME only feature".format(self.get_thermal_counter.__name__))

    def set_start_rx(self, start):
        if isinstance(self.device, PrimeDevice):
            return self.device.set_start_rx(start)
        else:
            self.log.warning("{} is a PRIME only feature".format(self.set_start_rx.__name__))

    def set_start_tx(self, test_value):
        if isinstance(self.device, PrimeDevice):
            return self.device.set_start_tx(test_value)
        else:
            self.log.warning("{} is a PRIME only feature".format(self.set_start_tx.__name__))

    def reset_stats(self):
        if isinstance(self.device, PrimeDevice):
            return self.device.reset_stats()
        else:
            self.log.warning("{} is a PRIME only feature".format(self.reset_stats.__name__))

    def get_stats_rx(self):
        if isinstance(self.device, PrimeDevice):
            return self.device.get_stats_rx()
        else:
            self.log.warning("{} is a PRIME only feature".format(self.get_stats_rx.__name__))

    def get_stats_tx(self):
        if isinstance(self.device, PrimeDevice):
            return self.device.get_stats_tx()
        else:
            self.log.warning("{} is a PRIME only feature".format(self.get_stats_tx.__name__))

    def thermal_cool_down(self, wait_time=5):
        if isinstance(self.device, G3Device):
            # Wait for the device to cool down if over 70 Celsius degrees (ThermalStatus=0)
            temp_reg = self.device.get_thermal_status()
            ld_str, lv_str = self.device.parse_thermal_status(temp_reg)
            while (temp_reg & 0x0F) > 0x00:
                self.log.info("Temperature is too high (LD: {}, LV: {}), "
                              "waiting {} s to cool down...".format(ld_str, lv_str, wait_time))
                time.sleep(wait_time)
                temp_reg = self.device.get_thermal_status()
                ld_str, lv_str = self.device.parse_thermal_status(temp_reg)
            self.log.info("Temperature is ok (LD: {}, LV: {})".format(ld_str, lv_str))
        elif isinstance(self.device, MMDevice):
            max_t, min_t = self.device.get_thermal_status()
            max_t_str, min_t_str = self.device.parse_thermal_status(max_t, min_t)
            while max_t > 0x00:
                self.log.info("Temperature is too high (Max T {}, Min T {}), waiting {} s to cool down...".format(
                    max_t_str, min_t_str, wait_time))
                time.sleep(wait_time)
                max_t, min_t = self.device.get_thermal_status()
                max_t_str, min_t_str = self.device.parse_thermal_status(max_t, min_t)
            self.log.info("Temperature is ok (Max T {}, Min T {})".format(max_t_str, min_t_str))
        else:
            sleep_time = 60
            self.log.info("Thermal cool down not supported outside G3, waiting {} s...".format(sleep_time))
            time.sleep(sleep_time)

    def sflash_request(self, operation: SpiFlashOperation, address=0, data=None, size=0):
        return self.device.sflash_request(operation, address, data, size)
