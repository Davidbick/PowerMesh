from threading import Thread
import serial
import time
from threading import Lock
from prime_validation_framework1_4.src.sci import SciInterface
from prime_validation_framework1_4.src.eci import EciInterface
from prime_validation_framework1_4.src.servicenode import ServiceNodeAbstract
from prime_validation_framework1_4.src.protocol import Modulation, SpecPibs
from prime_validation_framework1_4.src.global_prime_defs import Vendor


class PrimeIf(ServiceNodeAbstract):
    InitFailure = True
    mgnindthreadrunning = False
    CLNULLDataLooprunning = False
    ResponseAnswer = 0
    ManageIndications = True
    perf_loop_test = False
        # Dictionary of active connections info
        # Keys are: connection handles
        # Format of each entry: [type, data]
        # - Type (as returned by Indication, [] if connection was requested by SN)
        # - Data (as returned by Indication, [] if connection was requested by SN)
        # The reserved key 'lastCh' is used to store the connection handle of the
        # latest added connection  

    def __init__(self, port):
        try:
            self.serial_com = serial.Serial(port, baudrate=57600, timeout=1)
            self.serial_lock = Lock()
            self.sci = SciInterface(self.serial_com, self.serial_lock)
            self.eci = EciInterface(self.serial_com, self.serial_lock)
        except serial.SerialException:
            self.serial_com = None

    def __del__(self):
        self.close()

    def close(self):
        self.Stop()
        if self.serial_com is not None:
            self.serial_com.close()

    def Stop(self):
        self.StopManagingIndications()

    def NodeInfo(self):
        fwver = self.GetPib(self.Pibs.AppFwVersion)
        if ((len(fwver) != 16) and (len(self.eui48) != 6 )):
            #print "error reading node info"
            return "error reading node info"
        txt = ":".join("{:02X}".format(c) for c in self.eui48) + ", "
        txt += self.DeviceType(self.ProductID) + " "
        txt += "HALF_DUPLEX" + " "
        txt += "".join(chr(c) for c in fwver).strip("\0")
        return txt

    def GetPib(self, pib, iface="SCI", endianness=0):
        if iface == "SCI":
            arr = self.sci.GetPib(pib)
        else:
            arr = self.GetEciPib(pib)

        if len(arr) == 1:
            return arr[0] # cast to uint8_t
        elif len(arr) == 2:
            return (arr[0] << 8) + arr[1]  # cast to uint16_t
        elif len(arr) == 4:
            return (arr[0] << 24) + (arr[1] << 16) + (arr[2] << 8) + arr[3]  # cast to uint32_t
        else:
            return arr
            
    def GetPibAsArray(self, pib, iface="SCI", endianness=0):
        if iface == "SCI":
            return self.sci.GetPib(pib)
        else:
            return self.GetEciPib(pib)

    def SetPib(self, pib, value, iface="SCI"):
        if iface == "SCI":
            self.sci.SetPib(pib, value)
            time.sleep(0.1) 
            return True # no confirm for SCI set pib
        else: 
            return self.SetEciPib(pib, value)

    def GetPibList(self, pib, iface="SCI", endianness=0):
        if iface == "SCI":
            return self.GetSciPibList(pib)
        else:
            return self.GetEciPibList(pib)
    
    def SetTestMode(self, inTestModeMask, iface="ECI"):
        if self.ProductID == Vendor.ProductId.SN_EMB:
            iface = "ECI"
        tmpTestModeMask = self.currTestModeMask | inTestModeMask
        if iface == "SCI":
            self.sci.SetPib(self.Pibs.primeTM, [(tmpTestModeMask >> 24) & 0xFF, (tmpTestModeMask >> 16) & 0xFF, (tmpTestModeMask >> 8) & 0xFF, tmpTestModeMask & 0xFF]) 
            self.currTestModeMask = tmpTestModeMask
            return True
        else:
            ec = self.eci.CMD_MAC_TESTmode(tmpTestModeMask)
            if ec == 0:
                self.currTestModeMask = tmpTestModeMask
                return True
            else:
                return False
    
    def ClearTestMode(self, inTestModeMask=None, iface="ECI"):
        if self.ProductID == Vendor.ProductId.SN_EMB:
            iface = "ECI"
        if inTestModeMask is None:
            inTestModeMask = ~self.defTestModeMask
        tmpTestModeMask = self.currTestModeMask & ~inTestModeMask
        if iface == "SCI":
            self.sci.SetPib(self.Pibs.primeTM, [(tmpTestModeMask >> 24) & 0xFF, (tmpTestModeMask >> 16) & 0xFF, (tmpTestModeMask >> 8) & 0xFF, tmpTestModeMask & 0xFF]) 
            self.currTestModeMask = tmpTestModeMask
            return True
        else:
            ec = self.eci.CMD_MAC_TESTmode(tmpTestModeMask)
            if ec == 0:
                self.currTestModeMask = tmpTestModeMask
                return True
            else:
                return False
    
    def InitTestMode(self, inTestModeMask=None):
        tmpTestModeMask = self.defTestModeMask if inTestModeMask is None else inTestModeMask
        ec = self.eci.CMD_MAC_TESTmode(tmpTestModeMask)
        if ec == 0:
            self.currTestModeMask = self.defTestModeMask = tmpTestModeMask
            return True
        else:
            return False

    def GetTestMode(self):
        return self.currTestModeMask
        
    def GetDefTestMode(self):
        return self.defTestModeMask

    def SwReset(self, iface="ECI"):
        if iface == "SCI":
            self.sci.SetPib(self.Pibs.Res, [0x01])   
            time.sleep(1.5)
        else:
            self.eci.SwReset()
            time.sleep(1.5)

    def SerialComClose(self):
        self.serialcom.close()

    def SerialComOpen(self):
        self.serialcom.open()

    def GetEciPib (self, pib, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_MLME_GETrequest(pib) == 0:
            #print "ECI MAC_MLME_GETrequest ok"
            while True:
                ec, _, _, value = self.eci.MAC_MLME_GETconfirm()
                if ec == 0:
                    #print "ECI MAC_MLME_GETconfirm ok"
                    break
                if time.time() > endtime:
                    #print "ECI GetEciPib timeout"
                    return []
                time.sleep(0.015)
            return value
        return []

    def SetEciPib (self, pib, value, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_MLME_SETrequest(pib, value) == 0:
            while True:
                ec, result = self.eci.MAC_MLME_SETconfirm()
                if ec == 0:
                    break
                if time.time() > endtime:
                    return False
                time.sleep(0.015)
            if result == 0:
                return True
        return False

    def GetSciPibList(self, pib):
        lst = []
        for index in range(0x01, 0xFF):
            val, eol = self.sci.GetListEntry(pib, index)
            if val is None:
                break
            lst += [val]
            if eol:
                break
        return lst

    def GetEciPibList(self, pib, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_MLME_LIST_GETrequest(pib) == 0:
            while True:
                ec, status, _, raw = self.eci.MAC_MLME_LIST_GETconfirm()
                if ec == 0:
                    if status == 0:

                        if pib == self.Pibs.macListSwitchTable:
                            entrysize = 5
                        elif pib == self.Pibs.macListAvailableSwitches:
                            entrysize = 10
                        elif pib == self.Pibs.macListPhyComm:
                            return SpecPibs.convertToPhyCommList(raw)
                        else:
                            return []

                        lis = []
                        for i in range (0, int(len(raw)/entrysize)):
                            lis += [raw[i*entrysize:(i + 1)*entrysize]]
                        return lis
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return []

    def InitiatePromotion(self, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_PROMOTIONrequest() == 0:
            while True:
                ec, result = self.eci.MAC_PROMOTIONconfirm()
                if ec == 0:
                    if result == 0:
                        return True
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def InitiateDemotion(self, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_DEMOTIONrequest() == 0:
            while True:
                ec, result = self.eci.MAC_DEMOTIONconfirm()
                if ec == 0:
                    if result == 0:
                        return True
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def InitiateUnregister(self, timeout=20):
        endtime = time.time() + timeout
        if self.eci.MAC_UNREGISTERrequest() == 0:
            while True:
                ec, result = self.eci.MAC_UNREGISTERconfirm()
                if ec == 0:
                    if result == 0:
                        return True
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def CL432GetSNSession(self):
        ec, session = self.eci.CL432_GetSNSession()
        if ec == 0:
            return True, session
        else:
            return False, session

    def GetState(self):
        ec, state = self.eci.GetState()
        if ec == 0:
            return state
        return 0

    def CL432Establish(self, serial, timeout=20):
        endtime = time.time() + timeout
        # first, clear release confirm if present
        self.eci.CL432_ReleaseConfirm()
        if self.eci.CL432_EstablishRequest(serial) == 0:
            while True:
                ec, dst_addr, src_addr, ser = self.eci.CL432_EstablishConfirm()
                if ec == 0:
                    if serial == ser:
                        return True, dst_addr, src_addr
                    break
                # if an establish request fails, a release confirm is generated
                ec, _, _ = self.eci.CL432_ReleaseConfirm()
                if ec == 0:
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False, 0, 0

    def CL432Release(self, dst_addr, timeout=20):
        endtime = time.time() + timeout
        if self.eci.CL432_ReleaseRequest(dst_addr) == 0:
            while True:
                ec, dst_a, res = self.eci.CL432_ReleaseConfirm()
                if ec== 0:
                    if dst_a == dst_addr:
                        if res == 0:
                            return True
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def CL432GetData(self, dst_addr, src_addr):
        ec, _, _, dst_a, src_a, data = self.eci.DL_Data_Indication()
        if ec == 0:
            if dst_a == dst_addr and src_a == src_addr:
                return data
        return []

    def CL432SendData(self, dst_addr, src_addr, data, timeout=30):
        endtime = time.time() + timeout
        if self.eci.DL_Data_Request(dst_addr, src_addr, data) == 0:
            while True:
                ec, _, _, dst_a, src_a, txs = self.eci.DL_Data_Confirm()
                if ec == 0:
                    if dst_a == dst_addr and src_a == src_addr:
                        if txs == 0:
                            return True
                    break
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def CL_NULL_Establish(self, eui=[0,0,0,0,0,0], typ=0, arq=False, data=[], timeout=30):
        endtime = time.time() + timeout
        if self.eci.CL_NULL_ESTABLISHrequest(eui, typ, arq, data) == 0:
            while True:
                ec, ch, res, _, _, dat = self.eci.CL_NULL_ESTABLISHconfirm()
                if ec == 0:
                    return True, ch, res, dat
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False, 0, 0, 0

    def CL_NULL_Release(self, ch, timeout=30):
        endtime = time.time() + timeout
        if self.eci.CL_NULL_RELEASErequest(ch) == 0:
            while True:
                ec, _, answer = self.eci.CL_NULL_RELEASEconfirm()
                if ec == 0:
                    return True, answer
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False, 0

    def CL_NULL_GetData(self):
        ec, ch, data = self.eci.CL_NULL_DATAindication()
        if ec == 0:
            return ch, data
        return 0, []

    def CL_NULL_SendData(self, ch, buff, prio=1, timeout=20):
        endtime = time.time() + timeout
        if self.eci.CL_NULL_DATArequest(ch, buff, prio) == 0:
            while True:
                ec, _, res, _ = self.eci.CL_NULL_DATAconfirm()
                if ec == 0:
                    if res == 0:
                        return True
                    else:
                        return False
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False

    def CL_NULL_RMISendData(self, ch, buff, prio=1):
        if self.eci.CL_NULL_DATArequest(ch, buff, prio) == 0:
            return True
        return False

    def CL_NULL_Join(self, bcast, eui, data=[], timeout=30):
        endtime = time.time() + timeout
        if self.eci.CL_NULL_JOINrequest(bcast, 0, eui, 0, data) == 0:
            while True:
                ec, ch, _, result = self.eci.CL_NULL_JOINconfirm()
                if ec == 0:
                    return True, ch, result
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False, 0, 0

    def CL_NULL_Leave(self, ch, eui, timeout=30):
        endtime = time.time() + timeout
        if self.eci.CL_NULL_LEAVErequest(ch, eui)== 0:
            while True:
                ec, ch, _, result = self.eci.CL_NULL_LEAVEconfirm()
                if ec == 0:
                    return True, ch, result
                if time.time() > endtime:
                    break
                time.sleep(0.015)
        return False, 0 ,0

    def PHY_GetData(self):
        ec, lev, sch, tim, snr, rssi, di = self.eci.PHY_DATAindication1_4()
        if ec == 0:
            return di, lev, sch, tim, snr, rssi
        return [], 0, 0, 0, 0, 0

    def PHY_SendData(self, data, level=7, scheme=Modulation.TYPEA_DBPSKCC, timer=0, timeout=20, t_sleep=0.015):
        endtime = time.time() + timeout
        if self.eci.PHY_DATArequest1_4(data, level, scheme, timer) == 0:
            while True:
                if self.eci.PHY_DATAconfirm() == 0:
                    return True
                if time.time() > endtime:
                    break
                time.sleep(t_sleep)
        return False

    def IsUnregisterIndicationPresent(self):
        ec, _ = self.eci.MAC_UNREGISTERindication()
        return ec == 0

    def IsPromotionIndicationPresent(self):
        ec, _ = self.eci.MAC_PROMOTIONindication()
        return ec == 0

    def IsDemotionIndicationPresent(self):
        ec, _ = self.eci.MAC_DEMOTIONindication()
        return ec == 0

    def CLNULLDataLoopThread(self,ch):
        while self.CLNULLDataLooprunning:                    
            buff = list(range(0, 50))  
            self.CL_NULL_SendData(ch, buff, prio=2)
            
        time.sleep(0.25)
        
    def MGNINDThread(self):
        MANAGED_INDICATIONS = (self.Indication.MAC_ESTABLISHINDICATION |
                               self.Indication.MAC_RELEASEINDICATION |
                               self.Indication.MAC_JOININDICATION |
                               self.Indication.MAC_LEAVEINDICATION)
        if  self.perf_loop_test:
            MANAGED_INDICATIONS |= (self.Indication.CL_432_DATAINDICATION | self.Indication.CL_432_DATACONFIRM)
        while self.mgnindthreadrunning:
            if self.ManageIndications:
                

                state = self.GetState() & MANAGED_INDICATIONS
                if state != 0:
                    if state & self.Indication.CL_432_DATACONFIRM != 0:
                        #print " sn data confirm"    
                        ec, _, _, dst_a, src_a, txs = self.eci.DL_Data_Confirm()
                        #if ec == 0:
                            #print " c present, status: " + str(txs)                           
                    if state & self.Indication.MAC_ESTABLISHINDICATION != 0:
                        self.ManageClNullEstablishIndication()
                    if state & self.Indication.MAC_RELEASEINDICATION != 0:
                        self.ManageClNullReleaseIndication()
                    if state & self.Indication.MAC_JOININDICATION != 0:
                        self.ManageClNullJoinIndication()
                    if state & self.Indication.MAC_LEAVEINDICATION!= 0:
                        self.ManageClNullLeaveIndication()
                    if state & self.Indication.CL_432_DATAINDICATION!= 0:
                        ec, _, _, dst_a, src_a, data = self.eci.DL_Data_Indication()
                        if ec == 0:
                            #print "sn send reply"
                            ret = self.eci.DL_Data_Request(src_a, dst_a, data)
                            #if ret == 0:    
                                #print "sn reply sent"
                            #else:
                                #print "sn reply not sent, ret " +str(ret)
                                
            time.sleep(0.015)
        time.sleep(0.25)

    def StartManagingIndications(self, answer=0):
        self.ResponseAnswer = answer
        if not self.mgnindthreadrunning:
            self.mgnindthreadrunning = True
            self.mgnindthread = Thread(target=self.MGNINDThread)
            self.mgnindthread.start()

    def StopManagingIndications(self):
        if self.mgnindthreadrunning:
            self.mgnindthreadrunning = False
            self.mgnindthread.join()

    def StartSNDataLoop(self,ch):
        if not self.CLNULLDataLooprunning:
            self.CLNULLDataLooprunning = True
            self.CLNULLDataLoopthread = Thread(target=self.CLNULLDataLoopThread, args=[ch])
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
                ret, ch, _, _, _, data = self.eci.CL_NULL_ESTABLISHindication()
                if ret == 0:
                    self.eci.CL_NULL_ESTABLISHresponse(ch, response, data)
                    return True, data[0:len(data)]
                else:
                    return False, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, []

    def ManageClNullEstablishIndication(self):
        ret, ch, _, typ, _, data = self.eci.CL_NULL_ESTABLISHindication()
        if ret == 0:
            self.eci.CL_NULL_ESTABLISHresponse(ch, self.ResponseAnswer, [])
            if ch not in self.activeConn:
                self.activeConn['lastCh'] = ch
                self.activeConn[ch] = [typ, data]

    def ManageClNullReleaseIndication(self):
        ret, ch, reason= self.eci.CL_NULL_RELEASEindication()
        if ret == 0:
            if reason == 0:
                self.eci.CL_NULL_RELEASEresponse(ch, self.ResponseAnswer)
                del self.activeConn[ch]
                if self.activeConn['lastCh'] == ch:
                    self.activeConn['lastCh'] = None

    def WaitClNullJoinIndication(self, response=0, timeout=20):
        endtime = time.time() + timeout
        while True:
            state = self.GetState()
            if state & self.Indication.MAC_JOININDICATION != 0:
                ret, ch, eui, _, data = self.eci.CL_NULL_JOINindication()
                if ret == 0:
                    self.eci.CL_NULL_JOINresponse(ch, eui, response)
                    return True, ch, data
                else:
                    return False, 0, []
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return False, 0, []

    def ManageClNullJoinIndication(self):
        ec, ch, eui, typ, data = self.eci.CL_NULL_JOINindication()
        if ec == 0:
            self.eci.CL_NULL_JOINresponse(ch, eui, self.ResponseAnswer)
            if ch not in self.activeConn:
                self.activeConn['lastCh'] = ch
                self.activeConn[ch] = [typ, data]

    def ManageClNullLeaveIndication(self):
        ec, ch, _ = self.eci.CL_NULL_LEAVEindication()
        if ec == 0 and ch in self.activeConn:
            del self.activeConn[ch]
            if self.activeConn['lastCh'] == ch:
                self.activeConn['lastCh'] = None

    #======================= CL_IPv6 'concrete' methods ==================
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

    def CL_IPv6_Establish(self, timeout=20, ARQ = 0):
        endtime = time.time() + timeout
        # first, clear release confirm if present
        self.eci.CL_IPv6_ReleaseConfirm()
        if self.eci.CL_IPv6_EstablishRequest(ARQ) == 0:
            while True:
                time.sleep(0.015)
                ec = self.eci.CL_IPv6_EstablishConfirm() # here linkLocalAddress is also registered
                if ec == 0:
                    return True
                # if an establish request fails, a release confirm is generated
                ec, _ = self.eci.CL_IPv6_ReleaseConfirm()
                if ec == 0:
                    break
                if time.time() > endtime:
                    break
        return False

    # ------------------------------------------------------------------
    # The following functions split the 'CL_IPv6_Establish' 
    # in the 'CL_IPv6_EstablishRequest' and the 'CL_IPv6_EstablishConfirm',
    # allowing the caller to do something in between.
    
    def CL_IPv6_Establish_Request(self, ARQ = 0):
        '''
            This function does an EstablishReq 
            
        '''
        # first, clear release confirm if present
        self.eci.CL_IPv6_ReleaseConfirm()
        if self.eci.CL_IPv6_EstablishRequest(ARQ) == 0:
            return True
        else:
            return False

    def CL_IPv6_Establish_Confirm(self, timeout=20):
        '''
            This function calls (and wait) the CL_IPv6_Establish_Confirm on SN
        '''
        endtime = time.time() + timeout
        while True:
            time.sleep(0.015)
            ec = self.eci.CL_IPv6_EstablishConfirm() # here linkLocalAddress is also registered
            if ec == 0:
                return True
            # if an establish request fails, a release confirm is generated
            ec, _ = self.eci.CL_IPv6_ReleaseConfirm()
            if ec == 0:
                break
            if time.time() > endtime:
                break
        return False

    # ------------------------------------------------------------------
    def CL_IPv6_Release(self, timeout=20):
        '''
        Release the CL_IPv6 and return True if is all OK
        TODO: change the returned value with the 'res'
        '''
        endtime = time.time() + timeout
        if self.eci.CL_IPv6_ReleaseRequest() == 0:
            while True:
                time.sleep(0.015)
                ec, res = self.eci.CL_IPv6_ReleaseConfirm()
                if ec == 0:
                    if res == 0:
                        return True
                    else:
                        return False # error on release (error code it's in res)
                if time.time() > endtime:
                    break
        return False # timeout occurred
    # ------------------------------------------------------------------
    def CL_IPv6_Rel_Request(self):
        '''
        Call the Release Request of the CL_IPv6 and return True if is all OK
        '''
        if self.eci.CL_IPv6_ReleaseRequest() == 0:
            return True
        return False # ---------------

    def CL_IPv6_Rel_Confirm(self):
        '''
        Call the Release Confirm of the CL_IPv6 and return the result
        '''
        return self.eci.CL_IPv6_ReleaseConfirm()
    # ------------------------------------------------------------------

    def CL_IPv6_WaitReleaseConfirm(self, timeout=20):
        ''' 
        This function waits for a 'CL_IPv6_ReleaseConfirm' message.
        
        Returns:
            - 'True' if the message arrive,
            - 'False' if message has not been received in the predefined time.
        '''
        endtime = time.time() + timeout
        while True:
                time.sleep(0.015)
                ec, _ = self.eci.CL_IPv6_ReleaseConfirm() # don't care about the result...
                if ec == 0:
                    return True # RELEASEconfirm: PRESENT
                if time.time() > endtime:
                    break
        return False            # RELEASEconfirm: NOT PRESENT


    def CL_IPv6_WaitData(self,  timeout=120):
        '''This function waits for incoming data'''
        endtime = time.time() + timeout
        while True:
            data = self.CL_IPv6_GetData() 
            if data != []:
                return data
            if time.time() > endtime:
                break
            time.sleep(0.015)
        return []

    def CL_IPv6_GetData(self):
        ''' Get data from BN data connection.'''
        ec , _ , data = self.eci.CL_IPv6_DataIndication()
        if (ec == 0):
            return data
        time.sleep(0.015)
        return []

    def CL_IPv6_SendData(self, src_addr, dst_addr, payload, nextHeader=59, hopLimit=1, timeout=30):
        """ Function to send data to a particular Node via data connection.
            The functions returns two objects (confirm,txData):
            - confirm: the confirmation that data has been sent
            - txData : a list containing the data transmitted ( header + payload )
        """
        # prepare the final PDU using the IPv6 address passed as paramether 
        # traffic class = 0x00 FIXME: define 'traffic class' value
        # flow label = 0x00000 FIXME: define 'flow label' value
        payloadLen = len(payload)
        # lengthPDU = payloadLen + 40 # buffer + header        
        #payloadLen
        pdu  = [0x60,0x00,0x00,0x00]        # 0-3) pkt type (4bit) - traffic class (8bit) - flow label (20bit)
        pdu += [(payloadLen>>8)&0xFF , payloadLen&0xFF]   # 4-5) payload length
        pdu += [nextHeader]     # 6) next header 
        pdu += [hopLimit]   # 7) HOP limit (1 => inside PRIME net)
        pdu += src_addr     # 8-23) source address
        pdu += dst_addr     # 24-39) destination address
        pdu += payload
        
        return self.CL_IPv6_SendFrame(pdu,timeout) #####
        '''
        endtime = time.time() + timeout
        res = self.eci.CL_IPv6_DataRequest(pdu)
        if (res == 0):
            while True:
                time.sleep(0.05)
                msg = self.eci.CL_IPv6_DataConfirm()
                if msg[0] == 0:
                    if msg[1] == 0:
                        return True, pdu
                    break
                if time.time() > endtime:
                    break
                #time.sleep(0.015)
        return False, []
        '''
    ##############################################################################################################
    # the following function is commented because, in the CL_IPv6, there isn't a 'DataConfirm' after 'DataRequest'
    '''
    def CL_IPv6_SendFrame(self, dataFrame, timeout=20):
        # This function sends a preformatted IPv6 packet.
        #    It returns  0 and data sent if no errors occurs 
        endtime = time.time() + timeout
        res = self.eci.CL_IPv6_DataRequest(dataFrame)
        if (res == 0):
            while True:
                time.sleep(0.05)
                msg = self.eci.CL_IPv6_DataConfirm()
                if msg[0] == 0:
                    if msg[1] == 0:
                        return 0, dataFrame
                    break
                if time.time() > endtime:
                    break
                #time.sleep(0.015)
        return 1, []
    '''
    def CL_IPv6_SendFrame(self, dataFrame, timeout=20):
        ''' This function sends a preformatted IPv6 packet.
            It returns  0 and data sent if no errors occurs '''
        res = self.eci.CL_IPv6_DataRequest(dataFrame)
        if (res == 0):
            return 0, dataFrame
        return res, []

    def CL_IPv6_DataConfirm(self):
        res = self.eci.CL_IPv6_DataConfirm()
        return res
