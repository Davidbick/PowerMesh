/*****************************************************************************
*   @file    hi_adp_eap_psk.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains RFC4764 (The EAP-PSK Protocol: A Pre-Shared Key Extensible Authentication Protocol (EAP) Method)
*            implementation related to G3 needs
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
#include <cmsis_os.h>
#include <mem_pool.h>
#include <debug_print.h>
#include <hi_adp_eap_psk.h>
#include <utils.h>

/** @addtogroup EAP_PSK
  * @{
  */

/** @addtogroup EAP_PSK_Private_Code
  * @{
  */

/**
 * @brief   Computes MacS or MacP
 * @param   [out] mac The computed MAC
 * @param   [in] ak The Authentication Key
 * @param   [in] arg1 The first element for the computation
 * @param   [in] arg1_len The length of the first element
 * @param   [in] arg2 The second element for the computation
 * @param   [in] arg2_len The length of the second element
 * @param   [in] arg3 The third element for the computation (only for MacP)
 * @param   [in] arg3_len The length of the third element (only for MacP)
 * @param   [in] arg4 The fourth element for the computation (only for MacP)
 * @param   [in] arg4_len The length of the fourth element (only for MacP)
 */
static void eap_psk_compute_mac(uint8_t *mac,     const uint8_t *ak,
								const void* arg1, const uint8_t arg1_len,
								const void* arg2, const uint8_t arg2_len,
								const void* arg3, const uint8_t arg3_len,
								const void* arg4, const uint8_t arg4_len)
{
	/* Computation of MacP */
	uint8_t seed[ADP_EAP_MACP_SEED_MAX_LEN];
	uint8_t seed_len = 0;
	cmac_ctx ctx[1];

	// Compute Mac
	memcpy(&seed[0], arg1, arg1_len);
	seed_len += arg1_len;

	memcpy(&seed[seed_len], arg2, arg2_len);
	seed_len += arg2_len;

	if ((arg3 != NULL) && (arg3_len != 0))
	{
		memcpy(&seed[seed_len], arg3, arg3_len);
		seed_len += arg3_len;
	}

	if ((arg4 != NULL) && (arg4_len != 0))
	{
		memcpy(&seed[seed_len], arg4, arg4_len);
		seed_len += arg4_len;
	}


	cmac_init(ak, ADP_EAP_PSK_KEY_LEN, ctx);
	cmac_data(seed, seed_len, ctx);
	cmac_end(mac, ctx);
}

/**
 * @brief   Encrypts the data in the Protected Channel
 * @param   [out] channel The protected channel to encrypt (data must be already inside)
 * @param   [in] tek The Transient Encryption Key to use
 * @param   [in] header The header of the EAP-PSK message (EAP header + EAP-PSK header))
 * @param   [in] eax_nonce The Nonce to use (16 bytes)
 * @param   [in] extension_len The length of the extension
 */
static bool eap_psk_encrypt_channel(eap_psk_channel_t *channel,
									const uint8_t *tek,
									const eap_header_t *header,
									const uint8_t *eax_nonce,
									const uint16_t extension_len)
{
	int32_t result;
	eax_ctx eax_ctx[1]; 								/* EAX context */
	uint8_t auth_header[ADP_EAP_PSK_AUTH_HEADER_LEN];	/* Allocate another buffer as we have to modify the EAP packet */

	/* Initializes EAX context with the TEK key */
	result = eax_init_and_key(tek, ADP_EAP_PSK_KEY_LEN, eax_ctx);

	if (result == RETURN_GOOD)
	{
		/* Points to protected data */
		uint8_t  *protected_data    = (uint8_t*) &(channel->E_R); /* Starting from E-R flags byte */

		/* Protected data length = size of the E-R flag byte + extension length */
		uint16_t protected_data_len = sizeof(channel->E_R) + extension_len;

		/* Prepare Authentication Header and right shift Code field with 2 bits as indicated in the EAP specification */
		memcpy(auth_header, header, ADP_EAP_PSK_AUTH_HEADER_LEN);
		auth_header[0] >>= 2;

		/* Encrypts data in the Protected Channel (E-R flags + Extension). Fills the tag field */
		result = eax_encrypt_message(eax_nonce,                      		/* the initialization vector (input)  */
									 ADP_EAP_PSK_EAX_NONCE_LEN,      		/* and its length in bytes            */
									 auth_header,                    		/* the authentication header (input)  */
									 sizeof(auth_header),    				/* and its length in bytes            */
									 protected_data,                 		/* the data to encrypt (input)        */
									 protected_data_len,             		/* and its length in bytes            */
									 channel->tag,							/* the buffer for the tag (output)    */
									 sizeof(channel->tag),					/* and its length in bytes            */
									 eax_ctx);                       		/* the mode context  (input)          */
	}

	eax_end(eax_ctx);

	return (result == RETURN_GOOD);
}

