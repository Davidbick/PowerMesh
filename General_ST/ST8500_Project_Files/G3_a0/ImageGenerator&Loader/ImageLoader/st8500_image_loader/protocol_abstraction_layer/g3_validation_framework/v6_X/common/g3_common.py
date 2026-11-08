try:
    import g3_network
    from g3_network import *
    from ctypes import *
except:
    print("Error importing libraries in g3_common.py")
    raise ImportError

# Functions
def g3_get_default_tonemap_tonemask(band_plan=g3_hi_band_plan.CEN_A):
    if (g3_hi_band_plan.CEN_A == band_plan):
        tone_map    = g3_hi_vec_to_cbyte_vec([0x3F, 0x00, 0x00])
        tone_mask   = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00])
    elif (g3_hi_band_plan.CEN_B == band_plan):
        tone_map    = g3_hi_vec_to_cbyte_vec([0x0F, 0x00, 0x00])
        tone_mask   = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    elif (g3_hi_band_plan.FCC == band_plan):
        tone_map    = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF])
        tone_mask   = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF])
    elif (g3_hi_band_plan.FCC_LOW == band_plan):
        tone_map    = g3_hi_vec_to_cbyte_vec([0xFF, 0x07, 0x00])
        tone_mask   = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00])
    else: #FCC_HIGH
        tone_map    = g3_hi_vec_to_cbyte_vec([0xFF, 0x07, 0x00])
        tone_mask   = g3_hi_vec_to_cbyte_vec([0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00])

    return [tone_map, tone_mask]

def g3_get_default_tx_coeff():
    return g3_hi_vec_to_cbyte_vec([0, 0, 0, 0, 0, 0])

def g3_get_max_phy_len(band_plan=g3_hi_band_plan.CEN_A):
    if (g3_hi_band_plan.CEN_A == band_plan):
        max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:132, 
                                                   g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:234,
                                                   g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:234,
                                                   g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:226},
                    g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:120, 
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:238, 
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:230, 
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:230}}
        rs_blocks = g3_hi_two_rs_blocks.ONE_RS_BLOCK
    elif (g3_hi_band_plan.CEN_B == band_plan):
        max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:54,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:235,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:239,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:235},
                    g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:46,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:203,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:235,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:235}}
        rs_blocks = g3_hi_two_rs_blocks.ONE_RS_BLOCK
    elif (g3_hi_band_plan.FCC == band_plan):
        #max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:494, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:470, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:476} , g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:494, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:476}}
        #real max lengths are not supported by the RTE<->PE interface. Limitation is due to MAC layer max MSDU of 400 bytes
        max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410},
                    g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410}}
        rs_blocks = g3_hi_two_rs_blocks.TWO_RS_BLOCKS
    elif (g3_hi_band_plan.FCC_LOW == band_plan):
        #max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:244, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:472} , g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:220, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:476, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:476, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:472}}
        #real max lengths are not supported by the RTE<->PE interface. Limitation is due to MAC layer max MSDU of 400 bytes
        max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:244,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410},
                    g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:220,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410}}
        rs_blocks = g3_hi_two_rs_blocks.TWO_RS_BLOCKS
    else: #FCC HIGH
        #max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:236, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:478, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:470} , g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:212, g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:476, g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:474, g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:476}}
        #real max lengths are not supported by the RTE<->PE interface. Limitation is due to MAC layer max MSDU of 400 bytes
        max_len = {g3_hi_mod_scheme.DIFFERENTIAL: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:236,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                    g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410},
                    g3_hi_mod_scheme.COHERENT: {g3_hi_phy_mod_type.PHY_MOD_TYPE_ROBO:212,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DBPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_DQPSK:410,
                                                g3_hi_phy_mod_type.PHY_MOD_TYPE_D8PSK:410}}
        rs_blocks = g3_hi_two_rs_blocks.TWO_RS_BLOCKS
    return [max_len, rs_blocks]

