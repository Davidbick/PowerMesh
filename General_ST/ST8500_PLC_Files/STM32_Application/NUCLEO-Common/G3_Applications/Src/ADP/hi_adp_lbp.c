/*****************************************************************************
*   @file    hi_adp_lbp.c
*   @author  AMG/IPC Application Team
*   @brief   This file includes the functions used by the Bootstrap (ADP-LBP).
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
#include <string.h>
#include <stdlib.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <utils.h>
#include <hi_mac_sap_interface.h>
#include <hi_adp_sap_interface.h>
#include <hi_adp_lbp.h>
#include <hi_msgs_impl.h>

#include <settings.h>

/** @addtogroup G3_ADP
  * @{
  */

/** @addtogroup G3_ADP_LBP
  * @{
  */

/** @addtogroup G3_ADP_LBP_Private_Code
  * @{
  */

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST

/* Private Functions */

/**
 * @brief Function to convert the MediaType value to 1 bit (to be used in protocol message fields or tables)
 * @param [in] MediaType The original MediaType value
 * @return The corresponding MediaType bit value
 */
static uint8_t g3_adp_lbp_media_type_to_bit(const uint8_t media_type)
{
	uint8_t bit_value;

	if (media_type == MAC_MEDIATYPE_PLC)
	{
		bit_value = MAC_MEDIATYPE_PLC_RFbkp;
	}
	else if (media_type == MAC_MEDIATYPE_RF)
	{
		bit_value = MAC_MEDIATYPE_RF_PLCbkp;
	}
	else
	{
		bit_value = media_type;
	}

	return (bit_value & 0x01);
}

/**
 * @brief   Encode the LBP Accepted Message command
 * @param   [in] lbd_ext_addr Pointer to buffer that contains the Extended Address of the LBD
 * @param   [in] media_type Identifies the MediaType used for LBD – LBA communication (0x00 PLC, 0x01 RF)
 * @param   [in] disable_bkp Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled)
 * @param   [out] msg_buff The buffer on which the message is stored
 * @return  The length of the encoded message
 */
static uint16_t g3_adp_lbp_encode_accepted_message(const uint8_t *lbd_ext_addr, const uint8_t media_type, const uint8_t disable_bkp, uint8_t *msg_buff)
{
	lbp_msg_t* lbp_msg = (lbp_msg_t*) msg_buff;
	uint16_t lbp_msg_len = sizeof(lbp_header_t);

	memset(lbp_msg, 0, lbp_msg_len);

	// Start message encoding
	lbp_msg->header.reserved_1  = 0;
	lbp_msg->header.reserved_2  = 0;
	lbp_msg->header.code        = adp_lbs_accepted;
	lbp_msg->header.T           = adp_lbp_to_lbd;
	lbp_msg->header.media_type  = g3_adp_lbp_media_type_to_bit(media_type);
	lbp_msg->header.disable_bkp = disable_bkp;
	memcpy(lbp_msg->header.lbd_addr, lbd_ext_addr, MAC_ADDR64_SIZE);

	return lbp_msg_len;
}

/**
 * @brief   Encode the LBP Challenge Request command
 * @param   [in] A_LBD Pointer to buffer that contains the Extended Address of the LBD
 * @param   [in] media_type Identifies the MediaType used for LBD – LBA communication (0x00 PLC, 0x01 RF)
 * @param   [in] disable_bkp Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled)
 * @param   [out] msg_buff The buffer on which the message is stored
 * @return  The length of the encoded message
 */
static uint16_t g3_adp_lbp_encode_challange_request(const uint8_t *lbd_ext_addr, const uint8_t media_type, const uint8_t disable_bkp, uint8_t *msg_buff)
{
	lbp_msg_t* lbp_msg = (lbp_msg_t*) msg_buff;
	uint16_t lbp_msg_len = sizeof(lbp_header_t);

	memset(lbp_msg, 0, lbp_msg_len);

	// Start message encoding
	lbp_msg->header.reserved_1 = lbp_msg->header.reserved_2 = 0x00;
	lbp_msg->header.code       = adp_lbs_challange;
	lbp_msg->header.T          = adp_lbp_to_lbd;
	lbp_msg->header.media_type = g3_adp_lbp_media_type_to_bit(media_type);
	lbp_msg->header.disable_bkp = disable_bkp;
	memcpy(lbp_msg->header.lbd_addr, lbd_ext_addr, MAC_ADDR64_SIZE);

	return lbp_msg_len;
}

