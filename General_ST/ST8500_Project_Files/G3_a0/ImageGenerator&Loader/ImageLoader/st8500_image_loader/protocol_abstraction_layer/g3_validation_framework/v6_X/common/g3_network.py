try:
    import sys
    import os.path
    from ctypes import *
    import ctypes
    from queue import *
    import queue
    from threading import *
    import threading
    from enum import IntEnum
    import time
    import platform
    import binascii
    import serial
    import libscrc
except:
    print("Error importing libraries in g3_network.py")
    raise ImportError

platform = platform.system()

my_path = os.path.abspath(os.path.dirname(__file__))

# Generic Definitions
class g3_hi_cmd_error_codes(IntEnum):
    SUCCESS = 0x00
    WRONG_CRC = 0x01
    WRONG_SYNCH = 0x02
    WRONG_LEN = 0x03
    WRONG_CMD_ID = 0x04
    HW_RST_WWDG = 0x05
    HW_RST_RTE_ERROR = 0x06
    HW_RST_RTE_NOT_ACTIVE = 0x07
    TIMOUT_ON_CMD = 0x08
    WRONG_SEC_MODE = 0x09
    WRONG_AUTH = 0x0A
    WRONG_CMD_IN_CURRENT_STATE = 0x0B
    WRROR_ON_INIT = 0x0C
    def __str__(self):
        return self.name

class g3_network_info(Structure):
    pass

class g3_data_buff_info(Structure):
    _fields_ = [("len", c_ulong),
                ("data", c_void_p)]
    def __init__(self, data, length):
        self.data = c_void_p(data)
        self.len = length

class g3_data_buff_node(Structure):
    pass

g3_data_buff_node._fields_ = [("next", POINTER(g3_data_buff_node)),
                              ("info", g3_data_buff_info)]

class g3_hi_gen_req(Structure):
    _pack_ = 1
    _fields_ = [("sync", c_ushort),
        ("cmd_id", c_ubyte),
        ("msg_len", c_ushort),
        ("sec_mode", c_ubyte),
        ("counter", c_ulong)]
    def __str__(self):
        return  "cmdID = " + str(g3_hi_cmd_ids(self.cmd_id)) +\
            " - msg_len = 0x{:04X}".format(self.msg_len)

class g3_hi_gen_conf(Structure):
    _pack_ = 1
    _fields_ = [("sync", c_ushort),
        ("cmd_id", c_ubyte),
        ("msg_len", c_ushort),
        ("sec_mode", c_ubyte),
        ("counter", c_ulong),
        ("ec", c_ubyte)]
    def __str__(self):
        return  "cmdID = " + str(g3_hi_cmd_ids(self.cmd_id)) +\
            " - msg_len = 0x{:04X}".format(self.msg_len) +\
            " - ec = " + str(g3_hi_cmd_error_codes(self.ec))

class g3_hi_msg_ending(Structure):
    _pack_ = 1
    _fields_ = [("crc", c_ushort)]

class g3_hi_msg_sec_ending(Structure):
    _pack_ = 1
    _fields_ = [("tag", c_ubyte * 8),
        ("crc", c_ushort)]

class g3_hi_testtype(IntEnum):
    NONE            = 0x00
    DATA_RATE       = 0x01
    INFINITE_LOOP   = 0x02
    SECURE_HI       = 0x03
    RF              = 0x04
    RFTXDELAY       = 0x05

class g3_hi_hwconfig_type(IntEnum):
    GPIO_CONFIG = 0x00
    GPIO_WRITE = 0x01
    GPIO_READ = 0x02
    BAUDRATE_CONFIG = 0x03
    S2LP_REG_WRITE = 0x04
    S2LP_REG_READ = 0x05

class g3_hi_rxtxconf(IntEnum):
    RX      = 0
    TX      = 1
    TX_RAND = 2

class g3_hi_platform(IntEnum):
    INVALID   = 0x00
    STCOMET   = 0x01
    ST8500    = 0x02
    UNKNOWN   = 0xFF
    def __str__(self):
        return self.name

class g3_hi_device_type(IntEnum):
    DEVICE   = 0x00
    COORD    = 0x01
    def __str__(self):
        return self.name

class g3_hi_mode(IntEnum):
    MODE_PHY        = 0x00
    MODE_MAC        = 0x01
    MODE_ADP        = 0x02
    MODE_BOOT       = 0x03
    MODE_IPV6_ADP   = 0x04
    MODE_IPV6_BOOT  = 0x05
    MODE_INVALID    = 0xFF
    def __str__(self):
        return self.name

class g3_hi_sec_mode(IntEnum):
    CLEAR_TEXT = 0
    CRYPT_GET_SET = 1
    CRYPT_ALL = 2

class g3_hi_dbgtool_ctrl(IntEnum):
    HI_DBG_INFO             = 0
    HI_DBG_USART_TRC_CONF   = 1
    HI_DBG_LED_CONF         = 2
    def __str__(self):
        return self.name

class g3_hi_trc_config_operation(IntEnum):
    SET_FILTER = 5
    def __str__(self):
        return self.name

class g3_hi_band_plan(IntEnum):
    CEN_A           = 0x00
    CEN_B           = 0x01
    ARIB            = 0x02
    FCC             = 0x03
    FCC_LOW         = 0x04
    FCC_HIGH        = 0x05
    def __str__(self):
        return self.name

class g3_hi_mod_scheme(IntEnum):
    DIFFERENTIAL    = 0x00
    COHERENT        = 0x01
    def __str__(self):
        return self.name

class g3_hi_two_rs_blocks(IntEnum):
    TWO_RS_BLOCKS   = 0x00
    ONE_RS_BLOCK    = 0x01
    def __str__(self):
        return self.name

class g3_hi_phy_trx_state(IntEnum):
    PHY_STATE_TRX   = 0x09
    PHY_STATE_RX    = 0x0A
    def __str__(self):
        return self.name    

class g3_hi_mod_scheme(IntEnum):
    DIFFERENTIAL    = 0x00
    COHERENT        = 0x01
    def __str__(self):
        return self.name

class g3_hi_phy_mod_type(IntEnum):
    PHY_MOD_TYPE_ROBO   = 0x00
    PHY_MOD_TYPE_DBPSK  = 0x01
    PHY_MOD_TYPE_DQPSK  = 0x02
    PHY_MOD_TYPE_D8PSK  = 0x03
    def __str__(self):
        return self.name

class g3_hi_phy_dt(IntEnum):
    NO_RESP     = 0x00
    RESP_EXP    = 0x01
    ACK         = 0x02
    NACK        = 0x03

class g3_hi_mac_addr_type(IntEnum):
    NO_ADDR     = 0x00
    MODE_16     = 0x02
    MODE_64     = 0x03
    def __str__(self):
        return self.name

class g3_hi_mac_tx_opt(IntEnum):
    ACK_REQ_OFF  = 0x00
    ACK_REQ_ON   = 0x01

class g3_hi_mac_sec_level(IntEnum):
    NO_SEC      = 0x00
    ENC_MIC32   = 0x05

class g3_hi_mac_adp_qos(IntEnum):
    PRIO_NORMAL = 0x00
    PRIO_HIGH   = 0x01
    def __str__(self):
        return self.name

class g3_hi_mac_media_type(IntEnum):
    MEDIATYPE_PLC_RFbkp = 0x00
    MEDIATYPE_RF_PLCbkp = 0x01
    MEDIATYPE_PLCRF = 0x02
    MEDIATYPE_PLC = 0x03
    MEDIATYPE_RF = 0x04
    def __str__(self):
        return self.name

class g3_hi_channel_config(IntEnum):
    CHANNEL_NONE = 0x00000000
    CHANNEL_PLC_ONLY = 0x000000003
    CHANNEL_RF_ONLY = 0x0000000C
    CHANNEL_ALL = 0x0000000F
    def __str__(self):
        return self.name

class g3_hi_mac_scan_type(IntEnum):
    ACTIVE = 0x01
    def __str__(self):
        return self.name

class g3_hi_boot_start_type(IntEnum):
    NORMAL = 0x00
    FAST_RESTORE = 0x01    
    def __str__(self):
        return self.name   

class g3_hi_event_notification_type(IntEnum):
    RTEError = 0x00
    GMKUpdate = 0x01
    ContextTableUpdate = 0x02
    GroupTableUpdate = 0x03
    ActiveKeyIndexUpdate = 0x04
    ShortAddrUpdate = 0x05
    PANIdUpdate = 0x06
    ToneMapTx = 0x07
    ToneMapRx = 0x08
    RouteUpdate = 0x09
    PathRequestRx = 0x0A
    ThermalEvent = 0x0B
    SurgeEvent = 0x0C
    RTEWarningEvent = 0x0D
    RTEPollingError = 0x0E
    BootFailure = 0x0F
    RTEBandMismatch = 0x10
    PHYQualityRX = 0x011
    PHYQualityTX = 0x12
    ExtAddrUpdate = 0x13
    MemLeak = 0x14
    TimerError = 0x15
    PHYQualityRX_RF = 0x16
    PHYQualityTX_RF = 0x17
    ADPDataFW = 0x18
    def __str__(self):
        return self.name     

class g3_hi_nvm_section(IntEnum):
    ALL = 0x00,
    GEN = 0x01,
    PLC = 0x02,
    PLCC = 0x03,
    RF = 0x04,
    LAST = 0x05

class g3_hi_nvm_operation(IntEnum):
    ERASE = 0x00
    READ = 0x01
    WRITE = 0x02
    SAVE = 0x03
    TAB_INIT = 0x10
    TAB_ERASE = 0x11
    TAB_READ_ENTRY = 0x12
    TAB_WRITE_ENTRY = 0x13
    TAB_ADD_ENTRY = 0x14
    TAB_RESET_ENTRY = 0x15
    TAB_GET_ENTRY = 0x16
    TAB_SET_CONNECTED = 0x17
    TAB_SET_DISCONNECTED = 0x18
    TAB_IS_CONNECTED = 0x19
    DEBUG_TRACE = 0x1A
    OPERATION_LAST = 0x1B
     
class g3_hi_metric_type(IntEnum):
    LOADng_HOPCNT = 0x00
    LOADng_CAPACITY = 0x0E
    LOADng_G3 = 0x0F

class g3_hi_udp_app_type(IntEnum):
    UDP_NORMAL_ID = 0x00
    UDP_RSP_APP_UDP_REQUEST_ID = 0x01
    UDP_RSP_APP_UDP_RESPONSE_ID = 0x02
    UDP_RSP_APP_ECHO_REQUEST_ID = 0x03
    UDP_RSP_RFCONFIGCHANGERQ_CMD = 0x05
    UDP_RSP_RFCONFIGCHANGECONF_CMD = 0x06
    UDP_RSP_DENSENETDISABLE_CMD = 0x07
    UDP_RSP_DENSENETDISABLECONF_CMD = 0x08
    UDP_RSP_RFCONTTXCONF_CMD = 0x09
    UDP_RSP_LASTGASPENREQ_CMD = 0x0A
    UDP_RSP_APP_RHI_REQ_ID = 0x80
    UDP_RSP_APP_RHI_RSP_ID = 0x81
    UDP_RSP_APP_AUTOPING_ID = 0x82
    UDP_RSP_APP_AUTOUDPRSP_ID = 0x84
    UDP_RSP_APP_SEND_NO_RSP_ID = 0x86
    UDP_RSP_APP_UDP_REQUEST_LEN_ID = 0x88
    UDP_RSP_APP_UDP_RESPONSE_LEN_ID = 0x89 

class g3_hi_otp_operation(IntEnum):
    READ = 0x00,
    WRITE = 0x01
    def __str__(self):
        return self.name

class g3_hi_otp_region(IntEnum):
    USER_SERCURITY_FIELD = 0x00,
    PE_KEY_FIELD = 0x01,
    USER_DATA_OR_ADDITIONAL_KEY_FIELD = 0x02
    def __str__(self):
        return self.name

class g3_hi_otp_status(IntEnum):
    OK = 0x00,
    NOK = 0x01,
    NOTOK = 0x02,
    WRONG_REGION = 0x03
    def __str__(self):
        return self.name

class g3_hi_sflash_operation(IntEnum):
    ERASE = 0x00,
    READ = 0x01,
    WRITE = 0x02,
    WRITE_AND_EVAL_CRC = 0x03

class g3_hi_sflash_define(IntEnum):
    SPI_FLASH_PAGE_SIZE = 256
    SPI_FLASH_PAGES_IN_SECTOR_CNT = 256
    SPI_FLASH_SECTORS_CNT = 32
    SPI_FLASH_SECTOR_SIZE = SPI_FLASH_PAGE_SIZE * SPI_FLASH_PAGES_IN_SECTOR_CNT
    SPI_FLASH_SIZE = SPI_FLASH_SECTORS_CNT * SPI_FLASH_SECTOR_SIZE
    SPI_FLASH_DEBUG_TRACE_SECTOR_HEADER_LEN = 8

class g3_hi_adp_discovery_route(IntEnum):
    FALSE = 0x00
    TRUE = 0x01

class g3_hi_sniffer_frame_type(IntEnum):
    G3LIB_SNIFFER_PHY_DATA = 0x00
    G3LIB_SNIFFER_PHY_ACK = 0x01
    G3LIB_SNIFFER_MAC_BEACON = 0x02
    G3LIB_SNIFFER_MAC_DATA = 0x03
    G3LIB_SNIFFER_MAC_CMD = 0x04
    G3LIB_SNIFFER_PHY_RF = 0x05
    G3LIB_SNIFFER_MAC_RF_ACK = 0x06
    G3LIB_SNIFFER_UNKNOWN = 0xFF

class g3_hi_loadng_msg_type(IntEnum):
    RREQ = 0x00
    RREP = 0x01
    RERR = 0x02
    PREQ = 0xFC
    PREP = 0xFD
    RLCREQ = 0xFE
    RLCREP = 0xFF
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_loadng_error_code(IntEnum):
    NO_AVAILABLE_ROUTE = 0x00
    HOP_LIMIT_REACHED = 0x01
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_mac_optional_features(IntEnum):
    MAC_OPTIONALFEATURES_SETTMR = 0x00000001
    MAC_OPTIONALFEATURES_TMRESPENCR = 0x00000002
    MAC_OPTIONALFEATURES_TMREXTADDR = 0x00000004
    MAC_OPTIONALFEATURES_TMRESPPRIO = 0x00000008
    MAC_OPTIONALFEATURES_EXPEXTADDRS = 0x00000010
    MAC_OPTIONALFEATURES_IGNOREBCNREQ = 0x00000020
    MAC_OPTIONALFEATURES_STOPTMR = 0x00000040
    MAC_OPTIONALFEATURES_NOCOMMIND_PANCONFLICT = 0x00000080
    MAC_OPTIONALFEATURES_NOCOMMIND_SECURITYERR = 0x00000100
    MAC_OPTIONALFEATURES_NOCOMMIND_OTHER = 0x00000200
    MAC_OPTIONALFEATURES_PANCONFLICTFILTER  = 0x00000400
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_adp_optional_features(IntEnum):
    ADP_OPTFEAT_ALLOWINVALIDHOPSLEFT   = 0x00000001
    ADP_OPTFEAT_MULTFWDISABLED         = 0x00000002
    ADP_OPTFEAT_NOACK                  = 0x00000004
    ADP_OPTFEAT_RREQFWDISABLED         = 0x00000008
    ADP_OPTFEAT_RERRHOPSLEFTZERO       = 0x00000010
    ADP_OPTFEAT_DISCOVERYROUTECOST     = 0x00000020
    ADP_OPTFEAT_DISABLEPANIDCONFONBOOT = 0x00000040
    ADP_OPTFEAT_TRICKLEHOPSCONSISTENCY = 0x00000080
    ADP_OPTFEAT_TRICKLEILQI            = 0x00000100
    ADP_OPTFEAT_MCASTFW_PrFWD          = 0x10000000
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_g3lib_event_indications(IntEnum):
    G3_LIB_PEEVENTINDICATION_ERROR            = 0x0000000000000001
    G3_LIB_PEEVENTINDICATION_GMK              = 0x0000000000000002
    G3_LIB_PEEVENTINDICATION_CONTEXT          = 0x0000000000000004
    G3_LIB_PEEVENTINDICATION_GROUP            = 0x0000000000000008
    G3_LIB_PEEVENTINDICATION_ACTKEY           = 0x0000000000000010
    G3_LIB_PEEVENTINDICATION_PANID            = 0x0000000000000020
    G3_LIB_PEEVENTINDICATION_SHORTADDR        = 0x0000000000000040
    G3_LIB_PEEVENTINDICATION_THERMEV          = 0x0000000000000080
    G3_LIB_PEEVENTINDICATION_TMRTX            = 0x0000000000000100
    G3_LIB_PEEVENTINDICATION_TMRRX            = 0x0000000000000200
    G3_LIB_PEEVENTINDICATION_ROUTE            = 0x0000000000000400
    G3_LIB_PEEVENTINDICATION_PREQ             = 0x0000000000000800
    G3_LIB_PEEVENTINDICATION_SURGEEV          = 0x0000000000001000
    G3_LIB_PEEVENTINDICATION_RTEWARN          = 0x0000000000002000
    G3_LIB_PEEVENTINDICATION_RTEPOLL          = 0x0000000000004000
    G3_LIB_PEEVENTINDICATION_BOOTFAILURE      = 0x0000000000008000
    G3_LIB_PEEVENTINDICATION_BANDMISMATCH     = 0x0000000000010000
    G3_LIB_PEEVENTINDICATION_PHYQUALITY_RX    = 0x0000000000020000
    G3_LIB_PEEVENTINDICATION_PHYQUALITY_TX    = 0x0000000000040000
    G3_LIB_PEEVENTINDICATION_EXTADDR          = 0x0000000000080000
    G3_LIB_PEEVENTINDICATION_MEMLEAK          = 0x0000000000100000
    G3_LIB_PEEVENTINDICATION_TIMERERR         = 0x0000000000200000
    G3_LIB_PEEVENTINDICATION_PHYQUALITY_RX_RF = 0x0000000000400000
    G3_LIB_PEEVENTINDICATION_PHYQUALITY_TX_RF = 0x0000000000800000
    G3_LIB_PEEVENTINDICATION_ADPDATA_FW       = 0x0000000001000000
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_defines(IntEnum):
    PHY_TXCOEFF_SIZE_FCC        = 6
    PHY_TONEMAP_SIZE            = 3
    PHY_TONEMASK_SIZE           = 9
    PHY_NUM_OF_CARRIERS         = 72
    MAC_KEY_SIZE                = 16
    MAC_ADDR64_SIZE             = 8
    ADP_EAP_PSK_KEY_LEN         = 16
    ADP_EAP_PSK_ID_MAX_LEN      = 36
    ADP_CONTEXT_MAX_LEN         = 16
    ADP_PREFIX_MAX_LEN          = 16
    ADP_MAX_HOPS_SUPPORTED      = 15
    ADP_MAX_HOPS_DEFAULT        = 8
    NVM_TAB_NUMNODEMAX          = 2048
    NVM_IDP_MAX_SIZE            = 21
    NVM_PLC_ATTR_SIZE           = 436
    MAC_BRD_ADDR                = 0xFFFF
    MAC_BRD_PANID               = 0xFFFF
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name


class g3_hi_cmd_ids(IntEnum):
    HWRESET_REQ             = 0x00
    HWRESET_CONF            = 0x01
    MODESET_REQ             = 0x02
    MODESET_CONF            = 0x03
    MODEGET_REQ             = 0x04
    MODEGET_CONF            = 0x05
    BAUDRATESET_REQ         = 0x06
    BAUDRATESET_CONF        = 0x07
    RESETSTATE_REQ          = 0x08
    RESETSTATE_CONF         = 0x09
    TESTSET_REQ             = 0x0A
    TESTSET_CONF            = 0x0B
    TESTGET_REQ             = 0x0C
    TESTGET_CONF            = 0x0D
    SFLASH_REQ              = 0x0E
    SFLASH_CONF             = 0x0F
    NVM_REQ                 = 0x10
    NVM_CONF                = 0x11
    TRACE_REQ               = 0x12
    TRACE_CONF              = 0x13
    DBGTOOL_REQ             = 0x14
    DBGTOOL_CONF            = 0x15
    SEC_INIT_REQ            = 0x16
    SEC_INIT_CONF           = 0x17
    RFCONFIGSET_REQ         = 0x18
    RFCONFIGSET_CONF        = 0x19
    RFCONFIGGET_REQ         = 0x1A
    RFCONFIGGET_CONF        = 0x1B
    HWCONFIG_REQ            = 0x1C
    OTP_REQ                 = 0x1D
    OTP_CONF                = 0x1E
    HWCONFIG_CONF           = 0x1F
    G3LIB_GET_REQ           = 0x20
    G3LIB_GET_CONF          = 0x21
    G3LIB_SET_REQ           = 0x22
    G3LIB_SET_CONF          = 0x23
    G3LIB_SWRESET_REQ       = 0x24
    G3LIB_SWRESET_CONF      = 0x25
    G3LIB_EVENT_NOTIF_IND   = 0x28
    G3LIB_MULTIPLEGET_REQ   = 0x29
    G3LIB_MULTIPLEGET_CONF  = 0x2A
    G3LIB_SNIFFER_IND       = 0x2B
    G3PHY_DATA_REQ          = 0x40
    G3PHY_DATA_CONF         = 0x41
    G3PHY_DATA_IND          = 0x42
    G3PHY_ACK_REQ           = 0x43
    G3PHY_ACK_CONF          = 0x44
    G3PHY_ACK_IND           = 0x45
    G3PHY_RF_REQ            = 0x46
    G3PHY_RF_CONF           = 0x47
    G3PHY_RF_IND            = 0x48
    G3PHY_RSV1              = 0x49
    G3PHY_SETTRXSTATE_REQ   = 0x4A
    G3PHY_SETTRXSTATE_CONF  = 0x4B
    G3PHY_CS_REQ            = 0x4C
    G3PHY_CS_CONF           = 0x4D
    G3MAC_DATA_REQ          = 0x60
    G3MAC_DATA_CONF         = 0x61
    G3MAC_DATA_IND          = 0x62
    G3MAC_BCNNOTIFY_IND     = 0x63
    G3MAC_RESET_REQ         = 0x68
    G3MAC_RESET_CONF        = 0x69
    G3MAC_SCAN_REQ          = 0x6A
    G3MAC_SCAN_CONF         = 0x6B
    G3MAC_COMMSTATUS_IND    = 0x6C
    G3MAC_START_REQ         = 0x6D
    G3MAC_START_CONF        = 0x6E
    G3ADP_DATA_REQ          = 0x80
    G3ADP_DATA_CONF         = 0x81
    G3ADP_DATA_IND          = 0x82
    G3ADP_DISCOVERY_REQ     = 0x83
    G3ADP_DISCOVERY_CONF    = 0x84
    G3ADP_NETSTART_REQ      = 0x85
    G3ADP_NETSTART_CONF     = 0x86
    G3ADP_NETJOIN_REQ       = 0x87
    G3ADP_NETJOIN_CONF      = 0x88
    G3ADP_NETLEAVE_REQ      = 0x89
    G3ADP_NETLEAVE_CONF     = 0x8A
    G3ADP_NETLEAVE_IND      = 0x8B
    G3ADP_RESET_REQ         = 0x8C
    G3ADP_RESET_CONF        = 0x8D
    G3ADP_NETSTATUS_IND     = 0x92
    G3ADP_ROUTEDISC_REQ     = 0x93
    G3ADP_ROUTEDISC_CONF    = 0x94
    G3ADP_PATHDISC_REQ      = 0x95
    G3ADP_PATHDISC_CONF     = 0x96
    G3ADP_LBP_REQ           = 0x97
    G3ADP_LBP_CONF          = 0x98
    G3ADP_LBP_IND           = 0x99
    G3ADP_ROUTEOVER_REQ     = 0x9A
    G3ADP_ROUTEDELETE_REQ   = 0x9B
    G3ADP_ROUTEDELETE_CONF  = 0x9C
    G3BOOT_SRV_START_REQ    = 0xB0
    G3BOOT_SRV_START_CONF   = 0xB1
    G3BOOT_SRV_STOP_REQ     = 0xB2
    G3BOOT_SRV_STOP_CONF    = 0xB3
    G3BOOT_SRV_LEAVE_IND    = 0xB4
    G3BOOT_SRV_KICK_REQ     = 0xB5
    G3BOOT_SRV_KICK_CONF    = 0xB6
    G3BOOT_SRV_JOIN_IND     = 0xB7
    G3BOOT_DEV_START_REQ    = 0xB8
    G3BOOT_DEV_START_CONF   = 0xB9
    G3BOOT_DEV_LEAVE_REQ    = 0xBA
    G3BOOT_DEV_LEAVE_CONF   = 0xBB
    G3BOOT_DEV_LEAVE_IND    = 0xBC
    G3BOOT_DEV_PANSORT_IND  = 0xC1
    G3BOOT_DEV_PANSORT_REQ  = 0xC2
    G3BOOT_DEV_PANSORT_CONF = 0xC3
    G3BOOT_SRV_GETPSK_IND   = 0xC4
    G3BOOT_SRV_SETPSK_REQ   = 0xC5
    G3BOOT_SRV_SETPSK_CONF  = 0xC6
    G3UDP_DATA_REQ          = 0xD0
    G3UDP_DATA_CONF         = 0xD1
    G3UDP_DATA_IND          = 0xD2
    G3UDP_CONN_SET_REQ      = 0xD3
    G3UDP_CONN_SET_CONF     = 0xD4
    G3UDP_CONN_GET_REQ      = 0xD5
    G3UDP_CONN_GET_CONF     = 0xD6
    G3ICMP_ECHO_REQ         = 0xD7
    G3ICMP_ECHO_CONF        = 0xD8
    G3ICMP_ECHOREP_IND      = 0xD9
    G3UDP_APP_REQ           = 0xDA
    G3UDP_APP_CONF          = 0xDB
    G3UDP_APP_IND           = 0xDC
    G3ICMP_ECHOREQ_IND      = 0xDF    
    ERROR_IND               = 0xFF
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_results(IntEnum):
    SUCCESS = 0x00
    WRONG_CRC = 0x01
    WRONG_MSG_LEN = 0x02
    WRONG_CMD_ID = 0x03
    WRONG_MODE = 0x04
    NOT_EXPECTED = 0x05
    WRONG_STATE = 0x06
    WRONG_TRX_STATE = 0x07
    NOT_ACCEPTABLE = 0x08
    WRONG_TX_POWER = 0x09
    WRONG_MOD_SCHEME = 0x0A
    WRONG_MOD_TYPE = 0x0B
    WRONG_PREEMPHASIS = 0x0C
    WRONG_TONEMAP = 0x0D
    WRONG_TONEMASK = 0x0E
    WRONG_FCH = 0x0F
    WRONG_VALUE_LEN = 0x10
    WRONG_TEST_MODE = 0x11
    QUEUE_FULL = 0x12
    DATA_LIMIT_REACHED = 0x13
    RESERVERD_CMD_ID = 0x14
    RTE_INTERFACE_TIMEOUT = 0x15
    TEST_FAILED = 0x16
    TEST_NOT_ACTIVE = 0x17
    TEST_ALREADY_ACTIVE = 0x18
    TEST_NOT_IMPLEMENTED = 0x19
    TEST_NOT_IDLE = 0x1A
    TEST_NOT_ACCEPTABLE = 0x1B
    FAILED = 0x1C
    TX_BUSY = 0x1D
    RX_BUSY = 0x1E
    CS_IDLE = 0x1F
    CS_BUSY = 0x20
    PREVIOUS_REQUEST_FAILED = 0x21
    NO_INDICATION = 0x22
    PLME_CS_CONF_TMR_EXP = 0x23
    PLME_SET_TRX1_KO = 0x24
    PLME_SET_TRX1_TMR_EXP = 0x25
    PLME_SET_TRX2_KO = 0x26
    PLME_SET_TRX2_TMR_EXP = 0x27
    PLME_SET_TRX3_KO = 0x28
    PLME_SET_TRX3_TMR_EXP = 0x29
    PLME_SET_KO = 0x2A
    PLME_SET_TMR_EXP = 0x2B
    PD_DATA_CONF_FAILED = 0x2C
    PD_DATA_CONF_TMR_EXP = 0x2D
    SEGMENT_DISCARDED = 0x2E
    WRONG_VALUE = 0x2F
    WRONG_ATTR_NUM = 0x30
    WRONG_DT = 0x31
    ACTION_NOT_IMPLEMENTED = 0x32
    ACTION_UNKNOWN = 0x33
    DT_ACKREQ_MISMATCH = 0x34
    PLME_GET_CONF_TMR_EXP = 0x35
    PD_ACK_CONF_FAILED = 0x36
    PD_ACK_REQ_TMR_EXP = 0x37
    RX_NACK = 0x38
    ERR_SEG_COUNT = 0x39
    ERR_SEG_ID = 0x3A
    ERR_SEG_CC = 0x3B
    ERR_SEG_AR = 0x3C
    TX_QUEUE_TO_SHORT = 0x3D
    NO_EARLIER_SEG = 0x3E
    ERR_SEQ_NUM = 0x3F
    INTERNAL_ERROR_1 = 0x40
    SEGMENT_TOO_LATE = 0x41
    WRONG_TWORSBLOCKS = 0x42
    NOT_IDLE = 0x43
    WRONG_BAND = 0x44
    NOT_SUPPORTED_BAND = 0x45
    NOT_ACCEPTABLE_LEN = 0x46
    PLC_DISABLED = 0x47
    PHY_NOT_READY = 0x48
    WRONG_LEN = 0x49
    BANDSETOK_ATTRSETKO = 0x4A
    G3_INTERNAL_ERROR_2 = 0x4B
    G3_INTERNAL_ERROR_3 = 0x4C
    G3_INTERNAL_ERROR_4 = 0x4D
    BUSY = 0x60
    INVALID_IPv6_FRAME = 0x61
    INVALID_QOS = 0x62
    BAD_CCM_OUTPUT = 0x63
    ROUTE_ERROR = 0x64
    BT_TABLE_FULL = 0x65
    FRAME_NOT_BUFFERED = 0x66
    STARTUP_FAILURE = 0x67
    NOT_PERMITTED = 0x68
    UNKNOWN_DEVICE = 0x69
    SENDING_DATA_ERROR = 0x6A
    PARAMETER_ERROR = 0x6B
    WRONG_DEVICE_TYPE = 0x6C
    ADD_EVENT_ERROR = 0x6D
    GET_EVENT_ERROR = 0x6E
    MALLOC_FAIL = 0x6F
    RTEIPC_ERROR = 0x70
    WRONG_MEDIA_TYPE = 0x71
    UNAVAILABLE_MEDIA_TYPE = 0x72    
    DUTY_CYCLE_ERR = 0x73
    DISABLED_MEDIA_TYPE = 0x74
    RF_DISABLED = 0x75
    JOINING_DECLINE = 0x76
    JOINING_TIMEOUT = 0x77
    COUNTER_ERROR = 0xDB
    IMPROPER_KEY_TYPE = 0xDC
    IMPROPER_SECURITY_LEVEL = 0xDD
    UNSUPPORTED_LEGACY = 0xDE
    UNSUPPORTED_SECURITY = 0xDF
    BEACON_LOSS = 0xE0
    CHANNEL_ACCESS_FAILURE = 0xE1
    DENIED = 0xE2
    DISABLE_TRX_FAILURE = 0xE3
    SECURITY_ERROR = 0xE4
    FRAME_TOO_LONG = 0xE5
    INVALID_GTS = 0xE6
    INVALID_HANDLE = 0xE7
    INVALID_PARAMETER = 0xE8
    NO_ACK = 0xE9
    NO_BEACON = 0xEA
    NO_DATA = 0xEB
    NO_SHORT_ADDRESS = 0xEC
    OUT_OF_CAP = 0xED
    PAN_ID_CONFLICT = 0xEE
    REALIGNMENT = 0xEF
    TRANSACTION_EXPIRED = 0xF0
    TRANSACTION_OVERFLOW = 0xF1
    TX_ACTIVE = 0xF2
    UNAVAILABLE_KEY = 0xF3
    UNSUPPORTED_ATTRIBUTE = 0xF4
    INVALID_ADDRESS = 0xF5
    ON_TIME_TOO_LONG = 0xF6
    PAST_TIME = 0xF7
    TRACKING_OFF = 0xF8
    INVALID_INDEX = 0xF9
    LIMIT_REACHED = 0xFA
    READ_ONLY = 0xFB
    SCAN_IN_PROGRESS = 0xFC
    SUPERFRAME_OVERLAP = 0xFD
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name

