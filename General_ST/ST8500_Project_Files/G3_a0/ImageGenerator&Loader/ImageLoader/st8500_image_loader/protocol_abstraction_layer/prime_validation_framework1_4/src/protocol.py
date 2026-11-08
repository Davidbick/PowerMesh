try:
    import math
    import time
    from util_crc import Crc8, Crc32
    # following try/exception is needed for compatibility with other Application tools
except:
    print("import Issue in protocol.py")

class TimeConst:
    SYMBOL_TO_10US          = 224
    SYMBOL_TO_SEC           = SYMBOL_TO_10US * 1e-5
    FRAME_LENGTH_0          = 276
    FRAME_LENGTH_1          = FRAME_LENGTH_0 * 2
    FRAME_LENGTH_2          = FRAME_LENGTH_0 * 3
    FRAME_LENGTH_3          = FRAME_LENGTH_0 * 4
    FRAME_LENGTH_0_TO_10US  = FRAME_LENGTH_0 * SYMBOL_TO_10US
    FRAME_LENGTH_0_TO_SEC   = FRAME_LENGTH_0 * SYMBOL_TO_SEC
    FRAME_LENGTH_1_TO_10US  = FRAME_LENGTH_1 * SYMBOL_TO_10US
    FRAME_LENGTH_1_TO_SEC   = FRAME_LENGTH_1 * SYMBOL_TO_SEC
    FRAME_LENGTH_2_TO_10US  = FRAME_LENGTH_2 * SYMBOL_TO_10US
    FRAME_LENGTH_2_TO_SEC   = FRAME_LENGTH_2 * SYMBOL_TO_SEC
    FRAME_LENGTH_3_TO_10US  = FRAME_LENGTH_3 * SYMBOL_TO_10US
    FRAME_LENGTH_3_TO_SEC   = FRAME_LENGTH_3 * SYMBOL_TO_SEC
    SUPERFRAME_LEN          = 32
    SUPERFRAME_LEN_0_TO_SEC = SUPERFRAME_LEN * FRAME_LENGTH_0_TO_SEC
    SUPERFRAME_LEN_1_TO_SEC = SUPERFRAME_LEN * FRAME_LENGTH_1_TO_SEC
    SUPERFRAME_LEN_2_TO_SEC = SUPERFRAME_LEN * FRAME_LENGTH_2_TO_SEC
    SUPERFRAME_LEN_3_TO_SEC = SUPERFRAME_LEN * FRAME_LENGTH_3_TO_SEC
    GUARDTIME_LEN           = 1
    TIMER_10US_RANGE        = 1048576
    TIMER_10S_RANGE         = 10.48576
    ALVTIME_136             = [32, 64, 128, 256, 512, 1024, 2048, 4096]
    ALVTIME_14              = [128, 256, 512, 2048, 4096, 8192, 16384, 32768]

    # Method taking two 10us timer values along with their corresponding
    # PC time (datetime instance). Real (i.e. not bound to 20 bits) difference between time10us2-time10us1
    # (10us timer values) is computed and returned 
    @staticmethod
    def time10usDiff(time10us1, pcTime1, time10us2, pcTime2):
        sign = 1
        if pcTime2 < pcTime1:
            sign = -1
            pcTime1, pcTime2 = pcTime2, pcTime1
            time10us1, time10us2 = time10us2, time10us1 
        if time10us2 < time10us1:
            time10us2 += TimeConst.TIMER_10US_RANGE
        
        num_turnarounds = int(math.floor((pcTime2 - pcTime1).total_seconds() / float(TimeConst.TIMER_10S_RANGE)))
                
        delta = time10us2 + num_turnarounds * TimeConst.TIMER_10US_RANGE - time10us1

        return delta * sign
    
    # Same as diff(), but taking two packet dictionaries
    @staticmethod
    def pkt10usDiff(pkt1, pkt2):
        return TimeConst.time10usDiff(pkt1['phyTime'], pkt1['sysTime'], pkt2['phyTime'], pkt2['sysTime'])
    
    # Increase a 10us value of a given delta, which may be either positive or negative.
    # Delta shall be expressed in 10us time unit too.
    # The result is comprised in [0, 2^20-1] 
    @staticmethod
    def incr10us(time10us, delta):
        res = (time10us + delta) % TimeConst.TIMER_10US_RANGE
        if res < 0:
            res += TimeConst.TIMER_10US_RANGE
        return res

    # Returns time.time()-like time value from datetime
    @staticmethod
    def fromDatetimeToTime(t):
        return time.mktime(t.timetuple()) + t.microsecond / 1E6

class Addresses:
    LNID_INVALID    = 0x0000
    LNID_MULTICAST  = 0x3FFE
    LNID_BROADCAST  = 0x3FFF
    LNID_UNASSIGNED = LNID_BROADCAST
    LSID_BASENODE   = 0x00
    LSID_UNASSIGNED = 0xFF

class HTypes:
    GPDU  = 0
    PNPDU = 1
    BPDU  = 2
    PHY   = 3

class macState:
    Disconnected = 0
    Terminal     = 1
    Switch       = 2

class Modulation:
    DBPSK   = 0
    DQPSK   = 1
    D8PSK   = 2
    DBPSKCC = 4
    DQPSKCC = 5
    D8PSKCC = 6
    DBPSKRC = 12
    DQPSKRC = 13
    
    TYPEA  = 0
    TYPEB  = 1
    TYPEBC = 2
    
    TYPEA_MOD_OFFSET  = (TYPEA  << 4)
    TYPEB_MOD_OFFSET  = (TYPEB  << 4)
    TYPEBC_MOD_OFFSET = (TYPEBC << 4)
    
    TYPEA_DBPSK    = TYPEA_MOD_OFFSET  | (DBPSK   & 0x0F)
    TYPEA_DQPSK    = TYPEA_MOD_OFFSET  | (DQPSK   & 0x0F)
    TYPEA_D8PSK    = TYPEA_MOD_OFFSET  | (D8PSK   & 0x0F)
    TYPEA_DBPSKCC  = TYPEA_MOD_OFFSET  | (DBPSKCC & 0x0F)
    TYPEA_DQPSKCC  = TYPEA_MOD_OFFSET  | (DQPSKCC & 0x0F)
    TYPEA_D8PSKCC  = TYPEA_MOD_OFFSET  | (D8PSKCC & 0x0F)
    
    TYPEB_DBPSK    = TYPEB_MOD_OFFSET  | (DBPSK   & 0x0F)
    TYPEB_DQPSK    = TYPEB_MOD_OFFSET  | (DQPSK   & 0x0F)
    TYPEB_D8PSK    = TYPEB_MOD_OFFSET  | (D8PSK   & 0x0F)
    TYPEB_DBPSKCC  = TYPEB_MOD_OFFSET  | (DBPSKCC & 0x0F)
    TYPEB_DQPSKCC  = TYPEB_MOD_OFFSET  | (DQPSKCC & 0x0F)
    TYPEB_D8PSKCC  = TYPEB_MOD_OFFSET  | (D8PSKCC & 0x0F)
    TYPEB_DBPSKRC  = TYPEB_MOD_OFFSET  | (DBPSKRC & 0x0F)
    TYPEB_DQPSKRC  = TYPEB_MOD_OFFSET  | (DQPSKRC & 0x0F)
    
    TYPEBC_DBPSK   = TYPEBC_MOD_OFFSET | (DBPSK   & 0x0F)
    TYPEBC_DQPSK   = TYPEBC_MOD_OFFSET | (DQPSK   & 0x0F)
    TYPEBC_D8PSK   = TYPEBC_MOD_OFFSET | (D8PSK   & 0x0F)
    TYPEBC_DBPSKCC = TYPEBC_MOD_OFFSET | (DBPSKCC & 0x0F)
    TYPEBC_DQPSKCC = TYPEBC_MOD_OFFSET | (DQPSKCC & 0x0F)
    TYPEBC_D8PSKCC = TYPEBC_MOD_OFFSET | (D8PSKCC & 0x0F)
    TYPEBC_DBPSKRC = TYPEBC_MOD_OFFSET | (DBPSKRC & 0x0F)
    TYPEBC_DQPSKRC = TYPEBC_MOD_OFFSET | (DQPSKRC & 0x0F)
    
    # Values of beacon modulations as encoded in PRO packets as well as
    # stored in PIB's beacon-related params
    PROMOD_DBPSKCC = 0
    PROMOD_DQPSKRC = 1
    PROMOD_DBPSKRC = 2
    
    @staticmethod
    def getModulationString(modulation):
        mod = modulation & 0xF
        
        if mod == Modulation.DBPSK:
            return "DBPSK"
        if mod == Modulation.DQPSK:
            return "DQPSK"
        if mod == Modulation.D8PSK:
            return "D8PSK"
        if mod == Modulation.DBPSKCC:
            return "DBPSKCC"
        if mod == Modulation.DQPSKCC:
            return "DQPSKCC"
        if mod == Modulation.D8PSKCC:
            return "D8PSKCC"
        if mod == Modulation.DBPSKRC:
            return "DBPSKRC"
        if mod == Modulation.DQPSKRC:
            return "DQPSKRC"
        return ""
    
    @staticmethod
    def getProModulationString(modulation):
        mod = modulation & 0x3
        
        if mod == Modulation.PROMOD_DBPSKCC:
            return "DBPSKCC"
        if mod == Modulation.PROMOD_DQPSKRC:
            return "DQPSKRC"
        if mod == Modulation.PROMOD_DBPSKRC:
            return "DBPSKRC"
        return ""
    
    @staticmethod
    def getTypeString(modulation):
        fratype = (modulation >> 4) & 0xF
        
        if fratype == Modulation.TYPEA:
            return "TYPEA"
        if fratype == Modulation.TYPEB:
            return "TYPEB"
        if fratype == Modulation.TYPEBC:
            return "TYPEBC"
        return ""
    
