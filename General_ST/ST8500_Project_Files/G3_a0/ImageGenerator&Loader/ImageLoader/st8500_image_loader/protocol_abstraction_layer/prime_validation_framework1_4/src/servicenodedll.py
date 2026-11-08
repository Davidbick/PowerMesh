import sys
import re
from threading import Thread, Lock
import time
import os.path
from ctypes import cdll, Structure, c_uint8, c_uint16, c_uint32, pointer, c_char_p, POINTER
from servicenode import ServiceNodeAbstract
from protocol import Modulation
from global_prime_defs import Vendor

class ServiceNodeDll(ServiceNodeAbstract):
    InitFailure = True
    fsmthreadrunning = False
    image_letter_version = ord("c")
    ResponseAnswer = 0
    ManageIndications = False
    PrimeLibConfigMask = 0x0001

    def __init__(self, comport, baud="ignored"):
        self.comport = comport
        try:
            self.dllwrp = SnDllWrapper(self.comport)
        except:
            sys.stderr.write("SN library not present\n")
            time.sleep(0.5) # fake init time
            return
        
        if self.dllwrp.UartInit(self.comport) != 0:
            time.sleep(0.5) # fake init time
            return
        self.InitFailure = False
        self.manageStartupLBI()
        self.dllwrp.PRIME_LIB_Config(self.PrimeLibConfigMask)
        self.ProductID = Vendor.ProductId.SN_STM32        
        self.Start()
        time.sleep(5)
        for _ in range(0,10):
            self.eui48 = self.GetPib(0x8040)
            if self.eui48 != None and self.eui48 != [0,0,0,0,0,0]:
                return
            time.sleep(1)
        self.Stop()
        self.InitFailure = True
        self.defTestModeMask = self.TestMode.TmReset
        self.currTestModeMask = self.TestMode.TmReset

    def __del__(self):
        self.Stop()
        if hasattr(self, 'dllwrp'):
            self.dllwrp.UartClose()
            del self.dllwrp

    def manageStartupLBI(self):
        LBI_ACTION_TODO_MASK      = 0xC0
        LBI_LOAD_NEW_LIBRARY      = 0x40
        LBI_LOAD_STABLE_LIBRARY   = 0x80
        
        lbi_file_name = "lbi_config" # cannot be changed, defined in lib_prime_sn.dll

        if not os.path.isfile(lbi_file_name):
            # create first lbi_config
            with open(lbi_file_name, "wb") as fil:
                fil.write(bytes(b'\x15\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'))
        else:
            with open(lbi_file_name, "rb") as fil:
                lbi = bytearray(fil.read())
                if (lbi[0] & LBI_ACTION_TODO_MASK) == LBI_LOAD_NEW_LIBRARY:
                    lbi[0] = (lbi[0] & ~LBI_ACTION_TODO_MASK) | LBI_LOAD_STABLE_LIBRARY
            with open(lbi_file_name, "rb+") as fil:
                fil.write(lbi)

        try:
            if os.path.isfile("dfu_image_received"):
                #getting letter version from dfu image
                with open("dfu_image_received","rb") as f:
                    s = f.read()
                m = re.search("LeTtErVeRsIoN:(.)", s)
                self.image_letter_version = ord(m.group(1))
        except:
            pass

    def FakeRestart(self):
        self.dllwrp.UartClose()
        self.dllwrp.UartInit(self.comport)
        self.manageStartupLBI()
        self.dllwrp.PRIME_LIB_Config(self.PrimeLibConfigMask)

    def FSMThread(self):

        PRIME_LIB_GET_INFO_HOST_APPLI_RESET_ASSERT_WAIT_MASK = 0x0008
        PRIME_LIB_GET_INFO_LIB_RESET_ASSERT_WAIT_MASK = 0x0004
        PRIME_LIB_GET_INFO_ST7590_RESET_ASSERT_WAIT_MASK = 0x0002
        PRIME_LIB_SET_STATE_INIT_LIB_MASK = 0x0001
        PRIME_LIB_SET_STATE_ST7590_RESET_ASSERT_MASK = 0x0002

        MANAGED_INDICATIONS = (self.Indication.MAC_ESTABLISHINDICATION |
                               self.Indication.MAC_RELEASEINDICATION |
                               self.Indication.MAC_JOININDICATION |
                               self.Indication.MAC_LEAVEINDICATION)
        
        u16GetInfoReg = c_uint16()

        while self.fsmthreadrunning:
            while self.fsmthreadrunning:
                u16GetInfoReg = self.dllwrp.PRIME_LIB_GetInfo()

                if (u16GetInfoReg.value & PRIME_LIB_GET_INFO_HOST_APPLI_RESET_ASSERT_WAIT_MASK) != 0:
                    break
                elif (u16GetInfoReg.value & PRIME_LIB_GET_INFO_LIB_RESET_ASSERT_WAIT_MASK) != 0:
                    self.dllwrp.PRIME_LIB_SetState(PRIME_LIB_SET_STATE_INIT_LIB_MASK)

                elif (u16GetInfoReg.value & PRIME_LIB_GET_INFO_ST7590_RESET_ASSERT_WAIT_MASK) != 0:
                    self.dllwrp.Reset_ST7590_NoCallback()
                    self.dllwrp.PRIME_LIB_SetState(PRIME_LIB_SET_STATE_ST7590_RESET_ASSERT_MASK)

                self.dllwrp.PRIME_LIB_FsmEvolve()
                if self.ManageIndications:
                    state = self.GetState() & MANAGED_INDICATIONS
                    if state != 0:
                        if state & self.Indication.MAC_ESTABLISHINDICATION != 0:
                            self.ManageClNullEstablishIndication()
                        if state & self.Indication.MAC_RELEASEINDICATION != 0:
                            self.ManageClNullReleaseIndication()
                        if state & self.Indication.MAC_JOININDICATION != 0:
                            self.ManageClNullJoinIndication()
                        if state & self.Indication.MAC_LEAVEINDICATION != 0:
                            self.ManageClNullLeaveIndication()
                        continue
                time.sleep(0.015)

            if self.fsmthreadrunning:
                time.sleep(1)
                self.FakeRestart()
                self.dllwrp.PRIME_LIB_SetState(PRIME_LIB_SET_STATE_INIT_LIB_MASK)
            time.sleep(1)

    def Start(self):
        if not self.fsmthreadrunning:
            self.fsmthreadrunning = True
            self.fsmthread = Thread(target=self.FSMThread)
            self.fsmthread.start()

    def Stop(self):
        if self.fsmthreadrunning:
            self.fsmthreadrunning = False
            self.fsmthread.join()
            del self.fsmthread

    def NodeInfo(self):
        fwver = self.GetPib(self.Pibs.AppFwVersion)
        txt = ":".join("{:02X}".format(c) for c in self.eui48) + ", "
        txt += "SN DLL "
        txt += "".join(chr(c) for c in fwver).strip("\0")
        return txt

    def SetPib(self, pib, arrvalue, iface="SCI"):

        ret = self.dllwrp.MAC_MLME_SETrequest(pib, arrvalue)
        if ret == 0:
            while True:
                ret, result = self.dllwrp.MAC_MLME_SETconfirm()
                if ret == 0:
                    break
            if result == 0:
                return True
        return False

    def GetPib(self, pib, iface="SCI", endianness=0):
        ret = self.dllwrp.MAC_MLME_GETrequest(pib, endianness)
        if ret == 0:
            while True:
                ret, status, _, val = self.dllwrp.MAC_MLME_GETconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if status == 0:
                if pib == self.Pibs.AppFwVersion:
                    val[7] = self.image_letter_version # ugly work around
                return val
        return None

    def GetPibList(self, pib, iface="SCI", endianness=0):
        self.dllwrp.MAC_MLME_LIST_GETrequest(pib, endianness)
        _, _, _, arr = self.dllwrp.MAC_MLME_LIST_GETconfirm()

        # how to get list entry size?
        if pib == self.Pibs.macListSwitchTable:
            entry_size = 5
        elif pib == self.Pibs.macListAvailableSwitches:
            entry_size = 10
        elif pib == self.Pibs.macListPhyComm:
            entry_size = 10
        else:
            return []

        lst = []
        for j in range (0, int(len(arr)/entry_size)):
            lst += [arr[j*entry_size:(j+1)*entry_size]]
        return lst
    
    def SetTestMode(self, inTestModeMask):
        self.currTestModeMask |= inTestModeMask
        self.dllwrp.CMD_MAC_TESTmode(self.currTestModeMask)
    
    def ClearTestMode(self, inTestModeMask=None):
        if inTestModeMask is None:
            inTestModeMask = ~self.defTestModeMask
        self.currTestModeMask &= ~inTestModeMask
        self.dllwrp.CMD_MAC_TESTmode(self.currTestModeMask)
    
    def InitTestMode(self, inTestModeMask=None):
        if inTestModeMask is not None:
            self.defTestModeMask = inTestModeMask
        self.currTestModeMask = self.defTestModeMask
        self.dllwrp.CMD_MAC_TESTmode(self.currTestModeMask)

    def GetTestMode(self):
        return self.currTestModeMask
        
    def GetDefTestMode(self):
        return self.defTestModeMask
    
    def SwReset(self):
        pass

    def SerialComClose(self):
        pass

    def SerialComOpen(self):
        pass

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
        return state

    def InitiatePromotion(self):
        if self.dllwrp.MAC_PROMOTIONrequest() == 0:
            while True:
                ret, res = self.dllwrp.MAC_PROMOTIONconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def InitiateDemotion(self):
        if self.dllwrp.MAC_DEMOTIONrequest() == 0:
            while True:
                ret, res = self.dllwrp.MAC_DEMOTIONconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def InitiateUnregister(self):
        if self.dllwrp.MAC_UNREGISTERrequest() == 0:
            while True:
                ret, res = self.dllwrp.MAC_UNREGISTERconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def CL432GetSNSession(self):
        return self.dllwrp.CL432GetSNSession()

    def CL432Establish(self, ser_num):
        if self.dllwrp.CL_432_ESTABLISHrequest(ser_num) == 0:
            while True:
                ret, dest_addr, base_addr, ser = self.dllwrp.CL_432_ESTABLISHconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if ser_num == ser:
                return True, dest_addr, base_addr
        return False, 0, 0

    def CL432Release(self, dst_addr):
        if self.dllwrp.CL_432_RELEASErequest(dst_addr) == 0:
            while True:
                ret, d_addr, _ = self.dllwrp.CL_432_RELEASEconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if dst_addr == d_addr:
                return True
        return False

    def CL432GetData(self, dst_addr, src_addr):
        ret, _, _, d_addr, s_addr, _, data = self.dllwrp.DL_DATAindication(dst_addr, src_addr)
        if ret == 0 and s_addr == src_addr and d_addr == dst_addr:
            return data
        return []

    def CL432SendData(self, dst_addr, src_addr, data):
        if self.dllwrp.DL_DATArequest(dst_addr, src_addr, data) == 0:
            while True:
                ret, _, _, _, _, _ = self.dllwrp.DL_DATAconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True
        return False

    def CL_NULL_Establish(self, eui=[0,0,0,0,0,0], typ=0, arq=False, data=[]):
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

    def CL_NULL_GetData(self):
        ret, ch, _, data = self.dllwrp.CL_NULL_DATAindication()
        if ret == 0:
            return ch, data
        return 0, []

    def CL_NULL_SendData(self, ch, buff, prio=2):
        if self.dllwrp.CL_NULL_DATArequest(ch, buff, prio) == 0:
            while True:
                ret, _, res, _, _ = self.dllwrp.CL_NULL_DATAconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            if res == 0:
                return True
        return False

    def CL_NULL_RMISendData(self, ch, buff, prio=1):
        if self.dllwrp.CL_NULL_DATArequest(ch, buff, prio) == 0:
            return True
        return False              

    def CL_NULL_Join(self, bcast, eui, data=[]):
        if self.dllwrp.CL_NULL_JOINrequest(bcast, 0, eui, 0, data)== 0:
            while True:
                ret, ch, _, res, _ = self.dllwrp.CL_NULL_JOINconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, ch, res
        return False, 0, 0

    def CL_NULL_Leave(self, ch, eui):
        if self.dllwrp.CL_NULL_LEAVErequest(ch, eui) == 0:
            while True:
                ret, ch, res, _ ,_ = self.dllwrp.CL_NULL_LEAVEconfirm()
                if ret == 0:
                    break
                time.sleep(0.015)
            return True, ch, res
        return False, 0, 0

    def PHY_GetData(self):
        ret, lev, sch, tim, snr, rssi, di = self.dllwrp.PHY_DATAindication()
        if ret == 0:
            return di, lev, sch, tim, snr, rssi
        return [], 0, 0, 0, 0, 0

    def PHY_SendData(self, data, level=7, scheme=Modulation.TYPEA_DBPSKCC, timer=0, t_sleep=0.015):
        if self.dllwrp.PHY_DATArequest(data, level, scheme, timer) == 0:
            while True:
                ret, res = self.dllwrp.PHY_DATAconfirm()
                if ret == 0:
                    break
                time.sleep(t_sleep)
            if res == 0:
                return True
        return False

    def IsUnregisterIndicationPresent(self):
        return self.dllwrp.MAC_UNREGISTERindication() == 0

    def IsPromotionIndicationPresent(self):
        return self.dllwrp.MAC_PROMOTIONindication() == 0

    def IsDemotionIndicationPresent(self):
        return self.dllwrp.MAC_DEMOTIONindication() == 0

    def CLNULLDataLoopThread(self,ch):
        while self.CLNULLDataLooprunning:                    
            buff = list(range(0, 50))  
            self.CL_NULL_SendData(ch, buff, prio=2)
            
        time.sleep(0.25)
    def StartManagingIndications(self, answer):
        self.ResponseAnswer = answer
        self.ManageIndications = True

    def StopManagingIndications(self):
        self.ManageIndications = False

    def StartSNDataLoop(self,ch):
        if not self.CLNULLDataLooprunning:
            self.CLNULLDataLooprunning = True
            self.CLNULLDataLoopthread = Thread(target=self.CLNULLDataLoopThread,args = [ch])
            self.CLNULLDataLoopthread.start()
            

    def StopSNDataLoop(self):
        if self.CLNULLDataLooprunning:
            self.CLNULLDataLooprunning = False
            self.CLNULLDataLoopthread.join()
            del self.CLNULLDataLoopthread

    def WaitClNullEstablishRequest (self, response=0, timeout=20):
        endtime = time.time() + timeout
        while True:
            state = self.GetState()
            if state & self.Indication.MAC_ESTABLISHINDICATION != 0:
                ret, ch, _, _, _, data = self.dllwrp.CL_NULL_ESTABLISHindication()
                if ret == 0:
                    self.dllwrp.CL_NULL_ESTABLISHresponse(ch, response, data)
                    return True, data[0:(len(data))]
                else:
                    return False, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, []

    def ManageClNullEstablishIndication(self):
        ret, ch, _, _, _, _ = self.dllwrp.CL_NULL_ESTABLISHindication()
        if ret == 0:
            self.dllwrp.CL_NULL_ESTABLISHresponse(ch, self.ResponseAnswer, [])

    def ManageClNullReleaseIndication(self):
        ret, ch, reason, _ = self.dllwrp.CL_NULL_RELEASEindication()
        if ret == 0:
            if reason == 0:
                self.dllwrp.CL_NULL_RELEASEresponse(ch, self.ResponseAnswer)

    def WaitClNullJoinIndication(self, response=0, timeout=20):
        endtime = time.time() + timeout
        while True:
            state = self.GetState()
            if state & self.Indication.MAC_JOININDICATION != 0:
                ret, ch, eui, _, data = self.dllwrp.CL_NULL_JOINindication()
                if ret == 0:
                    self.dllwrp.CL_NULL_JOINresponse(ch, eui, response)
                    return True, ch, data
                else:
                    return False, 0, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, 0, []

    def ManageClNullJoinIndication(self):
        ret, ch, eui, _, _ = self.dllwrp.CL_NULL_JOINindication()
        if ret == 0:
            self.dllwrp.CL_NULL_JOINresponse(ch, eui, self.ResponseAnswer)

    def ManageClNullLeaveIndication(self):
        _, _, _, _ = self.dllwrp.CL_NULL_LEAVEindication()

