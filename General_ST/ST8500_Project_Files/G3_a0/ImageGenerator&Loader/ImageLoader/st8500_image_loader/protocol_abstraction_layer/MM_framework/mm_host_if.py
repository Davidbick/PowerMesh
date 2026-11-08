from enum import IntEnum
import logging
import queue
import serial
import sys
import threading
import time
from mm_defs import MMCmdID, RequestType, Channel, Encryption, StPlatform, Mib


# This class implements full/half duplex MM Host Interface.
class MMHostIf:
    # COM Port handle
    port = None

    # Threads and signals
    thread_rx = None
    thread_rx_stop = None
    thread_exceptions = None
    thread_exceptions_stop = None

    class QueueIndex(IntEnum):
        confirm = 0
        data_ind = 1
        asynch_ind = 2

    # Constant values
    class ByteValue(IntEnum):
        # ACK byte value
        ACK = 0x06

        # NACK byte Value
        NACK = 0x15

        # Status Message start byte value
        SM_START = 0x3F

        # Start of Frame byte value
        SOF = 0x02

        # Alternate (repeated frame) start of frame byte value
        A_SOF = 0x03

    # Constructor
    #  It opens the COM port and set the correct configuration.
    # \par Arguments:
    # - com_port_name: name of COM port connected to the modem
    # - platform: the type of the modem (platform)
    # \par Returns:
    # - None
    def __init__(self, port_name, platform: StPlatform, log_level_if=logging.ERROR):

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

        self.platform = platform

        if self.platform == StPlatform.ST75MM:
            baud_rate = 57600
            full_duplex = False
        else:
            baud_rate = 115200
            full_duplex = True

        # Configure Serial port
        self.port = serial.Serial(baudrate=baud_rate,
                                  bytesize=serial.EIGHTBITS,
                                  parity=serial.PARITY_NONE,
                                  stopbits=serial.STOPBITS_ONE,
                                  timeout=0.2,
                                  xonxoff=False,
                                  rtscts=False,
                                  dsrdtr=False)

        self.port.port = port_name

        self.full_duplex = full_duplex

        if self.full_duplex:
            self.treq_en = False
        else:
            self.treq_en = True

        self.thread_rx, self.thread_rx_stop = self.create_thread("RX thread", self.ser_port_rcv_handler)

        self.thread_exceptions, self.thread_exceptions_stop = self.create_thread("Unexpected event thread",
                                                                                 self.unexpected_event_handler)

        self.q_ack_nack = queue.Queue()
        self.q_status_message = queue.Queue()

        self.queues = [queue.Queue(), queue.Queue(), queue.Queue()]

        self.log.info("Initialize the MM host IF ({}) with {}".format(platform, port_name))

        self.open()

    def __del__(self):
        self.close()

    def open(self):
        try:
            # Clear RTS and DTR lines
            self.port.setRTS(False)
            self.port.setDTR(False)

            self.port.open()
            self.thread_rx_stop.clear()
            self.thread_exceptions_stop.clear()

            self.thread_rx.start()
            self.thread_exceptions.start()
        except serial.SerialException:
            self.log.critical("Unable to open serial port {}".format(self.port.port))

    def close(self):
        self.stop_thread(self.thread_exceptions, self.thread_exceptions_stop)
        self.stop_thread(self.thread_rx, self.thread_rx_stop)
        self.port.close()

    def get_baudrate(self):
        return self.port.baudrate

    # set_treq Method
    #
    # This method is used by the Host to assert TREQ line to initiate a Host to Modem transaction.
    # \par Arguments:
    # - None
    # \par Returns:
    # - None
    def set_treq(self):
        # Using Pyserial both RTS and DTR shall be set to have RTS signal asserted
        self.port.setRTS(True)
        self.port.setDTR(True)
        return

    # clear_treq Method
    #
    # This method is used by the Host to clear TREQ line after Status Message reception.
    # \par Arguments:
    # - None
    # \par Returns:
    # - None
    def clear_treq(self):
        # Using Pyserial both RTS and DTR shall be cleared to have RTS signal de-asserted
        self.port.setRTS(False)
        self.port.setDTR(False)
        return

    def ser_port_rcv_bytes(self, msg=None, numbytes=1, stop_event=None):

        if msg is None:
            msg = []

        for i in range(0, numbytes):
            try:
                if stop_event is not None and stop_event.is_set:
                    return False
                c = self.port.read(1)
                if c != b'':
                    # logger.debug("0x{0:02X}".format(ord(c)))
                    msg.append(ord(c))
                else:
                    return False

            except serial.SerialException:
                return False

        return True

    def ser_port_rcv_handler(self, arg1, stop_event):
        self.log.info("Serial port receive thread started")
        self.log.info("Stop event: {}".format(stop_event))
        while not stop_event.is_set():
            data = self.port.read(1)
            if data == b'':
                continue
            c = ord(data)
            msg = [c]
            if c == self.ByteValue.ACK or c == self.ByteValue.NACK:
                self.log.debug("First byte: {}".format(self.ByteValue(c).name))
                self.q_ack_nack.put(msg)
            elif c == self.ByteValue.SM_START:
                self.log.debug("First byte: {}".format(self.ByteValue(c).name))
                # optional Treq
                if self.treq_en:
                    self.log.debug("Cleared TREQ")
                    self.clear_treq()
                ret = self.ser_port_rcv_bytes(msg, 3)
                if ret:
                    self.q_status_message.put(msg)
            elif c == self.ByteValue.SOF or c == self.ByteValue.A_SOF:
                self.log.debug("First byte: {}".format(self.ByteValue(c).name))
                # get LT and CMD_ID
                ret = self.ser_port_rcv_bytes(msg, 2)
                if not ret:
                    continue
                # get  the full payload (LT + 1)
                ret = self.ser_port_rcv_bytes(msg, msg[1] + 1)
                if not ret:
                    continue
                # get SVT
                ret = self.ser_port_rcv_bytes(msg, 2)
                if not ret:
                    continue
                chk_sum = self.verify_checksum(msg)

                if not self.full_duplex:
                    #  Handle ACK/NACK
                    self.send_ack(chk_sum)

                if not chk_sum:
                    self.log.error("Bad checksum")
                    continue

                self.default_q_selector(msg)
                continue
            elif c == 0xFF:  # Can be triggered by reset, ignored
                continue
            else:
                self.log.warning("Unexpected first byte: {}".format(c))
                continue

        self.log.info("Serial port receive thread stopped")

    def default_q_selector(self, raw_msg):
        if len(raw_msg) != 0:
            msg = [raw_msg[2], raw_msg[1], raw_msg[3:-2]]

            # confirm messages
            if msg[0] == MMCmdID.MIB_WRITE_pos_conf_ID or \
                    msg[0] == MMCmdID.MIB_WRITE_neg_conf_ID or \
                    msg[0] == MMCmdID.MIB_READ_pos_conf_ID or \
                    msg[0] == MMCmdID.MIB_READ_neg_conf_ID or \
                    msg[0] == MMCmdID.DL_DATA_Slave_pos_conf_ID or \
                    msg[0] == MMCmdID.DL_DATA_Slave_neg_conf_ID or \
                    msg[0] == MMCmdID.DL_DATA_Master_pos_conf_ID or \
                    msg[0] == MMCmdID.DL_DATA_Master_neg_conf_ID or \
                    msg[0] == MMCmdID.HI_PING_confirm_ID or \
                    msg[0] == MMCmdID.RAW_DATA_pos_conf_ID or \
                    msg[0] == MMCmdID.RAW_DATA_neg_conf_ID or \
                    msg[0] == MMCmdID.BIO_RESET_pos_conf_ID or \
                    msg[0] == MMCmdID.HWRESET_confirm_ID or \
                    msg[0] == MMCmdID.SFLASH_confirm_ID:
                msg[0] = MMCmdID(msg[0])
                self.log.info("Confirm message : " + str(msg))
                self.queues[self.QueueIndex.confirm].put(msg)
            # indication messages
            elif msg[0] == MMCmdID.DL_DATA_Slave_ind_ID or \
                    msg[0] == MMCmdID.DL_DATA_Master_ind_ID or \
                    msg[0] == MMCmdID.RAW_DATA_indication_ID:
                msg[0] = MMCmdID(msg[0])
                request_type = RequestType(msg[2][0] & RequestType.mask)
                self.log.info("Received Data Indication, cmd ID {}, size {}, type {}".format(MMCmdID(msg[0]),
                                                                                             msg[1],
                                                                                             request_type))
                if self.platform == StPlatform.ST8500:
                    self.log.debug("Channel : " + str(Channel(msg[2][0] & Channel.mask)))
                self.log.debug("Encryption : " + str(Encryption(msg[2][0] & Encryption.mask)))
                self.log.debug("Indication message : " + str(msg))
                self.queues[self.QueueIndex.data_ind].put(msg)
            elif msg[0] == MMCmdID.MIB_WRITE_indication_ID or msg[0] == MMCmdID.BIO_RESET_indication_ID:
                msg[0] = MMCmdID(msg[0])
                self.log.info("Received Asynch Indication : {}, size {}, payload: {}".format(MMCmdID(msg[0]),
                                                                                             msg[1],
                                                                                             msg[2]))
                self.log.debug("Indication message : " + str(msg))
                self.queues[self.QueueIndex.asynch_ind].put(msg)
            # unexpected messages
            else:
                self.log.warning("Unexpected message {}".format(msg[0]))
                self.log.warning("Full message: {}".format(raw_msg))

    # wait_ack Method
    #
    # Wait for an ACK or NACK byte coming from <B>ST75MM</B>.
    # \par Arguments:
    # - None
    # \par Returns:
    # - Logical value True if ACK byte has been received
    # - None if NACK or no response
    def wait_ack(self):
        # Wait for ACK/NACK
        ack = None

        try:
            ack_message = self.q_ack_nack.get(block=True, timeout=1.0)
            if ack_message[0] == self.ByteValue.ACK:
                ack = True
            elif ack_message[0] == self.ByteValue.NACK:
                ack = False
            else:
                self.log.error("Unexpected ACK/NACK value: {}".format(ack_message[0]))
                ack = False
        except queue.Empty:
            self.log.error("ACK/NACK NOT received")

        return ack

    # send_ack Method
    #
    # This method sends an ACK or NACK byte to the <B>ST75MM</B>.
    # \par Arguments:
    # - ack: if True an ACK byte will be sent, if False a NACK byte will be sent
    # \par Returns:
    # - None
    def send_ack(self, ack=True):
        if ack:
            # Send Acknowledge byte
            self.log.debug("Sending ACK")
            ack_msg = [self.ByteValue.ACK]
        else:
            # Send NACK byte
            self.log.error("Sending NACK")
            ack_msg = [self.ByteValue.NACK]

        self.port.write(ack_msg)

    # wait_status Method
    #
    # This method wait for Status Message coming from the platform.
    # \par Arguments:
    # - None
    # \par Returns:
    # - a 4 byte string containing the Modem Status Message
    # - None if no status message is received
    def wait_status(self):
        # Wait for Status Message start bit
        try:
            status = self.q_status_message.get(block=True, timeout=1.0)
        except queue.Empty:
            status = None
            self.log.error("Status Message NOT received")

        return status

    # ParseStatusMessage Method
    #
    # This method is used to parse status message and detect a Modem busy condition which
    # could prevent the host from operating on the modem itself (both programming and sending
    # data).
    # \par Arguments:
    # - status_message: is the value returned by the WaitForStatusMessage method
    # \par Returns:
    # - Logical value True if the modem can accept commands (not busy)
    # - Logical value False if the modem is busy and cannot accept commands
    def parse_status(self, status_message):

        success = False
        # Verify status_message has been received
        if len(status_message) >= 2:
            # In order to verify if the ST75MM modem can accept a command
            # bit 3 (Modem Busy) must set to 0 in byte 1
            # if ord(status_message[1]) & 0x08: # MODIFIED
            if status_message[1] & 0x08:
                # ST75MM is not available, deny transaction
                self.log.error("ST75MM is not available, deny transaction")
            else:
                # ST75MM is available, clear transaction
                success = True
                self.log.debug("ST75MM is available, clear transaction")
        else:
            self.log.error("Status message is negative")

        return success

    # CalcCheckSum Method
    #
    # This method calculates the checksum over the given frame (except the first byte)
    # \par Arguments:
    # - frame: is the command starting from SOF (Start of Frame) byte, without the checksum
    # \par Returns:
    # - A 2 bytes value containing the checksum to be appended at the end of the frame
    @staticmethod
    def calculate_checksum(frame):
        # Initialize checksum variable
        CheckSum = 0
        # Calculate Checksum over the entire packet except SOF (first byte)
        for char in frame[1:]:
            CheckSum += char

        # Return Checksum value
        # self.log.debug("[{},{}]".format((CheckSum & 0xFF), (CheckSum >> 8) & 0xFF))
        crc_array = [(CheckSum & 0xFF), (CheckSum >> 8) & 0xFF]
        return crc_array

    # VerifyCheckSum Method
    #
    # This method calculates the checksum over a  received frame (complete of Start of Frame
    # and checksum), and verify if the calculated checksum matches with the transmitted one.
    # \par Arguments:
    # - frame: is the frame received from the <B>ST75MM</B> UART
    # \par Returns:
    # - Logical value True if the received checksum matches with the calculated one
    # - Logical value False if there is a checksum error
    @staticmethod
    def verify_checksum(frame):
        # Initialize checksum variable
        CheckSum = 0
        # Calculate Checksum over the entire packet except SOF (first byte) and CHK (last two bytes)
        for ch in frame[1:-2]:
            # sys.stdout.write(repr(ch))
            CheckSum += ch
        # Reduce to 16 bits
        Calc_Chk = CheckSum % 0xFFFF
        # Get Received Checksum
        Received_Chk = (frame[-1] << 8) | frame[-2]
        if Calc_Chk == Received_Chk:
            # Return True if chk matches
            return True
        else:
            # Otherwise False
            return False

    # Prepare Command Method
    #
    # This method is used by the host to prepare a message for the <B>ST75MM</B> UART.
    # \par Arguments:
    # - cmd_id: command identifier
    # - payload: payload of the command
    # \par Returns:
    # - the frame to be sent to the Host IF
    def prepare_command(self, cmd_id: MMCmdID, payload=None):

        if payload is None:
            payload = [0]

        # Start Of Frame (SOF)
        frame = [self.ByteValue.SOF]
        # Length
        frame += [len(payload) - 1]
        # Command Code
        frame += [cmd_id]
        # payload
        frame += payload
        # CheckSum
        frame += self.calculate_checksum(frame)

        self.log.debug("Dump Frame: ('%s')", frame)
        # Return the packet to be sent
        return frame

    def send_command(self, command):
        success = False

        if self.port.is_open:
            # optional Treq
            if self.treq_en:
                clear_to_send = False
                wait_time = 0.5
                timeout_time = 5
                timeout_counter = int(round(timeout_time / wait_time))

                while not clear_to_send:
                    # Assert TREQ line (TREQ is then cleared during Status Message reception)
                    self.set_treq()

                    # Wait for Status Message
                    status = self.wait_status()

                    if status is not None:
                        # Parse Status Message
                        clear_to_send = self.parse_status(status)

                    if not clear_to_send:
                        # Modem is not ready to send
                        self.log.warning("Modem Busy, retrying later")
                        time.sleep(wait_time)
                        timeout_counter -= 1

                        if timeout_counter == 0:
                            self.log.error("Modem does not respond")
                            break
            else:
                clear_to_send = True

            if clear_to_send:
                # Send the bytes
                self.log.info("Send data: {}".format(command))
                self.port.write(command)

                if not self.full_duplex:
                    # Wait for Acknowledge
                    ack = self.wait_ack()

                    # Check ack/nack
                    if ack is not None:
                        if ack:
                            success = True
                            self.log.info("ACK")
                        else:
                            # NACK or no answer from the modem
                            self.log.error("NACK")
                    else:
                        # NACK or no answer from the modem
                        self.log.error("NO ANSWER")
                else:
                    success = True
            else:
                success = True

        return success

    # wait_async_msg method
    #
    # This method is used to get a message frame from the UART
    # \par Arguments:
    # - tout: is the time after which the procedure is drop in case no
    # valid frame has been received (None for infinite timeout)
    # \par Returns:
    #   -# Command ID of the received frame
    #   -# Length of the received frame
    #   -# Payload of the received frame
    # - In case no valid frame has been received before the tout timeout expires,
    # a tuple of three None value is returned
    def wait_message(self, queue_index: QueueIndex, tout=None, silent=False):
        if not silent:
            self.log.debug("Waiting {} queue".format(queue_index.name))
        try:
            msg = self.queues[queue_index].get(block=True, timeout=tout)
            if msg is not None:
                self.log.debug("Received: {}, len: {}, Payload: {}".format(msg[0], msg[1], msg[2]))
                return MMCmdID(msg[0]), msg[1], msg[2]
            else:
                return None, None, None
        except queue.Empty:
            return None, None, None

    # wait_confirm Method
    #
    # This method can be used to wait incoming confirm in MASTER/SLAVE/RAW modes
    # Call this method in an application thread. It blocks on an internal queue waiting for a message
    # \par Returns:
    # - Command Code
    # - Payload Length
    # - Payload
    def wait_confirm(self, tout=None):
        # block on the indication queue
        cmd_id, msg_len, msg = self.wait_message(self.QueueIndex.confirm, tout)
        if cmd_id is not None:
            self.log.info("Received: {}, len: {}, Payload: {}".format(MMCmdID(cmd_id).name, msg_len, msg))
        else:
            self.log.warning("Confirm not received")
        return cmd_id, msg_len, msg

    # wait_data_ind Method
    #
    # This method can be used to wait incoming data in MASTER/SLAVE/RAW modes
    # Call this method in an application thread. It blocks on an internal queue waiting for a message
    # \par Returns:
    # - Command Code
    # - Message Length
    # - Message
    def wait_data_ind(self, tout=None):
        # block on the indication queue
        cmd_id, msg_len, msg = self.wait_message(self.QueueIndex.data_ind, tout, silent=True)
        if cmd_id is not None:
            self.log.info("Received: {}, len: {}, Payload: {}".format(MMCmdID(cmd_id).name, msg_len, msg))

        return cmd_id, msg_len, msg

    # wait_async_ind Method
    #
    # This method can be used to wait asynch events that are not indications
    # This function is called in teh unexpected_event_thread. It blocks on an internal queue waiting for a message
    # \par Returns:
    # - Command Code
    # - Payload Length
    # - Payload
    def wait_async_ind(self, tout=None):
        # block on the indication queue
        cmd_id, msg_len, msg = self.wait_message(self.QueueIndex.asynch_ind, tout, silent=True)
        if cmd_id is not None:
            if MMCmdID(cmd_id) == MMCmdID.MIB_WRITE_indication_ID:
                mib = Mib.Object(msg[0], msg[1:])
                self.log.info("MIB Write Indication index: {}, new value: {}".format(mib.index, mib.content))
            else:
                self.log.info("Received: " + str(MMCmdID(cmd_id).name))

        return cmd_id, msg_len, msg

    def unexpected_event_handler(self, arg1, stop_event):
        self.log.info("Default unexpected event thread started")
        self.log.debug("Stop event: {}".format(stop_event))
        while not stop_event.is_set():
            self.wait_async_ind(tout=1)

        self.log.info("Default unexpected event thread stopped")

    def create_thread(self, thread_name, routine):
        stop_event = threading.Event()
        thread = threading.Thread(target=routine, args=("", stop_event))
        thread.name = thread_name

        self.log.debug("{} created".format(thread.name))

        return thread, stop_event

    def stop_thread(self, thread, stop_event):
        if thread.is_alive():
            stop_event.set()
            thread.join()
            self.log.debug("{} closed".format(thread.name))