class PduLengths:
    BPDU_LEN    = 18
    PNPDU_LEN   = 14
    GENHDR_LEN  = 3
    PKTHDR_LEN  = 7
    CRC_LEN     = 4
    PKTOVHD_LEN = GENHDR_LEN + PKTHDR_LEN + CRC_LEN
    REG_LEN     = 9 #[DB:TODO:extend when security is addressed]
    CON_MIN_LEN = 4
    CON_E_LEN   = 6
    CON_D_LEN   = 6
    CON_MAX_LEN = CON_MIN_LEN + CON_E_LEN + CON_D_LEN
    PRO_MIN_LEN = 5
    PRO_MAX_LEN = 12
    FRA_LEN     = 3
    CFP_LEN     = 6
    ALV_MIN_LEN = 3
    ALV_ACK_LEN = 2
    MUL_LEN     = 4
    AUTH_TAG_SIZE = 6
    
class KeyTypes:
    WK      = 0
    SWK     = 1
    REGK    = 2
    WK_SWK  = 3    

class CTypes:
    REG = 1
    CON = 2
    PRO = 3
    FRA = 5
    CFP = 6
    ALV = 7
    MUL = 8
    SEC = 10

class DfuConst:

    S32 =  0
    S64 =  1
    S128 = 2
    S192 = 3

    @staticmethod
    def BytesPageSize(pagesize):
        return 64*pagesize if not 0 else 32

class MgmtTypes:
    GETreq = 0x00
    GETres = 0x01
    SETreq = 0x02
    RESET  = 0x03
    REBOOT = 0x04
    FWUPmsg = 0x05

class SarTypes:
    SAR_FIRST = 0
    SAR_INTERMEDIATE = 1
    SAR_LAST = 2
    SAR_LAST_WITH_CRC = 3