class g3_hi_attrib_id(IntEnum):
    ADP_SECURITY_LEVEL                     = 0x0000
    ADP_PREFIX_TABLE                       = 0x0001
    ADP_BROADCAST_LOG_TABLE_ENTRY_TTL      = 0x0002
    ADP_METRIC_TYPE                        = 0x0003
    ADP_LOW_LQI_VALUE                      = 0x0004
    ADP_HIGH_LQI_VALUE                     = 0x0005
    ADP_RREP_WAIT                          = 0x0006
    ADP_CONTEXT_INFORMATION_TABLE          = 0x0007
    ADP_COORD_SHORT_ADDRESS                = 0x0008
    ADP_RLC_ENABLED                        = 0x0009
    ADP_ADDR_EV_LINK_COST                  = 0x000A
    ADP_BROADCAST_LOG_TABLE                = 0x000B
    ADP_ROUTING_TABLE                      = 0x000C
    ADP_UNICAST_RREQ_GENENABLE             = 0x000D
    ADP_GROUP_TABLE                        = 0x000E
    ADP_MAX_HOPS                           = 0x000F
    ADP_DEVICE_TYPE                        = 0x0010
    ADP_NET_TRAVERSAL_TIME                 = 0x0011
    ADP_ROUTING_TABLE_ENTRY_TTL            = 0x0012
    ADP_KR                                 = 0x0013
    ADP_KM                                 = 0x0014
    ADP_KC                                 = 0x0015
    ADP_KQ                                 = 0x0016
    ADP_KH                                 = 0x0017
    ADP_RREQ_RETRIES                       = 0x0018
    ADP_RREQ_RERR_WAIT_TIME                = 0x0019
    ADP_WEAK_LQI_VALUE                     = 0x001A
    ADP_KRT                                = 0x001B
    ADP_SOFT_VERSION                       = 0x001C
    ADP_SNIFFER_MODE                       = 0x001D
    ADP_BLACKLIST_TABLE                    = 0x001E
    ADP_BLACKLIST_TABLE_ENTRY_TTL          = 0x001F
    ADP_MAX_JOIN_WAIT_TIME                 = 0x0020
    ADP_PATH_DISCOVERY_TIME                = 0x0021
    ADP_ACTIVE_KEY_INDEX                   = 0x0022
    ADP_DESTINATION_ADDRESS_SET            = 0x0023
    ADP_DEAFULT_COORDINATOR_ROUTE_ENABLE   = 0x0024
    ADP_DELAYLOWLQI_ID                     = 0x0025
    ADP_DELAYHIGHLQI_ID                    = 0x0026
    ADP_RREQJITTERLOWLQI_ID                = 0x0027
    ADP_RREQJITTERHIGHLQI_ID               = 0x0028
    ADP_TRICKLEDATAENABLED_ID              = 0x0029
    ADP_TRICKLELQITHRESHOLDLOW_ID          = 0x002A
    ADP_TRICKLESTEP_ID                     = 0x002B
    ADP_TRICKLEFACTORIMAX_ID               = 0x002C
    ADP_TRICKLEIMIN_ID                     = 0x002D
    ADP_TRICKLEMAXKI_ID                    = 0x002E
    ADP_TRICKLEADAPTIVEIMIN_ID             = 0x002F
# Uncomment for 6.3 version
    # ADP_TRICKLEADAPTIVEKI_ID               = 0x0030
    # ADP_CLUSTERTRICKLEENABLED_ID           = 0x0031
    # ADP_CLUSTERMINLQI_ID                   = 0x0032
    # ADP_CLUSTERK_ID                        = 0x0033
    # ADP_CLUSTERRREQRCDEVIATION_ID          = 0x0034
    # ADP_CLUSTERTRICKLEI_ID                 = 0x0035
    # ADP_TRICKLELQITHRESHOLDHIGH_ID         = 0x0036
# Uncomment for 6.2 version
    ADP_CLUSTERTRICKLEENABLED_ID           = 0x0030
    ADP_CLUSTERMINLQI_ID                   = 0x0031
    ADP_CLUSTERK_ID                        = 0x0032
    ADP_CLUSTERRREQRCDEVIATION_ID          = 0x0033
    ADP_CLUSTERTRICKLEI_ID                 = 0x0034
# / 6.2 version
    ADP_DISABLE_DEFAULT_ROUTING            = 0x00F0
    ADP_DATA_GRAM_TAG                      = 0x0090
    ADP_BROADCAST_SEQ_NUM                  = 0x0091
    ADP_ENABLE_HEADER_COMPRESSION          = 0x0092
    ADP_EAPPSKRAND_ID                      = 0x0093
    ADP_EAP_PSK_KEY                        = 0x0094
    ADP_EAP_PSK_IDP                        = 0x0095
    ADP_EAP_PSK_IDP_LEN                    = 0x0096
    ADP_MAX_REKEY_ING_WAIT_TIME            = 0x0097
    ADP_LOAD_NG_SEQ_NUM                    = 0x0098
    ADP_OPTIONAL_FEATURES                  = 0x0099
    ADP_REASSEMBLY_TIMEOUT                 = 0x009A
    ADP_RETX_TIME_FOR_CHBUSY               = 0x009B
    ADP_HOPS_FOR_DELAY_FRAG_N              = 0x009C
    ADP_DELAY_FRAG_N                       = 0x009D
    ADP_DELAY_FRAG_N_BROAD                 = 0x009E
    ADP_ROUTING_TABLE_BY_SHORT_ADDR        = 0x009F
    ADP_ROUTING_TABLE_VALID_ENTRY          = 0x00A0
    ADP_UNICAST_RX_FRAG_CNT                = 0x00A1
    ADP_UNICAST_TX_FRAG_CNT                = 0x00A2
    ADP_UNICAST_FW_FRAG_CNT                = 0x00A3
    ADP_UNICAST_IND_FRAG_CNT               = 0x00A4
    ADP_MCASTRXFRAGCNT_ID                  = 0x00A5
    ADP_MCAST_TX_FRAG_CNT                  = 0x00A6
    ADP_MCASTFWFRAGCNT_ID                  = 0x00A7
    ADP_MCASTINDFRAGCNT_ID                 = 0x00A8
    ADP_MCAST_FW_THRESHOLDS1               = 0x00A9
    ADP_MCAST_FW_THRESHOLDS2               = 0x00AA
    ADP_RETRYFORCHBUSY_ID                  = 0x00AB
    ADP_RREQRXCNT_ID                       = 0x00AC
    ADP_RREQFWDCNT_ID                      = 0x00AD
    ADP_RREQOVERWRITECNT_ID                = 0x00AE
    ADP_RSV1_ID                            = 0x00AF
    ADP_MEMORYLEAKTIMEOUT_ID               = 0x00B0
    ADP_HOPSDELAYFRAGNRF_ID                = 0x00B1
    ADP_DELAYFRAGNRF_ID                    = 0x00B2
    ADP_LOWLQIVALUE_RF_ID                  = 0x00D0
    ADP_HIGHLQIVALUE_RF_ID                 = 0x00D1
    ADP_KQ_RF_ID                           = 0x00D2
    ADP_KH_RF_ID                           = 0x00D3
    ADP_KRT_RF_ID                          = 0x00D4
    ADP_KDC_RF_ID                          = 0x00D5
    ADP_USEBACKUPMEDIA_ID                  = 0x00D6
    ADP_WEAKLQIVALUE_RF_ID                 = 0x00D7
    ADP_TRICKLEMINLQIVALUE_RF_ID           = 0x00D8
    ADP_DELAYLOWLQI_RF_ID                  = 0x00D9
    ADP_DELAYHIGHLQI_RF_ID                 = 0x00DA
    ADP_RREQJITTERLOWLQI_RF_ID             = 0x00DB
    ADP_RREQJITTERHIGHLQI_RF_ID            = 0x00DC
    ADP_CLUSTERTRICKLEI_RF_ID              = 0x00DD
    ADP_CLUSTERTRICKLEK_RF_ID              = 0x00DE
    ADP_CLUSTERMINLQI_RF_ID                = 0x00DF
    ADP_LASTGASP_ID                        = 0x00E0
    ADP_PROBINGINTERVAL_ID                 = 0x00E1
# Uncomment for 6.3 version
    # ADP_TRICKLELQITHHIGH_RF_ID             = 0x00E2
    MAC_RESERVED1_ID                       = 0x0040
    MAC_MAXBE_ID                           = 0x0047
    MAC_BSN_ID                             = 0x0049
    MAC_DSN_ID                             = 0x004C
    MAC_MAXCSMABACKOFFS_ID                 = 0x004E
    MAC_MINBE_ID                           = 0x004F
    MAC_PANID_ID                           = 0x0050
    MAC_PROMISCUOUSMODE_ID                 = 0x0051
    MAC_SHORTADDRESS_ID                    = 0x0053
    MAC_MAXFRAMERETRIES_ID                 = 0x0059
    MAC_TIMESTAMPSUPP_ID                   = 0x005C
    MAC_SECURITYENABLED_ID                 = 0x005D
    MAC_KEYTABLE_ID                        = 0x0071
    MAC_FRAMECOUNTER_ID                    = 0x0077
    MAC_DUPLICATEDETECTIONTTL_ID           = 0x0078
    MAC_HIGHPRIORITYWS_ID                  = 0x0100
    MAC_TXDATAPACKETCOUNT_ID               = 0x0101
    MAC_RXDATAPACKETCOUNT_ID               = 0x0102
    MAC_TXCMDPACKETCOUNT_ID                = 0x0103
    MAC_RXCMDPACKETCOUNT_ID                = 0x0104
    MAC_CSMAFAILCOUNT_ID                   = 0x0105
    MAC_CSMANOACKCOUNT_ID                  = 0x0106
    MAC_RXDATABROADCASTCOUNT_ID            = 0x0107
    MAC_TXDATABROADCASTCOUNT_ID            = 0x0108
    MAC_BADCRCCOUNT_ID                     = 0x0109
    MAC_NEIGHBORTABLE_ID                   = 0x010A
    MAC_RESERVED2_ID                       = 0x010B
    MAC_CSMAFAIRNESSLIMIT_ID               = 0x010C
    MAC_TMRTTL_ID                          = 0x010D
    MAC_POSTABLEENTRYTTL_ID                = 0x010E
    MAC_RCCOORD_ID                         = 0x010F
    MAC_TONEMASK_ID                        = 0x0110
    MAC_BCNRANDWINLEN_ID                   = 0x0111
    MAC_A_ID                               = 0x0112
    MAC_K_ID                               = 0x0113
    MAC_MINCWATTEMPTS_ID                   = 0x0114
    MAC_CENELECLEGACYMODE_ID               = 0x0115
    MAC_FCCLEGACYMODE_ID                   = 0x0116
    MAC_BROADCASTMAXCWENABLE_ID            = 0x011E
    MAC_TRANSMITATTEN_ID                   = 0x011F
    MAC_POSTABLE_ID                        = 0x0120
    MAC_POSRECENTENTRYTHRESHOLD_ID         = 0x0121
    MAC_POSRECENTENTRIES_ID                = 0x0122
    MAC_PREAMBLELENGHT_ID                  = 0x0124 
    MAC_DSN_RF_ID                          = 0X0200
    MAC_MAXBE_RF_ID                        = 0X0201
    MAC_MAXCSMABACKOFFS_RF_ID              = 0X0202
    MAC_MAXFRAMERETRIES_RF_ID              = 0X0203
    MAC_MINBE_RF_ID                        = 0X0204
    MAC_TIMESTAMPSUPPORTED_RF_ID           = 0X0205
    MAC_DEVICETABLE_RF_ID                  = 0X0206
    MAC_FRAMECOUNTER_RF_ID                 = 0X0207
    MAC_DUPLICATEDETECTIONTTL_RF_ID        = 0X0208
    MAC_COUNTEROCTETS_RF_ID                = 0X0209
    MAC_RETRYCOUNT_RF_ID                   = 0X020A
    MAC_MULTIPLERETRYCOUNT_RF_ID           = 0X020B
    MAC_TXFAILCOUNT_RF_ID                  = 0X020C
    MAC_TXSUCCESSCOUNT_RF_ID               = 0X020D
    MAC_FCSERRORCOUNT_RF_ID                = 0X020E
    MAC_SECURITYFAILURE_RF_ID              = 0X020F
    MAC_DUPLICATEFRAMECOUNT_RF_ID          = 0X0210
    MAC_RXSUCCESSCOUNT_RF_ID               = 0X0211
    MAC_EBRPERMITJOINING_RF_ID             = 0X0213
    MAC_EBRFILTERS_RF_ID                   = 0X0214
    MAC_EBRATTRIBUTELIST_RF_ID             = 0X0215
    MAC_BEACONAUTORESPOND_RF_ID            = 0X0216
    MAC_USEENHANCEDBEACON_RF_ID            = 0X0217
    MAC_EBHEADERIELIST_RF_ID               = 0X0218
    MAC_EBPAYLOADIELIST_RF_ID              = 0X0219
    MAC_EBFILTERINGENABLED_RF_ID           = 0X021A
    MAC_EBSN_RF_ID                         = 0X021B
    MAC_EBAUTOSA_RF_ID                     = 0X021C
    MAC_POSTABLE_RF_ID                     = 0X021D
    MAC_OPERATINGMODE_RF_ID                = 0X021E
    MAC_CHANNELNUMBER_RF_ID                = 0X021F
    MAC_DUTYCYCLEUSAGE_RF_ID               = 0X0220
    MAC_DUTYCYCLEPERIOD_RF_ID              = 0X0221
    MAC_DUTYCYCLELIMIT_RF_ID               = 0X0222
    MAC_DUTYCYCLETHRESHOLD_RF_ID           = 0X0223
    MAC_RESERVED7_ID                       = 0X0224
    MAC_FREQUENCYBAND_RF_ID                = 0x0225
    MAC_TRANSMITATTEN_RF_ID                = 0x0226
    MAC_ADAPTIVEPOWERSTEP_RF_ID            = 0x0227
    MAC_ADAPTIVEPOWERHIGHBOUND_RF_ID       = 0x0228
    MAC_ADAPTIVEPOWERLOWBOUND_RF_ID        = 0x0229
    MAC_POSRECENTENTRIES_RF_ID             = 0x022A
    MAC_HOPPINGENABLED_RF_ID               = 0x022B
    MAC_BROADCASTINTERVALDURATION_RF_ID    = 0x022C
    MAC_BROADCASTSLOTDURATION_RF_ID        = 0x022D
    MAC_UNICASTSLOTDURATION_RF_ID          = 0x022E
    MAC_EXTENDEDBITMAP_RF_ID               = 0x022F
    MAC_BCNRANDWINLEN_RF_ID                = 0x0230
    MAC_ADDITIONALCHSCANTIME_RF_ID         = 0x0231
    MAC_MAXCCAATTEMPSRETRIES_RF_ID         = 0x0232
    MAC_MININTERTXINTERVAL_RF_ID           = 0x0233
    MAC_INITIALRETRYTIME_RF_ID             = 0x0234
    MAC_MAXIMUMRETRYTIME_RF_ID             = 0x0235
    MAC_MAXCLOCKDRIFT_RF_ID                = 0x0236
    MAC_FHUNICASTSCHEDULEADDRESS_RF_ID     = 0x0237
    MAC_FHUNICASTSLOTNUMBER_RF_ID          = 0x0238
    MAC_FHUNICASTOFFSET_RF_ID              = 0x0239
    MAC_MINGUARDTIME_RF_ID                 = 0x023A
    MAC_MAXBCASTRESYNCHWAITUNIT_RF_ID      = 0x023B
    BOOT_DEVICE_DISCOVERY_TIME             = 0x1000
    BOOT_DEVICE_RESERVED1                  = 0x1001
    BOOT_DEVICE_RESERVED2                  = 0x1002
    BOOT_DEVICE_RESERVED3                  = 0x1003
    BOOT_DEVICE_RESERVED4                  = 0x1004
    BOOT_DEVICE_RESERVED5                  = 0x1005
    BOOT_DEVICE_RESERVED6                  = 0x1006
    BOOT_DEVICE_LQI_THRESHOLD              = 0x1007
    BOOT_DEVICE_SART_WAIT_TIME             = 0x1008
    BOOT_DEVICE_OPTIONS                    = 0x1009
    BOOT_DEVICE_ASSOCIATION_MAX_RETRIES    = 0x100A
    BOOT_DEVICE_ASSOCIATION_RAND_WAIT_TIME = 0x100B
    BOOT_DEVICE_ASSOCIATION_ALT_PAN        = 0x100C
    BOOT_DEVICE_LAST_USED_LBA              = 0x100D
    BOOT_DEVICE_STATE                      = 0x100F
    BOOT_SERVER_START_WAIT_TIME            = 0x1010
    BOOT_SERVER_DEFAULT_MAX_HOPS           = 0x1011
    BOOT_SERVER_DISCOVERY_TIME             = 0x1012
    BOOT_SERVER_FORCE_PAN_START            = 0x1013
    BOOT_SERVER_IDS                        = 0x1014
    BOOT_SERVER_IDS_LEN                    = 0x1015
    BOOT_DEVICE_PAN_SORT_TIMEOUT           = 0x1016
    BOOT_SERVER_PSK_GET_TIMEOUT            = 0x1017
    BOOT_SERVER_JOINING_IGNORE_TIME        = 0x1018
    BOOT_SERVER_JOINING_TABLE_ENTRY_TTL    = 0x1019
    MAC_EXTENDEDADDRESS_ID                 = 0x3000 
    MAC_RESERVED3_ID                       = 0x3001 
    MAC_TRXSTATENOTCORRECT_ID              = 0x3002 
    MAC_PSDUTOLONG_ID                      = 0x3003 
    MAC_INVALIDFRAMETYPE_ID                = 0x3004 
    MAC_INVALIDFRAMEVERSION_ID             = 0x3005 
    MAC_INVALIDDAM_ID                      = 0x3006 
    MAC_INVALIDSAM_ID                      = 0x3007 
    MAC_INVALIDPANID_ID                    = 0x3008 
    MAC_INVALIDSHORTADDR_ID                = 0x3009 
    MAC_INVALIDEXTADDR_ID                  = 0x300A 
    MAC_INVALIDBEACON_ID                   = 0x300B 
    MAC_INVALIDDATACMD_ID                  = 0x300C 
    MAC_INTERNALSTATENOTCORRECT_ID         = 0x300D 
    MAC_FRAMEDISCARDED_ID                  = 0x300E 
    MAC_ACKTRXSTATENOTCORRECT_ID           = 0x300F 
    MAC_ACKNOTCORRECT_ID                   = 0x3010 
    MAC_ACKINTSTATENOTCORRECT_ID           = 0x3011 
    MAC_ACKCRCNOTEXPECTED_ID               = 0x3012 
    MAC_ACKDISCARDED_ID                    = 0x3013 
    MAC_CSMASEED_ID                        = 0x3014 
    MAC_INVALIDFRAMEPENDING_ID             = 0x3015 
    MAC_INVALIDPIC_ID                      = 0x3016 
    MAC_PSDUTOOSHORT_ID                    = 0x3017 
    MAC_CNTFIXDT_ID                        = 0x3018 
    MAC_DISABLEPLC_ID                      = 0x3019 
    MAC_STARTLOWERINGRETRIES_ID            = 0x301A 
    MAC_TIMERWABEND_ID                     = 0x301B 
    MAC_DONTSENDNACK_ID                    = 0x301C
    MAC_ENABLECCTT230_ID                   = 0x301D
    MAC_LOWERINGBEHAVIOR_ID                = 0x301E 
    G3_LIB_RTEFWVERSION_ID                 = 0x3100
    G3_LIB_RTEPROTOVERSION_ID              = 0x3101
    G3_LIB_RTEDRQUEUESIZE_ID               = 0x3102
    G3_LIB_RTETXPACKETCOUNT_ID             = 0x3103
    G3_LIB_RTERXPACKETCOUNT_ID             = 0x3104
    G3_LIB_RTEMODE_ID                      = 0x3105
    G3_LIB_RTECAPABILITIES_ID              = 0x3106
    G3_LIB_RTEVALIDATIONTEST_ID            = 0x3107
    G3_LIB_RTEMAXNUMSEGFRAME_ID            = 0x3108
    G3_LIB_RTEGPIO_ID                      = 0x3109
    G3_LIB_PEFWVERSION_ID                  = 0x3200
    G3_LIB_PEPROTOVERSION_ID               = 0x3201
    G3_LIB_PECHANNELCONFIG_ID              = 0x3202
    G3_LIB_PEMODE_ID                       = 0x3203
    G3_LIB_PEOPTIONS_ID                    = 0x3204
    G3_LIB_PECURRENTTIME_ID                = 0x3205
    G3_LIB_PEMEMPOOLMAXALLOCS_ID           = 0x3206
    G3_LIB_PEMEMPOOLCURRALLOCS_ID          = 0x3207
    G3_LIB_PETOTALSIGNALPOWER_ID           = 0x3208
    G3_LIB_PEEVENTINDICATION_ID            = 0x3209
    G3_LIB_PEPOLLRTEINTERVAL_ID            = 0x320A
    G3_LIB_PEPOLLRTETIMEOUT_ID             = 0x320B
    G3_LIB_PEPOLLRTEEXPNUM_ID              = 0x320C
    G3_LIB_RESERVED1_ID                    = 0x320D
    G3_LIB_PESNIFFERMODE_ID                = 0x320E
    MAC_SNRDBROBO_ID                       = 0x3280 
    MAC_SNRDBDBPSK_ID                      = 0x3281 
    MAC_SNRDBDQPSK_ID                      = 0x3282 
    MAC_SNRDBD8PSK_ID                      = 0x3283 
    MAC_RXBCNLASTSCAN_ID                   = 0x3284 
    MAC_MODULATIONCONTROL_ID               = 0x3285 
    MAC_OPTIONALFEATURES_ID                = 0x3286 
    MAC_TMRMMODULATIONCONTROL_ID           = 0x3287 
    MAC_TMRMINNUMCARRIER_ID                = 0x3288 
    MAC_SNRDBSUPERROBO_ID                  = 0x3289 
    MAC_MAXTXPOWER_ID                      = 0x328A 
    MAC_DATATXTIMEOUT_ID                   = 0x328B 
    MAC_DEVICETABLE_ID                     = 0x328C 
    MAC_NEIGHBORTABLE_BYSHORT_ID           = 0x328D 
    MAC_NEIGHBORTABLE_VALIDENTRY_ID        = 0x328E 
    MAC_SNRDB_USEDIFF8PSK_ID               = 0x328F 
    MAC_POSTABLE_VALIDENTRY_ID             = 0x3290 
    MAC_RESERVED4_ID                       = 0x3291 
    MAC_RESERVED5_ID                       = 0x3292 
    MAC_RESERVED6_ID                       = 0x3293 
    MAC_IMPULSIVENOISELOWERING_ID          = 0x3294 
    MAC_DELAYTXAFTERTMR_ID                 = 0x3295 
    MAC_TMRLQILIMITS_ID                    = 0x3296
    MAC_DEVICETABLE_BYSHORT_ID             = 0x3297
    MAC_DEVICETABLE_VALIDENTRY_ID          = 0x3298
    MAC_ACKTIMEOUT_RF_ID                   = 0x3299
    MAC_MAXBACKOFFTIME_RF_ID               = 0x329A
    MAC_POSTABLE_VALIDENTRY_RF_ID          = 0x329B
    MAC_DEVICETABLE_BYSHORT_RF_ID          = 0x329C
    MAC_DEVICETABLE_VALIDENTRY_RF_ID       = 0x329D
    MAC_PANIDCONFLICTTABLETTL_ID           = 0x329E
    MAC_DATAREQTIMEOUT_ID                  = 0x329F
    MAC_PANIDCONFLICTTABLETTL_RF_ID        = 0x32A0
    MAC_NOISE_RF_ID                        = 0x32A1
    MAC_PHYQUALITYRXTABLE_ID               = 0x32A2
    MAC_PHYQUALITYRXTABLE_BYSHORT_ID       = 0x32A3
    MAC_PHYQUALITYRXTABLETTL_ID            = 0x32A4
    MAC_PHYQUALITYRXTABLE_VALIDENTRY_ID    = 0x32A5
    PHY_CRC_ERR_PACKET_COUNT               = 0x4000
    PHY_CRC_FAIL_PACKET_COUNT 			   = 0x4001
    PHY_LAST_RECEIVED_PACKET_MODE 		   = 0x4002
    PHY_ZC_PERIOD 				           = 0x4003
    PHY_LAST_RECEIVED_PACKET_LOCAL_PDC 	   = 0x4004
    PHY_IS_FIXED_PDC 				       = 0x4005
    PHY_PDC_VALUE 				           = 0x4006
    PHY_RS_FAIL_PACKET_COUNT 			   = 0x4007
    PHY_LAST_RECEIVED_PACKET_RS_STATUS 	   = 0x4008
    PHY_LAST_RECEIVED_PACKET_DT 	       = 0x4009
    PHY_LAST_RECEIVED_PACKET_PDC 		   = 0x400A
    PHY_LAST_RECEIVED_PACKET_TM 		   = 0x400B
    PHY_FREE_CHANNEL_TIMESTAMP 			   = 0x400C
    PHY_TX_POWER_OVERRIDE 				   = 0x400D
    PHY_LAST_RX_VALID_FRAME_FCH 		   = 0x400E
    PHY_LAST_TX_FCH 				       = 0x400F
    PHY_AGC_RESET_TIMEOUT 				   = 0x4010
    PHY_CENELEC_LEGACY_MODE 			   = 0x4011
    PHY_RS_FILTER_DISABLED 				   = 0x4012
    PHY_THR_PCS 				           = 0x4013
    PHY_ZC_DELAY_COMPENSATION 			   = 0x4014
    PHY_TONE_MASK 				           = 0x4015
    PHY_FCC_LEGACY_MODE 				   = 0x4016
    PHY_THR_SYNC_P 				           = 0x4017
    PHY_THR_SYNC_M 				           = 0x4018
    PHY_PREAMBLE_LENGTH 				   = 0x4019
    PHY_LAST_RECEIVED_PACKET_AGC 		   = 0x401A
    PHY_AGC_DISABLED 				       = 0x401B
    PHY_PGA_GAIN 				           = 0x401C
    PHY_CURRENT_AGC 				       = 0x401D
    PHY_LAST_FL 				           = 0x401E
    PHY_ACK_TX_POWER 				       = 0x401F
    PHY_POWER_EST_TRIM 				       = 0x4020
    PHY_CURRENT_SENSE_MAX_THRESHOLD 	   = 0x4021
    PHY_IS_FIXED_DT 				       = 0x4022
    PHY_DT_VALUE 				           = 0x4023
    PHY_LD_VCC 				               = 0x4024
    PHY_BGKN_EST_NORM1 				       = 0x4025
    PHY_THERMAL_STATUS 				       = 0x4026
    PHY_CURRENT_SENSE_SPIKES_FILTER 	   = 0x4027
    PHY_POWER_AMPLIFIER_SELECT 			   = 0x4028
    PHY_DIGITAL_SCALING_FACTOR 			   = 0x4029
    PHY_STATIC_PREEMPHASIS 				   = 0x402A
    PHY_MIN_SYNC_PVARIANCE 				   = 0x402B
    PHY_MIN_LINEAR_PGA 				       = 0x402C
    PHY_AGC_RESET_TIMEOUT_MIN 			   = 0x402D
    PHY_AGC_RESET_TIMEOUT_MAX 			   = 0x402E
    PHY_NOISE_LOW_MEAN 				       = 0x402F
    PHY_NOISE_HIGH_MEAN 				   = 0x4030
    RF_RSSINOISEINTERVAL                   = 0x5000
    RF_RSSINOISEMARGINRX                   = 0x5001
    RF_DISABLEPHY                          = 0x5002
    RF_RSSINOISEMARGINTX                   = 0x5003
    RF_FREQOFFSET                          = 0x5004
    RF_RSSINOISEEWMAALPHA                  = 0x5005
    RF_WMBUSENABLE                         = 0x5006
    RF_ACTIVESTATES_MAXTIME                = 0x5007
    RF_INITNOISE                           = 0x5008
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.name
        
class g3_hi_msg_conf_base(Structure):
    _pack_ = 1

class g3_hi_msg_conf_base_with_del(Structure):
    _pack_ = 1
    def __del__(self):
        pass

class g3_hi_thread_killer:
    def __init__(self):
        self.killed = False
        self.buried = False
    def thread_kill(self):
        self.killed = True
    def thread_killed(self):
        return self.killed
    def thread_bury(self):
        self.buried = True
    def thread_buried(self):
        return self.buried

def g3_hi_is_ctypes_base_type(data):
    ret = True
    
    try:
        sizeof(data)
    except TypeError:
        ret = False
    
    if isinstance(data, ctypes.Array):
        ret = False
    
    return ret

def g3_hi_int_to_ctype(val, new_type):
    if (g3_hi_is_ctypes_base_type(val)):
        return val
    else:
        return new_type(val)

def g3_hi_vec_to_cbyte_vec(vec):
    return (c_ubyte * len(vec))(*vec)

def g3_hi_memmove_checked(data_out_, data_in_, size):
    if (data_in_ == None) or (size == 0):
        return
    data_out = data_out_
    data_in = data_in_
    if (not isinstance(data_out_, ctypes.Array) and not issubclass(type(data_out_), (Structure, Union))):
        data_out = g3_hi_vec_to_cbyte_vec(data_out_)
    if (not isinstance(data_in_, ctypes.Array)  and not issubclass(type(data_in_), (Structure, Union))):
        data_in = g3_hi_vec_to_cbyte_vec(data_in_)
    if (sizeof(data_out) < size) or (sizeof(data_in) < size):
        raise ValueError
    memmove(byref(data_out), byref(data_in), size)