static bool eap_psk_decrypt_channel(const eap_psk_channel_t *channel,
									const uint8_t *tek,
									const eap_header_t *header,
									const uint8_t *eax_nonce,
									const uint16_t protected_data_len)
{
	int32_t result;
	eax_ctx eax_ctx[1]; 								/* EAX context */
	uint8_t auth_header[ADP_EAP_PSK_AUTH_HEADER_LEN];	/* Allocate another buffer as we have to modify the EAP packet */

	/* Initializes EAX context with the TEK key */
	result = eax_init_and_key(tek, ADP_EAP_PSK_KEY_LEN, eax_ctx);

	// Initialize TEK key (used to decode P-CHANNEL)
	if (result == RETURN_GOOD)
	{
		/* Points to protected data */
		uint8_t  *protected_data    = (uint8_t*) &(channel->E_R); /* Starting from E-R flags byte */

		/* Prepare Authentication Header and right shift Code field with 2 bits as indicated in the EAP specification */
		memcpy(auth_header, header, ADP_EAP_PSK_AUTH_HEADER_LEN);
		auth_header[0] >>= 2;

		/* RETURN_GOOD is returned if the input tag matches that for the decrypted message */
		result = eax_decrypt_message(eax_nonce,                   /* the initialization vector    */
									 ADP_EAP_PSK_EAX_NONCE_LEN,   /* and its length in bytes      */
									 auth_header,                 /* the header buffer            */
									 ADP_EAP_PSK_AUTH_HEADER_LEN, /* and its length in bytes      */
									 protected_data,              /* the message buffer           */
									 protected_data_len,          /* and its length in bytes      */
									 channel->tag,        		  /* the buffer for the tag       */
									 ADP_EAP_PSK_TAG_LEN,         /* and its length in bytes      */
									 eax_ctx);                    /* the mode context             */
	}

	eax_end(eax_ctx);

	return (result == RETURN_GOOD);
}

/**
 * @}
 */

/** @addtogroup EAP_PSK_Exported_Code
  * @{
  */

/**
 * @brief   Initialize the EAP-PSK cryptography library, by generating the Key-Derivation Key (KDK) from the PSK.
 * @param   [in] PSK The PSK of the device
 * @param   [out] The AES encryption context
 */
void eap_psk_initialize_psk(const uint8_t psk[ADP_EAP_PSK_KEY_LEN], eap_psk_context_t psk_context[1])
{
	aes_encrypt_ctx aesCtx[1];
	uint8_t au8Block[16] = {0};
	uint8_t au8Res[16] = {0};

	memset(psk_context, 0, sizeof(psk_context[0]));
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(psk_str, psk, ADP_EAP_PSK_KEY_LEN);
	PRINT_G3_BOOT_SRV_INFO("Initialized PSK: %s\n", psk_str);
#endif
	// initialize the AES context
	aes_encrypt_key128(psk, aesCtx);

	aes_encrypt(au8Block, au8Res, aesCtx);

	// xor with c1 = "1"
	au8Res[15] ^= 0x01;

	// generate AK
	aes_encrypt(au8Res, psk_context[0].m_au8_ak, aesCtx);

	// xor with c1 = "2"
	au8Res[15] ^= 0x03; // 3 instead of 2 because it has been already xor'ed with 1 and we want to get back the initial value

	// generate KDK
	aes_encrypt(au8Res, psk_context[0].m_au8_kdk, aesCtx);
}


/**
 * @brief   Generates the EAP-PSK Transient Encryption Key (TEK) from the KDK. Called after "eap_psk_initialize_psk".
 * @param   [in] rand_p 16-byte random challenge generated by the device (the peer)
 * @param   [out] The AES encryption context (psk_context)
 */