class SpecPibs:
    ''' specification PIB attributes '''
    macVersion                  = 0x0001 #M:ALL | SN:R  | BN:R
    macMinSwitchSearchTime      = 0x0010 #M:NO  | SN:RW | BN:-
    macSCPMaxTxAttempts         = 0x0014 #M:NO  | SN:RW | BN:RW
    macMinCtlReTxTimer          = 0x0015 #M:ALL | SN:RW | BN:RW
    macSCPRBO                   = 0x0016 #M:NO  | SN:R  | BN:R
    macSCPChSenseCount          = 0x0017 #M:NO  | SN:R  | BN:R
    macCtrlMsgFailTime          = 0x0018 #M:NO  | SN:RW | BN:RW
    macEMASmoothing             = 0x0019 #M:ALL | SN:RW | BN:RW
    macMinBandSearchTime        = 0x001A #M:NO  | SN:RW | BN:RW
    macPromotionMaxTxPeriod     = 0x001B #M:SN  | SN:RW | BN:-
    macPromotionMinTxPeriod     = 0x001C #M:SN  | SN:RW | BN:-
    macSARSize                  = 0x001D #M:ALL | SN:R  | BN:RW
    macEUI48                    = 0x001F #M:ALL | SN:R  | BN:R
    macLNID                     = 0x0020 #M:SN  | SN:R  | BN:-
    macLSID                     = 0x0021 #M:SN  | SN:R  | BN:-
    macSID                      = 0x0022 #M:SN  | SN:R  | BN:-
    macSNA                      = 0x0023 #M:SN  | SN:R  | BN:R
    macState                    = 0x0024 #M:SN  | SN:R  | BN:-
    macSCPLength                = 0x0025 #M:SN  | SN:R  | BN:R
    macNodeHierarchyLevel       = 0x0026 #M:SN  | SN:R  | BN:-
    macBeaconRxFrequency        = 0x002A #M:SN  | SN:R  | BN:- 
    macBeaconTxFrequency        = 0x002B #M:SN  | SN:R  | BN:R
    macCapabilities             = 0x002C #M:ALL | SN:R  | BN:R
    macFrameLength              = 0x002D #M:ALL | SN:R  | BN:R
    macCFPLength                = 0x002E #M:ALL | SN:R  | BN:R
    macBCMode                   = 0x0030 #M:ALL | SN:R  | BN:R
    macBeaconRxQlty             = 0x0032 #M:SN  | SN:R  | BN:-
    macBeaconTxQlty             = 0x0033 #M:ALL | SN:RW | BN:RW
    macCSMAR1                   = 0x0034 #M:ALL | SN:R  | BN:R
    macCSMAR2                   = 0x0035 #M:ALL | SN:R  | BN:R
    macCSMADelay                = 0x0038 #M:ALL | SN:R  | BN:R
    macBeaconRxPos              = 0x0039 #M:SN  | SN:R  | BN:- 
    macBeaconTxPos              = 0x003A #M:SN  | SN:R  | BN:R
    macCSMAR1Robust             = 0x003B #M:ALL | SN:R  | BN:R
    macCSMAR2Robust             = 0x003C #M:ALL | SN:R  | BN:R
    macCSMADelayRobust          = 0x003D #M:ALL | SN:R  | BN:R
    macAliveTimeMode            = 0x003E #M:BN  | SN:-  | BN:RW
    macTxDataPktCount           = 0x0040 #M:NO  | SN:R  | BN:R
    macRxDataPktCount           = 0x0041 #M:NO  | SN:R  | BN:R
    macTxCtrlPktCount           = 0x0042 #M:NO  | SN:R  | BN:R
    macRxCtrlPktCount           = 0x0043 #M:NO  | SN:R  | BN:R
    macCSMAFailCount            = 0x0044 #M:NO  | SN:R  | BN:R
    macCSMAChBusyCount          = 0x0045 #M:NO  | SN:R  | BN:R
    macRobustnessManagement     = 0x004A #M:NO  | SN:R  | BN:RW
    macUpdatedRMTimeout         = 0x004B #M:ALL | SN:RW | BN:RW
    macALVHopRepetitions        = 0x004C #M:ALL | SN:RW | BN:RW
    macListRegDevices           = 0x0050 #M:BN  | SN:-  | BN:R
    macListActiveConn           = 0x0051 #M:BN  | SN:-  | BN:R
    macListMcastEntries         = 0x0052 #M:NO  | SN:-  | BN:R
    macListDirectConn           = 0x0054 #M:NO  | SN:R  | BN:R
    macListDirectTable          = 0x0055 #M:NO  | SN:R  | BN:R
    macListAvailableSwitches    = 0x0056 #M:SN  | SN:R  | BN:-
    macListActiveConnEX         = 0x0058 #M:BN  | SN:-  | BN:R  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macListPhyComm              = 0x0059 #M:ALL | SN:R  | BN:R
    macListSwitchTable          = 0x005A #M:SN  | SN:R  | BN:R
    macSecDUK                   = 0x005B #M:NO  | SN:-  | BN:W
    macUpdateKeysTime           = 0x005C #M:NO  | SN:-  | BN:RW
    macActionTxData             = 0x0060 #M:SN  | SN:RW | BN:-
    macActionConnClose          = 0x0061 #M:SN  | SN:RW | BN:-
    macActionRegReject          = 0x0062 #M:SN  | SN:RW | BN:-
    macActionProReject          = 0x0063 #M:SN  | SN:RW | BN:-
    macActionUnregister         = 0x0064 #M:SN  | SN:RW | BN:-
    macActionPromote            = 0x0065 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionDemote             = 0x0066 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionReject             = 0x0067 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macAliveTime                = 0x0068 #M:BN  | SN:-  | BN:RW #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionBroadcastDataBurst = 0x006A #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionMgmtCon            = 0x006B #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionMgmtMul            = 0x006C #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionUnregisterBN       = 0x006D #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionConnCloseBN        = 0x006E #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionSegmented432       = 0x006F #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    AppFwdlRunning              = 0x0070 #M:SN  | SN:R  | BN:-
    AppFwdlRxPktCount           = 0x0071 #M:SN  | SN:R  | BN:-
    AppFwVersion                = 0x0075 #M:ALL | SN:R  | BN:R
    AppVendorId                 = 0x0076 #M:ALL | SN:R  | BN:R
    AppProductId                = 0x0077 #M:ALL | SN:R  | BN:R
    AppListZCStatus             = 0x0078 #M:ALL | SN:R  | BN:R
    macActionAppemuDataBurst    = 0x0080 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionMgmtDataBurst      = 0x0081 #M:ALL | SN:RW | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionProBcn             = 0x0082 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    macActionPromoteDS          = 0x0083 #M:BN  | SN:-  | BN:W  #[DB:also used as member of the BN class MgmtPibs for enhanced queries to pib]
    phyStatsCRCIncorrectCount   = 0x00A0 #M:NO  | SN:R  | BN:R
    phyStatsCRCFailCount        = 0x00A1 #M:NO  | SN:R  | BN:R
    phyStatsTxDropCount         = 0x00A2 #M:NO  | SN:R  | BN:R
    phyStatsRxDropCount         = 0x00A3 #M:NO  | SN:R  | BN:R
    phyStatsRxTotalCount        = 0x00A4 #M:NO  | SN:R  | BN:R
    phyStatsBlkAvgEvm           = 0x00A5 #M:NO  | SN:R  | BN:R
    phyEMASmoothing             = 0x00A8 #M:NO  | SN:R  | BN:R
    
    mandatory_readable_sn = {
        'macVersion':                  macVersion,
        'macSARSize':                  macSARSize,
        'macEUI48':                    macEUI48,
        'macLNID':                     macLNID,
        'macLSID':                     macLSID,
        'macSID':                      macSID,
        'macSNA':                      macSNA,
        'macState':                    macState,
        'macSCPLength':                macSCPLength,
        'macNodeHierarchyLevel':       macNodeHierarchyLevel,
        'macBeaconRxFrequency':        macBeaconRxFrequency,
        'macBeaconTxFrequency':        macBeaconTxFrequency,
        'macCapabilities':             macCapabilities,
        'macFrameLength':              macFrameLength,
        'macCFPLength':                macCFPLength,
        'macBCMode':                   macBCMode,
        'macBeaconRxQlty':             macBeaconRxQlty,
        'macCSMAR1':                   macCSMAR1,
        'macCSMAR2':                   macCSMAR2,
        'macCSMADelay':                macCSMADelay,
        'macBeaconRxPos':              macBeaconRxPos,
        'macBeaconTxPos':              macBeaconTxPos,
        'macCSMAR1Robust':             macCSMAR1Robust,
        'macCSMAR2Robust':             macCSMAR2Robust,
        'macCSMADelayRobust':          macCSMADelayRobust,
        'macListAvailableSwitches':    macListAvailableSwitches, #AP: list for the moment not supported by method GetPib of class ServiceNodeEmb
        'macListPhyComm':              macListPhyComm, #AP: list for the moment not supported by method GetPib of class ServiceNodeEmb
        'macListSwitchTable':          macListSwitchTable, #AP: list for the moment not supported by method GetPib of class ServiceNodeEmb
        'AppFwdlRunning':              AppFwdlRunning,
        'AppFwdlRxPktCount':           AppFwdlRxPktCount,
        'AppFwVersion':                AppFwVersion,
        'AppVendorId':                 AppVendorId,
        'AppProductId':                AppProductId,
        'AppListZCStatus':             AppListZCStatus, #AP: attribute to be implemented
    }
    
    mandatory_writable_sn = {
        'macMinCtlReTxTimer':          macMinCtlReTxTimer,
        'macEMASmoothing':             macEMASmoothing,
        'macPromotionMaxTxPeriod':     macPromotionMaxTxPeriod,
        'macPromotionMinTxPeriod':     macPromotionMinTxPeriod,
        'macBeaconTxQlty':             macBeaconTxQlty,
        'macUpdatedRMTimeout':         macUpdatedRMTimeout,
        'macALVHopRepetitions':        macALVHopRepetitions,
        'macActionTxData':             macActionTxData,
        'macActionConnClose':          macActionConnClose,
        'macActionRegReject':          macActionRegReject,
        'macActionProReject':          macActionProReject,
        'macActionUnregister':         macActionUnregister,
        'macActionMgmtDataBurst':      macActionMgmtDataBurst,
    }
    
    mandatory_list_sn = {
        'macListAvailableSwitches':    macListAvailableSwitches,
        'macListPhyComm':              macListPhyComm,
        'macListSwitchTable':          macListSwitchTable,
    }
    
    mandatory_readable_bn = {
        'macVersion':                  macVersion,
        'macMinCtlReTxTimer':          macMinCtlReTxTimer,
        'macEMASmoothing':             macEMASmoothing,
        'macSARSize':                  macSARSize,
        'macEUI48':                    macEUI48,
        'macCapabilities':             macCapabilities,
        'macFrameLength':              macFrameLength,
        'macCFPLength':                macCFPLength,
        'macBCMode':                   macBCMode,
        #'macBeaconTxQlty':             macBeaconTxQlty,
        'macCSMAR1':                   macCSMAR1,
        'macCSMAR2':                   macCSMAR2,
        'macCSMADelay':                macCSMADelay,
        'macCSMAR1Robust':             macCSMAR1Robust,
        'macCSMAR2Robust':             macCSMAR2Robust,
        'macCSMADelayRobust':          macCSMADelayRobust,
        'macAliveTimeMode':            macAliveTimeMode,
        'macUpdatedRMTimeout':         macUpdatedRMTimeout,
        'macALVHopRepetitions':        macALVHopRepetitions,
        #'macListRegDevices':           macListRegDevices,
        #'macListActiveConn':           macListActiveConn,
        #'macListActiveConnEX':         macListActiveConnEX,
        #'macListPhyComm':              macListPhyComm,
        #'macAliveTime':                macAliveTime,
        'AppFwVersion':                AppFwVersion,
        'AppVendorId':                 AppVendorId,
        'AppProductId':                AppProductId,
        #'AppListZCStatus':             AppListZCStatus,
    }
    
    mandatory_writable_bn = {
        'macMinCtlReTxTimer':          macMinCtlReTxTimer,
        'macEMASmoothing':             macEMASmoothing,
        'macSARSize':                  macSARSize,
        'macBeaconTxQlty':             macBeaconTxQlty,
        'macAliveTimeMode':            macAliveTimeMode,
        'macUpdatedRMTimeout':         macUpdatedRMTimeout,
        'macALVHopRepetitions':        macALVHopRepetitions,
        'macActionPromote':            macActionPromote,
        'macActionDemote':             macActionDemote,
        'macActionReject':             macActionReject,
        'macAliveTime':                macAliveTime,
        'macActionBroadcastDataBurst': macActionBroadcastDataBurst,
        'macActionMgmtCon':            macActionMgmtCon,
        'macActionMgmtMul':            macActionMgmtMul,
        'macActionUnregisterBN':       macActionUnregisterBN,
        'macActionConnCloseBN':        macActionConnCloseBN,
        'macActionSegmented432':       macActionSegmented432,
        'macActionAppemuDataBurst':    macActionAppemuDataBurst,
        'macActionMgmtDataBurst':      macActionMgmtDataBurst,
        'macActionProBcn':             macActionProBcn,
        'macActionPromoteDS':          macActionPromoteDS,
    }
    
    mandatory_list_bn = {
        'macListRegDevices':           macListRegDevices,
        'macListActiveConn':           macListActiveConn,
        #'macListActiveConnEX':         macListActiveConnEX,
        'macListPhyComm':              macListPhyComm,
    }
    
    # Static helper method to convert an array of octets into the PhyComm.
    # Return value is a list of dictionaries. Each dictionary keys representing
    # PhyComm list fields, i.e.:
    # - lnid
    # - sid
    # - txPwr
    # - rxLvl
    # - snr
    # - txMod
    # - phyTypeCap
    # - rxAge
    @staticmethod
    def convertToPhyCommList(octets):
        entrySize = 10
        lis = []
        
        for i in range (0, int(len(octets)/entrySize)):
            token = octets[i*entrySize:(i + 1)*entrySize]
            lis += [{'lnid' : (token[0] << 8) + token[1],
                     'sid' : token[2],
                     'txPwr' : token[3],
                     'rxLvl' : token[4],
                     'snr' : token[5],
                     'txMod' : token[6],
                     'phyTypeCap' : token[7],
                     'rxAge' : (token[8] << 8) + token[9]}]
            
        return lis

