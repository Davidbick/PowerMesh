/*****************************************************************************
*   @file    hi_adp_lbp_message_catalog.h
*   @author  AMG/IPC Application Team
*   @brief   Header file that contains the definition of types and messages used in ADP-LBP.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ADP_LBP_MESSAGECATALOG_H__
#define __ADP_LBP_MESSAGECATALOG_H__

#include <stdint.h>
#include <hi_mac_constants.h>

/** @defgroup LBP_MessageCatalog Message Catalog
  * @brief Definition of the messages used by LOADng protocol
  * @{
  */

/** @defgroup LBP_TypesDefinitions Definition of types
  * @brief Definition of macros and types used in messages for LBP
  * @{
  */

/* Macros */
#define ADP_EAP_MAC_LEN               ((uint8_t) 16)	/**< @brief MacS/MacP key size */
#define ADP_EAP_PSK_KEY_LEN           ((uint8_t) 16)	/**< @brief PSK Key size */
#define ADP_EAP_PSK_MSG_NONCE_LEN     ((uint8_t) 4)		/**< @brief PSK Nonce size */
#define ADP_CONFIG_PARAMS_BUF_LEN 	  ((uint8_t) 128)	/**< @brief Implementation defined buffer length for Configuration Parameters */

// Since EAX uses a 16-byte Nonce, the Nonce is padded with 96 zero bits
/* Message Length macros */
#define ADP_EAP_PSK_EAX_NONCE_LEN     ((uint8_t) 16)	/**< @brief PSK EAX NONCE size */
#define ADP_EAP_PSK_TAG_LEN           ((uint8_t) 16)	/**< @brief PSK TAG size */
#define ADP_EAP_PSK_RAND_LEN          ((uint8_t) 16)	/**< @brief PSK RAND size */
#define ADP_EAP_PSK_ID_MAX_LEN        ((uint8_t) 36)	/**< @brief PSK IDs (ID_P, ID_S) maximum size */
#define ADP_EAP_MACS_SEED_MAX_LEN     ((uint8_t) 52)	/**< @brief MCAS SEED maximum size (ADP_EAP_PSK_RAND_LEN + ADP_EAP_PSK_ID_MAX_LEN) */
#define ADP_EAP_MACP_SEED_MAX_LEN     ((uint8_t) 104)	/**< @brief MACP SEED maximum size 2*(ADP_EAP_PSK_RAND_LEN + ADP_EAP_PSK_ID_MAX_LEN) */

#define ADP_EAP_PSK_MSG_MIN_LEN        ((uint16_t) 6)	/**< @brief PSK MSG minimum size: EAP header (4) + EAP-PSK header (2) */
#define ADP_EAP_PSK_FIRST_MSG_MIN_LEN  ((uint16_t) 22)	/**< @brief PSK First message minimum size:  EAP/EAP-PSK headers (6) + RandS (16) */
#define ADP_EAP_PSK_SECOND_MSG_MIN_LEN ((uint16_t) 54)	/**< @brief PSK Second message minimum size: EAP/EAP-PSK headers (6) + RandS (16) + RandP (16) + MacP (16) */
#define ADP_EAP_PSK_THIRD_MSG_CLEAR    ((uint16_t) 58)	/**< @brief Un-encrypted part of the message: EAP/EAP-PSK headers (6) + RandS (16) + MacS (16) + Nonce (4) + Tag (16) */
#define ADP_EAP_PSK_AUTH_HEADER_LEN    ((uint16_t) 22)	/**< @brief As per RFC 4764 - 3.3 (EAP Code + Id + Length + Type + Flags + RandS) */
#define ADP_EAP_PSK_FOURTH_MSG_CLEAR   ((uint16_t) 42)	/**< @brief Un-encrypted part of the message: EAP/EAP-PSK headers (6) + RandS (16) + Nonce (4-byte) + Tag (16-byte) */

#define EAP_PSK_MSG3_LEN(ext_len)		(sizeof(eap_msg_t) - sizeof(eap_psk_msg_t) + sizeof(eap_psk_third_msg_t) - sizeof(eap_psk_channel_ext_t) + ext_len)
#define EAP_PSK_MSG3_EXT_LEN(msg_len)	(msg_len + sizeof(eap_psk_msg_t) - sizeof(eap_msg_t) + sizeof(eap_psk_channel_ext_t) - sizeof(eap_psk_third_msg_t))