void eap_psk_initialize_tek(const uint8_t rand_p[ADP_EAP_PSK_RAND_LEN], eap_psk_context_t psk_context[1])
{
	aes_encrypt_ctx aes_ctx[1];
	uint8_t au8_res[ADP_EAP_PSK_KEY_LEN] = { 0 };

	// initialize the AES context
	aes_encrypt_key128(psk_context[0].m_au8_kdk, aes_ctx);

	aes_encrypt(rand_p, au8_res, aes_ctx);

	// xor with c1 = "1"
	au8_res[15] ^= 0x01;

	// generate TEK
	aes_encrypt(au8_res, psk_context[0].m_au8_tek, aes_ctx);
}

/**
 * @brief   It is used to encode the EAP Success message
 * @param   [in] eap_identifier The EAP Identifier to be sent within the EAP success message
 * @param   [out] msg_buff The buffer on which the EAP-PSK message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t eap_encode_success(const uint8_t eap_identifier, uint8_t* msg_buff)
{
	eap_header_t* eap_header = (eap_header_t*) msg_buff;
	const uint16_t eap_header_len = sizeof(eap_header_t);
	memset(eap_header, 0, eap_header_len);

	// Set EAP header
	eap_header->code   = adp_eap_success;
	eap_header->id     = eap_identifier;
	eap_header->length = SWAP_U16(eap_header_len);

	return eap_header_len;
}

/**
 * @brief   It is used to encode the Configuration Parameter message
 * @param   [in] attr_id The attribute ID of the configuration parameter
 * @param   [in] type The type of the attribute: DSI (0) or PSI (1)
 * @param   [in] value The value of the attribute
 * @param   [in] value_len The length of the attribute
 * @param   [out] msg_buff The buffer on which the configuration parameter message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t conf_param_encode(const uint8_t attr_id, uint8_t type, const void* value, const uint8_t value_len, uint8_t* msg_buff)
{
	conf_param_msg_t* conf_param = (conf_param_msg_t*) msg_buff;
	const uint16_t conf_param_len = sizeof(*conf_param) - sizeof(conf_param->value) + value_len;
	memset(conf_param, 0, conf_param_len);

	// Set Configuration Parameter header
	conf_param->header.attr_id_type.is_cfg_par	= 1;
	conf_param->header.attr_id_type.M 			= type;
	conf_param->header.attr_id_type.attr_id		= attr_id;
	conf_param->header.length     				= value_len;

	memcpy(&conf_param->value, value, value_len);

	return conf_param_len;
}

#if IS_COORD
/**
 * @brief   It is used to encode the first EAP-PSK message (T = 0)
 * @param   [in] IdS The EAP server NAI
 * @param   [in] IdS The length of EAP server NAI
 * @param   [in] EapPskData The container for all the encryption related parameters
 * @param   [out] MemoryBuffer The buffer on which the EAP-PSK message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t eap_psk_encode_1(const uint8_t ids[ADP_EAP_PSK_ID_MAX_LEN],
						  const uint8_t ids_len,
						  const adp_lbs_eap_psk_data_t* eap_psk_data,
						  uint8_t* msg_buff)
{
	eap_msg_t* eap_msg = (eap_msg_t*) msg_buff;
	const uint16_t eap_msg_len = ADP_EAP_PSK_FIRST_MSG_MIN_LEN + ids_len;
	memset((uint8_t*) eap_msg, 0, eap_msg_len);

	/* Set EAP header */
	eap_msg->header.eap_header.code         = adp_eap_request;
	eap_msg->header.eap_header.id           = eap_psk_data->eap_id;
	eap_msg->header.eap_header.length       = SWAP_U16(eap_msg_len);

	/* Set EAP-PSK header */
	eap_msg->header.eap_psk_header.type     = adp_eap_psk_iana_type;
	eap_msg->header.eap_psk_header.reserved = 0x00;
	eap_msg->header.eap_psk_header.T        = adp_eap_psk_msg_1;

	// Set other EAP-PSK message fields
	memcpy(eap_msg->msg.n1.rand_s, eap_psk_data->rand_s, sizeof(eap_msg->msg.n1.rand_s));
	memcpy(eap_msg->msg.n1.id_s, ids, ids_len);

	return eap_msg_len;
}

#else

