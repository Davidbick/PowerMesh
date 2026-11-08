import time
from util_crc import Crc32

class SciInterface:
    GET_REQ =       0x00
    GET_RES =       0x01
    SET_REQ =       0x02
    RESET_STAT =    0x03
    REBOOT =        0x04

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

    def SciSendBuffer(self, buff):
        self.serlock.acquire()
        try:
            self.ser.write(buff)
        except Exception as ex:
            template = "SciSendBuffer An exception of type {0} occurred. Arguments:\n{1!r}"
            message = template.format(type(ex).__name__, ex.args)
            print(message)
            try:
                time.sleep(1)
                print("SciSendBuffer write retry\n")
                self.ser.write(buff)
            except Exception as ex:
                # write failed. Ignore and let's EciWaitResponse fail too.
                # It will release the lock.
                # [AP:TODO: manage in a better way, avoiding to call EciWaitResponse]
                template = "An exception of type {0} occurred. Arguments:\n{1!r}"
                message = template.format(type(ex).__name__, ex.args)
                print(message)
                time.sleep(1)
                print("SciSendBuffer Serial write failed again, ignore\n")  

    def SciWaitResponse(self, timeout=15):
        endtime = time.time() + timeout
        buff = []
        try:
            buff += [bytearray(self.ser.read(1))[0]]
        except:
            self.serlock.release()
            return [0, 0]

        while True:
            try:
                buff += [bytearray(self.ser.read(1))[0]]
            except:
                self.serlock.release()
                return [0, 0]
            if buff[-1] == 0x7e:
                break
            if time.time() > endtime:
                self.serlock.release()
                return [0, 0]
        #print "sci resp ok, len = " + str(len(buff[1:-1]))
        self.serlock.release()
        uns = self.CommandUnstuff(buff[1:-1])
        crc32 = (uns[-4] << 24) | (uns[-3] << 16) | (uns[-2] << 8) | uns[-1]
        if Crc32.Calculate(uns[0:-4]) != crc32:
            return [0, 0]
        return uns

    def CommandStuff(self, message):
        stuffed = [0x7E]

        for b in message:
            if b == 0x7E:
                stuffed += [0x7D, 0x5E]
            elif b == 0x7D:
                stuffed += [0x7D, 0x5D]
            else:
                stuffed += [b]

        stuffed += [0x7E]
        return stuffed

    def CommandUnstuff(self, stuffed):
        message = []
        
        i = 0
        while i < len(stuffed):
            if stuffed[i] == 0x7D:
                i += 1
                if stuffed[i] == 0x5E:
                    message += [0x7E]
                else:
                    message += [0x7D]
            else:
                message += [stuffed[i]]
            i += 1

        return message

    def CommandAddHeaderAndCrc(self, cmd, payload):
        tmp = (len(payload) << 6) + cmd
        header = [tmp >> 8, tmp & 0xFF]

        tmp = Crc32.Calculate(header + payload)
        crc32 = [(tmp >> 24) & 0xFF, (tmp >> 16) & 0xFF, (tmp >> 8) & 0xFF, tmp & 0xFF]

        message = header + payload + crc32
        return self.CommandStuff(message)

    def CommandCreate(self, cmd):
        payload = []
        return self.CommandAddHeaderAndCrc(cmd, payload)

    def CommandCreateGet(self, cmd, pib, index):
        payload = [ (pib >> 8) & 0xFF, pib & 0xFF, index]
        return self.CommandAddHeaderAndCrc(cmd, payload)

    def CommandCreateSet(self, cmd, pib, value):
        payload = [ (pib >> 8) & 0xFF, pib & 0xFF]
        payload += value
        return self.CommandAddHeaderAndCrc(cmd, payload)

    def GetPib(self, pib):
        buff = self.CommandCreateGet(self.GET_REQ, pib, 0)
        #print "SCI GetPib command created: " + str(buff)
        self.SciSendBuffer(buff)
        
        #print "SCI GetPib command send"
        resp = self.SciWaitResponse()
        #print "SCI GetPib command response received: " + str(resp)
        leng = (((resp[0] << 8) + resp[1]) >> 6) - 4
        if leng <= 0:
            return []
        return resp[5:5+leng]

    def GetListEntry(self, lis, index):
        buff = self.CommandCreateGet(self.GET_REQ, lis, index)
        self.SciSendBuffer(buff)
        resp = self.SciWaitResponse()
        leng = (((resp[0] << 8) + resp[1]) >> 6) - 4
        if leng <= 0:
            val = None
        else:
            val = resp[5:5+leng]
        return val, (resp[5+leng] == 0)

    def SetPib(self, pib, value):
        buff = self.CommandCreateSet(self.SET_REQ, pib, value)
        self.SciSendBuffer(buff)
        self.serlock.release()

    def Reboot(self):
        buff = self.CommandCreate(self.REBOOT)
        self.SciSendBuffer(buff)
        self.serlock.release()

    def ResetStatistical(self):
        buff = self.CommandCreate(self.RESET_STAT)
        self.SciSendBuffer(buff)
        self.serlock.release()
