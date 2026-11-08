from enum import IntEnum


class MMCmdID(IntEnum):
    # Command codes
    MIB_WRITE_request_ID = 0x08
    MIB_WRITE_pos_conf_ID = 0x09
    MIB_WRITE_indication_ID = 0x0A
    MIB_WRITE_neg_conf_ID = 0x0B
    MIB_READ_request_ID = 0x0C
    MIB_READ_pos_conf_ID = 0x0D
    MIB_READ_neg_conf_ID = 0x0F
    DL_DATA_Slave_req_ID = 0x24
    DL_DATA_Slave_pos_conf_ID = 0x25
    DL_DATA_Slave_ind_ID = 0x26
    DL_DATA_Slave_neg_conf_ID = 0x27
    DL_DATA_Master_req_ID = 0x28
    DL_DATA_Master_pos_conf_ID = 0x29
    DL_DATA_Master_ind_ID = 0x2A
    DL_DATA_Master_neg_conf_ID = 0x2B
    HI_PING_request_ID = 0x2C
    HI_PING_confirm_ID = 0x2D
    RAW_DATA_request_ID = 0x30
    RAW_DATA_pos_conf_ID = 0x31
    RAW_DATA_indication_ID = 0x32
    RAW_DATA_neg_conf_ID = 0x33
    BIO_RESET_request_ID = 0x3C
    BIO_RESET_pos_conf_ID = 0x3D
    BIO_RESET_indication_ID = 0x3E
    BIO_RESET_neg_conf_ID = 0x3F
    HWRESET_request_ID = 0x40
    HWRESET_confirm_ID = 0x41
    TESTSET_request = 0x4A
    TESTSET_confirm = 0x4B
    TESTGET_request = 0x4C
    TESTGET_confirm = 0x4D
    SFLASH_request_ID = 0x4E
    SFLASH_confirm_ID = 0x4F

    def __str__(self):
        return self.name


class SpiFlashOp(IntEnum):
    erase = 0x00
    read = 0x01
    write = 0x02

    def __str__(self):
        return self.name


class ErrorCode(IntEnum):
    WPL = 2
    WPV = 3
    BUSY = 4
    NOT_PRESENT = 6
    DISABLED = 7
    TIMEOUT = 8
    ERROR = 255

    def __str__(self):
        return self.name

# Protocol type field
# 0x01 MM without security on AMI channel
# 0x09 MM without security on IHD channel
# 0x81 MM with security on AMI channel
# 0x89 MM with security on IHD channel


class RequestType(IntEnum):
    slave = 0x00
    master = 0x01
    mask = 0x01

    def __str__(self):
        return self.name


# ST8500 has 2 channels:
# - The low channel (AMI) is used for Chain 1 (86 KHz)
# - The high channel (IHD) is used for Chain 2 (132.5 KHz)
class Channel(IntEnum):
    ami = 0x00  # Chain 1 (Meter to data concentrator)
    ihd = 0x08  # Chain 2 (Meter to home)
    mask = 0x8

    def __str__(self):
        return self.name


class Encryption(IntEnum):
    cleartext = 0x00
    cyphered = 0x80
    mask = 0x80

    def __str__(self):
        return self.name


class Modulations(IntEnum):
    BPSK = 0
    QPSK = 1
    _8PSK = 2
    BFSK = 3
    BPSK_COD = 4
    QPSK_COD = 5
    Reserved = 6
    BPSK_COD_PNA = 7


class StPlatform(IntEnum):
    ST_INVALID = 0x00
    STCOMET = 0x01
    ST8500 = 0x02
    ECOM2 = 0x03
    ST75MM = 0x04
    ST_NOTSPECIFIED = 0xFF

    def __str__(self):
        return self.name


class CsmaTimeout(IntEnum):
    seconds_1_5 = 0
    seconds_3 = 1
    seconds_5 = 2
    seconds_10 = 3

    def __str__(self):
        if self == self.seconds_1_5:
            value = 1.5
        elif self == self.seconds_3:
            value = 3
        elif self == self.seconds_5:
            value = 5
        elif self == self.seconds_10:
            value = 10
        else:
            value = "invalid"

        return "{} s".format(value)


class CsmaSlotSize(IntEnum):
    milliseconds_5 = 0
    milliseconds_10 = 1

    def __str__(self):
        if self == self.milliseconds_5:
            value = 5
        elif self == self.milliseconds_10:
            value = 10
        else:
            value = "invalid"

        return "{} ms".format(value)


class CsmaSlotNumber(IntEnum):
    slots_7 = 0
    slots_15 = 1
    slots_31 = 2
    slots_63 = 3

    def __str__(self):
        if self == self.slots_7:
            value = 7
        elif self == self.slots_15:
            value = 15
        elif self == self.slots_31:
            value = 31
        elif self == self.slots_63:
            value = 63
        else:
            value = "invalid"

        return "{} slots".format(value)


