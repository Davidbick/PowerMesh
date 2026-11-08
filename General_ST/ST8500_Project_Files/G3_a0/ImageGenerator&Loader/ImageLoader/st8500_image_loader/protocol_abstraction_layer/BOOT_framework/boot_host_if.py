import ctypes
from enum import IntEnum
import logging
import serial
import sys


class CmdId(IntEnum):
    COM_Init = 0x01
    SYS_Reset = 0x02
    IMG_Init = 0x11
    IMG_Write = 0x12
    IMG_Start = 0x13
    MIB_Get = 0x21


class MibId(IntEnum):
    RTE_boot_status = 0x00,
    PE_boot_status = 0x01,
    security_bits = 0x02,
    boot_mode = 0x03,
    boot_version = 0x04,
    ROM_RTE_code_version = 0x05,
    ROM_PE_code_version = 0x06,
    EUI64 = 0x07,
    reserved = 0x08,
    ROM_RTE_code_description = 0x09,
    ROM_PE_code_description = 0x0A


class StPlatform(IntEnum):
    ST_INVALID = 0x00
    STCOMET = 0x01
    ST8500 = 0x02
    ECOM2 = 0x03
    ST75MM = 0x04
    ST_NOTSPECIFIED = 0xFF

    def __str__(self):
        return self.name


