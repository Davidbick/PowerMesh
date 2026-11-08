from typing import Union
import logging as logger
import inspect


class AbstractDevice:
    def __init__(self, protocol, lg=logger):

        self.protocol = protocol
        self.log = lg
        self.protocol_if = None
        self.init_fail = True

    def close(self):
        self.not_implemented(inspect.stack()[0][3])

    def wait_startup(self):
        return self.not_implemented(inspect.stack()[0][3])
    
    def reset(self, confirm_expected):
        return self.not_implemented(inspect.stack()[0][3])

    def get_baudrate(self):
        return self.not_implemented(inspect.stack()[0][3])

    def set_baudrate(self, baudrate):
        return self.not_implemented(inspect.stack()[0][3])

    def change_baudrate(self, baudrate):
        return self.not_implemented(inspect.stack()[0][3])

    def info(self):
        return self.protocol_if.info()

    def version(self):
        return self.protocol_if.version()

    def check_protocol(self):
        return self.not_implemented(inspect.stack()[0][3])

    def set_configuration(self, band):
        return self.not_implemented(inspect.stack()[0][3])

    def set_phy_mode(self, channel_mode):
        return self.not_implemented(inspect.stack()[0][3])

    def set_channel(self, channel):
        return self.not_implemented(inspect.stack()[0][3])

    def send_phy_data(self, array, level, modulation):
        return self.not_implemented(inspect.stack()[0][3])

    def wait_phy_data(self, timeout=1):
        return self.not_implemented(inspect.stack()[0][3])

    def get_phy_data(self):
        return self.not_implemented(inspect.stack()[0][3])

    def set_ld_vcc(self, value):
        return self.not_implemented(inspect.stack()[0][3])

    def get_ld_vcc(self):
        return self.not_implemented(inspect.stack()[0][3])

    def set_mol_offset(self, value):
        return self.not_implemented(inspect.stack()[0][3])

    def get_mol_offset(self):
        return self.not_implemented(inspect.stack()[0][3])

    def get_thermal_status(self):
        return self.not_implemented(inspect.stack()[0][3])

    def get_thermal_counter(self):
        return self.not_implemented(inspect.stack()[0][3])

    def set_start_rx(self, start):
        return self.not_implemented(inspect.stack()[0][3])

    def set_start_tx(self, test_value):
        return self.not_implemented(inspect.stack()[0][3])

    def reset_stats(self):
        return self.not_implemented(inspect.stack()[0][3])

    def get_stats_rx(self):
        return self.not_implemented(inspect.stack()[0][3])

    def get_stats_tx(self):
        return self.not_implemented(inspect.stack()[0][3])

    def thermal_cool_down(self, wait_time):
        return self.not_implemented(inspect.stack()[0][3])

    def sflash_request(self, operation, address=0, data=None, size=0) -> Union[bool, list]:
        self.not_implemented(inspect.stack()[0][3])
        return False

    def not_implemented(self, f_name):
        self.log.warning("{} not implemented in {}".format(f_name, self.protocol.name))
        return None
