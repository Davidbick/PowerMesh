from enum import IntEnum, Enum, unique


@unique
class Protocol(IntEnum):
    G3 = 0
    MM = 1
    PRIME_1_3_6 = 2
    PRIME_1_4 = 3
    PRIME_BN_1_3_6 = 4
    PRIME_BN_1_4 = 5
    BOOT = 6

    def __str__(self):
        return self.name

    def dotted_string(self):
        if self == self.PRIME_1_3_6:
            return "PRIME 1.3.6"
        elif self == self.PRIME_1_4:
            return "PRIME 1.4"
        elif self == self.PRIME_BN_1_3_6:
            return "PRIME BN 1.3.6"
        elif self == self.PRIME_BN_1_4:
            return "PRIME BN 1.4"
        else:
            return self.name


@unique
class PathType(Enum):
    path_rx = 0

    path_tx = 1
    path_tx_2ohm = 2
    path_tx_spurious = 3

    path_rx_rf = 4
    path_tx_rf = 5

    def __str__(self):
        return self.name


@unique
class NodeMode(Enum):
    Transmitter = 0
    Receiver = 1

    def __str__(self):
        return self.name


@unique
class ChannelMode(Enum):
    PLC = 0
    RF = 1
    PLC_RF = 2

    def __str__(self):
        return self.name


@unique
class LowLvlDutyType(IntEnum):
    duty_cycle_50 = 0x00
    duty_cycle_40 = 0x40
    duty_cycle_30 = 0x80
    duty_cycle_20 = 0xC0

    def __str__(self):
        return self.name.replace("duty_cycle_", "") + "%"


@unique
class PLCBand(IntEnum):
    CEN_A = 0x00
    CEN_B = 0x01
    ARIB = 0x02
    FCC = 0x03
    FCC_LOW = 0x04
    FCC_HIGH = 0x05

    def __str__(self):
        return self.name.replace("band_", "")


@unique
class PrimeChannel(IntEnum):
    Channel1 = 0x01
    Channel2 = 0x02
    Channel3 = 0x04
    Channel4 = 0x08
    Channel34 = 0x0C  # Multichannel Channel3 | Channel4
    Channel5 = 0x10
    Channel45 = 0x18  # Multichannel Channel4 | Channel5
    Channel6 = 0x20
    Channel56 = 0x30  # Multichannel Channel5 | Channel6
    Channel7 = 0x40
    Channel67 = 0x60  # Multichannel Channel6 | Channel7
    Channel8 = 0x80
    Channel78 = 0xC0  # Multichannel Channel7 | Channel8

    def __str__(self):
        return self.name


@unique
class G3ModScheme(IntEnum):
    G3MOD_ROBO_DIFFERENTIAL = 0x00
    G3MOD_DBPSK_DIFFERENTIAL = 0x01
    G3MOD_DQPSK_DIFFERENTIAL = 0x02
    G3MOD_D8PSK_DIFFERENTIAL = 0x03
    G3MOD_ROBO_COHERENT = 0x04
    G3MOD_DBPSK_COHERENT = 0x05
    G3MOD_DQPSK_COHERENT = 0x06
    G3MOD_D8PSK_COHERENT = 0x07

    def __str__(self):
        return self.name.replace("G3MOD_", "")


@unique
class PrimeModulation(IntEnum):
    TYPEA_DBPSK = 0x00
    TYPEA_DQPSK = 0x01
    TYPEA_D8PSK = 0x02
    TYPEA_DBPSKCC = 0x04
    TYPEA_DQPSKCC = 0x05
    TYPEA_D8PSKCC = 0x06
    # TYPEA_DBPSKRC and TYPEA_DQPSKRC not defined in PRIME 1.4

    TYPEB_DBPSK = 0x10
    TYPEB_DQPSK = 0x11
    TYPEB_D8PSK = 0x12
    TYPEB_DBPSKCC = 0x14
    TYPEB_DQPSKCC = 0x15
    TYPEB_D8PSKCC = 0x16
    TYPEB_DBPSKRC = 0x1C
    TYPEB_DQPSKRC = 0x1D

    TYPEBC_DBPSK = 0x20
    TYPEBC_DQPSK = 0x21
    TYPEBC_D8PSK = 0x22
    TYPEBC_DBPSKCC = 0x24
    TYPEBC_DQPSKCC = 0x25
    TYPEBC_D8PSKCC = 0x26
    TYPEBC_DBPSKRC = 0x2C
    TYPEBC_DQPSKRC = 0x2D

    def __str__(self):
        return self.name


