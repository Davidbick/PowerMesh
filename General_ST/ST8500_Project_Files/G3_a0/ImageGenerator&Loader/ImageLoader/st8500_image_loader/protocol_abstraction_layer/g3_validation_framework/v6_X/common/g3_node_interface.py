#!/usr/bin/python3
try:
    import sys
    import os
    import platform
    import threading
    import time
    import statistics
    import queue
    import logging
    import pickle
    import xlsxwriter as xlsx
    import libscrc
    import random

    current_platform = platform.system()

    if "Linux" == current_platform:
        sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + "/common")
        coord_port_name = "/dev/ttyUSB1"
    elif "Windows" == current_platform:
        sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + "\\common")
        coord_port_name = "COM19"
    else:
        print("Error on supported platform")
        raise ImportError

    import g3_network
    from g3_network import *
    import g3_path_info as g3_path
    import g3_common as g3_cmn

except:
    print("Error importing libraries")
    raise ImportError


def filename_prefix_time():
    return "g3_" + time.strftime("%Y%m%d_%H%M") + "_"

logger = logging.getLogger(__name__)
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)
filename = filename_prefix_time() + "log.log"
handler = logging.FileHandler(filename)
handler.setFormatter(logging.Formatter('[%(asctime)s -   NONE - %(funcName)25s()] - %(message)s'))
logger.addHandler(handler)


class g3_coord_ping_stats(object):
    """The g3_coord_ping_stats contains the statistical information
       on the pings done by the coordinator """

    class ping_stat(object):
        def __init__(self):
            self.num_pings_ok = 0
            self.num_pings = 1
            self.rtt = []
            self.ping_start_time = time.time()
            self.ping_stop_time = time.time()

    def __init__(self, file_name="g3_coord_stats_"):
        self._dict = {}
        file_name_xlsx = filename_prefix_time()+file_name+".xlsx"
        self.workbook = xlsx.Workbook(file_name_xlsx)
        self.file_name_pkl = filename_prefix_time()+file_name+".pkl"

    def __str__(self):
        s = "lengths[byte] = "
        keys_len = self._dict.keys()
        for k in sorted(keys_len):
            s += " {:4d}".format(k)

    def calc_iteration_statistic(self, value_iter):
        ping_total_time = round(value_iter.ping_stop_time - value_iter.ping_start_time, 1)
        if (value_iter.num_pings != 0):
            ping_success_rate = round(value_iter.num_pings_ok*100/value_iter.num_pings, 3)
        else:
            ping_success_rate = 0
        if (len(value_iter.rtt) > 1): #at least 2 samples
            mean_rtt = statistics.mean(value_iter.rtt)
            var_rtt = statistics.stdev(value_iter.rtt)
        else:
            mean_rtt = 0
            var_rtt = 0
        return [value_iter.num_pings_ok, value_iter.num_pings, ping_success_rate, mean_rtt, var_rtt, ping_total_time]

    def print_statistics_iteration(self):
        s = "byte\t\t|iter\t\t|ping ok\t|on\t\t|succ rate [%]\t|mean rtt\t|var rtt\t|total time[s]\t|\n"
        keys_len = self._dict.keys()
        for k_len in sorted(keys_len):
            v_len = self._dict[k_len]
            keys_iter = v_len.keys()
            for k_iter in sorted(keys_iter):
                v_iter = v_len[k_iter]
                ret = self.calc_iteration_statistic(v_iter)
                s += "{:16d}|".format(k_len) + \
                    "{:15d}|".format(k_iter) + \
                    "{:15n}|".format(ret[0]) + \
                    "{:15n}|".format(ret[1]) + \
                    "{:14n}%|".format(ret[2]) + \
                    "{0:15n}|".format(ret[3]) + \
                    "{0:15n}|".format(ret[4]) + \
                    "{:15n}|".format(ret[5]) + '\n'
        print(s)
        return s

    def calc_aggregate_statistic(self, value_len_dic):
        keys_iter = value_len_dic.keys()

        aggregate_num_pings = 0
        aggregate_num_pings_ok = 0
        aggregate_ping_start = time.time()
        aggregate_ping_stop = 0
        rtt = []

        #aggregate all the iteration statistics
        for k_iter in sorted(keys_iter):
            v_iter = value_len_dic[k_iter]

            aggregate_num_pings += v_iter.num_pings
            aggregate_num_pings_ok += v_iter.num_pings_ok

            if (v_iter.ping_start_time < aggregate_ping_start):
                aggregate_ping_start = v_iter.ping_start_time
            if (v_iter.ping_stop_time > aggregate_ping_stop):
                aggregate_ping_stop = v_iter.ping_stop_time

            rtt += v_iter.rtt

        ping_total_time = round(aggregate_ping_stop - aggregate_ping_start, 1)
        if (aggregate_num_pings != 0):
            ping_success_rate = round(aggregate_num_pings_ok*100/aggregate_num_pings, 3)
        else:
            ping_success_rate = 0
        if (len(rtt) > 1): #at least 2 samples
            mean_rtt = statistics.mean(rtt)
            var_rtt = statistics.stdev(rtt)
        else:
            mean_rtt = 0
            var_rtt = 0

        return [aggregate_num_pings_ok, aggregate_num_pings, ping_success_rate, mean_rtt, var_rtt, ping_total_time]

    def print_aggregate_statistics(self):
        s = "byte\t\t|ping ok\t|on\t\t|succ rate [%]\t|mean rtt\t|var rtt\t|total time[s]\t|\n"
        keys_len = self._dict.keys()
        for k_len in sorted(keys_len):
            v_len = self._dict[k_len]
            ret = self.calc_aggregate_statistic(v_len)
            s += "{:16d}|".format(k_len) + \
                "{:15n}|".format(ret[0]) + \
                "{:15n}|".format(ret[1]) + \
                "{:14n}%|".format(ret[2]) + \
                "{0:15n}|".format(ret[3]) + \
                "{0:15n}|".format(ret[4]) + \
                "{:15n}|".format(ret[5]) + '\n'
        print(s)
        return s

    def save_stats_on_xlsx_file(self):
        worksheet = self.workbook.add_worksheet('aggregate')
        worksheet.write_row(0, 0, ["Payload Length", "Pings reply", "Pings request", "Success Rate (%)", "Mean RTT ()", "Var RTT", "Total Time"])
        row = 1
        keys_len = self._dict.keys()
        for k_len in sorted(keys_len):
            v_len = self._dict[k_len]

            ret = self.calc_aggregate_statistic(v_len)
            worksheet.write_row(row, 0, [k_len] + ret)
            row += 1

        worksheet = self.workbook.add_worksheet('iteration')
        worksheet.write_row(0, 0, ["Payload Length", "Iteration", "Pings reply", "Pings request", "Success Rate (%)", "Mean RTT ()", "Var RTT", "Total Time"])
        row = 1
        #s = self.STR_HEADER
        keys_len = self._dict.keys()
        for k_len in sorted(keys_len):
            v_len = self._dict[k_len]
            keys_iter = v_len.keys()
            for k_iter in sorted(keys_iter):
                v_iter = v_len[k_iter]
                ret = self.calc_iteration_statistic(v_iter)
                worksheet.write_row(row, 0, [k_len, k_iter] + ret)
                row += 1

        self.workbook.close()

    def serialize_stats_on_pkl_file(self):
        with open(self.file_name_pkl, 'wb') as pickle_file:
            pickle.dump(self._dict, pickle_file)

    def oprn_stats_from_pkl_file(self, file_name):
        self._dict.clear()
        with open(file_name, 'rb') as pickle_file:
            self._dict = pickle.load(pickle_file)

    def reset_stats(self):
        self._dict.clear()

    def increase_ping(self, iteration, length):
        if length in self._dict:
            if iteration in self._dict[length]:
                self._dict[length][iteration].num_pings += 1
                self._dict[length][iteration].ping_stop_time = time.time()
                return
            self._dict[length][iteration] = self.ping_stat()
            return
        self._dict[length] = {iteration:self.ping_stat()}
        return

    def increase_ping_ok(self, iteration, length, rtt):
        if length in self._dict:
            if iteration in self._dict[length]:
                self._dict[length][iteration].num_pings_ok += 1
                self._dict[length][iteration].ping_stop_time = time.time()
                self._dict[length][iteration].rtt.append(rtt)
                return
        #ops... here is an error, how a ping ok can happen if an increase ping was not done before

    def get_success_rate(self, length):
        if length in self._dict:
            ret = self.calc_aggregate_statistic(self._dict[length])
            return ret[2]
        else:
            return 0

    def get_complete_stats(self, length):
        if length in self._dict:
            ret = self.calc_aggregate_statistic(self._dict[length])
            return ret
        else:
            return 0

    def get_iterations_stats(self, length):
        # Prepare Return Dictionary
        ret = {'Transmitted Ping msg':[],
                'Received Ping msg': [],
                'Success rate': [],
                'RTT average': []}

        if length in self._dict:
            full_stats = self._dict[length]
            print("Full statistics: " + repr(full_stats))
            for iter_num in range(len(full_stats)):
                iter_stat = full_stats[iter_num]
                print("Iter stats: " + repr(iter_stat))
                # [num_pings_ok, num_pings, ping_success_rate, mean_rtt, var_rtt, ping_total_time] = self.calc_iteration_statistic(iter_stat)
                ret['Transmitted Ping msg'].append(iter_stat.num_pings)
                ret['Received Ping msg'].append(iter_stat.num_pings_ok)
                ret['Success rate'].append(float(iter_stat.num_pings_ok)/iter_stat.num_pings_ok*100.0)
                ret['RTT average'].append(sum(iter_stat.rtt)/len(iter_stat.rtt))
            return ret
        else:
            return 0