/**
 * @brief   It is used to decode the payload of first EAP-PSK message (T = 0)
 * @param   [in] eap_msg The EAP-PSK packet received from the LBD
 * @param   [out] encryption_param The container for all the encryption related parameters
 * @return  True if the EAP-PSK message is correctly formatted, false otherwise
 */
bool eap_psk_decode_1(const eap_msg_t* eap_msg, adp_lbd_eap_psk_data_t* eap_psk_data)
{
	bool msg_ok = false;
	const uint16_t eap_msg_len = SWAP_U16(eap_msg->header.eap_header.length);
	eap_psk_first_msg_t* msg_1 = (eap_psk_first_msg_t*) &(eap_msg->msg.n1);

	/* Check the length of the message */
	if (eap_msg_len > ADP_EAP_PSK_FIRST_MSG_MIN_LEN)
	{
		memcpy(eap_psk_data->rand_s, msg_1->rand_s, ADP_EAP_PSK_RAND_LEN);
		eap_psk_data->id_s_len = eap_msg_len - ADP_EAP_PSK_FIRST_MSG_MIN_LEN;
		memcpy(eap_psk_data->id_s, msg_1->id_s, eap_psk_data->id_s_len);
		msg_ok = true;
	}

	return msg_ok;
}

/**
 * @brief   It is used to encode the second EAP-PSK message (T = 1)
 * @param   [in] encryption_param The container for all the encryption related parameters
 * @param   [in] idp The EAP peer NAI
 * @param   [in] idp_len The length of EAP peer NAI
 * @param   [out] msg_buff The buffer on which the EAP-PSK message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t eap_psk_encode_2(	const adp_lbd_eap_psk_data_t* eap_psk_data,
							const uint8_t idp[ADP_EAP_PSK_ID_MAX_LEN],
							const uint8_t idp_len,
							uint8_t *msg_buff)
{
	eap_msg_t* eap_msg = (eap_msg_t*) msg_buff;
	const uint16_t eap_msg_len = ADP_EAP_PSK_SECOND_MSG_MIN_LEN + idp_len;
	memset(eap_msg, 0, eap_msg_len);

	// Set EAP header
	eap_msg->header.eap_header.code         = adp_eap_response;
	eap_msg->header.eap_header.id           = eap_psk_data->eap_id;
	eap_msg->header.eap_header.length       = SWAP_U16(eap_msg_len);

	/* Set EAP-PSK header */
	eap_msg->header.eap_psk_header.type     = adp_eap_psk_iana_type;
	eap_msg->header.eap_psk_header.T        = adp_eap_psk_msg_2;
	eap_msg->header.eap_psk_header.reserved = 0x00;

	/* Compute MacP = CMAC-AES-128 (AK, IdP || IdS || RandS || RandP) */
	eap_psk_compute_mac(eap_msg->msg.n2.mac_p, eap_psk_data->psk_context[0].m_au8_ak,
						idp, 				  idp_len,
						eap_psk_data->id_s,    eap_psk_data->id_s_len,
						eap_psk_data->rand_s,  ADP_EAP_PSK_RAND_LEN,
						eap_psk_data->rand_p,  ADP_EAP_PSK_RAND_LEN);

	// Set other EAP-PSK message fields
	memcpy(eap_msg->msg.n2.rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN);
	memcpy(eap_msg->msg.n2.rand_p, eap_psk_data->rand_p, ADP_EAP_PSK_RAND_LEN);
	memcpy(eap_msg->msg.n2.id_p, idp, idp_len);

	return eap_msg_len;
}

#endif /* IS_COORD */

#if IS_COORD

/**
 * @brief   It is used to decode the following field of the second EAP-PSK message (T = 1): IdP and RandP
 * @param   [in] eap_msg The EAP-PSK packet received from the LBD
 * @param   [out] eap_psk_data The container for all the encryption related parameters
 * @return  True if the EAP-PSK message length is correct and RandS is verified, false otherwise
 */
