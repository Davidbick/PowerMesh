import logging
import os
import sys
from serial import SerialException
from global_defs import Protocol, NodeMode, ChannelMode, G3ModScheme, TestResult, St8500Defs, SpiFlashOperation
from abstract_device import AbstractDevice

# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\g3_validation_framework\\v6_X\\common")

import g3_validation_framework.v6_X.common.g3_common as g3_com  # noqa: E402
import g3_validation_framework.v6_X.common.g3_network as g3_net  # noqa: E402


class G3Device(AbstractDevice):
    band_plan = None
    default_baudrate = 115200
    default_timeout = 1
    hw_reset_cnf_timeout = 3

    def __init__(self, port, baudrate=None, lg=logging, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):
        super().__init__(protocol=Protocol.G3, lg=lg)

        self.array_s = []
        self.array_n = []
        self.array_snr = []

        if baudrate is None:
            baudrate = self.default_baudrate

        self.protocol_if = g3_net.network_info_ex(port_name=port, baudrate=baudrate)

        try:
            self.protocol_if.open()
            self.init_fail = False
        except SerialException:
            self.log.error("Warning, device port was already open")

        # Get logger
        self.log_prot = logging.getLogger(g3_net.__name__)

        # Disable propagation
        self.log_prot.propagate = False

        # Only first time, to avoid duplicate handlers
        if len(self.log_prot.handlers) == 0:
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

        self.log_prot.info("Initialize the G3 interface with {}".format(port))

    def __str__(self):
        return "G3 Device"

    def __del__(self):
        self.close()

    def close(self):
        if self.protocol_if.ser.is_open:
            self.protocol_if.close()

    def wait_startup(self):
        success = False
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.hw_reset_cnf_timeout)
        if conf is not None:
            if conf.header.cmd_id == g3_net.g3_hi_cmd_ids.HWRESET_CONF:
                success = True
            else:
                self.log_prot.error("Received {} instead of {}".format(conf.header.cmd_id,
                                                                       g3_net.g3_hi_cmd_ids.HWRESET_CONF))
        else:
            self.log_prot.error("No HW-Reset confirm")

        return success

    def reset(self, confirm_expected):
        req = g3_net.g3_hi_msg_hwreset_req()
        g3_net.g3_hi_msg_send_message(self.protocol_if, req)

        self.set_baudrate(self.default_baudrate)

        if confirm_expected:
            ret = self.wait_startup()
        else:
            ret = True

        return ret

    def get_baudrate(self):
        """
        Returns the current baudrate in use in BOOT
        :return: The current used baudrate
        """
        return self.protocol_if.ser.baudrate

    def set_baudrate(self, baudrate):
        """
        Changes the baudrate to use in BOOT
        :param baudrate: Baudrate value to use
        :return: Nothing
        """
        self.protocol_if.ser.baudrate = baudrate

    def change_baudrate(self, baudrate):
        """
        Sets the baudrate to use in G3
        :param baudrate: Baudrate value to use
        :return: Result of the operation
        """

        old_baudrate = self.get_baudrate()
        self.log_prot.info("Configuring COM port (baudrate {} -> {} bps)".format(old_baudrate, baudrate))
        baudrate_config = g3_net.g3_hi_msg_hwconfig_baudrateconfig_req(baudrate)
        baudrate_config_req = g3_net.g3_hi_msg_hwconfig_req(g3_net.g3_hi_hwconfig_type.BAUDRATE_CONFIG, baudrate_config)
        g3_net.g3_hi_msg_send_message(self.protocol_if, baudrate_config_req)
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)
        if conf is not None:
            success = True
            self.set_baudrate(baudrate)
        else:
            success = False
            self.log_prot.error("Failed to set baudrate")

        return success

    def info(self):
        dbg_tool_req = g3_net.g3_hi_msg_dbgtool_req(g3_net.g3_hi_dbgtool_ctrl.HI_DBG_INFO, 0)
        g3_net.g3_hi_msg_send_message(self.protocol_if, dbg_tool_req)
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)
        if conf is None:
            self.log_prot.error("No DBGTOOL_CONF received")
            dbg_string = TestResult.error.value
        elif conf.header.cmd_id != g3_net.g3_hi_cmd_ids.DBGTOOL_CONF:
            self.log_prot.error("Received {} instead of DBGTOOL_CONF".format(conf.header.cmd_id))
            dbg_string = TestResult.error.value
        else:
            fw_version = "%d.%d.%d" % (conf.fwkversion_major, conf.fwkversion_minor, conf.fwkversion_patch)
            lib_version = '{0:08X}'.format(conf.lib_version)
            rte_version = '{0:08X}'.format(conf.rte_version)

            # Need to swap the MAC address array
            mac_addr = []
            for x in conf.mac_addr.addr:
                mac_addr.insert(0, x)

            dbg_string = ":".join('{0:02X}'.format(x) for x in mac_addr) + ", "
            dbg_string += g3_net.g3_hi_platform(conf.platform).name + ", "
            dbg_string += g3_net.g3_hi_device_type(conf.devtype).name + ", "
            dbg_string += g3_net.g3_hi_mode(conf.plc_mode).name + ", "
            dbg_string += g3_net.g3_hi_band_plan(conf.bandplan).name + ", "
            dbg_string += "PE: " + fw_version + ", G3LIB: " + lib_version + ", RTE: " + rte_version

        return dbg_string

    def version(self):
        version_pe = None
        version_rte = None

        dbg_tool_req = g3_net.g3_hi_msg_dbgtool_req(g3_net.g3_hi_dbgtool_ctrl.HI_DBG_INFO, 0)
        g3_net.g3_hi_msg_send_message(self.protocol_if, dbg_tool_req)
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)
        if conf is None:
            self.log_prot.error("No DBGTOOL_CONF received")
        elif conf.header.cmd_id != g3_net.g3_hi_cmd_ids.DBGTOOL_CONF:
            self.log_prot.error("Received {} instead of DBGTOOL_CONF".format(conf.header.cmd_id))
        else:
            version_pe = "%d.%d.%d" % (conf.fwkversion_major, conf.fwkversion_minor, conf.fwkversion_patch)
            major = (conf.rte_version >> 16) & 0xFF
            minor = (conf.rte_version >> 8) & 0xFF
            rc = (conf.rte_version & 0xFF) >> 4
            version_rte = "{}.{}rc{}".format(major, minor, rc)

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

    def set_configuration(self, band_plan=g3_net.g3_hi_band_plan.CEN_A,
                          channel_mode=g3_net.g3_hi_channel_config.CHANNEL_ALL):
        # SW resets the board (returns False if it fails)
        self.init_fail = not g3_com.g3_hi_node_clear(self.protocol_if, self.band_plan, channel=channel_mode)
        self.band_plan = band_plan

    def set_role(self, role: NodeMode):
        if role == NodeMode.Transmitter:
            role = g3_net.g3_hi_phy_trx_state.PHY_STATE_TRX
        else:
            role = g3_net.g3_hi_phy_trx_state.PHY_STATE_RX

        # Flush data indications
        while g3_net.g3_hi_msg_receive_message(self.protocol_if, to=0) is not None:
            pass

        # Configure device role...if RX or TX since in G3 must be configured...
        phy_role_req = g3_net.g3_hi_msg_phy_set_trx_state_req(role)

        g3_net.g3_hi_msg_send_message(self.protocol_if, phy_role_req)
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)
        if conf is None:
            self.log_prot.error("Failed to configure role to: " + str(g3_net.g3_hi_phy_trx_state(role).name) +
                                " (No Ack)")
            raise Exception("Failed to configure role (No Ack)")
        elif conf.header.cmd_id != g3_net.g3_hi_cmd_ids.G3PHY_SETTRXSTATE_CONF:
            msg_name = g3_net.g3_hi_cmd_ids(conf.header.cmd_id).name
            self.log_prot.error("Failed to set device role, received: {}".format(msg_name))
            raise Exception("Failed to set device role")
        elif conf.result != g3_net.g3_hi_results.SUCCESS:
            self.log_prot.error("Failed to set device role, status: {}".format(g3_net.g3_hi_results(conf.result).name))
            raise Exception("Failed to set device role")
        else:
            self.log_prot.info("Configured in {}".format(role.name))
            success = True

        return success

    def set_channel(self, channel: ChannelMode):

        if channel == ChannelMode.PLC_RF:
            channel = g3_net.g3_hi_channel_config.CHANNEL_ALL
        elif channel == ChannelMode.PLC:
            channel = g3_net.g3_hi_channel_config.CHANNEL_PLC_ONLY
        elif channel == ChannelMode.RF:
            channel = g3_net.g3_hi_channel_config.CHANNEL_RF_ONLY
        else:
            channel = g3_net.g3_hi_channel_config.CHANNEL_NONE

        dev_type = g3_net.g3_hi_device_type.DEVICE
        mode = g3_net.g3_hi_mode.MODE_PHY

        success = True

        # Trigger SW reset
        if g3_com.g3_hi_node_clear(net_info_ex=self.protocol_if, band_plan=self.band_plan, device_type=dev_type,
                                   device_mode=mode, pan_id=None, short_addr=None, channel=channel):
            self.log.info("Configured in {}, {}".format(mode.name, channel.name))

        if channel == g3_net.g3_hi_channel_config.CHANNEL_PLC_ONLY:
            rf_disable_value = 1
        else:
            rf_disable_value = 0

        ret = False
        retry_max = 10
        retry = 0
        while not ret:
            ret = g3_com.g3_hi_set_attr(self.protocol_if, g3_net.g3_hi_attrib_id.RF_DISABLEPHY, 0, 1, rf_disable_value)
            if not ret:
                retry += 1
                print("Retrying to set RF_DISABLEPHY ({})...".format(retry))
                if retry >= retry_max:
                    break

        if not ret:
            success = False
            if channel == g3_net.g3_hi_channel_config.CHANNEL_PLC_ONLY:
                print('Not able to disable RF')
            else:
                print('Not able to enable RF')

        return success

    @staticmethod
    def parse_modulation_scheme(modulation):
        if modulation == G3ModScheme.G3MOD_ROBO_DIFFERENTIAL:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO
            scheme = g3_net.g3_hi_mod_scheme.DIFFERENTIAL
        elif modulation == G3ModScheme.G3MOD_DBPSK_DIFFERENTIAL:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK
            scheme = g3_net.g3_hi_mod_scheme.DIFFERENTIAL
        elif modulation == G3ModScheme.G3MOD_DQPSK_DIFFERENTIAL:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK
            scheme = g3_net.g3_hi_mod_scheme.DIFFERENTIAL
        elif modulation == G3ModScheme.G3MOD_D8PSK_DIFFERENTIAL:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK
            scheme = g3_net.g3_hi_mod_scheme.DIFFERENTIAL
        elif modulation == G3ModScheme.G3MOD_ROBO_COHERENT:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO
            scheme = g3_net.g3_hi_mod_scheme.COHERENT
        elif modulation == G3ModScheme.G3MOD_DBPSK_COHERENT:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK
            scheme = g3_net.g3_hi_mod_scheme.COHERENT
        elif modulation == G3ModScheme.G3MOD_DQPSK_COHERENT:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK
            scheme = g3_net.g3_hi_mod_scheme.COHERENT
        elif modulation == G3ModScheme.G3MOD_D8PSK_COHERENT:
            mod = g3_net.g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK
            scheme = g3_net.g3_hi_mod_scheme.COHERENT
        else:
            raise Exception("Invalid modulation value")

        return mod, scheme

    def send_phy_data(self, array, level=0x20, modulation=G3ModScheme.G3MOD_ROBO_DIFFERENTIAL):
        # Flush Serial UART
        two_rs_blocks = g3_net.g3_hi_two_rs_blocks.ONE_RS_BLOCK
        self.protocol_if.ser.reset_input_buffer()

        if self.band_plan == g3_net.g3_hi_band_plan.CEN_A:
            tone_map = g3_net.g3_hi_vec_to_cbyte_vec([0x3F, 0x00, 0x00])
        elif self.band_plan == g3_net.g3_hi_band_plan.CEN_B:
            tone_map = g3_net.g3_hi_vec_to_cbyte_vec([0x0F, 0x00, 0x00])
        else:
            tone_map = g3_net.g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF])
            if len(array) > 128:
                two_rs_blocks = g3_net.g3_hi_two_rs_blocks.TWO_RS_BLOCKS

        tx_coefficient = g3_net.g3_hi_vec_to_cbyte_vec([0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        psdu_vec = g3_net.g3_hi_vec_to_cbyte_vec(array)

        mod, scheme = self.parse_modulation_scheme(modulation)

        phy_data_req = g3_net.g3_hi_msg_g3phy_data_req(level, mod, scheme, tx_coefficient, tone_map,
                                                       g3_net.g3_hi_phy_dt.NO_RESP, two_rs_blocks, len(psdu_vec),
                                                       psdu_vec)
        g3_net.g3_hi_msg_send_message(self.protocol_if, phy_data_req)

        success = False

        msg_id = 0

        while msg_id != g3_net.g3_hi_cmd_ids.G3PHY_DATA_CONF:
            msg = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)

            if msg is None:
                self.log_prot.error("PHY_SendData: missing CNF")
                break
            else:
                msg_id = msg.header.cmd_id

                if msg_id == g3_net.g3_hi_cmd_ids.G3PHY_DATA_CONF:
                    if msg.status == g3_net.g3_hi_results.SUCCESS:
                        success = True
                    else:
                        self.log_prot.error("PHY_SendData: CNF error: " + g3_net.g3_hi_results(msg.status).name)

                    if msg.thermal_warning != 0:
                        self.log_prot.error("PHY_SendData: thermal warning: {}".format(msg.thermal_warning))
                    break
                else:
                    wrong_msg_name = g3_net.g3_hi_cmd_ids(msg_id).name
                    self.log_prot.warning("PHY_SendData: received {} instead of G3PHY_DATA_CONF".format(wrong_msg_name))
                    # Do not break

        return success

    def receive_phy_data_indication(self, timeout):
        data = []
        mod_scheme = 0
        snr = 0.0
        signal = 0.0

        msg_id = 0

        while msg_id != g3_net.g3_hi_cmd_ids.G3PHY_DATA_IND:
            msg = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=timeout)

            if msg is None:
                break
            else:
                msg_id = msg.header.cmd_id

                if msg_id == g3_net.g3_hi_cmd_ids.G3PHY_DATA_IND:

                    mod_scheme = msg.mod_scheme

                    self.array_s = list(msg.signal_power)
                    self.array_n = list(msg.noise_power)
                    self.array_snr = list(msg.snr_carriers)
                    data = list(msg.payload)

                    if self.band_plan == g3_net.g3_hi_band_plan.CEN_A:
                        carrier_n = 36
                    elif self.band_plan == g3_net.g3_hi_band_plan.CEN_B:
                        carrier_n = 16
                    else:
                        carrier_n = 72

                    for i in range(0, carrier_n, 1):
                        signal += self.array_s[i]

                    signal /= carrier_n

                    # SNR = msg.lqi/4 - 10, RSSI = signal
                    snr = msg.lqi / 4 - 10

                    break
                else:
                    self.log_prot.warning("Received {} instead of G3PHY_DATA_IND, retrying...".format(
                        g3_net.g3_hi_cmd_ids(msg_id).name))
                    # Do not break

        return data, 0, mod_scheme, 0, snr, signal

    def wait_phy_data(self, timeout=1):
        return self.receive_phy_data_indication(timeout=timeout)

    def get_phy_data(self):
        return self.receive_phy_data_indication(timeout=0)

    def get_sn_array(self):
        return self.array_s, self.array_n, self.array_snr

    def set_ld_vcc(self, value):
        success = False

        if value in St8500Defs.ld_vcc_accepted_values:
            attrib_req = g3_net.g3_hi_msg_get_g3lib_set_attrib_req(g3_net.g3_hi_attrib_id.PHY_LD_VCC, 0, 1, value)
            g3_net.g3_hi_msg_send_message(self.protocol_if, attrib_req)
            conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=self.default_timeout)

            if conf.state == g3_net.g3_hi_results.SUCCESS:
                success = True
            else:
                self.log_prot.error("{}: CNF error: {}".format(self.set_ld_vcc.__name__,
                                                               g3_net.g3_hi_results(conf.state).name))
        else:
            self.log_prot.error("{}: {} is not a valid value (use 10, 12, 15 or 18)".format(self.set_ld_vcc.__name__,
                                                                                            value))

        return success

    def get_ld_vcc(self):
        return g3_com.g3_hi_get_attr(self.protocol_if, g3_net.g3_hi_attrib_id.PHY_LD_VCC, 0)

    def get_thermal_status(self):
        status = g3_com.g3_hi_get_attr(self.protocol_if, g3_net.g3_hi_attrib_id.PHY_THERMAL_STATUS, 0)
        return self.parse_thermal_status(status)

    @staticmethod
    def parse_thermal_status(thermal_status: int):
        line_driver = thermal_status & 0x0F
        low_voltage = (thermal_status >> 8) & 0x0F

        if line_driver == 0:
            line_driver_str = "T < 70 °C"
        elif line_driver == 1:
            line_driver_str = "T < 100 °C"
        elif line_driver == 2:
            line_driver_str = "T < 125 °C"
        elif line_driver == 3:
            line_driver_str = "T < 170 °C"
        elif line_driver == 4:
            line_driver_str = "T >= 170 °C"
        else:
            line_driver_str = "invalid ({})".format(line_driver)

        if low_voltage == 0:
            low_voltage_str = "T < 100 °C"
        elif low_voltage == 1:
            low_voltage_str = "T < 115 °C"
        elif low_voltage == 2:
            low_voltage_str = "T < 125 °C"
        elif low_voltage == 3:
            low_voltage_str = "T < 135 °C"
        elif low_voltage == 4:
            low_voltage_str = "T >= 135 °C"
        else:
            low_voltage_str = "invalid ({})".format(low_voltage)

        return thermal_status, line_driver_str, low_voltage_str

    def sflash_request(self, operation: SpiFlashOperation, address=0, data=None, size=0):

        success = False

        if data is None:
            data = []

        if operation == SpiFlashOperation.erase:
            g3_operation = g3_net.g3_hi_sflash_operation.ERASE
            payload = g3_net.c_ubyte(0)
        elif operation == SpiFlashOperation.read:
            g3_operation = g3_net.g3_hi_sflash_operation.READ
            payload = g3_net.c_ubyte(0)
        elif operation == SpiFlashOperation.write:
            g3_operation = g3_net.g3_hi_sflash_operation.WRITE
            size = len(data)
            payload = (g3_net.c_ubyte * size)()
            for i in range(0, size):
                payload[i] = g3_net.c_ubyte(data[i])
        else:
            raise Exception("Invalid operation")

        sflash_req = g3_net.g3_hi_msg_get_sflash_req(g3_operation, address, size, payload)
        g3_net.g3_hi_msg_send_message(self.protocol_if, sflash_req)
        conf = g3_net.g3_hi_msg_receive_message(self.protocol_if, to=5)

        if conf is not None:
            success = True
        else:
            self.log_prot.error("{}: missing CNF, address={}, data={}".format(self.sflash_request.__name__, address,
                                                                              data))
        if g3_operation == g3_net.g3_hi_sflash_operation.READ:
            if success:
                data_read = conf.payload
            else:
                data_read = []

            return data_read
        else:
            return success
