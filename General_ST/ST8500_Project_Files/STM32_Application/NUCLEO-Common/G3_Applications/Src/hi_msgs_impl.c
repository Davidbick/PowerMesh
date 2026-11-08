/*****************************************************************************
*   @file    hi_msgs_impl.c
*   @author  AMG/IPC Application Team
*   @brief   This code include functions that fill messages.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Inclusions */
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <hi_msgs_impl.h>
#include <hi_ip_sap_interface.h>
#include <hi_mac_sap_interface.h>
#include <hi_adp_lbp.h>
#include <hi_boot_sap_interface.h>
#include <hi_g3lib_sap_interface.h>
#include <hif_g3_common.h>
#include <g3_app_config_rf_params.h>
#include <g3_app_boot_constants.h>
#include <g3_app_boot_srv.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Msg_Def
  * @{
  */

/**
  * @brief  Function that returns the length of the MAC_DeviceAddress_t that is variable depending on the Mode field
  * @param  mac_address Pointer to the MAC_DeviceAddress_t structure.
  * @return Length of the MAC_DeviceAddress_t structure.
  */
static uint16_t hi_get_mac_addr_len(const MAC_DeviceAddress_t *mac_address)
{
    uint16_t len = sizeof(mac_address->addr_mode) + sizeof(mac_address->pan_id);

    if (2U == mac_address->addr_mode)
    {
        len += sizeof(mac_address->short_addr);
    }
    else if (3U == mac_address->addr_mode)
    {
        len += sizeof(mac_address->ext_addr);
    }

    return len;
}

/**
  * @brief  Function that captures the real size of the AdpMetaData structure (variable depending on BCO number).
  * @param  data_ind Pointer to the IP_DataIndication_t structure.
  * @return Length of the AdpMetaData structure.
  */
static uint16_t hi_ip_data_ind_get_meta_data_len(const IP_DataIndication_t *data_ind)
{
    const IP_AdpMetaData_t *metadata = &data_ind->meta_data;
    uint16_t len = sizeof(*metadata);

    len -= sizeof(metadata->previous_hop);

    len += hi_get_mac_addr_len(&metadata->previous_hop);

    len -= sizeof(metadata->bc0_list);
    len += sizeof(metadata->bc0_list[0]) * metadata->bc0_num;

    return len;
}

/**
  * @brief Builds the IPv6 address of a device from its PAN ID and its Short Address.
  * @param addr Pointer to the IPv6 to build.
  * @param pan_id PAN ID of the device.
  * @param short_addr Short Address of the device.
  * @retval None
  */
void hi_ipv6_set_ipaddr(ip6_addr_t *addr, const uint16_t pan_id, const uint16_t short_addr)
{
    uint8_t prefix[]       = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t interface_id[] = {0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x00};

    interface_id[0] = HIGH_BYTE(pan_id);
    interface_id[1] = LOW_BYTE(pan_id);
    interface_id[6] = HIGH_BYTE(short_addr);
    interface_id[7] = LOW_BYTE(short_addr);

    for (uint32_t i = 0; i < sizeof(prefix); i++)
    {
        addr->u8[i] = prefix[i];
    }

    for (uint32_t i = 0; i < sizeof(interface_id); i++)
    {
        addr->u8[i + sizeof(prefix)] = interface_id[i];
    }
}

/* Gets PAN ID and short address from a IPv6 address */

/**
  * @brief Builds the PAN ID and Short Address of a device from its IPv6 address.
  * @param ip6_addr IPv6 structure to decode.
  * @param pan_id Pointer to the variable where the PAN ID shall be saved.
  * @param short_addr Pointer to the variable where the Short Address shall be saved.
  * @retval None
  */
void hi_ipv6_get_saddr_panid(const ip6_addr_t ip6_addr, uint16_t *pan_id, uint16_t *short_addr)
{
    const uint16_t vec_elems_cnt = NUM_OF_ELEM(ip6_addr.u16);
    const uint16_t interface_idx = vec_elems_cnt / 2;

    if (pan_id != NULL)
    {
    	*pan_id     = IP6_HTONS(ip6_addr.u16[interface_idx]);
    }

    if (short_addr != NULL)
	{
    	*short_addr = IP6_HTONS(ip6_addr.u16[vec_elems_cnt - 1]);
	}
}

/**
  * @brief  Function that returns a pointer on the data in the G3ICMP-ECHOREQ.Indication
  * @param  data_ind Pointer to IP Data Indication_t structure
  * @return Pointer to the data G3ICMP-ECHOREQ.Indication payload
  */
IP_IcmpDataIndication_t *hi_ipv6_extract_icmp_from_ip(const IP_DataIndication_t *data_ind)
{
  return (IP_IcmpDataIndication_t*) (((uint8_t*) data_ind) + hi_ip_data_ind_get_meta_data_len(data_ind));
}

/**
  * @brief Function that returns a pointer on the data in the G3UDP-DATA.Indication message
  * @param data_ind Pointer to IP Data Indication_t structure.
  * @return Pointer to the data part of the G3UDP-DATA.Indication message.
  */
IP_UdpDataIndication_t *hi_ipv6_extract_udp_from_ip(const IP_DataIndication_t *data_ind)
{
    return (IP_UdpDataIndication_t*) (((uint8_t*) data_ind) + hi_ip_data_ind_get_meta_data_len(data_ind));
}

/* Requests payload preparation */

/* HOSTIF */

/**
  * @brief Prepares a HOSTIF-SFLASH.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param operation The type of operation.
  * @param size Size of the data to read or write.
  * @param address Address to read or write.
  * @param payload Pointer to the data to write.
  * @return The length of the message built.
  */
uint16_t hi_hostif_sflash_fill(void *msg_, flash_op_t operation, uint16_t size, uint32_t address, uint8_t *payload)
{
	hif_sflash_req_t *msg = msg_;

	msg->operation = operation;
	msg->size = size;
	msg->address = address;

	memcpy(msg->payload, payload, size);

	return (uint16_t) (sizeof(*msg) - sizeof(msg->payload) + size);
}

/**
  * @brief Prepares a HOSTIF-NVM.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param operation The type of operation.
  * @param section The section to erase/read/write.
  * @param offset Offset from the start of the section.
  * @param size Size of the data to read or write.
  * @param data Pointer to the data to write.
  * @return The length of the message built.
  */
uint16_t hi_hostif_nvm_fill(void *msg_, flash_op_t operation, nvm_section_t section, uint16_t offset, uint16_t size, uint8_t *data)
{
	hif_nvm_req_t *msg = msg_;

	msg->operation = operation;
	msg->section = section;
	msg->offset = offset;
	msg->size = size;

	if (data != NULL)
	{
		memcpy(msg->data, data, size);
	}

	return (uint16_t) (sizeof(*msg) - sizeof(msg->data) + size);
}

/**
  * @brief Prepares a HOSTIF-DBGTOOL.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param control The control field value.
  * @param info The info field value.
  * @return The length of the message built.
  */
uint16_t hi_hostif_dbgtoolreq_fill(void *msg_, uint8_t control, uint32_t info)
{
    hi_dbgtool_req_t *msg = msg_;

    msg->control = control;
    msg->info    = info;

    if (control == HI_TOOL_INFO)
    {
        return (uint16_t) (sizeof(*msg) - sizeof(msg->info));
    }
    else
    {
        return (uint16_t) sizeof(*msg);
    }
}

/**
  * @brief Prepares a HOSTIF-RFCONFIGSET.Request message inside a buffer
  * @param msg_ Pointer to the message buffer
  * @param base_frequency The base frequency to use
  * @param rf_type Type of the RF module
  * @return The length of the message built
  */
uint16_t hi_hostif_rfconfigsetreq_fill(void *msg_, uint32_t base_frequency, rf_type_t rf_type)
{
    hif_rfconfigset_req_t *msg = msg_;

    msg->s2lp_configdata.RadioBaseFreq = base_frequency;
    msg->s2lp_configdata.RadioModSelect = 0x00U;
    msg->s2lp_configdata.RadioDataRate = 50000U;
    msg->s2lp_configdata.RadioFreqDeviation = 12500U;
    msg->s2lp_configdata.RadioBandwidth = 100000U;
    msg->s2lp_configdata.RadioCsBlanking = 0x01U;
    msg->s2lp_configdata.RadioXtalFreq = 50000000U;
    msg->s2lp_configdata.RadioRssiGain = 14U;
    msg->s2lp_configdata.PktCRCMode = 0xA0U;
    msg->s2lp_configdata.PktEnFEC = 0x00U;
    msg->s2lp_configdata.PktEnWhitening = 0x01U;
    msg->s2lp_configdata.PktEnInterleaving = 0x01U;
    msg->s2lp_configdata.MCUClkEnable = 0x00U;
    msg->s2lp_configdata.MCUClkGpioPin = 0xFFU;
    msg->s2lp_configdata.MCUClkXORatio = 0xFFU;
    msg->s2lp_configdata.MCUClkRCORatio = 0xFFU;
    msg->s2lp_configdata.MCUClkClockCycles = 0xFFU;
    msg->s2lp_configdata.ExtSmpsEnable = 0x00U;
    msg->s2lp_configdata.FEMTxBypassEn = 0x00U;

    if (rf_type == RF_TYPE_915)
    {
        msg->s2lp_configdata.PowerdBm = RF_PARAM_915_POWER_DBM;
        msg->s2lp_configdata.IrqGpioPin = RF_PARAM_915_IRQ_GPIO_PIN;
        msg->s2lp_configdata.FEMEnabled = RF_PARAM_915_FEM_ENABLED;
        msg->s2lp_configdata.FEMGpioPinCSD = RF_PARAM_915_FEM_GPIO_PIN_CSD;
        msg->s2lp_configdata.FEMGpioPinCPS = RF_PARAM_915_FEM_GPIO_PIN_CPS;
        msg->s2lp_configdata.FEMGpioPinCTX = RF_PARAM_915_FEM_GPIO_PIN_CTX;
    }
    else
    {
        msg->s2lp_configdata.PowerdBm = RF_PARAM_868_POWER_DBM;
        msg->s2lp_configdata.IrqGpioPin = RF_PARAM_868_IRQ_GPIO_PIN;
        msg->s2lp_configdata.FEMEnabled = RF_PARAM_868_FEM_ENABLED;
        msg->s2lp_configdata.FEMGpioPinCSD = RF_PARAM_868_FEM_GPIO_PIN_CSD;
        msg->s2lp_configdata.FEMGpioPinCPS = RF_PARAM_868_FEM_GPIO_PIN_CPS;
        msg->s2lp_configdata.FEMGpioPinCTX = RF_PARAM_868_FEM_GPIO_PIN_CTX;
    }

    return (uint16_t) sizeof(*msg);
}

/* G3LIB */

/**
  * @brief Prepares a G3LIB-SWRESET.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param bandplan Type of bandplan to set.
  * @param device_type Type of device type to set.
  * @param mode Type of mode to set.
  * @param req_type Type of request type to set.
  * @return The length of the message built.
  */
uint16_t hi_g3lib_swresetreq_fill(void *msg_, BOOT_Bandplan_t bandplan, plcType_t device_type, plc_mode_t mode, BOOT_StartType_t req_type)
{
    G3_LIB_SWResetRequest_t *msg = msg_;

    msg->bandplan	 = (uint8_t) bandplan;
    msg->device_type = (uint8_t) device_type;
    msg->mode 		 = (uint8_t) mode;
    msg->req_type	 = (uint8_t) req_type;

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3LIB-GET.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param id The ID value of the attribute to get.
  * @param index The index value of the attribute to get.
  * @return The length of the message built.
  */
uint16_t hi_g3lib_getreq_fill(void *msg_, uint32_t id, uint16_t index)
{
    G3_LIB_GetAttributeRequest_t *msg = msg_;

    msg->attribute_id.id    = id;
    msg->attribute_id.index = index;

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3LIB-SET.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param id The ID value of the attribute to set.
  * @param index The index value of the attribute to set.
  * @param value The value of the attribute to set.
  * @param attr_len The length in bytes of the attribute to set.
  * @return The length of the message built.
  */
uint16_t hi_g3lib_setreq_fill(void *msg_, uint32_t id, uint16_t index, uint8_t* value, uint16_t attr_len)
{
    G3_LIB_SetAttributeRequest_t *msg = msg_;

    msg->attribute.attribute_id.id    = id;
    msg->attribute.attribute_id.index = index;
    msg->attribute.len                = attr_len;

    /* Must swap because the user uses MSB arrays/values,m while the request uses LSB arrays */
    memcpy(msg->attribute.value, value, attr_len);

    return (uint16_t) (sizeof(*msg) - sizeof(msg->attribute.value) + msg->attribute.len);
}

/* MAC */

/**
  * @brief Prepares a G3MAC-DATA.Request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @return The length of the message built.
  */
uint16_t hi_mac_data_fill(	void *msg_,
							const mac_addr_mode_t src_addr_mode,
							const uint16_t pan_id,
							const uint16_t dst_short_addr,
							const uint8_t *dst_ext_addr,
							const uint16_t msdu_len,
							const uint8_t* msdu,
							const uint8_t handle,
							const mac_tx_options_t tx_options,
							const mac_key_index_t key_index,
							const mac_mediatype_req_t media_type)
{
	uint16_t dst_addr_len;
	uint16_t key_index_len;
	uint32_t offset = 0;
	MAC_DataReq_t *msg = msg_;

	msg->src_addr_mode = src_addr_mode;

	msg->dst_addr.pan_id = pan_id;

	if (dst_short_addr != MAC_BROADCAST_SHORT_ADDR)
	{
		dst_addr_len = sizeof(msg->dst_addr.short_addr);
		msg->dst_addr.addr_mode = MAC_ADDR_MODE_16;
		msg->dst_addr.short_addr = dst_short_addr;

		offset += sizeof(msg->dst_addr.ext_addr) - dst_addr_len;  /* Shorter destination address field (2 bytes instead of 8) */
	}
	else
	{
		dst_addr_len = sizeof(msg->dst_addr.ext_addr);
		msg->dst_addr.addr_mode = MAC_ADDR_MODE_64;
		memcpy(msg->dst_addr.ext_addr, dst_ext_addr, MAC_ADDR64_SIZE);

		/* offset does not need to be changed */
	}

	VAR_SIZE_PAYLOAD_OFFSET(msg->msdu_len,   offset    ) = msdu_len & 0xFF;
	VAR_SIZE_PAYLOAD_OFFSET(msg->msdu_len,   (offset-1)) = (msdu_len >> 8) & 0xFF;

	if ((msdu_len != 0) && (msdu != NULL))
	{
		memcpy(msg->msdu - offset, msdu, msdu_len);
	}

	offset += sizeof(msg->msdu) - msdu_len;

	VAR_SIZE_PAYLOAD_OFFSET(msg->msdu_handle,    offset) = handle;
	VAR_SIZE_PAYLOAD_OFFSET(msg->tx_options,     offset) = tx_options;

	if ((key_index == 0) || (key_index == 1))
	{
		key_index_len = sizeof(msg->key_index);
		VAR_SIZE_PAYLOAD_OFFSET(msg->security_level, offset) = MAC_SECURITY_LEVEL_5_ENCMIC32;
		VAR_SIZE_PAYLOAD_OFFSET(msg->key_index,      offset) = 0;

		/* offset does not need to be changed */
	}
	else
	{
		key_index_len = 0;
		VAR_SIZE_PAYLOAD_OFFSET(msg->security_level, offset) = MAC_SECURITY_LEVEL_0;

		offset += sizeof(msg->key_index); /* No key index field */
	}

	VAR_SIZE_PAYLOAD_OFFSET(msg->qos,            offset) = MAC_CHANNEL_PRIORITY_NORMAL;
	VAR_SIZE_PAYLOAD_OFFSET(msg->media_type,     offset) = media_type;

	return (uint16_t) (sizeof(*msg) - sizeof(msg->dst_addr.ext_addr) + dst_addr_len - sizeof(msg->msdu) + msdu_len - sizeof(msg->key_index) + key_index_len);
}

#if (IS_COORD && ENABLE_BOOT_SERVER_ON_HOST) || (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST)

/* ADP */

/**
  * @brief Prepares a G3ADPM-DISCOVERY request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param duration The duration of the discovery phase.
  * @return The length of the message built.
  */
uint16_t hi_adp_discovery_fill(void *msg_, uint8_t duration)
{
	ADP_AdpmDiscoveryRequest_t *msg = msg_;

	msg->duration = duration;

    return (uint16_t) sizeof(*msg);
}

#endif /* (IS_COORD && ENABLE_BOOT_SERVER_ON_HOST) || (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST) */

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST

/**
  * @brief Prepares a G3ADPM-NETWORK-START request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param pan_id The PAN ID of the network to start.
  * @return The length of the message built.
  */
uint16_t hi_adp_network_start_fill(void *msg_, uint16_t pan_id)
{
	ADP_AdpmNetworkStartRequest_t *msg = msg_;

	msg->pan_id = pan_id;

    return (uint16_t) sizeof(*msg);
}

#endif /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST */

#if !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST

/**
  * @brief Prepares a G3ADPM-NETWORK-JOIN request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param pan_id The PAN ID of the network to join.
  * @param lba_short_addr The short address of the LowPAN Bootstrap Actor (LBA).
  * @param media_type The type of medium.
  * @return The length of the message built.
  */
uint16_t hi_adp_network_join_fill(void *msg_, const ADP_PanDescriptor_t *pan_descriptor)
{
	ADP_AdpmNetworkJoinRequest_t *msg = msg_;

	msg->pan_id		= pan_descriptor->pan_id;
	msg->lba_addr	= pan_descriptor->lba_addr;
	msg->media_type	= pan_descriptor->media_type;
#if COMPILE_7_X_VERSION
	msg->FH_LBAScheduleStartTime = pan_descriptor->FH_ScheduleStartTime;
#endif /* COMPILE_7_X_VERSION */

    return (uint16_t) sizeof(*msg);
}

#endif /* !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST */

#if (IS_COORD && ENABLE_BOOT_SERVER_ON_HOST) || (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST)

/**
  * @brief Prepares a G3ADP-LBP request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param nsdu_len Length of the NSDU.
  * @param nsdu Pointer to the NSDU buffer.
  * @param lbd_ext_addr The extended address of the LowPAN Bootstrap Device.
  * @param pan_id ID of the PAN.
  * @param lba_short_addr The short address of the LowPAN Bootstrap Actor.
  * @param media_type The type of medium.
  * @param handle Handle value of the message.
  * @return The length of the message built.
  */
uint16_t hi_adp_lbp_fill(	void *msg_,
							const uint16_t nsdu_len,
							const uint8_t* nsdu,
							const uint8_t* lbd_ext_addr,
							const uint16_t pan_id,
							const uint16_t lba_short_addr,
							const adp_mediatype_t media_type,
							const uint8_t handle)
{
	uint16_t dst_addr_len;
	uint32_t offset = 0;
	ADP_AdpmLbpRequest_t *msg = msg_;

	msg->dst_addr.pan_id = pan_id;

	if (lba_short_addr != MAC_BROADCAST_SHORT_ADDR)
	{
		// The LBP message is relayed through an LBA
		dst_addr_len = sizeof(msg->dst_addr.short_addr);
		msg->dst_addr.addr_mode = MAC_ADDR_MODE_16;
		msg->dst_addr.short_addr = lba_short_addr;

		offset += sizeof(msg->dst_addr.ext_addr) - dst_addr_len;
	}
	else
	{
		// The LBP message is directly sent to the LBD
		dst_addr_len = sizeof(msg->dst_addr.ext_addr);
		msg->dst_addr.addr_mode = MAC_ADDR_MODE_64;
		memcpy(msg->dst_addr.ext_addr, lbd_ext_addr, MAC_ADDR64_SIZE);
		utils_reverse_array(msg->dst_addr.ext_addr, MAC_ADDR64_SIZE);

		/*foot_offset does not need to be changed */
	}

	VAR_SIZE_PAYLOAD_OFFSET(msg->nsdu_len,   offset    ) = nsdu_len & 0xFF;
	VAR_SIZE_PAYLOAD_OFFSET(msg->nsdu_len,   (offset-1)) = (nsdu_len >> 8) & 0xFF;

	if ((nsdu_len != 0) && (nsdu != NULL))
	{
		memcpy(msg->nsdu - offset, nsdu, nsdu_len);
	}

	offset += sizeof(msg->nsdu) - nsdu_len;

	VAR_SIZE_PAYLOAD_OFFSET(msg->nsdu_handle, offset) = handle;
#if IS_COORD
	VAR_SIZE_PAYLOAD_OFFSET(msg->max_hops,    offset) = BOOT_SERVER_DEFAULT_MAX_HOPS;
#else
	VAR_SIZE_PAYLOAD_OFFSET(msg->max_hops,    offset) = BOOT_CLIENT_DEFAULT_MAX_HOPS;
#endif /* IS COORD */

	if (lba_short_addr != MAC_BROADCAST_SHORT_ADDR)
	{
		VAR_SIZE_PAYLOAD_OFFSET(msg->discover_route,   offset) = ADP_DISCOVERY_ROUTE_TRUE;
		VAR_SIZE_PAYLOAD_OFFSET(msg->security_enabled, offset) = ADP_SECURIY_ENABLED;
		VAR_SIZE_PAYLOAD_OFFSET(msg->media_type,       offset) = ADP_MEDIATYPE_PLC;
	}
	else
	{
		VAR_SIZE_PAYLOAD_OFFSET(msg->discover_route,   offset) = ADP_DISCOVERY_ROUTE_FALSE;
		VAR_SIZE_PAYLOAD_OFFSET(msg->security_enabled, offset) = ADP_SECURIY_DISABLED;
		VAR_SIZE_PAYLOAD_OFFSET(msg->media_type,       offset) = media_type;
	}

	VAR_SIZE_PAYLOAD_OFFSET(msg->qos,         offset) = ADP_QOS_STANDARD_PRIORITY;

	return (uint16_t) (sizeof(*msg) - sizeof(msg->dst_addr.ext_addr) + dst_addr_len - sizeof(msg->nsdu) + nsdu_len);
}

#endif /* (IS_COORD && ENABLE_BOOT_SERVER_ON_HOST) || (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST) */

#if IS_COORD
/**
  * @brief Prepares a G3BOOT-SRV-START request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param type The type of the requested network start.
  * @param pan_id The ID of the PAN to start.
  * @param short_addr The short address of the server.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvstartreq_fill(void *msg_, BOOT_StartType_t type, uint16_t pan_id, uint16_t short_addr)
{
	BOOT_ServerStartRequest_t *msg = msg_;

	/* for coordinator: starts server */
	msg->req_type    = type;
	msg->pan_id      = pan_id;
	msg->server_addr = short_addr;

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-SRV-KICK request message inside a buffer.
  * @param msg_   Pointer to the message buffer.
  * @param device Pointer to the boot_device_t of the device to kick.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvkickreq_fill(void *msg_, boot_device_t *device)
{
    BOOT_ServerKickRequest_t *msg = msg_;

    msg->short_addr = device->short_addr;
    memcpy(msg->ext_addr, device->ext_addr, sizeof(msg->ext_addr));

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-SRV-SETPSK request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param ext_addr The extended address of the device.
  * @param psk Pointer to the array containing the PSK to associate with the given extended address.
  * @param short_addr The short address to assign to the device with the given extended address.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvsetpskreq_fill(void *msg_, const uint8_t *ext_addr, const uint8_t *psk, const uint16_t short_addr)
{
    BOOT_ServerSetPSKRequest_t *msg = msg_;

    assert(msg_ != NULL);
    assert(ext_addr != NULL);
    assert(psk != NULL);

    memcpy(msg->ext_addr, ext_addr, sizeof(msg->ext_addr));
    memcpy(msg->psk, psk, sizeof(msg->psk));
    msg->short_addr = short_addr;

    return (uint16_t) sizeof(*msg);
}

#if ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Prepares a G3BOOT-SRV-JOIN indication message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param join Pointer to the boot_join_entry_t of the device to join.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvjoinind_fill(void *msg_, const boot_join_entry_t *join)
{
	BOOT_ServerJoinIndication_t *msg = msg_;

    msg->short_addr = join->short_addr;
    memcpy(msg->ext_addr, join->ext_addr, sizeof(msg->ext_addr));

    return (uint16_t) sizeof(*msg);
}


/**
  * @brief Prepares a G3BOOT-SRV-LEAVE indication message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param ext_addr The extended address of the device leaving the network.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvleaveind_fill(void *msg_, const uint8_t *ext_addr)
{
	BOOT_ServerLeaveIndication_t *msg = msg_;

    memcpy(msg->ext_addr, ext_addr, sizeof(msg->ext_addr));

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-SRV-REKEYING request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param gmk Pointer to the array containing the GMK to use for the update.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvrekeyingreq_fill(void *msg_, const uint8_t *gmk)
{
	BOOT_ServerRekeyingRequest_t *msg = msg_;

    memcpy(msg->gmk, gmk, sizeof(msg->gmk));

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-SRV-REKEYING confirm message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param status Result of the operation.
  * @param error Error returned by the operation.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvrekeyingcnf_fill(void *msg_, const uint8_t status, const uint8_t error)
{
	BOOT_ServerRekeyingConfirm_t *msg = msg_;

	msg->status = status;
	msg->error  = error;

    return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-SRV-GETPSK indication message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param join Pointer to the boot_join_entry_t of the device to join.
  * @return The length of the message built.
  */
uint16_t hi_boot_srvgetpskind_fill(void *msg_, const boot_join_entry_t *join_entry)
{
	BOOT_ServerGetPSKIndication_t *msg = msg_;

    memcpy(msg->ext_addr, join_entry->ext_addr, sizeof(msg->ext_addr));
    msg->idp_len = join_entry->eap_psk_data.id_p_len;
    memcpy(msg->idp, join_entry->eap_psk_data.id_p, join_entry->eap_psk_data.id_p_len);

    return (uint16_t) sizeof(*msg);
}

#endif /* ENABLE_BOOT_SERVER_ON_HOST */

#else

/**
  * @brief Prepares a G3BOOT-DEV-START request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param start_type The type of requested start.
  * @return The length of the message built.
  */
uint16_t hi_boot_devstartreq_fill(void *msg_, BOOT_StartType_t start_type)
{
    BOOT_DeviceStartRequest_t *msg = msg_;

    msg->req_type = start_type;

    return (uint16_t) sizeof(*msg);
}

/* G3BOOT-DEV-LEAVE has no payload */

/**
  * @brief Prepares a G3BOOT-DEV-PANSORT request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param pansort_ind Pointer to the PANSortIndication structure.
  * @return The length of the message built.
  */
uint16_t hi_boot_devpansortreq_fill(void *msg_, const BOOT_DevicePANSortIndication_t *pansort_ind)
{
    BOOT_DevicePANSortRequest_t *msg = msg_;

    msg->pan_count = pansort_ind->pan_count;

    memcpy(msg->pan_descriptor, pansort_ind->pan_descriptor, sizeof(msg->pan_descriptor[0]) * msg->pan_count);

    return (uint16_t) (sizeof(msg->pan_count) + (sizeof(msg->pan_descriptor[0]) * msg->pan_count));
}

#if ENABLE_BOOT_CLIENT_ON_HOST

/**
  * @brief Prepares a G3BOOT-DEV-START confirm message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param status The result of the operation.
  * @param network_addr The short address of the connection.
  * @param pan_id The PAN ID of the connection.
  * @return The length of the message built.
  */
uint16_t hi_boot_devstartcnf_fill(void *msg_, g3_result_t status, uint16_t network_addr, uint16_t pan_id)
{
	BOOT_DeviceStartConfirm_t *msg = msg_;

	msg->status = status;
	msg->network_addr = network_addr;
	msg->pan_id = pan_id;

	return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-DEV-LEAVE confirm message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param status The result of the operation.
  * @return The length of the message built.
  */
uint16_t hi_boot_devleavecnf_fill(void *msg_, g3_result_t status)
{
	BOOT_DeviceLeaveConfirm_t *msg = msg_;

	msg->status = status;

	return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3BOOT-DEV-PANSORT indication message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param discovery_cnf Pointer to the ADP_AdpmDiscoveryConfirm_t structure.
  * @return The length of the message built.
  */
uint16_t hi_boot_devpansortind_fill(void *msg_, const ADP_AdpmDiscoveryConfirm_t *discovery_cnf)
{
	BOOT_DevicePANSortIndication_t *msg = msg_;

    msg->pan_count = discovery_cnf->pan_count;

    memcpy(msg->pan_descriptor, discovery_cnf->pan_descriptor, sizeof(msg->pan_descriptor[0]) * msg->pan_count);

    return (uint16_t) (sizeof(msg->pan_count) + (sizeof(msg->pan_descriptor[0]) * msg->pan_count));
}

#if BOOT_CLIENT_DISCOVER_ROUTE
/**
  * @brief Prepares a G3ADP-ROUTE-DISCOVERY request message inside a buffer
  * @param msg_ Pointer to the message buffer
  * @param dst_addr The destination short address
  * @param max_hops The maximum number of hops
  * @param next_hop The next hop short address
  * @param media_type The type of medium
  * @return The length of the message built
  */
uint16_t hi_adp_routediscovery_fill(void *msg_, uint16_t dst_addr, uint8_t max_hops, uint16_t next_hop, uint8_t media_type)
{
	ADP_AdpdRouteDiscoveryRequest_t *msg = msg_;

	msg->dst_addr   = dst_addr;
	msg->max_hops   = max_hops;
	msg->next_hop   = next_hop;
	msg->media_type = media_type;

    return (uint16_t) sizeof(*msg);
}
#endif /* BOOT_CLIENT_DISCOVER_ROUTE */

#endif /* ENABLE_BOOT_CLIENT_ON_HOST */

#endif /* IS_COORD */

/* ICMP */

/**
  * @brief Prepares a G3ICMP-ECHO request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param dst_addr The pointer to the destination IPv6 address.
  * @param handle A user-defined value for the request identification.
  * @param data_len The number of echo payload bytes.
  * @param data The pointer to the echo payload.
  * @return The length of the message built.
  */
uint16_t hi_ipv6_echoreq_fill(void *msg_, const ip6_addr_t dst_addr, uint8_t handle, uint16_t data_len, const uint8_t *data)
{
    IP_G3IcmpDataRequest_t *msg = (IP_G3IcmpDataRequest_t *)msg_;

    msg->dest_address = dst_addr;
    msg->handle = handle;
    msg->data_len = data_len;

    if (data_len <= sizeof(msg->data))
    {
        memcpy(msg->data, data, data_len);
    }
    else
    {
    	Error_Handler(); /* data_len value too high */
    }

    return (uint16_t) G3_ICMP_DATA_REQ_CURR_LEN(msg);
}

/* UDP */

/**
  * @brief Prepares a G3UDP-CONN-SET request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param conn_id The ID value of the connection to set (0x0-0xA).
  * @param remote_addr The remote IPv6 address. The connection shall accept packets only from this address. Use 00..00 to receive from any IPv6 address.
  * @param remote_port The value of the remote UDP port. The connection shall accept packets only from this port. Use 0 to receive from any port.
  * @param local_port The value of the local UDP port.
  * @return The length of the message built.
  */
uint16_t hi_ipv6_udpconnsetreq_fill(void *msg_, uint8_t conn_id, const ip6_addr_t remote_addr, const uint16_t remote_port, const uint16_t local_port)
{
    IP_UdpConnSetRequest_t *msg = msg_;

    msg->connection_id  = conn_id;
    msg->remote_address = remote_addr;
    msg->remote_port    = remote_port;
    msg->local_port     = local_port;
    
    return (uint16_t) sizeof(*msg); //returns the frame size (IP_UdpConnSetRequest_t size)
}

/**
  * @brief Prepares a G3UDP-CONN-GET request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param conn_id The ID value of the connection to get (0x0-0xA).
  * @return The length of the message built.
  */
uint16_t hi_ipv6_udpconngetreq_fill(void *msg_, uint8_t conn_id)
{
  IP_UdpConnGetRequest_t *msg = msg_;

   msg->connection_id = conn_id;

   return (uint16_t) sizeof(*msg);
}

/**
  * @brief Prepares a G3UDP-DATA request message inside a buffer.
  * @param msg_ Pointer to the message buffer.
  * @param conn_id The ID value of the connection to use (0x0-0xA). Shall use its local port as source port.
  * @param dst_addr The pointer to the destination IPv6 address.
  * @param handle A user-defined value for the request identification.
  * @param data_len The number of echo payload bytes.
  * @param data The pointer to the echo payload.
  * @return The length of the message built.
  */
uint16_t hi_ipv6_udpdatareq_fill(void *msg_, const uint8_t conn_id, const ip6_addr_t dst_addr, const uint8_t handle, const uint16_t dst_port, const uint16_t data_len, const void *data)
{
    IP_G3UdpDataRequest_t *msg = msg_;

    msg->handle        = handle;
    msg->connection_id = conn_id;
    msg->dest_address  = dst_addr;
    msg->dest_port     = dst_port;
    msg->data_len      = data_len;

    if (data_len <= sizeof(msg->data))
    {
        memcpy(msg->data, data, data_len);
    }
    else
    {
    	Error_Handler(); /* data_len value too high */
    }

    return (uint16_t) G3_UDP_DATA_REQ_CURR_LEN(msg);
}

/**
  * @}
  */

/**
  * @}
  */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
