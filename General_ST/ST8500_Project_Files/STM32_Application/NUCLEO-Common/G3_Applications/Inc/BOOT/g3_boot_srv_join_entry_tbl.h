/*****************************************************************************
*   @file    g3_boot_srv_join_entry_tbl.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Boot joining entry table management.
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
#ifndef G3_APP_BOOT_SRV_TBL_H_
#define G3_APP_BOOT_SRV_TBL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <settings.h>
#include <hi_adp_eap_psk.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot_Srv
  * @{
  */

/** @addtogroup G3_App_Boot_Srv_Exported_Code
  * @{
  */

#if IS_COORD

/* Definitions */
#define BOOT_MAX_NUM_JOINING_NODES        8	/**< @brief The maximum number of supported connected devices */

#if ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Set of events for node connecting or connected to the bootstrap server
  */
typedef enum boot_srv_eap_event_enum
{
	BOOT_SRV_EAP_EV_NONE = 0,			/**< @brief No event ID */
	BOOT_SRV_EAP_EV_RECEIVED_JOIN, 		/**< @brief JOIN message event ID */
	BOOT_SRV_EAP_EV_RECEIVED_SECOND,	/**< @brief EAP #2 message event ID */
	BOOT_SRV_EAP_EV_PSK_ACQUIRED,		/**< @brief PSK received/timeout event ID */
	BOOT_SRV_EAP_EV_RECEIVED_FOURTH,	/**< @brief EAP #4 message event ID */
	BOOT_SRV_EAP_EV_RECEIVED_CNF,		/**< @brief EAP #4 message event ID */
	BOOT_SRV_EAP_EV_RECEIVED_PARAM,		/**< @brief Parameter activation Event ID */
	BOOT_SRV_EAP_EV_RECEIVED_KICK,		/**< @brief KICK message event ID (no effect in handshake) */
	BOOT_SRV_EAP_EV_CNT					/**< @brief Number of events */
} boot_srv_eap_event_t;

/**
  * @brief Set of states for node connecting or connected to the bootstrap server
  */
typedef enum boot_srv_eap_state_enum
{
    BOOT_SRV_EAP_ST_WAIT_JOIN = 0,		/**< @brief Waiting for EAP Join message state */
    BOOT_SRV_EAP_ST_WAIT_SECOND,		/**< @brief Waiting for EAP #2 message */
    BOOT_SRV_EAP_ST_WAIT_PSK,			/**< @brief Waiting for SETPSK request  */
    BOOT_SRV_EAP_ST_WAIT_FOURTH,		/**< @brief Waiting for EAP #4 message */
    BOOT_SRV_EAP_ST_WAIT_CNF,			/**< @brief Waiting for LBP.Confirm message */
	BOOT_SRV_EAP_ST_WAIT_PARAM,			/**< @brief Waiting for Parameter result message */
    BOOT_SRV_EAP_ST_CNT					/**< @brief Number of states */
} boot_srv_eap_state_t;

/**
  * @brief Data structure used by the Bootstrap Server during a Joining procedure
  */
typedef struct boot_join_entry_str
{
    uint8_t   				ext_addr[MAC_ADDR64_SIZE]; /**< @brief Node Extended Address (EUI-64) */
    uint16_t  				short_addr;                /**< @brief The assigned Short Address */
    uint16_t  				lba_addr;                  /**< @brief The Short Address of LBA */
    boot_srv_eap_state_t    curr_state;				   /**< @brief The Current State of the Node */
    boot_srv_eap_event_t 	curr_event;				   /**< @brief The Current Event of the Node */
    adp_lbs_eap_psk_data_t	eap_psk_data;              /**< @brief Encryption parameters used for EAP-PSK handshake */
    uint32_t  				join_time;                 /**< @brief The time at which the first LBP Joining message is received from the Node */
    uint32_t  				getpsk_timestamp;	   	   /**< @brief Time stamp of the transmission of the GETPSK-IND for this entry */
    uint8_t   				media_type;                /**< @brief Identifies the MediaType used for LBD – LBA communication (adp_mediatype_t) */
    uint16_t				accept_handle;
    uint8_t   				disable_bkp;               /**< @brief Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled) */
    bool 					rekeying;
} boot_join_entry_t;


/* Public functions */
uint8_t 			g3_boot_srv_join_entry_reset_expired_entries(void);
boot_join_entry_t*  g3_boot_srv_join_entry_add(const uint8_t* ext_addr, const uint16_t lba_short_addr, const uint8_t media_type, const uint8_t disable_bkp, const bool rekeying);
void 				g3_boot_srv_join_entry_remove(boot_join_entry_t* join_entry);
boot_join_entry_t* 	g3_boot_srv_join_entry_find(const uint8_t ext_addr[MAC_ADDR64_SIZE]);
boot_join_entry_t*  g3_boot_srv_join_entry_find_handle(const uint16_t handle);
boot_join_entry_t*	g3_boot_srv_join_entry_find_free();
boot_join_entry_t* 	g3_boot_srv_join_entry_get(uint32_t entry_index);
uint32_t 			g3_boot_srv_join_entry_index(boot_join_entry_t* join_entry);
void 				g3_boot_srv_init_tables(void);

#endif /* ENABLE_BOOT_SERVER_ON_HOST */
#endif /* IS_COORD */

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

#endif //G3_APP_BOOT_SRV_TBL_H_

/******************* (C) COPYRIGHT 2013 STMicroelectronics *****END OF FILE****/