/**
 * @brief   Encode the LBP Decline Message command
 * @param   [in] lbd_ext_addr Pointer to buffer that contains the Extended Address of the LBD
 * @param   [in] media_type Identifies the MediaType used for LBD – LBA communication (0x00 PLC, 0x01 RF)
 * @param   [in] disable_bkp Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled)
 * @param   [out] msg_buff The buffer on which the message is stored
 * @return  The length of the encoded message
 */
static uint16_t g3_adp_lbp_encode_decline_message(const uint8_t *lbd_ext_addr, const uint8_t media_type, const uint8_t disable_bkp, uint8_t *msg_buff)
{
	lbp_msg_t* lbp_msg = (lbp_msg_t*) msg_buff;
	uint16_t lbp_msg_len = sizeof(lbp_header_t);

	memset(lbp_msg, 0, lbp_msg_len);

	// Start message encoding
	lbp_msg->header.reserved_1  = 0;
	lbp_msg->header.reserved_2  = 0;
	lbp_msg->header.code        = adp_lbs_decline;
	lbp_msg->header.T           = adp_lbp_to_lbd;
	lbp_msg->header.media_type  = g3_adp_lbp_media_type_to_bit(media_type);
	lbp_msg->header.disable_bkp = disable_bkp;
	memcpy(lbp_msg->header.lbd_addr, lbd_ext_addr, MAC_ADDR64_SIZE);

	return lbp_msg_len;
}

/**
 * @brief   Encode a KICK frame sent from the LBS to a PAN device
 * @param   [in] ext_addr The extended address of the PAN device to kick
 * @param   [out] msg_buff The buffer on which the message is stored
 * @return  The length of the encoded message
 */
static uint16_t g3_adp_lbp_encode_kick(const uint8_t *ext_addr, uint8_t *msg_buff)
{
	lbp_msg_t* lbp_msg = (lbp_msg_t*) msg_buff;
	uint16_t lbp_msg_len = sizeof(lbp_header_t);

	memset(lbp_msg, 0, lbp_msg_len);

	// Start message encoding
	lbp_msg->header.reserved_1 = lbp_msg->header.reserved_2 = 0x00;
	lbp_msg->header.code = adp_lbs_kick;
	lbp_msg->header.T = adp_lbp_to_lbd;
	memcpy(lbp_msg->header.lbd_addr, ext_addr, MAC_ADDR64_SIZE);

	return lbp_msg_len;
}

/**
 * @}
 */

/** @addtogroup G3_ADP_LBP_Exported_Code
  * @{
  */

/* Public Functions */

/**
 * @brief   Encodes the Configuration Parameters sent from the LBS to a node in EAP-PSK #3 message
 * @param   [out] ext_data The buffer on which the Configuration Parameters are going to be written
 * @param   [in] short_addr The network address assigned to the node (ignored when equal to MAC_BROADCAST_SHORT_ADDR)
 * @param   [in] gmk_0 Provides the first GMK to write (ignored when NULL)
 * @param   [in] gmk_1 Provides the second GMK to write (ignored when NULL)
 * @param   [in] gmk_index Indicates the index of the active GMK (ignored when equal to NO_ACTIVE_INDEX)
 * @return  The length of the Configuration Parameters Extension field (ext_type and ext_payload)
 */