class g3_remote_dev_stats(object):
    """The g3_stats class contains the statistical information of the remote G3 device """

    STR_HEADER = " byte\ticmp ok\ticmp tx\tsucc rate [%]\tmean rtt[ms]\tvar rtt\n"

    class icmp_stat(object):
        def __init__(self):
            self.icmp_echo_req = 1
            self.icmp_echo_rep = 0
            self.icmp_rtt = []

        def __str__(self):
            if(len(self.icmp_rtt) > 2):
                return " {0:8d}".format(self.icmp_echo_req) + \
                    " {0:8d}".format(self.icmp_echo_rep) + \
                    " {0:8n}".format(round(100*self.icmp_echo_rep/self.icmp_echo_req, 1)) + \
                    " {0:8n}".format(statistics.mean(self.icmp_rtt)) + " {0:8n}".format(statistics.stdev(self.icmp_rtt))
            else:
                return " {0:8d}".format(self.icmp_echo_req) + \
                    " {0:8d}".format(self.icmp_echo_rep) + \
                    " {0:8n}".format(round(100*self.icmp_echo_rep/self.icmp_echo_req, 1)) + \
                    "\t0" +"\t0"

    def __init__(self):
        self.reg_time = 0
        self._dict = {}

    def __str__(self):
        s = "reg time [s]: " + str(self.str_reg_time()) + "\n"
        s += self.STR_HEADER
        keys = self._dict.keys()
        for k in sorted(keys):
            v = self._dict[k]
            s += "{:8d}".format(k) + str(v) + "\n"
        return s

    def str_reg_time(self):
        return "{0:8n}".format(round(self.reg_time, 1))

    def reset_ping(self):
        self._dict = {}

    def reset_reg(self):
        self.reg_time = 0

    def reset(self):
        self.reset_reg()
        self.reset_ping()

    def set_reg_time(self, delta_time):
        self.reg_time = delta_time

    def update_ping_ok(self, length, rtt):
        if length in self._dict:
            self._dict[length].icmp_echo_rep += 1
            self._dict[length].icmp_rtt.append(rtt) 
            return
        #ops, why we are here if the ping was not transmitted
        return

    def update_ping_tx(self, length):
        if length in self._dict:
            self._dict[length].icmp_echo_req += 1
            return
        self._dict[length] = self.icmp_stat()

class g3_remote_device(object):
    """The g3_remote_device class represents a single G3 device in the network from the G3 coordinator point of view
       It contains all the information related to the remote device ."""

    def __init__(self, extended_addr=[0,0,0,0,0,0,0,0], pan_id=0xFFFF, short_addr=0xFFFF, is_connected=False, position=0, platform=g3_hi_platform.UNKNOWN):
        self.extended_addr = extended_addr
        self.position = position
        self.platform = platform
        self.pan_id = pan_id
        self.short_addr = short_addr
        self.ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(pan_id, short_addr)
        self.is_connected = False
        self.stats = g3_remote_dev_stats()

    def __str__(self):
        return "Extended " + str(self.extended_addr) + ", Pos #{0:02d}".format(self.position) + ", Platform = " + str(self.platform) + ", Pan Id = 0x{0:04X}".format(self.pan_id) + ", Short Addr = 0x{0:04X}".format(self.short_addr) + ", Ip " + str(self.ipv6_link_local_addr)

    def __lt__(self, other):
        return self.short_addr < other.short_addr
    def __le__(self, other):
        return self.short_addr <= other.short_addr
    def __eq__(self, other):
        return self.short_addr == other.short_addr
    def __ne__(self, other):
        return self.short_addr != other.short_addr        
    def __gt__(self, other):
        return self.short_addr > other.short_addr
    def __ge__(self, other):
        return self.short_addr >= other.short_addr

    def set_connected(self):
        self.is_connected = True
    
    def set_disconnected(self):
        self.is_connected = False

    def print_stats(self):
        print(str(self) + " " + str(self.stats))

