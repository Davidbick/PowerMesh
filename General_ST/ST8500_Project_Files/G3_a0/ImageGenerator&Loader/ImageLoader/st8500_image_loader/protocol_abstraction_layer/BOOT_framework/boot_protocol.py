import ctypes
import sys
import logging
from boot_host_if import CmdId, MibId, BootHostIf


class BootInterface:

    default_baudrate = 9600

    def __init__(self, port_name, baudrate=None, log_level_prot=logging.ERROR, log_level_if=logging.ERROR):

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

        if baudrate is None:
            baudrate = self.default_baudrate

        self.host_if = BootHostIf(port_name=port_name, baudrate=baudrate, log_level_if=log_level_if)

    def __del__(self):
        self.host_if.close()

    @staticmethod
    def __round_up(x, base=16):
        return (x + base - 1) & (-1 * base)

    @staticmethod
    def __fill_c_buffer(a_p, a):
        i = 0
        while i < len(a):
            a_p[i] = a[i]
            i = i + 1

        padding = ctypes.sizeof(a_p) - len(a)
        while padding > 0:
            a_p[i + padding] = 0x0
            padding = padding - 1

    def close(self):
        self.host_if.close()

    def reset(self, confirm_expected):
        """
        Resets the device (hardware reset) in BOOT
        :return: Result of the operation
        """
        # Send System Reset command
        success, answer_payload = self.transaction(CmdId.SYS_Reset, wait_answer=confirm_expected)
        if confirm_expected:
            if success:
                self.log.info("Reset: OK")
            else:
                self.log.info("Reset: FAILED")
        else:
            success = True

        if success:
            self.set_baudrate(self.default_baudrate)

        return success

    def get_baudrate(self):
        """
        Returns the current baudrate in use in BOOT
        :return: The current used baudrate
        """
        return self.host_if.get_baudrate()

    def set_baudrate(self, baudrate):
        """
        Sets the baudrate to use in BOOT
        :param baudrate: Baudrate value to use
        :return: Nothing
        """
        self.host_if.set_baudrate(baudrate)

    def change_baudrate(self, baudrate):
        """
        Changes the baudrate to use in BOOT
        :param baudrate: Baudrate value to use
        :return: Result of the operation
        """
        # Init COM port
        current_baudrate = self.host_if.get_baudrate()
        self.log.info("Configuring COM port (baudrate {} -> {} bps): ".format(current_baudrate, baudrate))
        success = self.com_init(baudrate, 0, 1)
        if success:
            self.host_if.set_baudrate(baudrate)  # Updates the baudrate
        else:
            self.log.error("Error while configuring COM port")

        return success

    def info(self):
        return "Baudrate: " + str(self.get_baudrate()) + " bps"

    def version(self):
        """
        Detects if BOOT protocol is currently running on the platform
        :return: The BOOT version, None in case of error
        """
        success, mib_read = self.mib_get_request(MibId.boot_version)
        if success:
            boot_version = "{}.{}.{}.{}".format(mib_read[0], mib_read[1], mib_read[2], mib_read[3])
        else:
            boot_version = None

        self.log.info(boot_version)

        return boot_version

    def transaction(self, cmd_id: CmdId, payload=None, mib_id=None, wait_answer=True):

        expected_answer_len = self.host_if.expected_response_length(cmd_id, mib_id)

        request = self.host_if.create_msg(cmd_id, payload)

        self.log.debug("Request: " + ' '.join('{:02x}'.format(x) for x in request))

        answer = self.host_if.send_msg(request, wait_answer)

        if answer is not None:
            self.log.debug("Answer: " + ' '.join('{:02x}'.format(x) for x in answer))
        else:
            self.log.debug("No answer")

        if wait_answer:
            status_ok, answer_payload = self.host_if.check_answer(cmd_id, answer, expected_answer_len)
        else:
            status_ok = True
            answer_payload = None

        return status_ok, answer_payload

    def com_init(self, baudrate: int, parity=0, stopbits=1):
        """
        Sends the message used to initialize COM port in BOOT mode
        :param baudrate: The baudrate to set in BOOT mode
        :param parity: The parity to set in BOOT mode
        :param stopbits: The stop bits to set in BOOT mode
        :return: The prepared message
        """
        payload = ctypes.create_string_buffer(6)
        payload[0] = (baudrate & 0xFF)
        payload[1] = ((baudrate >> 8) & 0xFF)
        payload[2] = ((baudrate >> 16) & 0xFF)
        payload[3] = ((baudrate >> 24) & 0xFF)
        payload[4] = parity
        payload[5] = stopbits

        success, _ = self.transaction(CmdId.COM_Init, payload)

        if success:
            self.log.info("COM port init: OK")
        else:
            self.log.error("COM port init: FAILED")

        return success

    def mib_get_request(self, mib_id: MibId):
        """
        Sends the message used to read a MIB in BOOT mode
        :param mib_id: The ID of the MIB to read
        :return: Result of the operation
        """
        payload = ctypes.create_string_buffer(1)
        payload[0] = (mib_id & 0xFF)

        success, mib_value = self.transaction(CmdId.MIB_Get, payload, mib_id)

        if success:
            self.log.info("MIB read: OK")
        else:
            self.log.error("MIB read: FAILED")

        return success, mib_value

    def initialize_image(self, image_header):
        """
        Initializes the image to write in BOOT
        :param image_header: The image header
        :return: Result of the operation
        """
        # Fills c buffer
        header_p = ctypes.create_string_buffer(len(image_header))
        self.__fill_c_buffer(header_p, image_header)

        # Checks header integrity
        success, _ = self.transaction(CmdId.IMG_Init, header_p)

        if success:
            self.log.info("Image header: OK")
        else:
            self.log.error("Image header: FAILED")

        return success

    def write_image_data(self, image_data):
        """
        Writes image data in BOOT
        :param image_data: The image data to write
        :return: Result of the operation
        """
        payload_p = ctypes.create_string_buffer(self.__round_up(len(image_data)))
        self.__fill_c_buffer(payload_p, image_data)

        # Writes image data
        success, _ = self.transaction(CmdId.IMG_Write, payload_p)

        if success:
            self.log.debug("Image write: OK")
        else:
            self.log.error("Image write: FAILED")

        return success

    def start_image(self):
        """
        Starts image in BOOT
        :return: Result of the operation
        """
        # Writes image data
        success, _ = self.transaction(CmdId.IMG_Start)

        if success:
            self.log.debug("Image start: OK")
        else:
            self.log.error("Image start: FAILED")

        return success