def g3_hi_get_data_str(data, inv = True):
    data_str = ''.join('{:X}'.format(data))
    data_str_len = len(data_str)
    if (data_str_len % 2) > 0:
        data_str_len += 1
    return ''.join(('{:0' + str(data_str_len) + 'X}').format(data))

def g3_hi_get_msg_after_header_len(msg, sec_enabled):
    msg_len = msg.msg_len + sizeof(g3_hi_msg_ending)
    tag_len = sizeof(g3_hi_msg_sec_ending) - sizeof(g3_hi_msg_ending)

    if True == sec_enabled:
        msg_len += tag_len

    return msg_len

def g3_hi_conf_is_to_cipher(msg):
    sec_init_conf_type2_len = sizeof(g3_hi_msg_sec_init_req) + 1
    sec_mode = msg.sec_mode

    if ((g3_hi_cmd_ids.HWRESET_CONF == msg.cmd_id) or
        (g3_hi_cmd_ids.RESETSTATE_CONF == msg.cmd_id) or
        (g3_hi_cmd_ids.TESTSET_CONF == msg.cmd_id) or
        (g3_hi_cmd_ids.ERROR_IND == msg.cmd_id)):
        return False

    if ((g3_hi_sec_mode.CRYPT_ALL == sec_mode) or
        ((g3_hi_sec_mode.CRYPT_GET_SET == sec_mode) and ((g3_hi_cmd_ids.G3LIB_GET_CONF == msg.cmd_id) or
        (g3_hi_cmd_ids.G3LIB_SET_CONF == msg.cmd_id) or ((g3_hi_cmd_ids.SEC_INIT_CONF == msg.cmd_id) and
        (sec_init_conf_type2_len == msg.msg_len))))):
        return True

    return False

def g3_hi_msg_get_conf_len(msg):
	if g3_hi_conf_is_to_cipher(msg):
		return g3_hi_get_msg_after_header_len(msg, True)
	else:
		return g3_hi_get_msg_after_header_len(msg, False)

def g3_network_send(ser, req):
    #print(str(ser.name) + " Snd cmd " + str(g3_hi_cmd_ids(req[2])))
    #print(str(ser.name) + " Snd cmd " + ''.join('{:02X}'.format(x) for x in req))
    ser.write(req)

def g3_network_receive(ser,killer):
    sync_byte = 0x16
    sync_len = 2
    # here we use the structure of the req, because the ec fields is considered part of the payload
    header_len = sizeof(g3_hi_gen_req) - sync_len

    vec = []
    to_read_len = sync_len

    while not killer.thread_killed():
        while to_read_len > 0 and not killer.thread_killed():
            v = ser.read(to_read_len)
            to_read_len -= len(v)
            vec += v

        if not killer.thread_killed():
            if len(vec) >= 2 and vec[0] == sync_byte and vec[1] == sync_byte:
                break
            elif len(vec) >= 2 and vec[1] == sync_byte:
                vec = [sync_byte]
                to_read_len = sync_len - 1
            else:
                vec = []
                to_read_len = sync_len

    if killer.thread_killed():
        # killed
        raise StopIteration

    to_read_len = header_len

    v = ser.read(to_read_len)

    if len(v) != to_read_len:
        print("Wrong len 1")
        print(''.join('{:02X}'.format(x) for x in v))
        raise IOError

    vec += v

    c_vec = (c_ubyte * len(vec)).from_buffer_copy(bytes(vec))

    conf_temp = cast(byref(c_vec), POINTER(g3_hi_gen_req))

    conf = conf_temp.contents

    payload_len = g3_hi_msg_get_conf_len(conf)

    v = ser.read(payload_len)

    if len(v) != payload_len:
        print("Wrong len 2")
        print(''.join('{:02X}'.format(x) for x in v))
        raise IOError

    vec += v

    vec_len = len(vec)

    crc_calc = libscrc.xmodem(bytes(vec[:vec_len - 2]))
    crc_found = vec[vec_len - 2] | (vec[vec_len - 1] << 8)

    if (crc_calc != crc_found):
        print("Wrong CRC on ser calc {:04X} found {:04X}".format(crc_calc, crc_found))
        print(''.join('{:02X}'.format(x) for x in vec))
        raise IOError

    #print(str(ser.name) + " Rcv cmd " + str(g3_hi_cmd_ids(vec[2])))
    #print(''.join('{:02X}'.format(x) for x in vec))
    return (c_ubyte * vec_len).from_buffer_copy(bytes(vec))

def g3_network_worker(ser, q_sel, q_vec, killer):
    while not killer.thread_killed():
        try:
            msg = g3_network_receive(ser, killer)

            q_selected = q_sel(msg)
            if q_selected < len(q_vec):
                q_vec[q_selected].put(msg)
            else:
                q_vec[0].put(msg)

        except IOError:
            time.sleep(0.01)
            continue
        except StopIteration:
            break

    killer.thread_bury()

def g3_network_receive_thread_start(ser, q_sel, q_vec, killer):
    if (len(q_vec) < 1):
        raise ValueError
    t = Thread(target = g3_network_worker, args = (ser, q_sel, q_vec, killer))
    t.setDaemon(True)
    t.start()
    return t

def g3_network_get_received_msg(net_info, q, to = 15):
    try:
        ret = q.get(timeout = to)
        q.task_done()
    except queue.Empty:
        return None
    return ret

def g3_hi_msg_header_set(msg, cmd_id, msg_len, sec_mode, counter):
    msg.sync = 0x1616
    msg.cmd_id = cmd_id
    msg.msg_len = msg_len
    msg.sec_mode = sec_mode
    msg.counter = counter

def g3_hi_carray_cat(a, b):
    ret = None

    if (a is None) and (b is None):
        raise ValueError
    elif (not isinstance(a, ctypes.Array)) or (not isinstance(b, ctypes.Array)):
        raise ValueError

    if a is None:
        ret = b
    elif b is None:
        ret = a
    else:
        ret = (c_ubyte * (len(a) + len(b)))(*a, *b)

    return ret

def g3_hi_msg_cat(*args):
    ret = None

    for a in args:
        if isinstance(a, ctypes.Array):
            if ret is None:
                ret = a
            else:
                ret = (c_ubyte * (len(ret) + len(a)))(*ret, *a)
        elif isinstance(a, Structure):
            tmp = (c_ubyte * sizeof(a))()
            memmove(tmp, byref(a), sizeof(a))
            if ret is None:
                ret = tmp
            else:
                ret = g3_hi_carray_cat(ret, tmp)
        else:
            raise ValueError

    return ret

def g3_hi_get_complete_msg(msg, length, sec_mode):
    new_len = length
    if g3_hi_sec_mode.CLEAR_TEXT == sec_mode:
        complete_msg = g3_hi_msg_cat(msg, g3_hi_msg_ending())
    else:
        complete_msg = g3_hi_msg_cat(msg, g3_hi_msg_sec_ending())
        new_len = length + sizeof(g3_hi_msg_sec_ending().tag)
    crc = libscrc.xmodem(bytes(complete_msg[:len(complete_msg) - 2]))
    complete_msg[new_len] = crc & 0xFF
    complete_msg[new_len + 1] = (crc >> 8) & 0xFF
    return complete_msg

def g3_hi_req_strip_hi_header(msg):
    class hi_msg(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("footer", c_ubyte * (sizeof(msg) - sizeof(g3_hi_gen_req)))]
        def __init__(self, msg):
            memmove(byref(self), byref(msg), sizeof(msg))
    return hi_msg(msg).footer

def g3_hi_conf_strip_hi_header(msg):
    class hi_msg(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("footer", c_ubyte * (sizeof(msg) - sizeof(g3_hi_gen_conf)))]
        def __init__(self, msg):
            memmove(byref(self), byref(msg), sizeof(msg))
    return hi_msg(msg).footer

def g3_hi_hexstr_to_int(hex_str):
    return int(hex_str, base=16)

def g3_hi_str_to_cbyte_vec(string):
    byte_str = binascii.unhexlify(string)
    vec = [x for x in byte_str]
    return g3_hi_vec_to_cbyte_vec(vec)

def g3_hi_str_to_list(string):
    byte_str = binascii.unhexlify(string)
    return [x for x in byte_str]  

def g3_hi_vec_equal(a_vec, b_vec):
    if type(a_vec) == type(None) and type(b_vec) == type(None):
        return True
    if type(a_vec) == type(None) or type(b_vec) == type(None):
        return False
    vec_len = len(a_vec)
    if (vec_len != len(b_vec)):
        return False
    for i in range(0, vec_len):
        if (a_vec[i] != b_vec[i]):
            return False
    return True

def g3_hi_cvec_equal(a_vec, b_vec):
    if type(a_vec) == type(None) and type(b_vec) == type(None):
        return True
    if type(a_vec) == type(None) or type(b_vec) == type(None):
        return False
    if not isinstance(a_vec, ctypes.Array):
        a_cvec = g3_hi_vec_to_cbyte_vec(a_vec)
        a_cvec_len = sizeof(a_cvec)
    else:
        a_cvec = a_vec
        a_cvec_len = sizeof(a_vec)
    if not isinstance(b_vec, ctypes.Array):
        b_cvec = g3_hi_vec_to_cbyte_vec(b_vec)
        b_cvec_len = sizeof(b_cvec)
    else:
        b_cvec = b_vec
        b_cvec_len = sizeof(b_vec)
    if (a_cvec_len != b_cvec_len):
        return False
    for a, b in zip(a_cvec, b_cvec):
        if (a != b):
            return False
    return True

def g3_hi_vec_is_contained(a_vec, b_vec, offset_in_b=0):
    if type(a_vec) == type(None) and type(b_vec) == type(None):
        return True
    if type(a_vec) == type(None) or type(b_vec) == type(None):
        return False
    if len(a_vec) > len(b_vec) - offset_in_b:
        return False
    vec_len = len(a_vec)
    for i in range(0, vec_len):
        if (a_vec[i] != b_vec[i+offset_in_b]):
            return False
    return True

def g3_hi_vec_is_prefix(a_vec, b_vec):
    return g3_hi_vec_is_contained(a_vec, b_vec, 0)

#TODO: remove it from this file
def g3_hi_msg_get_g3lib_get_pan_id(get_conf_msg):
    if (g3_hi_attrib_id.MAC_PANID_ID != get_conf_msg.id.id):
        raise ValueError
    pan_id = get_conf_msg.value[0] | (get_conf_msg.value[1] << 8)
    return pan_id

def g3_hi_ctype2int(len, value_in, endianness ="little"):
    if isinstance(value_in, ctypes.Array):
        if (len == 1):        
            value = value_in[0]
        if (len == 2):
            if endianness == "little":
                value = value_in[0] | (value_in[1] << 8)
            elif endianness == "big":
                value = value_in[1] | (value_in[0] << 8)
            else:
                raise ValueError
        if (len == 3):
            if endianness == "little":
                value = value_in[0] | (value_in[1] << 8) | (value_in[2] << 16)
            elif endianness == "big":
                value = value_in[2] | (value_in[1] << 8) | (value_in[0] << 16)
            else:
                raise ValueError            
        if (len == 4):
            if endianness == "little":
                value = value_in[0] | (value_in[1] << 8) | (value_in[2] << 16) | (value_in[3] << 24)
            elif endianness == "big":
                value = value_in[3] | (value_in[2] << 8) | (value_in[1] << 16) | (value_in[0] << 24)
            else:
                raise ValueError
        if (len == 8):
            if endianness == "little":
                value = value_in[0] | (value_in[1] << 8) | (value_in[2] << 16) | (value_in[3] << 24) | (value_in[4] << 32) | (value_in[5] << 40) | (value_in[6] << 48) |  (value_in[7] << 56)
            elif endianness == "big":
                value = value_in[7] | (value_in[6] << 8) | (value_in[5] << 16) | (value_in[4] << 24) | (value_in[3] << 32) | (value_in[2] << 40) | (value_in[1] << 48) |  (value_in[0] << 56)
            else:
                raise ValueError

        return int(value)
    else:
        return int(value_in.value)

def g3_hi_int2ctype(len, value_in, endianness ="little"):
    value_out = (c_ubyte * len)()
    # special case: array of bytes    
    if isinstance(value_in, list):
        for idx in range(len):
            value_out[idx] = value_in[idx]
    elif isinstance(value_in, bytes):
        for idx in range(len):
            value_out[idx] = value_in[idx]
    elif len == 0x01:
        value_out[0] = value_in
    elif len == 0x02:
        if endianness == "little":
            value_out[0] = value_in & 0xFF
            value_out[1] = (value_in >> 8) & 0xFF
        elif endianness == "big":
            value_out[0] = (value_in >> 8) & 0xFF
            value_out[1] = value_in & 0xFF
        else:
            raise ValueError
    elif len == 0x04:
        if endianness == "little":
            value_out[0] = value_in & 0xFF
            value_out[1] = (value_in >> 8) & 0xFF
            value_out[2] = (value_in >> 16) & 0xFF
            value_out[3] = (value_in >> 24) & 0xFF
        elif endianness == "big":
            value_out[0] = (value_in >> 24) & 0xFF
            value_out[1] = (value_in >> 16) & 0xFF
            value_out[2] = (value_in >> 8) & 0xFF
            value_out[3] = value_in & 0xFF
        else:
            raise ValueError
    elif len == 0x08:
        if endianness == "little":
            value_out[0] = value_in & 0xFF
            value_out[1] = (value_in >> 8) & 0xFF
            value_out[2] = (value_in >> 16) & 0xFF
            value_out[3] = (value_in >> 24) & 0xFF
            value_out[4] = (value_in >> 32) & 0xFF
            value_out[5] = (value_in >> 40) & 0xFF
            value_out[6] = (value_in >> 48) & 0xFF
            value_out[7] = (value_in >> 56) & 0xFF
        elif endianness == "big":
            value_out[0] = (value_in >> 56) & 0xFF
            value_out[1] = (value_in >> 48) & 0xFF
            value_out[2] = (value_in >> 40) & 0xFF
            value_out[3] = (value_in >> 32) & 0xFF
            value_out[4] = (value_in >> 24) & 0xFF
            value_out[5] = (value_in >> 16) & 0xFF
            value_out[6] = (value_in >> 8) & 0xFF
            value_out[7] = value_in & 0xFF
        else:
            raise ValueError        
    else:
        raise ValueError
    return value_out

# Generic Definitions
# End

# Specific Fields Definitions

class g3_hi_ext_address(Structure):
    _pack_ = 1
    _fields_ = [("addr", c_ubyte * g3_hi_defines.MAC_ADDR64_SIZE)]
    def __init__(self, ext_addr=[0,0,0,0,0,0,0,0]):
        self.init_from_buffer(g3_hi_vec_to_cbyte_vec(ext_addr))
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "addr = 0x" + ''.join('{:02X}'.format(x) for x in self.addr)   

class g3_hi_psk_key(Structure):
    _pack_ = 1
    _fields_ = [("psk", c_ubyte * g3_hi_defines.ADP_EAP_PSK_KEY_LEN)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "psk = 0x" + ''.join('{:02X}'.format(x) for x in self.psk)

class g3_hi_adp_context_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("cid", c_ubyte, 4),
        ("c", c_ubyte, 1),
        ("res", c_ubyte, 3),
        ("valid_life_time", c_ushort),
        ("context_len", c_ubyte),
        ("context", c_ubyte * g3_hi_defines.ADP_CONTEXT_MAX_LEN)]
    def __init__(self, cid=0, c=0, valid_life_time=0, context_len=0, context=[0]):
        self.cid = cid
        self.c = c
        self.res = 0
        self.valid_life_time = valid_life_time
        self.context_len = context_len
        if len(context) < sizeof(self.context):
            l = len(context)
        else:
            l = sizeof(self.context)
        for i in range(0, l):
            self.context[i] = context[i]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)
    def __str__(self):
        return "cid = 0x{0:02X}".format(self.cid) + \
            " - c = 0x{0:02X}".format(self.c) + \
            " - valid_life_time = 0x{0:04X}".format(self.valid_life_time) + \
            " - context_len = 0x{0:02X}".format(self.context_len) + \
            " - context = 0x" + ''.join('{:02X}'.format(x) for x in self.context)

class g3_hi_adp_routing_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("dst_addr", c_ushort),
        ("next_hop", c_ushort),
        ("route_cost", c_ushort),
        ("hop_count", c_ubyte, 4),
        ("weak_link", c_ubyte, 4),
        ("valid_time", c_ushort),
        ("media_type", c_ubyte)]
    def __init__(self, dst_addr, next_hop, route_cost, hop_count, weak_link, valid_time, media_type=g3_hi_mac_media_type.MEDIATYPE_PLC_RFbkp):
        self.dst_addr = g3_hi_int_to_ctype(dst_addr, c_ushort)
        self.next_hop = g3_hi_int_to_ctype(next_hop, c_ushort)
        self.route_cost = g3_hi_int_to_ctype(route_cost, c_ushort)
        self.hop_count = hop_count
        self.weak_link = weak_link
        self.valid_time = g3_hi_int_to_ctype(valid_time, c_ushort)
        self.media_type = media_type
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "dst_addr = 0x{0:04X}".format(self.dst_addr) + \
            " - next_hop = 0x{0:04X}".format(self.next_hop) + \
            " - route_cost = 0x{0:04X}".format(self.route_cost) + \
            " - hop_count = 0x{0:02X}".format(self.hop_count) + \
            " - weak_link = 0x{0:02X}".format(self.weak_link) + \
            " - valid_time = 0x{0:04X}".format(self.valid_time) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_adp_broadcast_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("src_addr", c_ushort),
        ("seq_num", c_ubyte),
        ("valid_time", c_ushort)]
    def __init__(self, src_addr=0xFFFF, seq_num = 0, valid_time=0xFFFF):
        self.src_addr = src_addr
        self.seq_num = seq_num
        self.valid_time = valid_time        
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "src_addr = 0x{0:04X}".format(self.src_addr) + \
            " - seq_num = 0x{0:02X}".format(self.seq_num) + \
            " - valid_time = 0x{0:04X}".format(self.valid_time)

class g3_hi_adp_bl_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("addr", c_ushort),
        ("valid_time", c_ushort),
        ("media_type", c_ubyte)]
    def __init__(self, addr=0xFFFF, valid_time=0xFFFF, media_type=g3_hi_mac_media_type.MEDIATYPE_PLC_RFbkp):
        self.addr = addr
        self.valid_time = valid_time
        self.media_type = media_type
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "addr = 0x{0:04X}".format(self.addr) + \
            " - valid_time = 0x{0:04X}".format(self.valid_time) +\
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_adp_pref_tab_entry(Structure):
    _pack_ = 1
    _fields_ = [("pref_len", c_ubyte),
        ("l", c_ubyte, 1),
        ("a", c_ubyte, 1),
        ("reserved", c_ubyte, 6),
        ("valid_lifetime", c_uint),
        ("pref_lifetime", c_uint),
        ("prefix", c_ubyte * g3_hi_defines.ADP_PREFIX_MAX_LEN)]
    def __init__(self, pref_len=0, l=0, a=0, valid_lifetime=0, pref_lifetime=0, prefix=[0]):
        self.pref_len = g3_hi_int_to_ctype(pref_len, c_ubyte)
        self.l = l
        self.a = a
        self.reserved = 0
        self.valid_lifetime = g3_hi_int_to_ctype(valid_lifetime, c_uint)
        self.pref_lifetime = g3_hi_int_to_ctype(pref_lifetime, c_uint)
        if len(prefix) < sizeof(self.prefix):
            l = len(prefix)
        else:
            l = sizeof(self.prefix)
        for i in range(0, l):
            self.prefix[i] = prefix[i]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)
    def __str__(self):
        return "pref_len = 0x{0:02X}".format(self.pref_len) + \
            " - l = 0x{0:02X}".format(self.l) + \
            " - a = 0x{0:02X}".format(self.a) + \
            " - valid_lifetime = 0x{0:08X}".format(self.valid_lifetime) + \
            " - pref_lifetime = 0x{0:08X}".format(self.pref_lifetime) + \
            " - prefix = 0x" + ''.join('{:02X}'.format(x) for x in self.prefix)

class g3_hi_mac_key(Structure):
    _pack_ = 1
    _fields_ = [("free_element", c_ubyte),
        ("key_data", c_ubyte * g3_hi_defines.MAC_KEY_SIZE)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "free_element = 0x{0:02X}".format(self.free_element) + \
            " - key_data = 0x" + ''.join('{:02X}'.format(x) for x in self.key_data)

class g3_hi_mac_keytable(Structure):
    _pack_ = 1
    _fields_ = [("current_key", g3_hi_mac_key),
        ("prec_key", g3_hi_mac_key)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "current_key = " + str(self.current_key) + \
            " - prec_key = " + str(self.prec_key)

class g3_hi_pos_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("short_addr", c_ushort),
        ("lqi", c_ubyte),
        ("pos_valid_time", c_ubyte)]
    def __init__(self, short_addr=0xFFFF, lqi=0x00, pos_valid_time=0x00):
        self.short_addr = g3_hi_int_to_ctype(short_addr, c_ushort)
        self.lqi = g3_hi_int_to_ctype(lqi, c_ubyte)
        self.pos_valid_time = g3_hi_int_to_ctype(pos_valid_time, c_ubyte)
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - pos_valid_time = 0x{0:02X}".format(self.pos_valid_time)

class g3_hi_rf_pos_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("short_addr", c_ushort),
        ("fwd_lqi", c_ubyte),
        ("rev_lqi", c_ubyte),
        ("duty_cycle", c_ubyte),
        ("fwd_power_offset", c_ubyte),
        ("rev_power_offset", c_ubyte),
        ("bkg_noise", c_ubyte),
        ("pos_valid_time", c_ubyte),
        ("reverse_lqi_valid_time", c_ubyte)]
    def __init__(self, short_addr=0xFFFF, fwd_lqi=0xFF, rev_lqi=0xFF, duty_cycle=0xFF, fwd_power_offset=0xFF, rev_power_offset=0xFF, bkg_noise=0xFF, pos_valid_time=0x00, reverse_lqi_valid_time=0x00):
        self.short_addr = g3_hi_int_to_ctype(short_addr, c_ushort)
        self.fwd_lqi = g3_hi_int_to_ctype(fwd_lqi, c_ubyte)
        self.rev_lqi = g3_hi_int_to_ctype(rev_lqi, c_ubyte)
        self.duty_cycle = g3_hi_int_to_ctype(duty_cycle, c_ubyte)
        self.fwd_power_offset = g3_hi_int_to_ctype(fwd_power_offset, c_ubyte)
        self.rev_power_offset = g3_hi_int_to_ctype(rev_power_offset, c_ubyte)
        self.bkg_noise = g3_hi_int_to_ctype(bkg_noise, c_ubyte)
        self.pos_valid_time = g3_hi_int_to_ctype(pos_valid_time, c_ubyte)
        self.reverse_lqi_valid_time = g3_hi_int_to_ctype(reverse_lqi_valid_time, c_ubyte)
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)   
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - fwd_lqi = 0x{0:02X}".format(self.fwd_lqi) + \
            " - rev_lqi = 0x{0:02X}".format(self.rev_lqi) + \
            " - duty_cycle = 0x{0:02X}".format(self.duty_cycle) + \
            " - fwd_power_offset = 0x{0:02X}".format(self.fwd_power_offset) + \
            " - rev_power_offset = 0x{0:02X}".format(self.rev_power_offset) + \
            " - bkg_noise = 0x{0:02X}".format(self.bkg_noise) + \
            " - pos_valid_time = 0x{0:02X}".format(self.pos_valid_time) + \
            " - reverse_lqi_valid_time = 0x{0:02X}".format(self.reverse_lqi_valid_time)

class g3_hi_phy_fcc_txcoeff(Structure):
    _pack_ = 1
    _fields_ = [("coeff", c_ubyte * g3_hi_defines.PHY_TXCOEFF_SIZE_FCC)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "coeff = 0x" + ''.join('{:02X}'.format(x) for x in self.coeff)

class g3_hi_phy_cen_txcoeff(Structure):
    _pack_ = 1
    _fields_ = [("tx_coeff1", c_ubyte, 4),
        ("tx_coeff0", c_ubyte, 4),
        ("tx_coeff3", c_ubyte, 4),
        ("tx_coeff2", c_ubyte, 4),
        ("tx_coeff5", c_ubyte, 4),
        ("tx_coeff4", c_ubyte, 4)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "tx_coeff0 = 0x{0:02}".format(self.tx_coeff0) + \
            " - tx_coeff1 = 0x{0:02}".format(self.tx_coeff1) + \
            " - tx_coeff2 = 0x{0:02}".format(self.tx_coeff2) + \
            " - tx_coeff3 = 0x{0:02}".format(self.tx_coeff3) + \
            " - tx_coeff4 = 0x{0:02}".format(self.tx_coeff4) + \
            " - tx_coeff5 = 0x{0:02}".format(self.tx_coeff5)

#TODO: change into a class to have an initialization also from buffer and to have a human-readible str method
def g3_hi_adp_get_route_table(dst_addr, next_hop, route_cost, hop_count, weak_link, valid_time):
    val_vec = (c_ubyte * 9)()
    val_vec[0] = dst_addr
    val_vec[1] = dst_addr >> 8
    val_vec[2] = next_hop
    val_vec[3] = next_hop >> 8
    val_vec[4] = route_cost
    val_vec[5] = route_cost >> 8
    val_vec[6] = (hop_count & 0x0F) | ((weak_link & 0x0F) << 4)
    val_vec[7] = valid_time
    val_vec[8] = valid_time >> 8
    
    return val_vec

class g3_hi_mac_neigh_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("short_addr", c_ushort),
        ("mod", c_ubyte, 3),
        ("tx_gain", c_ubyte, 4),
        ("tx_res", c_ubyte, 1),
        ("tone_map", c_ubyte * g3_hi_defines.PHY_TONEMAP_SIZE),
        ("reverse_lqi", c_ubyte),
        ("tx_coeff", c_ubyte * g3_hi_defines.PHY_TXCOEFF_SIZE_FCC),
        ("p_diff", c_ubyte, 3),
        ("pay_mod_sch", c_ubyte, 1),
        ("rsv", c_ubyte, 4),
        ("tmr_valid_time", c_ubyte)]
    def __init__(self, short_addr, mod, tx_gain, tx_res, tone_map, reverse_lqi, tx_coefficient, p_diff, pay_mod_sch, tmr_valid_time):
        self.short_addr = g3_hi_int_to_ctype(short_addr, c_ushort)
        self.mod = mod
        self.tx_gain = tx_gain
        self.tx_res = tx_res
        g3_hi_memmove_checked(self.tone_map, tone_map, sizeof(self.tone_map))
        self.reverse_lqi = g3_hi_int_to_ctype(reverse_lqi, c_ubyte)
        g3_hi_memmove_checked(self.tx_coeff, tx_coefficient, sizeof(self.tx_coeff))
        self.p_diff = p_diff
        self.pay_mod_sch = pay_mod_sch
        self.tmr_valid_time = g3_hi_int_to_ctype(tmr_valid_time, c_ubyte)
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - mod = {}".format(str(g3_hi_phy_mod_type(self.mod))) + \
            " - tx_gain = 0x{0:02X}".format(self.tx_gain) + \
            " - tx_res = 0x{0:02X}".format(self.tx_res) + \
            " - reverse_lqi = 0x{0:02X}".format(self.reverse_lqi) + \
            " - p_diff = 0x{0:02X}".format(self.p_diff) + \
            " - pay_mod_sch = {}".format(str(g3_hi_mod_scheme(self.pay_mod_sch))) + \
            " - tmr_valid_time = 0x{0:02X}".format(self.tmr_valid_time) + \
            " - tone_map = 0x" + ''.join('{:02X}'.format(x) for x in self.tone_map) + \
            " - tx_coeff = 0x" + ''.join('{:02X}'.format(x) for x in self.tx_coeff)

class g3_hi_mac_dev_tab_entry(Structure):
    _pack_ = 1
    _fields_ = [("pan_id", c_ushort),
        ("short_addr", c_ushort),
        ("last_rcv_frame_cnt", c_uint)]
    def __init__(self, pan_id = 0xFFFF, short_addr = 0xFFFF, last_rcv_fc = 0xFFFFFFFF):
        self.pan_id = pan_id
        self.short_addr = short_addr
        self.last_rcv_frame_cnt = last_rcv_fc
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "pan_id = 0x{0:04X}".format(self.pan_id) + \
            " - short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - last_rcv_frame_cnt = 0x{0:08X}".format(self.last_rcv_frame_cnt)

class g3_hi_mac_phy_quality_rx_table_entry(Structure):
    _pack_ = 1
    _fields_ = [("short_addr", c_ushort),
                ("noise_power", c_ubyte * 72),
                ("signal_power", c_ubyte * 72),   
        ("valid_time", c_ubyte)]
    def __init__(self, short_addr, noise_power, signal_power, valid_time):
        self.short_addr = g3_hi_int_to_ctype(short_addr, c_ushort)
        g3_hi_memmove_checked(self.noise_power, noise_power, sizeof(self.noise_power))
        g3_hi_memmove_checked(self.signal_power, signal_power, sizeof(self.signal_power))
        self.valid_time = g3_hi_int_to_ctype(valid_time, c_ubyte)
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)   
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) + \
            " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) + \
            " - valid_time = 0x{0:02X}".format(self.valid_time)