@unique
class StreamResult(IntEnum):
    test_ok = 0
    test_fail = 1

    def __str__(self):
        return self.name


@unique
class TestResult(Enum):
    passed = "PASSED"
    failed = "FAILED"
    error = "ERROR"
    empty = ""

    def __str__(self):
        return self.value


@unique
class TestFailReason(Enum):
    excessive_fer = "FER is too high"
    excessive_snr = "Necessary SNR is too high"
    insufficient_isr = "Endured ISR is too low"
    insufficient_sensitivity = "Sensitivity is too low"
    insufficient_rms = "RMS is too low"
    unexpected_band = "Measured band not in range"
    above_limit = "Signal is above the limit"
    excessive_power = "Measured power is too high"
    insufficient_current = "Measured current is too low"
    test_error = "Error during the test"
    integrity_fail = "HW integrity compromised"

    def __str__(self):
        return self.value


class CarrierFrequency:
    # PRIME
    CarrierNb = 97  # Number of carriers in each bandplan_channel
    bandwidth = 46875  # Channel bandwidth (Hz)
    CarrierFreqStepPRIME = 488.28125  # Channel_Spacing / (CarrierNb + 15 guard spacings)
    Channel_Spacing = 54687.5  # Includes the bandplan_channel bandwidth (Hz)

    FirstCarrierFreqChannel1 = 41992.1875  # Starting frequency of the bandplan_channel (Hz)
    LastCarrierFreqChannel1 = 88867.1875

    FirstCarrierFreqChannel2 = 96679.68750
    LastCarrierFreqChannel2 = 143554.6875

    FirstCarrierFreqChannel3 = 151367.1875
    LastCarrierFreqChannel3 = 198242.1875

    FirstCarrierFreqChannel4 = 206054.6875
    LastCarrierFreqChannel4 = 252929.6875

    FirstCarrierFreqChannel5 = 260742.1875
    LastCarrierFreqChannel5 = 307617.18750

    FirstCarrierFreqChannel6 = 315429.6875
    LastCarrierFreqChannel6 = 362304.6875

    FirstCarrierFreqChannel7 = 370117.1875
    LastCarrierFreqChannel7 = 416992.1875

    FirstCarrierFreqChannel8 = 424804.6875
    LastCarrierFreqChannel8 = 471679.6875

    # G3
    CarrierNbCENA = 36
    FirstCarrierFreqCENA = 35937.5
    CarrierFreqStepCENA = 1562.5  # = (LastCarrierFreqCENA - FirstCarrierFreqCENA)/(CarrierNbCENA-1)
    LastCarrierFreqCENA = 90625

    CarrierNbCENB = 16
    FirstCarrierFreqCENB = 98437.5
    CarrierFreqStepCENB = 1562.5  # = (LastCarrierFreqCENB - FirstCarrierFreqCENB)/(CarrierNbCENB-1)
    LastCarrierFreqCENB = 121875

    CarrierNbFCC = 72
    FirstCarrierFreqFCC = 154687.5
    CarrierFreqStepFCC = 4687.5  # = (LastCarrierFreqFCC - FirstCarrierFreqFCC)/(CarrierNbFCC-1)
    LastCarrierFreqFCC = 487500

    @staticmethod
    def convert_channel_to_bandplan(channel):
        if channel == 1:
            return PLCBand.band_CENA
        elif channel == 2:
            return PLCBand.band_CENB
        else:
            return PLCBand.band_FCC

    @staticmethod
    def get_carrier_nb(protocol, bandplan):
        if protocol == Protocol.G3:
            if bandplan == PLCBand.band_CENA:
                return CarrierFrequency.CarrierNbCENA
            elif bandplan == PLCBand.band_CENB:
                return CarrierFrequency.CarrierNbCENB
            elif bandplan == PLCBand.band_FCC:
                return CarrierFrequency.CarrierNbFCC
        elif protocol == Protocol.PRIME_1_4 or protocol == Protocol.PRIME_1_3_6:
            return CarrierFrequency.CarrierNb
        elif protocol == Protocol.MM:
            raise Exception("UNSUPPORTED PROTOCOL")
        else:
            raise Exception

    @staticmethod
    def get_start_freq(protocol, bandplan):
        if protocol == Protocol.G3:
            if bandplan == PLCBand.band_CENA:
                return CarrierFrequency.FirstCarrierFreqCENA
            elif bandplan == PLCBand.band_CENB:
                return CarrierFrequency.FirstCarrierFreqCENB
            elif bandplan == PLCBand.band_FCC:
                return CarrierFrequency.FirstCarrierFreqFCC
            else:
                raise Exception("UNKNOWN BANDPLAN")
        elif protocol == Protocol.PRIME_1_3_6:
            return CarrierFrequency.FirstCarrierFreqChannel1
        elif protocol == Protocol.PRIME_1_4:
            channel = bandplan
            if channel == PrimeChannel.Channel1:
                return CarrierFrequency.FirstCarrierFreqChannel1
            elif channel == PrimeChannel.Channel2:
                return CarrierFrequency.FirstCarrierFreqChannel2
            elif channel == PrimeChannel.Channel3 or channel == PrimeChannel.Channel34:
                return CarrierFrequency.FirstCarrierFreqChannel3
            elif channel == PrimeChannel.Channel4 or channel == PrimeChannel.Channel45:
                return CarrierFrequency.FirstCarrierFreqChannel4
            elif channel == PrimeChannel.Channel5 or channel == PrimeChannel.Channel56:
                return CarrierFrequency.FirstCarrierFreqChannel5
            elif channel == PrimeChannel.Channel6 or channel == PrimeChannel.Channel67:
                return CarrierFrequency.FirstCarrierFreqChannel6
            elif channel == PrimeChannel.Channel7 or channel == PrimeChannel.Channel78:
                return CarrierFrequency.FirstCarrierFreqChannel7
            elif channel == PrimeChannel.Channel8:
                return CarrierFrequency.FirstCarrierFreqChannel8
            else:
                raise Exception("UNKNOWN CHANNEL")
        elif protocol == Protocol.MM:
            raise Exception("UNSUPPORTED PROTOCOL")
        else:
            raise Exception

    @staticmethod
    def get_stop_freq(protocol, bandplan):
        if protocol == Protocol.G3:
            if bandplan == PLCBand.band_CENA:
                return CarrierFrequency.LastCarrierFreqCENA
            elif bandplan == PLCBand.band_CENB:
                return CarrierFrequency.LastCarrierFreqCENB
            elif bandplan == PLCBand.band_FCC:
                return CarrierFrequency.LastCarrierFreqFCC
        elif protocol == Protocol.PRIME_1_3_6:
            return CarrierFrequency.LastCarrierFreqChannel1
        elif protocol == Protocol.PRIME_1_4:
            channel = bandplan
            if channel == PrimeChannel.Channel1:
                return CarrierFrequency.LastCarrierFreqChannel1
            elif channel == PrimeChannel.Channel2:
                return CarrierFrequency.LastCarrierFreqChannel2
            elif channel == PrimeChannel.Channel3:
                return CarrierFrequency.LastCarrierFreqChannel3
            elif channel == PrimeChannel.Channel4 or channel == PrimeChannel.Channel34:
                return CarrierFrequency.LastCarrierFreqChannel4
            elif channel == PrimeChannel.Channel5 or channel == PrimeChannel.Channel45:
                return CarrierFrequency.LastCarrierFreqChannel5
            elif channel == PrimeChannel.Channel6 or channel == PrimeChannel.Channel56:
                return CarrierFrequency.LastCarrierFreqChannel6
            elif channel == PrimeChannel.Channel7 or channel == PrimeChannel.Channel67:
                return CarrierFrequency.LastCarrierFreqChannel7
            elif channel == PrimeChannel.Channel8 or channel == PrimeChannel.Channel78:
                return CarrierFrequency.LastCarrierFreqChannel8
            else:
                raise Exception("UNKNOWN CHANNEL")
        elif protocol == Protocol.MM:
            raise Exception("UNSUPPORTED PROTOCOL")
        else:
            raise Exception

    @staticmethod
    def get_freq_step(protocol, bandplan):
        if protocol == Protocol.G3:
            if bandplan == PLCBand.band_CENA:
                return CarrierFrequency.CarrierFreqStepCENA
            elif bandplan == PLCBand.band_CENB:
                return CarrierFrequency.CarrierFreqStepCENB
            elif bandplan == PLCBand.band_FCC:
                return CarrierFrequency.CarrierFreqStepFCC
        elif protocol == Protocol.PRIME_1_4 or protocol == Protocol.PRIME_1_3_6:
            return CarrierFrequency.CarrierFreqStepPRIME
        elif protocol == Protocol.MM:
            raise Exception("UNSUPPORTED PROTOCOL")
        else:
            raise Exception

    @staticmethod
    def get_bandwidth(protocol, bandplan):
        if protocol == Protocol.G3:
            if bandplan == PLCBand.band_CENA:
                return 58000
            elif bandplan == PLCBand.band_CENB:
                return 26000
            elif bandplan == PLCBand.band_FCC:
                return 335000
            else:
                raise Exception("Unknown bandplan")
        elif protocol == Protocol.PRIME_1_4 or protocol == Protocol.PRIME_1_3_6:
            return 50000
        else:
            raise Exception("Unsupported protocol")

    @staticmethod
    def get_nearest_carrier(protocol, bandplan, frequency):
        start = CarrierFrequency.get_start_freq(protocol, bandplan)
        step = CarrierFrequency.get_freq_step(protocol, bandplan)
        carrier_n = CarrierFrequency.get_carrier_nb(protocol, bandplan)

        carrier_found = start

        for i in range(0, carrier_n - 1):
            carrier = start + (i * step)
            middle = carrier + (step / 2)

            if frequency <= middle:
                carrier_found = carrier
                break
            else:
                carrier_found = carrier + step

        return carrier_found


class St8500Defs:
    ld_vcc_accepted_values = [10, 12, 15, 18]


class Platform(IntEnum):
    ST_INVALID = 0x00
    STCOMET = 0x01
    ST8500 = 0x02
    ECOM2 = 0x03
    ST75MM = 0x04
    ST_NOTSPECIFIED = 0xFF

    def __str__(self):
        return self.name


class SpiFlashOperation(IntEnum):
    erase = 0x00
    read = 0x01
    write = 0x02

    def __str__(self):
        return self.name


class SpiFlashDefine(IntEnum):
    SPI_FLASH_PAGE_SIZE = 256
    SPI_FLASH_PAGES_IN_SECTOR_CNT = 256
    SPI_FLASH_SECTORS_CNT = 32
    SPI_FLASH_SECTOR_SIZE = SPI_FLASH_PAGE_SIZE * SPI_FLASH_PAGES_IN_SECTOR_CNT
    SPI_FLASH_SIZE = SPI_FLASH_SECTORS_CNT * SPI_FLASH_SECTOR_SIZE

    def __str__(self):
        return self.name