# This class implements half dulplex <B>MM</B> UART Interface.
# \par
# It makes use of <B>Python 3.5</B> native modules and <B>Pyserial</B> UART interface module,
# although both Python and Pyserial are available for many platforms, this driver
# has been tested on a specific hardware and software platform (MS Windows(TM)
# running on x86 processor).
class BootHostIf:

    # COM Port handle
    port = None

    head_foot_len = 12

    def __init__(self, port_name, baudrate, log_level_if=logging.ERROR):

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
        self.log.setLevel(log_level_if)

        # configure Serial port
        self.serial = serial.Serial(baudrate=baudrate,
                                    bytesize=serial.EIGHTBITS,
                                    parity=serial.PARITY_NONE,
                                    stopbits=serial.STOPBITS_ONE,
                                    timeout=0.2,
                                    xonxoff=False,
                                    rtscts=False,
                                    dsrdtr=False)
        self.serial.port = port_name

        self.log.info("Initialize the Boot node with {}".format(port_name))

        self.open()

    def __del__(self):
        self.close()

    # #############################################
    #  Low Level UART Methods
    # #############################################

    def open(self):
        try:
            self.serial.open()
        except serial.SerialException:
            self.log.critical("Unable to open serial port {}".format(self.serial.port))

    def close(self):
        self.serial.close()

    def get_baudrate(self):
        return self.serial.baudrate

    def set_baudrate(self, baudrate):
        self.serial.baudrate = baudrate

    @staticmethod
    def hex_string(a):
        if type(a) == 'list':
            a = bytes(a).hex()
        return ' '.join(a + b for a, b in zip(a[::2], a[1::2]))

    @staticmethod
    def compute_crc16_xmodem(data):
        """
        CRC-16 (XMODEM) implemented with a precomputed lookup table
        """
        table = [
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C,
            0xD1AD, 0xE1CE, 0xF1EF,
            0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD,
            0xC39C, 0xF3FF, 0xE3DE,
            0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
            0xF5CF, 0xC5AC, 0xD58D,
            0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF,
            0xE7FE, 0xD79D, 0xC7BC,
            0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948,
            0x9969, 0xA90A, 0xB92B,
            0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79,
            0x8B58, 0xBB3B, 0xAB1A,
            0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A,
            0xBD0B, 0x8D68, 0x9D49,
            0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B,
            0xAF3A, 0x9F59, 0x8F78,
            0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004,
            0x4025, 0x7046, 0x6067,
            0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235,
            0x5214, 0x6277, 0x7256,
            0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466,
            0x6447, 0x5424, 0x4405,
            0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
            0x7676, 0x4615, 0x5634,
            0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0,
            0x08E1, 0x3882, 0x28A3,
            0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1,
            0x1AD0, 0x2AB3, 0x3A92,
            0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2,
            0x2C83, 0x1CE0, 0x0CC1,
            0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93,
            0x3EB2, 0x0ED1, 0x1EF0
        ]
        # XMODEM starts with 0 value
        crc = 0x0000
        for byte in data:
            crc = (crc << 8) ^ table[(crc >> 8) ^ byte]
            crc &= 0xFFFF  # important, crc must stay 16bits all the way through
        return crc

    def create_msg(self, msg_id: int, payload=None):
        if payload is not None:
            payload_len = ctypes.sizeof(payload)
        else:
            payload_len = 0

        header_len = self.head_foot_len - 2

        buff = ctypes.create_string_buffer(self.head_foot_len + payload_len)

        # Fills the header
        buff[0] = 0x16  # SYNC
        buff[1] = 0x16  # SYNC
        buff[2] = msg_id  # CMD ID
        buff[3] = (payload_len & 0xFF)  # MSG length LSB
        buff[4] = ((payload_len >> 8) & 0xFF)  # MSG length MSB
        buff[5] = 0xFE  # Mode
        buff[6:10] = [0, 0, 0, 0]

        # Fills the payload
        i = 0
        if payload_len > 0:
            while i < ctypes.sizeof(payload):
                buff[header_len + i] = payload[i]
                i = i + 1

        # Fills the footer
        # Calculate CRC16-XMODEM
        crc = self.compute_crc16_xmodem(buff.raw[0:header_len + payload_len])
        buff[header_len + payload_len] = (crc & 0xFF)
        buff[header_len + payload_len + 1] = ((crc >> 8) & 0xFF)

        return buff.raw

    def send_msg(self, msg, wait_answer=True):
        """
        Sends a prepared message through the serial port, then receives an answer, if expected
        :param msg: message to send
        :param wait_answer: Should and answer be expected and then waited?
        :return: The answer, if the received, 'None' otherwise
        """
        self.serial.write(msg)
        if wait_answer:
            answer = self.receive_msg()
        else:
            answer = None

        return answer

    def receive_msg(self):
        """
        Receives an answer
        :return: The answer, if the received, 'None' otherwise
        """
        answer = None
        # Read header to get message length
        header_len = 10
        crc_len = 2

        answer_header = self.serial.read(header_len)
        if len(answer_header) == header_len:
            if bytearray(answer_header[0:2]) != bytearray([0x16, 0x16]):
                # print(list(bytes.hex(answer_header)))
                self.log.error("Bad sync field: 0x%X%X instead of 0x1616" % (answer_header[0], answer_header[1]))
            else:
                answer_len = (answer_header[4] << 8) | answer_header[3]

                # Read remaining expected bytes
                answer_payload = self.serial.read(answer_len + crc_len)
                answer = answer_header + answer_payload

        return answer

    def expected_response_length(self, cmd_id: CmdId, mib_id=None):
        if cmd_id == CmdId.COM_Init or cmd_id == CmdId.SYS_Reset or cmd_id == CmdId.IMG_Init or \
                cmd_id == CmdId.IMG_Write or cmd_id == CmdId.IMG_Start:
            payload_len = 0
        elif cmd_id == CmdId.MIB_Get:
            if mib_id is not None:
                if mib_id == MibId.RTE_boot_status or mib_id == MibId.PE_boot_status or \
                        mib_id == MibId.security_bits or mib_id == MibId.boot_mode:
                    payload_len = 2
                elif mib_id == MibId.boot_version or mib_id == MibId.ROM_RTE_code_version or \
                        mib_id == MibId.ROM_PE_code_version:
                    payload_len = 4
                elif mib_id == MibId.EUI64 or mib_id == MibId.reserved:
                    payload_len = 8
                elif mib_id == MibId.ROM_RTE_code_description or mib_id == MibId.ROM_PE_code_description:
                    payload_len = 16
                else:
                    raise Exception("Unknown MIB ID")
            else:
                raise Exception("MIB ID is None with MIB_Get")
        else:
            raise Exception("Unknown command ID")

        return self.head_foot_len + payload_len

    def check_answer(self, cmd_id: CmdId, msg: list, expected_len: int):
        """
        Checks if an answer from the ST8500 is ok, or not
        :param cmd_id: Message ID expected
        :param msg: message to parse and check
        :param expected_len: Expected message length
        :return: True if the ACK is ok False otherwise
        """
        status_ok = False
        status_index = 6

        if msg is not None:
            payload_len = ((msg[4] << 8) | msg[3])
            recv_payload_len = len(msg) - self.head_foot_len

            if msg[2] != cmd_id:
                self.log.error("ID is {} instead of {}".format(msg[3], cmd_id))
            elif len(msg) != expected_len:
                self.log.error("Length is {} instead of minimum {} bytes".format(len(msg), expected_len))
            elif bytearray(msg[0:2]) != bytearray([0x16, 0x16]):
                self.log.error("Bad sync field: 0x%X%X instead of 0x1616" % (msg[0], msg[1]))
            elif payload_len != recv_payload_len:
                self.log.error("Bad length field: {} instead of {}".format(payload_len, recv_payload_len))
            else:
                crc_msg = msg[len(msg) - 1] << 8 | msg[len(msg) - 2]
                crc_calc = self.compute_crc16_xmodem(msg[0:(len(msg) - 2)])

                if crc_msg != crc_calc:
                    self.log.error("Bad crc16 field: 0x%X instead of 0x%X" % (crc_msg, crc_calc))
                else:
                    if msg[status_index] == 0x00:
                        status_ok = True
                    elif msg[status_index] == 0x01:
                        self.log.error("NACK")
                    else:
                        self.log.error("ABORT")
        else:
            payload_len = 0

        if status_ok:
            answer_payload = msg[status_index+1:status_index+1+payload_len]
        else:
            answer_payload = []

        return status_ok, answer_payload