class CommonVendSpecPibs():
    ''' Vendor specific PIBs common to both BN and SN '''
    macZeroCrossInfoTarget = 0x00F2
    macZeroCrossPeriod     = 0x00F3
    phyStartTxTest         = 0x8001
    phyStartRxTest         = 0x8002
    phyCountersReset       = 0x8003
    phyPreDriverGain       = 0x8004
    phyChannelMap          = 0x8005
    phyThermCounter        = 0x8007
    primeTM                = 0x8009
    primeConfig            = 0x8010
    TypeNode               = 0x8011
    macZeroCrossInfo       = 0x8015
    macZeroCrossOffset10us = 0x8016
    macCountersReset       = 0x8020
    phyLDVcc               = 0x8023 
    phyMolOffset           = 0x8027
    macSARTxWithCrc        = 0x8029
    macBeaconTxMod         = 0x8031
    macBeaconDSTxFrequency = 0x8032
    macBeaconDSTxPos       = 0x8033
    macBeaconDSTxMod       = 0x8034
    macSwitchBeaconTxMod   = 0x8040
    macArqPiggybackTime    = 0x8042
    ConnAE                 = 0x8055
    SecurityProfile        = 0x8056
    HIOperatingMode        = 0x8051
    GpioConfig             = 0x8052
    GpioStatus             = 0x8053
    macDynamicChannelSel   = 0x8071
    macDynamicChannelInfo  = 0x8072
    macDynamicChannelInterval = 0x8073 
    macDynamicChannelMaxWait = 0x8074
    macDynamicChannelOffset = 0x8075
    macDynamicChannelPool  = 0x8076
    macBCmodeConfig        = 0x8090
    macEmaSnrWHigh         = 0x80F0
    macEmaSnrWLow          = 0x80F1
    macRmInc               = 0x80F2
    macRmMinThr            = 0x80F3
    Res                    = 0x8100
    
    ''' Helper functions to set macSwitchBeaconTxMod parameter '''
    @staticmethod
    def macSwitchBeaconTxMod_set(en=1, ds=0, mod=0):
        return ((en&1)<<7) | ((ds&1)<<6) | (mod&3)
    
    @staticmethod
    def macSwitchBeaconTxMod_en_check(val):
        return (val & 0x80) >> 7

    @staticmethod
    def macSwitchBeaconTxMod_ds_check(val):
        return (val & 0x70) >> 6

    @staticmethod
    def macSwitchBeaconTxMod_mod_get(val):
        return val & 0x3
    
    ''' Helper function to set  '''
    @staticmethod
    def macBeaconTxTimeShift_set(bcnTxTimeShift10us=0, bcnTxTimeVar10us=0):
        if bcnTxTimeShift10us > 127:
            bcnTxTimeShift10us = 127
        elif bcnTxTimeShift10us < -128:
            bcnTxTimeShift10us = -128
        if bcnTxTimeVar10us < 0:
            bcnTxTimeVar10us *= -1
        return ((bcnTxTimeShift10us & 0xFF) | ((bcnTxTimeVar10us & 0xFF) << 8))
    
    @staticmethod
    def macBeaconTxTimeShift_Shift_get(val):
        retVal = val & 0xFF
        if retVal & 0x80:
            retVal = -1*(retVal & 0x7F)
        return retVal
    
    @staticmethod
    def macBeaconTxTimeShift_Var_get(val):
        return (val >> 8) & 0xFF
    

class BeaconPdu:
    def __init__(self, pdu):
        # fields

        self.qlty   = pdu[0] & 0x0F
        self.sid    = pdu[1]
        self.level  = (pdu[2] & 0xFC) >> 2
        self.cfp    = ((pdu[2] & 0x03) << 8) | pdu[3]
        self.csma   = (pdu[4] & 0x80) >> 7
        self.pos    = pdu[4] & 0x7F
        self.fralen = (pdu[5] & 0xC0) >> 6
        self.phybc  = (pdu[5] & 0x20) >> 5
        self.macbc  = (pdu[5] & 0x10) >> 4
        self.cap_r  = (pdu[5] & 0x08) >> 3
        self.seq    = (pdu[6] & 0xF8) >> 3
        self.frq    = (pdu[6] & 0x07)
        self.sna    = pdu[7:13]
        self.cost   = pdu[13]
        self.crc    = (pdu[14] << 24) | (pdu[15] << 16) | (pdu[16] << 8) | pdu[17]
        # additional info
        self.crcerr = (Crc32.Calculate(pdu[0:-(PduLengths.CRC_LEN)] + [0x00, 0x01, 0x04, 0x00]) != self.crc)
        self.fmtstr = \
            " QLTY: "   + str(self.qlty) + \
            " CFP: "    + str(self.cfp) + \
            " CSMA: "   + str(self.csma) + \
            " POS: "    + str(self.pos) + \
            " FRALEN: " + str(self.fralen) + \
            " PHYBC: "  + str(self.phybc) + \
            " MACBC: "  + str(self.macbc) + \
            " CAP_R: "  + str(self.cap_r) + \
            " SEQ: "    + str(self.seq) + \
            " FRQ: "    + str(self.frq) + \
            " SNA: "    + "".join("{:02x}:".format(c) for c in self.sna) + \
            " COST: "   + str(self.cost)

class PromotionNeededPdu:
    def __init__(self, pdu):
        # fields
                
        self.ver    = (pdu[0] & 0x0C) >> 2
        self.cap_r  = (pdu[0] & 0x02) >> 1
        self.cap_bc = pdu[0] & 0x01 
        self.sna    = pdu[1:7]
        self.pna    = pdu[7:13]
        self.hcs    = pdu[13]
        # additional info
        self.hcserr = (Crc8.Calculate(pdu[0:-1]) != self.hcs)
        self.fmtstr = \
            " VER: "    + str(self.ver) + \
            " CAP_R: "  + str(self.cap_r) + \
            " CAP_BC: " + str(self.cap_bc) + \
            " SNA: "    + "".join("{:02x}:".format(c) for c in self.sna) + \
            " PNA: "    + "".join("{:02x}:".format(c) for c in self.pna)

class GenericHeader:
    def __init__(self, pdu, sna=None):
        # fields
        
        self.do     = (pdu[1] & 0x40) >> 6
        self.level  = pdu[1] & 0x3F
        self.hcs    = pdu[2]
        # additional info
        self.hcserr = (Crc8.Calculate(sna + pdu[0:2]) != self.hcs) if sna is not None else False

class PacketHeader:
    def __init__(self, pdu, sna=None):
        # fields
        offset = PduLengths.GENHDR_LEN
        self.rm    = (pdu[offset] & 0xF0) >> 4
        self.prio  = (pdu[offset] & 0x0C) >> 2
        self.c     = (pdu[offset] & 0x02) >> 1
        self.lcid  = ((pdu[offset] & 0x01) << 8) | pdu[offset + 1]
        self.ctype = self.lcid
        self.sid   = pdu[offset + 2]
        self.lnid  = (pdu[offset + 3] << 6) | ((pdu[offset + 4] & 0xFC) >> 2)
        self.len   = ((pdu[offset + 4] & 0x01) << 8) | pdu[offset + 5]
        self.nad   = (pdu[offset + 6] & 0x80) >> 7
        self.tref  = (pdu[offset + 6] & 0x40) >> 6
        self.arq   = (pdu[offset + 6] & 0x20) >> 5
        self.psh   = (pdu[offset + 6] & 0x10) >> 4
        # additional info
        self.crc    = (pdu[-4] << 24) | (pdu[-3] << 16) | (pdu[-2] << 8) | pdu[-1]
        self.crcerr = (Crc32.Calculate(sna + pdu[0:-(PduLengths.CRC_LEN)]) != self.crc) if sna is not None else False