uint16_t g3_adp_lbp_encode_3_params(uint8_t* ext_data, const uint16_t short_addr, const uint8_t* gmk_0, const uint8_t* gmk_1, const uint8_t gmk_index)
{
	bool valid = false;
	uint16_t ext_data_len = 0;
	uint16_t short_addr_param = SWAP_U16(short_addr);
	adp_gmk_param_value_t gmk_param;

	if (short_addr != MAC_BROADCAST_SHORT_ADDR)
	{
		/* Encode Short Address */
		ext_data_len += conf_param_encode(ADP_MSG3_PARAM_SHORT_ADDR_ID, conf_param_dsi, &short_addr_param, sizeof(short_addr), &ext_data[ext_data_len]);
	}

	if (gmk_0 != NULL)
	{
		/* Encode GMK at index 0, if not null */
		gmk_param.gmk_index = 0;
		memcpy(gmk_param.gmk, gmk_0, sizeof(gmk_param.gmk));

		ext_data_len += conf_param_encode(ADP_CONF_PARAM_GMK_ID, conf_param_psi, &gmk_param, sizeof(gmk_param), &ext_data[ext_data_len]);

		valid = true;
	}

	if (gmk_1 != NULL)
	{
		/* Encode GMK at index 1, if not null */
		gmk_param.gmk_index = 1;
		memcpy(gmk_param.gmk, gmk_1, sizeof(gmk_param.gmk));

		ext_data_len += conf_param_encode(ADP_CONF_PARAM_GMK_ID, conf_param_psi, &gmk_param, sizeof(gmk_param), &ext_data[ext_data_len]);

		valid = true;
	}

	assert(valid);

	if (gmk_index != NO_ACTIVE_INDEX)
	{
		/* Encode GMK Activation Index */
		ext_data_len += conf_param_encode(ADP_CONF_PARAM_GMK_ACTIVATION_ID, conf_param_psi, &gmk_index, sizeof(gmk_index), &ext_data[ext_data_len]);
	}

	return ext_data_len;
}

/**
 * @brief   Process the Configuration Parameters received from the Node in EAP-PSK #4 message
 * @param   [in] data The buffer containing the Configuration Parameters
 * @param   [in] data_length The length of the buffer containing the Configuration Parameters
 * @return  True if all the necessary parameters are received and correctly decoded, false otherwise
 */
bool g3_adp_lbp_decode_4_params(uint8_t* data, const uint16_t data_length)
{
	bool result = false;
	uint16_t offset = 0;
	conf_param_msg_t conf_param_msg;

	// Bootstrapping data carries the configuration parameters. Decode and process configuration parameters
	while (offset < data_length)
	{
		memcpy(&conf_param_msg.header, &data[offset], sizeof(conf_param_msg.header));
		offset += sizeof(conf_param_msg.header);

		memcpy(&conf_param_msg.value, &data[offset], conf_param_msg.header.length);
		offset += conf_param_msg.header.length;

		if (	(conf_param_msg.header.attr_id_type.is_cfg_par			) &&
				(conf_param_msg.header.attr_id_type.M == conf_param_dsi	) )
		{
			switch (conf_param_msg.header.attr_id_type.attr_id)
			{
			case ADP_CONF_PARAM_RESULT_ID:
				if (conf_param_msg.header.length == sizeof(conf_param_msg.value.param_result_param))
				{
					/* The result of the reception of the parameters */
					if (conf_param_msg.value.param_result_param.result == ADP_RESULT_PARAMETER_SUCCESS)
					{
						result = true;
					}
					else
					{
						/*Indicate the parameter related to the result, in case of error */
						PRINT_G3_BOOT_SRV_CRITICAL("Failed result in message #4: code %u for parameter %u\n",
													conf_param_msg.value.param_result_param.result,
													conf_param_msg.value.param_result_param.param_id.attr_id);
					}
				}
				break;
			default: // Unknown parameter received
				break;
			}
		}
	}

	return result;
}

/**
 * @brief   Sends EAP message #1 to the given joining entry
 * @param   [in/out] join_entry The joining entry pointer
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @param   [in] ids The ID of the server
 * @param   [in] ids_len The length of the ID of the server
 * @return  None
 */