class g3_node(object):
    """The g3_node class represents a generic G3 node. This class cannot be used as is but it has to derived into g3_coord or g3_device"""

    def __init__(self, position=0):
        self.extended_addr = g3_hi_ext_address()
        self.position = position
        self.platform = g3_hi_platform.UNKNOWN
        self.pan_id = 0xFFFF
        self.short_addr = 0xFFFF
        self.ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, self.short_addr)
        self.rte_version = 0
        self.lib_version = 0
        self.fwkversion_patch = 0
        self.fwkversion_minor = 0
        self.fwkversion_major = 0
        self.devtype = g3_hi_device_type.DEVICE
        self.bandplan = g3_hi_band_plan.CEN_A
        self.serial = None
        self.g3_net = None
        self.boot_thread_stop_ev = threading.Event()
        self.ping_thread_stop_ev = threading.Event()
        self.asynch_events_thread_stop_ev = threading.Event()
        self.usart_num = None
        self.rf_detected = None
        self.plc_mode = None
        self.led_conf = None

    def __str__(self):
        return  "Fwk ver " + '{0:02n}'.format(self.fwkversion_major) + '.{0:02n}'.format(self.fwkversion_minor) + '.{0:02n}'.format(self.fwkversion_patch) + \
             " - lib ver 0x{0:08X}".format(self.lib_version) + " - RTE ver 0x{0:08X}".format(self.rte_version) +\
             " - MAC " + str(self.extended_addr) + " - Pos #{0:02d}".format(self.position) + " " + str(self.platform) + " - PAN ID 0x{0:04X}".format(self.pan_id) + " - ShortAddr 0x{0:04X}".format(self.short_addr) + " - IPv6 LL {}".format(self.ipv6_link_local_addr)

    G3ICMP_ECHO_CONF_QUEUE_IDX = 1
    G3UDP_APP_CONF_QUEUE_IDX = 1
    G3ICMP_ECHO_IND_QUEUE_IDX = 4
    G3UDP_APP_IND_QUEUE_IDX = 4

    RTABLE_MAX_SIZE = 1200

    def q_selector(self, msg):
        msg_vec = msg
        cmd_id = msg_vec[2]
        if (g3_hi_cmd_ids.SFLASH_CONF == cmd_id) or (g3_hi_cmd_ids.NVM_CONF == cmd_id):
            return 5
        if (g3_hi_cmd_ids.G3ICMP_ECHOREP_IND == cmd_id) or (g3_hi_cmd_ids.G3ICMP_ECHOREQ_IND == cmd_id) or (g3_hi_cmd_ids.G3UDP_APP_IND == cmd_id):
            return 4
        if  (g3_hi_cmd_ids.HWRESET_CONF == cmd_id) or (g3_hi_cmd_ids.G3LIB_EVENT_NOTIF_IND == cmd_id) or (g3_hi_cmd_ids.G3MAC_COMMSTATUS_IND == cmd_id) or (g3_hi_cmd_ids.G3ADP_NETSTATUS_IND == cmd_id) or (g3_hi_cmd_ids.ERROR_IND == cmd_id):
            return 3
        if (g3_hi_cmd_ids.G3BOOT_DEV_START_CONF == cmd_id) or (g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_CONF == cmd_id) or (g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_IND == cmd_id) or(g3_hi_cmd_ids.G3BOOT_DEV_PANSORT_IND == cmd_id) or (g3_hi_cmd_ids.G3BOOT_DEV_PANSORT_CONF == cmd_id):
            return 2
        if (g3_hi_cmd_ids.G3BOOT_SRV_SETPSK_CONF == cmd_id) or (g3_hi_cmd_ids.G3BOOT_SRV_GETPSK_IND == cmd_id) or (g3_hi_cmd_ids.G3BOOT_SRV_JOIN_IND == cmd_id) or (g3_hi_cmd_ids.G3BOOT_SRV_LEAVE_IND == cmd_id) or (g3_hi_cmd_ids.G3BOOT_SRV_START_CONF == cmd_id) or (g3_hi_cmd_ids.G3BOOT_SRV_STOP_CONF == cmd_id):
            return 2
        if (g3_hi_cmd_ids.G3ICMP_ECHO_CONF == cmd_id) or (g3_hi_cmd_ids.G3UDP_APP_CONF == cmd_id):
            return 1
        else:
            #all the synchronous commands goes here
            return 0

    def open(self, port, baudrate=115200):
        self.serial = port
        extra = {'port':port}
        handler.setFormatter(logging.Formatter('[%(asctime)s - %(port)6s - %(funcName)25s()] - %(message)s'))
        self.logger = logging.LoggerAdapter(logger, extra)        
        if "Linux" == current_platform:
            #convert port to something like _port = b'/dev/ttyUSB0'
            _port = port
        elif "Windows" == current_platform:
            #convert port to something like _port = b'\\\\.\\COM7'
            _port = '\\\\.\\'+port
        self.g3_net = network_info_ex(_port, baudrate, 6, self.q_selector)
        self.g3_net.open()
        time.sleep(0.25)
        self.get_info()
        self.start_asynch_evt_handler()
        self.start_boot_handler()
  
    def close(self):
        self.stop_boot_handler()
        self.stop_ping_handler()
        self.stop_asynch_evt_handler()
        self.g3_net.close()

    def flush_rx_queue(self):
        self.g3_net.empty_queues()

    def set_baudrate(self, baudrate):
        baudrate_config = g3_hi_msg_hwconfig_baudrateconfig_req(baudrate)
        req = g3_hi_msg_hwconfig_req(g3_hi_hwconfig_type.BAUDRATE_CONFIG, baudrate_config)
        g3_hi_msg_send_message(self.g3_net, req)
        msg = g3_hi_msg_receive_message(self.g3_net)
        if g3_hi_cmd_ids.HWCONFIG_CONF != msg.header.cmd_id:
            raise ValueError
        if msg.status != 0x00:
            raise ValueError
        if msg.config_type != g3_hi_hwconfig_type.BAUDRATE_CONFIG:
            raise ValueError
        if msg.config_payload.baud_rate != baudrate:
            raise ValueError

    def get_attr(self, attr_id, attr_idx = 0):
        return g3_cmn.g3_hi_get_attr(self.g3_net, attr_id, attr_idx)        

    def set_attr(self, attr_id, attr_idx, attr_len, attr_value):
        return g3_cmn.g3_hi_set_attr(self.g3_net, attr_id, attr_idx, attr_len, attr_value)

    def get_rtable(self):
        r_table_list = []
        for i in range(0, self.RTABLE_MAX_SIZE):
            r_table = g3_cmn.g3_hi_get_attr(self.g3_net, g3_hi_attrib_id.ADP_ROUTING_TABLE, i) 
            if r_table != False:
                if r_table.dst_addr != 0xFFFF:
                    r_table_list.append(r_table)
        return r_table_list

    def set_mode(self, mode=g3_hi_mode.MODE_IPV6_BOOT):
        # HI-MODE-SET
        req = g3_hi_msg_mode_set_req(mode)
        g3_hi_msg_send_message(self.g3_net, req)
        msg = g3_hi_msg_receive_message(self.g3_net)
        if g3_hi_cmd_ids.MODESET_CONF != msg.header.cmd_id:
            return False
        else:
            return True

    def sw_reset(self, band_plan = g3_hi_band_plan.CEN_A, short_addr = None, pan_id = None, device_type = g3_hi_device_type.DEVICE, mode=g3_hi_mode.MODE_IPV6_BOOT):
        # G3LIB-SW-RESET
        req = g3_hi_msg_swreset_req(band_plan, device_type, mode)
        g3_hi_msg_send_message(self.g3_net, req)
        msg = g3_hi_msg_receive_message(self.g3_net)
        if g3_hi_cmd_ids.G3LIB_SWRESET_CONF != msg.header.cmd_id:
            raise ValueError
        if msg.status != 0x00:
            raise ValueError
        
        # Set ShortAddress
        if(short_addr != None):
            req = g3_hi_msg_get_g3lib_set_attrib_req(g3_hi_attrib_id.MAC_SHORTADDRESS_ID, 0, 2, c_ushort(short_addr))
            g3_hi_msg_send_message(self.g3_net, req)
            msg = g3_hi_msg_receive_message(self.g3_net)
            if g3_hi_cmd_ids.G3LIB_SET_CONF != msg.header.cmd_id:
                raise ValueError
            if msg.state != 0x00:
                raise ValueError

        # Set PANId
        if(pan_id != None):
            req = g3_hi_msg_get_g3lib_set_attrib_req(g3_hi_attrib_id.MAC_PANID_ID, 0, 2, c_ushort(self.pan_id))
            g3_hi_msg_send_message(self.g3_net, req)
            msg = g3_hi_msg_receive_message(self.g3_net)
            if g3_hi_cmd_ids.G3LIB_SET_CONF != msg.header.cmd_id:
                raise ValueError
            if msg.state != 0x00:
                raise ValueError


    def get_info(self):
        req = g3_hi_msg_dbgtool_req(g3_hi_dbgtool_ctrl.HI_DBG_INFO, 0)
        g3_hi_msg_send_message(self.g3_net, req)
        dbginfo_msg = g3_hi_msg_receive_message(self.g3_net)
        if dbginfo_msg != None:
            #print(dbginfo_msg)
            self.extended_addr = dbginfo_msg.mac_addr
            self.fwkversion_patch = dbginfo_msg.fwkversion_patch
            self.fwkversion_minor = dbginfo_msg.fwkversion_minor
            self.fwkversion_major = dbginfo_msg.fwkversion_major
            self.devtype = g3_hi_device_type(dbginfo_msg.devtype)
            self.bandplan = g3_hi_band_plan(dbginfo_msg.bandplan)
            self.rte_version = dbginfo_msg.rte_version
            self.lib_version = dbginfo_msg.lib_version
            self.platform = g3_hi_platform(dbginfo_msg.platform)
            self.pan_id = self.get_attr(g3_hi_attrib_id.MAC_PANID_ID)
            self.short_addr = self.get_attr(g3_hi_attrib_id.MAC_SHORTADDRESS_ID)
            self.usart_num = dbginfo_msg.usart_num
            self.rf_detected = dbginfo_msg.rf_detected
            self.plc_mode = dbginfo_msg.plc_mode
            self.led_conf = dbginfo_msg.led_conf
        if (self.pan_id == None) or (self.short_addr == None):
            self.pan_id = 0xFFFF
            self.short_addr = 0xFFFF
        self.ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, self.short_addr)
        self.logger.info(str(self.serial) + " Platform: " + str(self.platform))

    def boot_evt_handler(self, arg1, stop_event):
        self.logger.info("Generic unspecialized boot handler")

    def start_boot_handler(self):
        self.boot_thread_stop_ev.clear()
        self.boot_thread = threading.Thread(target=self.boot_evt_handler, args=("boot", self.boot_thread_stop_ev))
        self.boot_thread.name = "boot_thread"
        self.boot_thread.start()

    def stop_boot_handler(self):
        self.logger.info("Stop boot")
        self.boot_thread_stop_ev.set()

    def asynch_evt_handler(self, arg1, stop_event):
        self.logger.info("Start asynch events handler")
        while not stop_event.is_set():
            msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[3], to = 1)
            if msg != None:
                self.logger.info("Asynch Event - " + str(msg))
            #else: #wait is already in the receive message
            #    time.sleep(1)

    def start_asynch_evt_handler(self):
        self.asynch_events_thread_stop_ev.clear()
        self.asynch_events = threading.Thread(target=self.asynch_evt_handler, args=("asynch_events", self.asynch_events_thread_stop_ev))
        self.asynch_events.name = "asynch_events_thread"
        self.asynch_events.start()

    def stop_asynch_evt_handler(self):
        self.logger.info("Stop asynch events handler")
        self.asynch_events_thread_stop_ev.set()

    def ping(self, ipv6_link_local_addr, len, timeout=120):
        ret = True
        rtt = 0
        echo_payload = (c_ubyte * len)()
        for j in range(0, sizeof(echo_payload)):
            echo_payload[j] = c_ubyte(j % 255)
        req = g3_hi_msg_get_g3_icmp_echo_req(ipv6_link_local_addr, 0x5A, sizeof(echo_payload), echo_payload)
        g3_hi_msg_send_message(self.g3_net, req)
        req_time = time.time()
        msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3ICMP_ECHO_CONF_QUEUE_IDX], timeout)
        if (msg != None) and (g3_hi_cmd_ids.G3ICMP_ECHO_CONF == msg.header.cmd_id) and (0x00 == msg.data.status):
            pass
        elif (msg != None) and (g3_hi_cmd_ids.G3ICMP_ECHO_CONF == msg.header.cmd_id) and (0x00 != msg.data.status):
            self.logger.warning("G3 ICMP Echo Confirm: Error Status")
            return [False, 0]  
        else:
            self.logger.warning("No confirm")
            return [False, 0]  

        elapsed_time =  time.time() - req_time
        remaining_time = max(0, timeout - elapsed_time) # Avoid negative numbers
        msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3ICMP_ECHO_IND_QUEUE_IDX], remaining_time)
        if (msg != None) and (g3_hi_cmd_ids.G3ICMP_ECHOREP_IND == msg.header.cmd_id):
            indication_time = time.time()
            rtt = indication_time - req_time
        else:
            self.logger.warning("No indication")
            return [False, 0]

        return [True and ret, rtt]

    def udp_app(self, app_req_id, ipv6_link_local_addr, payload, timeout=120):
        ret = True
        req_time = time.time()
        req = g3_hi_msg_get_g3_udp_app_req(app_req_id, ipv6_link_local_addr, sizeof(payload), payload)
        g3_hi_msg_send_message(self.g3_net, req)
        conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3UDP_APP_CONF_QUEUE_IDX], timeout)
        if conf != None and g3_hi_cmd_ids.G3UDP_APP_CONF == conf.header.cmd_id and 0x00 == conf.data.status:
            pass
        elif conf != None and g3_hi_cmd_ids.G3UDP_APP_CONF == conf.header.cmd_id and 0x00 != conf.data.status:
            self.logger.warning("G3 UDP App Confirm: Error Status")
            return [False, None] 
        else:
            self.logger.warning("No confirm")
            return [False, None] 

        elapsed_time =  time.time() - req_time
        remaining_time = max(0, timeout - elapsed_time) # Avoid negative numbers

        ind = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3UDP_APP_IND_QUEUE_IDX], remaining_time)
        if ind != None and g3_hi_cmd_ids.G3UDP_APP_IND == ind.header.cmd_id:
            pass
        else:
            self.logger.warning("No indication")
            return [False, None]

        return [True and ret, ind]

    def udp_app_rhi(self, ipv6_link_local_addr, hi_msg, ind_expected=True, retries=2, timeout=120):
        ret = False
        rhi_resp = None
        while(retries > 0):
            req_time = time.time()
            req = g3_hi_msg_get_g3_udp_app_req(g3_hi_udp_app_type.UDP_RSP_APP_RHI_REQ_ID, ipv6_link_local_addr, sizeof(hi_msg), hi_msg)
            g3_hi_msg_send_message(self.g3_net, req)
            conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3UDP_APP_CONF_QUEUE_IDX], timeout)
            if conf != None and g3_hi_cmd_ids.G3UDP_APP_CONF == conf.header.cmd_id and 0x00 == conf.data.status:
                if (ind_expected == True):
                    elapsed_time =  time.time() - req_time
                    remaining_time = max(0, timeout - elapsed_time) # Avoid negative numbers
                    ind = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3UDP_APP_IND_QUEUE_IDX], remaining_time)
                    if ind != None and g3_hi_cmd_ids.G3UDP_APP_IND == ind.header.cmd_id and ind.app_req_id == g3_hi_udp_app_type.UDP_RSP_APP_RHI_RSP_ID:
                        if g3_hi_cvec_equal(ind.src_addr.u8, ipv6_link_local_addr.addr.u8):
                            rhi_resp = g3_hi_msg_get_app_rhi_resp(ind.data, ind.data_len)
                            if rhi_resp.status == 0x00:
                                return [True, rhi_resp]
                else:
                    return [True, None]
            retries -= 1
        self.logger.warning('{}, Error on UDP App Confirm'.format(ipv6_link_local_addr))
        return [False, None]

    def udp_app_send_no_resp(self, ipv6_addr, payload_len):
        payload = g3_hi_vec_to_cbyte_vec([random.randint(0,255) for i in range(payload_len)])
        req = g3_hi_msg_get_g3_udp_app_req(g3_hi_udp_app_type.UDP_RSP_APP_SEND_NO_RSP_ID, ipv6_addr, payload_len, payload)
        g3_hi_msg_send_message(self.g3_net, req)
        conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[self.G3UDP_APP_CONF_QUEUE_IDX], 100)
        if conf != None and g3_hi_cmd_ids.G3UDP_APP_CONF == conf.header.cmd_id and 0x00 == conf.data.status:
            return True
        elif conf != None and g3_hi_cmd_ids.G3UDP_APP_CONF == conf.header.cmd_id and 0x00 != conf.data.status:
            self.logger.warning("G3 UDP App Confirm: Error Status")
            return False
        else:
            self.logger.warning("No confirm")
            return False

    def ping_all_nodes(self, iter, lengths, num_wait_dev, stop_event_p):
        self.logger.error("Invalid ping handler")

    def start_ping_handler(self, iter, lengths, num_wait_dev = 0):
        self.ping_thread_stop_ev.clear()
        self.ping_thread = threading.Thread(target=self.ping_all_nodes, args=(iter, lengths, num_wait_dev, self.ping_thread_stop_ev))
        self.ping_thread.name = "ping_thread"
        self.ping_thread.start()

    def stop_ping_handler(self):
        self.logger.info("Stop ping handler")
        self.ping_thread_stop_ev.set()

    def is_ping_handler_alive(self):
        return self.ping_thread.isAlive()

    def file_write(self, file_name, start_sector):
        address = start_sector * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE
        with open(file_name, "rb") as f:
            total_size = os.fstat(f.fileno()).st_size
            current_size = 0
            payload = f.read(g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE)
            while(len(payload) != 0):
                if((address % g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE) == 0):
                    #self.logger.info("Erase - addr 0x:{0:08X}".format(address))                
                    req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.ERASE, address, 0, c_ubyte(0))
                    g3_hi_msg_send_message(self.g3_net, req)
                    conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
                    if conf == None:
                        self.logger.warning("Ops , no confirm after erase request")
                        return
                    if conf.header.cmd_id != g3_hi_cmd_ids.SFLASH_CONF:
                        self.logger.warning("Ops , wrong command after erase request: 0x" + str(conf.header.cmd_id))
                        return
                _pay = (c_ubyte * len(payload))()
                for j in range(0, len(payload)):
                    _pay[j] = c_ubyte(payload[j])
                req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.WRITE, address, len(payload), _pay)
                g3_hi_msg_send_message(self.g3_net, req)
                conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
                if conf == None:
                    self.logger.warning("Ops , no confirm")
                    return
                if conf.header.cmd_id != g3_hi_cmd_ids.SFLASH_CONF:
                    self.logger.warning("Ops , wrong command after write request: 0x" + str(conf.header.cmd_id))
                    return
                current_size += len(payload)
                self.logger.debug("Write " + str(round(current_size*100/total_size, 1)) + "%")
                address += len(payload)
                payload = f.read(g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE)

    def local_update(self, cortex_image=None, rte_image=None):
        if(cortex_image != None):
            sector = 2
            self.logger.info("Write starting from sector " + str(sector) + " file " + cortex_image)
            self.file_write(cortex_image, sector)
            self.logger.info("Done")
        if(rte_image != None):
            sector = 0
            self.logger.info("Write starting from sector " + str(sector) + " file " + rte_image)
            self.file_write(rte_image, sector)
            self.logger.info("Done")

    def hw_reset(self):
        self.stop_asynch_evt_handler()
        self.asynch_events.join()
        req = g3_hi_msg_hwreset_req()
        g3_hi_msg_send_message(self.g3_net, req)
        conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[3])
        if conf != None:
            self.logger.info("HW Reset OK")
        self.start_asynch_evt_handler()

    def read_nvm(self, section=g3_hi_nvm_section.GEN):
        if section == g3_hi_nvm_section.GEN:
            nvm_size = sizeof(g3_hi_nvm_gen)
        elif section == g3_hi_nvm_section.PLC:
            nvm_size = sizeof(g3_hi_nvm_plc)
        # Read the NVM content
        req = g3_hi_msg_nvm_req(g3_hi_nvm_operation.READ, section, 0, nvm_size, None)
        g3_hi_msg_send_message(self.g3_net, req)
        msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
        if msg == None:
            self.logger.warning("NVM Confirm not received")
            return None
        elif g3_hi_cmd_ids.NVM_CONF != msg.header.cmd_id or msg.status != 0x00:
            self.logger.warning("NVM Confirm error")
            return None
        # Parse the NVM content
        if section == g3_hi_nvm_section.GEN:
            nvm = g3_hi_nvm_gen(msg.data)
            self.logger.info(str(nvm))
        elif section == g3_hi_nvm_section.PLC:
            nvm = g3_hi_nvm_plc(msg.data)
            self.logger.info(str(nvm))
        else:
            nvm = None
            self.logger.warning("Unsupported NVM section")
        return nvm

    def write_nvm(self, section, nvm_data):
        req = g3_hi_msg_nvm_req(g3_hi_nvm_operation.WRITE, section, 0, sizeof(nvm_data), nvm_data)
        g3_hi_msg_send_message(self.g3_net, req)
        msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
        if msg == None:
            self.logger.warning("NVM Confirm not received")
        elif g3_hi_cmd_ids.NVM_CONF != msg.header.cmd_id or msg.status != 0x00:
            self.logger.warning("NVM Confirm error")
        else:
            self.logger.info("NVM write OK")

    def sflash_erase(self, sector):  
        address = sector * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE
        self.logger.info("Erase - addr 0x:{0:08X}".format(address))                
        req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.ERASE, address, 0, c_ubyte(0))
        g3_hi_msg_send_message(self.g3_net, req)
        conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
        if conf == None:
            self.logger.warning("Ops , no confirm after erase request")
            return False
        if conf.header.cmd_id != g3_hi_cmd_ids.SFLASH_CONF:
            self.logger.warning("Ops , wrong command after erase request: 0x" + str(conf.header.cmd_id))
            return False
        return True

    def phy_set_trx(self, trx=g3_hi_phy_trx_state.PHY_STATE_TRX):
        req = g3_hi_msg_phy_set_trx_state_req(trx)
        self.logger.info("PHY set TRX state: " + str(g3_hi_phy_trx_state(trx))) 
        g3_hi_msg_send_message(self.g3_net, req)
        conf = g3_hi_msg_receive_message(self.g3_net)
        if conf == None:
            self.logger.warning("Ops, no confirm after set TRX request")
            return False
        if conf.header.cmd_id != g3_hi_cmd_ids.G3PHY_SETTRXSTATE_CONF:
            self.logger.warning("Ops, wrong command after set TRX request: 0x" + str(conf.header.cmd_id))
            return False     
        if conf.result != g3_hi_results.SUCCESS:
            self.logger.warning("Ops, wrong command after set TRX request: 0x" + str(g3_hi_results(conf.result)))
            return False
        return True