bool eap_psk_decode_2_step1(const eap_msg_t* eap_msg, adp_lbs_eap_psk_data_t* eap_psk_data)
{
	bool msg_ok = false;
	uint16_t eap_msg_len = SWAP_U16(eap_msg->header.eap_header.length);
	eap_psk_second_msg_t* second_msg = (eap_psk_second_msg_t*) &(eap_msg->msg.n2);

	// Check the length of the message
	if (eap_msg_len > ADP_EAP_PSK_SECOND_MSG_MIN_LEN)
	{
		if (memcmp(second_msg->rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN) == 0)
		{
			eap_psk_data->id_p_len = eap_msg_len - ADP_EAP_PSK_SECOND_MSG_MIN_LEN;
			memcpy(eap_psk_data->id_p, second_msg->id_p, eap_psk_data->id_p_len);
			memcpy(eap_psk_data->rand_p, second_msg->rand_p, ADP_EAP_PSK_RAND_LEN);
			memcpy(eap_psk_data->mac_p, second_msg->mac_p, ADP_EAP_MAC_LEN);
			msg_ok = true;
		}
		else
		{
			ALLOC_DYNAMIC_HEX_STRING(rand_received_str, second_msg->rand_s, ADP_EAP_PSK_RAND_LEN);
			ALLOC_DYNAMIC_HEX_STRING(rand_expected_str, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN);
			PRINT_G3_BOOT_SRV_CRITICAL("RandS do not match: %s instead of %s\n", rand_received_str, rand_expected_str);
			FREE_DYNAMIC_HEX_STRING(rand_received_str);
			FREE_DYNAMIC_HEX_STRING(rand_expected_str);
		}
	}
	else
	{
		PRINT_G3_BOOT_SRV_CRITICAL("EAP msg length (%u) insufficient\n", eap_msg_len);
	}

	return msg_ok;
}

/**
 * @brief   It is used to decode the remaining fields of second EAP-PSK message (T = 1) and to verify the MAC_P
 * @param   [in] eap_psk_data The container for all the encryption related parameters
 * @param   [in] ids The EAP server NAI
 * @param   [in] ids_len The length of EAP server NAI
 * @return  True if the EAP-PSK message is correct and MacP is verified, false otherwise
 */
bool eap_psk_decode_2_step2(const adp_lbs_eap_psk_data_t* eap_psk_data, const uint8_t *ids, const uint8_t ids_len)
{
	bool msg_ok = false;
	uint8_t expected_mac_p[ADP_EAP_MAC_LEN];

	/* Compute MacP = CMAC-AES-128 (AK, IdP || IdS || RandS || RandP) */
	eap_psk_compute_mac(expected_mac_p,       eap_psk_data->psk_context[0].m_au8_ak,
						eap_psk_data->id_p,   eap_psk_data->id_p_len,
						ids,                  ids_len,
						eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN,
						eap_psk_data->rand_p, ADP_EAP_PSK_RAND_LEN);

	/* Verify MacP */
	if (memcmp(expected_mac_p, eap_psk_data->mac_p, sizeof(expected_mac_p)) == 0)
	{
		msg_ok = true;
	}
	else
	{
		PRINT_G3_BOOT_SRV_CRITICAL("MacP do not match (check PSK)\n");
	}

	return msg_ok;
}