#define EAP_PSK_MSG4_LEN(ext_len)		(sizeof(eap_msg_t) - sizeof(eap_psk_msg_t) + sizeof(eap_psk_fourth_msg_t) - sizeof(eap_psk_channel_ext_t) + ext_len)
#define EAP_PSK_MSG4_EXT_LEN(msg_len)	(msg_len + sizeof(eap_psk_msg_t) - sizeof(eap_msg_t) + sizeof(eap_psk_channel_ext_t) - sizeof(eap_psk_fourth_msg_t))

/* Types */
typedef enum adp_lbp_type_enum
{
	adp_lbp_from_lbd = 0x00, /**< @brief Type of LBP message: from LBD */
	adp_lbp_to_lbd   = 0x01  /**< @brief Type of LBP message: to LBD */
} adp_lbp_type_t;

typedef enum adp_lbs_code_enum
{
	adp_lbs_accepted  = 0x01, /**< @brief Authentication succeeded with delivery of device specific information (DSI) to the LBD */
	adp_lbs_challange = 0x02, /**< @brief Authentication in progress. PAN specific information (PSI) may be delivered to the LBD */
	adp_lbs_decline   = 0x03, /**< @brief Authentication failed */
	adp_lbs_kick	  = 0x04, /**< @brief KICK frame is used by a PAN coordinator to force a device to lose its MAC address */
	adp_lbs_ack       = 0x05  /**< @brief The LBS acknowledges the notification */
} adp_lbs_code_t;

typedef enum adp_lbd_code_enum
{
	adp_lbd_joining      = 0x01,	/**< @brief The LBD requests joining a PAN and provides the necessary authentication material */
	adp_lbd_kick         = 0x04,	/**< @brief KICK frame is used by a device to inform the coordinator that it left the PAN. */
	adp_lbd_notification = 0x05		/**< @brief The LBD notifies the LBS of its presence within the PAN */
} adp_lbd_code_t;

/* EAP message types */
typedef enum adp_eap_code_enum
{
	adp_eap_request  = 0x01,	/**< @brief Request (sent to the peer = LBP) */
	adp_eap_response = 0x02,	/**< @brief Response (sent by the peer) */
	adp_eap_success  = 0x03,	/**< @brief Success (sent to the peer) */
	adp_eap_failure  = 0x04		/**< @brief Failure (sent to the peer) */
} adp_eap_code_t;

typedef enum adp_eap_type_enum
{
	adp_eap_psk_iana_type = 0x2F /**< @brief EAP Method Type: EAP-PSK */
} adp_eap_type_t;

/* T-subfield types */
typedef enum adp_eap_subtype_enum
{
	adp_eap_psk_msg_1 = 0x00, /**< @brief T0, the first EAP-PSK message */
	adp_eap_psk_msg_2 = 0x01, /**< @brief T1, the second EAP-PSK message */
	adp_eap_psk_msg_3 = 0x02, /**< @brief T2, the third EAP-PSK message */
	adp_eap_psk_msg_4 = 0x03  /**< @brief T3, the forth EAP-PSK message */
} adp_eap_subtype_t;

typedef enum adp_conf_param_attr_info_enum
{
	conf_param_dsi = 0,
	conf_param_psi = 1,
} adp_conf_param_attr_info_t;

/* P-Channel E (extension) field */
typedef enum p_channel_extension_enum
{
	p_channel_extension_absent  = 0,
	p_channel_extension_present = 1
} p_channel_extension_t;

/* P-Channel R (result) field */
typedef enum p_channel_result_enum
{
	p_channel_result_continue = 0x01,	/**< @brief P-Channel result field: Continue */
	p_channel_result_success  = 0x02,	/**< @brief P-Channel result field: Success  */
	p_channel_result_failure  = 0x03	/**< @brief P-Channel result field: Failure  */
} p_channel_result_t;

 /* P-Channel EXT_Type field */
typedef enum p_channel_ext_type_enum
{
	adp_config_param_ext_type = 0x02 /**< @brief G3 EAP-PSK Extension */
} p_channel_ext_type_t;

#pragma pack(push, 1)

/**
  * @brief  LoWPAN bootstrapping protocol (LBP) header format
  */
