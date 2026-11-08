import os
import math
import config
import ctypes
import subprocess
import time
import sys
from threading import Thread, Event
from protocol import TimeConst, Addresses
from collections import deque
from datetime import timedelta
from global_prime_defs import Vendor, DFU_FileGenerationMode

def ClearCache():
    try:
        ctypes._reset_cache()
    except:
        ctypes._pointer_type_cache.clear()
        ctypes._c_functype_cache.clear()
        if os.name in ("nt", "ce"):
            ctypes._win_functype_cache.clear()
        ctypes.POINTER(ctypes.c_wchar).from_param = ctypes.c_wchar_p.from_param
        ctypes.POINTER(ctypes.c_char).from_param = ctypes.c_char_p.from_param
        ctypes._pointer_type_cache[None] = ctypes.c_void_p
        ctypes.CFUNCTYPE(ctypes.c_int)(lambda: None)


class StateMachine:
    def __init__(self):
        self.handlers = {}
        self.startState = None
        self.currentHandler = {}

    def add_state(self, name, handler):
        name = name.upper()
        self.handlers[name] = handler

    def set_start(self, name):
        self.startState = name.upper()
        self.currentHandler = self.handlers[self.startState]

    def handle_event(self, eventid, length = 0, parlist = []): #in funzione dello stato corrente (associato all'handler corrent) viene 
        nextState = self.currentHandler(eventid, length, parlist)
        self.currentHandler = self.handlers[nextState.upper()]