/**
 * @brief   It is used to encode the third EAP-PSK message (T = 2)
 * @param   [in] eap_psk_data The container for all the encryption related parameters
 * @param   [in] ids The EAP server NAI
 * @param   [in] ids_len The length of EAP server NAI
 * @param   [in] ext_payload The payload carried by EAP-PSK #3 message
 * @param   [in] ext_payload_len The length of the extension payload carried by EAP-PSK #3 message
 * @param   [out] msg_buff The buffer on which the EAP-PSK message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t eap_psk_encode_3(const adp_lbs_eap_psk_data_t* eap_psk_data,
						  const uint8_t *ids,
						  const uint8_t ids_len,
						  const uint8_t* ext_payload,
						  const uint16_t ext_payload_len,
						  uint8_t* msg_buff)
{
	bool result;										/* Result of the EAX functions */
	uint8_t eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN] = {0}; /* Nonce for EAX encryption */
	uint16_t extension_len;								/* Extension data length */

	/* Pointer to the buffer and size */
	eap_msg_t* eap_msg = (eap_msg_t*) msg_buff;
	uint16_t eap_msg_len = 0;	/* Total encoded message length */

	/* Calculates extension length */
	if (ext_payload_len > 0)
	{
		extension_len = sizeof(eap_msg->msg.n3.p_channel.ext.type) + ext_payload_len;
	}
	else
	{
		extension_len = 0;
	}

	/* Calculates the total EAP-PSK message length */
	eap_msg_len = EAP_PSK_MSG3_LEN(extension_len);

	/* Clears the message buffer */
	memset(eap_msg, 0, eap_msg_len);

	/* Fills the EAP header */
	eap_msg->header.eap_header.code         = adp_eap_request;
	eap_msg->header.eap_header.id           = eap_psk_data->eap_id;
	eap_msg->header.eap_header.length 		= SWAP_U16(eap_msg_len);

	/* Fills the EAP-PSK header */
	eap_msg->header.eap_psk_header.type     = adp_eap_psk_iana_type;
	eap_msg->header.eap_psk_header.reserved = 0x00;
	eap_msg->header.eap_psk_header.T        = adp_eap_psk_msg_3;

	/* Set RandS */
	memcpy(eap_msg->msg.n3.rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN);

	/* Compute MacS = CMAC-AES-128(AK, IdS||RandP) */
	eap_psk_compute_mac(eap_msg->msg.n3.mac_s, eap_psk_data->psk_context[0].m_au8_ak,
						ids, 				  ids_len,
						eap_psk_data->rand_p,  ADP_EAP_PSK_RAND_LEN,
						NULL,                 0,
						NULL,                 0);

	/* Set nonce (it has to be encoded in Big Endian) - Only the last byte is used to store Nonce in the EAP server */
	eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN-1] = eap_psk_data->nonce;

	/* Sets P-Channel header */
	eap_msg->msg.n3.p_channel.nonce[0] = 0;
	eap_msg->msg.n3.p_channel.nonce[1] = 0;
	eap_msg->msg.n3.p_channel.nonce[2] = 0;
	eap_msg->msg.n3.p_channel.nonce[3] = eap_psk_data->nonce;

	eap_msg->msg.n3.p_channel.E_R.reserved = 0;
	eap_msg->msg.n3.p_channel.E_R.R        = p_channel_result_success;

	/* If there is an extension, fills it with presence, type and data */
	if (extension_len > 0)
	{
		eap_msg->msg.n3.p_channel.E_R.E = p_channel_extension_present;
		eap_msg->msg.n3.p_channel.ext.type = adp_config_param_ext_type;
		memcpy(eap_msg->msg.n3.p_channel.ext.payload, ext_payload, ext_payload_len);
	}
	else
	{
		eap_msg->msg.n3.p_channel.E_R.E = p_channel_extension_absent;
	}

	result = eap_psk_encrypt_channel(&(eap_msg->msg.n3.p_channel),
									 eap_psk_data->psk_context[0].m_au8_tek,
									 &(eap_msg->header.eap_header),
									 eax_nonce,
									 extension_len);

	if (!result)
	{
		Error_Handler();
	}

	return eap_msg_len;
}
#endif /* IS_COORD */


/**
 * @brief   It is used to decode the third EAP-PSK message (T = 2)
 * @param   [in] eap_msg The EAP-PSK packet received from the LBA
 * @param   [in-out] eap_psk_data The container for all the encryption related parameters
 * @param   [out] p_channel_len Length of the field that constitutes the protected channel.
 * @return  True if the EAP-PSK message is correctly formatted, false otherwise
 */