class Reg:
    def __init__(self, pdu, do=0, lnid=Addresses.LNID_UNASSIGNED):
        # fields
        offset = 0
        
                
        Keystring = ""
        Cntstring = ""
        self.n        = (pdu[offset] & 0x80) >> 7
        self.r        = (pdu[offset] & 0x40) >> 6
        self.spc      = (pdu[offset] & 0x30) >> 4
        self.cap_r    = (pdu[offset] & 0x08) >> 3
        self.cap_bc   = (pdu[offset] & 0x04) >> 2
        self.cap_sw   = (pdu[offset] & 0x02) >> 1
        self.cap_pa   = pdu[offset] & 0x01
        self.cap_cfp  = (pdu[offset + 1] & 0x80) >> 7
        self.cap_dc   = (pdu[offset + 1] & 0x40) >> 6 
        self.alv_f    = (pdu[offset + 1] & 0x20) >> 5
        self.cap_arq  = (pdu[offset + 1] & 0x08) >> 3
        self.time     = pdu[offset + 1] & 0x07
        self.eui48    = pdu[(offset + 2):(offset + 8)]
        self.rm_f     = (pdu[offset + 8] & 0xC0) >> 6
        self.sar_size = (pdu[offset + 8] & 0x38) >> 3
        self.subtype = self.n + ((0 if lnid == Addresses.LNID_UNASSIGNED else 1) << 1) + (do << 2)
        subtypeName = ["REQ", "???", "ACK", "UNR_S", "???", "REJ", "RSP", "UNR_B"]
        if self.spc != 0:
            if self.subtype == 6: #REG_RSP 
                self.swk      = pdu[(offset + 13):(offset + 37)]
                self.wk       = pdu[(offset + 37):(offset + 61)]
                Keystring     = " SWK: "     + "".join("{:02X}".format(c) for c in self.swk) + \
                                " WK: "      + "".join("{:02X}".format(c) for c in self.wk)                    
            if ((self.subtype == 0) or (self.subtype == 6) or (self.subtype == 2) or (self.subtype == 5)): #REG_REQ, REQ_RSP, REG_ACK, REG_REJ
                self.cnt      = (pdu[offset + 9] << 24) | (pdu[offset + 10] << 16) | (pdu[offset + 11] << 8) | pdu[offset + 12]
                Cntstring     = " CNT: "     + str(self.cnt)
        
        # additional info
        self.fmtstr = \
            " (REG_" + subtypeName[self.subtype] + ")" + \
            " R: "        + str(self.r) + \
            " SPC: "      + str(self.spc) + \
            " CAP_R: "    + str(self.cap_r) + \
            " CAP_BC: "   + str(self.cap_bc) + \
            " CAP_SW: "   + str(self.cap_sw) + \
            " CAP_PA: "   + str(self.cap_pa) + \
            " CAP_CFP: "  + str(self.cap_cfp) + \
            " CAP_DC: "   + str(self.cap_dc) + \
            " ALV_F: "    + str(self.alv_f) + \
            " CAP_ARQ: "  + str(self.cap_arq) + \
            " TIME: "     + str(self.time) + \
            " EUI48: "    + "".join("{:02x}:".format(c) for c in self.eui48) + \
            " RM_F: "     + str(self.rm_f) + \
            " SAR_SIZE: " + str(self.sar_size) + Cntstring + Keystring
    
    def isReq(self):
        return (self.subtype == 0)
    
    def isAck(self):
        return (self.subtype == 2)
    
    def isUnrS(self):
        return (self.subtype == 3)
    
    def isRej(self):
        return (self.subtype == 5)
    
    def isRsp(self):
        return (self.subtype == 6)
    
    def isUnrB(self):
        return (self.subtype == 7)

class Con:
    def __init__(self, pdu, do=0):
        # fields
        offset = 0
        self.n         = (pdu[offset] & 0x80) >> 7
        self.d         = (pdu[offset] & 0x40) >> 6
        self.arq       = (pdu[offset] & 0x20) >> 5
        self.e         = (pdu[offset] & 0x10) >> 4
        self.ae        = (pdu[offset] & 0x08) >> 3
        self.lcid      = ((pdu[offset] & 0x01) << 8) | pdu[offset + 1]
        offset += 2
        if self.e == 1:
            self.eui48 = pdu[offset:(offset + 6)]
            offset += PduLengths.CON_E_LEN
        if self.d == 1:
            self.dclcid = ((pdu[offset] & 0x01) << 8) | pdu[offset + 1]
            self.dcnad  = (pdu[offset + 2] & 0x80) >> 7
            self.dclnid = ((pdu[offset + 2] & 0x3F) << 8) | pdu[offset + 3]
            self.dssid = pdu[offset + 4]
            self.dcsid = pdu[offset + 5]
            offset += PduLengths.CON_D_LEN
        self.type = pdu[offset]
        self.dlen = pdu[offset + 1]
        self.data = []
        if self.dlen > 0:
            self.data = pdu[(offset + 2):(offset + 2 + self.dlen)]
        # additional info
        self.subtype = do + (self.n << 1)
        subtypeName = ["REQ_S", "REQ_B", "CLS_S", "CLS_B"]
        self.fmtstr = \
            " (CON_" + subtypeName[self.subtype] + ")" + \
            " D: "    + str(self.d) + \
            " ARQ: "  + str(self.arq) + \
            " E: "    + str(self.e) + \
            " AE: "   + str(self.ae) + \
            " LCID: " + "{:04X}".format(self.lcid)
        if self.data != []:
            self.fmtstr += " DATA: " + "".join("{:02X}".format(c) for c in self.data)
        
    def isReqS(self):
        return (self.subtype == 0)
    
    def isReqB(self):
        return (self.subtype == 1)
    
    def isClsS(self):
        return (self.subtype == 2)
    
    def isClsB(self):
        return (self.subtype == 3)

class Pro:
    def __init__(self, pdu, extend=False, do=0):
        # fields
        offset = 0
        self.n        = (pdu[offset] & 0x80) >> 7
        self.bcn_pos  = pdu[offset] & 0x7F
        self.nsid     = pdu[offset + 1]
        self.rq       = (pdu[offset + 2] & 0xE0) >> 5
        self.time     = (pdu[offset + 2] & 0x1C) >> 2
        self.seq      = ((pdu[offset + 2] & 0x03) << 3) | ((pdu[offset + 3] & 0xE0) >> 5)
        self.frq      = (pdu[offset + 3] & 0x1C) >> 2
        self.mod      = pdu[offset + 3] & 0x03
        self.ack      = (pdu[offset + 4] & 0x80) >> 7
        self.ds       = (pdu[offset + 4] & 0x40) >> 6
        self.pn_bc    = (pdu[offset + 4] & 0x08) >> 3
        self.pn_r     = (pdu[offset + 4] & 0x04) >> 2
        self.sw_dc    = (pdu[offset + 4] & 0x02) >> 1
        self.sw_arq   = pdu[offset + 4] & 0x01
        if extend:
            self.pna  = pdu[(offset + 5):(offset + 11)]
            self.cost = pdu[offset + 11]
        # additional info
        self.subtype = self.ack + (self.n << 1) + (do << 2)
        subtypeName = ["REQ_S", "ACK", "DEM_S", "NACK", "REQ_B", "ACK", "DEM_B", "NACK"]
        self.fmtstr = \
            " (PRO_" + subtypeName[self.subtype] + ")" + \
            " BCN_POS: "  + str(self.bcn_pos) + \
            " NSID: "     + "{:02X}".format(self.nsid) + \
            " RQ: "       + str(self.rq) + \
            " TIME: "     + str(self.time) + \
            " SEQ: "      + str(self.seq) + \
            " FRQ: "      + str(self.frq) + \
            " MOD: "      + str(self.mod) + \
            " DS: "       + str(self.ds) + \
            " BC: "       + str(self.pn_bc) + \
            " PN_R: "     + str(self.pn_r) + \
            " SW_DC: "    + str(self.sw_dc) + \
            " SW_ARQ: "   + str(self.sw_arq)
        if extend:
            self.fmtstr += \
                " PNA: "  + "".join("{:02x}:".format(c) for c in self.pna) + \
                " COST: " + str(self.cost)
    
    def isReqS(self):
        return (self.subtype == 0)
    
    def isAck(self):
        return (self.subtype == 1) or (self.subtype == 5)
    
    def isDemS(self):
        return (self.subtype == 2)
    
    def isNack(self):
        return (self.subtype == 3) or (self.subtype == 7)
    
    def isReqB(self):
        return (self.subtype == 4)
    
    def isDemB(self):
        return (self.subtype == 6)

class Fra:
    def __init__(self, pdu):
        # fields
        offset = 0
        self.type    = (pdu[offset] & 0xC0) >> 6
        self.len     = (pdu[offset] & 0x30) >> 4
        self.phybc   = (pdu[offset] & 0x08) >> 3
        self.cfp_len = ((pdu[offset] & 0x03) << 8) | pdu[offset + 1] 
        self.seq     = (pdu[offset + 2] & 0xF8) >> 3
        # additional info
        self.fmtstr = \
            " TYPE: "    + "{:02X}".format(self.type) + \
            " LEN: "     + str(self.len) + \
            " PHYBC: "   + str(self.phybc) + \
            " CFP_LEN: " + str(self.cfp_len) + \
            " SEQ: "     + str(self.seq)

