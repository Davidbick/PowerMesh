/*****************************************************************************
*   @file    hi_adp_lbp.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for ADP LoWPAN Bootstrap Protocol.
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
#ifndef HI_ADP_LBP_H_
#define HI_ADP_LBP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <g3_boot_srv_join_entry_tbl.h>

/** @defgroup G3_ADP G3 ADP layer
  * @{
  */

/** @defgroup G3_ADP_LBP G3 ADP-LBP layer
  * @{
  */

/** @defgroup G3_ADP_LBP_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_ADP_LBP_Exported_Code Exported Code
  * @{
  */

/* Definitions */
#define NO_ACTIVE_INDEX	0xFF

/** @brief Configuration parameters value length */
#define ADP_CONF_PARAM_SHORT_ADDR_LEN         ((uint8_t) 0x02)
#define ADP_CONF_PARAM_GMK_LEN                ((uint8_t) 0x11)
#define ADP_CONF_PARAM_GMK_ACTIVATION_LEN     ((uint8_t) 0x01)
#define ADP_CONF_PARAM_GMK_REMOVAL_LEN        ((uint8_t) 0x01)
#define ADP_CONF_PARAM_RESULT_LEN             ((uint8_t) 0x02)
#define ADP_CONF_PARAM_SA_ADD_REQ_LEN         ((uint8_t) 0x01)

/**
  * @brief Data structure used by the Bootstrap Server to handle the LBP message received from the ADP Layer
  */
typedef struct lbp_ind_str
{
	lbp_msg_t*		 	lbp_msg;		/**< @brief The LoWPAN Bootstrap Protocol Message received from the Node */
    eap_msg_t*	 	 	eap_msg;		/**< @brief The EAP Packet carried by the LBP message received from the Node */
    conf_param_msg_t*	conf_param_msg;	/**< @brief The Configuration Parameter Packet carried by the LBP message received from the Node */
    uint16_t		 	lba_addr;		/**< @brief The Address of LBA */
} lbp_ind_t;

/** @brief Configuration parameters (already encoded on 1 byte - M field and last bit included) */
typedef enum adp_msg_param_id_enum
{
	ADP_MSG3_PARAM_SHORT_ADDR     = 0x1D, // Short Address
	ADP_MSG3_PARAM_GMK            = 0x27, // GMK
	ADP_MSG3_PARAM_GMK_ACTIVATION = 0x2B, // GMK-activation
	ADP_MSG3_PARAM_GMK_REMOVAL    = 0x2F, // GMK-removal
	ADP_MSG4_PARAM_RESULT         = 0x31, // Parameter-result
} adp_msg_param_id_t;

/** @brief Configuration parameters ID */
typedef enum adp_conf_param_id_enum
{
	ADP_MSG3_PARAM_SHORT_ADDR_ID     = 0x07, // Short Address
	ADP_CONF_PARAM_GMK_ID            = 0x09, // GMK
	ADP_CONF_PARAM_GMK_ACTIVATION_ID = 0x0A, // GMK-activation
	ADP_CONF_PARAM_GMK_REMOVAL_ID    = 0x0B, // GMK-removal
	ADP_CONF_PARAM_RESULT_ID         = 0x0C, // Parameter-result
} adp_conf_param_id_t;

/** @brief  Parameter result values   */
typedef enum adp_param_result_enum
{
	ADP_RESULT_PARAMETER_SUCCESS          = 0x00,
	ADP_RESULT_MISSING_REQUIRED_PARAMETER = 0x01,
	ADP_RESULT_INVALID_PARAMETER_VALUE    = 0x02,
	ADP_RESULT_UNKNOWN_PARAMETER_ID       = 0x03
} adp_param_result_t;

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST

/* Public Functions */
uint16_t g3_adp_lbp_encode_3_params(uint8_t* extension_data, const uint16_t short_addr, const uint8_t* old_gmk, const uint8_t* new_gmk, const uint8_t old_gmk_index);
bool     g3_adp_lbp_decode_4_params(uint8_t* extension_data, const uint16_t data_length);

void g3_adp_lbp_eap_send_1(  boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t *ids, const uint16_t ids_len);
void g3_adp_lbp_eap_send_3(  boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t *ids, const uint16_t ids_len, const uint16_t short_address, const uint8_t *old_gmk, const uint8_t *new_gmk, const uint8_t old_gmk_index);
void g3_adp_lbp_send_accept( boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle);
void g3_adp_lbp_send_decline(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle);
void g3_adp_lbp_send_kick(   boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle);

void g3_adp_lbp_send_gmk_activation(boot_join_entry_t *join_entry, const uint16_t pan_id, const uint8_t handle, const uint8_t gmk_index);

#elif !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST

/* Public Functions */
uint16_t g3_adp_lbp_decode_3_params(uint8_t* extension_data, const uint16_t short_addr, const uint8_t* old_gmk, const uint8_t* new_gmk, const uint8_t old_gmk_index);
bool     g3_adp_lbp_encode_4_params(uint8_t* extension_data, const uint16_t data_length);

void g3_adp_lbp_send_join( const uint16_t pan_id, const uint8_t handle);
void g3_adp_lbp_eap_send_2(const uint16_t pan_id, const uint8_t handle, const uint8_t *idp, const uint16_t idp_len);
void g3_adp_lbp_eap_send_4(const uint16_t pan_id, const uint8_t handle, const uint8_t *idp, const uint16_t idp_len);
void g3_adp_lbp_send_leave(const uint16_t pan_id, const uint8_t handle);

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

#ifdef __cplusplus
}
#endif

#endif /* HI_ADP_LBP_H_ */

/******************* (C) COPYRIGHT 2022 STMicroelectronics *******************/