bool eap_psk_decode_3(const eap_msg_t* eap_msg, adp_lbd_eap_psk_data_t* eap_psk_data, uint16_t* p_channel_len)
{
	bool msg_ok = false;
	uint8_t eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN] = {0};
	uint8_t expected_macs[ADP_EAP_MAC_LEN];
	uint16_t protected_data_len;

	const eap_psk_third_msg_t* msg_3 = &(eap_msg->msg.n3);
	const uint16_t eap_header_len = SWAP_U16(eap_msg->header.eap_header.length);

	do
	{
		memset(&expected_macs, 0, sizeof(expected_macs));

		/* Check length */
		if (eap_header_len <= ADP_EAP_PSK_THIRD_MSG_CLEAR)
		{
			break;
		}

		protected_data_len = eap_header_len - ADP_EAP_PSK_THIRD_MSG_CLEAR;

		/* Check RandS */
		if (memcmp(msg_3->rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN) != 0)
		{
			break;
		}

		/* Compute MacS = CMAC-AES-128(AK, IdS||RandP) */
		eap_psk_compute_mac(expected_macs,        eap_psk_data->psk_context[0].m_au8_ak,
							eap_psk_data->id_s,   eap_psk_data->id_s_len,
							eap_psk_data->rand_p, ADP_EAP_PSK_RAND_LEN,
							NULL,                 0,
							NULL,                 0);

		/* Check MacS */
		if (memcmp(msg_3->mac_s, expected_macs, ADP_EAP_MAC_LEN) != 0)
		{
			break;
		}

		/* Assemble 16 bytes nonce (nonce should be big endian) */
		eap_psk_data->nonce = ASSEMBLE_U32(msg_3->p_channel.nonce[0], msg_3->p_channel.nonce[1], msg_3->p_channel.nonce[2], msg_3->p_channel.nonce[3]);

		/* Check nonce (should never be null) */
		if (eap_psk_data->nonce != 0)
		{
			break;
		}

		/* Prepare 16 bytes nonce (nonce should be big endian) */
		eax_nonce[12] = msg_3->p_channel.nonce[0];
		eax_nonce[13] = msg_3->p_channel.nonce[1];
		eax_nonce[14] = msg_3->p_channel.nonce[2];
		eax_nonce[15] = msg_3->p_channel.nonce[3];

		/* De-crypt channel */
		msg_ok = eap_psk_decrypt_channel(&(msg_3->p_channel),
										 eap_psk_data->psk_context[0].m_au8_tek,
										 &(eap_msg->header.eap_header),
										 eax_nonce,
										 protected_data_len);

		if (msg_ok)
		{
			*p_channel_len = protected_data_len;
		}
	}
	while (0);

	return msg_ok;
}

#if !IS_COORD
/**
 * @brief   It is used to encode the fourth EAP-PSK message (T = 3)
 * @param   [in] eap_psk_data The container for all the encryption related parameters
 * @param   [in] ext_payload The pointer to the buffer containing the Protected Channel data
 * @param   [in] ext_payload_len The length of the Protected Channel data
 * @param   [out] msg_buff The buffer on which the EAP-PSK message is encoded
 * @return  The length of data written in the buffer
 */
uint16_t eap_psk_encode_4(const adp_lbd_eap_psk_data_t* eap_psk_data,
						  const uint8_t* ext_payload,
						  const uint16_t ext_payload_len,
						  uint8_t* msg_buff)
{
	bool result;										/* Result of the EAX functions */
	uint8_t eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN] = {0}; /* Nonce for EAX encryption */
	uint16_t extension_len;								/* Extension data length */

	/* Pointer to the buffer and size */
	eap_msg_t* eap_msg = (eap_msg_t*) msg_buff;
	uint16_t eap_msg_len = 0;	/* Total encoded message length */

	/* Calculates extension length */
	if (ext_payload_len > 0)
	{
		extension_len = sizeof(eap_msg->msg.n4.p_channel.ext.type) + ext_payload_len;
	}
	else
	{
		extension_len = 0;
	}

	/* Calculates the total EAP-PSK message length */
	eap_msg_len = EAP_PSK_MSG4_LEN(extension_len);

	/* Clears the message buffer */
	memset(eap_msg, 0, eap_msg_len);

	/* Fills the EAP header */
	eap_msg->header.eap_header.code         = adp_eap_response;
	eap_msg->header.eap_header.id           = eap_psk_data->eap_id;
	eap_msg->header.eap_header.length 		= SWAP_U16(eap_msg_len);

	/* Fills the EAP-PSK header */
	eap_msg->header.eap_psk_header.type     = adp_eap_psk_iana_type;
	eap_msg->header.eap_psk_header.reserved = 0x00;
	eap_msg->header.eap_psk_header.T        = adp_eap_psk_msg_4;

	/* Set RandS */
	memcpy(eap_msg->msg.n4.rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN);

	/* Set nonce (it has to be encoded in Big Endian) */
	eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN-4] = (eap_psk_data->nonce >> 24) & 0xFF;
	eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN-3] = (eap_psk_data->nonce >> 16) & 0xFF;
	eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN-2] = (eap_psk_data->nonce >> 8 ) & 0xFF;
	eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN-1] = (eap_psk_data->nonce      ) & 0xFF;

	/* Sets P-Channel header */
	eap_msg->msg.n4.p_channel.nonce[0] = (eap_psk_data->nonce >> 24) & 0xFF;
	eap_msg->msg.n4.p_channel.nonce[1] = (eap_psk_data->nonce >> 16) & 0xFF;
	eap_msg->msg.n4.p_channel.nonce[2] = (eap_psk_data->nonce >> 8 ) & 0xFF;
	eap_msg->msg.n4.p_channel.nonce[3] = (eap_psk_data->nonce      ) & 0xFF;

	eap_msg->msg.n4.p_channel.E_R.reserved = 0;
	eap_msg->msg.n4.p_channel.E_R.R        = p_channel_result_success;

	/* If there is an extension, fills it with presence, type and data */
	if (extension_len > 0)
	{
		eap_msg->msg.n4.p_channel.E_R.E = p_channel_extension_present;
		eap_msg->msg.n4.p_channel.ext.type = adp_config_param_ext_type;
		memcpy(eap_msg->msg.n4.p_channel.ext.payload, ext_payload, ext_payload_len);
	}
	else
	{
		eap_msg->msg.n4.p_channel.E_R.E = p_channel_extension_absent;
	}

	result = eap_psk_encrypt_channel(&(eap_msg->msg.n4.p_channel),
									 eap_psk_data->psk_context[0].m_au8_tek,
									 &(eap_msg->header.eap_header),
									 eax_nonce,
									 extension_len);

	if (!result)
	{
		Error_Handler();
	}

	return eap_msg_len;
}

