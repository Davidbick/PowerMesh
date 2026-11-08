import sys
import time
from abc import ABCMeta, abstractmethod
from protocol import SpecPibs, CommonVendSpecPibs, Modulation
from global_prime_defs import Vendor
#from __builtin__ import True uncomment for python2

class ServiceNodeAbstract():
    __metaclass__ = ABCMeta

    class Pibs(SpecPibs, CommonVendSpecPibs):
        ''' SN-specific vendor specific attributes '''
        macZeroCrossConfig            = 0x8014
        macPduReqQueueCtrlFullEvents  = 0x8017
        macPduReqQueueDataFullEvents  = 0x8018
        phyCurrentControlMaxThreshold = 0x8019
        phyCurrentControlSpikesFilter = 0x8021
        phyCurrentControlStatus       = 0x8022
        sarSegmentationLength         = 0x8030
        macNonRoboPnpduProbability    = 0x8035
        macAlvMode                    = 0x8050

    class PrimeConfig:
        FULL =      0
        PHY =       1
        APPEMU =    2

    class TestMode:
        TmReset              = 0x00000000
        TnOnly               = 0x00000001   # Forces the node to stay into Terminal state
        ForceLinkModeNonRobo = 0x00000002   # Forces the node to filter out received robust PDUs and
                                            # transmit PNPDUs with CAP_R=0 (robust mode not supported)
        ForceLinkModeRobo    = 0x00000004   # Forces the node to filter out received non-robust PDUs
        DisableMcastFilter   = 0x00000008   # Disable filtering of incoming multicast packet duplicates
        ForceAppEmuArq       = 0x00000020
        RxDataLoss           = 0x00000040
        RegReqCapArqDisable  = 0x00000080   # Set to 0 REG.CAP_ARQ bit (needed to BN cert test)
        RegSwitchLvl1        = 0x00000100   # Register under a first level switch
        RegSwitchLvl2        = 0x00000200   # Register under a second level switch
        FuCrcErr             = 0x00000400
        RMIConRej            = 0x00000800   # Reject a BN request connection
        SarCrcErr            = 0x00001000   # Corrupt the CRC in the last segment of a segmented message,
                                            # with SAR type 0b11
        AlvPhyErr            = 0x00004000
        PhyErr               = 0x00008000   # Corrupt a packet simulating a physical error (not used for ALV packets) ###
        ArqCong              = 0x00010000
        AddTref              = 0x00020000
        ForceMaxEst          = 0x00040000
        
        ForceCapPa           = 0x00080000   # Force REG_REQ.CAP_PA bit to 1
        AuthOnly             = 0x00200000   # when a packet need to be auth and encr/decr is instead only auth
        UseOldKeys           = 0x01000000   #  not increment the SecTxPkt counters before filling PSH_CNT
        UseOldKeys           = 0x02000000   #  not update WK when it receive a SEC with WK
        UseOldKeys           = 0x08000000   #  use old keys to tx next packet
         
    class RfuEvents:
        FU_NO_EVENT     = 0x00
        FU_START        = 0x01
        FU_DATA         = 0x02
        FU_UPGRADE      = 0x03
        FU_CONFIRM      = 0x04
        FU_KILL         = 0x05
        FU_COMPLETE     = 0x06
        STORAGE_READY   = 0x07
        DATA_BW         = 0x08
            
    class RfuStates:
        IDLE            = "IDLE"
        INIT_STORAGE    = "INIT_STORAGE"
        SAVE_PAGE       = "SAVE_PAGE"
        WAIT_FU_CONFIRM = "WAIT_FU_CONFIRM"
       
    
    class Indication:
        CL_432_ESTABLISHCONFIRM      = 0x00000001
        CL_432_RELEASECONFIRM        = 0x00000002
        CL_432_REPLYIINDICATION      = 0x00000004
        CL_432_UPDREPLYCONFIRM       = 0x00000008
        CL_432_DATAINDICATION        = 0x00000010
        CL_432_DATACONFIRM           = 0x00000020
        MAC_REGISTERSTATEINDICATION  = 0x00000040
        MAC_ESTABLISHCONFIRM         = 0x00000080
        MAC_ESTABLISHINDICATION      = 0x00000100
        MAC_RELEASECONFIRM           = 0x00000200
        MAC_RELEASEINDICATION        = 0x00000400
        MAC_JOINCONFIRM              = 0x00000800
        MAC_LEAVECONFIRM             = 0x00001000
        MAC_DATACONFIRM              = 0x00002000
        MAC_DATAINDICATION           = 0x00004000
        MAC_JOININDICATION           = 0x00008000
        MAC_LEAVEINDICATION          = 0x00010000
        RFU_EVENTINDICATION          = 0x00020000
        PHY_DATAINDICATION           = 0x80000000
        PHY_DATACONFIRM              = 0x40000000


        strings = { CL_432_ESTABLISHCONFIRM    : "  CL_432_ESTABLISHConfirm",
                    CL_432_RELEASECONFIRM      : "  CL_432_RELEASEConfirm",
                    CL_432_REPLYIINDICATION    : "  CL_432_REPLYIIndication",
                    CL_432_UPDREPLYCONFIRM     : "  CL_432_UPDREPLYConfirm",
                    CL_432_DATAINDICATION      : "  CL_432_DATAIndication",
                    CL_432_DATACONFIRM         : "  CL_432_DATAConfirm",
                    MAC_REGISTERSTATEINDICATION: "  MAC_REGISTERSTATEIndication",
                    MAC_ESTABLISHCONFIRM       : "  MAC_ESTABLISHConfirm",
                    MAC_ESTABLISHINDICATION    : "  MAC_ESTABLISHIndication",
                    MAC_RELEASECONFIRM         : "  MAC_RELEASEConfirm",
                    MAC_RELEASEINDICATION      : "  MAC_RELEASEIndication",
                    MAC_JOINCONFIRM            : "  MAC_JOINConfirm",
                    MAC_LEAVECONFIRM           : "  MAC_LEAVEConfirm",
                    MAC_DATACONFIRM            : "  MAC_DATAConfirm",
                    MAC_DATAINDICATION         : "  MAC_DATAIndication",
                    MAC_JOININDICATION         : "  MAC_JOINIndication",
                    MAC_LEAVEINDICATION        : "  MAC_LEAVEIndication",
                    PHY_DATAINDICATION         : "  PHY_DATAIndication",
                    PHY_DATACONFIRM            : "  PHY_DATAConfirm" }

    @abstractmethod
    def NodeInfo(self):
        pass

    @abstractmethod
    def GetPib(self, pib, iface="SCI", endianness=0):
        pass

    @abstractmethod
    def SetPib(self, pib, value, iface="SCI"):
        pass

    @abstractmethod
    def GetPibList(self, pib, iface="SCI", endianness=0):
        pass

    @abstractmethod
    def SetTestMode(self, inTestModeMask):
        pass
    
    @abstractmethod
    def ClearTestMode(self, inTestModeMask):
        pass
    
    @abstractmethod
    def InitTestMode(self, inTestModeMask=None):
        pass
    
    @abstractmethod
    def GetTestMode(self):
        pass
    
    @abstractmethod
    def GetDefTestMode(self):
        pass
    
    @abstractmethod
    def SwReset(self):
        pass

    @abstractmethod
    def SerialComClose(self):
        pass

    @abstractmethod
    def SerialComOpen(self):
        pass

    @abstractmethod
    def GetState(self):
        pass

    @abstractmethod
    def InitiatePromotion(self):
        pass

    @abstractmethod
    def InitiateDemotion(self):
        pass

    @abstractmethod
    def InitiateUnregister(self):
        pass

    @abstractmethod
    def StartManagingIndications(self, answer=0):
        pass

    @abstractmethod
    def StopManagingIndications(self):
        pass
        
    @abstractmethod
    def StartSNDataLoop(self,ch):
        pass

    @abstractmethod
    def StopSNDataLoop(self):
        pass    

    @abstractmethod
    def CL432Establish(self, SerNum):
        pass

    @abstractmethod
    def CL432Release(self, DestAddr):
        pass

    @abstractmethod
    def CL432GetData(self, dst_addr, src_addr):
        pass

    @abstractmethod
    def CL432SendData(self, dst_addr, src_addr, data):
        pass

    @abstractmethod
    def CL432GetSNSession(self):
        pass

    @abstractmethod
    def CL_NULL_Establish(self, eui=[0, 0, 0, 0, 0, 0], typ=0, arq=False, data=[]):
        pass

    @abstractmethod
    def CL_NULL_Release(self, ch):
        pass

    # According to spec, shall return connection handle and data
    @abstractmethod
    def CL_NULL_GetData(self):
        pass

    @abstractmethod
    def CL_NULL_SendData(self, ch, buff, prio=1):
        pass

    @abstractmethod
    def CL_NULL_RMISendData(self, ch, buff, prio=1):
        pass

    @abstractmethod
    def CL_NULL_Join(self, bcast, eui, data=[]):
        pass

    @abstractmethod
    def CL_NULL_Leave(self, ch, eui48):
        pass

    @abstractmethod
    def PHY_GetData(self):
        pass

    @abstractmethod
    def PHY_SendData(self, data, level=7, scheme=Modulation.TYPEA_DBPSKCC, timer=0, t_sleep=0.015):
        pass

    @abstractmethod
    def IsUnregisterIndicationPresent(self):
        pass

    @abstractmethod
    def IsPromotionIndicationPresent(self):
        pass

    @abstractmethod
    def IsDemotionIndicationPresent(self):
        pass

    @abstractmethod
    def WaitClNullEstablishRequest (self, response=0, timeout=20):
        pass

    @abstractmethod
    def WaitClNullJoinIndication(self, response=0, timeout=20):
        pass

    def DeviceType(self, productID):
        if productID == Vendor.ProductId.SN_EMB:
            return "SN EMB"
        if productID == Vendor.ProductId.SN_STM32:
            return "SN STM32"
        if productID == Vendor.ProductId.SN_STCOMET:
            return "SN STCOMET 1.4 (lib Binary)"
        if productID == Vendor.ProductId.SN_STCOMET_LIBCOMPILED:
            return "SN STCOMET 1.4 (lib Compiled)"
        if productID == Vendor.ProductId.SN_STARCOM_LIBCOMPILED:
            return "SN ST8500 1.4"
        return "UNKNOWN"

    def Unregister(self):
        self.SetPib(SpecPibs.macActionUnregister, [0x01])

    def closeActiveConnection(self):
        self.SetPib(SpecPibs.macActionConnClose, [0x01])

    def SetPrimeConfigFull(self, initialTM=None, timeout=15):
        self.SetPib(self.Pibs.primeConfig, [self.PrimeConfig.FULL])
        time.sleep(1.5) # wait reset 
        if initialTM != None and not self.SetTestMode(initialTM | self.TestMode.AuthOnly):
            return False
        
        if initialTM == None and not self.SetTestMode(self.TestMode.AuthOnly):
            return False
        
        endtime = time.time() + timeout
        while time.time() <= endtime:
            if self.ProductID == Vendor.ProductId.SN_STARCOM_LIBCOMPILED:
                time.sleep(1.5)  # 0.5 for STCOMET, 1.5 sec for ST8500
            else:
                time.sleep(0.5) 
            if self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.FULL: 
                self.SetPib(self.Pibs.SecurityProfile, [0])
                return True
        return False
    
    def SetPrimeConfigPhy(self, timeout=15):
        self.SetPib(self.Pibs.primeConfig, [self.PrimeConfig.PHY])
        
        endtime = time.time() + timeout
        while time.time() <= endtime:
            time.sleep(0.5)
            if self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.PHY:
                return True
        
        return False

    def SetPrimeConfigAppemu(self, timeout=15):
        self.SetPib(self.Pibs.primeConfig, [self.PrimeConfig.APPEMU])
        time.sleep(0.5)
        endtime = time.time() + timeout
        while True:
            if self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.APPEMU:
                return True
            if time.time() > endtime:
                break
            time.sleep(0.5)
        return False

    def StartPhyRxTest(self):
        return self.SetPib(self.Pibs.phyStartRxTest, [1])
    
    def StartPhyTxTest(self, modulation=Modulation.TYPEA_DBPSKCC):
        if self.ProductID == Vendor.ProductId.SN_EMB and modulation != Modulation.TYPEA_DBPSKCC:
            return False # SN EMB cannot use other modulations than DBPSK+CC
        return self.SetPib(self.Pibs.phyStartTxTest, [modulation])

    def WaitPhyRxTestEnd(self, timeout=2000, threshold=0):
        wait_lenght = 5
        time.sleep(1)
        old = self.GetPib(SpecPibs.phyStatsRxTotalCount)
        for _ in range(0, int(timeout / wait_lenght)):
            for _ in range (0, wait_lenght):
                time.sleep(1)
                sys.stdout.write(".")
                sys.stdout.flush()
            sys.stdout.write(",")
            sys.stdout.flush()
            act = self.GetPib(SpecPibs.phyStatsRxTotalCount)
            if (act - old) <= threshold:
                sys.stdout.write("\n")
                return True
            old = act
        sys.stdout.write("\n")
        return False

    def waitEndDfuProcess(self, timeout=5000):
        endtime = time.time() + timeout
        while True:
            sys.stdout.write(".")
            sys.stdout.flush()
            if self.GetPib(SpecPibs.AppFwdlRunning) == 0:
                sys.stdout.write("\n")
                return True
            time.sleep(1)
            if time.time() > endtime:
                break
        sys.stdout.write("\n")
        return False

    def CL432WaitData(self, dst_addr, src_addr, timeout=20):
        endtime = time.time() + timeout
        while True:
            data = self.CL432GetData(dst_addr, src_addr)
            if data != []:
                return data
            if time.time() > endtime:
                break
        return []

    def CL_NULL_WaitData(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            ch, data = self.CL_NULL_GetData()
            if data != []:
                return ch, data
            if time.time() > endtime:
                break
        return 0, []

    def PHY_WaitData(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            data, lev, sch, tim, snr, rssi = self.PHY_GetData()
            if data != []:
                return data, lev, sch, tim, snr, rssi
            if time.time() > endtime:
                break
        return [], 0, 0, 0, 0, 0

    def Cl432SNSessionPrint(self, sn_s):
        j = 0
        leng = len(sn_s)
        line = []
        while j < leng:
            SNLen = sn_s[j]
            j += 1
            line += ["DAdd: " + str((sn_s[j] << 8) + (sn_s[j + 1]))]
            j += 2
            line += ["DSAP: " + str(sn_s[j])]
            j += 1
            line += ["SSAP: " + str(sn_s[j])]
            j += 1
            line += ["SNLen:  " + str(SNLen)]
            line += ["SerNum: " + ' '.join('{:02x}'.format(c) for c in sn_s[j:(j + SNLen)])]
            j += SNLen
        return line

    def PRIMEGetStatePrint(self, bfr):
        line = []
        line += ["STATE: " + str(format(bfr, '08x'))]
        if bfr != 0:
            for key in list(self.Indication.strings.keys()):
                if bfr & key != 0:
                    line += [str(self.Indication.strings[key])]
        return line
    
    def SendManagementDataBurst(self, eui48, number, datalen, dutycycle=None):
        params = []
        params += eui48
        params += [(number >> 24) & 0xFF, (number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF]
        params += [datalen]
        params += [dutycycle if dutycycle is not None else 100]
        return self.SetPib(SpecPibs.macActionMgmtDataBurst, params)
    
    #================== CL_IPv6 abstract methods ===============
    @abstractmethod
    def CL_IPv6_Establish(self):
        pass
    @abstractmethod
    def CL_IPv6_Release(self):
        pass
    @abstractmethod
    def CL_IPv6_GetData(self):
        pass
    @abstractmethod
    def CL_IPv6_WaitData(self, timeout=30):
        pass
    @abstractmethod
    def CL_IPv6_SendData(self, data, timeout=30):
        pass
    #===========================================================