class BiuObservationWindow(IntEnum):
    refresh_2ms = 0
    refresh_4ms = 1
    refresh_8ms = 2
    refresh_16ms = 3

    def __str__(self):
        if self == self.refresh_2ms:
            value = 2
        elif self == self.refresh_4ms:
            value = 4
        elif self == self.refresh_8ms:
            value = 8
        elif self == self.refresh_16ms:
            value = 16
        else:
            value = "invalid"

        return "{} ms".format(value)


class Mib:

    class Index(IntEnum):
        # ST75MM & ST8500
        phy_data = 0
        mac_data = 1
        phy_configuration = 2
        mac_configuration = 3
        fw_release = 4
        last_frame_phy_data = 5
        manufacturer_data = 6
        logical_address = 7
        encryption_slave_keys = 8
        specific_feature_configuration = 9
        lmon = 10
        # reserved = 11
        internal_timeouts = 12
        # reserved = 13
        timings = 14  # NOT IMPLEMENTED ON ST8500
        # reserved = 15
        protection_error_counter = 16
        fik = 17
        zc_alarm = 18
        # reserved = 19
        # reserved = 20
        nb_phase_compression = 21
        reset_causes = 22
        custom_freq = 23
        stats = 24
        csma_configuration = 32
        biu_calibration = 33

        # Only ST8500
        rte_fw_version = 34  # HEX = 0x22
        mac_custom_configuration = 35  # HEX = 0x23
        rte_critical_errors = 36  # HEX = 0x24
        pe_fw_version = 37  # HEX = 0x25
        cc_sp = 38  # HEX = 0x26

    class Object:
        def __init__(self, index: int, content: list):
            self.index = index
            self.content = content

    @staticmethod
    def mib_phy_configuration(platform: StPlatform, current_control=1, zero_crossing=0, rx_channel=0, tx_channel=0,
                              biu_enable=0, csma_enable=0, tx_gain=21, tx_modulation=Modulations.BPSK_COD,
                              psk_preamble=2, rx_low_ch=0, rx_high_ch=1):

        mib_index = Mib.Index.phy_configuration.value

        if platform == StPlatform.ST8500:
            freq_pair = 1  # 86kHz (AMI), 132.5kHz(IHD)

            if rx_channel == 1:
                raise Exception("ST8500 does not support dual channel")

            if tx_channel == 1:
                raise Exception("ST8500 does not support TX on low channel")

            if tx_modulation == Modulations.BFSK:
                raise Exception("ST8500 does not support BFSK")

            if rx_low_ch == 1:
                raise Exception("ST8500 does not support TX on low channel modulation")

            if rx_high_ch == 0:
                raise Exception("ST8500 does not support FSK")
        else:
            freq_pair = 3  # Custom frequency (default)
            pass

        byte_0 = (current_control << 0) | (zero_crossing << 1) | (rx_channel << 2) | (tx_channel << 3) | \
                 (freq_pair << 4) | (biu_enable << 6) | (csma_enable << 7)

        byte_1 = (tx_gain << 0) | (tx_modulation.value << 5)

        byte_2 = (psk_preamble << 0) | (rx_low_ch << 2) | (rx_high_ch << 3)
        
        mib_val = [byte_0, byte_1, byte_2, 0x3B, 0x58, 0x9B]

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_mac_configuration_st75mm(rx_mode=0x1, tx_mode=1):

        mib_index = Mib.Index.mac_configuration.value

        # Bit 0-3: channel RX enable
        mib_val = [rx_mode, tx_mode]

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_mac_configuration_st8500(rx_mode_ihd=0x1, rx_mode_ami=0x1, tx_mode=1):

        mib_index = Mib.Index.mac_configuration.value

        # Bit 0-3: AMI channel RX enable, bit 4-7: IHD channel RX enable
        rx_mode = ((rx_mode_ihd & 0x0F) << 4) | (rx_mode_ami & 0x0F)

        # For ST8500: bits 0-3 rx configuration for AMI channel, bits 4-7 rx configuration for IHD channel
        mib_val = [rx_mode, tx_mode]

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_manufacturer_data(dev_id=None, aca=None):

        mib_index = Mib.Index.manufacturer_data.value

        if dev_id is None:
            dev_id = ([0] * 16)

        if aca is None:
            aca = [0xBD, 0x89, 0x1E, 0x0A, 0x04, 0x90]

        mib_val = dev_id
        mib_val.extend(aca)

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_custom_freq(platform: StPlatform, channel_ihd=None, channel_ami=None):

        if platform == StPlatform.ST8500:
            # In ST8500, only one channel is used
            tx_high_freq = 86000.0
            tx_low_freq = 72000.0
        else:
            # In ST75MM, both channel are used
            if channel_ihd is None:
                tx_high_freq = 132500.0
            else:
                tx_high_freq = channel_ihd

            if channel_ami is None:
                tx_low_freq = 110000.0
            else:
                tx_low_freq = channel_ami

        mib_index = Mib.Index.custom_freq.value

        tx_high_freq *= 4.194304
        tx_low_freq *= 4.194304

        rx_mix_if_freq = (tx_high_freq + tx_low_freq) / 2
        # please note: rx_mix_af_int uses a different scale factor: (54.525952 = 13 x 4.194304)
        rx_mix_af_freq = ((tx_high_freq - tx_low_freq) / 2) * 13

        tx_high_int = int(tx_high_freq)
        tx_low_int = int(tx_low_freq)
        rx_mix_if_int = int(rx_mix_if_freq)
        rx_mix_af_int = int(rx_mix_af_freq)

        tx_high_array = [((tx_high_int >> 16) & 0xFF), ((tx_high_int >> 8) & 0xFF), (tx_high_int & 0xFF)]
        tx_low_array = [((tx_low_int >> 16) & 0xFF), ((tx_low_int >> 8) & 0xFF), (tx_low_int & 0xFF)]
        rx_mix_if_array = [((rx_mix_if_int >> 16) & 0xFF), ((rx_mix_if_int >> 8) & 0xFF), (rx_mix_if_int & 0xFF)]
        rx_mix_af_array = [((rx_mix_af_int >> 16) & 0xFF), ((rx_mix_af_int >> 8) & 0xFF), (rx_mix_af_int & 0xFF)]

        mib_val = tx_high_array
        mib_val.extend(tx_low_array)
        mib_val.extend(rx_mix_if_array)
        mib_val.extend(rx_mix_af_array)

        if platform == StPlatform.ST8500:
            if channel_ihd is None:
                tx_f_ihd_mix = 132500.0
            else:
                tx_f_ihd_mix = channel_ihd

            tx_f_ihd_mix *= 4.194304

            tx_f_ihd_mix_int = int(tx_f_ihd_mix)

            tx_f_ihd_mix_array = [((tx_f_ihd_mix_int >> 16) & 0xFF), ((tx_f_ihd_mix_int >> 8) & 0xFF),
                                  (tx_f_ihd_mix_int & 0xFF)]

            mib_val.extend(tx_f_ihd_mix_array)
            mib_val.extend([0x00, 0x00])
        else:
            mib_val.extend([0x00, 0x00, 0x00, 0x00, 0x00])

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_mac_custom_configuration(platform: StPlatform, tx_delay=15, tsg=0, lc_rsg=0, hc_rsg=0, bsg=0):

        if platform != StPlatform.ST8500:
            raise Exception(" MIB present only on ST8500")

        mib_index = Mib.Index.mac_custom_configuration.value

        byte_1 = ((bsg << 6) | (hc_rsg << 2) | (lc_rsg << 1) | tsg)

        mib_val = [tx_delay, byte_1]

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_csma_configuration(immediate_ack=False, virtual_biu_after_rx=True, biu_false_after_rx=False,
                               mac_filtering=True, timeout=CsmaTimeout.seconds_3,
                               slot_size=CsmaSlotSize.milliseconds_5, slot_number=CsmaSlotNumber.slots_7):

        mib_index = Mib.Index.csma_configuration.value

        byte_0 = (immediate_ack << 1) | (virtual_biu_after_rx << 2) | (biu_false_after_rx << 3) | \
                 (mac_filtering << 4) | (timeout.value << 5)

        byte_1 = (slot_size.value << 0) | (slot_number.value << 1)

        mib_val = [byte_0, byte_1]

        return Mib.Object(mib_index, mib_val)

    @staticmethod
    def mib_biu_calibration(led=True, observation_window=BiuObservationWindow.refresh_4ms, gain_compensation=0):

        mib_index = Mib.Index.biu_calibration.value

        byte_0 = (led << 2) | (observation_window.value << 4)

        if gain_compensation < -32 or gain_compensation > 31:
            raise Exception("Out of range")

        byte_1 = gain_compensation

        mib_val = [byte_0, byte_1]

        return Mib.Object(mib_index, mib_val)


class MacFrameType(IntEnum):
    rip = 0x00  # Repetition (upstream)
    nor1 = 0x01  # Normal 1
    nor2 = 0x05  # Normal 2
    crp = 0x03  # Repetition control (downstream)
    mask = 0x7  # 3 bits are used

    def __str__(self):
        return self.name


class Discipline(IntEnum):

    def __str__(self):
        return self.name


class NetworkManagement(IntEnum):
    upper_layer = 0
    network_management = 1

    def __str__(self):
        return self.name
