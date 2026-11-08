import os
import time
import serial
from threading import Thread
from threading import Lock
from utils import DfuUtils, StateMachine
from util_crc import Crc16
from global_prime_defs import Vendor
from sci import SciInterface
from eci import EciInterface
from servicenodeemb import ServiceNodeEmb

class StarComBootCmds:

    class Commands:
        COM_Init                =           0x01 # change the UART configuration
        SYS_Reset               =           0x02 # perform a system reset
        IMG_Init                =           0x11 # signal the start of an image download
        IMG_Write               =           0x12 # download an image block
        IMG_Start               =           0x13 # signal the end of the image and the request of start the related computing subsystem
        MIB_Get                 =           0x21 # request the value of a specific MIB      
     
    class State:
        ACK                     =       [0,0,0,0]
        NACK                    =       [1,0,0,0]
        ABORT                   =       [2,0,0,0]
       
       
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

    def STARCOMSendBootCommand(self, cmd, msg):
        buff = [0x16, 0x16] 
        buff += [cmd]
        leng = len(msg)
        buff += [leng & 0xFF, leng >> 8]
        buff += [0xFE] # mode
        buff += [0, 0, 0, 0] #state
        buff += msg
        crc = Crc16.Calculate(buff) # crc16
        #print crc
        buff_to_send = buff + [crc & 0x00FF] +  [(crc & 0xFF00)>>8] 
        #print buff_to_send
        
        self.serlock.acquire()
        try:
            self.ser.write(buff_to_send)
        except Exception as ex:
            template = "STARCOMSendBootCommand: An exception of type {0} occurred. Arguments:\n{1!r}"
            message = template.format(type(ex).__name__, ex.args)
            print(message)
            try:
                time.sleep(1)
                print("STARCOMSendBootCommand:  write retry\n")
                self.ser.write(buff_to_send)
            except Exception as ex:
                # write failed. Ignore and let's EciWaitResponse fail too.
                # It will release the lock.
                # [AP:TODO: manage in a better way, avoiding to call EciWaitResponse]
                template = "An exception of type {0} occurred. Arguments:\n{1!r}"
                message = template.format(type(ex).__name__, ex.args)
                print(message)
                time.sleep(1)
                print("STARCOMSendBootCommand: Serial write failed again, ignore\n")  

    def STARCOMWaitBootResponse(self, timeout=15):
        endtime = time.time() + timeout
        pkt = []
        while True:
            try:
                pkt += [bytearray(self.ser.read(1))[0]]
            except:
                self.serlock.release()
                print("sono qui 1")
                return [0, 0, 0, 0], [0xFF]
            
            if len(pkt) == 5:
                leng = (pkt[4] << 8) + pkt[3]
                #print "rx MSGLEN: " + str(leng)
            elif len(pkt) > 5:
                if len(pkt) >= leng + 12:
                    break
            if time.time() > endtime:
                self.serlock.release()
                print("sono qui 2")
                return [0, 0, 0, 0], [0xFF]
           
        self.serlock.release()
        #print pkt
        #print "pkt[0:-2]" + str(pkt[0:-2])
        #print "Crc16.Calculate(pkt[0:-2]) " + str(Crc16.Calculate(pkt[2:-2]))
        if Crc16.Calculate(pkt[0:-2]) != ((pkt[-1] << 8) + pkt[-2]): # 16
            print("sono qui 3")
            return [0, 0, 0, 0], [0xFF]
        return pkt[6:10], pkt[10:-2]

    def IMG_Init(self, img_header ):
        self.STARCOMSendBootCommand(self.Commands.IMG_Init, img_header)
        state, _ = self.STARCOMWaitBootResponse()
        return state
        
    def IMG_Write(self, img_data_block ): #img_data_block must be multiple of 16
        self.STARCOMSendBootCommand(self.Commands.IMG_Write, img_data_block)
        state, _ = self.STARCOMWaitBootResponse()
        return state

    def IMG_Start(self ):
        self.STARCOMSendBootCommand(self.Commands.IMG_Start, [])
        time.sleep(0.5)
        state, _ = self.STARCOMWaitBootResponse()
        return state

    def SYS_Reset(self):
        self.STARCOMSendBootCommand(self.Commands.SYS_Reset, [])
        state, _ = self.STARCOMWaitBootResponse()
        return state

    def COM_Init(self, baudratevalue):
        baudlist = []
        baudlist += [ baudratevalue & 0x000000FF]
        baudlist += [(baudratevalue & 0x0000FF00) >> 8]
        baudlist += [(baudratevalue & 0x00FF0000) >> 16]
        baudlist += [(baudratevalue & 0xFF000000) >> 24]
        
        self.STARCOMSendBootCommand(self.Commands.COM_Init,baudlist + [0,1])
        state, _ = self.STARCOMWaitBootResponse()
        return state

    def MIB_Get(self, attributeid):
        self.STARCOMSendBootCommand(self.Commands.MIB_Get,attributeid)
        state, msg = self.STARCOMWaitBootResponse()
        return state,msg
        
 
        