class Cfp:
    def __init__(self, pdu, do=0):
        # fields
        offset = 0
        self.n    = (pdu[offset] & 0x80) >> 7
        self.dir  = (pdu[offset] & 0x40) >> 6
        self.seq  = (pdu[offset] & 0x3E) >> 1 
        self.lcid = ((pdu[offset] & 0x01) << 8) | pdu[offset + 1]
        self.len  = (pdu[offset + 2] & 0xFE) >> 1
        self.pos  = ((pdu[offset + 2] & 0x01) << 8) | pdu[offset + 3]
        self.type = (pdu[offset + 4] & 0xC0) >> 6 
        self.lnid = ((pdu[offset + 4] & 0x3F) << 8) | pdu[offset + 5]
        # additional info
        self.subtype = (0 if self.type == 0 else 1) + (do << 1) + ((self.n if self.type != 2 else 1) << 2)
        subtypeName = ["ALC_REQ_S", "DALC_REQ", "ALC_IND", "DALC_RSP", "???", "???", "ALC_REJ", "CHG_IND"]
        self.fmtstr = \
            " (CFP_" + subtypeName[self.subtype] + ")" + \
            " N: "    + str(self.n) + \
            " DIR: "  + str(self.dir) + \
            " SEQ: "  + str(self.seq) + \
            " LCID: " + "{:04X}".format(self.lcid) + \
            " LEN: "  + str(self.len) + \
            " POS: "  + str(self.pos) + \
            " TYPE: " + str(self.type) + \
            " LNID: " + "{:04X}".format(self.lnid)
    
    def isAlcReqS(self):
        return (self.subtype == 0)
    
    def isDalcReq(self):
        return (self.subtype == 1)
    
    def isAlcInd(self):
        return (self.subtype == 2)
    
    def isDalcRsp(self):
        return (self.subtype == 3)
    
    def isAlcRej(self):
        return (self.subtype == 6)
    
    def isChgInd(self):
        return (self.subtype == 7)

class Alv:
    def __init__(self, pdu, is14=True, do=0):
        offset = 0
        if not is14:
            # fields
            self.rxcnt = (pdu[offset] & 0xE0) >> 5
            self.txcnt = (pdu[offset] & 0x1C) >> 2
            self.time  = pdu[offset + 1] & 0x07
            self.ssid  = pdu[offset + 2]
            # additional info
            self.subtype = do
            subtypeName = ["S", "B"]
            self.fmtstr = \
                " (ALV_" + subtypeName[self.subtype] + ")" + \
                " RXCNT: " + str(self.rxcnt) + \
                " TXCNT: " + str(self.txcnt) + \
                " TIME: "  + str(self.time) + \
                " SSID: "  + "{:02X}".format(self.ssid)
        else:
            # fields            
            self.r = (pdu[offset] & 0x80) >> 7
            if self.r != 0:
                self.rtl        = (pdu[offset] & 0x78) >> 3
                self.time       = pdu[offset] & 0x07
                self.min_level  = (pdu[offset + 1] & 0xFC) >> 2
                self.tx_seq     = (pdu[offset + 2] & 0xE0) >> 5
                self.rx_enc     = pdu[offset + 2] & 0x0F
                rep_offset = PduLengths.ALV_MIN_LEN
                if self.rx_enc != 0:
                    self.rx_snr = (pdu[offset + 3] & 0xF0) >> 4
                    self.rx_pow = pdu[offset + 3] & 0x0F
                    rep_offset += 1
                self.vald       = []
                self.rep_d      = []
                self.valu       = []
                self.rep_u      = []
                for b in pdu[(offset + rep_offset):-(PduLengths.CRC_LEN)]:
                    self.vald.append((b & 0x80) >> 7)
                    self.rep_d.append((b & 0x70) >> 4)
                    self.valu.append((b & 0x08) >> 3)
                    self.rep_u.append(b & 0x07)
            else:
                self.rx_enc = pdu[offset] & 0x0F
                self.rx_snr = (pdu[offset + 1] & 0xF0) >> 4
                self.rx_pow = pdu[offset + 1] & 0x0F
            # additional info
            self.subtype = self.r + (do << 1) + ((0 if self.rx_enc == 0 else 1) << 2)
            subtypeName = ["???", "RSP_S", "???", "REQ_B", "ACK_S", "RSP_ACK", "ACK_B", "???"]
            self.fmtstr = " (ALV_" + subtypeName[self.subtype] + ")"
            if self.r != 0:
                self.fmtstr += \
                    " RTL: "        + str(self.rtl) + \
                    " TIME: "       + str(self.time) + \
                    " MIN_LEVEL: "  + str(self.min_level) + \
                    " TX_SEQ: "     + str(self.tx_seq)
                if self.rx_enc != 0:
                    self.fmtstr += \
                        " RX_ENC: " + str(self.rx_enc) + \
                        " RX_SNR: " + str(self.rx_snr) + \
                        " RX_POW: " + str(self.rx_pow)
                self.fmtstr += \
                    " VAL_D: ["      + " ".join(["I" if c == 0 else "V" for c in self.vald]) + "]" + \
                    " REP_D: ["      + " ".join("{:02d}".format(c) for c in self.rep_d) + "]" + \
                    " VAL_U: ["      + " ".join(["I" if c == 0 else "V" for c in self.valu]) + "]" + \
                    " REP_U: ["      + " ".join("{:02d}".format(c) for c in self.rep_u) + "]"
            else:
                self.fmtstr += \
                    " RX_ENC: " + str(self.rx_enc) + \
                    " RX_SNR: " + str(self.rx_snr) + \
                    " RX_POW: " + str(self.rx_pow)
        # additional info
        self.is14 = is14
    
    def isS(self):
        return ((not self.is14) and (self.subtype == 0))
    
    def isB(self):
        return ((not self.is14) and (self.subtype == 1))
    
    def isRspS(self):
        return (self.is14 and (self.subtype == 1))
    
    def isReqB(self):
        return (self.is14 and (self.subtype == 3))
    
    def isAckS(self):
        return (self.is14 and (self.subtype == 4))
    
    def isRspAck(self):
        return (self.is14 and (self.subtype == 5))
    
    def isAckB(self):
        return (self.is14 and (self.subtype == 6))

class Dfu_Rsp:
    def __init__(self, pdu):
        # fields
        offset = 0
        self.state    = ((pdu[offset + 1]) >> 4)
        self.crc      = pdu[(offset+2):(offset+5)]
        if self.state == 5:
            self.exception_code = pdu[offset + 6] & 0x7F
            self.permanent_bit = (pdu[offset + 6] & 0x80) >> 7
            
class Mul:
    def __init__(self, pdu, do=0, lnid=Addresses.LNID_INVALID):
        # fields
        offset = 0
        self.n    = (pdu[offset] & 0x80) >> 7
        self.ae   = (pdu[offset] & 0x40) >> 6
        self.lcid = ((pdu[offset] & 0x01) << 8) | pdu[offset + 1]
        self.type = pdu[offset + 2]
        self.dlen = pdu[offset + 3]
        if self.dlen > 0:
            self.data = pdu[(offset + 4):]
        # additional info
        self.subtype = (0 if lnid == Addresses.LNID_INVALID else 1) + (self.n << 1) + (do << 2)
        subtypeName = ["???", "JOIN_S", "SW_LEAVE_S", "LEAVE_S", "???", "JOIN_B", "SW_LEAVE_B", "LEAVE_B"]
        self.fmtstr = \
            " (MUL_" + subtypeName[self.subtype] + ")" + \
            " AE: "   + str(self.ae) + \
            " LCID: " + "{:04X}".format(self.lcid) + \
            " TYPE: " + str(self.type) + \
            " DLEN: " + str(self.dlen)
        if self.dlen > 0:
            self.fmtstr += " DATA: " + "".join("{:02X}".format(c) for c in self.data)
    
    def isJoinS(self):
        return (self.subtype == 1)
    
    def isSwLeaveS(self):
        return (self.subtype == 2)
    
    def isLeaveS(self):
        return (self.subtype == 3)
    
    def isJoinB(self):
        return (self.subtype == 5)
    
    def isSwLeaveB(self):
        return (self.subtype == 6)
    
    def isLeaveB(self):
        return (self.subtype == 7)

class Sec:
    def __init__(self, pdu, do=0):
        # fields
        offset = 0
        keystring = ""
        self.key        = (pdu[offset] & 0xC0) >> 6
        self.update_key = (pdu[offset] & 0x30) >> 4
        self.wk = []
        self.swk = []
        if (do==1): # is SEC_REQ
            if (self.key == KeyTypes.WK) or (self.key == KeyTypes.WK_SWK): # only WK key present or both keys present
                self.wk =  pdu[(offset + 1) : (offset + 1 + 24)]
                keystring += " WK: " + "".join("{:02X}".format(c) for c in self.wk)
                if (self.key == KeyTypes.WK_SWK): # both keys present
                    self.swk =  pdu[(offset + 25) : (offset + 25 + 24)]
                    keystring += " SWK: " + "".join("{:02X}".format(c) for c in self.swk)
            if (self.key == KeyTypes.SWK): # only SWK key present
                self.swk =  pdu[(offset + 1) : (offset + 1 + 24)]
                keystring += " SWK: " + "".join("{:02X}".format(c) for c in self.swk)
                
        # additional info
        self.subtype = do 
        subtypeName = ["RSP", "REQ"]
        self.fmtstr = \
            " (SEC_" + subtypeName[self.subtype] + ")" + \
            " KEY: "   + str(self.key) + \
            " UPDATE_KEY: " + str(self.update_key) + \
            keystring
            
    
    def isRsp(self):
        return (self.subtype == 0)
    
    def isReq(self):
        return (self.subtype == 1)