class g3_hi_pib_id(Structure):
    _pack_ = 1
    _fields_ = [("id", c_uint),
        ("idx", c_ushort)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __len__(self):
        return sizeof(self)        
    def __str__(self):
        return "id = 0x{0:08X}".format(self.id) + \
            " - idx = 0x{0:04X}".format(self.idx)

def g3_hi_g3lib_pib_gen(id, len, value):
    class g3_hi_g3lib_pib(Structure):
        _pack_ = 1
        _fields_ = [("id", g3_hi_pib_id),
            ("len", c_ushort),
            ("value", c_ubyte * len)]
        def __init__(self, id_, len_, value_):
            memmove(byref(self.id), byref(id_), sizeof(id_))
            self.len = c_ushort(len_)
            memmove(byref(self.value), byref(value_), len_)
        def __len__(self):
            return sizeof(self)            
        def __str__(self):
            return "id = " + str(self.id) + \
                " - len = 0x{0:04X}".format(self.len) + \
                " - value = 0x" + ''.join('{:02X}'.format(x) for x in self.value)
    return g3_hi_g3lib_pib(id, len, value)

class g3_hi_swreset_data(Structure):
    _pack_ = 1
    _fields_ = [("prot_version", c_ubyte),
        ("dev_type", c_ubyte),
        ("mode", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return  "prot_version = {}".format(str(g3_hi_band_plan(self.prot_version))) + \
        " - dev_type = {}".format(str(g3_hi_device_type(self.dev_type))) + \
        " - mode = {}".format(str(g3_hi_mode(self.dev_type)))

class g3_hi_mac_dev_addr(Structure):
    _pack_ = 1
    _fields_ = [("mode", c_ubyte),
        ("pan_id", c_ushort),
        ("addr", g3_hi_ext_address)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "mode = 0x{0:02X}".format(self.mode) + \
            " - pan_id = 0x{0:04X}".format(self.pan_id) + \
            " - addr = " + str(self.addr)

class g3_hi_loadng_prex_hop_info(Structure):
    _pack_ = 1
    _fields_ = [("addr", c_ushort),
        ("reserved", c_ubyte, 2),
        ("media_tx", c_ubyte, 1),
        ("media_rx", c_ubyte, 1),
        ("phase_diff", c_ubyte, 3),
        ("mns", c_ubyte, 1),
        ("link_cost", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.addr) + \
        " - media_tx = 0x{0:02X}".format(self.media_tx) + \
        " - media_rx = 0x{0:02X}".format(self.media_rx) + \
        " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
        " - msn = 0x{0:02X}".format(self.mns) + \
        " - link_cost = 0x{0:02X}".format(self.link_cost)

class g3_hi_pan_descriptor(Structure):
    _pack_ = 1
    _fields_ = [("pan_id", c_ushort),
        ("link_quality", c_ubyte),
        ("lba_addr", c_ushort),
        ("route_cost", c_ushort), 
        ("media_type", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "pan_id = 0x{0:04X}".format(self.pan_id) + \
            " - link_quality = 0x{0:02X}".format(self.link_quality) + \
            " - lba_addr = 0x{0:04X}".format(self.lba_addr) + \
            " - route_cost = 0x{0:04X}".format(self.route_cost) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_ip_data_conf(Structure):
    _pack_ = 1
    _fields_ = [("status", c_ubyte),
        ("handle", c_ubyte),
        ("next_hop_media", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "status = 0x{0:02X}".format(self.status) + \
            " - handle = 0x{0:02X}".format(self.handle) + \
            " - next_hop_media = " + str(g3_hi_mac_media_type(self.next_hop_media))

class g3_hi_udp_app_conf(Structure):
    _pack_ = 1
    _fields_ = [("status", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "status = 0x{0:02X}".format(self.status)

IPV6_ADDR_LEN = 16

class g3_hi_uip_ipaddr(Structure):
    _pack_ = 1
    _fields_ = [("u8", c_ubyte * IPV6_ADDR_LEN)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __init__(self, addr):
        self.u8 = g3_hi_vec_to_cbyte_vec(addr)
    def __str__(self):
        return "0x" + ''.join('{:02X}'.format(x) for x in self.u8)

class g3_hi_ipv6_link_local_addr(Structure):
    _pack_ = 1
    _fields_ = [("addr", g3_hi_uip_ipaddr)]
    def __init__(self, pan_id, short_addr):
        prefix = [0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
        interface_id = [0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x00]
        interface_id[0] = (pan_id >> 8) & 0xFF
        interface_id[1] = (pan_id) & 0xFF
        interface_id[6] = (short_addr >> 8) & 0xFF
        interface_id[7] = (short_addr) & 0xFF
        for i in range(0, len(prefix)):
            self.addr.u8[i] = prefix[i]
        for i in range(0, len(interface_id)):
            self.addr.u8[i + 8] = interface_id[i]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "addr = " + str(self.addr)

class g3_hi_ipv6_multicast_addr(Structure):
    _pack_ = 1
    _fields_ = [("addr", g3_hi_uip_ipaddr)]
    def __init__(self, group, ff1 = 0x0, scope = 0x2, ff2 = 0x0, rsv = 0x0, plen = 0x00, net_pref = [0,0,0,0,0,0,0,0]):
        self.addr.u8[0] = c_ubyte(0xFF)
        self.addr.u8[1] = c_ubyte( (ff1<<4 & 0xF0) + (scope & 0x0F) )
        self.addr.u8[2] = c_ubyte( (ff2<<4 & 0xF0) + (rsv & 0x0F) )
        self.addr.u8[3] = c_ubyte(plen)
        for i in range(0, len(net_pref)):
            self.addr.u8[4+i] = net_pref[i]
        self.addr.u8[4+8] = c_ubyte(0)
        self.addr.u8[4+8+1] = c_ubyte(0)
        self.addr.u8[4+8+2] = c_ubyte((group >>8) & 0x7F)
        self.addr.u8[4+8+3] = c_ubyte(group & 0xFF)
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "addr = " + str(self.addr)

def g3_hi_ipv6_addr_with_prefix(prefix, pan_id, short_addr):
    iid = [(pan_id >> 8) & 0xFF, (pan_id) & 0xFF, 0x00, 0xFF, 0xFE, 0x00, (short_addr >> 8) & 0xFF, (short_addr) & 0xFF] 
    addr = list(prefix) #copy the original list
    addr +=  [0] * (IPV6_ADDR_LEN - len(iid) - len(prefix)) #fill address with 0 if preix is not 8 bytes
    addr += iid 
    return g3_hi_uip_ipaddr(g3_hi_vec_to_cbyte_vec(addr))


def int_to_byte_list(number, lenght, byteorder):
    return list(number.to_bytes(lenght, byteorder))
def carry_around_add(a, b):
    c = a + b
    return (c & 0xffff) + (c >> 16)
def g3_hi_ipv6_checksum(msg):
    s = 0
    if len(msg)%2 != 0:
        msg += [0]
    for i in range(0, len(msg), 2):
        #w = ord(msg[i]) + (ord(msg[i+1]) << 8)
        w = (msg[i]) + ((msg[i+1]) << 8)
        s = carry_around_add(s, w)
    return ~s & 0xffff
def g3_hi_udp_pseudo_header_checksum(src_addr, dst_addr, next_header, src_port, dst_port, udp_lenght, data):

    ph = []
    ph += src_addr
    ph += dst_addr
    ph += int_to_byte_list(udp_lenght, 4, 'big')
    ph += [0,0,0]
    ph += int_to_byte_list(next_header, 1, 'big')
    ph += int_to_byte_list(src_port, 2, 'big')
    ph += int_to_byte_list(dst_port, 2, 'big')
    ph += int_to_byte_list(udp_lenght, 2, 'big')
    ph += [0,0]
    ph += data

    chk = g3_hi_ipv6_checksum(ph)

    return ((chk & 0xFF ) << 8) + ((chk >> 8) & 0xFF)

def g3_hi_icmp_pseudo_header_checksum(src_addr, dst_addr, next_header, icmp_type, code, icmp_lenght, data):

    ph = []
    ph += src_addr
    ph += dst_addr
    ph += int_to_byte_list(icmp_lenght, 4, 'big')
    ph += [0,0,0]
    ph += int_to_byte_list(next_header, 1, 'big')
    ph += [icmp_type]
    ph += [code]
    ph += [0,0] #checksum
    ph += data

    chk = g3_hi_ipv6_checksum(ph)

    return ((chk & 0xFF ) << 8) + ((chk >> 8) & 0xFF)    

def g3_hi_udp_packet_gen(src_addr, dst_addr, src_port, dst_port, data, next_hdr = 17, extension_headers = None, from_raw_data_c=None):
    data_len = len(data)
    data_c = g3_hi_vec_to_cbyte_vec(data)

    if extension_headers != None:
        ext_hdr_len = len(extension_headers)
    else:
        ext_hdr_len = 0

    if(from_raw_data_c != None):
        data_len = sizeof(from_raw_data_c) - (40+8)

    class g3_hi_udp_packet(BigEndianStructure):
        _pack_ = 1
        _fields_ = [("version", c_uint32, 4),
            ("traffic_class", c_uint32, 8),
            ("flow_label", c_uint32, 20),
            ("pay_len", c_ushort),
            ("next_hdr", c_ubyte),
            ("hop_limit", c_ubyte),
            ("src_addr", g3_hi_uip_ipaddr),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("ext_hdr", c_ubyte * ext_hdr_len),
            ("src_port", c_ushort),
            ("dst_port", c_ushort),
            ("len", c_ushort),
            ("checksum", c_ushort),
            ("data", c_ubyte * data_len)]
        def __init__(self, src_addr, dst_addr, src_port, dst_port, udp_data, next_hdr = 17, extension_headers = None, from_raw_data_c=None):
            if(from_raw_data_c == None):
                self.version = 6
                self.traffic_class = 0
                self.flow_label = 0
                self.pay_len = ext_hdr_len + data_len + 8
                self.next_hdr = next_hdr
                self.hop_limit = 14
                g3_hi_memmove_checked(self.ext_hdr, extension_headers, ext_hdr_len)
                self.src_addr = src_addr
                self.dst_addr = dst_addr
                self.src_port = src_port
                self.dst_port = dst_port
                self.len = data_len + 8
                g3_hi_memmove_checked(self.data, udp_data, sizeof(udp_data))
                self.checksum = g3_hi_udp_pseudo_header_checksum(self.src_addr.u8, self.dst_addr.u8, self.next_hdr, self.src_port, self.dst_port, self.pay_len, self.data)  
            else: #initialization from a buffer, e.g. from an ADPD-DATA.Indication
                g3_hi_memmove_checked(self, from_raw_data_c, sizeof(self))
        def __str__(self):
            if sizeof(self.ext_hdr) != 0:
                ext_hdr_s = " - next_hdr = 0x{:02X}".format(self.next_hdr) + "- ext_hdr 0x" + ''.join('{:02X}'.format(x) for x in self.ext_hdr)
            else: 
                ext_hdr_s = ""
            return "pay_len = 0x{:04X}".format(self.pay_len) + \
                " - src_{}".format(self.src_addr) + \
                " - dst_{}".format(self.dst_addr) + \
                ext_hdr_s +\
                " - src_port = 0x{:04X}".format(self.src_port) + \
                " - dst_port = 0x{:04X}".format(self.dst_port) + \
                " - checksum = 0x{:04X}".format(self.checksum) + \
                " - payload 0x" + ''.join('{:02X}'.format(x) for x in self.data)
        def to_buffer(self):
            buff = (c_ubyte * sizeof(self)).from_buffer_copy(self)
            return buff
    return g3_hi_udp_packet(src_addr, dst_addr, src_port, dst_port, data_c, next_hdr, extension_headers, from_raw_data_c)

def g3_hi_udp_ll_unicast_packet_gen(src_short_addr, src_pan_id, dst_short_addr, dst_pan_id, src_port, dst_port, data, next_hdr = 17, extension_headers = None, from_raw_data=None):
    src_addr_ll =  g3_hi_ipv6_link_local_addr(src_pan_id, src_short_addr)
    dst_addr_ll =  g3_hi_ipv6_link_local_addr(dst_pan_id, dst_short_addr)
    return g3_hi_udp_packet_gen(src_addr_ll.addr, dst_addr_ll.addr, src_port, dst_port, data, next_hdr, extension_headers, from_raw_data)


def g3_hi_udp_mcast_packet_gen(src_short_addr, src_pan_id, mcast_dst_short_addr, src_port, dst_port, data, next_hdr = 17, extension_headers = None, from_raw_data=None):
    src_addr_ll =  g3_hi_ipv6_link_local_addr(src_pan_id, src_short_addr)
    dst_addr_mcast =  g3_hi_ipv6_multicast_addr(mcast_dst_short_addr)
    return g3_hi_udp_packet_gen(src_addr_ll.addr, dst_addr_mcast.addr, src_port, dst_port, data, next_hdr, extension_headers, from_raw_data)


def g3_hi_icmp_pkt_gen(src_addr, dst_addr, icmp_type, code, data):
    data_len = len(data)
    data_c = g3_hi_vec_to_cbyte_vec(data)

    class g3_hi_icmp_pkt(BigEndianStructure):
        _pack_ = 1
        _fields_ = [("version", c_uint32, 4),
            ("traffic_class", c_uint32, 8),
            ("flow_label", c_uint32, 20),
            ("pay_len", c_ushort),
            ("next_hdr", c_ubyte),
            ("hop_limit", c_ubyte),
            ("src_addr", g3_hi_uip_ipaddr),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("type", c_ubyte),
            ("code", c_ubyte),
            ("checksum", c_ushort),
            ("data", c_ubyte * data_len)]
        def __init__(self, src_addr_, dst_addr_, icmp_type_, code_, data_):
                self.version = 6
                self.traffic_class = 0
                self.flow_label = 0
                self.pay_len = data_len + 4
                self.next_hdr = 58 #ICMP
                self.hop_limit = 255 
                self.src_addr = src_addr_
                self.dst_addr = dst_addr_
                self.type = icmp_type_
                self.code = code_
                g3_hi_memmove_checked(self.data, data_, sizeof(data_))
                self.checksum = g3_hi_icmp_pseudo_header_checksum(self.src_addr.u8, self.dst_addr.u8, self.next_hdr, self.type, self.code, self.pay_len, self.data)  
        def __str__(self):
            return "pay_len = 0x{:04X}".format(self.pay_len) + \
                " - src_{}".format(self.src_addr) + \
                " - dst_{}".format(self.dst_addr) + \
                " - type = 0x{:02X}".format(self.type) + \
                " - code = 0x{:02X}".format(self.code) + \
                " - checksum = 0x{:04X}".format(self.checksum) + \
                " - payload 0x" + ''.join('{:02X}'.format(x) for x in self.data)
        def to_buffer(self):
            buff = (c_ubyte * sizeof(self)).from_buffer_copy(self)
            return buff                
    return g3_hi_icmp_pkt(src_addr.addr, dst_addr.addr, icmp_type, code, data_c)

class g3_hi_rreq_rrep_msg(Structure):
    _pack = 1
    _fields_ = [("type", c_ubyte),
        ("destination", c_ushort),
        ("originator", c_ushort),
        ("sn", c_ushort),
        ("flags", c_ubyte, 4),
        ("metric_type", c_ubyte, 4),
        ("route_cost", c_ushort),
        ("hop_limit", c_ubyte, 4),
        ("hop_count", c_ubyte, 4),
        ("weak_link", c_ubyte, 4),
        ("rsv", c_ubyte, 4)]
    def __init__(self, data):
        g3_hi_memmove_checked(self, data, sizeof(data))
    def __str__(self):
        return "type = {:}".format(g3_hi_loadng_msg_type(self.type)) +\
            " - destination = 0x{:04X}".format(self.destination) + \
            " - originator = 0x{:04X}".format(self.originator) + \
            " - sn = 0x{:04X}".format(self.sn) + \
            " - flags = 0x{:02X}".format(self.flags) + \
            " - metric_type = 0x{:02X}".format(self.metric_type) + \
            " - route_cost = 0x{:04X}".format(self.route_cost) + \
            " - hop_limit = 0x{:02X}".format(self.hop_limit) + \
            " - hop_count = 0x{:02X}".format(self.hop_count) + \
            " - weak_link = 0x{:02X}".format(self.weak_link)

class g3_hi_rerr_msg(Structure):
    _pack = 1
    _fields_ = [("type", c_ubyte),
        ("destination", c_ushort),
        ("originator", c_ushort),
        ("ec", c_ubyte),
        ("unreachable_addr", c_ushort),
        ("hop_limit", c_ubyte, 4),
        ("rsv", c_ubyte, 4)]
    def __init__(self, data):
        g3_hi_memmove_checked(self, data, sizeof(data))
    def __str__(self):
        return "type = ".format(g3_hi_loadng_msg_type(self.type)) + \
            " - destination = 0x{:04X}".format(self.destination) + \
            " - originator = 0x{:04X}".format(self.originator) + \
            " - ec = ".format(g3_hi_loadng_error_code(self.ec)) + \
            " - unreachable_addr = 0x{:04X}".format(self.unreachable_addr) + \
            " - hop_limit = 0x{:02X}".format(self.hop_limit)

class g3_hi_nvm_gen(Structure):
    _pack_ = 1
    _fields_ = [("marker", c_uint),
        ("version", c_ushort),
        ("led_conf", c_ubyte),
        ("trace_com", c_ubyte),
        ("trace_filter", c_uint),
        ("rsv", c_ubyte * 12)]
    def __init__(self, data):
        memmove(byref(self), byref(data), sizeof(data))
    def __str__(self):
        return "section = GEN - marker = 0x{0:08X}".format(self.marker) + " - version = {0:04X}".format(self.version) + \
            " - led_conf = 0x{0:02X}".format(self.led_conf) + " - trace_com = 0x{0:02X}".format(self.trace_com) + \
            " - trace_filter = 0x{0:08X}".format(self.trace_filter)+ \
            " - rsv = 0x" + ''.join('{:02X}'.format(x) for x in (self.rsv))

class g3_hi_nvm_plc_mode_band_type(Structure):
    _pack_ = 1
    _fields_ = [("device_type", c_ubyte, 1),
        ("bandplan", c_ubyte, 3),
        ("hi_mode", c_ubyte, 4)]        
    def __str__(self):
        return "device_type = 0x{0:02X}".format(self.device_type) + \
            " - bandplan = 0x{0:02X}".format(self.bandplan) + \
            " - hi_mode = 0x{0:02X}".format(self.hi_mode)

class g3_hi_nvm_plc_test_config(Structure):
    _pack_ = 1
    _fields_ = [("rsv1", c_ubyte, 1),
        ("rsv2", c_ubyte, 1),
        ("fast_restore", c_ubyte, 1),
        ("nvm_attr", c_ubyte, 1),
        ("rsv3", c_ubyte, 4)]        
    def __str__(self):
        return "fast_restore = 0x{0:02X}".format(self.fast_restore) + \
            " - nvm_attr = 0x{0:02X}".format(self.nvm_attr)            

def g3_hi_nvm_plc_attr_gen(attr_id, attr_idx, attr_len, attr_value):
    class nvm_plc_attr(Structure):
        _pack_ = 1
        _fields_ = [("id", g3_hi_pib_id),
            ("len", c_ubyte),
            ("value", c_ubyte * attr_len)]
        def __init__(self, id_, idx_, len_, value_):
            self.id.id = id_
            self.id.idx = idx_
            self.len = len_
            if (len_ > 0):
                memmove(byref(self.value), byref(value_), len_)            
        def __str__(self):
            return "id = " + str(self.id) + " - len = {0:02d}".format(self.len) + \
                " - value = 0x" + ''.join('{:02X}'.format(x) for x in (self.value))
    if isinstance(attr_value, int):
        attr_value = g3_hi_int2ctype(attr_len, attr_value)
    return nvm_plc_attr(attr_id, attr_idx, attr_len, attr_value)

class g3_hi_nvm_plc(Structure):
    _pack_ = 1
    _fields_ = [("marker", c_uint),
        ("version", c_ushort),
        ("mode_band_type", g3_hi_nvm_plc_mode_band_type),
        ("config", g3_hi_nvm_plc_test_config),
        ("rsv1", c_ubyte * 8),
        ("fc", c_uint),
        ("rf_fc", c_uint),
        ("sn", c_ushort),
        ("padding", c_ubyte * 2),
        ("sn_extra", c_uint),
        ("fc_extra", c_uint),
        ("rf_fc_extra", c_uint),
        ("rsv3", c_ubyte * 2),
        ("crc", c_ushort),
        ("panid", c_ushort),
        ("short_addr", c_ushort),
        ("active_key_id", c_ubyte),
        ("rsv2", c_ubyte),
        ("key0_free", c_ubyte),
        ("key0", c_ubyte * 16),
        ("key1_free", c_ubyte),
        ("key1", c_ubyte * 16),
        ("prefix1", c_ubyte * 8),
        ("prefix2", c_ubyte * 8),
        ("attr_data", c_ubyte * g3_hi_defines.NVM_PLC_ATTR_SIZE)]
    def __init__(self, data):
        memmove(byref(self), byref(data), sizeof(data))
    def get_attrs(self):
        index = 0
        plc_attrs = []
        # nvm_plc_attr_fixed_size = 7
        while index + 7 <= sizeof(self.attr_data):
            attrib_len = self.attr_data[index + 6]
            if attrib_len == 0xFF:
                break
            class nvm_plc_attr(Structure):
                _pack_ = 1
                _fields_ = [("id", g3_hi_pib_id),
                    ("len", c_ubyte),
                    ("value", c_ubyte * attrib_len)]
                def __str__(self):
                    return "id = " + str(self.id) + " - len = {0:02d}".format(self.len) + \
                        " - value = 0x" + ''.join('{:02X}'.format(x) for x in (self.value))
            new_attr = nvm_plc_attr()
            memmove(byref(new_attr), byref(self.attr_data, index), sizeof(new_attr))
            if new_attr.id.id != 0xFFFFFFFF and new_attr.id.idx != 0xFFFF:
                plc_attrs.append(new_attr)
                index += sizeof(new_attr)
            else:
                break
        return plc_attrs
    def set_attrs(self, plc_attrs):
        memset(addressof(self.attr_data), 0xFF, sizeof(self.attr_data))
        offset = 0
        if plc_attrs == None or plc_attrs == []:
            return
        for attr in plc_attrs:
            memmove(byref(self.attr_data, offset), byref(attr), sizeof(attr)) 
            offset += sizeof(attr)         
    def __str__(self):
        return "section = PLC - marker = 0x{0:08X}".format(self.marker) + " - version = {0:04X}".format(self.version) + \
            " - mode_band_type = " + str(self.mode_band_type) + " - config = " + str(self.config) + \
            " - frame_counter = 0x{0:08X}".format(self.fc) + " - rf_frame_counter = 0x{0:08X}".format(self.rf_fc) + \
            " - loadng_sn = 0x{0:04X}".format(self.sn) + " - fc_extra = 0x{0:08X}".format(self.fc_extra) + \
            " - rf_fc_extra = 0x{0:04X}".format(self.rf_fc_extra) + " - sn_extra = 0x{0:08X}".format(self.sn_extra) + \
            " - panid = 0x{0:04X}".format(self.panid) + " - short_addr = 0x{0:04X}".format(self.short_addr) + \
            " - active_key_id = 0x{0:02X}".format(self.active_key_id) + " - key0_free = 0x{0:02X}".format(self.key0_free) + \
            " - key0 = 0x" + ''.join('{:02X}'.format(x) for x in (self.key0)) + " - key1_free = 0x{0:02X}".format(self.key1_free) + \
            " - key1 = 0x" + ''.join('{:02X}'.format(x) for x in (self.key1)) + " - prefix1 = 0x" + ''.join('{:02X}'.format(x) for x in (self.prefix1)) + \
            " - prefix2 = 0x" + ''.join('{:02X}'.format(x) for x in (self.prefix2))                                                                               

class g3_hi_nvm_plcc(Structure):
    NVRAM_MAX_NUM_NODES = 2048
    NVRAM_MAX_NUM_FLAGS = 256 #NVRAM_MAX_NUM_NODES/8
    _pack_ = 1
    _fields_ = [("marker", c_uint),
        ("version", c_ushort),
        ("isNotConnected", c_ubyte * NVRAM_MAX_NUM_FLAGS),
        ("hash", c_ubyte * NVRAM_MAX_NUM_NODES),
        ]
    def __init__(self, data):
        memmove(byref(self), byref(data), sizeof(data))      
    def __str__(self):
        return "section = PLCC - marker = 0x{0:08X}".format(self.marker) + " - version = {0:04X}".format(self.version) +\
            " - isNotConnected = 0x" + ''.join('{:02X}'.format(x) for x in (self.isNotConnected)) +\
            " - hash = " + ''.join(' 0x{:02X}'.format(x) for x in (self.hash))


class g3_hi_nvm_rf(Structure):
    _pack_ = 1
    _fields_ = [("marker", c_uint),
        ("version", c_ushort),
        ("rsv1", c_ushort),
        ("RadioBaseFreq", c_uint),      
        ("RadioModSelect", c_ubyte),     
        ("RadioDataRate", c_uint),      
        ("RadioFreqDeviation", c_uint), 
        ("RadioBandwidth", c_uint),     
        ("RadioCsBlanking", c_ubyte),    
        ("RadioXtalFreq", c_uint),      
        ("RadioRssiGain", c_ubyte),      
        ("PowerdBm", c_ubyte),           
        ("PktCRCMode", c_ubyte),         
        ("PktEnFEC", c_ubyte),           
        ("PktEnWhitening", c_ubyte),     
        ("PktEnInterleaving", c_ubyte),  
        ("IrqGpioPin", c_ubyte),         
        ("MCUClkEnable", c_ubyte),       
        ("MCUClkGpioPin", c_ubyte),      
        ("MCUClkXORatio", c_ubyte),      
        ("MCUClkRCORatio", c_ubyte),     
        ("MCUClkClockCycles", c_ubyte),  
        ("ExtSmpsEnable", c_ubyte),      
        ("FEMEnabled", c_ubyte),         
        ("FEMGpioPinCSD", c_ubyte),      
        ("FEMGpioPinCPS", c_ubyte),      
        ("FEMGpioPinCTX", c_ubyte),      
        ("FEMTxBypassEn", c_ubyte)]
    def __init__(self, data):
        memmove(byref(self), byref(data), sizeof(self))
    def __str__(self):
        return "section = RF - marker = 0x{0:08X}".format(self.marker) + " - version = {0:04X}".format(self.version) + \
                " - RadioBaseFreq = {0:08X}".format(self.RadioBaseFreq) + " - RadioModSelect = {0:02X}".format(self.RadioModSelect) + " - RadioDataRate = {0:08X}".format(self.RadioDataRate) + " - RadioFreqDeviation = {0:08X}".format(self.RadioFreqDeviation) +\
                " - RadioBandwidth = {0:08X}".format(self.RadioBandwidth) + " - RadioCsBlanking = {0:02X}".format(self.RadioCsBlanking) + " - RadioXtalFreq = {0:08X}".format(self.RadioXtalFreq) + " - RadioRssiGain = {0:02X}".format(self.RadioRssiGain) +\
                " - PowerdBm = {0:02X}".format(self.PowerdBm) + " - PktCRCMode = {0:02X}".format(self.PktCRCMode) + " - PktEnFEC = {0:02X}".format(self.PktEnFEC) + " - PktEnWhitening = {0:02X}".format(self.PktEnWhitening) +\
                " - PktEnInterleaving = {0:02X}".format(self.PktEnInterleaving) + " - IrqGpioPin = {0:02X}".format(self.IrqGpioPin) + " - MCUClkEnable = {0:02X}".format(self.MCUClkEnable) + " - MCUClkGpioPin = {0:02X}".format(self.MCUClkGpioPin) +\
                " - MCUClkXORatio = {0:02X}".format(self.MCUClkXORatio) + " - MCUClkRCORatio = {0:02X}".format(self.MCUClkRCORatio) + " - MCUClkClockCycles = {0:02X}".format(self.MCUClkClockCycles) + " - ExtSmpsEnable = {0:02X}".format(self.ExtSmpsEnable) +\
                " - FEMEnabled = {0:02X}".format(self.FEMEnabled) + " - FEMGpioPinCSD = {0:02X}".format(self.FEMGpioPinCSD) + " - FEMGpioPinCPS = {0:02X}".format(self.FEMGpioPinCPS) + " - FEMGpioPinCTX = {0:02X}".format(self.FEMGpioPinCTX) +\
                " - FEMTxBypassEn = {0:02X}".format(self.FEMTxBypassEn)                                                                           


class g3_hi_nvm_tab(Structure):
    _pack_ = 1
    _fields_ = [("short_addr", c_ushort),
        ("ext_addr", g3_hi_ext_address),
        ("idp_len", c_ubyte),
        ("idp", c_ubyte * g3_hi_defines.NVM_IDP_MAX_SIZE)]
    def __init__(self, short_addr=0xFFFF, ext_addr=[0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00], idp=None):
        self.short_addr = g3_hi_int_to_ctype(short_addr, c_ushort)
        self.ext_addr.addr = g3_hi_vec_to_cbyte_vec(ext_addr)
        # If IdP is not specified, set it to EUI64
        if idp == None:
            idp = ext_addr
        self.idp_len = len(idp)
        memset(addressof(self.idp), 0x00, sizeof(self.idp))
        memmove(byref(self.idp), byref(g3_hi_vec_to_cbyte_vec(idp)), self.idp_len) 
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "short_addr = 0x{0:04X}".format(self.short_addr) + " - ext_addr = " + str(self.ext_addr) + \
            " - idp_len = {0:02d}".format(self.idp_len) + " - idp = 0x" + ''.join('{:02X}'.format(x) for x in (self.idp))

class g3_hi_nvm_tab_is_connected(Structure):
    _pack_ = 1
    _fields_ = [("is_conn", c_bool)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "is_connected = " + str(self.is_conn)

class g3_hi_tonemapresppay_fcc(Structure):
    _pack_ = 1
    _fields_ = [("mod", c_ubyte, 3),
        ("tx_gain", c_ubyte, 4),
        ("tx_res", c_ubyte, 1),
        ("tone_map", c_ubyte * g3_hi_defines.PHY_TONEMAP_SIZE),
        ("lqi", c_ubyte),
        ("tx_coeff", g3_hi_phy_fcc_txcoeff),
        ("res", c_ubyte, 7),
        ("pay_mod_sch", c_ubyte, 1),
        ("fcs", c_ushort)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "mod = 0x{0:02X}".format(self.mod) + \
            " - tx_gain = 0x{0:02X}".format(self.tx_gain) + \
            " - tx_res = 0x{0:02X}".format(self.tx_res) + \
            " - tone_map = 0x" + ''.join('{:02X}'.format(x) for x in (self.tone_map)) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - tx_coeff = " + str(self.tx_coeff) + \
            " - pay_mod_sch = 0x{0:02X}".format(self.pay_mod_sch) + \
            " - fcs = 0x{0:04X}".format(self.fcs)

class g3_hi_tonemapresppay_cen(Structure):
    _pack_ = 1
    _fields_ = [("pay_mod_sch", c_ubyte, 1),
        ("mod", c_ubyte, 2),
        ("tx_gain", c_ubyte, 4),
        ("tx_res", c_ubyte, 1),
        ("tm05", c_ubyte, 6),
        ("res", c_ubyte, 2),
        ("lqi", c_ubyte),
        ("tx_coeff", g3_hi_phy_cen_txcoeff),
        ("rsv", c_ubyte),
        ("fcs", c_ushort)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "pay_mod_sch = 0x{0:02X}".format(self.pay_mod_sch) + \
            " - mod = 0x{0:02X}".format(self.mod) + \
            " - tx_gain = 0x{0:02X}".format(self.tx_gain) + \
            " - tx_res = 0x{0:02X}".format(self.tx_res) + \
            " - tm05 = 0x{0:02X}".format(self.tm05) + \
            " - res = 0x{0:02X}".format(self.res) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - tx_coeff = " + str(self.tx_coeff) + \
            " - fcs = 0x{0:04X}".format(self.fcs)

class g3_hi_gen_address(Union):
    _pack_ = 1
    _fields_ = [("short", c_ushort),
        ("ext_address", c_ubyte * g3_hi_defines.MAC_ADDR64_SIZE)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "short = 0x{0:04X}".format(self.short) + \
            " - ext_address = 0x" + ''.join('{:02X}'.format(x) for x in (self.ext_address))

class g3_hi_mac_device_addr(Structure):
    _pack_ = 1
    _fields_ = [("mode", c_ubyte),
        ("pan_id", c_ushort),
        ("addr", g3_hi_gen_address)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "mode = 0x{0:02X}".format(self.mode) + \
            " - pan_id = 0x{0:04X}".format(self.pan_id) + \
            " - addr = " + str(self.addr)

class g3_hi_tonemapresp_pay(Union):
    _pack_ = 1
    _fields_ = [("tonemap_cen", g3_hi_tonemapresppay_cen),
        ("tonemap_fcc", g3_hi_tonemapresppay_fcc)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "tonemap_cen = " + str(self.tonemap_cen) + \
            " - tonemap_fcc = " + str(self.tonemap_fcc)

class g3_hi_loadng_prep(Structure):
    _pack_ = 1
    _fields_ = [("type", c_ubyte),
        ("dst", c_ushort),
        ("expected_orig", c_ushort),
        ("ext_data", c_ushort),
        ("orig", c_ushort),
        ("hops_info", g3_hi_loadng_prex_hop_info * (g3_hi_defines.ADP_MAX_HOPS_SUPPORTED * 2))]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        hops_info = "hops_info[0] = " + str(self.hops_info[0])

        for i in range(1, len(self.hops_info)):
            hops_info += " - hops_info" + str(i) + " = " + str(self.hops_info[i])

        return "type = 0x{0:02X}".format(self.type) + \
            " - dst = 0x{0:04X}".format(self.dst) + \
            " - expected_orig = 0x{0:04X}".format(self.expected_orig) + \
            " - ext_data = 0x{0:04X}".format(self.ext_data) + \
            " - orig = 0x{0:04X}".format(self.orig) + \
            " - " + hops_info

class g3_hi_bcn_notify_ind(Structure):
    _pack_ = 1
    _fields_ = [("pan_id", c_ushort),
        ("lba_addr", c_ushort),
        ("rc_coord", c_ushort),
        ("snr_carriers", c_ubyte * 72),
        ("phase_diff", c_ubyte),
        ("mod_type", c_ubyte),
        ("mod_scheme", c_ubyte),
        ("tone_map", c_ubyte * 3),
        ("dt", c_ubyte),
        ("tworsblock", c_ubyte),
        ("lqi", c_ubyte),
        ("noise_power", c_ubyte * 72),
        ("signal_power", c_ubyte * 72),
        ("media_type", c_ubyte)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "pan_id = 0x{0:04X}".format(self.pan_id) + \
            " - lba_addr = 0x{0:04X}".format(self.lba_addr) + \
            " - rc_coord = 0x{0:04X}".format(self.rc_coord) + \
            " - snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) +\
            " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
            " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) +\
            " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) +\
            " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) +\
            " - dt = 0x{0:02X}".format(self.dt) + \
            " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) +\
            " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) +\
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_128bits_int(Structure):
    _pack_ = 1
    _fields_ = [("data", c_ubyte * 16)]
    def init_from_buffer(self, data):
        g3_hi_memmove_checked(self, data, sizeof(self))
    def __str__(self):
        return "data = 0x" + ''.join('{:02X}'.format(x) for x in self.data)

# Specific Fields Definitions
# End

# Specific Message Definitions

class g3_hi_msg_hwreset_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.HWRESET_REQ, 0, g3_hi_sec_mode.CLEAR_TEXT, 0)
    def __str__(self):
        return str(self.header)

class g3_hi_msg_hwreset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("sec_mode", c_ubyte)]
    def __str__(self):
        return str(self.header) + " - sec_mode: " + str(g3_hi_sec_mode(self.sec_mode))

class g3_hi_msg_mode_set_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("mode", c_ubyte)]
    def __init__(self, mode, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.MODESET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.mode = mode
    def __str__(self):
        return str(self.header) + " - mode: {}".format(g3_hi_mode(self.mode))

class g3_hi_msg_mode_set_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]    
    def __str__(self):
        return str(self.header)

class g3_hi_msg_mode_get_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.MODEGET_REQ, 0, g3_hi_sec_mode.CLEAR_TEXT, 0)    
    def __str__(self):
        return str(self.header)

class g3_hi_msg_mode_get_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("mode", c_ubyte)]
    def __str__(self):
        return str(self.header) + " - mode: {}".format(g3_hi_mode(self.mode))

class g3_hi_msg_sec_init_req(Structure):
    _pack_ = 1
    _fields_ = [("type", c_ubyte),
        ("rand_m", g3_hi_128bits_int)]

class g3_hi_msg_baudrate_set_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("baud_rate", c_uint)]
    def __init__(self, baud_rate, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = g3_hi_sec_mode.CLEAR_TEXT):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.BAUDRATESET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.baud_rate = baud_rate

class g3_hi_msg_baudrate_set_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("result", c_ubyte),
        ("baud_rate", c_uint)]

class g3_hi_msg_dbgtool_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("control", c_ubyte),
        ("data", c_uint)]
    def __init__(self, control, data, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.DBGTOOL_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.control = control
        self.data = data
    def __str__(self):   
        return str(self.header) + \
        " - control = " + str(g3_hi_dbgtool_ctrl(self.control)) + \
        " - data = 0x" + ''.join('{:02X}'.format(x) for x in self.data)

def g3_hi_msg_dbgtool_conf(msg, msg_len):
    msglen_nocrc = msg_len - sizeof(g3_hi_msg_ending)
    class g3_hi_msg_dbgtool_conf(g3_hi_msg_conf_base):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("fwkversion_patch", c_ubyte),
            ("fwkversion_minor", c_ubyte),
            ("fwkversion_major", c_ubyte),
            ("rsv", c_ubyte, 4),
            ("devtype", c_ubyte, 1),
            ("bandplan", c_ubyte, 3),
            ("lib_version", c_uint),
            ("rte_version", c_uint),
            ("mac_addr", g3_hi_ext_address),
            ("sflash_id", c_uint),
            ("usart_num", c_ubyte),
            ("led_conf", c_ubyte),
            ("platform", c_ubyte),
            ("rf_detected", c_ubyte),
            ("plc_mode", c_ubyte),        
            ("pan_id", c_ushort),        
            ("short_addr", c_ushort)]
        def __init__(self):
            # init default values (to handle legacy dbgtool conf in 4.x version)
            self.rf_detected = 0x00
            self.plc_mode = 0x00
            self.pan_id = 0xFFFF
            self.short_addr = 0xFFFF
        def __str__(self):   
            return str(self.header) + \
            " - fwk_version = " + '{0:02n}'.format(self.fwkversion_major) + '.{0:02n}'.format(self.fwkversion_minor) + '.{0:02n}'.format(self.fwkversion_patch) +\
            " - lib_version = 0x" + '{0:08X}'.format(self.lib_version) + \
            " - rte_version = 0x" + '{0:08X}'.format(self.rte_version) + \
            " - dev_type = " + str(g3_hi_device_type(self.devtype)) + \
            " - bandplan = " + str(g3_hi_band_plan(self.bandplan)) + \
            " - mac_addr = " + str(self.mac_addr) + \
            " - sflash_id = 0x" + '{0:08X}'.format(self.sflash_id) + \
            " - usart_num = " + str(self.usart_num) + \
            " - led_conf = " + str(self.led_conf) + \
            " - platform = " + str(g3_hi_platform(self.platform))+ \
            " - rf_detected = 0x" + '{0:02X}'.format(self.rf_detected) + \
            " - plc_mode = " + str(g3_hi_mode(self.plc_mode)) + \
            " - pan_id = 0x" + '{0:04X}'.format(self.pan_id) + \
            " - short_addr = 0x" + '{0:04X}'.format(self.short_addr)
    new_msg = g3_hi_msg_dbgtool_conf()
    memmove(byref(new_msg), byref(msg), msglen_nocrc)
    return new_msg

class g3_hi_msg_swreset_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("reset_data", g3_hi_swreset_data)]
    def __init__(self, prot_version, dev_type, mode, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3LIB_SWRESET_REQ, sizeof(g3_hi_swreset_data), sec_mode, counter)
        self.reset_data.prot_version = prot_version
        self.reset_data.dev_type = dev_type
        self.reset_data.mode = mode
    def __str__(self):
        return str(self.header) + " - reset_data = {}".format(str(self.reset_data))

class g3_hi_msg_swreset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("reset_data", g3_hi_swreset_data)]
    def __str__(self):   
        return str(self.header) + \
        " - status = {}".format(str(g3_hi_results(self.status))) + \
        " - reset_data = {}".format(str(self.reset_data))

class g3_hi_msg_g3lib_get_attrib_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("id", g3_hi_pib_id)]
    def __init__(self, attr_id, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3LIB_GET_REQ, sizeof(self.id), sec_mode, counter)
        self.id = attr_id

class g3_hi_msg_g3lib_set_attrib_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("state", c_ubyte),
        ("attribute", g3_hi_pib_id)]
    def __str__(self):   
        return str(self.header) + \
        " - state = {}".format(str(g3_hi_results(self.state))) + \
        " - attribute = {}".format(str(self.attribute))
        
class g3_hi_msg_adp_path_disc_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("dst_addr", c_ushort),
        ("metric_type", c_ubyte)]
    def __init__(self, dst_addr, metric_type = g3_hi_metric_type.LOADng_G3, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_PATHDISC_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.dst_addr = dst_addr
        self.metric_type = metric_type
    def __str__(self):
        return str(self.header) + " - dest_addr = 0x{0:04X}".format(self.dst_addr) + " - metric_type = " + str(self.metric_type)        

class g3_hi_msg_adp_route_disc_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("dst_addr", c_ushort),
        ("max_hops", c_ubyte),
        ("next_hop", c_ushort),
        ("media_type", c_ubyte)]
    def __init__(self, dst_addr, max_hops = g3_hi_defines.ADP_MAX_HOPS_DEFAULT, next_hop = g3_hi_defines.MAC_BRD_ADDR, media_type = g3_hi_mac_media_type.MEDIATYPE_PLC, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_ROUTEDISC_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.dst_addr = g3_hi_int_to_ctype(dst_addr, c_ushort)
        self.max_hops = g3_hi_int_to_ctype(max_hops, c_ubyte)
        self.next_hop = g3_hi_int_to_ctype(next_hop, c_ushort)
        self.media_type = g3_hi_int_to_ctype(media_type, c_ubyte)
    def __str__(self):
        return str(self.header) + " - dest_addr = 0x{0:04X}".format(self.dst_addr) + " - max_hops = 0x{0:02X}".format(self.dst_addr)  + " - next_hop = 0x{0:04X}".format(self.next_hop) + " - media_type = " + str(g3_hi_mac_media_type(self.media_type))             

class g3_hi_msg_adp_route_disc_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("route_entry", g3_hi_adp_routing_table_entry)]
    def __str__(self):
        return str(self.header) + " - status = " + str(g3_hi_results(self.status))  + " - rEntry = {}".format(self.route_entry)

class g3_hi_msg_boot_dev_pan_sort_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]

class g3_hi_msg_icmp_echo_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("data", g3_hi_ip_data_conf)]
    def __str__(self):
        return str(self.header) + " - " + str(self.data)

class g3_hi_msg_boot_srv_start_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("srv_addr", c_ushort),
        ("pan_id", c_ushort),
        ("req_type", c_ubyte)]
    def __init__(self, srv_addr, pan_id, req_type, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_SRV_START_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.srv_addr = srv_addr
        self.pan_id = pan_id
        self.req_type = req_type

class g3_hi_msg_boot_srv_start_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]

class g3_hi_msg_boot_srv_stop_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_SRV_STOP_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)

class g3_hi_msg_boot_srv_stop_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]