typedef struct lbp_header_struct
{
  uint8_t  reserved_1   : 2;       		/**< @brief Reserved by ITU-T, set to 0 by the sender and ignored by the receiver */
  uint8_t  disable_bkp  : 1;        	/**< @brief Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled) */
  uint8_t  media_type   : 1;        	/**< @brief Identifies the MediaType used for LBD – LBA communication (0x00 PLC, 0x01 RF) */
  uint8_t  code         : 3;        	/**< @brief Identifies the message code (JOINING, ACCEPTED, CHALLENGE, DECLINE, KICK) */
  uint8_t  T            : 1;        	/**< @brief Identifies the type of message (adp_type_t -> 0: Message from LBD, 1: Message to LBD) */
  uint8_t  reserved_2;              	/**< @brief Reserved by ITU-T, set to 0 by the sender and ignored by the receiver */
  uint8_t  lbd_addr[MAC_ADDR64_SIZE];	/**< @brief A_LBD: Indicates the EUI-64 address of the bootstrapping device (LBD) */
} lbp_header_t;

/**
  * @brief  EAP Header Format
  */
typedef struct eap_header_str
{
	uint8_t  is_cfg_par	:1;	/**< @brief One bit set as 0 (1 for configuration parameter) */
	uint8_t  reserved 	:1;	/**< @brief One bit set as 0 */
	uint8_t  code    	:6;	/**< @brief The Code field identifies the Type of EAP packet (Request, Response, Success, Failure)     */
	uint8_t  id;    		/**< @brief The Identifier field aids in matching Responses with Requests. */
	uint16_t length;		/**< @brief The Length field indicates the length, in octets, of the EAP packet including the Code, Identifier, Length, and Data fields.   */
} eap_header_t;

/**
  * @brief  Configuration Parameter Header Format
  */

typedef struct attr_id_type_str
{
	uint8_t  is_cfg_par	:1;	/**< @brief One bit set as 1 */
	uint8_t  M 			:1;	/**< @brief Identifies the type of the attribute: 0: Device specific information (DSI) 1: PAN specific information (PSI)*/
	uint8_t  attr_id	:6;	/**< @brief Represents the ID of the attribute in the LoWPAN information base (LIB)     */
} attr_id_type_t;

typedef struct conf_param_header_str
{
	attr_id_type_t	attr_id_type;	/**< @brief Attribute ID + Type (M) + "is_cfg_par" */
	uint8_t  		length;			/**< @brief Indicates the length, in bytes, of the value field.   */
} conf_param_header_t;

/**
  * @brief  EAP-PSK Header Format
  */
typedef struct eap_psk_header_str
{
	uint8_t  type;           /**< @brief This field indicates the Type of Request or Response. */
	uint8_t  reserved  :6;   /**< @brief Reserved subfield that is set to zero on transmission and ignored on reception */
	uint8_t  T         :2;   /**< @brief Indicates the type of EAP-PSK message */
} eap_psk_header_t;

typedef struct E_R_str
{
	uint8_t  reserved	:5;                         /**< @brief Reserved field, which is set to zero on emission and ignored on reception */
	uint8_t  E       	:1;                         /**< @brief A 1-bit extension flag E, which is set to 1 if an EAP-PSK Extension is present */
	uint8_t  R       	:2;                         /**< @brief A 2-bit result indication flag R */
} E_R_t;

typedef struct eap_psk_channel_ext_str
{
	uint8_t	type;                               /**< @brief The EXT_Type subfield indicates the type of the extension */
	uint8_t	payload[ADP_CONFIG_PARAMS_BUF_LEN]; /**< @brief The EXT_Payload subfield consists of the payload of the extension */
} eap_psk_channel_ext_t;

/**
  * @brief  EAP-PSK PCHANNEL field Format
  */
typedef struct eap_psk_channel_struct
{
	uint8_t					nonce[ADP_EAP_PSK_MSG_NONCE_LEN];	/**< @brief Nonce field */
	uint8_t					tag[ADP_EAP_PSK_TAG_LEN];        	/**< @brief Tag field */
	E_R_t					E_R;								/**< @brief Reserved + E + R fields */
	eap_psk_channel_ext_t	ext;								/**< @brief Extension */
} eap_psk_channel_t;

/**
  * @brief  EAP-PSK First Message Format
  */