def g3_hi_node_clear(net_info_ex, band_plan, device_type=g3_hi_device_type.DEVICE, device_mode=g3_hi_mode.MODE_PHY, pan_id=None, short_addr=None, channel=g3_hi_channel_config.CHANNEL_ALL):
    while g3_hi_msg_receive_message(net_info_ex, to=0) is not None:
        pass

    #SW-Reset
    sw_rst_req = g3_hi_msg_swreset_req(band_plan, device_type, device_mode)
    g3_hi_msg_send_message(net_info_ex, sw_rst_req)
    sw_rst_conf = g3_hi_msg_receive_message(net_info_ex)
    if (g3_hi_cmd_ids.G3LIB_SWRESET_CONF != sw_rst_conf.header.cmd_id):
        print('Expected cmd {}, got {}'.format(str(g3_hi_cmd_ids.G3LIB_SWRESET_CONF), str(g3_hi_cmd_ids(sw_rst_conf.header.cmd_id))))
        print(str(sw_rst_conf))
        return False
    if (sw_rst_conf.status != g3_hi_results.SUCCESS):
        print('Expected result {}, got {}'.format(str(g3_hi_results.SUCCESS), str(g3_hi_results(sw_rst_conf.status))))
        return False

    #configure PAN ID - only for saving in NVM and use IPv6_BOOT or BOOT mode
    if pan_id != None:
        if g3_hi_set_attr(net_info_ex, g3_hi_attrib_id.MAC_PANID_ID, 0, 2, pan_id) == False:
            print('Not able to configure PAN-ID')
            return False

    #configure short addresses - only for saving in NVM and use IPv6_BOOT or BOOT mode
    if short_addr != None:
        if g3_hi_set_attr(net_info_ex, g3_hi_attrib_id.MAC_SHORTADDRESS_ID, 0, 2, short_addr) == False:
            print('Not able to configure Short Address')
            return False

    #HW-Reset
    if g3_hi_node_hw_reset(net_info_ex) == False:
        print('Not able to do HW-Reset')
        return False

    # Channel configuration
    if g3_hi_set_attr(net_info_ex, g3_hi_attrib_id.G3_LIB_PECHANNELCONFIG_ID, 0, 4, channel) == False:
        print('Not able to configure Channel')
        return False

    while g3_hi_msg_receive_message(net_info_ex, to=0) is not None:
        pass
    
    return True


def g3_hi_node_hw_reset(net_info_ex):
    req = g3_hi_msg_hwreset_req()
    g3_hi_msg_send_message(net_info_ex, req)
    conf = g3_hi_msg_receive_message(net_info_ex) 
    if (g3_hi_cmd_ids.HWRESET_CONF != conf.header.cmd_id):
        print('Expected cmd {}, got {}'.format(g3_hi_cmd_ids.HWRESET_CONF, conf.header.cmd_id))
        return False
    return True

def g3_hi_set_attr_return_idx(net_info_ex, attr_id, attr_idx, attr_len, attr_value):
    if isinstance(attr_value, int):  #integer case
        attr_value_ = g3_hi_int2ctype(attr_len, attr_value)
    elif isinstance(attr_value, ctypes.Array) or g3_network.g3_hi_is_ctypes_base_type(attr_value): #ctypes
        attr_value_ = attr_value
    else: #list, bytearrays, tuples and so on
        attr_value_ = g3_hi_vec_to_cbyte_vec(attr_value)
    req = g3_hi_msg_get_g3lib_set_attrib_req(attr_id, attr_idx, attr_len, attr_value_)
    g3_hi_msg_send_message(net_info_ex, req)
    conf = g3_hi_msg_receive_message(net_info_ex)
    if (g3_hi_cmd_ids.G3LIB_SET_CONF != conf.header.cmd_id):
        print("0x{:08x}".format(attr_id) + " Wrong cmd_id, expected " + str(g3_hi_cmd_ids.G3LIB_SET_CONF) + ", got " + str(g3_hi_cmd_ids(conf.header.cmd_id)))
        return False
    if (conf.state != g3_hi_results.SUCCESS) or (conf.attribute.id != attr_id):
        #print("0x{:08x}".format(attr_id) + " Set fail, expected result " + str(g3_hi_results.SUCCESS) + ", got " + str(g3_hi_results(conf.state)) )
        return False
    return conf.attribute.idx

def g3_hi_set_attr(net_info_ex, attr_id, attr_idx, attr_len, attr_value):
    ret_idx = g3_hi_set_attr_return_idx(net_info_ex, attr_id, attr_idx, attr_len, attr_value)
    if ret_idx is False:
        return False
    return True
    