class SecData:
    def __init__(self, pdu):
        # fields
        offset = 0

        self.enc        = (pdu[offset]) >> 7
        self.key        = (pdu[offset] & 0x70) >> 4
        self.lnid_p     = (pdu[offset] & 0x08) >> 3
        self.cnt        =  pdu[offset+4] + (pdu[offset+3] << 8) + (pdu[offset+2] << 16) + (pdu[offset+1] << 24)
        if self.lnid_p == 1:
            self.lnid       = ((pdu[offset + 5] & 0x3F) << 8) + pdu[offset + 6]       
            self.hdrlen = offset + 7
        else:
            self.hdrlen = offset + 5	
        # additional info
        self.fmtstr = " (PSH)" + \
            " ENC: " + str(self.enc) + \
            " KEY: " + str(self.key) + \
            " LNID_P: " + str(self.lnid_p) + \
            " CNT: " + str(self.cnt)
        if self.lnid_p == 1:
            self.fmtstr += " LNID: " + "{:04X}".format(self.lnid)

class ArqData():
    def __init__(self, pdu):
        # fields
        offset = 0
        self.flush   = (pdu[offset] & 0x40) >> 6
        self.pktid   = pdu[offset] & 0x3F
        self.nackid  = []
        self.winsize = None
        self.ackid = None
        offset += 1
        while (offset < (len(pdu) - PduLengths.CRC_LEN - 1)):
            if ((pdu[offset] & 0x80) != 0): # if ARQ.M == 1
                if ((pdu[offset] & 0x40) != 0):
                    self.nackid += [pdu[offset] & 0x3F]
                else:
                    self.winsize = pdu[offset] & 0x1F
                offset += 1
            else:
                break
        self.ackid = pdu[offset] & 0x3F
        self.hdrlen = offset + 1
        
        # additional info
        self.fmtstr = " (ARQ)" + \
            " PKTID: " + str(self.pktid) + \
            " FLUSH: " + str(self.flush)
        if self.winsize is not None:
            self.fmtstr += " WINSIZE: " + str(self.winsize)
        for nackid in self.nackid:
            self.fmtstr += " NACKID: " + str(nackid)
        if self.ackid is not None:
            self.fmtstr += " ACKID: " + str(self.ackid)
        
class TrefData():
    def __init__(self, pdu):
        # fields
        offset = 0
        self.seq  = pdu[offset] >> 3
        self.time = pdu[offset+4] + (pdu[offset+3] << 8) + (pdu[offset+2] << 16) + (pdu[offset+1] << 24)
        self.hdrlen = offset + 5
        
        time_str = ""
        if self.time == 0x80000000:
            time_str = "<INV>"
        else:
            if self.time & 0x80000000:
                self.time = -0x100000000 + self.time
            time_str = str(self.time)
        
        # additional info
        self.fmtstr = " (TREF)" + \
            " SEQ: " + str(self.seq) + \
            " TIME: " + time_str

            