void g3_adp_lbp_eap_send_1(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t *ids, const uint16_t ids_len)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);
	uint16_t nsdu_len;

	uint8_t rand_s[ADP_EAP_PSK_RAND_LEN];

	srand(HAL_GetTick());
	for (uint8_t i = 0; i < ADP_EAP_PSK_RAND_LEN; i++)
	{
		rand_s[i] = rand() & 0xFF;
	}

	/* Sets RandS  for the joining entry */
	memcpy(join_entry->eap_psk_data.rand_s, rand_s, ADP_EAP_PSK_RAND_LEN);

	/* Generate LBP Challenge Message */
	nsdu_len = g3_adp_lbp_encode_challange_request(join_entry->ext_addr, join_entry->media_type, join_entry->disable_bkp, &nsdu[0]);

	/* Encode EAP-PSK message #1 */
	nsdu_len += eap_psk_encode_1(ids, ids_len, &(join_entry->eap_psk_data), &nsdu[nsdu_len]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, join_entry->ext_addr, pan_id, join_entry->lba_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_INFO("Sent bootstrap #1 to %s\n", ext_addr_str);
#endif

	/* Always expected message #2 after message #1 */
	join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_SECOND;
}

/**
 * @brief   Sends EAP message #3 to the given joining entry
 * @param   [in/out] join_entry The joining entry pointer
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @param   [in] ids The ID of the server
 * @param   [in] ids_len The length of the ID of the server
 * @param   [in] short_address The short address to set as configuration parameter
 * @param   [in] gmk_0 The GMK at index 0
 * @param   [in] gmk_1 The GMK at index 1
 * @param   [in] gmk_index The index of the active GMK used after the re-keying process/for the bootstrap
 * @return  None
 */
void g3_adp_lbp_eap_send_3(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t *ids, const uint16_t ids_len, const uint16_t short_address, const uint8_t* gmk_0, const uint8_t* gmk_1, const uint8_t gmk_index)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);
	uint16_t nsdu_len;

	uint8_t ext_data[ADP_CONFIG_PARAMS_BUF_LEN];	/* Extension data*/
	uint16_t ext_data_len = 0;						/* Extension data length */

	/* Initialize TEK */
	eap_psk_initialize_tek(join_entry->eap_psk_data.rand_p, join_entry->eap_psk_data.psk_context);

	/* Encode the Configuration Parameters extension */
	ext_data_len = g3_adp_lbp_encode_3_params(ext_data, short_address, gmk_0, gmk_1, gmk_index);

	/* Increment the EAP Identifier to be used for the next Request */
	join_entry->eap_psk_data.eap_id++;

	/* Generate LBP Challenge Message */
	nsdu_len = g3_adp_lbp_encode_challange_request(join_entry->ext_addr, join_entry->media_type, join_entry->disable_bkp, &nsdu[0]);

	/* Encode EAP-PSK message #3 */
	nsdu_len += eap_psk_encode_3(&(join_entry->eap_psk_data), ids, ids_len, ext_data, ext_data_len, &nsdu[nsdu_len]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, join_entry->ext_addr, pan_id, join_entry->lba_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

	/* Increment the Nonce to be verified in the Node's Response */
	join_entry->eap_psk_data.nonce++;

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_INFO)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_INFO("Sent bootstrap #3 to %s\n", ext_addr_str);
#endif
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	if (short_address != MAC_BROADCAST_SHORT_ADDR)
	{
		PRINT_G3_BOOT_SRV_INFO("> Short address: %u\n", short_address);
	}

	if (gmk_0 != NULL)
	{
		ALLOC_STATIC_HEX_STRING(gmk_0_str, gmk_0, MAC_KEY_SIZE);
		PRINT_G3_BOOT_SRV_INFO("> GMK 0: %s\n", gmk_0_str);
	}

	if (gmk_1 != NULL)
	{
		ALLOC_STATIC_HEX_STRING(gmk_1_str, gmk_1, MAC_KEY_SIZE);
		PRINT_G3_BOOT_SRV_INFO("> GMK 1: %s\n", gmk_1_str);
	}

	if (gmk_index != NO_ACTIVE_INDEX)
	{
		PRINT_G3_BOOT_SRV_INFO("> Active GMK index: %u\n", gmk_index);
	}