#endif /* !IS_COORD */

#if IS_COORD

/**
 * @brief   It is used to decode the fourth EAP-PSK message (T = 3)
 * @param   [in] eap_msg The EAP-PSK packet received from the LBA
 * @param   [in] eap_psk_data The container for all the encryption related parameters
 * @param   [out] p_channel_len Length of the field that constitutes the protected channel.
 * @return  True if the EAP-PSK message is correctly formatted, falsebool otherwise
 */
uint8_t eap_psk_decode_4(const eap_msg_t* eap_msg, const adp_lbs_eap_psk_data_t* eap_psk_data, uint16_t* p_channel_len)
{
	bool msg_ok = false;
	uint8_t eax_nonce[ADP_EAP_PSK_EAX_NONCE_LEN] = {0};
	uint32_t received_nonce = 0;
	uint16_t protected_data_len;

	const eap_psk_fourth_msg_t* msg_4 = &(eap_msg->msg.n4);
	const uint16_t eap_header_len = SWAP_U16(eap_msg->header.eap_header.length);

	do
	{
		/* Check length */
		if (eap_header_len <= ADP_EAP_PSK_FOURTH_MSG_CLEAR)
		{
			break;
		}

		protected_data_len = eap_header_len - ADP_EAP_PSK_FOURTH_MSG_CLEAR;

		/* Check RandS */
		if (memcmp(msg_4->rand_s, eap_psk_data->rand_s, ADP_EAP_PSK_RAND_LEN) != 0)
		{
			break;
		}

		/* Assemble received nonce */
		received_nonce = ASSEMBLE_U32(msg_4->p_channel.nonce[0], msg_4->p_channel.nonce[1], msg_4->p_channel.nonce[2], msg_4->p_channel.nonce[3]);

		/* Check nonce rollover (only 1 Byte Nonce is stored by the EAP server) */
		if (received_nonce >= 0x000000FF)
		{
			break;
		}

		/* Check received nonce value */
		if (eap_psk_data->nonce != ((uint8_t) received_nonce))
		{
			break;
		}

		/* Prepare 16 bytes nonce (nonce should be big endian) */
		eax_nonce[12] = msg_4->p_channel.nonce[0];
		eax_nonce[13] = msg_4->p_channel.nonce[1];
		eax_nonce[14] = msg_4->p_channel.nonce[2];
		eax_nonce[15] = msg_4->p_channel.nonce[3];

		/* De-crypt channel */
		msg_ok = eap_psk_decrypt_channel(&(msg_4->p_channel),
										 eap_psk_data->psk_context[0].m_au8_tek,
										 &(eap_msg->header.eap_header),
										 eax_nonce,
										 protected_data_len);

		if (msg_ok)
		{
			*p_channel_len = protected_data_len;
		}

	} while(0);

	return msg_ok;
}

#endif /* IS_COORD */

/**
 * @}
 */

/**
 * @}
 */

/******************* (C) COPYRIGHT 2013 STMicroelectronics *****END OF FILE****/