class Packet():
    def __init__(self, packet, sna=None):
        self.raw = packet
        self.ht = HTypes.PHY
        if "PRIME IS A WONDERFUL TECHNOLOGY" not in "".join(map(chr, packet)):
            self.ht = (packet[0] & 0x30) >> 4
        try:
            if self.ht == HTypes.BPDU:
                self._bpdu = BeaconPdu(packet)
            elif self.ht == HTypes.PNPDU:
                self._pnpdu = PromotionNeededPdu(packet)
            elif self.ht == HTypes.GPDU:
                chunks = {}
                chunks['ghdr'] = GenericHeader(packet, sna)
                chunks['phdr'] = PacketHeader(packet, sna)
                
                # Parsing sub-headers
                offset = PduLengths.GENHDR_LEN + PduLengths.PKTHDR_LEN
                self._payloadEnd  = offset + chunks['phdr'].len
                
                pduminlenOffset = 0
                if chunks['phdr'].psh:
                    chunks['secdata'] = SecData(packet[offset:])
                    offset += chunks['secdata'].hdrlen
                    newpacket = packet[:self._payloadEnd] + packet[(self._payloadEnd + PduLengths.AUTH_TAG_SIZE): (self._payloadEnd + PduLengths.AUTH_TAG_SIZE + PduLengths.AUTH_TAG_SIZE + 1)]
                    packet = newpacket
                    pduminlenOffset = chunks['secdata'].hdrlen
                    
                if chunks['phdr'].tref:
                    chunks['trefdata'] = TrefData(packet[offset:])
                    offset += chunks['trefdata'].hdrlen
                    
                if chunks['phdr'].arq:
                    chunks['arqdata'] = ArqData(packet[offset:])
                    offset += chunks['arqdata'].hdrlen
                    
                    
                self._payloadOffset = offset

                # Parsing control packets
                if chunks['phdr'].c:
                    if chunks['phdr'].ctype == CTypes.REG:
                        chunks['reg'] = Reg(packet[offset:], do=chunks['ghdr'].do, lnid=chunks['phdr'].lnid)
                    elif chunks['phdr'].ctype == CTypes.CON:
                        chunks['con'] = Con(packet[offset:], do=chunks['ghdr'].do)
                    elif chunks['phdr'].ctype == CTypes.PRO:
                        chunks['pro'] = Pro(packet[offset:], (chunks['phdr'].len - pduminlenOffset) > PduLengths.PRO_MIN_LEN, do=chunks['ghdr'].do)
                    elif chunks['phdr'].ctype == CTypes.FRA:
                        chunks['fra'] = Fra(packet[offset:])
                    elif chunks['phdr'].ctype == CTypes.CFP:
                        chunks['cfp'] = Cfp(packet[offset:], do=chunks['ghdr'].do)
                    elif chunks['phdr'].ctype == CTypes.ALV:
                        chunks['alv'] = Alv(packet[offset:], (chunks['phdr'].len - pduminlenOffset)!= PduLengths.ALV_MIN_LEN, do=chunks['ghdr'].do)
                    elif chunks['phdr'].ctype == CTypes.MUL:
                        chunks['mul'] = Mul(packet[offset:], do=chunks['ghdr'].do, lnid=chunks['phdr'].lnid)
                    elif chunks['phdr'].ctype == CTypes.SEC:
                        chunks['sec'] = Sec(packet[offset:], do=chunks['ghdr'].do)
                else: #parse DATA payload (different from ARQ)
                    if (packet[offset + 3] == 0x50 ): #dfu_rsp parsin
                        self._dfursp = Dfu_Rsp(packet[(offset + 3):])
                # Completing the packet with previously parsed chunks
                self._ghdr = chunks['ghdr']
                self._phdr = chunks['phdr']
                
                if 'secdata' in chunks:
                    self._secdata = chunks['secdata']
                if 'trefdata' in chunks:
                    self._trefdata = chunks['trefdata']
                if 'arqdata' in chunks:
                    self._arqdata = chunks['arqdata']
                    
                if 'reg' in chunks:
                    self._reg = chunks['reg']
                elif 'con' in chunks:
                    self._con = chunks['con']
                elif 'pro' in chunks:
                    self._pro = chunks['pro']
                elif 'fra' in chunks:
                    self._fra = chunks['fra']
                elif 'cfp' in chunks:
                    self._cfp = chunks['cfp']
                elif 'alv' in chunks:
                    self._alv = chunks['alv']
                elif 'mul' in chunks:
                    self._mul = chunks['mul']
                elif 'sec' in chunks:
                    self._sec = chunks['sec']
                
            else:
                self.ht = HTypes.PHY
        except: 
            self.ht = HTypes.PHY
    
    def bpdu(self):
        try:
            return self._bpdu
        except:
            return None
    
    def pnpdu(self):
        try:
            return self._pnpdu
        except:
            return None
    
    def ghdr(self):
        try:
            return self._ghdr
        except:
            return None
    
    def phdr(self):
        try:
            return self._phdr
        except:
            return None

    def payloadStartOffset(self):
        return self._payloadOffset
    
    def payloadEndOffset(self):
        return self._payloadEnd
        
    def reg(self):
        try:
            return self._reg
        except:
            return None
    
    def con(self):
        try:
            return self._con
        except:
            return None
    
    def pro(self):
        try:
            return self._pro
        except:
            return None
    
    def fra(self):
        try:
            return self._fra
        except:
            return None
    
    def cfp(self):
        try:
            return self._cfp
        except:
            return None
    
    def alv(self):
        try:
            return self._alv
        except:
            return None
    
    def mul(self):
        try:
            return self._mul
        except:
            return None
    
    def sec(self):
        try:
            return self._sec
        except:
            return None
    
    def secdata(self):
        try:
            return self._secdata
        except:
            return None
    
    def arqdata(self):
        try:
            return self._arqdata
        except:
            return None
    
    def trefdata(self):
        try:
            return self._trefdata
        except:
            return None
    
    def dfursp(self):
        try:
            return self._dfursp
        except:
            return None
            
    def phy(self):
        if self.isPhy:
            return self.raw
        return None
    
    def isBpdu(self):
        return (self.ht == HTypes.BPDU)
    
    def isPnpdu(self):
        return (self.ht == HTypes.PNPDU)
    
    def isGpdu(self):
        return (self.ht == HTypes.GPDU)
    
    def isPhy(self):
        return (self.ht == HTypes.PHY)
        
    def isControl(self):
        try:
            return (self._phdr.c != 0)
        except:
            return False
    
    def isReg(self):
        try:
            return (self._phdr.ctype == CTypes.REG)
        except:
            return False
    
    def isCon(self):
        try:
            return (self._phdr.ctype == CTypes.CON)
        except:
            return False
    
    def isPro(self):
        try:
            return (self._phdr.ctype == CTypes.PRO)
        except:
            return False
    
    def isFra(self):
        try:
            return (self._phdr.ctype == CTypes.FRA)
        except:
            return False
    
    def isCfp(self):
        try:
            return (self._phdr.ctype == CTypes.CFP)
        except:
            return False
    
    def isAlv(self):
        try:
            return (self._phdr.ctype == CTypes.ALV)
        except:
            return False
    
    def isMul(self):
        try:
            return (self._phdr.ctype == CTypes.MUL)
        except:
            return False
    
    def isSec(self):
        try:
            return (self._phdr.ctype == CTypes.SEC)
        except:
            return False
    
    def hasArqData(self):
        try:
            return (self._phdr.arq != 0)
        except:
            return False
    
    def hasTrefData(self):
        try:
            return (self._phdr.tref != 0)
        except:
            return False
    
    def hasSecData(self):
        try:
            return (self._phdr.psh != 0)
        except:
            return False
			
    def isDfuRsp(self):
        try:
            return ( self.raw[self._payloadOffset + 3 ] == 0x50)
            print("isDfuRsp")
        except:
            return False
    
    def length(self):
        if self.isBpdu():
            return PduLengths.BPDU_LEN
        if self.isPnpdu():
            return PduLengths.PNPDU_LEN
        if self.isGpdu():
            if self.hasSecData():
                return (PduLengths.PKTOVHD_LEN + self._phdr.len + PduLengths.AUTH_TAG_SIZE)
            else:
                return (PduLengths.PKTOVHD_LEN + self._phdr.len)
        return len(self.raw)
    
    def rxError(self):
        if self.isBpdu():
            return self._bpdu.crcerr
        if self.isPnpdu():
            return self._pnpdu.hcserr
        if self.isGpdu():
            return (self._ghdr.hcserr or self._phdr.crcerr)
        return False
    
    # Returns the packet transmission time in 10us, given the modulation 
    # (value shall be in protocol.Modulation)
    def calcTxDuration(self, modulation):
        N_CH                    = 1
        DURATION_COMP_BITS      = 20
        
        MPDU1_BYTE_SIZE_TYPE_A  = ((N_CH * 84 - 30) + 2) >> 3
        MPDU1_BYTE_SIZE_TYPE_B  = ((N_CH - 1) * 84) >> 4
        MPDU1_BYTE_SIZE_TYPE_BC = MPDU1_BYTE_SIZE_TYPE_B
        PHY_PRE_DURATION_A      = int(2048 / 10 + 1)
        PHY_PRE_DURATION_B      = int(8192 / 10 + 1)
        PHY_PRE_DURATION_BC     = 1024
        
        durComp   = [(-1 if i==0 else int((1<<DURATION_COMP_BITS)/i)) for i in [12, 24, 36, 0, 6, 12, 18, 0, 0, 0, 0, 0, 6, 12, 0, 0]]
        mpdu1Size = [MPDU1_BYTE_SIZE_TYPE_A, MPDU1_BYTE_SIZE_TYPE_B, MPDU1_BYTE_SIZE_TYPE_BC]
        phyHdrLen = [2, 4, 6]
        phyPreDur = [PHY_PRE_DURATION_A, PHY_PRE_DURATION_B, PHY_PRE_DURATION_BC]
        
        tx_sch = modulation & 0xF
        tx_type = (modulation >> 4) & 0xF
        
        if tx_type > 3:
            return -1
        
        if durComp[tx_sch] < 0:
            return -1
        
        # Computing MPDU2
        mpdu2_len = self.length() - mpdu1Size[tx_type]

        # Adding FLUSHING_P
        mpdu2_len += 1
    
        # Computing symbols
        duration = (durComp[tx_sch]*mpdu2_len + ((1<<DURATION_COMP_BITS)-1)) >> DURATION_COMP_BITS;

        # Repetition
        if tx_sch == Modulation.DBPSKRC or tx_sch == Modulation.DQPSKRC:
            duration <<= 2

        # Adding PHY Header
        duration += phyHdrLen[tx_type]
        
        # Converting in 10us
        duration *= TimeConst.SYMBOL_TO_10US
        
        # Adding PHY preamble
        duration += phyPreDur[tx_type]
        
        return duration
    
    def getTypeString(self):
        if self.isBpdu(): 
            return ("BCN" if (self._bpdu.sid == Addresses.LSID_BASENODE) else "BCN_S")    
        if self.isPnpdu():
            return "PNPDU"
        if self.isGpdu():
            if not self.isControl():
                return "DATA"
            if self.isReg():
                return "REG"
            if self.isCon():
                return "CON"
            if self.isPro():
                return "PRO"
            if self.isFra():
                return "FRA"
            if self.isCfp():
                return "CFP"
            if self.isAlv():
                return "ALV"
            if self.isMul():
                return "MUL"
            if self.isSec():
                return "SEC"
            return "UNKNOWN"
        return "PHY"
    
    def getDoString(self):
        if self.isGpdu():
            return ("UP" if (self._ghdr.do == 0) else "DW")
        return "-"
    
    def getLevelString(self):
        if self.isBpdu():
            return "{0:02d}".format(self._bpdu.level)
        if self.isGpdu():
            return "{0:02d}".format(self._ghdr.level)
        return "-"
    
    def getHcsString(self):
        if self.isPnpdu():
            return ("ERR" if self._pnpdu.hcserr else "OK")
        if self.isGpdu():
            return ("ERR" if self._ghdr.hcserr else "OK")
        return "-"
    
    def getRmString(self):
        if self.isGpdu():
            return str(self._phdr.rm)
        return "-"
    
    def getSidString(self):
        if self.isBpdu():
            return "{:02X}".format(self._bpdu.sid)
        if self.isGpdu():
            return "{:02X}".format(self._phdr.sid)
        return "-"
    
    def getLnidString(self):
        if self.isGpdu():
            return "{:04X}".format(self._phdr.lnid)
        return "-"
    
    def getPrioString(self):
        if self.isGpdu():
            return str(self._phdr.prio + 1)
        return "-"
    
    def getLenString(self):
        if self.isBpdu() or self.isPnpdu():
            return "-"
        if self.isGpdu():
            return str(self._phdr.len)
        return str(len(self.raw))
    
    def getCrcString(self):
        if self.isBpdu():
            return ("ERR" if self._bpdu.crcerr else "OK")
        if self.isGpdu():
            return ("ERR" if self._phdr.crcerr else "OK")
        return "-"
    
    def getFormatString(self):
    
        retStr = ""
        
        if self.isBpdu():
            retStr = self._bpdu.fmtstr
        elif self.isPnpdu():
            retStr = self._pnpdu.fmtstr
        elif self.isGpdu():
            if self.hasSecData():
                retStr += self._secdata.fmtstr
            if self.hasTrefData():
                retStr += self._trefdata.fmtstr
            if self.hasArqData():
                retStr += self._arqdata.fmtstr
            
            if self.isControl():
                if self.isReg():
                    retStr += self._reg.fmtstr
                elif self.isCon():
                    retStr += self._con.fmtstr
                elif self.isPro():                
                    retStr += self._pro.fmtstr
                elif self.isFra():
                    retStr += self._fra.fmtstr
                elif self.isCfp():
                    retStr += self._cfp.fmtstr
                elif self.isAlv():
                    retStr += self._alv.fmtstr
                elif self.isMul():
                    retStr += self._mul.fmtstr
                elif self.isSec():
                    retStr += self._sec.fmtstr
            else: # data packet
                retStr += " RawData: " + \
                    "".join("{:02X}".format(c)
                            for c in self.raw[self._payloadOffset:-(PduLengths.CRC_LEN)])
        else:
            retStr = " RawData: " + "".join("{:02X}".format(c) for c in self.raw) 
        
        return retStr

    def getFieldsToStrings(self):
        return (self.getTypeString(), 
                self.getDoString(), 
                self.getLevelString(),
                self.getHcsString(),
                self.getRmString(),
                self.getSidString(),
                self.getLnidString(),
                self.getPrioString(),
                self.getLenString(),
                self.getCrcString(),
                self.getFormatString())