class g3_hi_msg_boot_srv_leave_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("ext_addr", g3_hi_ext_address)]
    def __str__(self):
        return str(self.header) + " - ext_addr = " + str(self.ext_addr)

class g3_hi_msg_boot_srv_kick_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("ext_addr", g3_hi_ext_address),
        ("short_addr", c_ushort)]
    def __init__(self, ext_addr, short_addr, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_SRV_KICK_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.ext_addr = ext_addr
        self.short_addr = short_addr
    def __str__(self):
        return str(self.header) + " - " + str(self.ext_addr) + " - short_addr = 0x{:04X}".format(self.short_addr)

class g3_hi_msg_boot_srv_kick_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return str(self.header) + " - status = " + str(g3_hi_results(self.status)) 

class g3_hi_msg_boot_srv_join_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("ext_addr", g3_hi_ext_address),
        ("short_addr", c_ushort)]
    def __str__(self):
        return str(self.header) + " - ext_addr = " + str(self.ext_addr) + " - short_addr = 0x" + '{0:04X}'.format(self.short_addr)      

class g3_hi_boot_dev_start_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("req_type", c_ubyte)]
    def __init__(self, req_type, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_DEV_START_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.req_type = req_type

class g3_hi_msg_boot_dev_start_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("net_addr", c_ushort),
        ("pan_id", c_ushort)]

class g3_hi_msg_boot_dev_leave_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)

class g3_hi_msg_boot_dev_leave_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]

class g3_hi_msg_boot_dev_leave_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]

class g3_hi_msg_boot_srv_setpsk_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("ext_addr", g3_hi_ext_address),
        ("psk", g3_hi_psk_key),
        ("short_addr", c_ushort)]
    def __init__(self, ext_addr, psk, short_addr, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_SRV_SETPSK_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.ext_addr = ext_addr
        self.psk = psk
        self.short_addr = short_addr

class g3_hi_msg_boot_srv_setpsk_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]

class g3_hi_msg_udp_data_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("data", g3_hi_ip_data_conf)]
    def __str__(self):
        return  str(self.header) + \
            " - " + str(self.data)        

class g3_hi_msg_udp_app_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("data", g3_hi_udp_app_conf)]   
    def __str__(self):
        return  str(self.header) + \
            " - " + str(self.data)

class g3_hi_msg_udp_conn_set_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("conn_id", c_ubyte),
        ("remote_addr", g3_hi_uip_ipaddr),
        ("remote_port", c_ushort),
        ("local_port", c_ushort)]
    def __init__(self, conn_id, remote_addr, remote_port, local_port, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3UDP_CONN_SET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.conn_id = conn_id
        memmove(addressof(self.remote_addr), addressof(remote_addr), sizeof(remote_addr))
        self.remote_port = remote_port
        self.local_port = local_port
    def __str__(self):
        return  str(self.header) + \
            " - conn_id = 0x{0:02X}".format(self.conn_id) + \
            " - remote_addr = " + str(self.remote_addr) + \
            " - remote_port = 0x{0:04X}".format(self.remote_port) + \
            " - local_port = 0x{0:04X}".format(self.local_port)

class g3_hi_msg_udp_conn_set_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return  str(self.header) + \
            " - status = " + str(g3_hi_results(self.status))   

class g3_hi_msg_udp_conn_get_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("conn_id", c_ubyte)]
    def __init__(self, conn_id, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3UDP_CONN_GET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.conn_id = conn_id
    def __str__(self):        
        return  str(self.header) + \
            " - conn_id = 0x{0:02X}".format(self.conn_id)

class g3_hi_msg_udp_conn_get_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("conn_id", c_ubyte),
        ("remote_addr", g3_hi_uip_ipaddr),
        ("remote_port", c_ushort),
        ("local_port", c_ushort)]
    def __str__(self):
        return  str(self.header) + \
            " - status = " + str(g3_hi_results(self.status)) + \
            " - conn_id = 0x{0:02X}".format(self.conn_id) + \
            " - remote_addr = " + str(self.remote_addr) + \
            " - remote_port = 0x{0:04X}".format(self.remote_port) + \
            " - local_port = 0x{0:04X}".format(self.local_port)        

class g3_hi_msg_phy_data_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("cs_threshold_ev", c_ubyte),
        ("tx_power_begin", c_ubyte),
        ("tx_power_end", c_ubyte),
        ("thermal_warning", c_ubyte)]

class g3_hi_msg_phy_rf_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):   
        return  str(self.header) + \
                "status = " + str(g3_hi_results(self.status))

class g3_hi_msg_phy_ack_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("fcs", c_ushort),
        ("dt_ssca", c_ubyte),
        ("crc5_8", c_ubyte)]
    def __init__(self, fcs, dt_ssca, crc5_8, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3PHY_ACK_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.fcs = fcs
        self.dt_ssca = dt_ssca
        self.crc5_8 = crc5_8

class g3_hi_msg_phy_ack_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("result", c_ubyte)]

class g3_hi_msg_phy_ack_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("fcs", c_ushort),
        ("dt_ssca", c_ubyte),
        ("crc5_8", c_ubyte)]

class g3_hi_msg_phy_set_trx_state_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("trx_state", c_ubyte)]
    def __init__(self, trx_state, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3PHY_SETTRXSTATE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.trx_state = trx_state

class g3_hi_msg_phy_set_trx_state_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("result", c_ubyte)]

class g3_hi_msg_phy_cs_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3PHY_CS_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)

class g3_hi_msg_phy_cs_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("result", c_ubyte)]

class g3_hi_msg_mac_data_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("msdu_handle", c_ubyte),
        ("status", c_ubyte),
        ("time_stamp", c_uint),
        ("media_type", c_ubyte)]
    def __str__(self):   
        return  str(self.header) + \
                " - status = " + str(g3_hi_results(self.status)) + \
                " - msdu_handle = 0x{0:02X}".format(self.msdu_handle) +\
                " - time_stamp = 0x{0:08X}".format(self.time_stamp) +\
                " - media_type = " + str(g3_hi_mac_media_type(self.media_type))   

class g3_hi_msg_mac_bcn_notify_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("bcn_notify_ind", g3_hi_bcn_notify_ind)]
    def __str__(self):   
        return  str(self.header) + " - " + str(self.bcn_notify_ind)     

class g3_hi_msg_mac_reset_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("set_default_pib", c_ubyte)]
    def __init__(self, set_default_pib, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3MAC_RESET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.set_default_pib = set_default_pib
    def __str__(self):   
        return  str(self.header) + " - set_default_pib = 0x{:02X}".format(self.set_default_pib)  

class g3_hi_msg_mac_reset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):   
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))       

class g3_hi_msg_mac_scan_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("scan_type", c_ubyte),
        ("scan_duration", c_ubyte)]
    def __init__(self, scan_type, scan_duration, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3MAC_SCAN_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.scan_type = scan_type
        self.scan_duration = scan_duration
    def __str__(self):   
        return  str(self.header) + \
                " - scan_type = " + str(g3_hi_mac_scan_type(self.scan_type)) + \
                " - scan_duration = {0:02X}".format(self.scan_duration)

class g3_hi_msg_mac_scan_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("scan_type", c_ubyte)]
    def __str__(self):
        return  str(self.header) + \
                " - status = " + str(g3_hi_results(self.status)) + \
                " - scan_type = {}".format(g3_hi_mac_scan_type(self.scan_type))


class g3_hi_msg_mac_start_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("pan_id", c_ushort)]
    def __init__(self, pan_id, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3MAC_START_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.pan_id = pan_id
    def __str__(self):
        return  str(self.header) + " - pan_id = {0:04X}".format(self.pan_id)

class g3_hi_msg_mac_start_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))

class g3_hi_msg_multipleget_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("id", c_uint),
        ("index", c_ushort)]
    def __init__(self, id, index, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3LIB_MULTIPLEGET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.id = id
        self.index = index
    def __str__(self):
        return  str(self.header) + " - id = 0x{0:04X}".format(self.id) + \
                    " - start index = 0x{0:02X}".format(self.index)

class g3_hi_msg_adp_data_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("nsdu_handle", c_ubyte),
        ("next_hop_media", c_ubyte)]
    def __str__(self):
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))+ " - nsdu_handle = {0:02X}".format(self.nsdu_handle) +\
                " - next_hop_media = " + str(g3_hi_mac_media_type(self.next_hop_media))


class g3_hi_msg_adp_discovery_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("duration", c_ubyte)]
    def __init__(self, duration, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_DISCOVERY_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.duration = duration
    def __str__(self):
        return  str(self.header) + " - duration = {0:02X}".format(self.duration)

class g3_hi_msg_adp_netstart_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("pan_id", c_ushort)]
    def __init__(self, pan_id, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_NETSTART_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.pan_id = pan_id
    def __str__(self):
        return  str(self.header) + " - pan_id = {0:04X}".format(self.pan_id)

class g3_hi_msg_adp_netstart_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))

class g3_hi_msg_adp_netjoin_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("pan_id", c_ushort),
        ("lba_addr", c_ushort),
        ("media_type", c_ubyte)]
    def __init__(self, pan_id, lba_addr, media_type=g3_hi_mac_media_type.MEDIATYPE_PLC, sec_mode=g3_hi_sec_mode.CLEAR_TEXT, counter=0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_NETJOIN_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.pan_id = pan_id
        self.lba_addr = lba_addr
        self.media_type = media_type
    def __str__(self):
        return  str(self.header) + " - pan_id = {0:04X}".format(self.pan_id) + " - lba_addr = {0:04X}".format(self.lba_addr) +\
                " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_msg_adp_netjoin_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("short_addr", c_ushort),
        ("pan_id", c_ushort)]
    def __str__(self):
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))+\
            " - pan_id = {0:04X}".format(self.pan_id) + " - short_addr = {0:04X}".format(self.short_addr) 
        
class g3_hi_msg_adp_netleave_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_NETLEAVE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
    def __str__(self):
        return str(self.header)

class g3_hi_msg_adp_netleave_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return str(self.header) + " - status = " + str(g3_hi_results(self.status))

class g3_hi_msg_adp_netleave_ind(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]
    def __str__(self):
        return str(self.header)

class g3_hi_msg_adp_reset_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_RESET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
    def __str__(self):
        return str(self.header)

class g3_hi_msg_adp_reset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return  str(self.header) + " - status = " + str(g3_hi_results(self.status))

class g3_hi_msg_adp_lbp_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("nsdu_handle", c_ubyte),
        ("media_type", c_ubyte)]
    def __str__(self):
        return  str(self.header)+ " - status = " + str(g3_hi_results(self.status)) + " - nsdu_handle = 0x{:02X}".format(self.nsdu_handle) +\
                " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

class g3_hi_msg_adp_routedelete_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("final_dst_addr", c_ushort)]
    def __init__(self, final_dst_addr, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_ROUTEDELETE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.final_dst_addr = final_dst_addr
    def __str__(self):
        return  str(self.header) + " - final_dst_addr = 0x{:04X}".format(self.final_dst_addr)

class g3_hi_msg_adp_routedelete_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte),
        ("final_dst_addr", c_ushort)]
    def __str__(self):
        return  str(self.header)+ " - status = " + str(g3_hi_results(self.status))+\
            " - final_dst_addr = 0x{:04X}".format(self.final_dst_addr)

class g3_hi_msg_resetstate_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.RESETSTATE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)

class g3_hi_msg_resetstate_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]

class g3_hi_msg_testset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]

class g3_hi_msg_testget_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req)]
    def __init__(self, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.TESTGET_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)

class g3_hi_msg_testget_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("nb_packets", c_uint),
        ("nb_bytes", c_uint),
        ("duration", c_uint)]

class g3_hi_msg_trace_req(Structure):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_req),
        ("operation", c_ubyte),
        ("data_1", c_uint),
        ("data_2", c_uint)]
    def __init__(self, operation, data_1, data_2, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
        g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.TRACE_REQ, sizeof(self) - sizeof(self.header), sec_mode, counter)
        self.operation = operation
        self.data_1 = data_1
        self.data_2 = data_2
    def __str__(self):
        return  str(self.header) +\
            " - operation = {}".format(str(g3_hi_trc_config_operation(self.operation))) +\
            " - data_1 = {:08X}".format(self.data_1) +\
            " - data_2 = {:08X}".format(self.data_2)

class g3_hi_msg_trace_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf)]
    def __str__(self):
        return  str(self.header)

# Specific Message Definitions
# End

# Functions to Create Variable Length Conf Messages

def g3_hi_msg_get_adp_net_status_ind(msg):
    msg_buff = msg
    src_addr_len = 0
    src_addr_mode = msg_buff[sizeof(g3_hi_gen_conf) + 1]
    if (2 == src_addr_mode):
        src_addr_len = 2
    elif (3 == src_addr_mode):
        src_addr_len = 8
    dst_addr_mode_idx = sizeof(g3_hi_gen_conf) + 1 + 1 + 2 + src_addr_len
    dst_addr_mode = msg_buff[dst_addr_mode_idx]
    dst_addr_len = 0
    if (2 == dst_addr_mode):
        dst_addr_len = 2
    elif (3 == dst_addr_mode):
        dst_addr_len = 8
    class adp_netstatus_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("status", c_ubyte),
            ("src_addr_mode", c_ubyte),
            ("src_pan_id", c_ushort),
            ("src_addr", c_ubyte * src_addr_len),
            ("dst_addr_mode", c_ubyte),
            ("dst_pan_id", c_ushort),
            ("dst_addr", c_ubyte * dst_addr_len),
            ("security_level", c_ubyte),
            ("key_id_mode", c_ubyte),
            ("key_index", c_ubyte),
            ("lqi", c_ubyte),
            ("media_type", c_ubyte),
            ("snr_carriers", c_ubyte * 72)]
        def __str__(self):
            if self.src_addr_mode == g3_hi_mac_addr_type.MODE_64:
                src = " - src_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.src_addr[::-1]) 
            elif self.src_addr_mode == g3_hi_mac_addr_type.MODE_16:
                src = " - src_addr = 0x{:04X}".format((self.src_addr[0] + self.src_addr[1]<<8))
            else:
                src = ''
            if self.dst_addr_mode == g3_hi_mac_addr_type.MODE_64:
                dst = " - dst_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.dst_addr[::-1]) 
            elif self.dst_addr_mode == g3_hi_mac_addr_type.MODE_16:
                dst = " - dst_addr = 0x{:04X}".format((self.dst_addr[0] + self.dst_addr[1]<<8))
            else:
                dst = ''
            return "status = " + str(g3_hi_results(self.status)) + \
            " - src_pan_id = 0x{0:04X}".format(self.src_pan_id) + \
            src + \
            " - dst_pan_id = 0x{0:04X}".format(self.dst_pan_id) + \
            dst + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

    new_msg = adp_netstatus_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_adp_path_disc_conf(msg, msg_len):
    hops_cnt = msg_len - sizeof(g3_hi_gen_conf) - sizeof(c_ushort) - \
        sizeof(c_ubyte) - sizeof(c_ushort) - sizeof(c_ushort) - 2 # the last 2 is for the crc
    if (hops_cnt % sizeof(g3_hi_loadng_prex_hop_info)) != 0:
        raise ValueError
    hops_cnt = int(hops_cnt / sizeof(g3_hi_loadng_prex_hop_info))
    class adp_path_disc_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("dst_addr", c_ushort),
            ("result", c_ubyte),
            ("metric_type", c_ushort, 4),
            ("reserved", c_ushort, 12),
            ("orig", c_ushort),
            ("hops_info", g3_hi_loadng_prex_hop_info * hops_cnt)]
        def __str__(self):
            s = str(self.header) + " - result = {}".format(str(g3_hi_results(self.result))) + \
                    " - dest_addr = 0x{0:04X}".format(self.dst_addr) + \
                    " - metric_type = " + str(self.metric_type) + \
                    " - orig_addr = 0x{0:04X}".format(self.orig)
            for i in range (0, hops_cnt):
                s += " - hop[{:2d}] ".format(i) + str(self.hops_info[i])
            return s

    new_msg = adp_path_disc_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_g3_hi_boot_dev_pan_sort_ind(msg, msg_len):
    msg_buff = msg
    pandescr_cnt = msg_buff[sizeof(g3_hi_gen_conf)]
    class boot_dev_pan_sort_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("pan_cnt", c_ubyte),
            ("pan_descr", g3_hi_pan_descriptor * pandescr_cnt)]
    new_msg = boot_dev_pan_sort_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_udp_data_ind(msg, msg_len):
    msg_buff = msg
    dev_addr_mode = msg_buff[sizeof(g3_hi_gen_conf)]
    dev_addr_len = 0
    if 2 == dev_addr_mode:
        dev_addr_len = 2
    elif 3 == dev_addr_mode:
        dev_addr_len = 8
    bc0num_displ = sizeof(g3_hi_gen_conf) + sizeof(c_ubyte) + \
        sizeof(c_ushort) + (sizeof(c_ubyte) * dev_addr_len) + \
        sizeof(c_ubyte) + sizeof(c_ubyte) + sizeof(c_ubyte) + \
        sizeof(c_ubyte) + (sizeof(c_ubyte) * g3_hi_defines.PHY_TONEMAP_SIZE)
    bc0_num = msg_buff[bc0num_displ]
    data_len_displ = bc0num_displ + sizeof(c_ubyte) + (sizeof(c_ubyte) * bc0_num) + sizeof(c_ubyte) + sizeof(c_ubyte) + (sizeof(g3_hi_uip_ipaddr) * 2) + \
        (sizeof(c_ushort) * 2)
    data_len = msg_buff[data_len_displ] | (msg_buff[data_len_displ + 1] << 8)
    class udp_data_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("prev_hop_addr_mode", c_ubyte),
            ("prev_hop_addr_pan_id", c_ushort),
            ("prev_hop_addr", c_ubyte * dev_addr_len),
            ("lqi", c_ubyte),
            ("phase_diff", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("tone_map", c_ubyte * g3_hi_defines.PHY_TONEMAP_SIZE),
            ("bc0_num", c_ubyte),
            ("bc0_list", c_ubyte * bc0_num),
            ("prev_hop_media", c_ubyte),
            ("conn_id", c_ubyte),
            ("src_addr", g3_hi_uip_ipaddr),
            ("src_port", c_ushort),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("dst_port", c_ushort),
            ("data_len", c_ushort),
            ("data", c_ubyte * data_len)]
        def __str__(self):   
            s = str(self.header)
            s += " - prev_hop_addr_mode = " + str(g3_hi_mac_addr_type(self.prev_hop_addr_mode))
            s += " - prev_hop_addr_pan_id = 0x{0:04X}".format(self.prev_hop_addr_pan_id)
            if 2 == dev_addr_mode:
                s += " - prev_hop_addr = {0:02X}{0:02X}".format(self.prev_hop_addr[1], self.prev_hop_addr[0])
            elif 3 == dev_addr_mode:
                s += " - prev_hop_addr = " + ''.join('{:02X}'.format(x) for x in self.prev_hop_addr)
            s += " - lqi = 0x{0:02X}".format(self.lqi)
            s += " - phase_diff = 0x{0:02X}".format(self.phase_diff)
            s += " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type))
            s += " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme))
            s += " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map)
            s += " - src_addr = " + str(self.src_addr)
            s += " - bc0_num = 0x{0:02X}".format(self.bc0_num)
            if self.bc0_num != 0:
                s += " - bc0_list = " +  ''.join(' {:02X}'.format(x) for x in self.bc0_list)
            s += " - prev_hop_media = " + str(g3_hi_mac_media_type(self.prev_hop_media))
            s += " - conn_id = 0x{0:04X}".format(self.conn_id)
            s += " - src_addr = " + str(self.src_addr)
            s += " - src_port = 0x{0:04X}".format(self.src_port)
            s += " - dst_addr = " + str(self.dst_addr)
            s += " - dst_port = 0x{0:04X}".format(self.dst_port)
            s += " - data_len = 0x{0:04X}".format(self.data_len)
            s += " - data = " + ''.join('{:02X}'.format(x) for x in self.data)
            return s
    new_msg = udp_data_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_udp_app_ind(msg, msg_len):
    payload_len = msg_len - sizeof(g3_hi_gen_conf) - \
        sizeof(c_ubyte) - sizeof(g3_hi_uip_ipaddr) - \
        sizeof(c_ushort) - 2 # the last 2 is for the crc
    class udp_app_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("app_req_id", c_ubyte),
            ("src_addr", g3_hi_uip_ipaddr),
            ("data_len", c_ushort),
            ("data", c_ubyte * payload_len)]
        def __str__(self):   
            return "app_req_id = " + str(g3_hi_udp_app_type(self.app_req_id)) + \
            " - src_" + str(self.src_addr) + \
            " - data_len = 0x{0:04X}".format(self.data_len) + \
            " - data = " + ''.join('{:02X}'.format(x) for x in self.data)
    new_msg = udp_app_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg    