def g3_hi_get_attr(net_info_ex, attr_id, attr_idx=0):
    attrib = g3_hi_pib_id()
    attrib.id = attr_id
    attrib.idx = attr_idx
    req = g3_hi_msg_g3lib_get_attrib_req(attrib)
    g3_hi_msg_send_message(net_info_ex, req)
    conf = g3_hi_msg_receive_message(net_info_ex)
    if (g3_hi_cmd_ids.G3LIB_GET_CONF != conf.header.cmd_id):
        print(" Wrong cmd_id, expected " + str(g3_hi_cmd_ids.G3LIB_GET_CONF) + ", got " + str(g3_hi_cmd_ids(conf.header.cmd_id)))
        print(conf)
        return False
    if (conf.state != g3_hi_results.SUCCESS):
        print(" Get fail, expected result " + str(g3_hi_results.SUCCESS) + ", got " + str(g3_hi_results(conf.state)) )
        return False

    #first handle all the "special" attributes such as tables
    if (attr_id == g3_hi_attrib_id.MAC_NEIGHBORTABLE_ID) or \
        (attr_id == g3_hi_attrib_id.MAC_NEIGHBORTABLE_BYSHORT_ID) or \
        (attr_id == g3_hi_attrib_id.MAC_NEIGHBORTABLE_VALIDENTRY_ID):
        tone_map = g3_get_default_tonemap_tonemask(g3_hi_band_plan.CEN_A)[0]
        tx_coeff = g3_get_default_tx_coeff()
        n_table = g3_hi_mac_neigh_table_entry(0xFFFF, 0, 0, 0, tone_map, 0, tx_coeff, 0, 0, 0)
        n_table.init_from_buffer(conf.value)
        return n_table
    elif (attr_id == g3_hi_attrib_id.ADP_ROUTING_TABLE) or \
            (attr_id == g3_hi_attrib_id.ADP_ROUTING_TABLE_BY_SHORT_ADDR) or \
            (attr_id == g3_hi_attrib_id.ADP_ROUTING_TABLE_VALID_ENTRY):
        r_table = g3_hi_adp_routing_table_entry(0, 0, 0, 0, 0, 0)
        r_table.init_from_buffer(conf.value)
        return r_table
    elif (attr_id == g3_hi_attrib_id.MAC_POSTABLE_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_POSTABLE_VALIDENTRY_ID):
        pos_table = g3_hi_pos_table_entry()
        pos_table.init_from_buffer(conf.value)
        return pos_table
    elif (attr_id == g3_hi_attrib_id.MAC_POSTABLE_RF_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_POSTABLE_VALIDENTRY_RF_ID):
        rf_pos_table = g3_hi_rf_pos_table_entry()
        rf_pos_table.init_from_buffer(conf.value)
        return rf_pos_table
    elif (attr_id == g3_hi_attrib_id.ADP_PREFIX_TABLE):
        prefix_table = g3_hi_adp_pref_tab_entry()
        prefix_table.init_from_buffer(conf.value)
        return prefix_table
    elif (attr_id == g3_hi_attrib_id.ADP_CONTEXT_INFORMATION_TABLE):
        context_table = g3_hi_adp_context_table_entry()
        context_table.init_from_buffer(conf.value)
        return context_table
    elif attr_id == g3_hi_attrib_id.MAC_EXTENDEDADDRESS_ID:
        return conf.value
    elif attr_id == g3_hi_attrib_id.ADP_BLACKLIST_TABLE:
        bl_entry = g3_hi_adp_bl_table_entry()
        bl_entry.init_from_buffer(conf.value)
        return bl_entry
    elif attr_id == g3_hi_attrib_id.ADP_BROADCAST_LOG_TABLE:
        brd_entry = g3_hi_adp_broadcast_table_entry()
        brd_entry.init_from_buffer(conf.value)
        return brd_entry
    elif ((attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_BYSHORT_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_VALIDENTRY_ID)):
        dt_entry = g3_hi_mac_dev_tab_entry()
        dt_entry.init_from_buffer(conf.value)
    elif ((attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_RF_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_BYSHORT_RF_ID) or \
            (attr_id == g3_hi_attrib_id.MAC_DEVICETABLE_VALIDENTRY_RF_ID)):
        dt_entry = g3_hi_mac_dev_tab_entry()
        dt_entry.init_from_buffer(conf.value)
    else:
        #if not special, return the integer value
        return g3_hi_ctype2int(conf.len, conf.value)