class g3_device(g3_node):
    """The g3_coord class represents the G3 PAN Device"""

    def __init__(self, position=0):
        super().__init__(position)
        self.coord_ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, 0x0000)

    def sw_reset(self, band_plan = g3_hi_band_plan.CEN_A, short_addr = None, pan_id = None):
        super().sw_reset(band_plan, short_addr, pan_id, g3_hi_device_type.DEVICE)

    def get_info(self):
        super().get_info()
        self.coord_ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, 0x0000)
        if (self.pan_id == 0xFFFF) or (self.short_addr == 0xFFFF) or (self.short_addr == 0x0000):
            self.logger.warning("Device, something wrong with PAN ID (should not be 0xFFFF) or Short Address (should not be 0x0000 or 0xFFFF)")
    
    def boot_evt_handler(self, arg1, stop_event):
        self.logger.info("Start Device bootstrap handler")
        while not stop_event.is_set():
            msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[2], to = 1)
            if msg != None:
                if (g3_hi_cmd_ids.G3BOOT_DEV_START_CONF == msg.header.cmd_id) and (msg.status == 0x00):
                    self.pan_id = msg.net_addr
                    self.short_addr = msg.pan_id
                    self.ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, self.short_addr)
                    self.coord_ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, 0x0000)
                if ((g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_CONF == msg.header.cmd_id) and (msg.status == 0x00)) or\
                    (g3_hi_cmd_ids.G3BOOT_DEV_LEAVE_IND == msg.header.cmd_id):
                    self.pan_id = c_ushort(0xFFFF)
                    self.short_addr = c_ushort(0xFFFF)
                    self.ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, self.short_addr)
                    self.coord_ipv6_link_local_addr = g3_hi_ipv6_link_local_addr(self.pan_id, 0x0000)

    def ping_coord(self, length):
        ret = self.ping(self.coord_ipv6_link_local_addr, length)

    def ping_all_nodes(self, iter, lengths, num_wait_dev, stop_event_p):
        for l in lengths:
            for i in iter:
                ret = self.ping_coord(l)