class SN_STarCOMBootFromHost(ServiceNodeEmb):
 
    istanceCounter = 0
    

    def __init__(self, port, baud=57600):

        self.ImageSize       = 0
        self.safetytimer     = 0
        self.lastCRC32       = 0
        self.page_nbytes     = 0
        self.Offset      = 0
        self.pageBytes       = [] 
        self.RfuFSM = []
        self.serialcom       = []
        self.seriallock      = []
        SN_STarCOMBootFromHost.istanceCounter += 1
        self.HI_baud = baud
        self.HI_port = port
        self.BOOT_port = self.HI_port
        self.eraseendtime = 0
        self.safetyendtime = 0
        self.rte_update = []
        self.cpu_update = []
        self.firsttime = 0;
         
        self.BootSTarCOMImgs(DfuUtils.dfu_file_starcom_cpu_libCompiled, DfuUtils.dfu_file_starcom_rte)
        self.startRFUFSMThread()       
        time.sleep(1)
        print("SCI GetPib ProductID")
        self.ProductID = self.GetPib(self.Pibs.AppProductId)
        print("SCI GetPib ProductID ok, value: " + str(self.ProductID))
        if self.ProductID == Vendor.ProductId.SN_EMB:
            #use 0x0000 pib to access EUI48
            self.eui48 = self.GetPib(0x0000, iface = "ECI")
            print("ECI GetPib EUI48 ok")
        else:
            self.eui48 = self.GetPib(self.Pibs.macEUI48)
            print("SCI GetPib EUI48 ok")
            
 
        if self.SetPrimeConfigPhy() == False:
            print("SCI SetPrimeConfigPhy not ok")
            return
        print("SCI SetPrimeConfigPhy ok")
        self.InitFailure = False
    
    def __del__(self):
        self.Stop()
        self.stopRFUFSMThread()
        if not self.InitFailure:
            
            del self.eci
            print("del self.eci")
            del self.sci
            print("del self.sci")
            del self.StarcomBoot

            self.serialcom.close()
            del self.serialcom
            print("del self.serialcom")  
            
    def SerialComClose(self):
        pass
        
    def SerialComOpen(self):
        pass
            
    def BootImg(self,image):
    
        with open(image,"rb") as f:
            fileContent = bytearray(f.read())
             
        payload_Start_offset = (fileContent[84] | (fileContent[85]<<8) | (fileContent[86] << 16) | (fileContent[87]<< 24)) * 8 + 88
        #payload_Start_offset = (fileContent[84]) * 8 + 88
        payload_Start_offset += payload_Start_offset % 16 #padding to 16
        payload_size = ((fileContent[40]) | (fileContent[41]<<8)  | (fileContent[42] << 16) | (fileContent[43]<< 24))
        print("payload_size: " + str(payload_size))
        state = self.StarcomBoot.IMG_Init(fileContent[0:payload_Start_offset])
        
        while state == self.StarcomBoot.State.NACK:
            state = self.StarcomBoot.IMG_Init(fileContent[0:payload_Start_offset])
            print("repeat IMG_Init")
        
        payloadContent = fileContent[payload_Start_offset: (payload_Start_offset + payload_size)]
   
        pagesize = 1024
        
        numOfChunck = len(payloadContent) / pagesize
        remainingbytes = len(payloadContent) % pagesize
        chunkCnt = 0
        while (chunkCnt < numOfChunck):
            chunkData = list(range(0, pagesize))
            chunkData[0:pagesize] = payloadContent[chunkCnt*pagesize : (chunkCnt+1)*pagesize]
            state = self.StarcomBoot.IMG_Write(chunkData)
            while state == self.StarcomBoot.State.NACK:
                state = self.StarcomBoot.IMG_Write(chunkData)
                print("repeat IMG_Write")
            chunkCnt = chunkCnt + 1
            
        
    
        if (remainingbytes != 0):
            chunkData = list(range(0, remainingbytes))
            chunkData[0:remainingbytes] = payloadContent[chunkCnt*pagesize : chunkCnt*pagesize + remainingbytes]   
            state = self.StarcomBoot.IMG_Write(chunkData)
            while state == self.StarcomBoot.State.NACK:
                state = self.StarcomBoot.IMG_Write(chunkData)
                print("repeat IMG_Write")
            
        self.StarcomBoot.IMG_Start()
     
        while state == self.StarcomBoot.State.NACK:
            state = self.StarcomBoot.IMG_Write(chunkData)
            print("repeat IMG_Start")
            
        print("BootOK")  
        
    def BootSTarCOMImgs(self, cpu_bin_file, rte_img_file, cpu_img_file = []):
  
        #boot UPDATE code
        try:
            print("OpenCom SN_STarCOMBootFromHost")
            self.serialcom = serial.Serial(self.BOOT_port, 9600, timeout=1)
        except:
            time.sleep(0.5) # fake init time
            print("OpenCom fail")
            return
        print("OpenCom ok")
        self.seriallock = Lock()

        self.StarcomBoot = StarComBootCmds(self.serialcom, self.seriallock)
        
        state = self.StarcomBoot.COM_Init(921600)
        print("StarComBootCmds istance, COM_Inited")
        while state == self.StarcomBoot.State.NACK:
            state = self.StarcomBoot.COM_Init(921600)
            print("repeat COM_Init")
            
        try:
            self.serialcom.close() 
            self.serialcom = serial.Serial(self.BOOT_port, 921600, timeout=1)
        except:
            time.sleep(0.5) # fake init time
            #print "OpenCom fail"
            return
        del self.StarcomBoot        
        self.StarcomBoot = StarComBootCmds(self.serialcom, self.seriallock)
        
        #BOOT RTE IMG image 
        rte_file = os.path.join(DfuUtils.dfu_files_folder, rte_img_file)   
        self.BootImg(rte_file)
        #BOOT PE IMG image           
        if (cpu_img_file == []): #if the IMG is not passed it is created from the BIN file pointed by DfuUtils.dfu_files_folder + cpu_bin_file
            cpu_file = os.path.join(DfuUtils.dfu_files_folder, cpu_bin_file)
            with open(cpu_file,"rb") as f:
                s=f.read()
            # the BIN is copied into a fixed name (dfu_file_starcom_cpu_libCompiled) file, in order to reuse the STarCOM_config.xml (already used for COD file generation) 
            cpu_mod = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_starcom_cpu_libCompiled[:-3] + "mod")
            with open(cpu_mod,"wb") as f:
                f.write(s)
                
            imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "STarCOM_ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\STarCOM_config.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STARCOM_CPU.img" + " -b") 
            os.system(imagegeneratorCmd + "> t.txt");  #out redirected to a temp file (not leave to the screee) because BootSTarCOMImgs is called on every testcase setup
            self.BootImg(os.path.join(DfuUtils.dfu_files_folder, "STARCOM_CPU.img"))    
        else:
            #BOOT the PE IMG image passing its full path        
            self.BootImg(os.path.join(DfuUtils.dfu_files_folder, cpu_img_file))  
        
        try:
            self.serialcom.close() 
            self.serialcom = serial.Serial(self.HI_port, self.HI_baud, timeout=1)
        except:
            time.sleep(0.5) # fake init time
            #print "OpenCom fail"
            return
        
        del self.StarcomBoot
        self.seriallock = Lock()
        self.StarcomBoot = StarComBootCmds(self.serialcom, self.seriallock)
        self.sci = SciInterface(self.serialcom, self.seriallock)
        self.eci = EciInterface(self.serialcom, self.seriallock)
    
    def startRFUFSMThread(self):
    
        self.RfuFSM = StateMachine()
        self.RfuFSM.add_state( self.RfuStates.IDLE,              self.idle_transitions)
        self.RfuFSM.add_state( self.RfuStates.INIT_STORAGE,      self.init_storage_transitions)
        self.RfuFSM.add_state( self.RfuStates.SAVE_PAGE,         self.save_page_transitions)
        self.RfuFSM.add_state( self.RfuStates.WAIT_FU_CONFIRM,   self.wait_fu_confirm_transitions)
        self.RfuFSM.set_start( self.RfuStates.IDLE)
        
        self.rfufsmthread = Thread(target=self.RFUFSMThread)
        self.rfufsmthread.start()
        
    def stopRFUFSMThread(self):
        del self.RfuFSM
        self.rfufsmthread.stop()
        del self.rfufsmthread
        
    def getEvParValue(self,parlist, EvParPos, EvParDim):
        i = EvParDim * 8 - 8 
        value = 0
        for j in range (EvParPos, EvParPos + EvParDim ):
            value += (parlist[j] << i) 
            i -= 8
            
        return value

    def fillRFUEvParameters(self, parlist):

        if(parlist != []):
            self.ImageSize       = self.getEvParValue(parlist,0,4) 
            self.safetytimer     = self.getEvParValue(parlist,4,2)  
            self.lastCRC32       = self.getEvParValue(parlist,6,4)
            self.page_nbytes     = self.getEvParValue(parlist,10,1)
            self.Offset          = self.getEvParValue(parlist,11,4)
            if self.page_nbytes > 0:
                self.pageBytes       = parlist[15: 15 + (self.page_nbytes)]
                
            else:
                self.pageBytes = []
            #print "\n fillRFUEvParameters ImageSize safetytimer lastCRC32 page_nbytes Offset " + str(self.ImageSize) + " "+ str(self.safetytimer) + " "+ str(self.lastCRC32) + " "+ str(self.page_nbytes) + " "+ str(self.Offset) 
            
            
        
        
        

    def RFUFSMThread(self):  
        
        CTSState = self.serialcom.getCTS()
        CTSCurrentState = CTSState
        print("Intial CTS State is: " + str (CTSState))
        while(True):
            eventid = self.RfuEvents.FU_NO_EVENT 
            length = 0
            parlist = []
            CTSCurrentState = self.serialcom.getCTS()
            if (CTSCurrentState != CTSState):
                CTSState = CTSCurrentState
                
            if (CTSState == False): # if CTS is fale, an event has been produced by the modem
                
                state = self.GetState()       # Calling GetState to understand which events are pending to be consumed
                                
                #print "\State " + str(state)
                if (state & self.Indication.MAC_REGISTERSTATEINDICATION): #if is an MAC_REGISTERSTATEINDICATION
                    self.eci.MAC_REGISTERSTATEindication() #consumed only for not sending GetState continuosly
                    
                if (state & self.Indication.RFU_EVENTINDICATION):                
                    
                    ec, eventid, length, parlist = self.eci.RFU_EVENTindication() #send the RFU_EVENTindication message to the modem for consuming the related event
                    
                    if (ec != 0x00) : #if EC is 0 the is a problem, the thread is halted
                        print("RFU inconsistent event signaling")
                        return
                    if(self.firsttime and eventid == 8):
                        self.firsttime = 0;
                        print("\n\n\n\n\n\n\n")
                        print("eventid: " + str(eventid))
                    self.fillRFUEvParameters(parlist) #get all the event attributes
                    print("\nOffset " + str(self.Offset))

                    
            self.RfuFSM.handle_event(eventid, length, parlist)
            
            time.sleep(0.015)            
  
    def idle_transitions(self, eventid, parlist = []):
        nextState = self.RfuStates.IDLE
        if eventid == self.RfuEvents.FU_NO_EVENT:
            pass
        elif eventid == self.RfuEvents.FU_START:
            nextState = self.RfuStates.INIT_STORAGE            

            # set an erase timeout in order to simulate the time need to erase the block
            self.eraseendtime = time.time() + 5       
            
            # create the files STarCOMFromHost_istanceN_PE.update and STarCOMFromHost_istanceN_RTE.update where N is taken from the static variable istanceCounter
            # on embedeed host without file system, a block will be erase and prepared to get the COD file (cpu+rte) 
            rte_updatepath = os.path.join(DfuUtils.dfu_files_folder, "STarCOMFromHost_" +str(SN_STarCOMBootFromHost.istanceCounter) + "_RTE.update")
            cpu_updatepath = os.path.join(DfuUtils.dfu_files_folder, "STarCOMFromHost_" +str(SN_STarCOMBootFromHost.istanceCounter) + "_CPU.update")
            try: 
                self.rte_update = open(rte_updatepath,"w+b")
                self.cpu_update = open(cpu_updatepath,"w+b")
            except:
                print (" fopen error in idle_transitions")
                return
        else:
            print("Unforseen RFU event " + str(eventid) + "in idle_transitions")
        return nextState
    
    def init_storage_transitions(self, eventid, parlist = []):
        nextState = self.RfuStates.INIT_STORAGE
        if eventid == self.RfuEvents.FU_NO_EVENT:
            #check if erasetimeout is set and elapsed. if it is, sends a STORAGE_READY event to the modem  and goes to SAVE_PAGE state
            if time.time() > self.eraseendtime :
                self.eci.RFU_EVENT_SETrequest(self.RfuEvents.STORAGE_READY,[])
                nextState = self.RfuStates.SAVE_PAGE
        else:
            print("Unforseen RFU event " + str(eventid) + "in init_storage_transitions")
        return nextState
 
    def save_page_transitions(self, eventid, parlist = []):
        nextState = self.RfuStates.SAVE_PAGE
        if eventid == self.RfuEvents.FU_NO_EVENT:
            pass
        elif eventid == self.RfuEvents.FU_DATA:
            #store the page in the UPDATE block using the Offset info carried with the event
            #here the incoming CPU and RTE portion are saved in the rteupdatefile. The CPU portion will be copied in the CPU file
            self.rte_update.seek(self.Offset)
            self.rte_update.write(bytearray(self.pageBytes))
            time.sleep(0.2)

            
        elif eventid == self.RfuEvents.DATA_BW:
            #read the page in the UPDATE block using the Offset info carried with the event
            #here the incoming CPU and RTE portion are read from the rteupdatefile. The CPU portion will be copied in the CPU file
            self.rte_update.seek(self.Offset)
            
            data = bytearray(self.rte_update.read(int(self.page_nbytes)))
            parlist = [0] * (15 + self.page_nbytes)
            parlist[10] = self.page_nbytes
            parlist[11] = (self.Offset & 0xFF000000) >> 24
            parlist[12] = (self.Offset & 0x00FF0000) >> 16
            parlist[13] = (self.Offset & 0x0000FF00) >> 8
            parlist[14] =  self.Offset & 0x000000FF
            parlist[15:] = data
            
            self.eci.RFU_EVENT_SETrequest(self.RfuEvents.DATA_BW,parlist)
      
        elif eventid == self.RfuEvents.FU_UPGRADE:
            print("RFU EVENT FU_UPGRADE")
            self.rte_update.seek( 0x20000 )
            cpu = self.rte_update.read()
            self.cpu_update.write(cpu)
            self.rte_update.seek( 0 )
            self.rte_update.truncate(0x20000)
            self.rte_update.close()
            self.cpu_update.close()
            
            #the STarCOM must be reset (SW reset sent)
            self.eci.SwReset();
            del self.eci
            del self.sci
            self.serialcom.close()
            del self.serialcom           
            self.BootSTarCOMImgs([],  "STarCOMFromHost_" +str(SN_STarCOMBootFromHost.istanceCounter)+ "_RTE.update",   "STarCOMFromHost_" +str(SN_STarCOMBootFromHost.istanceCounter)+ "_CPU.update")
         
            #the safetytimeout is set using the bytes in the event
            self.safetyendtime = time.time() + self.safetytimer
            # goes into WAIT_FU_CONFIRM state
            nextState = self.RfuStates.WAIT_FU_CONFIRM
        else:
            print("Unforseen RFU event " + str(eventid) + " in save_page_transitions")
        return nextState 

    def wait_fu_confirm_transitions(self, eventid, parlist = []):
        nextState = self.RfuStates.WAIT_FU_CONFIRM
        if eventid == self.RfuEvents.FU_NO_EVENT:
            #check if safetytimeout is set and elapsed. if it is, 
            if time.time() > self.safetyendtime :
            
                # boot STABLE code
                #the STarCOM must be reset (SW reset sent)
                
                self.eci.SwReset();
                del self.eci
                del self.sci
                self.serialcom.close()
                del self.serialcom  
                self.BootSTarCOMImgs(DfuUtils.dfu_file_starcom_cpu_libCompiled, DfuUtils.dfu_file_starcom_rte)
                # goes into WAIT_FU_CONFIRM state
                nextState = self.RfuStates.IDLE
                
        elif eventid == self.RfuEvents.FU_CONFIRM:        
            print("RFU EVENT FU_CONFIRM")
            nextState = self.RfuStates.IDLE
        else:
            print("Unforseen RFU event " + str(eventid) + "in wait_fu_confirm_transitions")
        return nextState        