def g3_hi_mac_neigh_table_clear(net_info_ex, index, band_plan):

    tone_map = g3_get_default_tonemap_tonemask(band_plan)[0]
    tx_coeff = g3_get_default_tx_coeff()
    n_table = g3_hi_mac_neigh_table_entry(0xFFFF, 0, 0, 0, tone_map, 0, tx_coeff, 0, 0, 0)

    req = g3_hi_msg_get_g3lib_set_attrib_req(g3_hi_attrib_id.MAC_NEIGHBORTABLE_ID, index, sizeof(n_table), n_table)
    g3_hi_msg_send_message(net_info_ex, req)
    msg1 = g3_hi_msg_receive_message(net_info_ex)
    #TODO: check returned command and results

def g3_hi_adp_route_table_clear(net_info_ex, index):
    route_table = g3_hi_adp_routing_table_entry(0xFFFF, 0xFFFF, 0x0000, 0, 0, 0)

    req = g3_hi_msg_get_g3lib_set_attrib_req(g3_hi_attrib_id.ADP_ROUTING_TABLE, index, sizeof(route_table), route_table)
    g3_hi_msg_send_message(net_info_ex, req)
    msg1 = g3_hi_msg_receive_message(net_info_ex)
    #TODO: check returned command and results

def g3_hi_msg_print(msg):
    if isinstance(msg, Structure) or isinstance(msg, ctypes.Array):
        tmp = (c_ubyte * sizeof(msg))()
        memmove(byref(tmp), byref(msg), sizeof(msg))
    else:
        msg_len = len(msg)
        tmp = (c_ubyte * msg_len)()
        for i in range(0, msg_len):
            tmp[i] = msg[i]

    print("msg-len is", sizeof(tmp))
    print(''.join('{:02X} '.format(x) for x in tmp))

def g3_hi_debug_trace_log_clear(log_vector):
    def validate_trace(log, idx, length):
        ret = [False, 0, 0]
        first_byte_found = False
        len_parsed = False
        trace_data_cnt = 0
        for i in range(idx, idx + length):
            if (first_byte_found and not len_parsed):
                len_parsed = True
                trace_data_cnt = log[i]
            elif (first_byte_found and (trace_data_cnt > 0)):
                trace_data_cnt -= 1
            elif (0xC3 == log[i]):
                first_byte_found = True
                ret[1] = i
            elif (first_byte_found and (0xB4 == log[i])):
                return [True, ret[1], i + 1]
        ret[2] = idx + length
        return ret

    data_to_clear_len = len(log_vector)
    payload_cleared = []
    curr_idx = 0
    while (data_to_clear_len > 0):
        ret = validate_trace(log_vector, curr_idx, data_to_clear_len)
        if (ret[0]):
            payload_cleared += log_vector[ret[1]:ret[2]]
        data_to_clear_len -= (ret[2] - curr_idx)
        curr_idx = ret[2]
    return payload_cleared

def read_trace_log(net_info_ex, log_bin_stream, sector,
    log_len = g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE * g3_hi_sflash_define.SPI_FLASH_PAGES_IN_SECTOR_CNT):
    strace_log_sectors_range = [13, 22]
    log_last_address = (23 * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE) - 1
    address = sector * g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE
    total_payload = []

    if ((sector < strace_log_sectors_range[0]) or ((address + log_len) > log_last_address)):
        return False

    while (log_len > 0):
        curr_to_read_len = min(log_len, g3_hi_sflash_define.SPI_FLASH_PAGE_SIZE)
        req = g3_hi_msg_get_sflash_req(g3_hi_sflash_operation.READ, address, curr_to_read_len, None)
        g3_hi_msg_send_message(net_info_ex, req)
        msg = g3_hi_msg_receive_message(net_info_ex)
        read_msg_len = len(msg.payload)
        if ((address % g3_hi_sflash_define.SPI_FLASH_SECTOR_SIZE) == 0):
            payload = msg.payload[g3_hi_sflash_define.SPI_FLASH_DEBUG_TRACE_SECTOR_HEADER_LEN:]
        else:
            payload = msg.payload
        log_len -= read_msg_len
        address += read_msg_len
        total_payload += payload

    total_payload = g3_hi_debug_trace_log_clear(total_payload)
    log_bin_stream.write(bytes(total_payload))
    
    return True

def save_trace_log_to_file(net_info_ex, log_file_name, sector, log_len):
    with open(log_file_name, "wb") as f:
        ret = read_trace_log(net_info_ex, f, sector, log_len)
    return ret