def g3_hi_msg_get_app_rhi_resp(msg, msg_len):
    data_len = msg_len - sizeof(c_ubyte)
    class app_rhi_resp(Structure):
        _pack_ = 1
        _fields_ = [("status", c_ubyte),
            ("data", c_ubyte * data_len)]
    new_msg = app_rhi_resp()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg     

def g3_hi_msg_get_boot_srv_getpsk_ind(msg, msg_len):
    idp_len = msg_len - sizeof(g3_hi_gen_conf) - \
        sizeof(g3_hi_ext_address) - 2 # the last 2 is for the crc
    class boot_srv_getpsk_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("ext_addr", g3_hi_ext_address),
            ("idp", c_ubyte * idp_len)]   
        def __str__(self):
            return str(self.header) + " - ext_addr = {}".format(self.ext_addr) + \
                    " - idp = 0x" + ''.join('{:02X}'.format(x) for x in self.idp)
    new_msg = boot_srv_getpsk_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

#note: this function can be used for both echo request and echo reply
def g3_hi_msg_get_icmp_echo_ind(msg, msg_len):
    msg_buff = msg
    dev_addr_mode = msg_buff[sizeof(g3_hi_gen_conf)]
    dev_addr_len = 0
    if 2 == dev_addr_mode:
        dev_addr_len = 2
    elif 3 == dev_addr_mode:
        dev_addr_len = 8
    bc0num_displ = sizeof(g3_hi_gen_conf) + sizeof(c_ubyte) + \
        sizeof(c_ushort) + (sizeof(c_ubyte) * dev_addr_len) + \
        sizeof(c_ubyte) + sizeof(c_ubyte) + sizeof(c_ubyte) + \
        sizeof(c_ubyte) + (sizeof(c_ubyte) * g3_hi_defines.PHY_TONEMAP_SIZE)
    bc0_num = msg_buff[bc0num_displ]
    data_len_displ = bc0num_displ + sizeof(c_ubyte) + (sizeof(c_ubyte) * bc0_num) + \
        sizeof(c_ubyte) + sizeof(g3_hi_uip_ipaddr)
    data_len = msg_buff[data_len_displ] | (msg_buff[data_len_displ + 1] << 8)
    class icmp_echo_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("prev_hop_addr_mode", c_ubyte),
            ("prev_hop_addr_pan_id", c_ushort),
            ("prev_hop_addr", c_ubyte * dev_addr_len),
            ("lqi", c_ubyte),
            ("phase_diff", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("tone_map", c_ubyte * g3_hi_defines.PHY_TONEMAP_SIZE),
            ("bc0_num", c_ubyte),
            ("bc0_list", c_ubyte * bc0_num),
            ("prev_hop_media", c_ubyte),
            ("src_addr", g3_hi_uip_ipaddr),
            ("data_len", c_ushort),
            ("data", c_ubyte * data_len)]
        def __str__(self):
            s = str(self.header)
            s += " - prev_hop_addr_mode = " + str(g3_hi_mac_addr_type(self.prev_hop_addr_mode))
            s += " - prev_hop_addr_pan_id = 0x{0:04X}".format(self.prev_hop_addr_pan_id)
            if 2 == dev_addr_mode:
                s += " - prev_hop_addr = {0:02X}{0:02X}".format(self.prev_hop_addr[1], self.prev_hop_addr[0])
            elif 3 == dev_addr_mode:
                s += " - prev_hop_addr = " + ''.join('{:02X}'.format(x) for x in self.prev_hop_addr)
            s += " - lqi = 0x{0:02X}".format(self.lqi)
            s += " - phase_diff = 0x{0:02X}".format(self.phase_diff)
            s += " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type))
            s += " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme))
            s += " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map)
            s += " - src_addr = " + str(self.src_addr)
            s += " - bc0_num = 0x{0:02X}".format(self.bc0_num)
            if self.bc0_num != 0:
                s += " - bc0_list = " +  ''.join(' {:02X}'.format(x) for x in self.bc0_list)
            s += " - prev_hop_media = " + str(g3_hi_mac_media_type(self.prev_hop_media))
            s += " - data_len = 0x{0:04X}".format(self.data_len)
            s += " - data = " + ''.join('{:02X}'.format(x) for x in self.data)
            return s
    new_msg = icmp_echo_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_sflash_conf(msg, msg_len):
    payload_len = msg_len - sizeof(g3_hi_gen_conf) - sizeof(g3_hi_msg_ending)
    class sflash_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("payload", c_ubyte * payload_len)]
        def __str__(self):
            return str(self.header) + \
                " - payload = 0x" + ''.join('{0:02X}'.format(x) for x in self.payload)
    new_msg = sflash_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_g3lib_event_ind(msg, msg_len):
    msg_buff = msg
    ind_type = msg_buff[sizeof(g3_hi_gen_conf)]
    hops_cnt = 0
    if g3_hi_event_notification_type.PathRequestRx == ind_type:
        hops_cnt = int((msg_len - sizeof(g3_hi_gen_conf) -
            sizeof(c_ubyte) - sizeof(c_ubyte) -
            sizeof(c_ushort) - sizeof(c_ushort) -
            sizeof(c_uint)) / sizeof(g3_hi_loadng_prex_hop_info))
    class loadng_preq(Structure):
        _pack_ = 1
        _fields_ = [("type", c_ubyte),
            ("dst", c_ushort),
            ("orig", c_ushort),
            ("ext_data", c_uint),
            ("hops_info", g3_hi_loadng_prex_hop_info * hops_cnt)]
        def __str__(self):
            s = "type = " + str(g3_hi_loadng_msg_type(self.type)) +\
                    " - dest_addr = 0x{0:04X}".format(self.dst) + \
                    " - orig_addr = 0x{0:04X}".format(self.orig) + \
                    " - ext_data = 0x{0:08X}".format(self.ext_data)
            for i in range (0, hops_cnt):
                s += " - hop[{:2d}] ".format(i) + str(self.hops_info[i])
            return s
    class path_req_rx_info(Structure):
        _pack_ = 1
        _fields_ = [("preq_len", c_ushort),
            ("preq", loadng_preq)]
        def __str__(self):
            return " preq_len = {:04X}".format(self.preq_len) +\
                " - preq = {}".format(self.preq)
    class mem_leak_info(Structure):
        _pack_ = 1
        _fields_ = [("state", c_ubyte),
            ("final_addr", c_ushort)]
        def __str__(self):
            return " state = 0x{:02X}".format(self.state) +\
                " - final_addr = 0x{0:04X}".format(self.final_addr)
    class boot_failure_info(Structure):
        _pack_ = 1
        _fields_ = [("pan_descr", g3_hi_pan_descriptor),
                ("lba_joining_attempts", c_ubyte),
                ("status", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "pan_descr = {}, lba_joining_attempts = {}, status = {}".format(str(self.pan_descr),str(self.lba_joining_attempts),str(g3_hi_results(self.status)))
    class rte_band_mismatch_info(Structure):
        _pack_ = 1
        _fields_ = [("rte_band_mismatch", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "rte_band_mismatch = 0x{0:02X}".format(self.thermal_event_type)   
    class thermal_event_info(Structure):
        _pack_ = 1
        _fields_ = [("thermal_event_type", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "thermal_event_type = 0x{0:02X}".format(self.thermal_event_type)
    class tonemap_txrx_info(Structure):
        _pack_ = 1
        _fields_ = [("address", g3_hi_mac_device_addr),
            ("tonemap_resp_pay", g3_hi_tonemapresp_pay)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "address = " + str(self.address) + \
                " - tonemap_resp_pay = " + str(self.tonemap_resp_pay)
    class route_update_info(Structure):
        _pack_ = 1
        _fields_ = [("routing_table_entry", g3_hi_adp_routing_table_entry),
            ("originator", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "routing_table_entry = " + str(self.routing_table_entry) + \
                " - originator = " + str(self.originator)
    class phyqualityrx_info(Structure):
        _pack_ = 1
        _fields_ = [("src_address", g3_hi_mac_device_addr),
                ("snr_carriers", c_ubyte * 72),
                ("phase_diff", c_ubyte),
                ("mod_type", c_ubyte),
                ("mod_scheme", c_ubyte),
                ("tone_map", c_ubyte * 3),
                ("dt", c_ubyte),
                ("tworsblock", c_ubyte),
                ("lqi", c_ubyte),
                ("noise_power", c_ubyte * 72),
                ("signal_power", c_ubyte * 72)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "src_address = " + str(self.src_address) + \
                " - snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) +\
                " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
                " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) +\
                " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) +\
                " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) +\
                " - dt = 0x{0:02X}".format(self.dt) + \
                " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
                " - lqi = 0x{0:02X}".format(self.lqi) + \
                " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) +\
                " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power)
    class phyqualitytx_info(Structure):
        _pack_ = 1
        _fields_ = [("dst_address", g3_hi_mac_device_addr),
                ("signal_power", c_ubyte * 72)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "dst_address = " + str(self.dst_address) + \
                " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power)
    class phyqualityrx_rf_info(Structure):
        _pack_ = 1
        _fields_ = [("src_address", g3_hi_mac_device_addr),
                ("lqi", c_ubyte),
                ("bkgnoise", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "src_address = " + str(self.src_address) + \
                " - lqi = " + str(self.lqi) + \
                " - bkgnoise = " + str(self.bkgnoise)
    class phyqualitytx_rf_info(Structure):
        _pack_ = 1
        _fields_ = [("dst_address", g3_hi_mac_device_addr),
                ("rf_transmitpower", c_ubyte),
                ("dutycycle", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "dst_address = " + str(self.dst_address) + \
                " - rf_transmitpower = " + str(self.rf_transmitpower) + \
                " - dutycycle = " + str(self.dutycycle)
    class adpdatafw_info(Structure):
        _pack_ = 1
        _fields_ = [("dst_address", g3_hi_mac_device_addr),
                    ("data_type", c_ubyte)]
        def init_from_buffer(self, data):
            g3_hi_memmove_checked(self, data, sizeof(self))
        def __str__(self):
            return "dst_address = " + str(self.dst_address) + \
                   "data_type = " + str(self.data_type)

    class event_ind(Union):
        _pack_ = 1
        _fields_ = [("tonemap_txrx_info", tonemap_txrx_info),
            ("route_update_info", route_update_info),
            ("path_req_rx_info", path_req_rx_info),
            ("thermal_event_info", thermal_event_info),
            ("boot_failure_info", boot_failure_info),
            ("rte_band_mismatch_info", rte_band_mismatch_info),
            ("mem_leak_info", mem_leak_info),
            ("phyqualityrx_info", phyqualityrx_info),
            ("phyqualitytx_info", phyqualitytx_info),
            ("phyqualityrx_rf_info", phyqualityrx_rf_info),
            ("phyqualitytx_rf_info", phyqualitytx_rf_info),
            ("adpdatafw_info", adpdatafw_info),]
        def __str__(self):
            if g3_hi_event_notification_type.PathRequestRx == ind_type:
                return str(self.path_req_rx_info)
            elif g3_hi_event_notification_type.MemLeak == ind_type:
                return str(self.mem_leak_info)
            elif g3_hi_event_notification_type.BootFailure == ind_type:
                return str(self.boot_failure_info)
            elif g3_hi_event_notification_type.RTEBandMismatch == ind_type:
                return str(self.rte_band_mismatch_info)
            elif g3_hi_event_notification_type.ThermalEvent == ind_type:
                return str(self.thermal_event_info)
            elif g3_hi_event_notification_type.ToneMapRx == ind_type:
                return str(self.tonemap_txrx_info)
            elif g3_hi_event_notification_type.RouteUpdate == ind_type:
                return str(self.route_update_info)
            elif g3_hi_event_notification_type.PHYQualityRX == ind_type:
                return str(self.phyqualityrx_info)
            elif g3_hi_event_notification_type.PHYQualityTX == ind_type:
                return str(self.phyqualitytx_info)
            elif g3_hi_event_notification_type.PHYQualityRX_RF == ind_type:
                return str(self.phyqualityrx_rf_info)
            elif g3_hi_event_notification_type.PHYQualityTX_RF == ind_type:
                return str(self.phyqualitytx_rf_info)
            elif g3_hi_event_notification_type.ADPDataFW == ind_type:
                return str(self.adpdatafw_info)
            else:
                return ""
    class g3lib_event_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("type", c_ubyte),
            ("info", event_ind)]
        def __str__(self):
            return str(self.header) + \
                " - type = " + str(g3_hi_event_notification_type(self.type)) +\
                " - info = " + str(self.info)
    new_msg = g3lib_event_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_get_g3lib_sniffer_ind(msg, msg_len):
    msg_buff = msg
    index = sizeof(g3_hi_gen_conf) + sizeof(c_ubyte) + sizeof(c_uint)
    sniffer_payload_len = msg_buff[index] | (msg_buff[index + 1] << 8)
    class sniffer_phy_data_fixed(Structure):
        _pack_ = 1
        _fields_ = [("snr_carriers", c_ubyte * 72),
            ("phase_diff", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("tone_map", c_ubyte * 3),
            ("dt", c_ubyte),
            ("tworsblock", c_ubyte),
            ("lqi", c_ubyte),
            ("noise_power", c_ubyte * 72),
            ("signal_power", c_ubyte * 72),
            ("psdu_len", c_ushort)]
        def __str__(self):   
            return "snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) + \
            " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
            " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) + \
            " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) + \
            " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) + \
            " - dt = 0x{0:02X}".format(self.dt) + \
            " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) + \
            " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) + \
            " - psdu_len = 0x{0:04X}".format(self.psdu_len)
    if sniffer_payload_len - sizeof(sniffer_phy_data_fixed) >= 0:
        sniffer_phy_data_len = sniffer_payload_len - sizeof(sniffer_phy_data_fixed)
    else:
        sniffer_phy_data_len = 0
    class sniffer_phy_data(Structure):
        _pack_ = 1
        _fields_ = [("phy_data_fixed", sniffer_phy_data_fixed),
            ("psdu", c_ubyte * sniffer_phy_data_len)]
        def __str__(self):
            return str(self.phy_data_fixed) + " - psdu = " + ''.join('{:02X}'.format(x) for x in self.psdu)
    class sniffer_phy_ack(Structure):
        _pack_ = 1
        _fields_ = [("fcs", c_ushort),
            ("dt_ssca", c_ubyte),
            ("crc5_8", c_ubyte)]
        def __str__(self):
            return "fcs = 0x{0:04X}".format(self.fcs) + \
            " - dt_ssca = 0x{0:02X}".format(self.dt_ssca) + \
            " - crc5_8 = 0x{0:02X}".format(self.crc5_8)
    class sniffer_mac_beacon(Structure):
        _pack_ = 1
        _fields_ = [("src_addr", g3_hi_mac_device_addr),
            ("dst_addr", g3_hi_mac_device_addr),
            ("media_type", c_ubyte),
            ("lqi", c_ubyte),
            ("bsn", c_ubyte),
            ("rc_coord", c_ushort)]
        def __str__(self):
            return "src_addr = " + str(self.src_addr) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type)) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - bsn = 0x{0:02X}".format(self.bsn) + \
            " - rc_coord = 0x{0:04X}".format(self.rc_coord)
    class sniffer_mac_data_fixed(Structure):
        _pack_ = 1
        _fields_ = [("src_addr", g3_hi_mac_device_addr),
            ("dst_addr", g3_hi_mac_device_addr),
            ("media_type", c_ubyte),
            ("lqi", c_ubyte),
            ("dsn", c_ubyte),
            ("security_level", c_ubyte),
            ("key_index", c_ubyte),
            ("qos", c_ubyte),
            ("msdu_len", c_ushort)]
        def __str__(self):
            return "src_addr = " + str(self.src_addr) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type)) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - dsn = 0x{0:02X}".format(self.dsn) + \
            " - security_level = 0x{0:02X}".format(self.security_level) + \
            " - key_index = 0x{0:02X}".format(self.key_index) + \
            " - qos = 0x{0:02X}".format(self.qos)
    if sniffer_payload_len >= sizeof(sniffer_mac_data_fixed):
        sniffer_mac_data_len = sniffer_payload_len - sizeof(sniffer_mac_data_fixed)
    else:
        sniffer_mac_data_len = 0
    class sniffer_mac_data(Structure):
        _pack_ = 1
        _fields_ = [("mac_data_fixed", sniffer_mac_data_fixed),
            ("msdu", c_ubyte * sniffer_mac_data_len)]
        def __str__(self):
            return str(self.mac_data_fixed) + " - msdu = " + ''.join('{:02X}'.format(x) for x in self.msdu)
    class sniffer_mac_cmd_fixed(Structure):
        _pack_ = 1
        _fields_ = [("src_addr", g3_hi_mac_device_addr),
            ("dst_addr", g3_hi_mac_device_addr),
            ("media_type", c_ubyte),
            ("lqi", c_ubyte),
            ("cmd_id", c_ubyte)]
        def __str__(self):
            return "src_addr = " + str(self.src_addr) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type)) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - cmd_id = 0x{0:02X}".format(self.cmd_id)
    if sniffer_payload_len >= sizeof(sniffer_mac_cmd_fixed):
        sniffer_mac_cmd_len = sniffer_payload_len - sizeof(sniffer_mac_cmd_fixed)
    else:
        sniffer_mac_cmd_len = 0
    class sniffer_mac_cmd(Structure):
        _pack_ = 1
        _fields_ = [("mac_cmd_fixed", sniffer_mac_cmd_fixed),
            ("cmd", c_ubyte * sniffer_mac_cmd_len)]
        def __str__(self):
            return str(self.mac_cmd_fixed) + " - cmd = " + ''.join('{:02X}'.format(x) for x in self.cmd)
    class sniffer_phy_rf_fixed(Structure):
        _pack_ = 1
        _fields_ = [("rssi", c_ubyte),
            ("noise", c_ubyte),
            ("payload_len", c_short)]
        def __str__(self):   
            return "rssi = 0x{0:02X}".format(self.rssi) + \
            " - noise = 0x{0:02X}".format(self.noise) + \
            " - payload_len = 0x{0:04X}".format(self.payload_len)
    if sniffer_payload_len >= sizeof(sniffer_phy_rf_fixed):
        sniffer_phy_rf_len = sniffer_payload_len - sizeof(sniffer_phy_rf_fixed)
    else:
        sniffer_phy_rf_len = 0
    class sniffer_phy_rf(Structure):
        _pack_ = 1
        _fields_ = [("phy_rf_fixed", sniffer_phy_rf_fixed),
            ("payload", c_ubyte * sniffer_phy_rf_len)]
        def __str__(self):
            return str(self.phy_rf_fixed) + " - payload = " + ''.join('{:02X}'.format(x) for x in self.payload)
    class sniffer_mac_rf_ack(Structure):
        _pack_ = 1
        _fields_ = [("src_addr", g3_hi_mac_device_addr),
            ("dst_addr", g3_hi_mac_device_addr),
            ("lqi", c_ubyte),
            ("seq_num", c_ubyte)]
        def __str__(self):
            return "src_addr = " + str(self.src_addr) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - seq_num = 0x{0:02X}".format(self.seq_num)
    class sniffer_payload(Union):
        _pack_ = 1
        _fields_ = [("phy_data", sniffer_phy_data),
            ("phy_ack", sniffer_phy_ack),
            ("mac_beacon", sniffer_mac_beacon),
            ("mac_data", sniffer_mac_data),
            ("mac_cmd", sniffer_mac_cmd),
            ("phy_rf", sniffer_phy_rf),
            ("mac_rf_ack", sniffer_mac_rf_ack)]
    class g3lib_sniffer_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("frame_type", c_ubyte),
            ("time_stamp", c_uint),
            ("payload_len", c_ushort),
            ("payload", sniffer_payload)]
        def __str__(self):
            return str(self.header) + \
            " - frame_type = " + str(g3_hi_sniffer_frame_type(self.frame_type)) + \
            " - time_stamp = 0x{0:08X}".format(self.time_stamp)
    new_msg = g3lib_sniffer_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg    

def g3_hi_msg_get_g3lib_get_attrib_conf(msg, msg_len):
    msg_buff = msg
    index = sizeof(g3_hi_gen_conf) + sizeof(c_ubyte) + sizeof(g3_hi_pib_id)
    attrib_len = msg_buff[index] | (msg_buff[index + 1] << 8)
    class get_attrib_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("state", c_ubyte),
            ("id", g3_hi_pib_id),
            ("len", c_ushort),
            ("value", c_ubyte * attrib_len)]
        def __str__(self):
            return str(self.header) + " - state = " + str(self.state) + \
                    " - id = " + str(self.id) + \
                    " - len = 0x" + '{0:04X}'.format(self.len) + \
                    " - value = 0x" + ''.join('{0:02X}'.format(b) for b in self.value)            
    new_msg = get_attrib_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_nvm_conf(msg, msg_len):
    msg_buff = msg
    index = sizeof(g3_hi_gen_conf) + sizeof(c_ubyte)
    msg_len = msg_buff[index] | (msg_buff[index + 1] << 8)
    class nvm_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("status", c_ubyte),
            ("size", c_ushort),
            ("data", c_ubyte * msg_len)]
        def __str__(self):
            return str(self.header) + " - status = " + str(self.status) + \
                    " - size = 0x" + '{0:04X}'.format(self.size) + \
                    " - data = 0x" + ''.join('{0:02X}'.format(b) for b in self.data)
    new_msg = nvm_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_phy_data_ind(msg, msg_len):
    msg_buff = msg
    index = sizeof(g3_hi_gen_conf) + 72 + 1 + 1 + 1 + 3 + 1 + 1 + 1 + 72 + 72
    payload_len = msg_buff[index] | (msg_buff[index + 1] << 8)
    class phy_data_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("snr_carriers", c_ubyte * 72),
            ("phase_diff", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("tone_map", c_ubyte * 3),
            ("dt", c_ubyte),
            ("tworsblock", c_ubyte),
            ("lqi", c_ubyte),
            ("noise_power", c_ubyte * 72),
            ("signal_power", c_ubyte * 72),
            ("payload_len", c_ushort),
            ("payload", c_ubyte * payload_len)]
        def __str__(self):   
            return str(self.header) + \
            " - snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) +\
            " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
            " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) +\
            " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) +\
            " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) +\
            " - dt = 0x{0:02X}".format(self.dt) + \
            " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) +\
            " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) +\
            " - payload_len = 0x{0:04X}".format(self.payload_len) + \
            " - payload = " + ''.join('{:02X}'.format(x) for x in self.payload)
    new_msg = phy_data_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_phy_rf_ind(msg, msg_len):
    class phy_rf_ind_fixed(Structure):
        _pack_ = 1
        _fields_ = [("rssi", c_ubyte),
            ("noise", c_ubyte),
            ("payload_len", c_short)]
        def __str__(self):   
            return " - rssi = 0x{0:02X}".format(self.rssi) + \
            " - noise = 0x{0:02X}".format(self.noise) + \
            " - payload_len = 0x{0:04X}".format(self.payload_len)
    # Compute the size of the fixed part of the message (header + fixed part of RF-DATA.ind + CRC)
    ind_fixed_size = sizeof(g3_hi_gen_conf) + sizeof(phy_rf_ind_fixed) + sizeof(c_short)  
    if msg_len - ind_fixed_size >= 0:
        payload_len = msg_len - ind_fixed_size
    else:
        payload_len = 0
    class phy_rf_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("phy_rf_ind_fixed", phy_rf_ind_fixed),
            ("payload", c_ubyte * payload_len)]
        def __str__(self):
            return str(self.header) + \
                str(self.phy_rf_ind_fixed) + \
                " - payload = " + ''.join('{:02X}'.format(x) for x in self.payload)
    new_msg = phy_rf_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_mac_data_ind(msg, msg_len):
    msg_buff = msg
    src_addr_len = 0
    src_addr_mode = msg_buff[sizeof(g3_hi_gen_conf)]
    if (2 == src_addr_mode):
        src_addr_len = 2
    elif (3 == src_addr_mode):
        src_addr_len = 8
    dst_addr_mode_idx = sizeof(g3_hi_gen_conf) + 1 + 2 + src_addr_len
    dst_addr_mode = msg_buff[dst_addr_mode_idx]
    dst_addr_len = 0
    if (2 == dst_addr_mode):
        dst_addr_len = 2
    elif (3 == dst_addr_mode):
        dst_addr_len = 8
    msdu_len_idx = dst_addr_mode_idx + 1 + 2 + dst_addr_len
    msdu_len = msg_buff[msdu_len_idx] | (msg_buff[msdu_len_idx + 1] << 8)
    sec_level_idx = dst_addr_mode_idx + 1 + 2 + dst_addr_len + 2 + msdu_len + 1 + 4
    sec_level = msg_buff[sec_level_idx]
    if sec_level == 0:
        if src_addr_len == 8 and dst_addr_len == 8:
            class mac_data_ind_no_sec_6464(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ubyte * src_addr_len),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ubyte * dst_addr_len),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    #("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_no_sec_6464()
        if src_addr_len == 2 and dst_addr_len == 8:
            class mac_data_ind_no_sec_1664(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ushort),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ubyte * dst_addr_len),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    #("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_no_sec_1664()            
        if src_addr_len == 8 and dst_addr_len == 2:
            class mac_data_ind_no_sec_6416(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ubyte * src_addr_len),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ushort),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    #("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
                def __str__(self):  
                    return str(self.header) + \
                    " - src_addr_mode = 0x{0:02X}".format(self.src_addr_mode) + \
                    " - src_pan_id = 0x{0:02X}".format(self.src_pan_id) + \
                    " - src_addr = ".join('{:02X}'.format(x) for x in self.src_addr) + \
                    " - dst_addr_mode = 0x{0:02X}".format(self.dst_addr_mode) + \
                    " - dst_pan_id = 0x{0:02X}".format(self.dst_pan_id) + \
                    " - dst_addr = 0x{0:02X}".format(self.dst_addr) + \
                    " - msdu_len = 0x{0:04X}".format(self.msdu_len) + \
                    " - msdu = " + ''.join('{:02X}'.format(x) for x in self.msdu) +\
                    " - dsn = 0x{0:02X}".format(self.dsn) + \
                    " - time_stamp = 0x{0:08X}".format(self.time_stamp) + \
                    " - security_level = 0x{0:02X}".format(self.security_level) + \
                    " - qos = 0x{0:02X}".format(self.qos) + \
                    " - snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) + \
                    " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
                    " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) + \
                    " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) + \
                    " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) + \
                    " - dt = 0x{0:02X}".format(self.dt) + \
                    " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
                    " - lqi = 0x{0:02X}".format(self.lqi) + \
                    " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) + \
                    " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) +\
                    " - media_type = " + str(g3_hi_mac_media_type(self.media_type))
            new_msg = mac_data_ind_no_sec_6416()    
        if src_addr_len == 2 and dst_addr_len == 2:
            class mac_data_ind_no_sec_1616(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ushort),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ushort),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    #("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_no_sec_1616()                    
    else:
        if src_addr_len == 8 and dst_addr_len == 8:
            class mac_data_ind_6464(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ubyte * src_addr_len),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ubyte * dst_addr_len),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    ("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_6464()        
        if src_addr_len == 2 and dst_addr_len == 8:
            class mac_data_ind_1664(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ushort),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ubyte * dst_addr_len),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    ("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_1664()        
        if src_addr_len == 8 and dst_addr_len == 2:
            class mac_data_ind_6416(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ubyte * dst_addr_len),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ushort),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    ("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
            new_msg = mac_data_ind_6416()        
        if src_addr_len == 2 and dst_addr_len == 2:
            class mac_data_ind_1616(Structure):
                _pack_ = 1
                _fields_ = [("header", g3_hi_gen_conf),
                    ("src_addr_mode", c_ubyte),
                    ("src_pan_id", c_ushort),
                    ("src_addr", c_ushort),
                    ("dst_addr_mode", c_ubyte),
                    ("dst_pan_id", c_ushort),
                    ("dst_addr", c_ushort),
                    ("msdu_len", c_ushort),
                    ("msdu", c_ubyte * msdu_len),
                    ("dsn", c_ubyte),
                    ("time_stamp", c_uint),
                    ("security_level", c_ubyte),
                    ("key_index", c_ubyte),
                    ("qos", c_ubyte),
                    ("snr_carriers", c_ubyte * 72),
                    ("phase_diff", c_ubyte),
                    ("mod_type", c_ubyte),
                    ("mod_scheme", c_ubyte),
                    ("tone_map", c_ubyte * 3),
                    ("dt", c_ubyte),
                    ("tworsblock", c_ubyte),
                    ("lqi", c_ubyte),
                    ("noise_power", c_ubyte * 72),
                    ("signal_power", c_ubyte * 72),
                    ("media_type", c_ubyte)]
                def __str__(self):  
                    return str(self.header) + \
                    " - src_addr_mode = 0x{0:02X}".format(self.src_addr_mode) + \
                    " - src_pan_id = 0x{0:02X}".format(self.src_pan_id) + \
                    " - src_addr = 0x{0:02X}".format(self.src_addr) + \
                    " - dst_addr_mode = 0x{0:02X}".format(self.dst_addr_mode) + \
                    " - dst_pan_id = 0x{0:02X}".format(self.dst_pan_id) + \
                    " - dst_addr = 0x{0:02X}".format(self.dst_addr) + \
                    " - msdu_len = 0x{0:04X}".format(self.msdu_len) + \
                    " - msdu = " + ''.join('{:02X}'.format(x) for x in self.msdu) +\
                    " - dsn = 0x{0:02X}".format(self.dsn) + \
                    " - time_stamp = 0x{0:08X}".format(self.time_stamp) + \
                    " - security_level = 0x{0:02X}".format(self.security_level) + \
                    " - key_index = 0x{0:02X}".format(self.key_index) + \
                    " - qos = 0x{0:02X}".format(self.qos) + \
                    " - snr_carriers = " + ''.join('{:02X}'.format(x) for x in self.snr_carriers) + \
                    " - phase_diff = 0x{0:02X}".format(self.phase_diff) + \
                    " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) + \
                    " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) + \
                    " - tone_map = " + ''.join('{:02X}'.format(x) for x in self.tone_map) + \
                    " - dt = 0x{0:02X}".format(self.dt) + \
                    " - tworsblock = 0x{0:02X}".format(self.tworsblock) + \
                    " - lqi = 0x{0:02X}".format(self.lqi) + \
                    " - noise_power = " + ''.join('{:02X}'.format(x) for x in self.noise_power) + \
                    " - signal_power = " + ''.join('{:02X}'.format(x) for x in self.signal_power) +\
                    " - media_type = " + str(g3_hi_mac_media_type(self.media_type))
            new_msg = mac_data_ind_1616()            
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_mac_commstatus_ind(msg, msg_len):
    msg_buff = msg
    src_addr_len = 0
    src_addr_mode = msg_buff[sizeof(g3_hi_gen_conf) + 1]
    if (2 == src_addr_mode):
        src_addr_len = 2
    elif (3 == src_addr_mode):
        src_addr_len = 8
    dst_addr_mode_idx = sizeof(g3_hi_gen_conf) + 1 + 1 + 2 + src_addr_len
    dst_addr_mode = msg_buff[dst_addr_mode_idx]
    dst_addr_len = 0
    if (2 == dst_addr_mode):
        dst_addr_len = 2
    elif (3 == dst_addr_mode):
        dst_addr_len = 8
    class mac_commstatus_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("status", c_ubyte),
            ("src_addr_mode", c_ubyte),
            ("src_pan_id", c_ushort),
            ("src_addr", c_ubyte * src_addr_len),
            ("dst_addr_mode", c_ubyte),
            ("dst_pan_id", c_ushort),
            ("dst_addr", c_ubyte * dst_addr_len),
            ("security_level", c_ubyte),
            ("key_id_mode", c_ubyte),
            ("key_index", c_ubyte),
            ("lqi", c_ubyte),
            ("media_type", c_ubyte),
            ("snr_carriers", c_ubyte * 72)]
        def __str__(self):   
            if self.src_addr_mode == g3_hi_mac_addr_type.MODE_64:
                src = " - src_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.src_addr[::-1]) 
            elif self.src_addr_mode == g3_hi_mac_addr_type.MODE_16:
                src = " - src_addr = 0x{:04X}".format((self.src_addr[0] + self.src_addr[1]<<8))
            else:
                src = ''
            if self.dst_addr_mode == g3_hi_mac_addr_type.MODE_64:
                dst = " - dst_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.dst_addr[::-1]) 
            elif self.dst_addr_mode == g3_hi_mac_addr_type.MODE_16:
                dst = " - dst_addr = 0x{:04X}".format((self.dst_addr[0] + self.dst_addr[1]<<8))
            else:
                dst = ''
            return "status = " + str(g3_hi_results(self.status)) + \
            " - src_pan_id = 0x{0:04X}".format(self.src_pan_id) + \
            src + \
            " - dst_pan_id = 0x{0:04X}".format(self.dst_pan_id) + \
            dst + \
            " - lqi = 0x{0:02X}".format(self.lqi) + \
            " - media_type = " + str(g3_hi_mac_media_type(self.media_type))
    new_msg = mac_commstatus_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg


