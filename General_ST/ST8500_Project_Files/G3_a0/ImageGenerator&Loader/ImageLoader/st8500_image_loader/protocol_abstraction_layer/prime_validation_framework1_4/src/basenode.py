import os
import sys
from threading import Thread
import time
import _ctypes
import ctypes
from protocol import SpecPibs, CommonVendSpecPibs, DfuConst, MgmtTypes, Addresses, Modulation
from global_prime_defs import SignatureAlg

class BaseNode:

    InitFailure = True
    ManageIndications = False
    fsmthreadrunning = False
    perf_loop_test = False

    class TestMode:
        TmReset          = 0x00000000
        RegRej           = 0x00000001
        ProRej           = 0x00000002
        AlvStop          = 0x00000004
        BpiChange        = 0x00000008
        EnableDs         = 0x00000010
        DisableDs        = 0x00000020
        FraX2            = 0x00000040
        FraX4            = 0x00000080
        FraX1Cfp100      = 0x00000100
        FraX1CfpDef      = 0x00000200
        DfuHiSpeed       = 0x00000400
        ConRej           = 0x00000800
        RxDataLoss       = 0x00001000
        RmiConAllow      = 0x00002000
        AlvCrcErr        = 0x00004000
        PduCrcErr        = 0x00008000
        ArqCong          = 0x00010000
        DisablePnpduProc = 0x00020000
        SarCrcErr        = 0x00040000
        MaxBcnFreq       = 0x00080000
        FraX3            = 0x00100000
        FuFirstByteErr   = 0x00200000
        FuLastByteErr    = 0x00400000
        AuthOnly         = 0x00800000   # when a packet need to be auth and encr/decr is instead only auth
        TxPshCntNotUpd   = 0x01000000 
        UpdateKWandSWK   = 0x02000000
        WrapWithWrongKWK = 0x04000000
        UseOldKeys       = 0x08000000
        DataCrcErr       = 0x10000000
    
    class Pibs(SpecPibs, CommonVendSpecPibs):
        ''' BN-specific vendor specific attributes '''
        primeTM                    = 0x8009
        macTrefSeq                 = 0x8012
        macTrefTime                = 0x8013
        macZeroCrossConfig         = 0x8014
        phyCurrentControlMaxThreshold = 0x8019         
        macMgmtConWithARQ          = 0x8025
        macBeaconTxTimeShift       = 0x8036
        macSwitchBeaconTxFrequency = 0x8041
        SecurePolicy               = 0x8054 
        ALVPollingTime             = 0x8060
        ALVPollsPerNode            = 0x8061
        ALVMaxPathRetx             = 0x8064
        ALVMinLevel                = 0x8065
        ALVPollType                = 0x8066
        ALVRepSampleWeight         = 0x8067
        PROWindowSize              = 0x8068
        PRODemoteInactiveDelay     = 0x8069
        PROMinReliability          = 0x806A
        PRODeafWindowSize          = 0x806B
        PROMaxParallelQueues       = 0x806C
        ClIpv6UcastDefARQ          = 0x8080
    
    class PrimeConfig:
        FULL =      0
        PHY =       1
        APPEMU =    2

    class DfuModes:
        mixed =     0
        multicast = 1
        unicast =   2

    class Indication:
        CL_432_JOININDICATION       = 0x00000001
        CL_432_LEAVEINDICATION      = 0x00000002
        CL_432_REPLYCONFIRM         = 0x00000004
        CL_432_UPDREPLYINDICATION   = 0x00000008
        CL_432_DATAINDICATION       = 0x00000010
        CL_432_DATACONFIRM          = 0x00000020
        CL_NULL_ESTABLISHCONFIRM    = 0x00000080
        CL_NULL_ESTABLISHINDICATION = 0x00000100
        CL_NULL_RELEASECONFIRM      = 0x00000200
        CL_NULL_RELEASEINDICATION   = 0x00000400
        CL_NULL_JOINCONFIRM         = 0x00000800
        CL_NULL_LEAVECONFIRM        = 0x00001000
        CL_NULL_DATACONFIRM         = 0x00002000
        CL_NULL_DATAINDICATION      = 0x00004000
        CL_NULL_JOININDICATION      = 0x00008000
        CL_NULL_LEAVEINDICATION     = 0x00010000
        PHY_DATAINDICATION          = 0x80000000
        PHY_DATACONFIRM             = 0x40000000

        strings = { CL_432_JOININDICATION      : "  CL_432_JOINIndication",
                    CL_432_LEAVEINDICATION     : "  CL_432_LEAVEIndication",
                    CL_432_REPLYCONFIRM        : "  CL_432_REPLYConfirm",
                    CL_432_UPDREPLYINDICATION  : "  CL_432_UPDREPLYIndication",
                    CL_432_DATAINDICATION      : "  CL_432_DATAIndication",
                    CL_432_DATACONFIRM         : "  CL_432_DATAConfirm",
                    CL_NULL_ESTABLISHCONFIRM   : "  CL_NULL_ESTABLISHConfirm",
                    CL_NULL_ESTABLISHINDICATION: "  CL_NULL_ESTABLISHIndication",
                    CL_NULL_RELEASECONFIRM     : "  CL_NULL_RELEASEConfirm",
                    CL_NULL_RELEASEINDICATION  : "  CL_NULL_RELEASEIndication",
                    CL_NULL_JOINCONFIRM        : "  CL_NULL_JOINConfirm",
                    CL_NULL_LEAVECONFIRM       : "  CL_NULL_LEAVEConfirm",
                    CL_NULL_DATACONFIRM        : "  CL_NULL_DATAConfirm",
                    CL_NULL_DATAINDICATION     : "  CL_NULL_DATAIndication",
                    CL_NULL_JOININDICATION     : "  CL_NULL_JOINIndication",
                    CL_NULL_LEAVEINDICATION    : "  CL_NULL_LEAVEIndication",
                    PHY_DATAINDICATION         : "  PHY_DATAIndication",
                    PHY_DATACONFIRM            : "  PHY_DATAConfirm" }
                    
    def __init__(self, comport, baud=57600, start=0, PollingForIndication=False):
        self.InitFailure = True
        
        # Parameter to enable/disable GetPib/SetPib overriding with LocalManagement's homonyn methods
        # NOTE that this parameter's setup MUST be here (because of the SetPrimeConfigPHY()
        #      called later on in this method.
        self.pibAccessLocalMngm = False

        try:
            self.dllwrp = BnDllWrapper(comport)
        except Exception as inst:
            print(f"Operation failed: {type(inst).__name__}")
            sys.stderr.write("BN library not present\n")
            time.sleep(7) # fake init time
            return
            
        if PollingForIndication:
            def IsCTSActive():
                return 0
            self.pvSetCallbackPointer(IsCTSActive)
        
        if self.dllwrp.UartInit(comport) != 0:
            #print("BN UART open error")
            time.sleep(7) # fake init time
            return
        self.InitFailure = False
        self.LocalMgmt = LocalManagement(self.dllwrp)
        self.dllwrp.PRIME_LIB_Init(2)
        
        if start:
            self.Start()
            time.sleep(1)  
            self.SetPrimeConfigPHY()
        self.defTestModeMask = self.TestMode.TmReset
        self.currTestModeMask = self.TestMode.TmReset

    def CloseDll(self):
        time.sleep(1)
        #print ("Close")
        if hasattr(self, 'dllwrp'):
            #print ("PRIME_LIB_Stop")
            self.dllwrp.PRIME_LIB_Stop()
            time.sleep(1)
            #print ("UartClose")
            self.dllwrp.UartClose()
            time.sleep(1)
            #print ("Unloadcdll")
            self.dllwrp.Unloadcdll()
            #del self.dllwrp

    def FSMThread(self):
        MANAGED_INDICATIONS = (self.Indication.CL_NULL_ESTABLISHINDICATION |
                               self.Indication.CL_NULL_RELEASEINDICATION |
                               self.Indication.CL_NULL_JOININDICATION |
                               self.Indication.CL_NULL_LEAVEINDICATION )
        if self.perf_loop_test:
            MANAGED_INDICATIONS |= self.Indication.CL_432_DATAINDICATION
        while self.fsmthreadrunning:
            if self.dllwrp.PRIME_LIB_FsmEvolve() == 0:
                # manage indications when library is not busy
                if self.ManageIndications:
                    state = self.GetState() & MANAGED_INDICATIONS
                    if state != 0:
                        if state & self.Indication.CL_NULL_ESTABLISHINDICATION:
                            self.ManageClNullEstablishIndication()
                        if state & self.Indication.CL_NULL_RELEASEINDICATION:
                            self.ManageClNullReleaseIndication()
                        if state & self.Indication.CL_NULL_JOININDICATION:
                            self.ManageClNullJoinIndication()
                        if state & self.Indication.CL_NULL_LEAVEINDICATION:
                            self.ManageClNullLeaveIndication()
                        if state & self.Indication.CL_432_DATAINDICATION:
                            ret, _, _, dadr, sadr, _, data = self.dllwrp.DL_DATAindication()
                        continue
                time.sleep(0.015)

    def Start(self, defaultFraming=False, useLocalManagement=False):
        if not self.fsmthreadrunning:
            self.fsmthreadrunning = True
            self.fsmthread = Thread(target=self.FSMThread)
            self.fsmthread.start()
            time.sleep(10) # heuristic sleep to wait end of RT fw upgrade
        self.InitTestMode(self.TestMode.DisablePnpduProc | self.TestMode.FraX1CfpDef | self.TestMode.MaxBcnFreq | self.TestMode.AuthOnly)
        
        if defaultFraming :
            self.ClearTestMode(self.TestMode.FraX1CfpDef)
        # keep BN from processing PNPDUs and disable double switching, and
        # set the frame length to 276 symbols by default, and
        # set the frequency of beacon tx for BN and switches to the maximum (every frame)
        
        self.pibAccessLocalMngm = useLocalManagement
        self.SetPrimeConfigAPPEMU()

    def Stop(self):
        #print ("stop fsm thread")
        if self.fsmthreadrunning:
            self.fsmthreadrunning = False
            self.fsmthread.join()
            del self.fsmthread
            
    def NodeInfo(self):
        fwver = self.GetPib(self.Pibs.AppFwVersion)
        eui48 = self.GetPib(self.Pibs.macEUI48)
        txt = ":".join("{:02X}".format(c) for c in eui48) + ", "
        txt += "BN 1.4 DLL "
        txt += "".join(chr(c) for c in fwver).strip("\0")
        return txt

    def isConnectionActive(self,eui48):
        lis = self.GetPibList(self.Pibs.macListActiveConn)
        for entry in lis:
            if eui48 == entry[5:11]:
                return True
        return False

    def isNodeSwitch(self,eui48):
        lis = self.GetPibList(self.Pibs.macListRegDevices)
        for entry in lis:
            if eui48 == entry[0:6]:
                if entry[9] != Addresses.LSID_UNASSIGNED:
                    return True
        return False

    def isNodeRegistered(self,eui48):
        lis = self.GetPibList(self.Pibs.macListRegDevices)
        for entry in lis:
            if eui48 == entry[0:6]:
                return True
        return False
    
    def GetPib(self, pib):
        if self.pibAccessLocalMngm:
            return self.LocalMgmt.GetPib(pib)
        
        if self.dllwrp.MAC_MLME_GETrequest(pib) == 0:
            while True:
                ret, status, _, arr_val = self.dllwrp.MAC_MLME_GETconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if status == 0:
                if len(arr_val) == 1:
                    val = arr_val[0] # cast to uint8_t
                elif len(arr_val) == 2:
                    val = (arr_val[0] << 8) + arr_val[1]  # cast to uint16_t
                elif len(arr_val) == 4:
                    if (pib != 0x8015):
                        val = (arr_val[0] << 24) + (arr_val[1] << 16) + (arr_val[2] << 8) + arr_val[3]  # cast to uint32_t
                    else:
                        return arr_val[0], arr_val[1], (arr_val[2] << 8) + arr_val[3]
                        
                else:
                    val = arr_val
                return val
        return []

    def SetPib(self, pib, value):
        if self.pibAccessLocalMngm:
            if isinstance(value, list):
                arr_value = value
            else:
                if (pib == self.Pibs.macTrefTime):
                    arr_value = [0] * 4
                    arr_value[0] = value >> 24
                    arr_value[1] = value >> 16
                    arr_value[2] = value >> 8
                    arr_value[3] = value & 0xFF
                elif (pib == self.Pibs.macUpdatedRMTimeout or  
                         pib == self.Pibs.macCapabilities or
                         pib == self.Pibs.macZeroCrossConfig):                   
                    arr_value = [0] * 2
                    arr_value[0] = value >> 8
                    arr_value[1] = value & 0xFF
                else:
                    arr_value = [0] * 1
                    arr_value[0] = value
                    
            self.LocalMgmt.SetPib(pib, arr_value)
            return True
        
        if self.dllwrp.MAC_MLME_SETrequest(pib, value) == 0:
            while True:
                ret, res = self.dllwrp.MAC_MLME_SETconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def GetPibList(self, list_id):
        if self.dllwrp.MAC_MLME_LIST_GETrequest(list_id) == 0:
            while True:
                ret, status, _, length, arr = self.dllwrp.MAC_MLME_LIST_GETconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)  
            if status == 0:
                if list_id == self.Pibs.macListRegDevices:
                    entry_size = 20
                elif list_id == self.Pibs.macListActiveConn:
                    entry_size = 11
                elif list_id == self.Pibs.macListMcastEntries:    
                    entry_size = 4
                elif list_id == self.Pibs.macListSwitchTable:    
                    entry_size = 5
                elif list_id == self.Pibs.macListPhyComm:    
                    return SpecPibs.convertToPhyCommList(arr)
                elif list_id == self.Pibs.macListActiveConnX:    
                    entry_size = 13
                else:
                    return []

                lis = []
                for j in range (0,  int(length/entry_size)):
                    lis += [arr[j * entry_size:(j + 1) * entry_size]]
                
                return lis       
        return []

    def PromoteNode(self, eui):
        if self.dllwrp.MAC_PROMOTIONrequest(eui) == 0:
            time.sleep(0.015)
            for _ in range(0, 60):
                ret, res = self.dllwrp.MAC_PROMOTIONconfirm()
                if ret == 0:
                    if res == 0:
                        sys.stdout.write("\n")
                        return True
                    else:
                        sys.stdout.write("PromoteNode: res != 0\n") 
                        print(str(res))
                time.sleep(0.5)
        sys.stdout.write("PromoteNode: No CONFIRM\n")
        return False
    
    def PromoteNodeExtendedMode(self, eui, mod, ds=0):
        if self.Pibs.macSwitchBeaconTxMod_en_check(self.GetPib(self.Pibs.macSwitchBeaconTxMod)):
            # lock used by both promote and demote APIs
            return False
        self.SetPib(self.Pibs.macSwitchBeaconTxMod, self.Pibs.macSwitchBeaconTxMod_set(1, ds, mod))
        ret = self.PromoteNode(eui)
        if ret == False:
            self.SetPib(self.Pibs.macSwitchBeaconTxMod, self.Pibs.macSwitchBeaconTxMod_set(0))
        return ret
    
    def DemoteNode(self, eui):
        if self.dllwrp.MAC_DEMOTIONrequest(eui) == 0:
            while True:
                ret, res = self.dllwrp.MAC_DEMOTIONconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False
    
    def DemoteNodeExtendedMode(self, eui, mod):
        if self.Pibs.macSwitchBeaconTxMod_en_check(self.GetPib(self.Pibs.macSwitchBeaconTxMod)):
            # lock used by both promote and demote APIs
            return False
        self.SetPib(self.Pibs.macSwitchBeaconTxMod, self.Pibs.macSwitchBeaconTxMod_set(1, 1, mod))
        ret = self.DemoteNode(eui)
        if ret == False:
            self.SetPib(self.Pibs.macSwitchBeaconTxMod, self.Pibs.macSwitchBeaconTxMod_set(0))
        return ret

    def Unregister(self, eui):
        if self.dllwrp.MAC_UNREGISTERrequest(eui) == 0:
            while True:
                ret, res = self.dllwrp.MAC_UNREGISTERconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def SetTestMode(self, inTestModeMask):
        self.currTestModeMask |= inTestModeMask
        self.dllwrp.BNRT_TestMode(self.currTestModeMask)
    
    def ClearTestMode(self, inTestModeMask=None):
        if inTestModeMask is None:
            inTestModeMask = ~self.defTestModeMask
        self.currTestModeMask &= ~inTestModeMask
        self.dllwrp.BNRT_TestMode(self.currTestModeMask)
    
    def InitTestMode(self, inTestModeMask=None):
        if inTestModeMask is not None:
            self.defTestModeMask = inTestModeMask
        self.currTestModeMask = self.defTestModeMask
        self.dllwrp.BNRT_TestMode(self.currTestModeMask)

    def GetTestMode(self):
        return self.currTestModeMask
        
    def GetDefTestMode(self):
        return self.defTestModeMask

    def getLNIDFromEUI(self, eui48):
        lis = self.GetPibList(self.Pibs.macListRegDevices)
        for entry in lis:
            if eui48 == entry[0:6]:
                return (entry[6] << 8) + entry[7]
        return 0x3FFF

    def StartDFUProcess (self, fileupgrade, nodes_eui48_list=[], mode=DfuModes.unicast, psize=DfuConst.S128, signatureAlg = SignatureAlg.NOSIGN, signatureSize = 0):
        if not os.path.isfile(fileupgrade):
            return False
        print("StartDFUProcess psize " + str(psize))
        if self.dllwrp.DFU_Init_Req(fileupgrade, psize, mode, signatureAlg, signatureSize) != 0: # [AP: aggiungere il passaggio di signatureAlg e signatureLen nella DFU_Init_Req]
            return False
        time.sleep(1)
        if self.dllwrp.DFU_SNsInit(nodes_eui48_list) != 0:
            return False
        time.sleep(3)
        if self.dllwrp.DFU_StartUpgrade(nodes_eui48_list) != 0:
            return False
        self.dllwrp.DFU_Exec(nodes_eui48_list)
        return True

    def StopDFUProcess(self, nodes_eui48_list=[]):
        if self.dllwrp.DFU_StopUpgrade(nodes_eui48_list)!= 0:
            return False
        time.sleep(3)
        if self.dllwrp.DFU_CloseConn(nodes_eui48_list)!= 0:
            return False
        time.sleep(3)
        return True

    def SetPrimeConfigPHY(self):
        self.SetPib(self.Pibs.primeConfig, self.PrimeConfig.PHY)
        time.sleep(8)
        return (self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.PHY)

    def SetPrimeConfigFULL(self):
        self.SetPib(self.Pibs.primeConfig, self.PrimeConfig.FULL)
        time.sleep(8)
        return (self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.FULL)

    def SetPrimeConfigAPPEMU(self):
        self.SetPib(self.Pibs.primeConfig, self.PrimeConfig.APPEMU)
        time.sleep(0.5)
        return(self.GetPib(self.Pibs.primeConfig) == self.PrimeConfig.APPEMU)

    def StartPhyTxTest(self, modulation):
        return self.SetPib(self.Pibs.phyStartTxTest, modulation)

    def StartPhyRxTest(self):
        return self.SetPib(self.Pibs.phyStartRxTest, 1)
    
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
    
    def GetState(self):
        state = 0
        ret, state_phy = self.dllwrp.PHY_GetState()
        if ret == 0:
            state |= state_phy
        ret, state_null = self.dllwrp.CL_NULL_GetState()
        if ret == 0:
            state |= state_null
        ret, state_432 = self.dllwrp.CL_432_GetState()   
        if ret == 0:
            state |= state_432
        ret, state_IPv6 = self.dllwrp.CL_IPv6_GetState()   
        if ret == 0:
            state |= state_IPv6
        return state
        
    def pvSetCallbackPointer(self,func):
        self.dllwrp.pvSetCallbackPointer(func)
        return 

    def PRIMEGetStatePrint(self, bfr):
        line = []
        line += ["STATE: " + str(format(bfr, '08x'))]
        if bfr != 0:
            for key in list(self.Indication.strings.keys()):
                if bfr & key != 0:
                    line +=[str(self.Indication.strings[key])]
        return line

    def DLBroadcastRequest(self, ssap, data):
        if self.dllwrp.DL_BROADCASTrequest(ssap, data) == 0:
            return True
        return False

    def DlDataRequest(self, d_adr, s_adr, data):
        return (self.dllwrp.DL_DATArequest(d_adr, s_adr, data) == 0)

    def DlDataConfirm(self, d_adr, s_adr, timeout=20):
        endtime = time.time() + timeout
        while True:
            ret, _, _, dadr, sadr, txstatus = self.dllwrp.DL_DATAconfirm()
            if ret == 0:
                if ((dadr == d_adr) and (sadr == s_adr) and (txstatus == 0)):
                    return True
                break
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False
    
    def DlDataIndicationLoop(self, d_adr, s_adr, bfrlen, timeout = 60):
        endtime = time.time() + timeout
        while True:
            ret, _, _, sadr, dadr, dsudata = self.dllwrp.DL_DATAIndication()
            if ret == 0:
                if ((dadr == d_adr) and (sadr == s_adr)):
                    return True, dsudata                
            if time.time() > endtime :
                break
            time.sleep(0.015)
        return False, []    
   
    def CL432GetSNSession(self):
        return self.dllwrp.CL_432_GetSNSession()

    def Cl432SNSessionPrint(self, sn_s):
        j = 0
        leng = len(sn_s)
        line = []
        while j < leng:
            line += ["EUI48:  " + " ".join('{:02x}'.format(c) for c in sn_s[j:(j+6)])]
            j += 6
            SNLen = sn_s[j]
            j += 1
            line += ["DAdd: " + str((sn_s[j]<< 8 )+ (sn_s[j+1]))]
            j += 2
            line += ["DSAP: " + str(sn_s[j])]
            j += 1
            line += ["SSAP: " + str(sn_s[j])]
            j += 1
            line += ["SNLen:  " + str(SNLen)]
            line += ["SerNum: "+ ' '.join('{:02x}'.format(c) for c in sn_s[j:(j+SNLen)])]
            j += SNLen
        return line
    
    def CL432SessionPrintGetSerNum(self, sn_s):
        j = 6
        SNLen = sn_s[j]
        j += 1        
        j += 2       
        j += 1       
        j += 1
        sernum = ' '.join('{:02x}'.format(c) for c in sn_s[j:(j+SNLen)])            
        return sernum

    def CL432JoinInd(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            res, d_adr, sernum = self.dllwrp.CL_432_JOINindicate()
            if res == 0:
                return True, d_adr, sernum
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, 0, []

    def CL432LeaveInd(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            ret, d_adr = self.dllwrp.CL_432_LEAVEindicate()
            if ret == 0:
                return True, d_adr
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, 0

    def CL432GetData(self, dst_addr, src_addr, timeout=20):
        endtime = time.time() + timeout
        while True:
            ret, _, _, dadr, sadr, _, data = self.dllwrp.DL_DATAindication()
            if ret == 0 and dadr == dst_addr and sadr == src_addr:
                return data
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return []

    def CL432SendData(self, dst_addr, src_addr, buff, timeout=20):
        if self.DlDataRequest(dst_addr, src_addr, buff):
            return self.DlDataConfirm(dst_addr, src_addr, timeout)
        else:
            return False

    def CL432WaitData(self, dst_addr, src_addr, timeout=20):
        endtime = time.time() + timeout
        while True:
            data = self.CL432GetData(dst_addr, src_addr)
            if data != []:
                return data
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return []


    #################################################################################
    ''' IPv6 Packet:
            || 4bit | 4bit || 4bit | 4bit || 4bit | 4bit || 4bit | 4bit ||
            --------------------------------------------------------------
            || 0110 | traffic class| -------- flow label --------       ||
            || ----- payload length ------||-next header-||- HOP limit -||
            || -----------    source address (16 bytes)  ----------- ...
               ...                                                      ||
            || ---------    destination address (16 bytes)  -------- ...
               ...                                                      || 
    '''
    def CL_IPv6_SendData(self, src_addr, dst_addr, payload, nextHeader=59, hopLimit=1, timeout=20):
        """ Function to send data to a particular Node via Unicast connection.
            The functions returns two objects (confirm,txData):
            - confirm: the confirmation that data has been sent
            - txData : a list containinf the data transmitted ( header + payload )
        """
        # prepare the final PDU using the IPv6 address passed as paramether 
        # traffic class = 0x00 FIXME: define 'traffic class' value
        # flow label = 0x00000 FIXME: define 'flow label' value
        payloadLen = len(payload)
        # lengthPDU = payloadLen + 40 # buffer + header      
        pdu  = [0x60,0x00,0x00,0x00]        # 0-3) pkt type (4bit) - traffic class (8bit) - flow label (20bit)
        pdu += [(payloadLen>>8)&0xFF , payloadLen&0xFF]   # 4-5) payload length
        pdu += [nextHeader] # 6) next header
        pdu += [hopLimit] # 7) HOP limit
        pdu += src_addr # 8-23) source address
        pdu += dst_addr # 24-39) destination address
        pdu += payload

        pduDataLength = len(pdu) # max 1500 (MTU)
        ret = self.dllwrp.CL_IPv6_DATArequest (pduDataLength, pdu)
        if ret != 0:
            print("BN - CL_IPv6_SendData - Error on CL_IPv6_DATArequest ( "+str(ret)+" ).")
            return ret,[]
        return ret,pdu
        '''
        endtime = time.time() + timeout
        while True:
            time.sleep(0.05)  
            ret,confirm = self.dllwrp.CL_IPv6_DATAconfirm()
            if ret == 0:
                return confirm, pdu # CL_IPv6_SAP_[ NOERR | NULLPOINTER_ERR | 
            if time.time() > endtime:
                break
        return ret,[]
        '''
    
    # -------------------------------------------------------------------------
    ###########################################################################
    # the following function is commented because, in the CL_IPv6, there isn't a 'DataConfirm' after 'DataRequest'
    '''
    def CL_IPv6_SendFrame(self, dataFrame, timeout=20):
        # This function sends a preformatted IPv6 packet.
        #    It returns 
        pduDataLength = len(dataFrame)
        ret = self.dllwrp.CL_IPv6_DATArequest (pduDataLength, dataFrame)
        if ret != 0:
            return ret,[]
        endtime = time.time() + timeout
        while True:
            time.sleep(0.05)  
            ret,confirm = self.dllwrp.CL_IPv6_DATAconfirm()
            if ret == 0:
                return confirm, dataFrame # CL_IPv6_SAP_[ NOERR | NULLPOINTER_ERR | 
            if time.time() > endtime:
                break
        return ret,[]
    '''
    ###########################################################################
    def CL_IPv6_SendFrame(self, dataFrame, timeout=20):
        ''' This function sends a preformatted IPv6 packet.
            It returns  '''
        pduDataLength = len(dataFrame)
        ret = self.dllwrp.CL_IPv6_DATArequest (pduDataLength, dataFrame)
        if ret == 0:
            return ret, dataFrame
        '''
        endtime = time.time() + timeout
        while True:
            time.sleep(1)  
            ret = self.dllwrp.CL_IPv6_DATArequest (pduDataLength, dataFrame)
            if ret == 0:
                return ret, dataFrame 
            if time.time() > endtime:
                break
        '''
        return ret,[]
    ###########################################################################
    # -------------------------------------------------------------------------

    def CL_IPv6_GetData(self):
        ''' This function gets data from PLC connection. '''
        ret, _ , PDU = self.dllwrp.CL_IPv6_DATAindicate()
        if ret == 0:
            return PDU
        time.sleep(0.015)
        return []
    
    def CL_IPv6_WaitData(self, timeout=120):
        ''' This function wait for data from a PLC connection. '''
        endtime = time.time() + timeout
        while True:
            data = self.CL_IPv6_GetData()
            if data != []:
                return data
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return []

    def CL_IPv6_GetUCASTSession(self):
        ''' This function gets the list of active UNICAST IPv6 sessions '''
        ret=self.dllwrp.CL_IPv6_GetUCASTSession()
        return ret

    def CL_IPv6_GetARtable(self):
        ''' This function gets the list of the AR IPv6 active sessions '''
        ret=self.dllwrp.CL_IPv6_GetARtable()
        return ret
    ###############################################################################



    def CL_NULL_GetData(self):
        ret, ch, data = self.dllwrp.CL_NULL_DATAindication()
        if ret == 0:
            return ch, data
        return 0, []

    def CL_NULL_SendData(self, ch, data, prio=1, seg_size=64):
        if self.dllwrp.CL_NULL_DATArequest(ch, data, prio, seg_size) == 0:
            
            while True:
                ret, _, res, _ = self.dllwrp.CL_NULL_DATAconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def CL_NULL_WaitData(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            ch, data = self.CL_NULL_GetData()
            if data != []:
                return ch, data
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return 0, []

    def CL_NULL_Establish(self, eui, typ=0, arq=False, data=[]):
        if self.dllwrp.CL_NULL_ESTABLISHrequest(eui, typ, arq, data) == 0:
            while True:
                ret, ch, res, _, _, rdata = self.dllwrp.CL_NULL_ESTABLISHconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, ch, res, rdata
        return False, 0, 0, []

    def CL_NULL_Release(self, ch):
        if self.dllwrp.CL_NULL_RELEASErequest(ch) == 0:
            while True:
                ret, _, res, _ = self.dllwrp.CL_NULL_RELEASEconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, res
        return False, 0

    def CL_NULL_Join(self, eui48, ch=0, data=[]):
        if self.dllwrp.CL_NULL_JOINrequest(0, ch, eui48, 0, data) == 0:
            while True:
                ret, ch, eui, res = self.dllwrp.CL_NULL_JOINconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, ch,  eui, res
        return False, 0, [0,0,0,0,0,0], 0

    def CL_NULL_Leave(self, ch, eui48):
        if self.dllwrp.CL_NULL_LEAVErequest(ch, eui48) == 0:
            while True:
                ret, ch, res, eui, typ = self.dllwrp.CL_NULL_LEAVEconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, ch, res, eui, typ
        return False, 0, 0, [0,0,0,0,0,0],0

    def RMI_SendMsg_WaitResp(self, ch, typ, payload):  
        tmp = (len(payload) << 6) + typ
        header = [tmp >> 8, tmp & 0xFF]
        self.CL_NULL_SendData(ch, header + payload, prio=4)
        time.sleep(0.5) 
        ch_rx, data = self.CL_NULL_WaitData()
        if ch == ch_rx:
            return data
        else:
            return []
            
    def RMI_SendMsg_NoResp(self, ch, typ, payload):  
        tmp = (len(payload) << 6) + typ
        header = [tmp >> 8, tmp & 0xFF]
        return self.CL_NULL_SendData(ch, header + payload, prio=4)
         
    def RMI_Reboot(self, ch):  
        Payload = [0x00]
        self.RMI_SendMsg_NoResp(ch,MgmtTypes.REBOOT, Payload)  
        # rebbot can occurs before cl_null response!
        return True        
        
    def RMI_SetAttr(self, ch, AttributeID, Value = []):
        #action attributes can have no value
        if Value == []:
            Payload = [AttributeID >> 8, AttributeID & 0x00FF] 
        else:
            Payload = [AttributeID >> 8, AttributeID & 0x00FF, Value]      
        return self.RMI_SendMsg_NoResp(ch,MgmtTypes.SETreq,Payload)   
    
    def RMI_GetAttr(self, ch, AttributeID, index=0):
        Payload = [AttributeID >> 8, AttributeID & 0x00FF, index] 
        RMIrspPay = self.RMI_SendMsg_WaitResp(ch,MgmtTypes.GETreq,Payload)       
        if RMIrspPay == []:
            return []  
        ret = 0
        val = []
        for i in range(5, len(RMIrspPay) - 1): 
            #print "i: " + str(i)
            #print "RMIrspPay[i]: {:2x}".format(RMIrspPay[i])
            #print "(8*(len(RMIrspPay) - i - 1)):    " + str((8*(len(RMIrspPay) - i - 2)))
            ret += RMIrspPay[i] << (8*(len(RMIrspPay) - i - 2))
            val += [RMIrspPay[i]]
        return val
        
    def RMI_GetList(self, ch, ListID, index):
        return self.RMI_GetAttr(ch, ListID, index)
    
    def getPhaseAndPolarityCode(self, BnToSnPhaseDiff_10us, mains_period_10us):
        if (BnToSnPhaseDiff_10us < mains_period_10us/12):
            return 0x00    
        else: 
            if ((BnToSnPhaseDiff_10us >= mains_period_10us/12) and (BnToSnPhaseDiff_10us < 3*mains_period_10us/12)):
                return 0x12   
            else: 
                if ((BnToSnPhaseDiff_10us >= 3*mains_period_10us/12) and (BnToSnPhaseDiff_10us < 5*mains_period_10us/12)):
                    return 0x01   
                else:
                    if ((BnToSnPhaseDiff_10us >= 5*mains_period_10us/12) and (BnToSnPhaseDiff_10us < 7*mains_period_10us/12)):
                        return 0x10   
                    else:
                        if ((BnToSnPhaseDiff_10us >= 7*mains_period_10us/12) and (BnToSnPhaseDiff_10us < 9*mains_period_10us/12)):
                            return 0x02    
                        else: 
                            if ((BnToSnPhaseDiff_10us >= 9*mains_period_10us/12) and (BnToSnPhaseDiff_10us < 11*mains_period_10us/12)):
                                return 0x11            
                            else: 
                                if (BnToSnPhaseDiff_10us >= 11*mains_period_10us/12):
                                    return 0x00    
  
    def PHYDataRequest(self, level, scheme, timer, psdu):
        return self.dllwrp.PHY_DATArequest(level, scheme, timer, psdu)

    def PHYDataConfirm(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            ret, res =  self.dllwrp.PHY_DATAconfirm()
            if ret == 0:
                return True, res
            if time.time() > endtime:
                break
            time.sleep(0.15)
        return False, 0

    def PHYGetData(self):
        ret, length, lvl, scheme, type, snr, rxqlty, timer, psdu = self.dllwrp.PHY_DATAindication()
        if ret == 0:
            return 0, length, lvl, scheme, type, snr, rxqlty, timer, psdu
        return ret, 0, 0, 0, 0, 0, 0, 0, []

    def PHYWaitData(self, timeout=20):
        endtime = time.time() + timeout
        while True:
            _, length, _, _, _, _, _, _, data = self.PHYGetData()
            if data != []:
                return data,length
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return [],0

    def PHYSendData(self, buff=[], level=1, scheme=0, timer=0):
        if self.PHYDataRequest(level, scheme, timer, buff) == 0 :
            return self.PHYDataConfirm()        
        return False
        

    def WaitClNullEstablishRequest (self, response=0, timeout=20):
        endtime = time.time() + timeout
        while True:
            sys.stdout.write(".")
            sys.stdout.flush()
            state = self.GetState()
            if state & self.Indication.CL_NULL_ESTABLISHINDICATION != 0:
                ret, ch, _, _, _, data = self.dllwrp.CL_NULL_ESTABLISHindication()
                if ret == 0:
                    ret = self.dllwrp.CL_NULL_ESTABLISHresponse(ch, response, data)
                    return True, data
                else:
                    return False, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, []

    def ManageClNullEstablishIndication(self):
        ret, ch, _, _, _, _ = self.dllwrp.CL_NULL_ESTABLISHindication()
        if ret == 0:
            self.dllwrp.CL_NULL_ESTABLISHresponse(ch, 0, [])

    def ManageClNullReleaseIndication(self):
        ret, ch, reason, _ = self.dllwrp.CL_NULL_RELEASEindication()
        if ret == 0:
            if reason == 0:
                self.dllwrp.CL_NULL_RELEASEresponse(ch, 0)

    def WaitClNullJoinIndication (self, response=0, timeout=20):
        endtime = time.time() + timeout
        while True:
            sys.stdout.write(".")
            sys.stdout.flush()
            state = self.GetState()
            if state & self.Indication.CL_NULL_JOININDICATION != 0:
                sys.stdout.write("\n")
                ret, ch, eui, _, data = self.dllwrp.CL_NULL_JOINindication()
                if ret == 0:
                    self.dllwrp.CL_NULL_JOINresponse(ch, eui, response)
                    return True, ch, data
                else:
                    return False, 0, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        sys.stdout.write("\n")
        return False, 0, []

    def ManageClNullJoinIndication(self):
        ret, ch, eui, _, _ =  self.dllwrp.CL_NULL_JOINindication()
        if ret == 0:
            self.dllwrp.CL_NULL_JOINresponse(ch, eui, 0)

    def ManageClNullLeaveIndication(self):
        _, _, _ = self.dllwrp.CL_NULL_LEAVEindication()


class BnDllWrapper():
    wrapped_py_func = []
    def __init__(self, comport):

        relative_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

        if comport.startswith("COM"):
            relative_path += "\\libraries\\lib_prime_bn.dll"
            self.bndll = ctypes.cdll.LoadLibrary(relative_path)
            self.OS = "Windows"
        else:
            relative_path += "/libraries/lib_prime_bn.so"
            self.bndll = ctypes.cdll.LoadLibrary(relative_path)
            self.OS = "Linux"

        #print ("base node handle = " + str(self.bndll))
            
    def Unloadcdll(self):
        #print ("base node handle = " + str(self.bndll)) 
        if self.OS == "Windows":
            _ctypes.FreeLibrary(self.bndll._handle)
        else:
            self.dldll = ctypes.cdll.LoadLibrary('libdl.so')
            self.dldll.dlclose(self.bndll._handle)
            self.dldll.dlclose(self.dldll._handle)
            del self.dldll

    def UartInit(self, comport):
        if comport.startswith("COM"):
            return self.bndll.UartInit(int(comport[3:]))
        else:
            return self.bndll.UartInit(c_char_p(comport.encode()))

    def UartClose(self):
        self.bndll.UartClose()
    
    def PRIME_LIB_Init(self, lnid_window):
        self.bndll.PRIME_LIB_Init(c_uint8(lnid_window))
    
    def PRIME_LIB_FsmEvolve(self):
        return self.bndll.PRIME_LIB_FsmEvolve()
    
    def PRIME_LIB_Stop(self):
        self.bndll.PRIME_LIB_Stop()
    
    def MAC_MLME_GETrequest(self, pib):
        class MAC_MlmeGetRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("PIBAttribute", c_uint16),
                        ("Endianness", c_uint8)]
        req = MAC_MlmeGetRequest_t(pib, 0)
        return self.bndll.MAC_MLME_GETrequest(pointer(req))

    def MAC_MLME_GETconfirm(self):
        class MAC_MlmeGetConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("Status", c_uint8),
                        ("PIBAttribute", c_uint16),
                        ("Len", c_uint8),
                        ("PIBAttributeValue", c_uint8 * 16),
                        ("Endianness", c_uint8)]
        conf = MAC_MlmeGetConfirm_t()
        ret = self.bndll.MAC_MLME_GETconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.Status, conf.PIBAttribute, conf.PIBAttributeValue[:conf.Len]
        return ret, 0, 0, []

    def MAC_MLME_SETrequest(self, pib, value):
        class MAC_MlmeSetRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("PIBAttribute", c_uint16),
                        ("PIBAttributeValue", c_uint32)]
        req = MAC_MlmeSetRequest_t(pib, value)
        return self.bndll.MAC_MLME_SETrequest(pointer(req))

    def MAC_MLME_SETconfirm(self):
        class MAC_MlmeSetConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("result", c_uint8)]
        conf = MAC_MlmeSetConfirm_t()
        ret = self.bndll.MAC_MLME_SETconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.result
        return ret, 0

    def MAC_MLME_LIST_GETrequest(self, list_id):
        return self.bndll.MAC_MLME_LIST_GETrequest(pointer(c_uint16(list_id)))

    def MAC_MLME_LIST_GETconfirm(self):
        class MAC_MlmeListGetConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("Status", c_uint8),
                        ("PIBListAttribute", c_uint16),
                        ("PIBListAttributeLen", c_uint16),
                        ("PIBListAttributeValue", c_uint8 * (     (13) * ((0x7FF) -2) * (3)     )              )]
        conf = MAC_MlmeListGetConfirm_t()
        ret = self.bndll.MAC_MLME_LIST_GETconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.Status,  conf.PIBListAttribute, conf.PIBListAttributeLen, conf.PIBListAttributeValue[0:conf.PIBListAttributeLen]
        return ret, 0, 0,0, []

    def BNRT_TestMode(self, testmode):
        self.bndll.BNRT_TestMode(c_uint32(testmode))

    def MAC_PROMOTIONrequest(self, eui):
        class Eui48(Structure):
            _pack_ = 1
            _fields_ = [("EUI48", c_uint8 * 6)]
        req = Eui48((c_uint8 * 6)(*(eui)))
        return self.bndll.MAC_PROMOTIONrequest(pointer(req))

    def MAC_PROMOTIONconfirm(self):
        result = c_uint8()
        ret = self.bndll.MAC_PROMOTIONconfirm(pointer(result))
        if ret == 0:
            return ret, result.value
        return ret, 0

    def MAC_PROMOTIONindication(self):
        return self.bndll.MAC_PROMOTIONindication()

    def MAC_DEMOTIONrequest(self, eui):
        class Eui48(Structure):
            _pack_ = 1
            _fields_ = [("EUI48", c_uint8 * 6)]
        eui48 = Eui48((c_uint8 * 6)(*(eui)))
        return self.bndll.MAC_DEMOTIONrequest(pointer(eui48))

    def MAC_DEMOTIONconfirm(self):
        result = c_uint8()
        ret = self.bndll.MAC_DEMOTIONconfirm(pointer(result))
        if ret == 0:
            return ret, result.value
        return ret, 0

    def MAC_DEMOTIONindication(self):
        return self.bndll.MAC_DEMOTIONindication()

    def MAC_UNREGISTERrequest(self, eui):
        class Eui48(Structure):
            _pack_ = 1
            _fields_ = [("EUI48", c_uint8 * 6)]
        eui48 = Eui48((c_uint8 * 6)(*(eui)))
        return self.bndll.MAC_UNREGISTERrequest(pointer(eui48))

    def MAC_UNREGISTERconfirm(self):
        result = c_uint8()
        ret = self.bndll.MAC_UNREGISTERconfirm(pointer(result))
        if ret == 0:
            return ret, result.value
        return ret, 0

    def MAC_UNREGISTERindication(self):
        return self.bndll.MAC_UNREGISTERindication()

    def CL_NULL_DATAindication(self):
        class CL_NULL_DataIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("DataLength", c_uint16),
                        ("Data", c_uint8 * 2051)]
        ind = CL_NULL_DataIndication_t()
        ret = self.bndll.CL_NULL_DATAindication(pointer(ind))
        if ret == 0:
            return ret, ind.ConHandle, ind.Data[0:ind.DataLength]
        return ret, 0, []

    def CL_NULL_DATArequest(self, ch, data, prio=1, seg_size=64):
        class CL_NULL_TxRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("DataLength", c_uint16),
                        ("Priority", c_uint8),
                        ("SegmentSize", c_uint16),
                        ("Data", c_uint8 * 2051)]
        req = CL_NULL_TxRequest_t(ch, len(data), prio, seg_size, (c_uint8 * 2051)(*(data)))
        return self.bndll.CL_NULL_DATArequest(pointer(req))

    def CL_NULL_DATAconfirm(self):
        class CL_NULL_TxConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("Data", c_uint8)]

        conf = CL_NULL_TxConfirm_t()
        ret = self.bndll.CL_NULL_DATAconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.Data
        return ret, 0, 0, 0

    def CL_NULL_ESTABLISHindication(self):
        class CL_NULL_EstablishIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("CfBytes", c_uint16),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 256)]

        ind = CL_NULL_EstablishIndication_t()
        ret = self.bndll.CL_NULL_ESTABLISHindication(pointer(ind))
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48, ind.Type, ind.CfBytes, ind.Data[0:ind.DataLength]
        return ret, 0, [0,0,0,0,0,0], 0, 0, []

    def CL_NULL_ESTABLISHresponse(self, ch, answer, data):
        class CL_NULL_EstablishResponse_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("Answer", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 256)]

        resp = CL_NULL_EstablishResponse_t(ch, answer, len(data), (c_uint8 * 256)(*(data)))
        return self.bndll.CL_NULL_ESTABLISHresponse(pointer(resp))

    def CL_NULL_ESTABLISHrequest(self, eui, typ, arq=False, data=[]):
        class CL_NULL_EstablishRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("EUI48", c_uint8 * 6),
                        ("Type", c_uint8 ),
                        ("ARQ", c_uint8 ),
                        ("CfBytes", c_uint16 ),
                        ("DataLength", c_uint8 ),
                        ('Data', c_uint8 * 256)]
        req = CL_NULL_EstablishRequest_t((c_uint8 * 6)(*(eui)), typ, 1 if arq else 0, 0, len(data), (c_uint8 * 256)(*(data)))
        return self.bndll.CL_NULL_ESTABLISHrequest(pointer(req))


    def CL_NULL_ESTABLISHconfirm(self):
        class CL_NULL_EstablishConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 256)]
        conf = CL_NULL_EstablishConfirm_t()
        ret = self.bndll.CL_NULL_ESTABLISHconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.EUI48, conf.Type, conf.Data[0:conf.DataLength]
        return ret, 0, 0, [0,0,0,0,0,0], 0, []

    def CL_NULL_RELEASEindication(self):
        class CL_NULL_ReleaseIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("Reason", c_uint8),
                        ("Type", c_uint8)]
        ind = CL_NULL_ReleaseIndication_t()
        ret = self.bndll.CL_NULL_RELEASEindication(pointer(ind))
        if ret == 0:
            return ret, ind.ConHandle, ind.Reason, ind.Type
        return ret, 0, 0, 0

    def CL_NULL_RELEASEresponse(self, ch, answer):
        class CL_NULL_ReleaseResponse_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("Answer", c_uint8)]
        resp = CL_NULL_ReleaseResponse_t(ch, answer)
        return self.bndll.CL_NULL_RELEASEresponse(pointer(resp))

    def CL_NULL_RELEASErequest(self, ch):
        ret = self.bndll.CL_NULL_RELEASErequest(c_uint32(ch))
        return ret

    def CL_NULL_RELEASEconfirm(self):
        class CL_NULL_ReleaseConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("AccessType", c_uint8)]
        conf = CL_NULL_ReleaseConfirm_t()
        ret = self.bndll.CL_NULL_RELEASEconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.AccessType
        return ret, 0, 0, 0

    def CL_NULL_JOINrequest(self, bcast, ch, eui, typ, data=[]):
        class CL_NULL_JoinRequest_t(Structure):
            _pack_   = 1
            _fields_ = [("Broadcast", c_uint8),
                        ("CH_LCID", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 498)]
        req = CL_NULL_JoinRequest_t(bcast, ch, (c_uint8 * 6)(*(eui)), typ, len(data), (c_uint8 * 498)(*(data)))
        return self.bndll.CL_NULL_JOINrequest(pointer(req))

    def CL_NULL_JOINconfirm(self):
        class CL_NULL_JoinConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Result", c_uint8)]

        conf = CL_NULL_JoinConfirm_t()
        ret = self.bndll.CL_NULL_JOINconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.ConHandle, conf.EUI48[0:6], conf.Result
        return ret, 0, [0,0,0,0,0,0], 0

    def CL_NULL_JOINindication(self):
        class CL_NULL_JoinIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 498)]

        ind = CL_NULL_JoinIndication_t()
        ret = self.bndll.CL_NULL_JOINindication(pointer(ind))
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48[0:6], ind.Type, ind.Data[0:ind.DataLength]
        return ret, 0, [0,0,0,0,0,0], 0, []

    def CL_NULL_JOINresponse(self, ch, eui, answer):
        class CL_NULL_JoinResponse_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Answer", c_uint8)]

        res = CL_NULL_JoinResponse_t(ch, (c_uint8 * 6)(*(eui)), answer)
        return self.bndll.CL_NULL_JOINresponse(pointer(res))

    def CL_NULL_LEAVErequest(self, ch, eui):
        class CL_NULL_LeaveRequest_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6)]

        req = CL_NULL_LeaveRequest_t(ch, (c_uint8 * 6)(*(eui)))
        return self.bndll.CL_NULL_LEAVErequest(pointer(req))

    def CL_NULL_LEAVEconfirm(self):
        class CL_NULL_LeaveConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("EUI48", c_uint8 * 6),
                        ("AccessType", c_uint8)]

        conf = CL_NULL_LeaveConfirm_t()
        ret = self.bndll.CL_NULL_LEAVEconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.EUI48[0:6], conf.AccessType
        return ret, 0, 0, [0,0,0,0,0,0], 0

    def CL_NULL_LEAVEindication(self):
        class CL_NULL_LeaveIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6)]
        ind = CL_NULL_LeaveIndication_t()
        ret = self.bndll.CL_NULL_LEAVEindication(pointer(ind))
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48
        return ret, 0, [0,0,0,0,0,0]

    def CL_432_GetSNSession(self):
        class CL_432_SNSessionBuffer(Structure):
            _pack_ = 1
            _fields_ = [("SNSession", c_uint8 * 512)]
        cl432_session = CL_432_SNSessionBuffer()
        self.bndll.CL_432_GetSNSession(pointer(cl432_session))
        leng = (((cl432_session.SNSession[1] << 8) + (cl432_session.SNSession[0])) - 2)
        return True, cl432_session.SNSession[2:leng+2]

    def PHY_GetState(self):
        state = c_uint32(0)
        ret = self.bndll.PHY_GetState(pointer(state))
        return ret, state.value

    def CL_NULL_GetState(self):
        state = c_uint32(0)
        ret = self.bndll.CL_NULL_GetState(pointer(state))
        return ret, state.value

    def CL_432_GetState(self):
        state = c_uint32(0)
        ret = self.bndll.CL_432_GetState(pointer(state))
        return ret, state.value
    
    def pvSetCallbackPointer(self,func):
        if self.bndll.OS == "Windows":
            CWRAPPER = WINFUNCTYPE (c_uint8)
        else:
            CWRAPPER = CFUNCTYPE (c_uint8)
        self.wrapped_py_func = CWRAPPER(func)
        ret = self.bndll.pvSetCallbackPointer(self.wrapped_py_func)
        return ret

    def CL_432_JOINindicate(self):
        class CL_432_JoinIndicate_t(Structure):
            _pack_ = 1
            _fields_ = [("DestinationAddress", c_uint16 ) ,
                        ("SerNumLen", c_uint8 ) ,
                        ("SerNum", c_uint8 * 16)]
        ind = CL_432_JoinIndicate_t()
        ret = self.bndll.CL_432_JOINindicate(pointer(ind))
        if ret == 0:
            return ret, ind.DestinationAddress, ind.SerNum[0:ind.SerNumLen]
        else:
            return ret, 0, []

    def DL_REPLYconfirm(self):
        class DL_ReplyConfirm_t(Structure):
            _pack_   = 1
            _fields_ = [("DSAP", c_uint8 ),
                        ("SSAP", c_uint8 ),
                        ("DestinationAddress", c_uint16),
                        ("SourceAddress", c_uint16),
                        ("TransmissionStatus", c_uint8 ),
                        ("SDUDataLength", c_uint8 ),
                        ("SDUData", c_uint8 * 8 )]
        conf = DL_ReplyConfirm_t()
        ret = self.bndll.DL_REPLYconfirm(pointer(conf))
        if ret == 0:
            return ret, conf.DSAP, conf.SSAP, conf.DestinationAddress, conf.SourceAddress, conf.TransmissionStatus, conf.SDUDataLength, conf.SDUData
        else:
            return ret, 0, 0, 0, 0, 0, 0, []

    def DL_REPLYrequest(self, dsap, ssap, dadr, leng):
        class DL_ReplyRequest_t(Structure):
            _pack_   = 1
            _fields_ = [("DSAP", c_uint8 ),
                        ("SSAP", c_uint8 ),
                        ("DestinationAddress", c_uint16),
                        ("LengthAllowed", c_uint8)]

        req = DL_ReplyRequest_t(dsap, ssap, dadr, leng)
        return self.bndll.DL_REPLYrequest(pointer(req))

    def DL_UPDATE_REPLYindication(self):
        class DL_ReplyRequest_t(Structure):
            _pack_   = 1
            _fields_ = [("DSAP", c_uint8 ),
                        ("SSAP", c_uint8 ),
                        ("DestinationAddress", c_uint16),
                        ("SourceAddress", c_uint16),
                        ("LengthRequested", c_uint8)]
        ind =  DL_ReplyRequest_t()
        ret = self.bndll.DL_UPDATE_REPLYindication(pointer(ind))
        return ret, ind.DSAP , ind.SSAP, ind.DestinationAddress, ind.SourceAddress, ind.LengthRequested


    def CL_432_LEAVEindicate(self):
        class pDestinationAddress(Structure):
            _pack_ = 1
            _fields_ = [("DestinationAddress", c_uint16 )]
        ind = pDestinationAddress(0)
        ret = self.bndll.CL_432_LEAVEindicate(pointer(ind))
        return ret, ind.DestinationAddress

    def DL_DATAindication(self):
        class DL_DataIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("DSAP", c_uint8 ) ,
                        ("SSAP", c_uint8 ) ,
                        ("DestinationAddress", c_uint16 ) ,
                        ("SourceAddress", c_uint16 ) ,
                        ("LinkClass", c_uint8 ) ,
                        ("SDUDataLength", c_uint16 ),
                        ("Data", c_uint8 * 2048)]
        req = DL_DataIndication_t()
        ret =  self.bndll.DL_DATAindication(pointer(req))
        if ret == 0:
            return ret, req.DSAP, req.SSAP, req.DestinationAddress, req.SourceAddress, req.LinkClass,  req.Data[0:req.SDUDataLength]
        return ret, 0, 0, 0, 0, 0, []

    def CL_432_AddDeviceToBlacklist(self, sernum):
        return self.bndll.DL_DATAindication((c_uint8 * len(sernum))(*(sernum)), len(sernum))

    def DL_DATArequest(self, d_adr, s_adr, data):
        class DL_DataRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("DSAP", c_uint8 ) ,
                        ("SSAP", c_uint8 ) ,
                        ("DestinationAddress", c_uint16 ) ,
                        ("SourceAddress", c_uint16 ) ,
                        ("LinkClass", c_uint8 ) ,
                        ("SDUDataLength", c_uint16 ),
                        ("Data", c_uint8 * 2048)]
        req = DL_DataRequest_t(0, 0, d_adr, s_adr, 0, len(data),(c_uint8 * 2048)(*(data)))
        return self.bndll.DL_DATArequest(pointer(req))

    def DL_BROADCASTrequest(self, ssap, data, sclass=0):
        class DL_BroadcastRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("SSAP", c_uint8 ),
                        ("ServiceClass", c_uint8 ),
                        ("SDUDataLength", c_uint16 ),
                        ("SDUData", c_uint8 * 2048)]
        req = DL_BroadcastRequest_t(ssap, sclass, len(data), (c_uint8 * 2048)(*(data)))
        return self.bndll.DL_BROADCASTrequest(pointer(req))

    def DL_DATAconfirm(self):
        class DL_DataConfirm_t(Structure):
            _pack_ = 1
            _fields_ = [("DSAP", c_uint8 ) ,
                        ("SSAP", c_uint8 ) ,
                        ("DestinationAddress", c_uint16 ) ,
                        ("SourceAddress", c_uint16 ) ,
                        ("TransmissionStatus", c_uint8 ) ]
        conf = DL_DataConfirm_t()
        ret = self.bndll.DL_DATAconfirm(pointer(conf))
        return ret, conf.DSAP, conf.SSAP, conf.DestinationAddress, conf.SourceAddress, conf.TransmissionStatus

    def DL_DATAIndication(self):
        class DL_DataIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("DSAP", c_uint8 ) ,
                        ("SSAP", c_uint8 ) ,
                        ("DestinationAddress", c_uint16 ) ,
                        ("SourceAddress", c_uint16 ) ,
                        ("LinkClass", c_uint8 ) ,
                        ("SDUDataLength", c_uint16 ) ,
                        ("SDUData", c_uint8 * 2048) ]
        ind = DL_DataIndication_t()
        ret = self.bndll.DL_DATAindication(pointer(ind))
        return ret, ind.DSAP, ind.SSAP, ind.DestinationAddress, ind.SourceAddress, ind.SDUData[0:ind.SDUDataLength]

    # CL_IPv6 - lower level wrapping =======================================================
    def CL_IPv6_DATArequest (self, pduDataLength, pdu):
        class CL_IPv6_DATArequest_t(Structure):
            _pack_ = 1
            _fields_ = [( "PDUDataLength", c_uint16 ) ,
                        ( "PDU", c_uint8 * 1500 )] # (CL_IPv6) MTU = 1500
        req = CL_IPv6_DATArequest_t(pduDataLength, (c_uint8 * 1500)(*(pdu)))
        ret = self.bndll.CL_IPv6_DATArequest(pointer(req))
        return ret
    # --------------------------------------------------------------------------------------        
    def CL_IPv6_DATAconfirm (self):
        conf = c_uint8(0)
        ret = self.bndll.CL_IPv6_DATAconfirm(pointer(conf))
        return ret, conf.value
    # --------------------------------------------------------------------------------------
    def CL_IPv6_DATAindicate (self):
        class CL_IPv6_DATAindicate_t(Structure):
            _pack_ = 1
            _fields_ = [( "PDUDataLength", c_uint16 ) ,
                        ( "PDU", c_uint8 * 1500 )] # (CL_IPv6) MTU = 1500
        ind = CL_IPv6_DATAindicate_t()
        ret = self.bndll.CL_IPv6_DATAindicate(pointer(ind))
        return ret, ind.PDUDataLength, ind.PDU[0:ind.PDUDataLength]
    # --------------------------------------------------------------------------------------
    def CL_IPv6_GetState(self):
        state = c_uint32(0)
        ret = self.bndll.CL_IPv6_GetState(pointer(state))
        return ret, state.value
    # --------------------------------------------------------------------------------------
    # ######################################################################################
    def CL_IPv6_GetARtable(self):
        class CL_IPv6_ARtableBuffer(Structure):
            nrELM = 200 # modify this is expected more than 200 address in ARtable
            nBytes = 2 + 22*nrELM  # = 2 + 2200 = 2202 bytes
            _pack_ = 1
            _fields_ = [("ARtableElm", c_uint8 * nBytes)] # 22 bytes per elm + 2 bytes (size)
        cl_ipv6_AR_session = CL_IPv6_ARtableBuffer()
        ret = self.bndll.CL_IPv6_GetARTable(pointer(cl_ipv6_AR_session))
        if (ret!=0):
            print ("CL_IPv6_GetARtable - ERROR "+str(ret)+" -  self.bndll.CL_IPv6_GetARTable(pointer(cl_ipv6_AR_session))") ####
            return []
        addrResTableLen = (((cl_ipv6_AR_session.ARtableElm[1] << 8) + (cl_ipv6_AR_session.ARtableElm[0]))) # size
        print (" AR table length = " + str(addrResTableLen))
        # build the data structure:
        addrResTable = []
        eui48Size = 6
        ipv6addrSize = 16
        sessionSize = eui48Size + ipv6addrSize # 16 + 6 = 22 bytes
        
        # build the Unicast Session structure
        for i in range(0,addrResTableLen):
            # build the EUI48 field
            sEui48 = []
            for j in range(0,6):
                k = 2 + i*sessionSize + j
                sEui48+=[cl_ipv6_AR_session.ARtableElm[ k ]]
            # build the IPv6 field
            sIPv6LocalAddr = []          
            for j in range(0,16):
                k = 2 + i*sessionSize + eui48Size + j
                sIPv6LocalAddr+=[ cl_ipv6_AR_session.ARtableElm[ k ] ]
            # build the Unicast Session Element
            addrResTable+=[[sEui48 , sIPv6LocalAddr]]
        return addrResTable
    # ######################################################################################
    # --------------------------------------------------------------------------------------
    def CL_IPv6_GetUCASTSession(self):
        class CL_IPv6_UCASTSessionBuffer(Structure):
            _pack_ = 1
            _fields_ = [("UCASTSession", c_uint8 * 512)] #################### TODO: is 512 ok ?
        cl_ipv6_session = CL_IPv6_UCASTSessionBuffer()
        ret = self.bndll.CL_IPv6_GetUCASTSession(pointer(cl_ipv6_session))
        if (ret!=0):
            return []
        leng = (((cl_ipv6_session.UCASTSession[1] << 8) + (cl_ipv6_session.UCASTSession[0])))
        # build the data structure:
        uSessions=[]
        eui48Size=6
        ipv6addrSize=16
        sessionSize=eui48Size+2*ipv6addrSize 
        
        # build the Unicast Session structure
        for i in range(0,leng):
            # build the EUI48 field
            sEui48 = []
            for j in range(0,6):
                k = 2 + i*sessionSize + j
                sEui48+=[cl_ipv6_session.UCASTSession[ k ]]
                
            # build the local IPv6 field
            sIPv6LocalAddr = []          
            for j in range(0,16):
                k = 2 + i*sessionSize + eui48Size + j
                sIPv6LocalAddr+=[ cl_ipv6_session.UCASTSession[ k ] ]

            # build the remote IPv6 field       
            sIPv6RemoteAddr = []
            for j in range(0,16):
                k = 2 + i*sessionSize + eui48Size + ipv6addrSize + j
                sIPv6RemoteAddr+=[cl_ipv6_session.UCASTSession[ k ]]
            # build the Unicast Session Element
            uSessions+=[[sEui48,sIPv6LocalAddr,sIPv6RemoteAddr]]
        return uSessions
    # ======================================================================================

    def DFU_Init_Req(self, file_name, page_size, send_mode, SignatureAlgorithm = 0, SignatureLength = 0, restart_time_mode = 1, restart_timer = 60, safety_timer = 7200):
        class DLL_InitReq_t(Structure):
            _pack_ = 1
            _fields_ = [("FileUpgrade", POINTER(c_char * 256)),
                        ("PageSize", c_uint8),
                        ("PagesSendMode", c_uint8),
                        ("SignatureAlgorithm", c_uint8),
                        ("SignatureLength", c_uint8),
                        ("RestartTimeMode", c_uint8),
                        ("RestartTimer", c_uint16),
                        ("SafetyTimer", c_uint16)]
        if sys.version < '3':
            buf_name = create_string_buffer(file_name, 256)
        else:
            buf_name = create_string_buffer(bytes(file_name, "latin1"), 256)
        
        print("DFU_Init_Req page_size " + str(page_size))
        init_req = DLL_InitReq_t(pointer(buf_name), page_size, send_mode, SignatureAlgorithm, SignatureLength, restart_time_mode, restart_timer, safety_timer)
        return self.bndll.DFU_Init_Req(init_req)

    def DFU_SNsInit(self, nodes_eui48_list=[]):
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("PagesSendMode", c_uint8 * 160770)]
        InitReq = []
        for i in range (0, len(nodes_eui48_list)):
            for k in range(0, 6):
                InitReq += [nodes_eui48_list[i][k]]
            for k in range(0, 16):
                InitReq += [0]
            InitReq += [0]
            for k in range(1, 133):
                InitReq += [0]
        numOfSN = len(nodes_eui48_list)
        sn_init_req = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(InitReq)))
        return self.bndll.DFU_SNsInit(pointer(sn_init_req))

    def DFU_StartUpgrade(self, nodes_eui48_list=[]):
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("PagesSendMode", c_uint8 * 160770)]
        StartReq = []
        for i in range (0, len(nodes_eui48_list)):
            for k in range(0, 6):
                StartReq += [nodes_eui48_list[i][k]]
            for k in range(0, 16):
                StartReq += [0]
            StartReq += [0]
            for k in range(1, 133):
                StartReq += [0]
        numOfSN = len(nodes_eui48_list)
        start_req = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(StartReq)))
        fail_req = DLL_Msg_t(0, (c_uint8 * 160770)(*([])))
        return self.bndll.DFU_StartUpgrade(pointer(start_req), pointer(fail_req))

    def DFU_Exec(self, nodes_eui48_list=[]):
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("PagesSendMode", c_uint8 * 160770)]
        Exec = []
        for i in range (0, len(nodes_eui48_list)):
            for k in range(0, 6):
                Exec += [nodes_eui48_list[i][k]]
            for k in range(0, 16):
                Exec += [0]
            Exec += [0]
            for k in range(1, 133):
                Exec += [0]
        numOfSN = len(nodes_eui48_list)
        exec_ = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(Exec)))
        fail_req = DLL_Msg_t(0, (c_uint8 * 160770)(*([])))
        return self.bndll.DFU_Exec(pointer(exec_), pointer(fail_req))

    def DFU_SingleNode_State_Req(self, node_eui48=[]):     
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("DLLMsgList", c_uint8 * 160770)]
                       
        numOfSN = 1
        StateReq = []
 
        for k in range(0, 6):
            StateReq += [node_eui48[k]]
        for k in range(0, 16):
            StateReq += [0]
        StateReq += [0]
        for k in range(1, 133):
            StateReq += [0]
                
        state_req = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(StateReq)))
        fail_req = DLL_Msg_t(0, (c_uint8 * 160770)(*([])))               
        return self.bndll.DFU_State_Req(pointer(state_req), pointer(fail_req))
    
    def DFU_SingleNode_State_Res(self, node_eui48=[]): 
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                        ("DLLMsgList", c_uint8 * 160770)]
        state_res = DLL_Msg_t()
        self.bndll.DFU_State_Req(pointer(state_res))
        #for each SN in the list
        for i in range (0, state_res.NumofSNs):
            #for the EUI48 is the same EUI48 passed to the method
            if (node_eui48[i] == state_res.DLLMsgList[i + 133]):
                return state_res.DLLMsgList[22]  
        return []
    
    def waitExceptionState(self, node_eui48, timeout=400):    
        while timeout > 0: 
            sys.stdout.write(".")
            sys.stdout.flush()
            time.sleep(5)
            timeout -= 5
            self.DFU_SingleNode_State_Req(node_eui48)
            if (self.DFU_SingleNode_State_Res(node_eui48) == 5):
                return True
        return False    
        
               
    def DFU_Get_PhaseInfo(self, eui=0):
    
        class DLL_Info_Res_t(Structure):
            _pack_ = 1
            _fields_ = [("Firmware", c_uint8 ),
                        ("State", c_uint8 ),
                        ("Percent", c_uint8 ),
                        ("FwVerLen", c_uint8 ),
                        ("FwVersion", c_uint8 * 30 ),
                        ("VendorLen", c_uint8 ),
                        ("VendorID", c_uint8 * 30),
                        ("ModelLen", c_uint8 ),
                        ("ModelID", c_uint8 * 30),
                        ("Phase", c_uint8 ),
                        ("Dummy", c_uint8 * 38)]

        class DLL_Msg_List_t(Structure):
            _pack_ = 1
            _fields_ = [("EUI48", c_uint8 * 6),
                       ("SerialNumber", c_uint8 *  16),
                       ("InfoRes", DLL_Info_Res_t)]

        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("MsgList", DLL_Msg_List_t * ((0x7FF) + 1))]

        InfoResBuf= DLL_Msg_t()     
        InfoReqBuf= DLL_Msg_t()
        InfoFailBuf= DLL_Msg_t()      
        InfoReqBuf.NumofSNs = 1
        InfoReqBuf.MsgList[0].EUI48 = (c_uint8 * 6)(*eui)
        self.bndll.DFU_Info_Req(pointer(InfoReqBuf),pointer(InfoFailBuf))
        self.bndll.DFU_Info_Res(pointer(InfoResBuf))       
        return InfoResBuf.MsgList[0].InfoRes.Phase   

    def DFU_StopUpgrade(self, nodes=[]):
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("PagesSendMode", c_uint8 * 160770)]
        nodeList = []
        for i in range (0, len(nodes)):
            for k in range(0, 6):
                nodeList += [nodes[i][k]]
            for k in range(0, 16):
                nodeList += [0]
            nodeList += [0]
            for k in range(1, 133):
                nodeList += [0]
        numOfSN = len(nodes)
        stop_req = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(nodeList)))
        fail_req = DLL_Msg_t(0, (c_uint8 * 160770)(*([])))
        return self.bndll.DFU_StopUpgrade(pointer(stop_req), pointer(fail_req))

    def DFU_CloseConn(self, nodes=[]):
        class DLL_Msg_t(Structure):
            _pack_ = 1
            _fields_ = [("NumofSNs", c_uint16),
                       ("PagesSendMode", c_uint8 * 160770)]
        nodeList = []
        for i in range (0, len(nodes)):
            for k in range(0, 6):
                nodeList += [nodes[i][k]]
            for k in range(0, 16):
                nodeList += [0]
            nodeList += [0]
            for k in range(1, 133):
                nodeList += [0]
        numOfSN = len(nodes)
        close_req = DLL_Msg_t(numOfSN, (c_uint8 * 160770)(*(nodeList)))
        fail_req = DLL_Msg_t(0, (c_uint8 * 160770)(*([])))
        return self.bndll.DFU_CloseConn(pointer(close_req), pointer(fail_req))

    def LOCAL_MGMT_bytes_received(self, buff):
        class SimpleBuffer(Structure):
            _fields_ = [("buffer", c_uint8 * (len(buff)))]
        c_buff = SimpleBuffer((c_uint8 * len(buff))(*(buff)))
        self.bndll.LOCAL_MGMT_bytes_received(pointer(c_buff), c_uint16(len(buff)))

    def LOCAL_MGMT_bytes_to_transmit(self):
        class SimpleBuffer(Structure):
            _fields_ = [("buffer", c_uint8 * (4096))]
        buf = SimpleBuffer()
        func_out = self.bndll.LOCAL_MGMT_bytes_to_transmit
        func_out.argtypes = [POINTER(POINTER(c_uint8))]
        func_out.restype = c_uint16
        u8pnt = POINTER(c_uint8)()
        count = func_out(byref(u8pnt))  
        self.bndll.LOCAL_MGMT_bytes_to_transmit.restype = c_int
        memmove(buf.buffer, u8pnt, count)
        return buf.buffer[0:count]
        
    def PHY_DATAconfirm(self, res=0):
        class Result(Structure):
            _fields_ = [("res", c_uint8)]
        conf = Result(res)
        ret =  self.bndll.PHY_DATAconfirm(pointer(conf))
        return ret, conf.res

    def PHY_DATArequest(self,  level, scheme, timer, psdu=[]):
        class PHY_DataRequest_t(Structure):
            _pack_ = 1
            _fields_ = [("uiLength", c_uint16),
                        ("ucLevel", c_uint8),
                        ("ucScheme", c_uint8),
                        ("ulTimer", c_uint32),
                       ("pucPsdu", c_uint8 * 385)]
        req = PHY_DataRequest_t(len(psdu), level, scheme, timer, (c_uint8 * 385)(*psdu))
        ret = self.bndll.PHY_DATArequest(pointer(req))
        return ret

    def PHY_DATAindication(self):
        class PHY_DataIndication_t(Structure):
            _pack_ = 1
            _fields_ = [("EC", c_uint8),
                        ("Length", c_uint16),
                        ("Level", c_uint8),
                        ("Scheme", c_uint8),
                        ("Type", c_uint8),
                        ("SNR", c_uint8),
                        ("RQ", c_uint8),
                        ("Timer", c_uint32),
                        ("Psdu", c_uint8 * 384)]
        ind = PHY_DataIndication_t()
        ret = self.bndll.PHY_DATAindication(pointer(ind))
        if ret == 0:
            return ret, ind.Length, ind.Level, ind.Scheme, ind.Type, ind.SNR, ind.RQ, ind.Timer, ind.Psdu   
        return ret, 0, 0, 0, 0, 0, 0, 0, []

