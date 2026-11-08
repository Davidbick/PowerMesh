try:
    import time
    from util_crc import Crc8
    from protocol import Modulation
except:
    print("import Issue in eci.py")

class EciInterface:

    class Commands:
        CL432_Establish_Request =           0xA0 # Opens a CS session.
        CL432_Establish_Confirm =           0xA1 # Confirm CS session opening.
        CL432_Release_Request =             0xA2 # Closes a CS session.
        CL432_Release_Confirm =             0xA3 # Confirm a CS session closing.
        CL432_Join_Indicate =               0xA4 # Indicates a CS session opened.
        CL432_Leave_Indicate =              0xA5 # Indicates a CS session closed.
        DL_Data_Request =                   0xA6 # Send Data (Unicast).
        DL_Data_Confirm =                   0xA7 # Local confirmation of data send.
        DL_Data_Indication =                0xA8 # Indicates an incoming SDU.
        DL_Broadcast_request =              0xA9 # Send Data (Broadcast).
        DL_Reply_Request =                  0xAA # Request that a previously prepared SDU be sent by a remote station.
        DL_Reply_Confirm =                  0xAB # Receives a prepared LSDU
        DL_Reply_Indication =               0xAC # Indicates the status of a prepared SDU.
        DL_UpdateReply_request =            0xAD # Prepare an SDU to be sent when requested.
        DL_UpdateReply_Confirm =            0xAE # Confirm the SDU preparation.
        DL_UpdateReply_Indication =         0xAF # Indicates the presence of a prepared SDU on remote node.
        PRIME_GetState =                    0xB0 # Get the State Bytes
        MAC_MLME_LIST_GETrequest =          0xB1 # Request a PRIME MAC List
        MAC_MLME_LIST_GETconfirm =          0xB2 # Get the requested PRIME MAC List
        primeConfig =                       0xB3 # Configure the PRIME protocol stack
        CL_432_GetSNSession =               0xB4 # Get the available CL 4-32 Sessions
        PIB_GETrequest =                    0xB5 # Request a get of a PRIME MAC Attribute
        PIB_GETconfirm =                    0xB6 # Get a PRIME MAC Attribute
        PIB_SETrequest =                    0xB7 # Request a set of a PRIME Attribute
        PIB_SETconfirm =                    0xB8 # Confirm the set of a PRIME Attribute
        MAC_MLME_REGISTERSTATE_Indication = 0xB9 # Indicate that the SN is De/Registered
        MAC_TESTmode =                      0xBA # Set a Test Mode for MAC
        RFU_EVENT_SETrequest =              0xBC # (only for STarCOM) Set a PRIME RFU event
        RFU_EVENTindication =               0xBD # (only for STarCOM) Get a PRIME RFU event 
        RFU_EVENTindicationConfirm =        0xBE # (only for STarCOM) Provide to the modem a confirm about the reception of a RFU_EventIndication
        MAC_Establish_Request =             0xC0 # Establish a unicast MAC Connection
        MAC_Establish_Indication =          0xC1 # Indicate that a unicast MAC Connection will be open
        MAC_Establish_Response =            0xC2 # Reply to the MAC Establish Indication
        MAC_Establish_Confirm =             0xC3 # Confirm the MAC Establish Connection
        MAC_Relase_Request =                0xC4 # Close a MAC Connection
        MAC_Release_Indication =            0xC5 # Indicate that a MAC Connection will be closed
        MAC_Release_Response =              0xC6 # Reply to the MAC Release Indication
        MAC_Release_Confirm =               0xC7 # Confirm the MAC Release Request
        MAC_Join_Request =                  0xC8 # Join a Broadcast or Multicast MAC Channel
        MAC_Join_Confirm =                  0xC9 # Confirm the joining of a Channel
        MAC_Leave_Request =                 0xCA # Leave a Broadcast or Multicast MAC Channel
        MAC_Leave_Confirm =                 0xCB # Confirm the leaving of a Channel
        MAC_Data_Request =                  0xCC # Send a Data through a unicast MAC Connection
        MAC_Data_Confirm =                  0xCD # Confirm the Data Sending through the Data Request
        MAC_Data_Indication =               0xCE # Indicate that a Data is arrived on a MAC Connection
        MAC_Join_Indication =               0xCF # Indicate the new incoming MAC joining procedure
        MAC_Join_Response =                 0xD0 # Reply to a previously issued MAC joining procedure
        MAC_Unregister_Request =            0xD1 # Ask a SN to start a un-registration procedure
        MAC_Unregister_Confirm =            0xD2 # Confirm the SN un-registration procedure
        MAC_Unregister_Indication =         0xD3 # Indicate the SN un-registration (not supported)
        MAC_Promotion_Request =             0xD4 # Ask for a promotion of a SN
        MAC_Promotion_Confirm =             0xD5 # Confirm the SN promotion procedure
        MAC_Promotion_Indication =          0xD6 # Indicate the SN promotion (not supported)
        MAC_Demotion_Request =              0xD7 # Ask for a demotion of a SN
        MAC_Demotion_Confirm =              0xD8 # Confirm the SN demotion procedure
        MAC_Demotion_Indication =           0xD9 # Indicate the SN demotion (not supported)
        MAC_Leave_Indication =              0xDA # Indicate the MAC Leaving of a multicast channel
        PHY_DATAIndication =                0xE1 # Receive Data at Physical Layer
        PHY_DATARequest =                   0xE2 # Send Data at Physical Layer
        PHY_DATAConfirm =                   0xE3 # Confirm the Data Sending at Physical Layer
        PHY_DATAIndication1_4 =             0xE7 # Receive Data at Physical Layer
        PHY_DATARequest1_4 =                0xE8 # Send Data at Physical Layer
        CL_IPv6_Establish_Request =         0xE9 # Opens a CL IPv6 session.
        CL_IPv6_Establish_Confirm =         0xEA # Ask confirmation about a previous CL_IPv6 _Establish_Request command.
        CL_IPv6_Release_Request =           0xEB # Closes an active Convergence layer session.
        CL_IPv6_Release_Confirm =           0xEC # Request confirmation about a previous CL_IPv6_Release_Request.
        CL_IPv6_Data_Request =              0xED # Requires that an LSDU be sent to a remote entity.
        CL_IPv6_Data_Confirm =              0xEE # Requires a Local Confirmation of a previous CL_IPv6_Data_Request command.
        CL_IPv6_Data_Indicate =             0xEF # Gets a received SDU.
        SW_Reset =                          0x90 # Manage a Software Reset

    def __init__(self, ser, serlock):
        self.ser = ser
        self.serlock = serlock
        self.serlock.acquire(False)
        self.serlock.release()
        
    def __del__(self):
        self.serlock.acquire(False)
        self.serlock.release()
        del self.serlock
        del self.ser

    def EciSendCommand(self, cmd, msg):
        buff = []
        buff += [cmd]
        leng = len(msg)
        buff += [leng & 0xFF, leng >> 8]
        buff += [0, 0, 0, 0]
        buff += msg
        crc = Crc8.Calculate(buff)
        buff_to_send = [0x16, 0x16] + buff + [crc]

        self.serlock.acquire()
        
        #self.ser.write(buff_to_send)
        
        try:
            self.ser.write(buff_to_send)
        except Exception as ex:
            template = "EciSendCommand An exception of type {0} occurred. Arguments:\n{1!r}"
            message = template.format(type(ex).__name__, ex.args)
            print(message)
           
            try:
                time.sleep(1)
                print("EciSendCommand write retry\n")
                self.ser.write(buff_to_send)
            except Exception as ex:
                # write failed. Ignore and let's EciWaitResponse fail too.
                # It will release the lock.
                # [AP:TODO: manage in a better way, avoiding to call EciWaitResponse]
                template = "An exception of type {0} occurred. Arguments:\n{1!r}"
                message = template.format(type(ex).__name__, ex.args)
                print(message)
                time.sleep(1)
                print("EciSendCommand Serial write failed again, ignore\n")
    def EciWaitResponse(self, timeout=15):
        endtime = time.time() + timeout
        pkt = []
        while True:
            try:
                pkt += [bytearray(self.ser.read(1))[0]]
            except:
                self.serlock.release()
                return [0, 0, 0, 0], [0xFF]
            
            if len(pkt) == 5:
                leng = (pkt[4] << 8) + pkt[3]
            elif len(pkt) > 5:
                if len(pkt) >= leng + 10:
                    break
            if time.time() > endtime:
                self.serlock.release()
                return [0, 0, 0, 0], [0xFF]
           
        self.serlock.release()
        if Crc8.Calculate(pkt[2:-1]) != pkt[-1]:
            return [0, 0, 0, 0], [0xFF]
        return pkt[5:9], pkt[9:-1]

    def GetState(self):
        self.EciSendCommand(self.Commands.PRIME_GetState,[])
        state, msg = self.EciWaitResponse()
        return msg[0], (state[0] << 24) + (state[1] << 16) + (state[2] << 8) + state[3]

    def CL432_GetSNSession(self):
        self.EciSendCommand(self.Commands.CL_432_GetSNSession, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return msg[0], []
        leng = (((msg[1] << 8) + (msg[2])) - 2)
        return msg[0], msg[3: leng+3]

    def CL432_EstablishRequest(self, serial ):
        buf = [len(serial)]
        buf += serial
        self.EciSendCommand(self.Commands.CL432_Establish_Request, buf)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL432_EstablishConfirm(self):
        self.EciSendCommand(self.Commands.CL432_Establish_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0, []
        dst_addr = ((msg[1] << 8) + msg[2])
        src_addr = ((msg[3] << 8) + msg[4])
        leng = msg[5]
        serial = msg[6:6 + leng]
        return ec, dst_addr, src_addr, serial

    def CL432_ReleaseRequest(self, dst_addr):
        buf = [(dst_addr >> 2 ) & 0xFF, dst_addr & 0xFF]
        self.EciSendCommand(self.Commands.CL432_Release_Request, buf)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL432_ReleaseConfirm(self):
        self.EciSendCommand(self.Commands.CL432_Release_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0
        dst_addr  = (msg[1] << 8) + msg[2]
        res = msg[3]
        return ec, dst_addr, res

    def DL_Data_Indication(self):
        self.EciSendCommand(self.Commands.DL_Data_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0, 0, 0, []
        dsap = msg[1]
        ssap = msg[2]
        dst_addr  = (msg[3] << 8) + msg[4]
        src_addr  = (msg[5] << 8) + msg[6]
        #lc = msg[7] # for future uses
        leng = (msg[8] << 8) + msg[9]
        data = msg[10:10 + leng]
        return ec, dsap, ssap, dst_addr, src_addr, data

    def DL_Data_Request(self, dst_addr, src_addr, data):
        buf = [0, 0]
        buf += [dst_addr >> 8, dst_addr & 0xFF]
        buf += [src_addr >> 8, src_addr & 0xFF]
        buf += [0]
        buf += [len(data) >> 8, len(data) & 0xFF]
        buf += data
        self.EciSendCommand(self.Commands.DL_Data_Request, buf)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def DL_Data_Confirm (self):
        self.EciSendCommand(self.Commands.DL_Data_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0, 0, 0, 0
        dsap = msg[1]
        ssap = msg[2]
        dst_addr  = (msg[3] << 8) + msg[4]
        src_addr  = (msg[5] << 8) + msg[6]
        txs = msg[7]
        return ec, dsap, ssap, dst_addr, src_addr, txs

    def MAC_MLME_GETrequest(self, pib):
        self.EciSendCommand(self.Commands.PIB_GETrequest, [pib >> 8, pib & 0xFF])
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_MLME_GETconfirm(self):
        self.EciSendCommand(self.Commands.PIB_GETconfirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            status = msg[1]
            pib = (msg[2] << 8) + msg[3]
            leng = msg[4]
            return ec, status, pib, msg[5:5 + leng]
        return ec, 0, 0, []

    def MAC_MLME_SETrequest(self, pib, value):
        msg = [pib >> 8, pib & 0xFF]
        msg += [len(value)]
        msg += value
        self.EciSendCommand(self.Commands.PIB_SETrequest, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_MLME_SETconfirm(self):
        self.EciSendCommand(self.Commands.PIB_SETconfirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1]
        return ec, 0

    def MAC_MLME_LIST_GETrequest(self, pib):
        self.EciSendCommand(self.Commands.MAC_MLME_LIST_GETrequest, [pib & 0xFF])
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_MLME_LIST_GETconfirm(self):
        self.EciSendCommand(self.Commands.MAC_MLME_LIST_GETconfirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            status = msg[1]
            pib = msg[2]
            size = (msg[3] << 8) + msg[4]
            raw_entries = msg[5:5 + size]
            return ec, status, pib, raw_entries
        return ec, 0, 0, []

    def CMD_MAC_TESTmode (self, tm):
        self.EciSendCommand(self.Commands.MAC_TESTmode, [(tm >> 24) & 0xFF, (tm >> 16) & 0xFF, (tm >> 8) & 0xFF, tm & 0xFF])
        _, res = self.EciWaitResponse()
        return res[0]

    def MAC_UNREGISTERrequest(self, eui=[0,0,0,0,0,0]):
        self.EciSendCommand(self.Commands.MAC_Unregister_Request, eui)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_UNREGISTERconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Unregister_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1]
        return ec, 0

    def MAC_UNREGISTERindication(self):
        self.EciSendCommand(self.Commands.MAC_Unregister_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1:7]
        return ec, [0, 0, 0, 0, 0, 0]

    def MAC_REGISTERSTATEindication(self):
        self.EciSendCommand(self.Commands.MAC_MLME_REGISTERSTATE_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            result = msg[1]
            sna = msg[2:2+6]
            eui = msg[8:8+6]
            return ec, result, sna, eui
        return ec, 0, [0,0,0,0,0,0], [0,0,0,0,0,0]

    def MAC_PROMOTIONrequest(self, eui=[0,0,0,0,0,0]):
        self.EciSendCommand(self.Commands.MAC_Promotion_Request, eui)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_PROMOTIONconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Promotion_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1]
        return ec, 0

    def MAC_PROMOTIONindication(self):
        self.EciSendCommand(self.Commands.MAC_Promotion_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1:7]
        return ec, [0, 0, 0, 0, 0, 0]

    def MAC_DEMOTIONrequest(self, eui=[0,0,0,0,0,0]):
        self.EciSendCommand(self.Commands.MAC_Demotion_Request, eui)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def MAC_DEMOTIONconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Demotion_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1]
        return ec, 0

    def MAC_DEMOTIONindication(self):
        self.EciSendCommand(self.Commands.MAC_Demotion_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            return ec, msg[1:7]
        return ec, [0, 0, 0, 0, 0, 0]

    def PHY_DATAindication(self):
        self.EciSendCommand(self.Commands.PHY_DATAIndication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            leng = (msg[1] << 8) + msg[2]
            lev = msg[3]
            sch = msg[4]
            tim = (msg[5] << 24) + (msg[6] << 16) + (msg[7] << 8) + msg[8]
            snr = msg[9]
            rq = msg[10]
            di = msg[11:11+leng]
            return ec, lev, sch, tim, snr, rq, di
        return ec, 0, 0, 0, 0, 0, []
     
    def PHY_DATAindication1_4(self):
        self.EciSendCommand(self.Commands.PHY_DATAIndication1_4, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            length = (msg[1] << 8) + msg[2]
            level = msg[3]
            modulation = (msg[4] << 4) | (msg[5] & 0xF)
            timer = (msg[6] << 24) + (msg[7] << 16) + (msg[8] << 8) + msg[9]
            snr = msg[10]
            rssi = msg[11]
            data = msg[12:12+length]
            return ec, level, modulation, timer, snr, rssi, data
        return ec, 0, 0, 0, 0, 0, []

    def PHY_DATArequest(self, data, level=7, sch=Modulation.TYPEA_DBPSKCC, tim=0):
        leng = len(data)
        msg = []
        msg += [((leng & 0xFF00) >> 8), leng & 0xFF]
        msg += [level]
        msg += [sch]
        msg += [(tim >> 24) & 0xFF, (tim >> 16) & 0xFF, (tim >> 8) & 0xFF, tim & 0xFF]
        msg += data

        self.EciSendCommand(self.Commands.PHY_DATARequest, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def PHY_DATArequest1_4(self, data, level=7, sch=Modulation.TYPEA_DBPSKCC, tim=0):
        # Notice: PHY Data Request has two 1.4-specific fields:
        # - Scheduled: set to 1 means that PHY is expected to delay
        #   the transmission till a given timer value is reached
        # - Timer: the timer value mentioned above.
        # These two fields are ignored by the RT-MAC when PHY Data Request
        # primitive is invoked. Hence they are just set to 0 here.
        leng = len(data)
        msg = []
        msg += [((leng & 0xFF00) >> 8), leng & 0xFF]
        msg += [level]
        msg += [(sch >> 4) & 0xF]   # Frame type
        msg += [sch & 0xF]          # Modulation
        msg += [0]                  # Scheduled
        msg += [(tim >> 24) & 0xFF, (tim >> 16) & 0xFF, (tim >> 8) & 0xFF, tim & 0xFF]
        msg += data

        self.EciSendCommand(self.Commands.PHY_DATARequest1_4, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def PHY_DATAconfirm(self):
        self.EciSendCommand(self.Commands.PHY_DATAConfirm, [])
        _, msg = self.EciWaitResponse()
        return msg[0]

    def SwReset(self):
        self.EciSendCommand(self.Commands.SW_Reset, [])
        time.sleep(1)
        self.serlock.release()
    
    def RFU_EVENT_SETrequest(self, eventid, parlist):
        leng = len(parlist)
        msg = []
        msg += [(eventid)]
        msg += [(leng)]
        msg += parlist

        self.EciSendCommand(self.Commands.RFU_EVENT_SETrequest, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]  
    
    def RFU_EVENTindication(self):
        self.EciSendCommand(self.Commands.RFU_EVENTindication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            eventid = (msg[1])
            leng = (msg[2])
            parlist = msg[3:(3+leng)]
            return ec, eventid, leng, parlist
        return ec, 0, 0, []
    
    def RFU_EVENTindicationConfirm(self, eventid):
        msg = []
        msg += [(eventid)]

        self.EciSendCommand(self.Commands.RFU_EVENTindicationConfirm, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL_NULL_ESTABLISHrequest(self, eui=[0,0,0,0,0,0], typ=0, arq=False, data=[], cfb=[0,0]):
        msg = []
        msg += eui
        msg += [typ]
        if arq:
            msg += [1]
        else:
            msg += [0]
        msg += cfb
        msg += [len(data)]
        msg += data

        self.EciSendCommand(self.Commands.MAC_Establish_Request, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL_NULL_ESTABLISHconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Establish_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0, 0, 0, 0
        ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
        res = msg[5]
        eui = msg[6:6+6]
        typ = msg[12]
        leng = msg[13]
        data = msg[14:14+leng]
        return ec, ch, res, eui, typ, data

    def CL_NULL_ESTABLISHindication(self):
        self.EciSendCommand(self.Commands.MAC_Establish_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            eui = msg[5:5+6]
            typ = msg[11]
            cfb = (msg[12] << 8) + msg[13]
            leng = msg[14]
            data = msg[15:15+leng]
            return ec, ch, eui, typ, cfb, data
        return ec, 0, 0, 0, 0, 0

    def CL_NULL_ESTABLISHresponse(self, ch, answer, data=[]):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += [answer]
        msg += [len(data)]
        msg += data
        self.EciSendCommand(self.Commands.MAC_Establish_Response, msg)
        _, res = self.EciWaitResponse()
        return res[0]

    def CL_NULL_RELEASErequest(self, ch):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        self.EciSendCommand(self.Commands.MAC_Relase_Request, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL_NULL_RELEASEconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Release_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0
        ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
        answer = msg[5]
        return ec, ch, answer

    def CL_NULL_RELEASEindication(self):
        self.EciSendCommand(self.Commands.MAC_Release_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            reason = msg[5]
            return ec, ch, reason
        return ec, 0, 0

    def CL_NULL_RELEASEresponse(self, ch, answer):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += [answer]
        self.EciSendCommand(self.Commands.MAC_Release_Response, msg)
        _, res = self.EciWaitResponse()
        return res[0]

    def CL_NULL_JOINrequest(self, bcast, ch, eui=[0,0,0,0,0,0], typ=0, data=[]):
        msg = [bcast]
        msg += [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += eui
        msg += [typ]
        msg += [len(data)]
        msg += data
        self.EciSendCommand(self.Commands.MAC_Join_Request, msg)
        _, res = self.EciWaitResponse()
        return res[0]

    def CL_NULL_JOINconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Join_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            eui = msg[5:5+6]
            result = msg[11]
            return ec, ch, eui, result
        return ec, 0, [0,0,0,0,0,0], 0

    def CL_NULL_JOINindication(self):
        self.EciSendCommand(self.Commands.MAC_Join_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            eui = msg[5:5+6]
            typ = msg[11]
            leng = msg[12]
            data = msg[13:13+leng]
            return ec, ch, eui, typ, data
        return ec, 0, [0,0,0,0,0,0], 0, []

    def CL_NULL_JOINresponse(self, ch, eui=[0,0,0,0,0,0], answer=0):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += eui
        msg += [answer]
        self.EciSendCommand(self.Commands.MAC_Join_Response, msg)
        _, res = self.EciWaitResponse()
        return res[0]

    def CL_NULL_LEAVErequest(self, ch, eui):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += eui
        self.EciSendCommand(self.Commands.MAC_Leave_Request, msg)
        _, res = self.EciWaitResponse()
        return res[0]

    def CL_NULL_LEAVEconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Leave_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            eui = msg[5:5+6]
            result = msg[11]
            return ec, ch, eui, result
        return ec, 0, [0,0,0,0,0,0], 0

    def CL_NULL_LEAVEindication(self):
        self.EciSendCommand(self.Commands.MAC_Leave_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec == 0:
            ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
            eui = msg[5:5+6]
            return ec, ch, eui
        return ec, 0, [0,0,0,0,0,0]

    def CL_NULL_DATArequest(self, ch, data, prio):
        msg = [(ch >> 24) & 0xFF, (ch >> 16) & 0xFF, (ch >> 8) & 0xFF, ch & 0xFF]
        msg += [prio]
        msg += [len(data) >> 8, len(data) & 0xFF]
        msg += data
        self.EciSendCommand(self.Commands.MAC_Data_Request, msg)
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL_NULL_DATAconfirm(self):
        self.EciSendCommand(self.Commands.MAC_Data_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0, 0
        ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
        result = msg[5]
        data = msg[6:]
        return ec, ch, result, data

    def CL_NULL_DATAindication(self):
        self.EciSendCommand(self.Commands.MAC_Data_Indication, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0, 0
        ch = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]
        leng = (msg[5] << 8) + msg[6]
        data = msg[7:7+leng]
        return ec, ch, data
        
    #----------------------------- CL_IPv6 -----------------------------
    def CL_IPv6_EstablishRequest(self, ARQ=0):
        self.EciSendCommand(self.Commands.CL_IPv6_Establish_Request, [ARQ])
        _, msg = self.EciWaitResponse() # [state, MSG]
        return msg[0] 

    def CL_IPv6_EstablishConfirm(self):
        self.EciSendCommand(self.Commands.CL_IPv6_Establish_Confirm, [])
        _, msg = self.EciWaitResponse()   
        return msg[0]

    def CL_IPv6_ReleaseRequest(self):
        self.EciSendCommand(self.Commands.CL_IPv6_Release_Request, [])
        _, msg = self.EciWaitResponse()
        return msg[0]

    def CL_IPv6_ReleaseConfirm(self):
        self.EciSendCommand(self.Commands.CL_IPv6_Release_Confirm, [])
        _, msg = self.EciWaitResponse()
        ec = msg[0]
        if ec != 0:
            return ec, 0
        res = msg[1]
        return ec, res

    def CL_IPv6_DataRequest(self, data):
        '''
        \ndescription: This function sends data using CL_IPv6.
        \nreturn: 
        - 0x00 - no errors
        - ERR codes: 
            0xFD - ERR Layer is not active
            0x90 - MAC_SAP_NULLPOINTER_ERR  - data pointer is NULL
            0x91 - MAC_SAP_LENERR           - the size of PDU exceeds the maximum (1500)
            0x93 - MAC_SAP_EVENT_ERR        - the Convergence Layer IPv6 is already busy
        '''
        msg = [len(data) >> 8, len(data) & 0xFF]
        msg += data
        self.EciSendCommand(self.Commands.CL_IPv6_Data_Request, msg)
        _, respMsg = self.EciWaitResponse()
        return respMsg[0]
    
    def CL_IPv6_DataConfirm(self):
        self.EciSendCommand(self.Commands.CL_IPv6_Data_Confirm, [])
        _, msg = self.EciWaitResponse()
        return msg    

    def CL_IPv6_DataIndication(self):
        self.EciSendCommand(self.Commands.CL_IPv6_Data_Indicate, [])
        _, msg = self.EciWaitResponse()   
        ec = msg[0]     # Error Code
        #print("(SN) eci.CL_IPv6_DataIndication().self.EciWaitResponse() -> ec = " + hexList(ec)) ### --- DEBUG
        #print("(SN) eci.CL_IPv6_DataIndication().self.EciWaitResponse() -> msg = "+ hexList(msg)) ### --- DEBUG
        if ec != 0:
            return ec, 0, []
        dataLen = (msg[1]<<8)+msg[2] # SDU data length (16 bit)
        data = msg[3:dataLen+3]    # SDU data
        return ec , dataLen , data    

    #-------------------------------------------------------------------