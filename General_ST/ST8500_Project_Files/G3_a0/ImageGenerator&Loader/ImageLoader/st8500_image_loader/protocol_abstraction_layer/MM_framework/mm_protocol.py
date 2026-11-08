import sys
import time
import math
import logging
from typing import Union
from bytes_op import ByteOp
from mm_host_if import StPlatform, MMHostIf
from mm_defs import MMCmdID, SpiFlashOp, ErrorCode
from mm_defs import RequestType, Channel, Encryption, Mib, Modulations, MacFrameType, NetworkManagement
from mm_defs import CsmaTimeout, CsmaSlotSize, CsmaSlotNumber, BiuObservationWindow


# This class implements MM Interface
class MMinterface:
    host_if = None
    platform = None

    address_len = 6  # Length of the ACA address

    def __init__(self, port_name, platform: StPlatform, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):
        """
        Changes the baudrate to use in BOOT
        :param port_name: The name of the COM port
        :param platform: Platform type
        :param log_level_prot: Logging level for the protocol log
        :param log_level_if: Logging level for the interface log
        """
        # Get logger
        self.log = logging.getLogger(__name__)

        # Disable propagation
        self.log.propagate = False

        # Only first time, to avoid duplicate handlers
        if len(self.log.handlers) == 0:
            # Define logging format
            logger_fmt = "%(asctime)s %(levelname)s %(filename)s - %(funcName)s(): %(message)s"
            logger_datefmt = "%H:%M:%S"
            logger_formatter = logging.Formatter(fmt=logger_fmt, datefmt=logger_datefmt)

            # Console Logger handler
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setFormatter(logger_formatter)

            # Use defined handler
            self.log.addHandler(console_handler)

        # Set logger level
        self.log.setLevel(log_level_prot)

        self.log.info("Initialize the MM interface with {}".format(port_name))

        self.last_request_id = 0
        self.master_request_id = 0
        self.platform = platform

        self.host_if = MMHostIf(port_name=port_name, platform=self.platform, log_level_if=log_level_if)

    def close(self):
        """
        Closes the serial communication
        """
        self.host_if.close()

    def wait_startup(self):
        """
        Waits for the platform startup
        :return: Result of the operation
        """
        success, _, _ = self.receive_confirm(MMCmdID.HWRESET_request_ID, timeout=4)

        return success

    def reset(self, confirm_expected) -> bool:
        """
        Resets the platform
        :param confirm_expected: If set to True, waits for the platform startup
        :return: Result of the operation
        """
        self.log.info("HW-RESET Request")
        success, _, _ = self.request_confirm(MMCmdID.HWRESET_request_ID, expect_cnf=False)
        if success and confirm_expected:
            success = self.wait_startup()

        return success

    def get_baudrate(self):
        """
        Returns the current baudrate in use in BOOT
        :return: The current used baudrate
        """
        return self.host_if.get_baudrate()

    def info(self):
        """
        Returns the platform information
        :return: The platform information
        """
        return self.configuration_info()

    def version(self):
        """
        Returns the FW version
        :return: The FW version
        """
        return self.fw_version()

    def request_confirm(self, request_id: MMCmdID, tx_payload=None, expect_cnf=True):  # -> tuple[bool, int, list]
        """
        Sends a request to the platform, through the host interface, and then receives the confirmation
        :param request_id: ID of the request to send
        :param tx_payload: Payload of the request to send
        :param expect_cnf: If set to True, waits for the answer (confirm)
        :return: Tuple with the result of the operation, the length of the answer (in bytes) and its payload
        """

        if tx_payload is None:
            tx_payload = [0]

        success = False
        rx_payload_len = 0
        rx_payload = []

        # Prepare command frame
        command = self.host_if.prepare_command(request_id, tx_payload)
        self.log.info("Request: {}, command: {}".format(request_id.name, command))
        # Send the message
        send_ok = self.host_if.send_command(command)
        if send_ok:
            # Wait for confirm
            if expect_cnf:
                success, rx_payload_len, rx_payload = self.receive_confirm(request_id)
            else:
                success = True

        # In any other case we assume request not successfully handled
        return success, rx_payload_len, rx_payload

    def receive_confirm(self, request_id: MMCmdID, timeout=0.5):
        """
        Sends a request to the platform, through the host interface, and then receives the answer (confirm)
        :param request_id: ID of the request the confirmation is related to
        :param timeout: Timeout for the reception of the confirmation
        :return: Tuple with the result of the operation, the length of the answer (in bytes) and its payload
        """
        success = False
        # Wait for confirm
        cnf_id, rx_payload_len, rx_payload = self.host_if.wait_confirm(tout=timeout)
        if cnf_id is not None:
            cnf_id = MMCmdID(cnf_id)
            if cnf_id.value == (request_id.value + 1):  # the positive confirm ID is always the request ID + 1
                success = True
                self.log.info("{}: positive CNF".format(cnf_id.name))
            elif cnf_id.value == (request_id.value + 3):  # the negative confirm ID is always the request ID + 3
                self.log.error("{}: negative CNF".format(cnf_id.name))
            else:
                self.log.error("Unexpected message: {}".format(MMCmdID(cnf_id).name))
        else:
            self.log.error("No confirm to {}".format(request_id.name))

        return success, rx_payload_len, rx_payload

    def sflash_request(self, operation: SpiFlashOp, address=0, data=None, size=0) -> Union[bool, list]:
        """
        Requests an SPI Flash operation to the platform
        :param operation: Type of SPI Flash operation (read/write/erase)
        :param address: Address for the SPI Flash operation
        :param data: Data to write (in case of write operation)
        :param size: Number of bytes to read or write
        :return: Result of the operation or data read
        """
        if data is None:
            data = [0]

        if operation == SpiFlashOp.write:
            size = len(data)

        payload = [operation.value]
        payload.extend(ByteOp.uint16_to_bytes(size))
        payload.extend(ByteOp.uint32_to_bytes(address))
        payload.extend(data)

        success = False
        data_read = None

        for retry in range(0, 3):
            success, _, data_read = self.request_confirm(MMCmdID.SFLASH_request_ID, payload)

            if success:
                break
            else:
                wait = 5
                self.log.warning("Retrying after {} s...".format(wait))
                time.sleep(wait)

        if operation == SpiFlashOp.read:
            return data_read
        else:
            return success

    def bio_reset(self):
        """
        Sends a BIO reset to the board
        :return: Result of the operation
        """
        self.log.info("BIO-RESET Request")
        success, _, payload_rx = self.request_confirm(MMCmdID.BIO_RESET_request_ID)

        if success:
            # sanity sleep to wait the indications from RTE
            time.sleep(0.5)
        elif payload_rx is not None:
            if len(payload_rx) != 0:
                err_code = ErrorCode(payload_rx[0])
                self.log.warning("BIO-RESET Negative Confirm, code: " + str(err_code))

        return success

    def set_mib_value(self, mib_object: Mib.Object):
        """
        Sets a MIB object on the device
        :param mib_object: MIB object to set (contains index and value)
        :return: Result of the operation
        """
        # Prepare payload
        payload = [mib_object.index]
        payload.extend(mib_object.content)
        self.log.info("Set MIB {} ({})".format(mib_object.index, Mib.Index(mib_object.index).name))

        success, _, _ = self.request_confirm(MMCmdID.MIB_WRITE_request_ID, payload)

        return success

    def get_mib_value(self, mib_index: Mib.Index) -> Union[Mib.Object, None]:
        """
        Gets a MIB object from the device
        :param mib_index: Index of the MIB object
        :return: The MIB object (None in case of failure)
        """
        # Prepare payload
        payload = [mib_index.value]

        self.log.info("Get MIB {} ({})".format(mib_index.value, mib_index.name))

        success, _, value_read = self.request_confirm(MMCmdID.MIB_READ_request_ID, payload)

        mib_object_read = None

        if success:
            mib_object_read = Mib.Object(index=mib_index.value, content=value_read)

        return mib_object_read

    def set_mib_value_with_check(self, mib_object: Mib.Object):
        """
        Sets a MIB object on the device, checking it afterward
        :param mib_object: MIB object to set (contains index and value)
        :return: Result of the operation
        """
        success = False
        mib_index = Mib.Index(mib_object.index)

        ret = self.set_mib_value(mib_object)
        if ret:
            # Read back MIB objects and verify the content
            mib_object_read = self.get_mib_value(mib_index)
            if mib_object_read is not None:
                if mib_object_read.content == mib_object.content:
                    success = True
                else:
                    # Error while reading back MIB
                    self.log.error("ERROR: MIB {} has a wrong value: {} instead of {}\n".format(mib_index,
                                                                                                mib_object_read.content,
                                                                                                mib_object.content))
            else:
                self.log.error("ERROR: could not read MIB\n")
        else:
            # Error while trying to configure MIB
            self.log.error("ERROR: while trying to configure MIB {}\n".format(mib_index))

        return success

    def configuration_info(self):
        """
        Reads the main MIB related to the configuration of the device
        :return: A string describing the configuration of the device
        """
        phy_configuration = self.get_mib_value(Mib.Index.phy_configuration)
        mac_configuration = self.get_mib_value(Mib.Index.mac_configuration)
        csma_configuration = self.get_mib_value(Mib.Index.csma_configuration)
        biu_calibration = self.get_mib_value(Mib.Index.biu_calibration)
        manufacturer_data = self.get_mib_value(Mib.Index.manufacturer_data)
        custom_freq = self.get_mib_value(Mib.Index.custom_freq)

        phy_configuration_hex = ":".join('{0:02X}'.format(x) for x in phy_configuration.content)
        mac_configuration_hex = ":".join('{0:02X}'.format(x) for x in mac_configuration.content)
        csma_configuration_hex = ":".join('{0:02X}'.format(x) for x in csma_configuration.content)
        biu_calibration_hex = ":".join('{0:02X}'.format(x) for x in biu_calibration.content)
        manufacturer_data_hex = ":".join('{0:02X}'.format(x) for x in manufacturer_data.content)
        custom_freq_hex = ":".join('{0:02X}'.format(x) for x in custom_freq.content)

        dbg_string = "PHY CONF: {}, ".format(phy_configuration_hex)
        dbg_string += "MAC CONF: {}, ".format(mac_configuration_hex)
        dbg_string += "CSMA CONF: {}, ".format(csma_configuration_hex)
        dbg_string += "BIU CALIB: {}, ".format(biu_calibration_hex)
        dbg_string += "MANU DATA: {}, ".format(manufacturer_data_hex)
        dbg_string += "FREQUENCIES: {}".format(custom_freq_hex)

        return dbg_string

    def fw_version(self):
        """
        Reads the FW version
        :return: The PE version and the RTE version (ST8500). The FW release and None (ST75MM)
        """
        version_1 = None
        version_2 = None

        if self.platform == StPlatform.ST75MM:
            fw_release = self.get_mib_value(Mib.Index.fw_release)

            if fw_release is not None:
                version_1 = "{}".format(bytearray(fw_release.content).hex())
        elif self.platform == StPlatform.ST8500:
            pe_fw_version = self.get_mib_value(Mib.Index.pe_fw_version)

            rte_fw_version = self.get_mib_value(Mib.Index.rte_fw_version)

            if pe_fw_version is not None:
                version_1 = "{}.{}.{}".format(pe_fw_version.content[2], pe_fw_version.content[1],
                                              pe_fw_version.content[0])

            if rte_fw_version is not None:
                version_2 = ByteOp.bytes_to_uint16(rte_fw_version.content[2:4], little_endian=True)
        else:
            self.log.critical("Unknown platform")

        return version_1, version_2

    def get_temperature(self):
        """
        Reads the device temperature
        :return: The max temperature, the minimum temperature
        """
        phy_data = self.get_mib_value(Mib.Index.phy_data)

        if len(phy_data.content) == 18:
            max_temperature = phy_data.content[7]
            min_temperature = phy_data.content[8]
        else:
            max_temperature = None
            min_temperature = None

        return max_temperature, min_temperature

    def default_configuration(self, address, tx_enable=True):
        """
        Configures the MM device with the default MIB values
        :param address: the ACA address for the device
        :param tx_enable: Enables the transmission on the device
        :return: The success of the operation
        """
        phy_configuration = Mib.mib_phy_configuration(self.platform, biu_enable=1, csma_enable=1)

        if self.platform == StPlatform.ST8500:
            mac_configuration = Mib.mib_mac_configuration_st8500(rx_mode_ihd=1, rx_mode_ami=1, tx_mode=tx_enable)
        else:
            mac_configuration = Mib.mib_mac_configuration_st75mm(rx_mode=1, tx_mode=tx_enable)

        csma_configuration = Mib.mib_csma_configuration()
        biu_calibration = Mib.mib_biu_calibration()
        manufacturer_data = Mib.mib_manufacturer_data(aca=address)
        custom_freq = Mib.mib_custom_freq(self.platform)

        success = True

        success &= self.set_mib_value_with_check(phy_configuration)
        success &= self.set_mib_value_with_check(mac_configuration)
        success &= self.set_mib_value_with_check(csma_configuration)
        success &= self.set_mib_value_with_check(biu_calibration)
        success &= self.set_mib_value_with_check(manufacturer_data)
        success &= self.set_mib_value_with_check(custom_freq)

        return success

    def set_level_and_modulation(self, tx_gain=21, tx_modulation=Modulations.BPSK_COD):
        """
        Configures the TX gain and the modulation
        :param tx_gain: The TX gain to set
        :param tx_modulation: The modulation to set
        :return: The success of the operation
        """
        self.log.info("TX level {} dB, TX modulation: {}".format(tx_gain, tx_modulation.name))

        mib_obj = self.get_mib_value(Mib.Index.phy_configuration)

        byte_1 = (tx_gain << 0) | (tx_modulation.value << 5)

        mib_obj.content[1] = byte_1

        return self.set_mib_value_with_check(mib_obj)

    def set_custom_frequencies(self, high_freq=132500, low_freq=110000):
        """
        Configures the working frequencies of the device
        :param high_freq: High Frequency
        :param low_freq: Low frequency
        :return: The success of the operation
        """
        self.log.info("High frequency: {} Hz, low frequency: {} Hz".format(high_freq, low_freq))

        custom_freq = Mib.mib_custom_freq(self.platform, high_freq, low_freq)

        return self.set_mib_value_with_check(custom_freq)

    def set_csma_biu(self, timeout=CsmaTimeout.seconds_3, slot_size=CsmaSlotSize.milliseconds_5,
                     slot_number=CsmaSlotNumber.slots_7, observation_window=BiuObservationWindow.refresh_4ms,
                     gain_compensation=0):
        """
        Configures the CSMA and the BiU of the device
        :param timeout: CSMA timeout
        :param slot_size: Size of each CSMA slot
        :param slot_number: Number of CSMA slots
        :param observation_window: Bans in Use observation window
        :param gain_compensation: Gain compensation
        :return: The success of the operation
        """
        self.log.info("CSMA timeout: {}, slot size: {}, slot number: {}".format(timeout, slot_size, slot_number))
        self.log.info("Observation window: {}, gain compensation: {}".format(observation_window, gain_compensation))

        csma_configuration = Mib.mib_csma_configuration(timeout=timeout, slot_size=slot_size, slot_number=slot_number)
        biu_calibration = Mib.mib_biu_calibration(observation_window=observation_window,
                                                  gain_compensation=gain_compensation)

        success = True

        success &= self.set_mib_value_with_check(csma_configuration)
        success &= self.set_mib_value_with_check(biu_calibration)

        return success

    def raw_data(self, channel=Channel.ihd, req_id=0x01, payload=None):
        """
        Sends a raw data request
        :param channel: channel to use for the transmission (relevant only for ST8500)
        :param req_id: ID of the request
        :param payload: Payload of the request
        :return: The success of the operation
        """
        result = False

        if payload is None:
            payload = []

        self.log.info("Raw data Request")

        protocol_byte = RequestType.master | Encryption.cleartext

        if self.platform == StPlatform.ST8500:
            protocol_byte |= channel

        if len(payload) != 0 and req_id != 0:

            payload_tx = [protocol_byte.value] + [req_id] + payload

            success, _, payload_rx = self.request_confirm(MMCmdID.RAW_DATA_request_ID, payload_tx)

            if not success and payload_rx is not None:
                if len(payload_rx) > 0:
                    err_code = ErrorCode(payload_rx[0])
                    self.log.warning("Send raw data negative confirm, code: " + str(err_code))

        else:
            self.log.warning("Invalid input parameters")

        return result

    def hi_ping(self, payload=None):
        """
        Pings the device through the host interface, to verify it
        :param payload: Ping payload
        :return: Result of the operation
        """
        result = False

        if payload is None:
            payload = []

        self.log.info("HI PING Request")

        if len(payload) != 0:
            success, _, payload_echo = self.request_confirm(MMCmdID.HI_PING_request_ID, payload)

            if success:
                if payload_echo == payload:
                    result = True
                else:
                    self.log.error("Ping mismatch")
            else:
                self.log.error("Error while sending ping")
        else:
            self.log.error("Invalid input parameters")

        return result

    def build_mac_frame(self, destination_address: list, payload: list, request_answer: bool, repeater_list: list):
        """
        Builds a MAC frame according to CLC/TS 50568-4
        :param destination_address: Destination ACA address
        :param payload: Payload of the MAC frame
        :param request_answer: Indicates if an answer is expected
        :param repeater_list: List of repeaters for the frame
        :return: The MAC frame built
        """
        # MAC frame
        mac_frame = []

        # The LT field is filled at the end of this function

        # Destination address
        if len(destination_address) == MMinterface.address_len:
            mac_frame.extend(destination_address)
            self.log.debug("Destination: {}".format(destination_address))
        else:
            raise Exception("Wrong address length (must be 6 bytes)")

        # Control field
        if request_answer:
            # Assumed only RA
            discipline = 2 + math.floor((len(payload) - 2) / 20)  # 2 util 21 bytes, 3 until 41 bytes....
        else:
            if len(payload) == 0:
                discipline = 0x01
            else:
                discipline = 0x00

        self.log.debug("Discipline: {} ({} bytes)".format(discipline, len(payload)))

        if len(repeater_list) > 0:
            repetition = len(repeater_list) - 1  # Number of repetition is 1 + this field
        else:
            repetition = 0

        self.log.debug("Repetition: {} ({} repeaters)".format(repetition, len(repeater_list)))

        if repetition > 0:
            frame_type = MacFrameType.rip
        else:
            frame_type = MacFrameType.nor1

        self.log.debug("Frame type: {}".format(frame_type))

        if frame_type == MacFrameType.rip:
            mac_control = frame_type | (repetition << 1) | (discipline << 4)
        else:
            mac_control = frame_type | (discipline << 4)

        mac_frame.append(mac_control)

        self.log.debug("MAC control byte: 0x{:02X}".format(mac_control))

        # Repetition Parameters
        rp = []

        if frame_type == MacFrameType.rip:
            for repeater_address in repeater_list:
                if len(repeater_address) == MMinterface.address_len:
                    rp.extend(repeater_address)
                    self.log.debug("Repeater: ".format(repeater_address))
                else:
                    self.log.error("Wrong address length ({} instead of {} bytes)".format(len(repeater_address),
                                                                                          MMinterface.address_len))

        elif frame_type == MacFrameType.crp:
            pass  # TODO

        mac_frame.extend(rp)

        # NB = Number of B?
        nb = 0
        mac_frame.append(nb)

        # Information field (not for CRP frames)
        if frame_type != MacFrameType.crp:
            inf = []

            # Fixed 0x3 with P as bit 4
            llc_control = 0x03 | (request_answer << 4)

            # 0 = Upper layer, 1 = Network management
            network_management = 0

            llc_ectl = network_management & 0x01

            # Extend with Control and ECTL
            inf.extend([llc_control, llc_ectl])

            # Extend with payload data
            inf.extend(payload)

            self.log.debug("Request answer: ".format(request_answer))
            self.log.debug("Network management: ".format(network_management))
            self.log.debug("LLC control byte: 0x{:02X}".format(llc_control))
            self.log.debug("LLC ECTL byte: 0x{:02X}".format(llc_ectl))

            mac_frame.extend(inf)

        # Frame checking sequence:
        # polynomial CRC32 calculated on the bytes between LT and INF (included)
        # Calculated inside

        # Calculate and insert MAC frame length
        mac_lt = len(mac_frame)
        mac_frame.insert(0, mac_lt)

        self.log.debug("MAC LT: {}".format(mac_lt))

        return mac_frame

    def dl_data_master_request(self, destination_address: list, payload: list, channel=Channel.ihd, request_answer=True,
                               repeater_list=None):
        """
        Sends a Data-Link Master request to another MM node
        :param destination_address: Destination ACA address
        :param payload: Payload to send
        :param channel: Selection of the used channel (relevant only for ST8500)
        :param request_answer: Indicates if an answer is expected
        :param repeater_list: List of repeaters for the frame
        :return: Result of the operation
        """
        if repeater_list is None:
            repeater_list = []

        # Header
        protocol_byte = RequestType.master | Encryption.cleartext

        # ST8500 ahs 2 channels, ST75MM has only one channel
        if self.platform == StPlatform.ST8500:
            protocol_byte |= channel

        # 0x01 – 0x7F: for frames to be transmitted on the PLC line
        # 0x80 – 0xFF: for frames to be transmitted on the ZVEI line
        zvei_line = 0  # No optical port

        # Request cannot be 0 (reserved value)
        if self.master_request_id == 0x7F:
            self.master_request_id = 1
        else:
            self.master_request_id += 1

        request_id = self.master_request_id | (zvei_line << 7)

        header = [protocol_byte, request_id]

        if protocol_byte & Encryption.cyphered:
            # Insert 30 bytes for authentication (ACA, LMON, KEY)
            # header.extend(...)
            pass

        # MAC frame (application payload according to CLC/TS 50568-4)
        mac_frame = self.build_mac_frame(destination_address, payload, request_answer, repeater_list)

        # Assemble the message
        cl_frame = header + mac_frame

        self.log.info("MASTER: message build: {}".format(cl_frame))

        success, _, payload_rx = self.request_confirm(MMCmdID.DL_DATA_Master_req_ID, cl_frame)

        if not success and payload_rx is not None:
            if len(payload_rx) > 0:
                err_code = ErrorCode(payload_rx[0])
                self.log.warning("DL Data Master negative confirm, code: " + str(err_code))

        return success

    def dl_data_slave_request(self, payload: list, request_id=None, channel=Channel.ihd):
        """
        Sends a Data-Link Slave request as answer to the MM node (from which a Data-Link Master request was received)
        :param payload: Payload to send
        :param request_id: Request ID to use (if set to None, the ID of the last DL Master request is used)
        :param channel: Selection of the used channel (relevant only for ST8500)
        :return: Result of the operation
        """
        # Header
        protocol_byte = RequestType.slave | Encryption.cleartext

        # ST8500 ahs 2 channels, ST75MM has only one channel
        if self.platform == StPlatform.ST8500:
            protocol_byte |= channel

        # 0x01 – 0x7F: for frames to be transmitted on the PLC line
        # 0x80 – 0xFF: for frames to be transmitted on the ZVEI line
        zvei_line = 0

        # Request cannot be 0 (reserved value)
        if request_id is None:
            request_id = self.last_request_id

        byte_1 = ((zvei_line << 7) & 0x80) | (request_id & 0x7F)

        header = [protocol_byte, byte_1]

        if protocol_byte & Encryption.cyphered:
            # Insert 30 bytes for authentication (ACA, LMON, KEY)
            # header.extend(...)
            pass

        # Assemble the message
        cl_frame = header + payload

        self.log.info("SLAVE: message build: {}".format(cl_frame))

        # The confirmation cannot be expected in the positive case
        success, _, payload_rx = self.request_confirm(MMCmdID.DL_DATA_Slave_req_ID, cl_frame, expect_cnf=False)

        if not success and payload_rx is not None:
            if len(payload_rx) > 0:
                err_code = ErrorCode(payload_rx[0])
                self.log.error("DL Data Slave negative confirm, code {} = {}".format(payload_rx[0], str(err_code)))
                self.log.debug("DL Data Slave negative confirm payload: {}".format(str(payload_rx)))

        return success

    def dl_data_master_indication(self, indication):
        """
        Parses a Data-Link Master indication
        :param indication: Message data to parse
        :return: The request ID and the payload
        """
        protocol_byte = indication[0]
        request_id = indication[1] & 0x7F
        zvei_line = (indication[1] >> 7) & 0x01
        frame_length = indication[2]

        self.log.info("DL Data Master Indication, request ID: {}".format(request_id))
        if zvei_line:
            self.log.info("ZVEI frame")

        request_type = RequestType(protocol_byte & RequestType.mask)
        encryption = Encryption(protocol_byte & Encryption.mask)

        self.log.debug("Request type: {}".format(request_type))
        self.log.debug("Encryption: {}".format(encryption))

        # ST8500 ahs 2 channels, ST75MM has only one channel
        if self.platform == StPlatform.ST8500:
            channel = Channel(protocol_byte & Channel.mask)
            self.log.debug("Channel: {}".format(channel))

        if frame_length == (len(indication) - 3):  # frame_length excludes the first 3 bytes
            source_address = indication[3:9]
            mac_control = indication[9]

            frame_type = MacFrameType(mac_control & MacFrameType.mask)
            discipline = (mac_control >> 4) & 0xF

            self.log.debug("Source address: {}".format(source_address))
            self.log.debug("Frame type: {}".format(frame_type))
            self.log.debug("Discipline: {}".format(discipline))

            if frame_type == MacFrameType.rip:
                repetition = (mac_control >> 1) & 0x07

                for i in range(0, repetition):
                    repeater_address = []
                    for j in range(0, MMinterface.address_len):
                        repeater_address.append(indication.pop(10))  # Extracts byte at index 10
                    self.log.debug("Repeater {}: {}".format(i, repeater_address))

            # RP field has been extracted, now at 10 there is the NB field
            mac_nb = indication[10]

            self.log.debug("NB: {}".format(mac_nb))

            # Information field (not for CRP frames)
            if frame_type != MacFrameType.crp:
                llc_control = indication[11]
                llc_ectl = indication[12]

                request_answer = (llc_control >> 4) & 0x0F
                network_management = NetworkManagement(llc_ectl & 0x01)

                self.log.debug("Request answer: {}".format(request_answer))
                self.log.debug("Network management: {}".format(network_management))

            payload = indication[12:]
            self.log.debug("Payload: {}".format(payload))
        else:
            payload = None
            self.log.warning("Wrong frame length: {} instead of {}".format(frame_length, (len(indication) - 3)))

        return request_id, payload

    def dl_data_slave_indication(self, indication):
        """
        Parses a Data-Link Slave indication
        :param indication: Message data to parse
        :return: The request ID and the payload
        """
        protocol_byte = indication[0]
        request_id = indication[1] & 0x7F
        zvei_line = (indication[1] >> 7) & 0x01
        payload = indication[2:]

        self.log.info("DL Data Slave Indication, request ID: {}".format(request_id))
        if zvei_line:
            self.log.info("ZVEI frame")

        request_type = RequestType(protocol_byte & RequestType.mask)
        encryption = Encryption(protocol_byte & Encryption.mask)

        self.log.debug("Request type: {}".format(request_type))
        self.log.debug("Encryption: {}".format(encryption))

        # ST8500 ahs 2 channels, ST75MM has only one channel
        if self.platform == StPlatform.ST8500:
            channel = Channel(protocol_byte & Channel.mask)
            self.log.debug("Channel: {}".format(channel))

        self.log.info("Payload: {}".format(payload))

        self.last_request_id = request_id

        return request_id, payload

    def received_msg_handler(self, timeout=None):
        """
        Waits and receives a data indication
        :param timeout: Timeout for the indication reception
        :return: The request ID and the payload
        """
        req_id, payload_ind = None, None
        cmd_id, msg_len, msg = self.host_if.wait_data_ind(tout=timeout)

        if cmd_id is not None:
            self.log.debug("Indication, command ID: {}, length: {}, payload: {}".format(cmd_id, msg_len, msg))

            if cmd_id == MMCmdID.DL_DATA_Slave_ind_ID:
                req_id, payload_ind = self.dl_data_slave_indication(msg)
            elif cmd_id == MMCmdID.DL_DATA_Master_ind_ID:
                req_id, payload_ind = self.dl_data_master_indication(msg)
            else:
                self.log.warning("Unexpected indication received: {}".format(cmd_id))

        return req_id, payload_ind