def g3_hi_msg_g3lib_multipleget_conf(msg, msg_len):
    pibs_data_len = msg_len - sizeof(g3_hi_gen_conf) - sizeof(g3_hi_msg_ending)
    class g3lib_multipleget_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("pibs", c_ubyte * pibs_data_len)]
        def __str__(self):
            s = " - pibs = 0x" + ''.join('{:02X}'.format(x) for x in self.pibs) 
            return  str(self.header) + s
    new_msg = g3lib_multipleget_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_adp_data_ind(msg, msg_len):
    msg_buff = msg
    nsdu_len = msg_buff[sizeof(g3_hi_gen_conf)] | (msg_buff[sizeof(g3_hi_gen_conf) + 1] << 8)
    prevhop_addr_idx = sizeof(g3_hi_gen_conf) + 2
    prevhop_addr_mode = msg_buff[prevhop_addr_idx]
    prevhop_addr_len = 0
    if (2 == prevhop_addr_mode):
        prevhop_addr_len = 2
    elif (3 == prevhop_addr_mode):
        prevhop_addr_len = 8
    bc0_num = msg_buff[prevhop_addr_idx + 1 + 2 + prevhop_addr_len + 1 + 1 + 1 + 1 + 3]
    class adp_data_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("nsdu_len", c_ushort),
            ("prevhop_addr_mode", c_ubyte),
            ("prevhop_pan_id", c_ushort),
            ("prevhop_addr", c_ubyte * prevhop_addr_len),
            ("lqi", c_ubyte),
            ("phase_diff", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("tone_map", c_ubyte * 3),
            ("bc0_num", c_ubyte),
            ("bc0", c_ubyte * bc0_num),
            ("prev_hop_media", c_ubyte),
            ("nsdu", c_ubyte * nsdu_len)]
        def __str__(self):
            return  str(self.header) + " - lqi = 0x{0:02X}".format(self.lqi) + \
                    " - prevhop_pan_id = 0x{0:04X}".format(self.prevhop_pan_id)  + \
                    " - prevhop_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.prevhop_addr) + \
                    " - phase_diff = 0x{0:02X}".format(self.phase_diff)  + \
                    " - mod_type = " + str(g3_hi_phy_mod_type(self.mod_type)) + \
                    " - mod_scheme = " + str(g3_hi_mod_scheme(self.mod_scheme)) + \
                    " - tone_map = 0x" + ''.join('{:02X}'.format(x) for x in self.tone_map) + \
					" - prev_hop_media = " + str(g3_hi_mac_media_type(self.prev_hop_media))  + \
                    " - bc0_num = " + str(self.bc0_num) + \
                    " - nsdu = 0x" + ''.join('{:02X}'.format(x) for x in self.nsdu)
    new_msg = adp_data_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_adp_discovery_conf(msg, msg_len):
    msg_buff = msg
    pan_cnt = msg_buff[sizeof(g3_hi_gen_conf) + 1]
    class adp_discovery_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("status", c_ubyte),
            ("pan_count", c_ubyte),
            ("pan_descr_list", g3_hi_pan_descriptor * pan_cnt)]
        def __str__(self):   
            s = ''
            for i in range(0, pan_cnt):                
                s += ' [{}] '.format(i) + str(self.pan_descr_list[i])
            return str(self.header) + \
            " - status = " + str(g3_hi_results(self.status)) +\
            " - pan_count = 0x{:02X}".format(self.pan_count) +\
            " - pan_descr_list =" + s            
    new_msg = adp_discovery_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_adp_lbp_ind(msg, msg_len):
    msg_buff = msg
    src_addr_len = 0
    src_addr_mode = msg_buff[sizeof(g3_hi_gen_conf)]
    if (2 == src_addr_mode):
        src_addr_len = 2
    elif (3 == src_addr_mode):
        src_addr_len = 8
    nsdu_len_idx = sizeof(g3_hi_gen_conf) + 1 + 2 + src_addr_len
    nsdu_len = msg_buff[nsdu_len_idx] | (msg_buff[nsdu_len_idx + 1] << 8)
    class adp_lbp_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("src_addr_mode", c_ubyte),
            ("src_pan_id", c_ushort),
            ("src_addr", c_ubyte * src_addr_len),
            ("nsdu_len", c_ushort),
            ("nsdu", c_ubyte * nsdu_len),
            ("lqi", c_ubyte),
            ("security_enabled", c_ubyte),
			("media_type", c_ubyte)]
        def __str__(self):  
            if self.src_addr_mode == g3_hi_mac_addr_type.MODE_64:
                s = " - src_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.src_addr[::-1]) 
            elif self.src_addr_mode == g3_hi_mac_addr_type.MODE_16:
                s = " - src_addr = 0x{:04X}".format((self.src_addr[0] + self.src_addr[1]<<8))
            else:
                s = ''
            return str(self.header) + \
                " - src_addr_mode = " + str(g3_hi_mac_addr_type(self.src_addr_mode)) +\
                " - src_pan_id = 0x{:04X}".format(self.src_pan_id) +\
                s +\
                " - nsdu_len = 0x{0:04X}".format(self.nsdu_len) + \
                " - nsdu = " + ''.join('{:02X}'.format(x) for x in self.nsdu) +\
                " - lqi = 0x{0:02X}".format(self.lqi) + \
                " - security_enabled = 0x{0:02X}".format(self.security_enabled) +\
                " - media_type = " + str(g3_hi_mac_media_type(self.media_type))

    new_msg = adp_lbp_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_otp_conf(msg, msg_len):
    msg_buff = msg
    payload_len = msg_len - sizeof(g3_hi_gen_conf) - 1 - 1 - 1 - 2
    class otp_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("operation", c_ubyte),
            ("region", c_ubyte),
            ("status", c_ubyte),
            ("payload", c_ubyte * payload_len)]
        def __str__(self):
            return  str(self.header) +\
                    " - operation = " + str(g3_hi_otp_operation(self.operation)) +\
                    " - region = " + str(g3_hi_otp_region(self.region)) +\
                    " - status = " + str(g3_hi_otp_status(self.status)) +\
                    " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)        
    new_msg = otp_conf()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

def g3_hi_msg_hwconfig_conf(msg, msg_len):
    msg_payload_len = msg_len - sizeof(g3_hi_gen_conf) - sizeof(g3_hi_msg_ending)
    hwconfig_payload_len = msg_payload_len - 3 * sizeof(c_ubyte) # msg_len - 1 (Status) - 1 (Type) - 1 (PayloadLen)
    if hwconfig_payload_len > 1:
        s2lpregread_datalen = hwconfig_payload_len - 1
    else:
        s2lpregread_datalen = 0
    class hwconfig_s2lpregread(Structure):
        _pack_ = 1
        _fields_ = [("len", c_ubyte),
            ("data", c_ubyte * s2lpregread_datalen)]
        def __str__(self):
            return "data = 0x" + ''.join('{:02X}'.format(x) for x in self.data)
    class hwconfig_conf_payload(Union):
        _pack_ = 1
        _fields_ = [("value", c_ubyte),
            ("baud_rate", c_uint),
            ("s2lp_regread", hwconfig_s2lpregread)]              
    class hwconfig_conf(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
            ("status", c_ubyte),
            ("config_type", c_ubyte),
            ("config_paylen", c_ubyte),
            ("config_payload", hwconfig_conf_payload)]
        def __str__(self):
            s = str(self.header) + \
                " - status = " + str(g3_hi_results(self.status)) +\
                " - config_type = " + str(g3_hi_hwconfig_type(self.config_type)) +\
                " - config_paylen = 0x{:02X}".format(self.config_paylen)                    
            if self.config_paylen > 0:
                if self.config_type == g3_hi_hwconfig_type.GPIO_READ:
                    s += " - value = " + str(self.config_payload.value)
                elif self.config_type == g3_hi_hwconfig_type.BAUDRATE_CONFIG:
                    s += " - baud_rate = " + str(self.config_payload.baud_rate)
                elif self.config_type == g3_hi_hwconfig_type.S2LP_REG_READ:
                    s += " - " + str(self.config_payload.s2lp_regread)
            return s    
    new_msg = hwconfig_conf()
    memmove(byref(new_msg), byref(msg), msg_len - sizeof(g3_hi_msg_ending))
    return new_msg

def g3_hi_msg_error_ind(msg, msg_len):
    payload_len = msg_len - sizeof(g3_hi_gen_conf)
    class error_ind(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_conf),
                    ("payload", c_ubyte * payload_len)]
        def __str__(self):
            return str(self.header) + \
                " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)
    new_msg = error_ind()
    memmove(byref(new_msg), byref(msg), sizeof(new_msg))
    return new_msg

# Functions to Create Variable Length Conf Messages
# End

# Functions to Create Request Messages

