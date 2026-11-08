/*****************************************************************************
*   @file    g3_app_boot_srv.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Boot Server application.
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
#ifndef G3_APP_BOOT_SRV_H_
#define G3_APP_BOOT_SRV_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup G3_App_Boot_Srv G3 Boot Server Application
  * @{
  */

/**
  * @}
  */


/** @defgroup G3_App_Boot_Srv_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Boot_Srv_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <g3_comm.h>
#include <g3_boot_srv_join_entry_tbl.h>

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

#if ENABLE_BOOT_SERVER_ON_HOST

/* Custom types */
typedef enum boot_srv_event_enum
{
	BOOT_SRV_EV_NONE = 0,					/**< @brief No event ID */
	BOOT_SRV_EV_TIMER_EXPIRED,				/**< @brief TIMER EXPIRED event ID */
	BOOT_SRV_EV_RECEIVED_DISCOVERY_CNF,		/**< @brief DISCOVERY CONFIRM event ID */
	BOOT_SRV_EV_RECEIVED_NETWORK_START_CNF,	/**< @brief NETWORK START CONFIRM event ID */
	BOOT_SRV_EV_RECEIVED_LBP_CNF,			/**< @brief LBP CONFIRM event ID */
	BOOT_SRV_EV_RECEIVED_LBP_IND,			/**< @brief LBP INDICATION event ID */
	BOOT_SRV_EV_RECEIVED_SETPSK_REQ,		/**< @brief SETPSK REQUEST event ID */
	BOOT_SRV_EV_RECEIVED_STOP_REQ,			/**< @brief STOP REQUEST event ID */
	BOOT_SRV_EV_RECEIVED_KICK_REQ,			/**< @brief KICK REQUEST event ID */
	BOOT_SRV_EV_RECEIVED_REKEYING_REQ,		/**< @brief REKEYING REQUEST event ID */
	BOOT_SRV_EV_RECEIVED_ABORT_RK_REQ,		/**< @brief ABORT REKEYING REQUEST event ID */
	BOOT_SRV_EV_CNT							/**< @brief Number of events */
} boot_srv_event_t;

typedef enum boot_srv_state_enum
{
  BOOT_SRV_ST_INACTIVE,						/**< @brief INACTIVE state ID  */
  BOOT_SRV_ST_WAIT_DISCOVER_CNF,			/**< @brief DISCOVER state ID  */
  BOOT_SRV_ST_WAIT_NETWORK_START_CNF,		/**< @brief START NETWORK state ID  */
  BOOT_SRV_ST_ACTIVE,						/**< @brief ACTIVE state ID  */
  BOOT_SRV_ST_CNT							/**< @brief Number of states  */
} boot_srv_state_t;

typedef enum boot_srv_substate_enum
{
	boot_srv_idle,
	boot_srv_kickig,
	boot_srv_rekeying_step_set_gmk,
	boot_srv_rekeying_step_send_gmk,
	boot_srv_rekeying_step_activate_gmk,
	boot_srv_rekeying_step_deactivate_gmk,
	boot_srv_rekeying_step_set_gmk_index,
	boot_srv_rekeying_step_set_gmk_index_wait_cnf,
} boot_srv_substate_t;

typedef enum boot_srv_rk_err_enum
{
	rekeying_error_none,
	rekeying_error_table_full,
	rekeying_error_procedure,
	rekeying_error_msg_2,
	rekeying_error_msg_4,
	rekeying_error_param,
	rekeying_error_set_attribute,
	rekeying_error_no_device,
	rekeying_error_abort
} boot_srv_rk_err_t;

#endif /* ENABLE_BOOT_SERVER_ON_HOST */

typedef enum boot_conn_status_enum
{
	boot_state_disconnected	 = 0x01, /* Bit masks: use only powers of 2 */
	boot_state_bootstrapping = 0x02,
	boot_state_connected	 = 0x04,
} boot_conn_state_t;

typedef struct boot_device_str
{
	boot_conn_state_t	conn_state;
#if ENABLE_ICMP_KEEP_ALIVE
	uint8_t  			lives;
    uint32_t 			last_ka_ts;
#endif /* ENABLE_ICMP_KEEP_ALIVE */
	uint16_t 			short_addr;
	uint8_t  			ext_addr[MAC_ADDR64_SIZE];
#if ENABLE_BOOT_SERVER_ON_HOST
	uint8_t  			media_type;
	uint8_t  			disable_bkp;
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
} boot_device_t;

/**
  * @brief  6LoWPAN_Boot_Server FSM Internal Data structure
  */
typedef struct boot_server_str
{
	uint16_t 				connected_devices_number;						/**< @brief The current number of connected devices */
	boot_device_t 			connected_devices[BOOT_MAX_NUM_JOINING_NODES];	/**< @brief The array of connected devices */
#if ENABLE_BOOT_SERVER_ON_HOST
    boot_srv_event_t		curr_event;					/**< @brief The current event of the FSM */
    boot_srv_state_t		curr_state;					/**< @brief The current state of the FSM */
    boot_srv_substate_t 	curr_substate;				/**< @brief The current sub-state of the FSM, for re-keying/kicking */

    uint16_t				pan_id;						/**< @brief The ID representing the PAN that is going to be set up */
    uint16_t				short_addr;					/**< @brief The short address of the LBS */

    uint8_t					nsdu_handle;				/**< @brief NsduHandle to be used for the next ADPM-LBP.request */
#if BOOT_SERVER_IDS_LEN != 0
    uint8_t 				ids[BOOT_SERVER_IDS_LEN];	/**< @brief The IDS of the server */
#else
    uint8_t 				ids[MAC_ADDR64_SIZE];		/**< @brief The IDS of the server */
#endif
    /* Kick-out */
    uint8_t  				kick_handle;

    /* Re-keying/GMK update */
    boot_srv_rk_err_t  		rekeying_error : 8;					/**< @brief Error occurred during Re-keying/kicking */

    uint8_t 				gmk[MAC_KEY_NUMBER][MAC_KEY_SIZE];	/**< @brief The two GMK of the server */
	uint8_t 				gmk_index;							/**< @brief The index of the active GMK*/
	uint8_t 				gmk_index_new;						/**< @brief The index of the future active GMK*/

	uint32_t 				rekeying_index;
	uint32_t 				rekeying_count;
	uint32_t 				rekeyed_count;
	uint32_t 				activated_count;
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
} boot_server_t;

/* Public functions */
#if ENABLE_BOOT_SERVER_ON_HOST
void g3_app_boot_srv_init(void);
void g3_app_boot_srv(void *payload);

bool g3_app_boot_srv_msg_needed( const g3_msg_t *g3_msg);
void g3_app_boot_srv_msg_handler(const g3_msg_t *g3_msg);
void g3_app_boot_srv_req_handler(const g3_msg_t *g3_msg);
void g3_app_boot_srv_rekeying(   const g3_msg_t *g3_msg);

void g3_app_boot_srv_timeoutCallback(void *argument);
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
#endif /* IS_COORD ST */

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

#endif /* G3_APP_BOOT_SRV_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
