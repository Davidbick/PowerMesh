/*****************************************************************************
*   @file    hi_adp_eap_psk.h
*   @author  AMG/IPC Application Team
*   @brief   Header file that contains RFC4764 (The EAP-PSK Protocol) types and definitions
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef HI_ADP_EAP_PSK_H_
#define HI_ADP_EAP_PSK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <eax.h>
#include <cmac.h>
#include <hi_adp_lbp_message_catalog.h>

/** @addtogroup G3_ADP
  * @{
  */

/** @addtogroup G3_ADP_LBP
  * @{
  */

/** @addtogroup G3_ADP_LBP_EAP
  * @{
  */

/** @defgroup EAP_PSK EAP-PSK layer
  * @{
  */

/** @defgroup EAP_PSK_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup EAP_PSK_Exported_Code Exported Code
  * @{
  */

/** @brief The EAP_PSK_Context type keeps information needed for EAP-PSK calls
  */
typedef struct eap_psk_context_str
{
    uint8_t m_au8_kdk[ADP_EAP_PSK_KEY_LEN];	// KDK: Key-Derivation key
    uint8_t m_au8_ak[ADP_EAP_PSK_KEY_LEN];	// AK:  Authentication Key
    uint8_t m_au8_tek[ADP_EAP_PSK_KEY_LEN]; // TEK: Transient Encryption Key
} eap_psk_context_t;

typedef struct adp_lbd_eap_psk_data_str
{
  eap_psk_context_t psk_context[1];
  uint8_t           eap_id;
  uint32_t          nonce;
  uint8_t           rand_s[ADP_EAP_PSK_RAND_LEN];
  uint8_t           rand_p[ADP_EAP_PSK_RAND_LEN];
  uint8_t           id_s[ADP_EAP_PSK_ID_MAX_LEN];
  uint8_t           id_s_len;
} adp_lbd_eap_psk_data_t;

typedef struct adp_lbs_eap_psk_data_str
{
  eap_psk_context_t psk_context[1];
  uint8_t           eap_id;
  uint8_t           nonce;
  uint8_t           rand_s[ADP_EAP_PSK_RAND_LEN];
  uint8_t           rand_p[ADP_EAP_PSK_RAND_LEN];
  uint8_t           id_p[ADP_EAP_PSK_ID_MAX_LEN];
  uint8_t           id_p_len;
  uint8_t           mac_p[ADP_EAP_MAC_LEN];
} adp_lbs_eap_psk_data_t;

/* Public Functions */
void eap_psk_initialize_psk(const uint8_t au8_eap_psk[ADP_EAP_PSK_KEY_LEN], eap_psk_context_t psk_context[1]);
void eap_psk_initialize_tek(const uint8_t au8_rand_p[ADP_EAP_PSK_RAND_LEN], eap_psk_context_t psk_context[1]);

uint16_t eap_encode_success(const uint8_t eap_identifier, uint8_t* msg_buff);
uint16_t conf_param_encode(const uint8_t attr_id, uint8_t type, const void* value, const uint8_t value_len, uint8_t* msg_buff);
#if IS_COORD
uint16_t eap_psk_encode_1(const uint8_t ids[ADP_EAP_PSK_ID_MAX_LEN], const uint8_t ids_len, const adp_lbs_eap_psk_data_t* eap_psk_data, uint8_t* mem_buff);
bool	 eap_psk_decode_1(const eap_msg_t* eap_msg, adp_lbd_eap_psk_data_t* eap_psk_data);
#else
uint16_t eap_psk_encode_2(const adp_lbd_eap_psk_data_t* eap_psk_data, const uint8_t id_p[ADP_EAP_PSK_ID_MAX_LEN], const uint8_t id_p_len, uint8_t *mem_buff);
#endif
#if IS_COORD
bool 	 eap_psk_decode_2_step1(const eap_msg_t* eap_msg, adp_lbs_eap_psk_data_t* eap_psk_data);
bool 	 eap_psk_decode_2_step2(const adp_lbs_eap_psk_data_t* eap_psk_data, const uint8_t *ids, const uint8_t ids_len);
uint16_t eap_psk_encode_3(const adp_lbs_eap_psk_data_t* eap_psk_data, const uint8_t *ids, const uint8_t ids_len, const uint8_t* ext_payload, const uint16_t ext_payload_len, uint8_t* mem_buff);
#else
bool	 eap_psk_decode_3(const eap_msg_t* eap_msg, adp_lbd_eap_psk_data_t* eap_psk_data, uint16_t* p_channel_len);
uint16_t eap_psk_encode_4(const adp_lbd_eap_psk_data_t* eap_psk_data, const uint8_t* ext_payload, const uint16_t ext_payload_len, uint8_t* mem_buff);
#endif
#if IS_COORD
uint8_t eap_psk_decode_4(const eap_msg_t* eap_msg, const adp_lbs_eap_psk_data_t* eap_psk_data, uint16_t* p_channel_len);
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* HI_ADP_EAP_PSK_H_ */

/******************* (C) COPYRIGHT 2022 STMicroelectronics *******************/