typedef struct eap_psk_first_msg_str
{
  uint8_t          rand_s[ADP_EAP_PSK_RAND_LEN];	/**< @brief A 16-byte random number */
  uint8_t          id_s[ADP_EAP_PSK_ID_MAX_LEN];	/**< @brief A field that conveys the server's NAI: ID_S  */
} eap_psk_first_msg_t;

/**
  * @brief  EAP-PSK Second Message Format
  */
typedef struct eap_psk_second_msg_str
{
  uint8_t          rand_s[ADP_EAP_PSK_RAND_LEN];	/**< @brief The 16-byte random number sent by the server in the first EAP-PSK message */
  uint8_t          rand_p[ADP_EAP_PSK_RAND_LEN];	/**< @brief A 16-byte random number */
  uint8_t          mac_p[ADP_EAP_MAC_LEN];      	/**< @brief A 16-byte MAC */
  uint8_t          id_p[ADP_EAP_PSK_ID_MAX_LEN];	/**< @brief A field that conveys the peer's NAI: ID_P  */
} eap_psk_second_msg_t;

/**
  * @brief  EAP-PSK Third Message Format
  */
typedef struct eap_psk_third_msg_str
{
  uint8_t			rand_s[ADP_EAP_PSK_RAND_LEN]; 	/**< @brief The 16-byte random number sent by the server in the first EAP-PSK message */
  uint8_t			mac_s[ADP_EAP_MAC_LEN];       	/**< @brief A 16-byte MAC */
  eap_psk_channel_t	p_channel;                  	/**< @brief A variable length field that constitutes the protected channel */
} eap_psk_third_msg_t;

/**
  * @brief  EAP-PSK Fourth Message Format
  */
typedef struct eap_psk_fourth_msg_str
{
  uint8_t			rand_s[ADP_EAP_PSK_RAND_LEN]; /**< @brief The 16-byte random number sent by the server in the first EAP-PSK message */
  eap_psk_channel_t p_channel;                    /**< @brief A variable length field that constitutes the protected channel */
} eap_psk_fourth_msg_t;

typedef union eap_psk_msg_union
{
	eap_psk_first_msg_t  n1;  	/**< @brief First Message */
	eap_psk_second_msg_t n2;	/**< @brief Second Message */
	eap_psk_third_msg_t  n3;	/**< @brief Third Message */
	eap_psk_fourth_msg_t n4;	/**< @brief Fourth Message */
} eap_psk_msg_t;

/**
  * @brief  EAP Packet Format
  */
typedef struct eap_msg_str
{
	struct header_struct
	{
		eap_header_t     eap_header;    	/**< @brief EAP Header */
		eap_psk_header_t eap_psk_header;	/**< @brief PSK-PSK Header */
	} header;
	eap_psk_msg_t	 msg;				/**< @brief EAP-PSK message */
} eap_msg_t;

typedef struct adp_gmk_param_value_str
{
	uint8_t gmk_index;
	uint8_t gmk[MAC_KEY_SIZE];
} adp_gmk_param_value_t;

typedef struct adp_param_result_param_value_str
{
	uint8_t			result;
	attr_id_type_t	param_id;
} adp_param_result_param_value_t;

/**
  * @brief  Configuration parameter Format
  */
typedef struct conf_param_msg_str
{
	conf_param_header_t	header;    				/**< @brief Header */
	union conf_param_msg_union
	{
		uint16_t 						short_addr_param;
		adp_gmk_param_value_t			gmk_param;
		uint8_t							gmk_activation_param;
		adp_param_result_param_value_t	param_result_param;
	} value;
} conf_param_msg_t;

/**
  * @brief  LoWPAN bootstrapping protocol (LBP) message format
  */
typedef struct lbp_msg_struct
{
  lbp_header_t header;                                    	/**< @brief Headers */
  union {
	  uint8_t			bootstrap_data[ADP_CONFIG_PARAMS_BUF_LEN];	/**< @brief Contains additional information elements (EAP message) */
	  eap_msg_t 		eap;
	  conf_param_msg_t	param;
  };
} lbp_msg_t;

#pragma pack(pop)

/**
  * @}
  */

/**
  * @}
  */

#endif //__ADP_LBP_MESSAGECATALOG_H__

/******************* (C) COPYRIGHT 2013 STMicroelectronics *******************/