class ServiceNodeDllQfn48(ServiceNodeDll):
    PrimeLibConfigMask = 0x0002

class SnDllWrapper():

    def __init__(self, comport):

        self.biglock = Lock()
        if comport.startswith("COM"):
            self.sndll = cdll.LoadLibrary("../libraries/lib_prime_sn.dll")
        else:
            self.sndll = cdll.LoadLibrary("../libraries/lib_prime_sn.so")

    def UartInit(self, comport):

        if comport.startswith("COM"):
            comport = "\\\\.\\" + comport

        self.biglock.acquire()
        ret = self.sndll.UartInit(c_char_p(comport.encode()))
        self.biglock.release()
        return ret

    def PRIME_LIB_Config(self, config):

        self.biglock.acquire()
        self.sndll.PRIME_LIB_Config(c_uint16(config))
        self.biglock.release()

    def UartClose(self):

        self.biglock.acquire()
        self.sndll.UartClose()
        self.biglock.release()

    def PRIME_LIB_GetInfo (self):
        u16GetInfoReg = c_uint16()

        self.biglock.acquire()
        self.sndll.PRIME_LIB_GetInfo(pointer(u16GetInfoReg))
        self.biglock.release()
        return u16GetInfoReg

    def PRIME_LIB_SetState(self, state):

        self.biglock.acquire()
        self.sndll.PRIME_LIB_SetState(state)
        self.biglock.release()

    def Reset_ST7590_NoCallback(self):

        self.biglock.acquire()
        self.sndll.Reset_ST7590_NoCallback()
        self.biglock.release()

    def PRIME_LIB_FsmEvolve(self):

        self.biglock.acquire()
        self.sndll.PRIME_LIB_FsmEvolve()
        self.biglock.release()

    def CL_NULL_DATArequest(self, ch, buff, prio):
        class CL_NULL_TxRequest_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("DataLength", c_uint16),
                        ("Data", POINTER(c_uint8 * len(buff))),
                        ("Priority", c_uint8)]

        self.data = (c_uint8 * len(buff))(*buff)
        req = CL_NULL_TxRequest_t(ch, len(buff), pointer(self.data), prio)

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_DATArequest(pointer(req))
        self.biglock.release()
        return ret

    def CL_NULL_DATAconfirm(self):
        class CL_NULL_TxConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("Data", c_uint8),
                        ("Type", c_uint8)]

        conf = CL_NULL_TxConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_DATAconfirm(pointer(conf))
        self.biglock.release()
        return ret, conf.ConHandle, conf.Result, conf.Data, conf.Type

    def CL_NULL_DATAindication(self):
        class CL_NULL_DataIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("DataLength", c_uint16),
                        ("Type", c_uint8),
                        ("Data", c_uint8 * 256)]

        ind = CL_NULL_DataIndication_t()
        self.biglock.acquire()
        ret = self.sndll.CL_NULL_DATAindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ConHandle, ind.Type, ind.Data[0:ind.DataLength]
        return ret, 0, 0, []

    def CL_NULL_RELEASErequest(self, ch):

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_RELEASErequest(c_uint32(ch))
        self.biglock.release()
        return ret

    def CL_NULL_RELEASEconfirm(self):
        class CL_NULL_ReleaseConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("AccessType", c_uint8)]

        conf = CL_NULL_ReleaseConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_RELEASEconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.AccessType
        return ret, 0, 0, 0

    def CL_NULL_ESTABLISHrequest(self, eui, typ, arq, data):
        class CL_NULL_EstablishRequest_t(Structure):
            _fields_ = [("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("ARQ", c_uint8),
                        ("CfBytes", c_uint16),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        req = CL_NULL_EstablishRequest_t((c_uint8 * 6)(*(eui)), typ, 1 if arq else 0, 0, len(data), (c_uint8 * 32)(*(data)))


        self.biglock.acquire()
        ret = self.sndll.CL_NULL_ESTABLISHrequest(pointer(req))
        self.biglock.release()
        return ret

    def CL_NULL_ESTABLISHconfirm(self):
        class CL_NULL_EstablishConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        conf = CL_NULL_EstablishConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_ESTABLISHconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.EUI48, conf.Type, conf.Data[0:conf.DataLength]
        return ret, 0, 0, [0,0,0,0,0,0], 0, []

    def CL_NULL_ESTABLISHindication(self):
        class CL_NULL_EstablishIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("CfBytes", c_uint16),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        ind = CL_NULL_EstablishIndication_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_ESTABLISHindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48, ind.Type, ind.CfBytes, ind.Data[0:ind.DataLength]
        return ret, 0, [0,0,0,0,0,0], 0, 0, []

    def CL_NULL_ESTABLISHresponse(self, ch, answer, data):
        class CL_NULL_EstablishResponse_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Answer", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        res = CL_NULL_EstablishResponse_t(ch, answer, len(data), (c_uint8 * 32)(*(data)))

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_ESTABLISHresponse(pointer(res))
        self.biglock.release()
        return ret

    def CL_NULL_RELEASEresponse(self, ch, answer):
        class CL_NULL_ReleaseResponse_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Answer", c_uint8)]

        res = CL_NULL_ReleaseResponse_t(ch, answer)

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_RELEASEresponse(pointer(res))
        self.biglock.release()
        return ret

    def CL_NULL_RELEASEindication(self):
        class CL_NULL_ReleaseIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Reason", c_uint8),
                        ("Type", c_uint8)]

        ind = CL_NULL_ReleaseIndication_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_RELEASEindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ConHandle, ind.Reason, ind.Type
        return ret, 0, 0, 0

    def CL_NULL_JOINrequest(self, bcast, eui, typ, data):
        class CL_NULL_JoinRequest_t(Structure):
            _fields_ = [("Broadcast", c_uint8),
                        ("CH_LCID", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        req = CL_NULL_JoinRequest_t(bcast, 0, (c_uint8 * 6)(*(eui)), typ, len(data), (c_uint8 * 32)(*(data)))

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_JOINrequest(pointer(req))
        self.biglock.release()
        return ret

    def CL_NULL_JOINconfirm(self):
        class CL_NULL_JoinConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Result", c_uint8),
                        ("AccessType", c_uint8)]

        conf = CL_NULL_JoinConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_JOINconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.ConHandle, conf.EUI48[0:6], conf.Result, conf.AccessType
        return ret, 0, [0,0,0,0,0,0], 0, 0

    def CL_NULL_JOINindication(self):
        class CL_NULL_JoinIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8),
                        ("DataLength", c_uint8),
                        ("Data", c_uint8 * 32)]

        ind = CL_NULL_JoinIndication_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_JOINindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48[0:6], ind.Type, ind.Data[0:ind.DataLength]
        return ret, 0, [0,0,0,0,0,0], 0, []

    def CL_NULL_JOINresponse(self, ch, eui, answer):
        class CL_NULL_JoinResponse_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Answer", c_uint8)]

        res = CL_NULL_JoinResponse_t(ch, (c_uint8 * 6)(*(eui)), answer)

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_JOINresponse(pointer(res))
        self.biglock.release()
        return ret

    def CL_NULL_LEAVErequest(self, ch, eui):
        class CL_NULL_LeaveRequest_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6)]

        req = CL_NULL_LeaveRequest_t(ch, (c_uint8 * 6)(*(eui)))

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_LEAVErequest(pointer(req))
        self.biglock.release()
        return ret

    def CL_NULL_LEAVEconfirm(self):
        class CL_NULL_LeaveConfirm_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("Result", c_uint8),
                        ("EUI48", c_uint8 * 6),
                        ("AccessType", c_uint8)]

        conf = CL_NULL_LeaveConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_LEAVEconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.ConHandle, conf.Result, conf.EUI48[0:6], conf.AccessType
        return ret, 0, 0, [0,0,0,0,0,0], 0

    def CL_NULL_LEAVEindication(self):
        class CL_NULL_LeaveIndication_t(Structure):
            _fields_ = [("ConHandle", c_uint32),
                        ("EUI48", c_uint8 * 6),
                        ("Type", c_uint8)]

        ind = CL_NULL_LeaveIndication_t()

        self.biglock.acquire()
        ret = self.sndll.CL_NULL_LEAVEindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ConHandle, ind.EUI48[0:6], ind.Type
        return ret, 0, [0,0,0,0,0,0], 0

    def DL_DATArequest(self, dst_addr, src_addr, data):
        class DL_DataRequest_t(Structure):
            _fields_ = [("DSAP", c_uint8),
                        ("SSAP", c_uint8),
                        ("DestinationAddress", c_uint16),
                        ("SourceAddress", c_uint16),
                        ("LinkClass", c_uint8),
                        ("SDUDataLength", c_uint16),
                        ("SDUData", c_uint8 * 256)]

        req = DL_DataRequest_t(0, 0, dst_addr, src_addr, 0, len(data), (c_uint8 * 256)(*(data)))

        self.biglock.acquire()
        ret = self.sndll.DL_DATArequest(pointer(req))
        self.biglock.release()
        return ret

    def DL_DATAconfirm(self):
        class DL_DataConfirm_t(Structure):
            _fields_ = [("DSAP", c_uint8),
                        ("SSAP", c_uint8),
                        ("DestinationAddress", c_uint16),
                        ("SourceAddress", c_uint16),
                        ("TransmissionStatus", c_uint8)]

        conf = DL_DataConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.DL_DATAconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.DSAP, conf.SSAP, conf.DestinationAddress, conf.SourceAddress, conf.TransmissionStatus
        return ret, 0, 0, 0, 0, 0

    def DL_DATAindication(self, dst_addr, src_addr):
        class DL_DataIndication_t(Structure):
            _fields_ = [("DSAP", c_uint8),
                        ("SSAP", c_uint8),
                        ("DestinationAddress", c_uint16),
                        ("SourceAddress", c_uint16),
                        ("LinkClass", c_uint8),
                        ("SDUDataLength", c_uint16),
                        ("SDUData", c_uint8 * 256)]

        ind = DL_DataIndication_t()

        self.biglock.acquire()
        ret = self.sndll.DL_DATAindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.DSAP, ind.SSAP, ind.DestinationAddress, ind.SourceAddress, ind.LinkClass, ind.SDUData[0:ind.SDUDataLength]
        return ret, 0, 0, 0, 0, 0, []

    def PHY_GetState(self):
        state = c_uint32(0)
        self.biglock.acquire()
        ret = self.sndll.PHY_GetState(pointer(state))
        self.biglock.release()
        return ret, state.value

    def CL_NULL_GetState(self):
        state = c_uint32(0)
        self.biglock.acquire()
        ret = self.sndll.CL_NULL_GetState(pointer(state))
        self.biglock.release()
        return ret, state.value

    def CL_432_GetState(self):
        state = c_uint32(0)
        self.biglock.acquire()
        ret = self.sndll.CL_432_GetState(pointer(state))
        self.biglock.release()
        return ret, state.value

    def CL432GetSNSession(self):
        class CL_432_SNSessionBuffer(Structure):
            _fields_ = [("SNSession", c_uint8 * 512)]

        cl432_session = CL_432_SNSessionBuffer()

        self.biglock.acquire()
        self.sndll.CL_432_GetSNSession(pointer(cl432_session))
        self.biglock.release()

        leng = (((cl432_session.SNSession[1] << 8) + (cl432_session.SNSession[0])) - 2)
        return True, cl432_session.SNSession[2:leng+2]

    def CL_432_RELEASErequest(self, dst_addr):
        self.biglock.acquire()
        ret = self.sndll.CL_432_RELEASErequest(pointer(c_uint16(dst_addr)))
        self.biglock.release()
        return ret

    def CL_432_RELEASEconfirm(self):
        class CL_432_ReleaseConfirm_t(Structure):
            _fields_ = [("DestinationAddress", c_uint16),
                        ("Result", c_uint8)]

        conf = CL_432_ReleaseConfirm_t()
        self.biglock.acquire()
        ret = self.sndll.CL_432_RELEASEconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.DestinationAddress, conf.Result
        return ret, 0, 0

    def CL_432_ESTABLISHrequest(self, ser_num):
        class CL_432_EstablishRequest_t(Structure):
            _fields_ = [("SerialNumberLength", c_uint8),
                        ("SerialNumber", c_uint8 * 16)]

        req = CL_432_EstablishRequest_t(len(ser_num), (c_uint8 * 16)(*(ser_num)))

        self.biglock.acquire()
        ret = self.sndll.CL_432_ESTABLISHrequest(pointer(req))
        self.biglock.release()
        return ret


    def CL_432_ESTABLISHconfirm(self):
        class CL_432_EstablishConfirm_t(Structure):
            _fields_ = [("DestinationAddress", c_uint16),
                        ("BaseAddress", c_uint16),
                        ("SerialNumberLength", c_uint8),
                        ("SerialNumber", c_uint8 * 16)]

        conf = CL_432_EstablishConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.CL_432_ESTABLISHconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.DestinationAddress, conf.BaseAddress, conf.SerialNumber[0:conf.SerialNumberLength]
        return ret, 0, 0, []

    def MAC_PROMOTIONrequest(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_PROMOTIONrequest()
        self.biglock.release()
        return ret

    def MAC_PROMOTIONconfirm(self):
        result = c_uint8(0)
        self.biglock.acquire()
        ret = self.sndll.MAC_PROMOTIONconfirm(pointer(result))
        self.biglock.release()
        return ret, result

    def MAC_PROMOTIONindication(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_PROMOTIONindication()
        self.biglock.release()
        return ret

    def MAC_DEMOTIONrequest(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_DEMOTIONrequest()
        self.biglock.release()
        return ret

    def MAC_DEMOTIONconfirm(self):
        result = c_uint8(0)
        self.biglock.acquire()
        ret = self.sndll.MAC_DEMOTIONconfirm(pointer(result))
        self.biglock.release()
        return ret, result

    def MAC_DEMOTIONindication(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_DEMOTIONindication()
        self.biglock.release()
        return ret

    def MAC_UNREGISTERrequest(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_UNREGISTERrequest()
        self.biglock.release()
        return ret

    def MAC_UNREGISTERconfirm(self):
        result = c_uint8(0)
        self.biglock.acquire()
        ret = self.sndll.MAC_UNREGISTERconfirm(pointer(result))
        self.biglock.release()
        return ret, result

    def MAC_UNREGISTERindication(self):
        self.biglock.acquire()
        ret = self.sndll.MAC_UNREGISTERindication()
        self.biglock.release()
        return ret

    def MAC_REGISTERSTATEindication(self):
        class MAC_RegisterStateIndication_t(Structure):
            _fields_ = [("Result", c_uint8),
                        ("SNA", c_uint8 * 6),
                        ("EUI48", c_uint8 * 6)]

        ind = MAC_RegisterStateIndication_t()

        self.biglock.acquire()
        ret = self.sndll.MAC_UNREGISTERindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.Result, ind.SNA[0:6], ind.EUI48[0:6]
        return ret, 0, [0,0,0,0,0,0], [0,0,0,0,0,0]

    def PHY_DATArequest(self, data, level, scheme, timer):
        class PHY_DataRequest_t(Structure):
            _fields_ = [("uiLength", c_uint16),
                        ("ucLevel", c_uint8),
                        ("ucScheme", c_uint8),
                        ("ulTimer", c_uint32),
                        ("pucPsdu", c_uint8 * (377 + 7))]

        req = PHY_DataRequest_t(len(data), level, scheme, timer, (c_uint8 * (377 + 7))(*(data)))

        self.biglock.acquire()
        ret = self.sndll.PHY_DATArequest(pointer(req))
        self.biglock.release()
        return ret

    def PHY_DATAconfirm(self):
        result = c_uint8(0)
        self.biglock.acquire()
        ret = self.sndll.PHY_DATAconfirm(pointer(result))
        self.biglock.release()
        return ret, result

    def PHY_DATAindication(self):
        class PHY_DataIndication_t(Structure):
            _fields_ = [("uiLength", c_uint16),
                        ("ucLevel", c_uint8),
                        ("ucScheme", c_uint8),
                        ("ulTimer", c_uint32),
                        ("SNR", c_uint8),
                        ("Qlt", c_uint8),
                        ("pucPsdu", c_uint8 * (377 + 7))]

        ind = PHY_DataIndication_t()

        self.biglock.acquire()
        ret = self.sndll.PHY_DATAindication(pointer(ind))
        self.biglock.release()
        if ret == 0:
            return ret, ind.ucLevel,  ind.ucScheme,  ind.ulTimer,  ind.SNR,  ind.Qlt, ind.pucPsdu[0:ind.uiLength]
        return ret, 0, 0, 0, 0, 0, []

    def PRIME_LIB_CalcCRC32(self, crc, data):
        class SimpleBuffer(Structure):
            _fields_ = [("Data", c_uint8 * len(data))]
        c_crc32 = c_uint32(crc)
        c_data = SimpleBuffer((c_uint8 * len(data))(*(data)))
        c_len = c_uint16(len(data))

        self.biglock.acquire()
        self.sndll.PRIME_LIB_CalcCRC32(pointer(c_crc32), pointer(c_data), c_len)
        self.biglock.release()
        return c_crc32

    def PRIME_LIB_CalcCRC8(self, crc, data):
        class SimpleBuffer(Structure):
            _fields_ = [("Data", c_uint8 * len(data))]
        c_crc8 = c_uint8(crc)
        c_data = SimpleBuffer((c_uint8 * len(data))(*(data)))
        c_len = c_uint16(len(data))

        self.biglock.acquire()
        self.sndll.PRIME_LIB_CalcCRC8(pointer(c_crc8), pointer(c_data), c_len)
        self.biglock.release()
        return c_crc8

    def CMD_MAC_TESTmode(self, tm):
        self.biglock.acquire()
        self.sndll.CMD_MAC_TESTmode(pointer(c_uint32(tm)))
        self.biglock.release()

    def MAC_MLME_LIST_GETrequest(self, pib, endianness):
        class MAC_MlmeGetRequest_t(Structure):
            _fields_ = [("PIBListAttribute", c_uint16),
                        ("Endianness", c_uint8)]

        req = MAC_MlmeGetRequest_t(pib, endianness)

        self.biglock.acquire()
        self.sndll.MAC_MLME_LIST_GETrequest(pointer(req))
        self.biglock.release()

    def MAC_MLME_LIST_GETconfirm(self):
        class MAC_MlmeListGetConfirm_t(Structure):
            _fields_ = [("Status", c_uint8),
                        ("PIBListAttribute", c_uint16),
                        ("PIBListAttributeLen", c_uint16),
                        ("PIBListAttributeValue", c_uint8 *(((1) + (1)) * (253))),
                        ("Endianness", c_uint8)]

        conf = MAC_MlmeListGetConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.MAC_MLME_LIST_GETconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.Status, conf.PIBListAttribute, conf.PIBListAttributeValue[0:conf.PIBListAttributeLen]
        return ret, 0, 0, []

    def MAC_MLME_GETrequest(self, pib, endianness=0):
        class MAC_MlmeGetRequest_t(Structure):
            _fields_ = [("PIBAttribute", c_uint16),
                        ("Endianness", c_uint8)]

        req = MAC_MlmeGetRequest_t(pib, endianness)

        self.biglock.acquire()
        ret = self.sndll.MAC_MLME_GETrequest(pointer(req))
        self.biglock.release()
        return ret

    def MAC_MLME_GETconfirm(self):
        class MAC_MlmeGetConfirm_t(Structure):
            _fields_ = [("Status", c_uint8),
                        ("PIBAttribute", c_uint16),
                        ("Len", c_uint8),
                        ("PIBAttributeValue", c_uint8 * 16),
                        ("Endianness", c_uint8)]

        conf = MAC_MlmeGetConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.MAC_MLME_GETconfirm(pointer(conf))
        self.biglock.release()

        if ret == 0:
                if conf.Len <= 0:
                    val = None
                if conf.Len == 1:
                    val = conf.PIBAttributeValue[0] # cast to uint8_t
                elif conf.Len == 2:
                    val = (conf.PIBAttributeValue[0] << 8) + conf.PIBAttributeValue[1]  # cast to uint16_t
                elif conf.Len == 4:
                    val = (conf.PIBAttributeValue[0] << 24) + (conf.PIBAttributeValue[1] << 16) + (conf.PIBAttributeValue[2] << 8) + conf.PIBAttributeValue[3]  # cast to uint32_t
                else:
                    val = conf.PIBAttributeValue[:conf.Len]
                return ret, conf.Status, conf.PIBAttribute, val
        return ret, 0, 0, []

    def MAC_MLME_SETrequest(self, pib, arrvalue):
        class MAC_MlmeSetRequest_t(Structure):
            _fields_ = [("PIBAttribute", c_uint16),
                        ("PIBAttributeValue", c_uint32)]

        if len(arrvalue) == 1:
            value = arrvalue[0]
        elif len(arrvalue) == 2:
            value = (arrvalue[0] << 8) + arrvalue[1]
        elif len(arrvalue) == 4:
            value = (arrvalue[0] << 24) + (arrvalue[1] << 16) + (arrvalue[2] << 8) + arrvalue[3]
        else:
            return -1

        req = MAC_MlmeSetRequest_t(pib, value)

        self.biglock.acquire()
        ret = self.sndll.MAC_MLME_SETrequest(pointer(req))
        self.biglock.release()
        return ret

    def MAC_MLME_SETconfirm(self):
        class MAC_MlmeSetConfirm_t(Structure):
            _fields_ = [("result", c_uint8)]

        conf = MAC_MlmeSetConfirm_t()

        self.biglock.acquire()
        ret = self.sndll.MAC_MLME_SETconfirm(pointer(conf))
        self.biglock.release()
        if ret == 0:
            return ret, conf.result
        return ret, 0