#endif

	/* Message #4 is always expected after message #3 */
	join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_FOURTH;
}

/**
 * @brief   This function an LBP Accept message to the Node, to complete the bootstrap.
 * @param   [in/out] join_entry The data used by the server to complete the Joining procedure with the Node
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @return  None.
 */
void g3_adp_lbp_send_accept(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);

	// Generate LBP Accepted Message
	uint16_t nsdu_len = g3_adp_lbp_encode_accepted_message(join_entry->ext_addr, join_entry->media_type, join_entry->disable_bkp, &nsdu[0]);

	// Encode EAP Success Message
	nsdu_len += eap_encode_success(join_entry->eap_psk_data.eap_id, &nsdu[nsdu_len]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, join_entry->ext_addr, pan_id, join_entry->lba_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_INFO("Sent accept to %s\n", ext_addr_str);
#endif
}

/**
 * @brief   This function an LBP Decline message to the Node (because the node is blacklisted, or because authentication fails).
 * 			It also clears its Joining table entry.
 * @param   [in/out] join_entry The data used by the server to complete the Joining procedure with the Node
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @return  None.
 */
void g3_adp_lbp_send_decline(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);

	// Generate LBP Decline Message
	uint16_t nsdu_len = g3_adp_lbp_encode_decline_message(join_entry->ext_addr, join_entry->media_type, join_entry->disable_bkp, &nsdu[0]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, join_entry->ext_addr, pan_id, join_entry->lba_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_WARNING)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_WARNING("Sent decline to %s\n", ext_addr_str);
#endif
}

/**
 * @brief   This function an LBP Kick message to the Node, to kick it out of the PAN.
 * @param   [in] join_entry The data used by the server to complete the Joining procedure with the Node
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @return  None.
 */
void g3_adp_lbp_send_kick(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);

	// Generate KICK Message
	uint16_t nsdu_len = g3_adp_lbp_encode_kick(join_entry->ext_addr, &nsdu[0]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, NULL, pan_id, join_entry->short_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_INFO("Sent kick to short addr %u, ext addr %s\n", join_entry->short_addr, ext_addr_str);
#endif
}

/**
 * @brief   This function an LBP Configuration Parameter message to the Node, to set the GMK Activation parameter.
 * @param   [in] join_entry The data used by the server to complete the Joining procedure with the Node
 * @param   [in] pan_id The PAN ID of the network
 * @param   [in] handle The handle of the message
 * @param   [in] gmk_index The GMK active index parameter value to set
 * @return  None.
 */
void g3_adp_lbp_send_gmk_activation(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t gmk_index)
{
	ADP_AdpmLbpRequest_t *lbp_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmLbpRequest_t));
	uint8_t *nsdu = MEMPOOL_MALLOC(ADP_MAX_CTRL_PKT_SIZE);

	// Generate LBP Accepted Message
	uint16_t nsdu_len = g3_adp_lbp_encode_accepted_message(join_entry->ext_addr, join_entry->media_type, join_entry->disable_bkp, &nsdu[0]);

	// Encode GMK-activation Message
	nsdu_len += conf_param_encode(ADP_CONF_PARAM_GMK_ACTIVATION_ID, conf_param_psi, &gmk_index, sizeof(gmk_index), &nsdu[nsdu_len]);

	/* Send ADP LBP request */
	uint16_t len = hi_adp_lbp_fill(lbp_req, nsdu_len, nsdu, join_entry->ext_addr, pan_id, join_entry->lba_addr, join_entry->media_type, handle);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_LBP_REQ, lbp_req, len);

	MEMPOOL_FREE(nsdu);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
	PRINT_G3_BOOT_SRV_INFO("Sent GMK-activation (index=%u) to %s\n", gmk_index, ext_addr_str);
#endif
}

#endif /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/******************* (C) COPYRIGHT 2013 STMicroelectronics *******************/