class LocalManagement():

    def __init__(self, dllwrp):
        self.dllwrp = dllwrp

    class MgmtCommands:
        Get =            0x00
        Set =            0x02
        Enhanced =       0x06

    class EnhancedType:
        ListQuery =     0x0E
        ListResponse =  0x0F

    class RejectType:
        PRO_REQ =       0x00
        CON_REQ =       0x02
        REG_REQ =       0x03

    class MgmtPibs(SpecPibs):
        ''' enhanced pib attributes/actions (see queries) '''

    def SendBuffer(self, cmd, buff):
        tmp = (len(buff) << 6) + cmd
        header = [tmp >> 8, tmp & 0xFF]
        msg = header + buff
        self.dllwrp.LOCAL_MGMT_bytes_received(msg)

    def GetBuffer(self):
        buff = self.dllwrp.LOCAL_MGMT_bytes_to_transmit()
        if len(buff) < 2:
            return 0, []
        leng = (buff[0] << 2) + (buff[1] >> 6)
        cmd = buff[1] & 0x3F
        return cmd, buff[2:2+leng]

    def SetPib(self, pib, arr_value):
        buff = [(pib >> 8) & 0xFF, pib & 0xFF]
        buff += arr_value
        self.SendBuffer(self.MgmtCommands.Set, buff)
        time.sleep(1)

    def GetPib(self, pib, timeout=3):
        buff = [(pib >> 8) & 0xFF, pib & 0xFF, 0x00]
        self.SendBuffer(self.MgmtCommands.Get, buff)
        endtime = time.time() + timeout
        while True:
            _, msg = self.GetBuffer()
            if len(msg) > 0:
                arr_msg = msg[3:-1]
                break
            if msg == [] and time.time() > endtime:
                return None
            time.sleep(0.015)
        if len(arr_msg) == 1:
            val = arr_msg[0] # cast to uint8_t
        elif len(arr_msg) == 2:
            val = (arr_msg[0] << 8) + arr_msg[1]  # cast to uint16_t
        elif len(arr_msg) == 4:
            val = (arr_msg[0] << 24) + (arr_msg[1] << 16) + (arr_msg[2] << 8) + arr_msg[3]  # cast to uint32_t
        else:
            val = arr_msg
        return val

    def GetPibList(self, pib):
        iterator = 0
        lis = []
        while True:
            buff = [self.EnhancedType.ListQuery]
            buff += [(pib >> 8) & 0xFF, pib & 0xFF]
            buff += [16] # 16 entries at a time
            buff += [(iterator >> 8) & 0xFF, iterator & 0xFF]
            self.SendBuffer(self.MgmtCommands.Enhanced, buff)
            while True:
                _, msg = self.GetBuffer()
                if len(msg) > 0:
                    break
                time.sleep(0.015)
            num = msg[3]
            #end = msg[4]
            leng = msg[4]
            for i in range (0, num):
                lis += [msg[i * (2 + leng) + 7:i * (2 + leng) + 7 + leng]]
            if num < 16:
                break
            iterator += 16
        return lis

    def ForceAliveTime(self, alv_time):
        arr_val = [alv_time]
        self.SetPib(self.MgmtPibs.macAliveTime, arr_val)     
    
    def IsNodeRegistered(self,eui48):
        lis = self.GetPibList(self.MgmtPibs.macListRegDevices)
        if lis == []:
            return False
        for entry in lis:
            if eui48 == entry[0:6]:
                return True
        return False
    
    def Unregister(self, eui):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        self.SetPib(self.MgmtPibs.macActionUnregisterBN, arr_val)

    def Promote(self, eui):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        self.SetPib(self.MgmtPibs.macActionPromote, arr_val)
    
    def ProBcnModChange(self, eui, enable):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        arr_val += [enable & 0x1]
        self.SetPib(self.MgmtPibs.macActionProBcn, arr_val)
    
    def PromoteDS(self, eui, enc):
        allowedEnc = [Modulation.PROMOD_DBPSKCC, Modulation.PROMOD_DQPSKRC, Modulation.PROMOD_DBPSKRC]
        if enc in allowedEnc:
            arr_val = [] # avoid to modify caller eui
            arr_val += eui
            arr_val += [enc]
            self.SetPib(self.MgmtPibs.macActionPromoteDS, arr_val)

    def Demote(self, eui):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        self.SetPib(self.MgmtPibs.macActionDemote, arr_val)

    def _StartStopReject(self, eui, rej, typ):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        arr_val += [rej, typ]
        self.SetPib(self.MgmtPibs.macActionReject, arr_val)

    def StartReject(self, eui, typ):
        self._StartStopReject(eui, 1, typ)

    def StopReject(self, eui, typ):
        self._StartStopReject(eui, 0, typ)
    
    def BroadcastDataBurst(self, num, dlen, duty, lcid, prio):
        arr_val = [(num >> 24) & 0xFF, (num >> 16) & 0xFF, (num >> 8) & 0xFF, num & 0xFF]
        arr_val += [dlen, duty]
        arr_val += [lcid >> 8, lcid & 0xFF]
        arr_val += [prio]
        self.SetPib(self.MgmtPibs.macActionBroadcastDataBurst, arr_val)

    def _OpenCloseMgmtCon(self, eui, connect):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        arr_val += [connect]
        self.SetPib(self.MgmtPibs.macActionMgmtCon, arr_val)

    def OpenManagementConnection(self, eui):
        self._OpenCloseMgmtCon(eui, 1)

    def CloseManagementConnection(self, eui):
        self._OpenCloseMgmtCon(eui, 0)

    def _JoinLeaveMgmtMul(self, eui, join):
        arr_val = [] # avoid to modify caller eui
        arr_val += eui
        arr_val += [join]
        self.SetPib(self.MgmtPibs.macActionMgmtMul, arr_val)

    def JoinManagementConnection(self, eui):
        self._JoinLeaveMgmtMul(eui, 1)

    def LeaveManagementConnection(self, eui):
        self._JoinLeaveMgmtMul(eui, 0)
    
    def SendAppemuDataBurst(self, eui48, number, datalen, dutycycle=None):
        params = [] # avoid to modify caller eui
        params += eui48
        params += [(number >> 24) & 0xFF, (number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF]
        params += [datalen]
        params += [dutycycle if dutycycle is not None else 100]
        self.SetPib(SpecPibs.macActionAppemuDataBurst, params)
    
    def SendManagementDataBurst(self, eui48, number, datalen, dutycycle=None):
        params = [] # avoid to modify caller eui
        params += eui48
        params += [(number >> 24) & 0xFF, (number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF]
        params += [datalen]
        params += [dutycycle if dutycycle is not None else 100]
        self.SetPib(SpecPibs.macActionMgmtDataBurst, params)
    
    def ReadActiveConnEx(self, eui48):
        lis = self.GetPibList(self.MgmtPibs.macListActiveConnEX)
        for entry in lis:
            if eui48 == entry[6:12]:
                sid  = (entry[0]<<8) + entry[1]
                lnid = (entry[2]<<8) + entry[3]
                lcid = (entry[4]<<8) + entry[5]
                typefield = entry[12]
                return True, sid, lnid, lcid, typefield
        return False, 0, 0, 0, 0   