class g3_coord(g3_node):
    """The g3_coord class represents the G3 PAN Coordinator"""

    def __init__(self, position=0):
        super().__init__(position)
        self.server_start_time = time.time()
        self.dev = []
        self.pings_stat = g3_coord_ping_stats()

    def sw_reset(self, band_plan = g3_hi_band_plan.CEN_A, short_addr = None, pan_id = None):
        super().sw_reset(band_plan, short_addr, pan_id, g3_hi_device_type.COORD)

        # Wait for BOOT-SERVER-START
        while True:
            bootServerStartWaitTime = self.get_attr(g3_hi_attrib_id.BOOT_SERVER_START_WAIT_TIME)
            bootServerDiscoveryTime = self.get_attr(g3_hi_attrib_id.BOOT_SERVER_DISCOVERY_TIME)
            msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[2], bootServerStartWaitTime + bootServerDiscoveryTime + 2)
            if g3_hi_cmd_ids.G3BOOT_SRV_START_CONF == msg.header.cmd_id:
                self.logger.info("Boot Server start")
                self.server_start_time = time.time()
                self.dev = []
                return

    def get_info(self):
        super().get_info()
        if (self.pan_id == 0xFFFF) or (self.short_addr != 0x0000):
            self.logger.warning("Coordinator, something wrong with PAN ID (should not be 0xFFFF) or Short Address (should be 0x0000)")

    def get_conn_devices(self):
        # scan NVM to find nodes info
        for i in range (0, g3_hi_defines.NVM_TAB_NUMNODEMAX):
            req = g3_hi_msg_nvm_req(g3_hi_nvm_operation.TAB_READ_ENTRY, g3_hi_nvm_section.ALL, i, 0, None)
           
            g3_hi_msg_send_message(self.g3_net, req)
            conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
            nvm_tab = g3_hi_nvm_tab()
            nvm_tab.init_from_buffer(conf.data)
            # for valid entry check if it is already connected
            if nvm_tab.short_addr != g3_hi_defines.MAC_BRD_ADDR:
                req = g3_hi_msg_nvm_req(g3_hi_nvm_operation.TAB_IS_CONNECTED, g3_hi_nvm_section.ALL, i, 0, None)
                g3_hi_msg_send_message(self.g3_net, req)
                conf = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[5])
                nvm_conn = g3_hi_nvm_tab_is_connected()
                nvm_conn.init_from_buffer(conf.data)
                # for connected devices create an entry in the remote dev list
                if  nvm_conn.is_conn == True:
                    device = g3_remote_device(nvm_tab.ext_addr, self.pan_id, nvm_tab.short_addr, nvm_conn.is_conn)
                    self.logger.info("Joined " + str(device) + " in [s] {:8n}".format(round(self.server_start_time - self.server_start_time,1)))
                    device.stats.set_reg_time(self.server_start_time - self.server_start_time)
                    self.dev.append(device)
            else:
                #at the first unknow entry, exit
                return

    def sort_conn_dev_custom(self, sorting_function, reverse=False):
        # Sorts Device lists basing on a custom sorting function
        # The sorting function shall return an integer used by the 
        # sorted method to order the list
        # g3_remote_device class must be in the scope of sorting function
        self.dev = sorted(self.dev,key=sorting_function, reverse=reverse)

    def sort_conn_devices(self, reverse=False):
        self.dev = sorted(self.dev, reverse=reverse)

    def print_dev(self):
        for d in self.dev:
            self.logger.info(str(d))

    def print_dev_stats(self):
        for d in self.dev:
            d.print_stats()

    def boot_evt_handler(self, arg1, stop_event):
        self.logger.info("Start Coordinator bootstrap handler")
        while not stop_event.is_set():
            msg = g3_hi_msg_receive_message(self.g3_net, self.g3_net.queues[2], to = 1)
            if msg != None:
                if g3_hi_cmd_ids.G3BOOT_SRV_JOIN_IND == msg.header.cmd_id:
                    # check if the device is already in the list: in case we need to keep only the most updated instance
                    is_rejoin = False
                    for dev in self.dev:
                        if str(dev.extended_addr) == str(msg.ext_addr):
                            is_rejoin = True
                            self.dev.remove(dev)
                            break
                    device = g3_remote_device(msg.ext_addr, self.pan_id, msg.short_addr, True)
                    if is_rejoin is True:
                        self.logger.info('Re-join, {} in [s] {:8n}'.format(device, round(time.time() - self.server_start_time,1)))
                    else:
                        self.logger.info('Join #{}, {} in [s] {:8n}'.format(len(self.dev) + 1, device, round(time.time() - self.server_start_time,1)))
                    device.stats.set_reg_time(time.time() - self.server_start_time)
                    self.dev.append(device)   

    def reset_remote_node(self, dev, reset_in_secs=60, is_hw_reset=True, band_plan=g3_hi_band_plan.CEN_A):
        if is_hw_reset is True:
            hi_msg = g3_hi_msg_hwreset_req()
        else:
            hi_msg = g3_hi_msg_swreset_req(band_plan, g3_hi_device_type.DEVICE, g3_hi_mode.MODE_IPV6_BOOT)
        dev_reset_delay = int(reset_in_secs * 1000)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg, dev_reset_delay)
        self.logger.info('{}, reset in {} ms'.format(dev.ipv6_link_local_addr, str(dev_reset_delay)))
        ret = self.udp_app_rhi(dev.ipv6_link_local_addr, rhi_req)
        if ret[0] is False:
            self.logger.warning('Error sending RHI Reset request')
            return False
        return True

    def reset_all_nodes(self, reset_in_secs=60, is_hw_reset=True, band_plan=g3_hi_band_plan.CEN_A):
        if is_hw_reset is True:
            hi_msg = g3_hi_msg_hwreset_req()
        else:
            hi_msg = g3_hi_msg_swreset_req(band_plan, g3_hi_device_type.DEVICE, g3_hi_mode.MODE_IPV6_BOOT)
        retry_list = []
        abs_reset_t = time.time() + reset_in_secs
        #1st attempt
        for d in self.dev:
            dev_reset_delay_secs = abs_reset_t - time.time()
            if dev_reset_delay_secs <= 0:
                self.logger.warning("Reset all nodes, Abs time reached")
                return False
            dev_reset_delay = int(dev_reset_delay_secs * 1000)
            rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg, dev_reset_delay)
            self.logger.info('{}, reset in {} ms'.format(d.ipv6_link_local_addr, str(dev_reset_delay)))
            ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req)
            if ret[0] is False:
                retry_list.append(d)
        #2nd attempt
        for d in retry_list:
            dev_reset_delay_secs = abs_reset_t - time.time()
            if dev_reset_delay_secs <= 0:
                self.logger.warning("Reset all nodes, Abs time reached")
                return False
            dev_reset_delay = int(dev_reset_delay_secs * 1000)
            rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg, dev_reset_delay)
            self.logger.info('{}, reset in {} ms'.format(d.ipv6_link_local_addr, str(dev_reset_delay)))
            ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req)
            if ret[0] is False:
                self.logger.warning('2nd attempt, Error sending RHI Reset request')
        return True
        
    def update_all_nodes(self, stcomet_cortex_image=None, stcomet_rte_image=None, st8500_cortex_image=None, st8500_rte_image=None, check_update=False, retries=2):
        for device in self.dev:
            if (g3_hi_platform.ST8500 == device.platform):
                curr_retries = retries
                while self.remote_update(device, st8500_cortex_image, st8500_rte_image, check_update) != [True, True] and curr_retries > 0:
                    print(str(device.short_addr) + ": Update as " + str(device.platform) + "...")
                    print(str(device.short_addr) + ": ... Done")
                    curr_retries -= 1
            elif (g3_hi_platform.STCOMET == device.platform):
                curr_retries = retries
                while self.remote_update(device, stcomet_cortex_image, stcomet_rte_image, check_update) != [True, True] and curr_retries > 0:
                    print(str(device.short_addr) + ": Update as " + str(device.platform) + "...")
                    print(str(device.short_addr) + ": ... Done")
                    curr_retries -= 1
            else:
                print(str(device.short_addr) + ": Wrong platform: " + str(device.platform))

    def dbginfo_remote_device(self, d, timeout=120):
        hi_msg = g3_hi_msg_dbgtool_req(g3_hi_dbgtool_ctrl.HI_DBG_INFO, 0x00)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg)
        #1st attempt
        ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req, timeout=timeout)
        if None != ret[1]:
            dbgtool_conf = g3_hi_msg_dbgtool_conf(ret[1].data, sizeof(ret[1].data))
            self.logger.info("{}".format(d.ipv6_link_local_addr) + ", " + str(dbgtool_conf))
            d.platform = dbgtool_conf.platform
        else:
            ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req, timeout=timeout)
            if None == ret[1]:
                self.logger.error('2nd attempt, Error sending DBG INFO request')
                return False
            else:
                dbgtool_conf = g3_hi_msg_dbgtool_conf(ret[1].data, sizeof(ret[1].data))
                self.logger.info("{}".format(d.ipv6_link_local_addr) + ", " + str(dbgtool_conf))
                d.platform = dbgtool_conf.platform
                retry_list.remove(d)

        #return the debug info structure
        return dbgtool_conf


    def dbginfo_all_nodes(self, inter_pkt_delay=0, timeout=120):
        retry_list = []
        hi_msg = g3_hi_msg_dbgtool_req(g3_hi_dbgtool_ctrl.HI_DBG_INFO, 0x00)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg)
        #1st attempt
        for d in self.dev:
            ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req, timeout=timeout)
            if None == ret[1]:
                retry_list.append(d)
            else:
                dbgtool_conf = g3_hi_msg_dbgtool_conf(ret[1].data, sizeof(ret[1].data))
                self.logger.info("{}".format(d.ipv6_link_local_addr) + ", " + str(dbgtool_conf))
                d.platform = dbgtool_conf.platform
            if inter_pkt_delay != 0:
                time.sleep(inter_pkt_delay)
        #2nd attempt
        for d in retry_list:
            ret = self.udp_app_rhi(d.ipv6_link_local_addr, rhi_req, timeout=timeout)
            if None == ret[1]:
                self.logger.error('2nd attempt, Error sending DBG INFO request')
            else:
                dbgtool_conf = g3_hi_msg_dbgtool_conf(ret[1].data, sizeof(ret[1].data))
                self.logger.info("{}".format(d.ipv6_link_local_addr) + ", " + str(dbgtool_conf))
                d.platform = dbgtool_conf.platform
                retry_list.remove(d)
            if inter_pkt_delay != 0:
                time.sleep(inter_pkt_delay)
        #return the number of successfully read node
        return len(self.dev)-len(retry_list)

    def nvm_read_remote_device(self, device, section):
        if section == g3_hi_nvm_section.GEN:
            nvm_size = sizeof(g3_hi_nvm_gen)
        elif section == g3_hi_nvm_section.PLC:
            nvm_size = sizeof(g3_hi_nvm_plc)
        hi_msg = g3_hi_msg_nvm_req(g3_hi_nvm_operation.READ, section, 0, nvm_size, None)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg)
        ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
        if None == ret[1]:
            self.logger.warning("Error sending NVM READ request")
            return None
        nvm_conf = g3_hi_msg_nvm_conf(ret[1].data, sizeof(ret[1].data))
        if g3_hi_cmd_ids.NVM_CONF != nvm_conf.header.cmd_id or nvm_conf.status != 0x00:
            self.logger.warning("Remote NVM Confirm error")
            return None
 
        # Parse the NVM content
        if section == g3_hi_nvm_section.GEN:
            nvm = g3_hi_nvm_gen(nvm_conf.data)
            self.logger.info("{}".format(device.ipv6_link_local_addr) + ", " + str(nvm))
        elif section == g3_hi_nvm_section.PLC:
            nvm = g3_hi_nvm_plc(nvm_conf.data)
            self.logger.info("{}".format(device.ipv6_link_local_addr) + ", " + str(nvm))
        else:
            nvm = None
            self.logger.warning("{}".format(device.ipv6_link_local_addr) + ", unsupported NVM section")            
        return nvm

    def nvm_write_remote_device(self, device, section, nvm_data):
        hi_msg = g3_hi_msg_nvm_req(g3_hi_nvm_operation.WRITE, section, 0, sizeof(nvm_data), nvm_data)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg)
        ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
        if None == ret[1]:
            self.logger.warning("Error sending NVM write request")
        else:
            nvm_conf = g3_hi_msg_nvm_conf(ret[1].data, sizeof(ret[1].data))
            if g3_hi_cmd_ids.NVM_CONF != nvm_conf.header.cmd_id or nvm_conf.status != 0x00:
                self.logger.warning("Remote NVM confirm error")
            else:
                self.logger.info("{}".format(device.ipv6_link_local_addr) + ", NVM write OK")

    def ping_all_nodes(self, iter, lengths, num_wait_dev, stop_event_p, save_stats=False):

        self.pings_stat.reset_stats()

        #wait the devices in the network
        while len(self.dev) < num_wait_dev:
            time.sleep(1)

        #just wait in case of route establishment
        time.sleep(4)

        for l in lengths:
            self.logger.info("Start ping loop for " + str(l) + " bytes")
            for i in range(0, iter):
                d_idx = 0
                for d in self.dev:
                    d_idx += 1
                    self.pings_stat.increase_ping(i, l)
                    d.stats.update_ping_tx(l) 
                    self.logger.info("Node {:3d}/{:3d} ping  {:4d} bytes".format(d_idx, len(self.dev), l)+ " {:8d}".format(i) + " {}".format(d.ipv6_link_local_addr) )
                    ret = self.ping(d.ipv6_link_local_addr, l)
                    result = ret[0]
                    rtt = ret[1]                   
                    if result == True:
                        self.pings_stat.increase_ping_ok(i, l, rtt)
                        d.stats.update_ping_ok(l, rtt)
                    #save stat at the end of each ping
                    self.pings_stat.serialize_stats_on_pkl_file()
                    #check for ping stop
                    if stop_event_p.is_set():
                        if save_stats:
                            self.pings_stat.save_stats_on_xlsx_file()
                        self.pings_stat.print_statistics_iteration()
                        self.pings_stat.print_aggregate_statistics()
                        self.print_dev_stats()
                        return

        #save stats only at the end
        self.pings_stat.save_stats_on_xlsx_file()

    def path_disc(self, short_addr, retries=3):
        req = g3_hi_msg_adp_path_disc_req(short_addr)
        while (retries > 0):
            g3_hi_msg_send_message(self.g3_net, req)
            conf = g3_hi_msg_receive_message(self.g3_net, to=40+5) #path discovery procedure has a specific timeout (ADP_PATHDISCOVERY_TIMEOUT)
            if conf != None and conf.header.cmd_id == g3_hi_cmd_ids.G3ADP_PATHDISC_CONF:
                self.logger.info(str(conf))
                return conf
            retries -= 1
        return None

    def get_path_info(self, short_addr, retries=3):
        # Use Path Discovery to get the info for the specific node
        pd_conf = self.path_disc(short_addr, retries)
        pd_info = {'Hop count': 0, 'Forward path addr' : [], 'Fwd link cost': [], 'Backward path addr': [], 'Bck link cost': []}
        if pd_conf != None:
            #  Check status (byte 2)
            if pd_conf.result != 0:
                return pd_info  
            # next hop list starts at byte 7
            addr_list = []
            link_cost = []
            for item in pd_conf.hops_info:
                addr_list.append(item.addr)
                link_cost.append(item.link_cost)
            # Search for destination address in path
            dest_ind = addr_list.index(short_addr)
            # First part is fwd path
            pd_info['Forward path addr'] = addr_list[:dest_ind+1]
            pd_info['Fwd link cost'] = link_cost[:dest_ind+1]
            pd_info['Hop count'] = len(pd_info['Forward path addr'])
            # Second part is bck path
            pd_info['Backward path addr'] = addr_list[dest_ind+1:]
            pd_info['Bck link cost'] = link_cost[dest_ind+1:]

        return pd_info

    def path_discovery(self, generate_graph=False, filename_additional_info="", retries=3):
        topology_info_list = []
        self.logger.info("Create network graph info")
        for d in self.dev:
            while(retries > 0):
                req = g3_hi_msg_adp_path_disc_req(d.short_addr)
                g3_hi_msg_send_message(self.g3_net, req)
                conf = g3_hi_msg_receive_message(self.g3_net, to=40+5) #path discovery procedure has a specific timeout (ADP_PATHDISCOVERY_TIMEOUT)
                if conf != None and conf.header.cmd_id == g3_hi_cmd_ids.G3ADP_PATHDISC_CONF:
                    self.logger.info(str(conf))
                    hops_info = []
                    # create the list of the hops info
                    for hop_inf in conf.hops_info:
                        hops_info.append(g3_path.g3_hop_info(hop_inf.addr, hop_inf.link_cost))
                    # add the path for the specific device in the topology list
                    topology_info_list.append(g3_path.g3_device_path_info(self.short_addr, d.short_addr, g3_hi_metric_type.LOADng_G3, len(conf.hops_info), hops_info))
                    break
                retries -= 1
        save_pkl_file = filename_prefix_time()+filename_additional_info+"topology_info_list.pkl"
        g3_path.serialize_topology_info(topology_info_list, save_pkl_file)
        if(generate_graph == True):
            try:
                #g3_path.g3_generate_graph_gv(g3_path.g3_graph_type[1], pkl_file=save_pkl_file, filename_prefix=filename_suffix_time)
                g3_path.g3_generate_graph_gv(pkl_file=save_pkl_file, filename_prefix=(filename_prefix_time()+filename_additional_info))
            except:
                self.logger.warning("Exception in graph creation, topology file: " + save_pkl_file)
                return

    def get_attr_remote_device(self, device, attr_id, attr_idx=0, retries=3):
        val = None
        
        attrib = g3_hi_pib_id()
        attrib.id = attr_id
        attrib.index = attr_idx
        req = g3_hi_msg_g3lib_get_attrib_req(attrib)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(req)
        while(retries > 0):
            ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
            if None == ret[1]:
                self.logger.warning("Error sending get request to {}".format(device.ipv6_link_local_addr))
                retries -= 1
                continue
            conf = g3_hi_msg_get_g3lib_get_attrib_conf(ret[1].data, sizeof(ret[1].data))
            if (g3_hi_cmd_ids.G3LIB_GET_CONF != conf.header.cmd_id) or (conf.state != g3_hi_results.SUCCESS):
                self.logger.error("{} wrong {}, id: {} status: {}".format(device.ipv6_link_local_addr, g3_hi_cmd_ids.G3LIB_GET_CONF, conf.header.cmd_id, conf.state))
                retries -= 1
                continue
            val = g3_hi_ctype2int(conf.len, conf.value)
            self.logger.info("{}, id {}, idx {:04d}, value 0x{:X}".format(device.ipv6_link_local_addr, str(attr_id), attr_idx, val))
            break
        return val

    def get_attr_all_nodes(self, attr_id, attr_idx=0):
        attr_val_list = []
        for d in self.dev:
            attr_val = self.get_attr_remote_device(d, attr_id, attr_idx)
            attr_val_list.append(attr_val)
        return attr_val_list

    def set_attr_remote_device(self, device, attr_id, attr_idx=0, attr_val_len=0, attr_val=None):
        req = g3_hi_msg_get_g3lib_set_attrib_req(attr_id, attr_idx, attr_val_len, attr_val)
        rhi_req = g3_hi_msg_get_g3_app_rhi_req(req)
        ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
        if None == ret[1]:
            self.logger.warning("Error sending set request to {}".format(device.ipv6_link_local_addr))
            return False
        conf =g3_hi_msg_get(ret[1].data, None)
        #conf = g3_hi_msg_get_g3lib_set_attrib_conf(ret[1].data, sizeof(ret[1].data))
        if (g3_hi_cmd_ids.G3LIB_SET_CONF != conf.header.cmd_id) or (conf.state != g3_hi_results.SUCCESS):
            self.logger.error("{} wrong {}, id: {} status: {}".format(device.ipv6_link_local_addr, g3_hi_cmd_ids.G3LIB_SET_CONF, conf.header.cmd_id, conf.state))
            return False
        if type(attr_val) is list    :
            val_str = ''.join('{:02X}'.format(x) for x in attr_val)
        else:
            val_str = '{:X}'.format(attr_val)
        self.logger.info("{} - id {} - idx {:04d} - value 0x".format(device.ipv6_link_local_addr, str(attr_id), attr_idx)+val_str)
        return True

    def set_attr_list_remote_device(self, device, attr_list):
        for attr in attr_list:
            self.set_attr_remote_device(device, attr[0], attr[1], attr[2], attr[3])
        return True

    def set_attr_list_all_nodes(self, attr_list):
        for d in self.dev:
            self.set_attr_list_remote_device(d, attr_list)
        return True

    def file_write_remote(self, device, file_name, start_sector):
        address = start_sector * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE
        self.logger.info("Begin writing to device: " + str(device.ipv6_link_local_addr))
        with open(file_name, "rb") as f:
            #total_size = os.fstat(f.fileno()).st_size
            current_size = 0
            payload = f.read(g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE)
            while(len(payload) != 0):
                if((address % g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE) == 0):
                    hi_msg_req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.ERASE, address, 0, c_ubyte(0))
                    rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg_req)
                    ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
                    if ret[0] is False:
                        return False
                crc_data = libscrc.xmodem(bytes(payload))
                _pay = (c_ubyte * len(payload))()
                for j in range(0, len(payload)):
                    _pay[j] = c_ubyte(payload[j])
                hi_msg_req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.WRITE_AND_EVAL_CRC, address, len(payload), _pay)
                rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg_req)
                ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
                if ret[0] is False:
                    return False
                hi_msg_conf = ret = g3_hi_msg_get(ret[1].data, None)
                if(sizeof(hi_msg_conf.payload) >= 2):
                    crc_calc = hi_msg_conf.payload[0] | (hi_msg_conf.payload[1] << 8)
                    if (crc_data != crc_calc):
                        self.logger.info("Wrong write {} {}".format(crc_data, crc_calc))
                        return False
                else:
                    self.logger.info("Wrong message payload")
                    return False
                current_size += len(payload)
                #self.logger.info("Write " + str(round(current_size*100/total_size, 1)) + "%")
                address += len(payload)
                payload = f.read(g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE)
        self.logger.info("Completed writing to device: " + str(device.ipv6_link_local_addr))
        return True

    def file_read_remote(self, device, file_len, start_sector):
        #total_size = file_len
        current_size = 0
        address = start_sector * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE
        payload_ret = []
        self.logger.info("Begin reading from device: " + str(device.ipv6_link_local_addr))
        while(file_len > 0):
            current_toread_len = g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE if \
                (file_len > g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE) else file_len
            hi_msg = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.READ, address, current_toread_len, None)
            rhi_req = g3_hi_msg_get_g3_app_rhi_req(hi_msg)
            ret = self.udp_app_rhi(device.ipv6_link_local_addr, rhi_req)
            if None == ret[1]:
                return None
            conf = g3_hi_msg_get_sflash_conf(ret[1].data, sizeof(ret[1].data))
            payload_read = list(conf.payload)
            current_read_len = len(payload_read)
            current_size += current_read_len
            address += current_read_len
            file_len -= current_read_len
            payload_ret += payload_read
            #self.logger.info("Read " + str(round(current_size*100/total_size, 1)) + "%")
        self.logger.info("Completed reading from device: " + str(device.ipv6_link_local_addr))
        return payload_ret

    def remote_update(self, device, cortex_image=None, rte_image=None, check_update=False):
        ret = [True, True]
        if (cortex_image != None):
            with open(cortex_image, "rb") as f:
                file_toread_len = os.fstat(f.fileno()).st_size
                cortex_data = f.read()
            sector = 2
            self.logger.info("Write starting from sector " + str(sector) + " file " + cortex_image)
            ret[0] = self.file_write_remote(device, cortex_image, sector)
            if check_update:
                cortex_ret = self.file_read_remote(device, file_toread_len, sector)
                if (g3_hi_cvec_equal(cortex_ret, cortex_data)):
                    self.logger.info("File " + cortex_image + " written correctly")
                else:
                    self.logger.warning("Error: failed to write file " + cortex_image)
                    ret[0] = False
        if (rte_image != None):
            with open(rte_image, "rb") as f:
                file_toread_len = os.fstat(f.fileno()).st_size
                rte_data = f.read()
            sector = 0
            self.logger.info("Write starting from sector " + str(sector) + " file " + rte_image)
            ret[1] = self.file_write_remote(device, rte_image, sector)
            if check_update:
                rte_ret = self.file_read_remote(device, file_toread_len, sector)
                if (g3_hi_cvec_equal(rte_ret, rte_data)):
                    self.logger.info("File " + rte_image + " written correctly")
                else:
                    self.logger.warning("Error: failed to write file " + rte_image)
                    ret[1] = False
        return ret
        
if __name__ == "__main__":

    # Start PAN Coord from the scratch
    # coord = g3_coord()
    # coord.open(coord_port_name)
    # coord.sw_reset(g3_hi_band_plan.CEN_A, 0x0000, 0x781D, g3_hi_device_type.COORD)

    # PAN Coord already up
    coord = g3_coord()
    #coord.open(coord_port_name, 115200)
    coord.open("COM35", 115200)
    coord.logger.info(str(coord))

    coord.get_conn_devices()

    '''
    coord.dbginfo_all_nodes()

    coord.get_attr_all_nodes(g3_hi_attrib_id.G3_LIB_PEFWVERSION_ID)

    coord.path_discovery()

    coord.start_ping_handler(iter = 10, lengths = [100, 600, 1200], num_wait_dev = 2)

    #wait ping thread to be completed
    coord.ping_thread.join()

    coord.path_discovery(generate_graph=True)
    '''
    coord.close()
    