def g3_hi_msg_get_g3lib_set_attrib_req(attrib_id, attrib_idx, attrib_len, attrib_val, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    if isinstance(attrib_val, int):
        attrib_val = g3_hi_int2ctype(attrib_len, attrib_val)
    class set_attr_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("id", g3_hi_pib_id),
            ("len", c_ushort),
            ("value", c_ubyte * attrib_len)]
        def __init__(self, attrib_id_, attrib_idx_, attrib_len_, attrib_val_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3LIB_SET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.id.id = c_uint(attrib_id_)
            self.id.idx = c_ushort(attrib_idx_)
            self.len = c_ushort(attrib_len_)
            memmove(byref(self.value), addressof(attrib_val_), attrib_len_)

    return set_attr_req(attrib_id, attrib_idx, attrib_len, attrib_val, sec_mode, counter)

def g3_hi_msg_get_g3_icmp_echo_req(dst_addr, handle, data_len, data, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class icmp_echo_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("handle", c_ubyte),
            ("data_len", c_ushort),
            ("data", c_ubyte * data_len)]
        def __str__(self):
            return str(self.header) + \
            " - handle = 0x{0:02X}".format(self.handle) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - data_len = 0x{0:04X}".format(self.data_len) + \
            " - data = " + ''.join('{:02X}'.format(x) for x in self.data)
        def __init__(self, dst_addr_, handle_, data_len_, data_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ICMP_ECHO_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            memmove(addressof(self.dst_addr), addressof(dst_addr_), sizeof(dst_addr_))
            self.handle = handle_
            self.data_len = data_len_
            memmove(addressof(self.data), addressof(data_), data_len_)

    return icmp_echo_req(dst_addr, handle, data_len, data, sec_mode, counter)

def g3_hi_msg_get_g3_udp_data_req(handle, conn_id, dst_addr, dst_port, data_len, data, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class udp_data_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("handle", c_ubyte),
            ("conn_id", c_ubyte),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("dst_port", c_ushort),
            ("data_len", c_ushort),
            ("data", c_ubyte * data_len)]
        def __init__(self, handle_, conn_id_, dst_addr_, dst_port_, data_len_, data_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3UDP_DATA_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.handle = handle_
            self.conn_id = conn_id_
            memmove(addressof(self.dst_addr), addressof(dst_addr_), sizeof(dst_addr_))
            self.dst_port = dst_port_
            self.data_len = data_len_
            memmove(addressof(self.data), addressof(data_), data_len_)
        def __str__(self):
            return str(self.header) + \
            " - handle = 0x{0:02X}".format(self.handle) + \
            " - conn_id = 0x{0:02X}".format(self.conn_id) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - dst_port = 0x{0:04X}".format(self.dst_port) + \
            " - data_len = 0x{0:04X}".format(self.data_len) + \
            " - data = " + ''.join('{:02X}'.format(x) for x in self.data)            
    return udp_data_req(handle, conn_id, dst_addr, dst_port, data_len, data, sec_mode, counter)

def g3_hi_msg_get_g3_udp_app_req(app_req_id, dst_addr, data_len, data, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class udp_app_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("app_req_id", c_ubyte),
            ("dst_addr", g3_hi_uip_ipaddr),
            ("data_len", c_ushort),
            ("data", c_ubyte * data_len)]
        def __init__(self, app_req_id_, dst_addr_, data_len_, data_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3UDP_APP_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.app_req_id = app_req_id_
            memmove(addressof(self.dst_addr), addressof(dst_addr_), sizeof(dst_addr_))
            self.data_len = data_len_
            memmove(addressof(self.data), addressof(data_), data_len_)
        def __len__(self):
            return sizeof(self)
        def __str__(self):
            return str(self.header) + \
            " - app_req_id = 0x{0:02X}".format(self.app_req_id) + \
            " - conn_id = 0x{0:02X}".format(self.conn_id) + \
            " - dst_addr = " + str(self.dst_addr) + \
            " - data_len = 0x{0:04X}".format(self.data_len) + \
            " - data = " + ''.join('{:02X}'.format(x) for x in self.data)                 
    return udp_app_req(app_req_id, dst_addr, data_len, data, sec_mode, counter)    

def g3_hi_msg_get_g3_app_rhi_req(data, delay = 0):
    data_len = sizeof(data)
    class app_rhi_req(Structure):
        _pack_ = 1
        _fields_ = [("delay", c_uint),
            ("data", c_ubyte * data_len)]
        def __init__(self, delay_, data_len_, data_):
            self.delay = delay_
            memmove(addressof(self.data), addressof(data_), data_len_)
        def __len__(self):
            return sizeof(self)
    return app_rhi_req(delay, data_len, data)       

def g3_hi_msg_get_g3_hi_boot_dev_pan_sort_req(pancount, pandescr_list, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class boot_dev_pan_sort_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("pan_cnt", c_ubyte),
            ("pan_descr", g3_hi_pan_descriptor * pancount)]
        def __init__(self, pan_cnt, pandescr_list_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3BOOT_DEV_PANSORT_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.pan_cnt = pan_cnt
            for i in range(0, pan_cnt):
                self.pan_descr[0] = pandescr_list_[0]
    return boot_dev_pan_sort_req(pancount, pandescr_list, sec_mode, counter)

def g3_hi_msg_get_sflash_req(operation, address, size, payload, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class sflash_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("operation", c_ubyte),
            ("size", c_ushort),
            ("address", c_uint),
            ("payload", c_ubyte * size)]
        def __init__(self, operation_, address_, size_, payload_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.SFLASH_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.operation = operation_
            self.size = size_
            self.address = address_
            if ( (g3_hi_sflash_operation.WRITE == operation) or (g3_hi_sflash_operation.WRITE_AND_EVAL_CRC == operation) ) and (size > 0):
                memmove(addressof(self.payload), byref(payload_), size_)
        def __str__(self):
            return str(self.header) + \
                " - operation = " + str(g3_hi_sflash_operation(self.operation)) + \
                " - size = 0x" + '{0:04X}'.format(self.size) + \
                " - address = 0x" + '{0:08X}'.format(self.address) + \
                " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)
    return sflash_req(operation, address, size, payload, sec_mode, counter)

def g3_hi_msg_nvm_req(operation, section, offset, size, data = None, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    data_len = 0
    if data != None:
        data_len = size
    class nvm_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("operation", c_ubyte),
            ("section", c_ubyte),
            ("offset", c_ushort),
            ("size", c_ushort),
            ("data", c_ubyte * data_len)]
        def __init__(self, operation_, section_, offset_, size_, data_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.NVM_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.operation = operation_
            self.section = section_
            self.offset = offset_
            self.size = size_
            if data_ != None:
                g3_hi_memmove_checked(self.data, data_, size_)
                #memmove(addressof(self.data), addressof(g3_hi_vec_to_cbyte_vec(data_)), size_)
        def __str__(self):
            return str(self.header) + \
                " - operation = " + str(g3_hi_nvm_operation(self.operation)) + \
                " - section = " + str(g3_hi_nvm_section(self.section)) + \
                " - offset = 0x" + '{0:04X}'.format(self.offset) + \
                " - size = 0x" + '{0:04X}'.format(self.size) + \
                " - data = 0x" + ''.join('{:02X}'.format(x) for x in self.data)            
    return nvm_req(operation, section, offset, size, data, sec_mode, counter)

def g3_hi_msg_g3phy_data_req(tx_power, mod_type, mod_scheme, preemphasis, tonemap,
    dt, tworsblock, len, payload, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    payload_len = len
    class g3phy_data_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("tx_power", c_ubyte),
            ("mod_type", c_ubyte),
            ("mod_scheme", c_ubyte),
            ("preemphasis", c_ubyte * 24),
            ("tonemap", c_ubyte * 3),
            ("dt", c_ubyte),
            ("tworsblock", c_ubyte),
            ("len", c_ushort),
            ("payload", c_ubyte * payload_len)]
        def __init__(self, tx_power_, mod_type_, mod_scheme_, preemphasis_, tonemap_,
            dt_, tworsblock_, len_, payload_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3PHY_DATA_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.tx_power = tx_power_
            self.mod_type = mod_type_
            self.mod_scheme = mod_scheme_
            memmove(byref(self.preemphasis), byref(preemphasis_), sizeof(self.preemphasis))
            memmove(byref(self.tonemap), byref(tonemap_), sizeof(self.tonemap))
            self.dt = dt_
            self.tworsblock = tworsblock_
            self.len = len_
            memmove(byref(self.payload), byref(payload_), len_)
        def __str__(self):
            return str(self.header) + \
                " - tx_power = 0x" + '{0:02X}'.format(self.tx_power) + \
                " - mod_type = 0x" + '{0:02X}'.format(self.mod_type) + \
                " - mod_scheme = 0x" + '{0:02X}'.format(self.mod_scheme) + \
                " - preemphasis = 0x" + ''.join('{:02X}'.format(x) for x in self.preemphasis) + \
                " - tonemap = 0x" + ''.join('{:02X}'.format(x) for x in self.tonemap) + \
                " - dt = 0x" + '{0:02X}'.format(self.dt) + \
                " - tworsblock = 0x" + '{0:02X}'.format(self.tworsblock) + \
                " - len = 0x" + '{0:05X}'.format(self.len) + \
                " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)   
    return g3phy_data_req(tx_power, mod_type, mod_scheme, preemphasis, tonemap, dt, tworsblock, payload_len, payload, sec_mode, counter)

def g3_hi_msg_g3phy_rf_req(len, payload, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    payload_len = len
    class g3phy_rf_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("len", c_ushort),
            ("payload", c_ubyte * payload_len)]
        def __init__(self, len_, payload_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3PHY_RF_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.len = len_
            memmove(byref(self.payload), byref(payload_), len_)
        def __str__(self):
            return str(self.header) + \
                " - len = 0x" + '{0:05X}'.format(self.len) + \
                " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)   
    return g3phy_rf_req(payload_len, payload, sec_mode, counter)

def g3_hi_msg_mac_data_req(src_addr_mode, dst_addr_mode, dst_pan_id, dst_addr, msdu_len, msdu,
    msdu_handle, tx_options, security_level, key_index, qos, media_type=g3_hi_mac_media_type.MEDIATYPE_PLC, 
    sec_mode=g3_hi_sec_mode.CLEAR_TEXT, counter=0):
    dst_addr_len = 0
    if 2 == dst_addr_mode.value:
        dst_addr_len = 2
    elif 3 == dst_addr_mode.value:
        dst_addr_len = 8
    if security_level == 0:
        key_idx_len = 0
    elif security_level == 5:
        key_idx_len = 1
    class mac_data_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("src_addr_mode", c_ubyte),
            ("dst_addr_mode", c_ubyte),
            ("dst_pan_id", c_ushort),
            ("dst_addr", c_ubyte * dst_addr_len),
            ("msdu_len", c_ushort),
            ("msdu", c_ubyte * msdu_len),
            ("msdu_handle", c_ubyte),
            ("tx_options", c_ubyte),
            ("security_level", c_ubyte),
            ("key_index", c_ubyte * key_idx_len),
            ("qos", c_ubyte),
            ("media_type", c_ubyte)]
        def __init__(self, src_addr_mode_, dst_addr_mode_, dst_pan_id_, dst_addr_, msdu_len_, msdu_,
            msdu_handle_, tx_options_, security_level_, key_index_, qos_, media_type_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3MAC_DATA_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.src_addr_mode = src_addr_mode_
            self.dst_addr_mode = dst_addr_mode_
            self.dst_pan_id = dst_pan_id_
            if (dst_addr_len == 2):
                dst_addr__ = c_ushort(dst_addr_)
                memmove(byref(self.dst_addr), byref(dst_addr__), dst_addr_len)
            if (dst_addr_len == 8):
                memmove(byref(self.dst_addr), byref(dst_addr_), dst_addr_len)
            self.msdu_len = msdu_len_
            if (msdu_len > 0):
                memmove(byref(self.msdu), byref(msdu_), msdu_len)
            self.msdu_handle = msdu_handle_
            self.tx_options = tx_options_
            self.security_level = security_level_
            if self.security_level == 5:
                self.key_index[0] = c_ubyte(key_index_)
            self.qos = qos_
            self.media_type = media_type_
        def __str__(self):
            return  str(self.header)
    return mac_data_req(src_addr_mode, dst_addr_mode, dst_pan_id, dst_addr, msdu_len, msdu, msdu_handle, tx_options, security_level, key_index, qos, media_type, sec_mode, counter)

def g3_hi_msg_adp_data_req(nsdu_len, nsdu_handle, discovery_route, qos, nsdu, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class adp_data_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("nsdu_len", c_ushort),
            ("nsdu_handle", c_ubyte),
            ("discovery_route", c_ubyte),
            ("qos", c_ubyte),
            ("nsdu", c_ubyte * nsdu_len)]
        def __init__(self, nsdu_len_, nsdu_handle_, discovery_route_, qos_, nsdu_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_DATA_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.nsdu_len = nsdu_len_
            self.nsdu_handle = nsdu_handle_
            self.discovery_route = discovery_route_
            self.qos = qos_
            memmove(byref(self.nsdu), byref(nsdu_), nsdu_len_)
        def __str__(self):
            return  str(self.header) + " - nsdu_len = 0x{0:02X}".format(self.nsdu_len) + \
                    " - nsdu_handle = 0x{0:02X}".format(self.nsdu_handle) + \
                    " - qos = " + str(g3_hi_mac_adp_qos(self.qos)) + \
                    " - nsdu_len = 0x{0:02X}".format(self.nsdu_len) + \
                    " - nsdu = 0x" + ''.join('{:02X}'.format(x) for x in self.nsdu)

    return adp_data_req(nsdu_len, nsdu_handle, discovery_route, qos, nsdu, sec_mode, counter)

def g3_hi_msg_adp_route_over_data_req(nsdu_len, nsdu_handle, next_hop_addr_mode, next_hop_pan_id, next_hop_addr, ack_req, qos, \
    media_type, nsdu, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    next_hop_addr_len = 0
    if 2 == next_hop_addr_mode.value:
        next_hop_addr_len = 2
    elif 3 == next_hop_addr_mode.value:
        next_hop_addr_len = 8
    class adp_route_over_data_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("nsdu_len", c_ushort),
            ("nsdu_handle", c_ubyte),
            ("next_hop_addr_mode", c_ubyte),
            ("next_hop_pan_id", c_ushort),
            ("next_hop_addr", c_ubyte * next_hop_addr_len),
            ("ack_req", c_ubyte),
            ("qos", c_ubyte),
            ("media_type", c_ubyte),
            ("nsdu", c_ubyte * nsdu_len)]
        def __init__(self, nsdu_len_, nsdu_handle_, next_hop_addr_mode_, next_hop_pan_id_, next_hop_addr_, ack_req_, qos_, media_type_, nsdu_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_ROUTEOVER_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.nsdu_len = nsdu_len_
            self.nsdu_handle = nsdu_handle_
            self.next_hop_addr_mode = next_hop_addr_mode_
            self.next_hop_pan_id = next_hop_pan_id_
            if (next_hop_addr_len == 2):
                memmove(byref(self.next_hop_addr), byref(c_ushort(next_hop_addr_)), next_hop_addr_len)
            if (next_hop_addr_len == 8):
                memmove(byref(self.next_hop_addr), byref(next_hop_addr_), next_hop_addr_len)
            self.ack_req = ack_req_
            self.qos = qos_
            self.media_type = media_type_
            memmove(byref(self.nsdu), byref(nsdu_), nsdu_len_)
        def __str__(self):
            if self.next_hop_addr_mode == g3_hi_mac_addr_type.MODE_64:
                s = " - next_hop_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.next_hop_addr[::-1]) 
            elif self.next_hop_addr_mode == g3_hi_mac_addr_type.MODE_16:
                s = " - next_hop_addr = 0x{:04X}".format((self.next_hop_addr[1] + self.next_hop_addr[0]<<8))
            else:
                s = ''
            return  str(self.header) + " - nsdu_len = 0x{0:02X}".format(self.nsdu_len) + \
                    " - nsdu_handle = 0x{0:02X}".format(self.nsdu_handle) + \
                    " - next_hop_addr_mode = {}".format(g3_hi_mac_addr_type(self.next_hop_addr_mode)) + \
                    s + \
                    " - next_hop_pan_id = 0x{0:04X}".format(self.next_hop_pan_id) + \
                    " - qos = " + str(g3_hi_mac_adp_qos(self.qos)) + \
                    " - media_type = " + str(g3_hi_mac_media_type(self.media_type)) + \
                    " - nsdu_len = 0x{0:02X}".format(self.nsdu_len) + \
                    " - nsdu = 0x" + ''.join('{:02X}'.format(x) for x in self.nsdu)
    return adp_route_over_data_req(nsdu_len, nsdu_handle, next_hop_addr_mode, next_hop_pan_id, next_hop_addr, ack_req, qos, media_type, nsdu, sec_mode, counter)

def g3_hi_msg_adp_lbp_req(dst_addr_mode, dst_pan_id, dst_addr, nsdu_len, nsdu, nsdu_handle, max_hops, discovery_route, qos, security_enabled,
    media_type = g3_hi_mac_media_type.MEDIATYPE_PLC, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    dst_addr_len = 0
    if 2 == dst_addr_mode:
        dst_addr_len = 2
    elif 3 == dst_addr_mode:
        dst_addr_len = 8
    class adp_lbp_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("dst_addr_mode", c_ubyte),
            ("dst_pan_id", c_ushort),
            ("dst_addr", c_ubyte * dst_addr_len),
            ("nsdu_len", c_ushort),
            ("nsdu", c_ubyte * nsdu_len),
            ("nsdu_handle", c_ubyte),
            ("max_hops", c_ubyte),
            ("discovery_route", c_ubyte),
            ("qos", c_ubyte),
            ("security_enabled", c_ubyte),
            ("media_type", c_ubyte)]
        def __init__(self, dst_addr_mode_, dst_pan_id_, dst_addr_, nsdu_len_, nsdu_, nsdu_handle_, max_hops_, discovery_route_,
            qos_, security_enabled_, media_type_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.G3ADP_LBP_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.dst_addr_mode = dst_addr_mode_
            self.dst_pan_id = dst_pan_id_
            memmove(addressof(self.dst_addr), byref(dst_addr_), dst_addr_len)
            self.nsdu_len = nsdu_len_
            memmove(addressof(self.nsdu), byref(nsdu_), nsdu_len_)
            self.nsdu_handle = nsdu_handle_
            self.max_hops = max_hops_
            self.discovery_route = discovery_route_
            self.qos = qos_
            self.security_enabled = security_enabled_
            self.media_type = media_type_
        def __str__(self):  
            if self.dst_addr_mode == g3_hi_mac_addr_type.MODE_64:
                s = " - dst_addr = 0x" + ''.join('{:02X}'.format(x) for x in self.dst_addr[::-1]) 
            elif self.dst_addr_mode == g3_hi_mac_addr_type.MODE_16:
                s = " - dst_addr = 0x{:04X}".format((self.dst_addr[0] + self.dst_addr[1]<<8))
            else:
                s = ''
            return str(self.header) + \
                " - dst_addr_mode = " + str(g3_hi_mac_addr_type(self.dst_addr_mode)) +\
                " - dst_pan_id = 0x{:04X}".format(self.dst_pan_id) +\
                s +\
                " - nsdu_len = 0x{0:04X}".format(self.nsdu_len) + \
                " - nsdu = " + ''.join('{:02X}'.format(x) for x in self.nsdu) +\
                " - nsdu_handle = 0x{0:02X}".format(self.nsdu_handle) + \
                " - max_hops = 0x{0:02X}".format(self.max_hops) + \
                " - discovery_route = 0x{0:02X}".format(self.discovery_route) + \
                " - qos = {}".format(g3_hi_mac_adp_qos(self.qos))  + \
                " - security_enabled = 0x{0:02X}".format(self.security_enabled) +\
                " - media_type = " + str(g3_hi_mac_media_type(self.media_type))
    return adp_lbp_req(dst_addr_mode, dst_pan_id, dst_addr, nsdu_len, g3_hi_vec_to_cbyte_vec(nsdu), nsdu_handle, max_hops, discovery_route,
        qos, security_enabled, media_type, sec_mode, counter)

def g3_hi_msg_testset_req(test_type, mode=None, rx_tx_conf=None, nb_pkts=None, delay=None, req_msg=None, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    req_msg_len = 0
    testpar_len = 0
    if g3_hi_testtype.DATA_RATE == test_type:
        if g3_hi_rxtxconf.TX == rx_tx_conf:
            req_msg_len = sizeof(req_msg)
        class dr_test_param(Structure):
            _pack_ = 1
            _fields_ = [("mode", c_ubyte),
                ("rx_tx_conf", c_ubyte),
                ("nb_pkts", c_uint),
                ("delay", c_uint),
                ("req", c_ubyte * req_msg_len)]
            def __init__(self, mode_, rx_tx_conf_, nb_pkts_, delay_, req_msg_):
                self.mode = mode_
                self.rx_tx_conf = rx_tx_conf_
                self.nb_pkts = nb_pkts_
                self.delay = delay_
                if g3_hi_rxtxconf.TX == rx_tx_conf_:
                    memmove(byref(self.req), byref(req_msg_), sizeof(req_msg_))
            def __str__(self):
                return "" #TODO
        testpar_len = sizeof(dr_test_param)
        class dr_testset_req(Structure):
            _pack_ = 1
            _fields_ = [("header", g3_hi_gen_req),
                ("test_type", c_ubyte),
                ("testpar", c_ubyte * testpar_len)]
            def __init__(self, test_type_, mode__, rx_tx_conf__, nb_pkts__, delay__, req_msg__, sec_mode_, counter_):
                g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.TESTSET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
                self.test_type = test_type_
                if (testpar_len > 0):
                    t = dr_test_param(mode__, rx_tx_conf__, nb_pkts__, delay__, req_msg__)
                    memmove(addressof(self.testpar), addressof(t), testpar_len)
            def __str__(self):
                return  str(self.header) +\
                        " - test_type = " + str(g3_hi_testtype(self.test_type)) +\
                        " - param = " + str(self.testpar)
        return dr_testset_req(test_type, mode, rx_tx_conf, nb_pkts, delay, req_msg, sec_mode, counter)
    elif g3_hi_testtype.RFTXDELAY == test_type:
        payload_len = sizeof(req_msg)
        class rftxdelay_testset_req(Structure):
            _pack_ = 1
            _fields_ = [("header", g3_hi_gen_req),
                ("test_type", c_ubyte),
                ("delay", c_ushort),
                ("len", c_ushort),
                ("payload", c_ubyte * payload_len)]
            def __init__(self, test_type_, delay_, payload_, sec_mode_, counter_):
                g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.TESTSET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
                self.test_type = test_type_
                self.delay = delay_
                self.len = len(payload_)
                if (self.len > 0):
                    payload__ = g3_hi_vec_to_cbyte_vec(payload_)
                    memmove(addressof(self.payload), addressof(payload__), payload_len)
            def __str__(self):
                return  str(self.header) +\
                        " - test_type = " + str(g3_hi_testtype(self.test_type)) +\
                        " - delay = 0x{0:04X}".format(self.delay) +\
                        " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)
        return rftxdelay_testset_req(test_type, delay, req_msg, sec_mode, counter)
    else: #g3_hi_testtype.INFINITE_LOOP == test_type or g3_hi_testtype.NONE == test_type
        class no_pay_testset_req(Structure):
            _pack_ = 1
            _fields_ = [("header", g3_hi_gen_req),
                ("test_type", c_ubyte)]
            def __init__(self, test_type_, sec_mode_, counter_):
                g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.TESTSET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
                self.test_type = test_type_
            def __str__(self):
                return  str(self.header) +\
                        " - test_type = " + str(g3_hi_testtype(self.test_type))
        return no_pay_testset_req(test_type, sec_mode, counter)

def g3_hi_msg_otp_req(operation, region, payload_len, payload, sec_mode = g3_hi_sec_mode.CLEAR_TEXT, counter = 0):
    class otp_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("operation", c_ubyte),
            ("region", c_ubyte),
            ("payload", c_ubyte * payload_len)]
        def __init__(self, operation_, region_, payload_len_, payload_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.OTP_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
            self.operation = operation_
            self.region = region_
            if (payload_len > 0):
                payload__ = g3_hi_vec_to_cbyte_vec(payload_)
                memmove(addressof(self.payload), addressof(payload__), payload_len)
        def __str__(self):
            return  str(self.header) +\
                    " - operation = " + str(g3_hi_otp_operation(self.operation)) +\
                    " - region = " + str(g3_hi_otp_region(self.region)) +\
                    " - payload = 0x" + ''.join('{:02X}'.format(x) for x in self.payload)
    return otp_req(operation, region, payload_len, payload, sec_mode, counter)

def g3_hi_msg_hwconfig_gpioconfig_req(port, pin, mode):
    class hwconfig_gpioconfig_req(Structure):
        _pack_ = 1
        _fields_ = [("port", c_ubyte),
            ("pin", c_ubyte),
            ("mode", c_ubyte)]
        def __init__(self, port, pin, mode):
            self.port = port
            self.pin = pin
            self.mode = mode
    return hwconfig_gpioconfig_req(port, pin, mode)

def g3_hi_msg_hwconfig_gpiowrite_req(port, pin, value):
    class hwconfig_gpiowrite_req(Structure):
        _pack_ = 1
        _fields_ = [("port", c_ubyte),
            ("pin", c_ubyte),
            ("value", c_ubyte)]
        def __init__(self, port, pin, value):
            self.port = port
            self.pin = pin
            self.value = value 
    return hwconfig_gpiowrite_req(port, pin, value)

def g3_hi_msg_hwconfig_gpioread_req(port, pin):
    class hwconfig_gpioread_req(Structure):
        _pack_ = 1
        _fields_ = [("port", c_ubyte),
            ("pin", c_ubyte)]
        def __init__(self, port, pin):
            self.port = port
            self.pin = pin 
    return hwconfig_gpioread_req(port, pin)

def g3_hi_msg_hwconfig_baudrateconfig_req(baud_rate):
    class hwconfig_baudrateconfig_req(Structure):
        _pack_ = 1
        _fields_ = [("baud_rate", c_uint)]
        def __init__(self, baud_rate):
            self.baud_rate = baud_rate
    return hwconfig_baudrateconfig_req(baud_rate)

def g3_hi_msg_hwconfig_s2lpregwrite_req(addr, datalen, data):
    class hwconfig_s2lpregwrite_req(Structure):
        _pack_ = 1
        _fields_ = [("addr", c_ubyte),
            ("len", c_ubyte),
            ("data", c_ubyte * datalen)]
        def __init__(self, addr_, len_, data_):
            self.addr = addr_
            self.len = len_
            memmove(addressof(self.data), addressof(data_), len_)
    return hwconfig_s2lpregwrite_req(addr, datalen, data)

def g3_hi_msg_hwconfig_s2lpregread_req(addr, datalen):
    class hwconfig_s2lpregread_req(Structure):
        _pack_ = 1
        _fields_ = [("addr", c_ubyte),
            ("len", c_ubyte)]
        def __init__(self, addr_, len_):
            self.addr = addr_
            self.len = len_
    return hwconfig_s2lpregread_req(addr, datalen)

def g3_hi_msg_hwconfig_req(config_type, config_payload, sec_mode=g3_hi_sec_mode.CLEAR_TEXT, counter=0):
    payload_len = sizeof(config_payload)  
    class hwconfig_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("config_type", c_ubyte),
            ("config_paylen", c_ubyte),
            ("config_payload", c_ubyte * payload_len)]
        pay = None
        def __init__(self, config_type_, config_payload_, payload_len_, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.HWCONFIG_REQ, sizeof(c_ubyte) + sizeof(c_ubyte) + payload_len_, sec_mode_, counter_)
            self.config_type = config_type_
            self.config_paylen = payload_len_
            memmove(addressof(self.config_payload), addressof(config_payload_), payload_len_)
            if config_type == g3_hi_hwconfig_type.BAUDRATE_CONFIG:
                self.pay = config_payload_
        def __str__(self):
            s = str(self.header) + \
                " - config_type = " + str(g3_hi_hwconfig_type(self.config_type)) +\
                " - config_paylen = 0x{:02X}".format(self.config_paylen)                    
            if self.config_paylen > 0:
                s += " - config_payload = 0x" + ''.join('{:02X}'.format(x) for x in self.config_payload)
                if self.pay != None:
                    s += " - " + str(self.pay)
            return s    
    return hwconfig_req(config_type, config_payload, payload_len, sec_mode, counter)

def g3_hi_msg_rfconfigset_req(radio_basefreq, radio_modselect, radio_datarate, radio_freqdev, radio_bandwidth, radio_csblanking, radio_xtalfreq, radio_rssigain, powerdbm, 
            pkt_crcmode, pkt_enfec, pkt_enwhitening, pkt_eninterleaving, irqgpiopin, mcu_clkenable, mcu_clkgpiopin, mcu_clkxoratio, mcu_clkrcoratio, mcu_clkclockcycles, 
            extsmpsenable, fem_enabled, fem_gpiopincsd, fem_gpiopincps, fem_gpiopinctx, fem_txbypassen, sec_mode=g3_hi_sec_mode.CLEAR_TEXT, counter=0):
    class rfconfigset_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req),
            ("radio_basefreq", c_uint),
            ("radio_modselect", c_ubyte),
            ("radio_datarate", c_uint),
            ("radio_freqdev", c_uint),
            ("radio_bandwidth", c_uint),
            ("radio_csblanking", c_ubyte),
            ("radio_xtalfreq", c_uint),
            ("radio_rssigain", c_ubyte),
            ("powerdbm", c_ubyte),
            ("pkt_crcmode", c_ubyte),
            ("pkt_enfec", c_ubyte),
            ("pkt_enwhitening", c_ubyte),
            ("pkt_eninterleaving", c_ubyte),
            ("irqgpiopin", c_ubyte),
            ("mcu_clkenable", c_ubyte),
            ("mcu_clkgpiopin", c_ubyte),
            ("mcu_clkxoratio", c_ubyte),
            ("mcu_clkrcoratio", c_ubyte),
            ("mcu_clkclockcycles", c_ubyte),
            ("extsmpsenable", c_ubyte),
            ("fem_enabled", c_ubyte),
            ("fem_gpiopincsd", c_ubyte),
            ("fem_gpiopincps", c_ubyte),
            ("fem_gpiopinctx", c_ubyte),
            ("fem_txbypassen", c_ubyte)]
        def __init__(self, radio_basefreq_, radio_modselect_, radio_datarate_, radio_freqdev_, radio_bandwidth_, 
            radio_csblanking_, radio_xtalfreq_, radio_rssigain_, powerdbm_, pkt_crcmode_, pkt_enfec_, pkt_enwhitening_, 
            pkt_eninterleaving_, irqgpiopin_, mcu_clkenable_, mcu_clkgpiopin_, mcu_clkxoratio_, mcu_clkrcoratio_, 
            mcu_clkclockcycles_, extsmpsenable_, fem_enabled_, fem_gpiopincsd_, fem_gpiopincps_, fem_gpiopinctx_, fem_txbypassen_, sec_mode_, counter_):
                g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.RFCONFIGSET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
                self.radio_basefreq = radio_basefreq_
                self.radio_modselect = radio_modselect_
                self.radio_datarate = radio_datarate_
                self.radio_freqdev = radio_freqdev_
                self.radio_bandwidth = radio_bandwidth_
                self.radio_csblanking = radio_csblanking_
                self.radio_xtalfreq = radio_xtalfreq_
                self.radio_rssigain = radio_rssigain_
                self.powerdbm = powerdbm_
                self.pkt_crcmode = pkt_crcmode_
                self.pkt_enfec = pkt_enfec_
                self.pkt_enwhitening = pkt_enwhitening_
                self.pkt_eninterleaving = pkt_eninterleaving_
                self.irqgpiopin = irqgpiopin
                self.mcu_clkenable = mcu_clkenable
                self.mcu_clkgpiopin = mcu_clkgpiopin
                self.mcu_clkxoratio = mcu_clkxoratio
                self.mcu_clkrcoratio = mcu_clkrcoratio
                self.mcu_clkclockcycles = mcu_clkclockcycles
                self.extsmpsenable = extsmpsenable
                self.fem_enabled = fem_enabled
                self.fem_gpiopincsd = fem_gpiopincsd
                self.fem_gpiopincps = fem_gpiopincps
                self.fem_gpiopinctx = fem_gpiopinctx
                self.fem_txbypassen = fem_txbypassen
        def __str__(self):
            return str(self.header) + \
                " - radio_basefreq = " + str(radio_basefreq) + \
                " - radio_modselect = 0x{:02X}".format(self.radio_modselect) + \
                " - radio_datarate = " + str(radio_datarate) + \
                " - radio_freqdev = " + str(radio_freqdev) + \
                " - radio_bandwidth = " + str(radio_bandwidth) + \
                " - radio_csblanking = 0x{:02X}".format(self.radio_csblanking) + \
                " - radio_xtalfreq = " + str(radio_xtalfreq) + \
                " - radio_rssigain = 0x{:02X}".format(self.radio_rssigain) + \
                " - powerdbm = 0x{:02X}".format(self.powerdbm) + \
                " - pkt_crcmode = 0x{:02X}".format(self.pkt_crcmode) + \
                " - pkt_enfec = 0x{:02X}".format(self.pkt_enfec) + \
                " - pkt_enwhitening = 0x{:02X}".format(self.pkt_enwhitening) + \
                " - pkt_eninterleaving = 0x{:02X}".format(self.pkt_eninterleaving) + \
                " - irqgpiopin = 0x{:02X}".format(self.irqgpiopin) + \
                " - mcu_clkenable = 0x{:02X}".format(self.mcu_clkenable) + \
                " - mcu_clkgpiopin = 0x{:02X}".format(self.mcu_clkgpiopin) + \
                " - mcu_clkxoratio = 0x{:02X}".format(self.mcu_clkxoratio) + \
                " - mcu_clkrcoratio = 0x{:02X}".format(self.mcu_clkrcoratio) + \
                " - mcu_clkclockcycles = 0x{:02X}".format(self.mcu_clkclockcycles) + \
                " - extsmpsenable = 0x{:02X}".format(self.extsmpsenable) + \
                " - fem_enabled = 0x{:02X}".format(self.fem_enabled) + \
                " - fem_gpiopincsd = 0x{:02X}".format(self.fem_gpiopincsd) + \
                " - fem_gpiopincps = 0x{:02X}".format(self.fem_gpiopincps) + \
                " - fem_gpiopinctx = 0x{:02X}".format(self.fem_gpiopinctx) + \
                " - fem_txbypassen = 0x{:02X}".format(self.fem_txbypassen)
    return rfconfigset_req(radio_basefreq, radio_modselect, radio_datarate, radio_freqdev, radio_bandwidth, radio_csblanking, radio_xtalfreq,
        radio_rssigain, powerdbm, pkt_crcmode, pkt_enfec, pkt_enwhitening, pkt_eninterleaving, irqgpiopin, mcu_clkenable, mcu_clkgpiopin, mcu_clkxoratio,
        mcu_clkrcoratio, mcu_clkclockcycles, extsmpsenable, fem_enabled, fem_gpiopincsd, fem_gpiopincps, fem_gpiopinctx, fem_txbypassen, sec_mode, counter)

class g3_hi_msg_rfconfigset_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("status", c_ubyte)]
    def __str__(self):
        return  str(self.header) + \
            " - status = " + str(g3_hi_results(self.status))

def g3_hi_msg_rfconfigget_req(sec_mode=g3_hi_sec_mode.CLEAR_TEXT, counter=0):
    class rfconfigget_req(Structure):
        _pack_ = 1
        _fields_ = [("header", g3_hi_gen_req)]
        def __init__(self, sec_mode_, counter_):
            g3_hi_msg_header_set(self.header, g3_hi_cmd_ids.RFCONFIGGET_REQ, sizeof(self) - sizeof(self.header), sec_mode_, counter_)
        def __str__(self):
            return str(self.header)
    return rfconfigget_req(sec_mode, counter)

class g3_hi_msg_rfconfigget_conf(g3_hi_msg_conf_base):
    _pack_ = 1
    _fields_ = [("header", g3_hi_gen_conf),
        ("radio_basefreq", c_uint),
        ("radio_modselect", c_ubyte),
        ("radio_datarate", c_uint),
        ("radio_freqdev", c_uint),
        ("radio_bandwidth", c_uint),
        ("radio_csblanking", c_ubyte),
        ("radio_xtalfreq", c_uint),
        ("radio_rssigain", c_ubyte),
        ("powerdbm", c_ubyte),
        ("pkt_crcmode", c_ubyte),
        ("pkt_enfec", c_ubyte),
        ("pkt_enwhitening", c_ubyte),
        ("pkt_eninterleaving", c_ubyte),
        ("irqgpiopin", c_ubyte),
        ("mcu_clkenable", c_ubyte),
        ("mcu_clkgpiopin", c_ubyte),
        ("mcu_clkxoratio", c_ubyte),
        ("mcu_clkrcoratio", c_ubyte),
        ("mcu_clkclockcycles", c_ubyte),
        ("extsmpsenable", c_ubyte),
        ("fem_enabled", c_ubyte),
        ("fem_gpiopincsd", c_ubyte),
        ("fem_gpiopincps", c_ubyte),
        ("fem_gpiopinctx", c_ubyte),
        ("fem_txbypassen", c_ubyte)]
    def __str__(self):
        return  str(self.header) + \
                " - radio_basefreq = " + str(self.radio_basefreq) + \
                " - radio_modselect = 0x{:02X}".format(self.radio_modselect) + \
                " - radio_datarate = " + str(self.radio_datarate) + \
                " - radio_freqdev = " + str(self.radio_freqdev) + \
                " - radio_bandwidth = " + str(self.radio_bandwidth) + \
                " - radio_csblanking = 0x{:02X}".format(self.radio_csblanking) + \
                " - radio_xtalfreq = " + str(self.radio_xtalfreq) + \
                " - radio_rssigain = 0x{:02X}".format(self.radio_rssigain) + \
                " - powerdbm = 0x{:02X}".format(self.powerdbm) + \
                " - pkt_crcmode = 0x{:02X}".format(self.pkt_crcmode) + \
                " - pkt_enfec = 0x{:02X}".format(self.pkt_enfec) + \
                " - pkt_enwhitening = 0x{:02X}".format(self.pkt_enwhitening) + \
                " - pkt_eninterleaving = 0x{:02X}".format(self.pkt_eninterleaving) + \
                " - irqgpiopin = 0x{:02X}".format(self.irqgpiopin) + \
                " - mcu_clkenable = 0x{:02X}".format(self.mcu_clkenable) + \
                " - mcu_clkgpiopin = 0x{:02X}".format(self.mcu_clkgpiopin) + \
                " - mcu_clkxoratio = 0x{:02X}".format(self.mcu_clkxoratio) + \
                " - mcu_clkrcoratio = 0x{:02X}".format(self.mcu_clkrcoratio) + \
                " - mcu_clkclockcycles = 0x{:02X}".format(self.mcu_clkclockcycles) + \
                " - extsmpsenable = 0x{:02X}".format(self.extsmpsenable) + \
                " - fem_enabled = 0x{:02X}".format(self.fem_enabled) + \
                " - fem_gpiopincsd = 0x{:02X}".format(self.fem_gpiopincsd) + \
                " - fem_gpiopincps = 0x{:02X}".format(self.fem_gpiopincps) + \
                " - fem_gpiopinctx = 0x{:02X}".format(self.fem_gpiopinctx) + \
                " - fem_txbypassen = 0x{:02X}".format(self.fem_txbypassen)

# Functions to Create Request Messages
# End

def g3_hi_msg_get(m, net_info_ex):
    cmd_vec = cast(byref(m), POINTER(c_char))
    cmd_id = ord(cmd_vec[2])
    if g3_hi_cmd_ids.HWRESET_CONF == cmd_id:
        ret = g3_hi_msg_hwreset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.MODESET_CONF == cmd_id:
        ret = g3_hi_msg_mode_set_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.MODEGET_CONF == cmd_id:
        ret = g3_hi_msg_mode_get_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.BAUDRATESET_CONF == cmd_id:
        ret = g3_hi_msg_baudrate_set_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        if 0x00 == ret.result:
            net_info_ex.ser.baudrate = ret.baud_rate
        return ret
    elif g3_hi_cmd_ids.RESETSTATE_CONF == cmd_id:
        ret = g3_hi_msg_resetstate_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.TESTSET_CONF == cmd_id:
        ret = g3_hi_msg_testset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.TESTGET_CONF == cmd_id:
        ret = g3_hi_msg_testget_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.TRACE_CONF == cmd_id:
        ret = g3_hi_msg_trace_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.SFLASH_CONF == cmd_id:
        return g3_hi_msg_get_sflash_conf(m, len(m))
    elif g3_hi_cmd_ids.NVM_CONF == cmd_id:
        return  g3_hi_msg_nvm_conf(m, len(m))
    elif g3_hi_cmd_ids.DBGTOOL_CONF == cmd_id:
        return g3_hi_msg_dbgtool_conf(m, len(m))
    elif g3_hi_cmd_ids.HWCONFIG_CONF == cmd_id:
        ret = g3_hi_msg_hwconfig_conf(m, len(m))
        if ret.config_type == g3_hi_hwconfig_type.BAUDRATE_CONFIG and ret.status == 0x00:
            net_info_ex.ser.baudrate = ret.config_payload.baud_rate
        return ret    
    elif g3_hi_cmd_ids.OTP_CONF == cmd_id:
        return g3_hi_msg_otp_conf(m, len(m))
    elif g3_hi_cmd_ids.RFCONFIGSET_CONF == cmd_id:
        ret = g3_hi_msg_rfconfigset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.RFCONFIGGET_CONF == cmd_id:
        ret = g3_hi_msg_rfconfigget_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_DATA_CONF == cmd_id:
        ret = g3_hi_msg_phy_data_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_DATA_IND == cmd_id:
        return g3_hi_msg_phy_data_ind(m, sizeof(m))
    elif g3_hi_cmd_ids.G3PHY_ACK_CONF == cmd_id:
        ret = g3_hi_msg_phy_ack_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_ACK_IND == cmd_id:
        ret = g3_hi_msg_phy_ack_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_SETTRXSTATE_CONF == cmd_id:
        ret = g3_hi_msg_phy_set_trx_state_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_CS_CONF == cmd_id:
        ret = g3_hi_msg_phy_cs_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3PHY_RF_CONF == cmd_id:
        ret = g3_hi_msg_phy_rf_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret        
    elif g3_hi_cmd_ids.G3PHY_RF_IND == cmd_id:
        return g3_hi_msg_phy_rf_ind(m, sizeof(m))
    elif g3_hi_cmd_ids.G3MAC_DATA_CONF == cmd_id:
        ret = g3_hi_msg_mac_data_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3MAC_DATA_IND == cmd_id:
        return g3_hi_msg_mac_data_ind(m, len(m))
    elif g3_hi_cmd_ids.G3MAC_BCNNOTIFY_IND == cmd_id:
        ret = g3_hi_msg_mac_bcn_notify_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3MAC_RESET_CONF == cmd_id:
        ret = g3_hi_msg_mac_reset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3MAC_SCAN_CONF == cmd_id:
        ret = g3_hi_msg_mac_scan_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3LIB_GET_CONF == cmd_id:
        return g3_hi_msg_get_g3lib_get_attrib_conf(m, len(m))
    elif g3_hi_cmd_ids.G3LIB_SET_CONF == cmd_id:
        ret = g3_hi_msg_g3lib_set_attrib_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3LIB_SWRESET_CONF == cmd_id:
        ret = g3_hi_msg_swreset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3LIB_EVENT_NOTIF_IND == cmd_id:
        return g3_hi_msg_get_g3lib_event_ind(m, len(m))
    elif g3_hi_cmd_ids.G3LIB_MULTIPLEGET_CONF == cmd_id:
        return g3_hi_msg_g3lib_multipleget_conf(m, len(m))
    elif g3_hi_cmd_ids.G3LIB_SNIFFER_IND == cmd_id:
        return g3_hi_msg_get_g3lib_sniffer_ind(m, len(m))        
    elif g3_hi_cmd_ids.G3ICMP_ECHO_CONF == cmd_id:
        ret = g3_hi_msg_icmp_echo_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_NETSTATUS_IND == cmd_id:
        return g3_hi_msg_get_adp_net_status_ind(m)
    elif g3_hi_cmd_ids.G3ADP_ROUTEDISC_CONF == cmd_id:
        ret = g3_hi_msg_adp_route_disc_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_PATHDISC_CONF == cmd_id:
        return g3_hi_msg_get_adp_path_disc_conf(m, len(m))
    elif g3_hi_cmd_ids.G3BOOT_DEV_PANSORT_IND == cmd_id:
        return g3_hi_msg_get_g3_hi_boot_dev_pan_sort_ind(m, len(m))
    elif g3_hi_cmd_ids.G3BOOT_DEV_PANSORT_CONF == cmd_id:
        ret = g3_hi_msg_boot_dev_pan_sort_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_SRV_GETPSK_IND == cmd_id:
        return g3_hi_msg_get_boot_srv_getpsk_ind(m, len(m))
    elif g3_hi_cmd_ids.G3BOOT_SRV_SETPSK_CONF == cmd_id:
        ret = g3_hi_msg_boot_srv_setpsk_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ICMP_ECHOREP_IND == cmd_id:
        return g3_hi_msg_get_icmp_echo_ind(m, len(m))
    elif g3_hi_cmd_ids.G3ICMP_ECHOREQ_IND == cmd_id:
        return g3_hi_msg_get_icmp_echo_ind(m, len(m))
    elif g3_hi_cmd_ids.G3BOOT_SRV_START_CONF == cmd_id:
        ret = g3_hi_msg_boot_srv_start_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_SRV_STOP_CONF == cmd_id:
        ret = g3_hi_msg_boot_srv_stop_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_SRV_JOIN_IND == cmd_id:
        ret = g3_hi_msg_boot_srv_join_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_SRV_LEAVE_IND == cmd_id:
        ret = g3_hi_msg_boot_srv_leave_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_SRV_KICK_CONF == cmd_id:
        ret = g3_hi_msg_boot_srv_kick_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_DEV_START_CONF == cmd_id:
        ret = g3_hi_msg_boot_dev_start_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_CONF == cmd_id:
        ret = g3_hi_msg_boot_dev_leave_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_IND == cmd_id:
        ret = g3_hi_msg_boot_dev_leave_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3UDP_DATA_CONF == cmd_id:
        ret = g3_hi_msg_udp_data_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3UDP_DATA_IND == cmd_id:
        return g3_hi_msg_get_udp_data_ind(m, len(m))
    elif g3_hi_cmd_ids.G3UDP_CONN_SET_CONF == cmd_id:
        ret = g3_hi_msg_udp_conn_set_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3UDP_APP_CONF == cmd_id:
        ret = g3_hi_msg_udp_app_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3UDP_APP_IND == cmd_id:
        return g3_hi_msg_get_udp_app_ind(m, len(m))        
    elif g3_hi_cmd_ids.G3MAC_COMMSTATUS_IND == cmd_id:
        ret = g3_hi_msg_mac_commstatus_ind(m, len(m))
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3MAC_START_CONF == cmd_id:
        ret = g3_hi_msg_mac_start_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_DATA_CONF == cmd_id:
        ret = g3_hi_msg_adp_data_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_DATA_IND == cmd_id:
        return g3_hi_msg_adp_data_ind(m, len(m))
    elif g3_hi_cmd_ids.G3ADP_DISCOVERY_CONF == cmd_id:
        return g3_hi_msg_adp_discovery_conf(m, len(m))
    elif g3_hi_cmd_ids.G3ADP_NETSTART_CONF == cmd_id:
        ret = g3_hi_msg_adp_netstart_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_NETJOIN_CONF == cmd_id:
        ret = g3_hi_msg_adp_netjoin_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_NETLEAVE_CONF == cmd_id:
        ret = g3_hi_msg_adp_netleave_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_NETLEAVE_IND == cmd_id:
        ret = g3_hi_msg_adp_netleave_ind()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_RESET_CONF == cmd_id:
        ret = g3_hi_msg_adp_reset_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_LBP_CONF == cmd_id:
        ret = g3_hi_msg_adp_lbp_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3ADP_LBP_IND == cmd_id:
        return g3_hi_msg_adp_lbp_ind(m, len(m))
    elif g3_hi_cmd_ids.G3ADP_ROUTEDELETE_CONF == cmd_id:
        ret = g3_hi_msg_adp_routedelete_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.G3UDP_CONN_GET_CONF == cmd_id:
        ret = g3_hi_msg_udp_conn_get_conf()
        memmove(byref(ret), byref(m), sizeof(ret))
        return ret
    elif g3_hi_cmd_ids.ERROR_IND == cmd_id:
        return g3_hi_msg_error_ind(m, len(m))
    else:
        print("msg couldn't be cast to any known type. cmd_id =", cmd_id)
        raise ValueError

# Higher level structures and functions

class network_info_ex:
    def __init__(self, port_name, baudrate=115200, queues_cnt = 1, q_selector = lambda msg_buff_info : 0):
        self.queues = []
        self.q_selector = q_selector
        self.port_name = port_name
        self.baudrate = baudrate
        self.queues_cnt = queues_cnt
        self.ser = None
        self.thread = None
        self.thread_killer = g3_hi_thread_killer()
        for i in range(0, queues_cnt):
            self.queues.append(queue.Queue())

    def open(self, retries = 15):
        self.ser = serial.Serial()
        self.ser.baudrate = self.baudrate
        self.ser.port = self.port_name
        self.ser.timeout = 0.5
        while (retries > 0):
            try:
                self.ser.open()
                break
            except serial.serialutil.SerialException:
                time.sleep(0.1)
                retries -= 1
        if self.ser.is_open is True:
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.thread = g3_network_receive_thread_start(self.ser, self.q_selector, self.queues, self.thread_killer)
            return True
        return False

    def close(self, retries = 40):
        # Sets variable to kill
        self.thread_killer.thread_kill()
        # Waits for the thread to end
        self.thread.join()
        self.thread = None
        self.empty_queues()
        time.sleep(0.1)
        while (self.ser.is_open is True) and (retries > 0):
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.ser.close()

            if self.ser.is_open:
                print("Retrying...")
                time.sleep(0.25)
            retries -= 1

    def empty_queues(self):
        for q in self.queues:
            while not q.empty():
                q.get()
                q.task_done()


def g3_hi_msg_send_message(net_info_ex, req):
    req_compl = g3_hi_get_complete_msg(req, sizeof(req), req.header.sec_mode)
    g3_network_send(net_info_ex.ser, req_compl)

def g3_hi_msg_receive_message(net_info_ex, q = None, to = 15):
    if (None == q):
        q = net_info_ex.queues[0]
    m = g3_network_get_received_msg(net_info_ex.ser, q, to)
    if m == None:
        return None
    ret = g3_hi_msg_get(m, net_info_ex)
    return ret