class DfuUtils:
    
    dfu_files_folder = os.path.join("..", "DFU_files")
    
    dfu_file_emb = "CM3_PRIME_1.3.6_SN_Library.bin"
    dfu_file_stm32 = "CM3_PRIME_1.3.6_SN_Library.bin"
    dfu_file_stcomet_cpu = "STCOMET_CPU.bin"
    dfu_file_stcomet_cpu_libCompiled = "STCOMET_CPU_libCompiled.bin"
    dfu_file_starcom_cpu_libCompiled = "STARCOM_CPU_libCompiled.bin"
    dfu_file_stcomet_rte = "STCOMET_RTE.img"
    dfu_file_starcom_rte = "STARCOM_RTE.img"
    
    @staticmethod
    def CreateSnEmbFile(fw_versions):
        # TODO 
        return os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_emb)

    @staticmethod
    def CreateSnStm32File(fw_versions):
        # create a file with different version
        original = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stm32)
        fileupgrade = original + ".mod"
        int_versions = []
        for v in fw_versions:
            int_versions += [v[7]]
        with open(original,"rb") as f:
            s=f.read()
        if len(int_versions) > 0:
            m = max(int_versions) + 1
            to_rep = bytearray("LeTtErVeRsIoN:" + chr(m), "latin1")
            for v in int_versions:
                to_fin = bytearray("LeTtErVeRsIoN:" + chr(v), "latin1")
                s = s.replace(to_fin, to_rep)
        with open(fileupgrade,"wb") as f:
            f.write(s)
        return fileupgrade
    
    @staticmethod
    def CreateSnStcometFile(fw_versions, DFU_File_Generation_Mode , signatureLen=0):
        '''
        HERE the COD file must be created according to 1.4 format (image+signature) as function of
        the specific DFU_File_Generation_Mode passed. However, until a clarification about the algoritms
        specified, this function (and all the python tool) will not manage the RSA and ECDSA signature 
        algoritms. The unique signature algoritms managed for the moment are NOSIGN and STVENDOR
        '''
        
        os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stcomet_rte)          
        CPUbin_Original = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stcomet_cpu)
        CPUbin_mod = os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU.mod")
            
        int_versions = []
        for v in fw_versions:
            int_versions += [v[7]]
        with open(CPUbin_Original,"rb") as f:
            s=f.read()
        if len(int_versions) > 0:
            m = max(int_versions) + 1
            to_rep = bytearray("LeTtErVeRsIoN:" + chr(m), "latin1")
            for v in int_versions:
                to_fin = bytearray("LeTtErVeRsIoN:" + chr(v), "latin1")
                s = s.replace(to_fin, to_rep)
        with open(CPUbin_mod,"wb") as f:
            f.write(s)
            
        imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\configCPU.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STCOMET_CPU.img" + " -b") 
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(imagegeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
                
        imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\configBundle.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.img " + " -b") 
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(imagegeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
                
        if (DFU_File_Generation_Mode == DFU_FileGenerationMode.NOSIGN):               
            subprocess.call("move " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.img " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.cod", shell=True)    
            return os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU_RTE.cod")
            
        if (DFU_File_Generation_Mode == DFU_FileGenerationMode.STVENDOR or DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTPAY or DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTSIG):
    
            # generate cod file. "ciaociaociaociaociaociaociaociao" is the signature and "0" is the initial crc value 
            if(DFU_File_Generation_Mode != DFU_FileGenerationMode.CORRUPTSIG):    
                codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 0 " +  DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.img " + " 0 ciaociaociaociaociaociaociaociao")
            else:
                codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 0 " +  DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.img " + " 0 ciaociaociaociaociaociaociaocia1")
                
            #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
            proc = subprocess.Popen(codFileGeneratorCmd, close_fds=True);
            endtime = time.time() + 60
            while proc.poll() == None:
                # We can do other things here while we wait
                time.sleep(1)
                proc.poll()
                if time.time() > endtime:
                    proc.terminate()
                    break              
                
            subprocess.call("move output.cod " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.cod", shell=True)
            
            if(DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTPAY):
                with open(DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE.cod","r+t") as file:     
                    file.seek(0)
                    file.write(str(5)) # 0x35 is written in pos. 0
                    file.close()                    
                    
            return os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU_RTE.cod")
        
    @staticmethod 
    def CreateSnStarcomFileLibCompiled(fw_versions, DFU_File_Generation_Mode , signatureLen=0):
        
        RTEimage = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_starcom_rte)    
        
        CPUbin_Original = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_starcom_cpu_libCompiled)
        CPUbin_mod = os.path.join(DfuUtils.dfu_files_folder, "STARCOM_CPU_libCompiled.mod")
        
        int_versions = []
        for v in fw_versions:
            int_versions += [v[7]]
        with open(CPUbin_Original,"rb") as f:
            s=f.read()
        if len(int_versions) > 0:
            m = max(int_versions) + 1
            to_rep = bytearray("LeTtErVeRsIoN:" + chr(m), "latin1")
            for v in int_versions:
                to_fin = bytearray("LeTtErVeRsIoN:" + chr(v), "latin1")
                s = s.replace(to_fin, to_rep)
        with open(CPUbin_mod,"wb") as f:
            f.write(s) #andrea del [0:135000]
                 
        imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "STarCOM_ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\STarCOM_config.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STARCOM_CPU.img" + " -b") 
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(imagegeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
           
        # generate cod file. "0014goodgoodgoodgoodgoodgoodgood" is the signature and "0" is the initial crc value 
        if(DFU_File_Generation_Mode != DFU_FileGenerationMode.CORRUPTSIG):    
            codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 1 " +  DfuUtils.dfu_files_folder + "\STARCOM_RTE.img " +  DfuUtils.dfu_files_folder + "\STARCOM_CPU.img " + " 0 0014goodgoodgoodgoodgoodgoodgood")
        else:
            codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 1 " +  DfuUtils.dfu_files_folder + "\STARCOM_RTE.img " +  DfuUtils.dfu_files_folder + "\STARCOM_CPU.img " + " 0 0014goodgoodgoodgoodgoodgoodgoo1")
       
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(codFileGeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
                
        subprocess.call("move output.cod " + DfuUtils.dfu_files_folder + "\STARCOM_RTE_CPU_libCompiled.cod", shell=True)

        
        if(DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTPAY):
            with open(DfuUtils.dfu_files_folder + "\STARCOM_RTE_CPU_libCompiled.cod","r+t") as file:     
                file.seek(0)
                file.write(str(5))
                file.close()  
                    
        return os.path.join(DfuUtils.dfu_files_folder, "STARCOM_RTE_CPU_libCompiled.cod")

    @staticmethod
    def CreateSnStcometFileLibCompiled(fw_versions, DFU_File_Generation_Mode , signatureLen=0):
        '''
        HERE the COD file must be created according to 1.4 format (image+signature) as function of
        the specific DFU_File_Generation_Mode passed. However, until a clarification about the algoritms
        specified, this function (and all the python tool) will not manage the RSA and ECDSA signature 
        algoritms. The unique signature algoritms managed for the moment are NOSIGN and STVENDOR
        '''
        
        os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stcomet_rte)          
        CPUbin_Original = os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stcomet_cpu_libCompiled)
        CPUbin_mod = os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU_libCompiled.mod")
            
        int_versions = []
        for v in fw_versions:
            int_versions += [v[7]]
        with open(CPUbin_Original,"rb") as f:
            s=f.read()
        if len(int_versions) > 0:
            m = max(int_versions) + 1
            to_rep = bytearray("LeTtErVeRsIoN:" + chr(m), "latin1")
            for v in int_versions:
                to_fin = bytearray("LeTtErVeRsIoN:" + chr(v), "latin1")
                s = s.replace(to_fin, to_rep)
        with open(CPUbin_mod,"wb") as f:
            f.write(s)
            
        imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\configCPU.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STCOMET_CPU.img" + " -b") 
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(imagegeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
                
        imagegeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "ImageGenerator.exe -c=" + DfuUtils.dfu_files_folder + "\configBundle.xml" + " -o=" + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.img" + " -b") 
        #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
        proc = subprocess.Popen(imagegeneratorCmd, close_fds=True);
        endtime = time.time() + 60
        while proc.poll() == None:
            # We can do other things here while we wait
            time.sleep(1)
            proc.poll()
            if time.time() > endtime:
                proc.terminate()
                break
                
        if (DFU_File_Generation_Mode == DFU_FileGenerationMode.NOSIGN):               
            subprocess.call("move " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.img " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.cod", shell=True)    
            return os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU_RTE_libCompiled.cod")
            
        if (DFU_File_Generation_Mode == DFU_FileGenerationMode.STVENDOR or DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTPAY or DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTSIG):
    
            # generate cod file. "ciaociaociaociaociaociaociaociao" is the signature and "0" is the initial crc value 
            if(DFU_File_Generation_Mode != DFU_FileGenerationMode.CORRUPTSIG):    
                codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 0 " +  DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.img " + " 0 ciaociaociaociaociaociaociaociao")
            else:
                codFileGeneratorCmd = os.path.join(DfuUtils.dfu_files_folder, "CODfile_Generator1_4.exe " + " 0 " +  DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.img " + " 0 ciaociaociaociaociaociaociaocia1")
                
            #used to avoid jenkins hang up, details about subprocesses in python here https://docs.python.org/2.7/library/subprocess.html
            proc = subprocess.Popen(codFileGeneratorCmd, close_fds=True);
            endtime = time.time() + 60
            while proc.poll() == None:
                # We can do other things here while we wait
                time.sleep(1)
                proc.poll()
                if time.time() > endtime:
                    proc.terminate()
                    break              
                
            subprocess.call("move output.cod " + DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.cod", shell=True)
            
            if(DFU_File_Generation_Mode == DFU_FileGenerationMode.CORRUPTPAY):
                with open(DfuUtils.dfu_files_folder + "\STCOMET_CPU_RTE_libCompiled.cod","r+t") as file:     
                    file.seek(0)
                    file.write(str(5)) # 0x35 is written in pos. 0
                    file.close()                    
                    
            return os.path.join(DfuUtils.dfu_files_folder, "STCOMET_CPU_RTE_libCompiled.cod")

    @staticmethod
    def CreateDfuFileToSend(nodes=[], DFU_FileGenerationMode=DFU_FileGenerationMode.NOSIGN, signatureLen=0):
        
        if len(nodes) == 0:
            # no nodes to upgrade, it's only to generate traffic
            return os.path.join(DfuUtils.dfu_files_folder, DfuUtils.dfu_file_stm32)
        
        product_ids = []
        fw_versions = []
        for node in nodes:
            p_id = node.GetPib(node.Pibs.AppProductId)
            product_ids += [p_id]
            # check if nodes have the same product ID
            if p_id != product_ids[0]:
                return ""
            fw_versions += [node.GetPib(node.Pibs.AppFwVersion)]
        
        if product_ids[0] == Vendor.ProductId.SN_EMB:
            return DfuUtils.CreateSnEmbFile(fw_versions)
        elif product_ids[0] == Vendor.ProductId.SN_STM32:
            return DfuUtils.CreateSnStm32File(fw_versions)
        elif product_ids[0] == Vendor.ProductId.SN_STCOMET:
            return DfuUtils.CreateSnStcometFile(fw_versions, DFU_FileGenerationMode, signatureLen)
        elif product_ids[0] == Vendor.ProductId.SN_STCOMET_LIBCOMPILED:
            return DfuUtils.CreateSnStcometFileLibCompiled(fw_versions, DFU_FileGenerationMode, signatureLen)
        elif product_ids[0] == Vendor.ProductId.SN_STARCOM_LIBCOMPILED:
            return DfuUtils.CreateSnStarcomFileLibCompiled(fw_versions, DFU_FileGenerationMode, signatureLen)
        return ""

class DevUtils:
    @staticmethod
    def Requirements(*args):
        for arg in args:
            if arg.InitFailure:
                return False
        return True

class StatUtils:
    @staticmethod
    def UniformityCheck(samples, lower_limit, upper_limit , macPromotionMinTxPeriod, macPromotionMaxTxPeriod):
        confidence = {
                      "0.10":   1.22,
                      "0.05":   1.36,
                      "0.025":  1.48,
                      "0.01":   1.63,
                      "0.005":  1.73,
                      "0.001":  1.95,
                      }
                                           
              
        numOfClasses =  len(samples)
        threshold = confidence["0.05"] / math.sqrt(len(samples))
        if len(samples) < 1:
            return False, 0, 0 # too few samples
        exp_cumul = [] # cumulative probability expected from an uniform distribution
        mea_cumul = [] # cumulative probability from measured delta time samples
        dif_cumul = [] # differences between cumulative functions
        ul_cumul = [] 
        ll_cumul = [] 
        class_width = 1.0 * (upper_limit - lower_limit) / numOfClasses
        txt = "Class value,Cumulative expected,Cumulative measured (minTxPeriod,maxTxPeriod) (" + str(macPromotionMinTxPeriod) + "_" + str(macPromotionMaxTxPeriod) + "), UpperLimit, LowerLimit\n"
        txt += "0,0.00000000,0.00000000,0.00000000,0.00000000\n"
        for i in range(0, numOfClasses):
            exp_cumul += [(1.0 + i) / numOfClasses]
            ul_cumul  += [(1.0 + i) / numOfClasses + threshold]
            ll_cumul  += [(1.0 + i) / numOfClasses - threshold]               
            mea_cumul += [1.0 * sum(1 for x in samples if x < (lower_limit + class_width * (i + 1))) / len(samples)]
            dif_cumul += [abs(mea_cumul[-1] - exp_cumul[-1])]
            txt += str(lower_limit + class_width * (i + 1)) + ","
            txt += "{0:.8f}".format(exp_cumul[i]) + ","
            txt += "{0:.8f}".format(mea_cumul[i]) + ","
            txt += "{0:.8f}".format(ul_cumul[i]) + ","
            txt += "{0:.8f}".format(ll_cumul[i]) + "\n\n"
        d_value = max(dif_cumul)
        
        with open(os.path.join(config.ResultsFolder, str(macPromotionMinTxPeriod) + "_" + str(macPromotionMaxTxPeriod) + "_uniformity_check.csv"), "w") as myfile:
            myfile.write(txt)
        with open(os.path.join(config.ResultsFolder, str(macPromotionMinTxPeriod) + "_" + str(macPromotionMaxTxPeriod) + "_uniformity_check_it.csv"), "w") as myfile:
            myfile.write(txt.replace(",", ";").replace(".", ","))
        if d_value > threshold:
            return False, d_value, threshold
        return True, d_value, threshold
    
    @staticmethod
    
    def IndependenceCheck(samples=[]):
    
        txt = ""
        
        leng = len(samples)
        mean = 1.0 * sum(samples) / leng
        array = []
        n0 = 0  #will store the N of samples below the median      
        for i in range(0, leng):
            if samples[i] < mean:
                array += [0]
            else:
                array += [1]
                n0 += 1
                
        n1 = leng - n0  #N of samples above the median      
        R = 1    # R will store the Measured N of Runs
        for i in range(1, leng):
            if array[i] != array[i - 1]:
                R += 1
         
        E =  1.0 + 2.0*n0*n1/leng #Expected N of Runs
        Var = 2.0*n0*n1*(2*n0*n1 - leng)/((pow(leng,2))*(leng - 1)) # Variance of the expected Runs
        Dev = math.sqrt(Var) #Standard deviation of the expected Runs 
        Z = (R - E) / Dev  # Test statistic
        
        # Critical value = Z at 1-alpha/2 of standard normal distribution with alpha = 0.05
        Critical = 1.96
        
        txt += "\tMedian: " + str(mean) + "\n"
        txt += "\tMeasured Num of Runs:  " + str(R) + "\n"
        txt += "\tExpected Num of Runs:  " + str(E) + "\n"
        txt += "\tVariance of Expected Runs:  " + str(Var) + "\n"
        txt += "\tStd Deviation of Expected Runs:  " + str(Dev) + "\n"
        txt += "\tNum of samples below the median:  " + str(n0) + "\n"
        txt += "\tNum of samples above the median:  " + str(n1) + "\n"
        txt += "\tZ (Test statistic):  " + str(Z) + "\n"
        txt += "\tCritical Value @ confidence 5%:  " + str(Critical) + "\n"
        
        txt += "\tTest fails if abs(Z) > Critical Value\n\n"

        if abs(Z) > Critical:
            txt += "\t\tSamples  ARE NOT independent \n"
            return False, txt
        else:
            txt += "\t\tSamples ARE independent \n"
            return True , txt
        

    
    @staticmethod
    def PPDURandomnessCheck(samples=[]):
        return True
    
    @staticmethod  
    def checkCorrectnessFrameLen(samples, tolerance = 10, fralenExp = TimeConst.FRAME_LENGTH_0_TO_10US):
        for dt in samples:
            if abs(dt-fralenExp) > tolerance:
                return False
        return True
    
    @staticmethod  
    def checkdeltaTime(firstime, secondtime, checktime):
        if secondtime-firstime < checktime:
            return True
        return False
    
    @staticmethod  
    def checkSarSample(sampleslen, samplesval):
        # check len of the segmented data
        first = sampleslen[0]
        if sampleslen[len(sampleslen)-1] > first:
            return False 
        newsamples = sampleslen[0:len(sampleslen)-2]
        for value in newsamples:
            if first != value:
                return False 
        # check SAR.TYPE value
        if (samplesval[0] != 0) or ((samplesval[len(samplesval)-1]) != 2):
            return False
        newsamples = samplesval[1:len(samplesval)-2]
        for value in newsamples:
            if value != 1:
                return False    
        return True
        
            
class DottedStopwatch:
    
    def __init__(self):
        self.stopReq = Event()
        
        self.stopReq.clear()
            
    def __del__(self):
        del self.stopReq
            
    def __tick1Second__(self):
        while True:
            sys.stdout.write(".")
            sys.stdout.flush()
            if self.stopReq.wait(1):
                sys.stdout.write("\n")
                return

    def start(self):
        self.tick1Second_thread = Thread(target=self.__tick1Second__)
        self.tick1Second_thread.start()
        
    def stop(self):
        self.stopReq.set()
        self.tick1Second_thread.join()
        del self.tick1Second_thread        
                
                
# Class implementing simple wait operation along with dots on-screen printout
class WaitAndShow:
    
    @staticmethod
    def wait(timeout):
        stopwatch = DottedStopwatch()
        stopwatch.start()
        
        time.sleep(timeout)
            
        stopwatch.stop()
        del stopwatch


# Packet instances passed to PacketFilter shall be dictionaries with following fields:
# - packet:  the Packet instance
# - level:   rx level
# - scheme:  modulation and frame type, values from Modulations class
# - phyTime: 20-bits rx time
# - sysTime: PC rx indication get time (datetime instance)
# - snr:     rx SNR
# - rxQ:     receive quality
#
# IMPORTANT:
# Notice that by default filter does not collects packets with wrong HCS/CRC,
# unless CRC filter is disabled (by calling acceptWrongCrc() - but user can always
# filter away those packets by placing a check on HCS/CRC into
# checking condition.
#
# [TODO]
# - There's a level of confusion on how filters handle packets. That is to say,
#   the same word 'packet' is used for both the packet dictionary and the packet
#   member of that dictionary. Following changes may help:
#   - collectPacketDict() method to take the single piece of information and to build
#     the dictionary (in that way we're sure that filter lists have correct data
#     structures.
#   - Change, wherever it occurs, the dictionary name 'packet' into something
#     different (e.g. packetDict), to make it clear the difference from the packet.
#     (mostly done)
class PacketFilter:

    # Checking function checkF is expected to:
    # - Have at least one input parameter, the packet to be verified
    # - Return True or False if the packet meet the condition or not
    def __init__(self, checkF = None, *argv):
        self.collectedPktDicts = []

        self.enabled = True
        self.checkF = checkF
        self.checkFArgv = argv

        self.rejectWrongCrc = True

        self.queue = deque()

        self.monitorEvent = Event()
        self.monitorEvent.clear()

    def __del__(self):
        self.monitorEvent.set()
        del self.monitorEvent

    # Intended to be executed by the main thread (test code)
    # Enable packets collection (in case filtering condition is met)
    def enable(self):
        self.enabled = True
        self.monitorEvent.clear()

    # Intended to be executed by the main thread (test code)
    # Disable packets collection
    def disable(self):
        self.enabled = False

    # Return True if the filtering condition is enabled (hence packets that
    # fulfill the condition are collected)
    def isEnabled(self):
        return self.enabled

    # Intended to be executed by the main thread (test code)
    def rejectWrongCrc(self):
        self.rejectWrongCrc = True

    # Intended to be executed by the main thread (test code)
    def acceptWrongCrc(self):
        self.rejectWrongCrc = False

    # Return True if packets with wrong CRC/HCS are not collected
    # (no matter if they met the filtering condition)
    def isRejectingWrongCrc(self):
        return self.rejectWrongCrc

    # Return the list of collected packets so far
    def getList(self):
        try:
            while True:
                elem = self.queue.popleft()
                self.collectedPktDicts += [elem]
        except (IndexError):
            pass

        self.monitorEvent.clear()

        return self.collectedPktDicts

    # Clear the collected packets list, created in the previous getList() call
    def clearList(self):
        self.getList()  # clear the queue
        self.collectedPktDicts = []
        return


    # Shall return True if the packet fulfills the implemented test condition
    def checkPacket(self, packetDict):
        return self.checkF is not None and self.checkF(packetDict, *self.checkFArgv)

    # Intended to be executed by the sniffer thread
    # Returns True if the filter is enabled and packet met the condition nd
    # CRC is ok (if rejection in case of wrong CRC is enabled)
    def collectPacketDict(self, packetDict):
        if self.enabled:
            if self.rejectWrongCrc and packetDict['packet'].rxError():
                return False
            if self.checkPacket(packetDict):
                self.queue.append(packetDict)
                self.monitorEvent.set()
                return True

        return False

    # Intended to be executed by the main thread (test code)
    # The procedure waits until the packet testing condition is met or
    # the timeout period (if provided) is elapsed.
    # It returns True if the testing condition has been met. Afterwards,
    # this testing condition met state is always reset.
    # Setting 'silent' to False makes no dot be printed out
    def wait(self, timeout = None, silent = True):
        if not self.enabled:
            return False

        if timeout is None:
            timeout = 15 * 60  # 15m waiting time

        silent = (silent == True)

        if not silent:
            stopwatch = DottedStopwatch()
            stopwatch.start()

        res = self.monitorEvent.wait(timeout)
        self.monitorEvent.clear()

        if not silent:
            stopwatch.stop()
            del stopwatch

        return res

    # Intended to be executed by the main thread (test code)
    # Check if the condition has been bet since last call and returns True if so.
    # The condition met state is always reset after having been read.
    def check(self):
        retVal = self.monitorEvent.isSet()
        self.monitorEvent.clear()
        return retVal



# Packet Monitor keeping track of frame and regions boundaries (by picking BCN from BN) 
# and telling if  packets completely fall into CFP or SCP; placement of PNPDUs is not checked.  
class FramingMonitor(PacketFilter):
    
    def __init__(self):
        PacketFilter.__init__(self)
        
        # List of frame descriptors as they're received through beacons
        # Only when at least one frame characteristic is changed a new entry is added here.
        # Dictionariy keys:
        # - seq: beacons's sequence number
        # - len: frame's length in symbols
        # - cfpLen: CFP's length in symbols
        # - startTime: frame's start time (datetime instance)
        # - startTime10us: frame's start time (in 10us-timer value)
        # - bcnRxTime: beacon's rx start time (datetime instance)
        # - bcnRxTime10us : beacon's rx start time (in 10us-timer value)
        self.frameDesc = []
        
    def isEnabled(self):
        return self.frameDesc != [] and PacketFilter.isEnabled(self)
    
    def checkPacket(self, packetDict):
        bpdu = packetDict['packet'].bpdu()
        if (bpdu is not None and bpdu.sid == Addresses.LSID_BASENODE and 
            (self.frameDesc == [] or bpdu.fralen != self.frameDesc[-1]['len'] or bpdu.cfp != self.frameDesc[-1]['cfpLen'])):
            delta = timedelta(
                seconds = (packetDict['packet'].calcTxDuration(packetDict['scheme']) + bpdu.pos * TimeConst.SYMBOL_TO_10US) / 100000.0)
            self.frameDesc += [{'seq' : bpdu.seq,
                                'len' : (bpdu.fralen + 1) * TimeConst.FRAME_LENGTH_0,
                                'cfpLen' : bpdu.cfp,
                                'startTime' : packetDict['sysTime'] - delta,
                                'startTime10us' : TimeConst.incr10us(packetDict['phyTime'], -(delta.total_seconds() * 100000.0)),
                                'bcnRxTime' : packetDict['sysTime'],
                                'bcnRxTime10us' : packetDict['phyTime']}]
            
        # Always return False (no need to collect beacons here)
        return False
    
    def wait(self):
        return True
        
    # Internal procedure to get the most recent frame descriptor earlier than the packet
    def __getFraDescId(self, packetDict):
        pktTxStartT = (packetDict['sysTime'] - 
                       timedelta(seconds = packetDict['packet'].calcTxDuration(packetDict['scheme']) / 100000.0)) 
        for i in range(-1, -len(self.frameDesc)-1, -1):
            if self.frameDesc[i]['startTime'] <= pktTxStartT:
                return i

        return len(self.frameDesc)
    
    # Procedure to compute frame times of the frame which contains the passed packet.
    # Info returned are:
    # - frame start real (datetime)
    # - frame start 10us
    # - SCP start real (datetime)
    # - SCP start 10us
    # - frame end real (datetime)
    # - frame start 10us
    def getCurrentFrameRegions(self, packetDict):
        # Looking for the most recent frame descriptor earlier that the packet
        try:
            fraDesc = self.frameDesc[self.__getFraDescId(packetDict)]
        except IndexError:
            return [None]*6
        
        pktTxStartT = (packetDict['sysTime'] - 
                       timedelta(seconds = packetDict['packet'].calcTxDuration(packetDict['scheme']) / 100000.0))         
        elapsedFrames = int(math.floor((pktTxStartT - fraDesc['startTime']).total_seconds() / 
                                       (fraDesc['len'] * TimeConst.SYMBOL_TO_SEC)))
        
        times = []
        for i in range (0,3):
            if i == 0:
                # Frame start (w.r.t. latest known frame start time)
                delta10usT = elapsedFrames * fraDesc['len'] * TimeConst.SYMBOL_TO_10US
            elif i == 1:
                # SCP start (w.r.t. latest known frame start time)
                delta10usT += fraDesc['cfpLen'] * TimeConst.SYMBOL_TO_10US
            else:
                # Frame end time (w.r.t. latest known frame start time)
                delta10usT += (fraDesc['len'] - fraDesc['cfpLen']) * TimeConst.SYMBOL_TO_10US

            times += [fraDesc['startTime'] + timedelta(seconds = delta10usT / 100000.0),
                      TimeConst.incr10us(fraDesc['startTime10us'], delta10usT)]
            
        return times
        
    def isInCfp(self, packetDict):
        if not self.isEnabled():
            return False
        
        # PNPDUs placement is not verified
        if packetDict['packet'].isPnpdu():
            return False
        
        try:
            desc = self.frameDesc[self.__getFraDescId(packetDict)]
        except IndexError:
            return False
        
        pktEndT = TimeConst.time10usDiff(desc['startTime10us'], desc['startTime'], packetDict['phyTime'], packetDict['sysTime'])
        pktStartT = pktEndT - packetDict['packet'].calcTxDuration(packetDict['scheme'])
        
        elapsedFrames = int(math.floor(pktStartT / (desc['len'] * TimeConst.SYMBOL_TO_10US)))
        cfpStartT = elapsedFrames * desc['len'] * TimeConst.SYMBOL_TO_10US
        cfpEndT = cfpStartT + desc['cfpLen'] * TimeConst.SYMBOL_TO_10US
        
        return pktEndT < cfpEndT
    
    def isInScp(self, packetDict):
        if not self.isEnabled():
            return False
        
        # PNPDUs placement is not verified
        if packetDict['packet'].isPnpdu():
            return False

        try:
            desc = self.frameDesc[self.__getFraDescId(packetDict)]
        except IndexError:
            return False
        
        pktEndT = TimeConst.time10usDiff(desc['startTime10us'], desc['startTime'], packetDict['phyTime'], packetDict['sysTime'])
        pktStartT = pktEndT - packetDict['packet'].calcTxDuration(packetDict['scheme'])
        
        elapsedFrames = int(math.floor(pktStartT / (desc['len'] * TimeConst.SYMBOL_TO_10US)))
        frameStartT = elapsedFrames * desc['len'] * TimeConst.SYMBOL_TO_10US
        scpStartT = frameStartT + desc['cfpLen'] * TimeConst.SYMBOL_TO_10US
        scpEndT = frameStartT + desc['len'] * TimeConst.SYMBOL_TO_10US
        
        return pktStartT >= scpStartT and pktEndT < scpEndT
    
    
# Packet Monitor collecting packets being transmitted within CFP
class CfpPacketFilter(PacketFilter):
    
    # framingMonitor shall be instance of FramingMonitor class
    def __init__(self, framingMonitor):
        PacketFilter.__init__(self)
        
        self.framingMonitor = framingMonitor
    
    def checkPacket(self, packetDict):
        return self.framingMonitor.isInCfp(packetDict)


# Packet Monitor collecting packets being transmitted within SCP
class ScpPacketFilter(PacketFilter):
    
    # framingMonitor shall be instance of FramingMonitor class
    def __init__(self, framingMonitor):
        PacketFilter.__init__(self)
        
        self.framingMonitor = framingMonitor
    
    def checkPacket(self, packetDict):
        return self.framingMonitor.isInScp(packetDict)
