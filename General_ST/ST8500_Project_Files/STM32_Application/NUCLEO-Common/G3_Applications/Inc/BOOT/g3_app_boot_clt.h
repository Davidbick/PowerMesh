/*****************************************************************************
*   @file    g3_app_boot_clt.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Boot Client application.
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
#ifndef G3_APP_BOOT_CLT_H_
#define G3_APP_BOOT_CLT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup G3_App_Boot_Clt G3 Boot Client Application
  * @{
  */

/**
  * @}
  */


/** @defgroup G3_App_Boot_Clt_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Boot_Clt_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <g3_comm.h>
#include <hi_adp_sap_interface.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot_Clt
  * @{
  */

/** @addtogroup G3_App_Boot_Clt_Exported_Code
  * @{
  */

#if !IS_COORD
#if ENABLE_BOOT_CLIENT_ON_HOST

/* Custom types */
typedef enum boot_clt_event_enum
{
	BOOT_CLT_EV_NONE = 0,						/**< @brief No event ID */
	BOOT_CLT_EV_TIMER_EXPIRED,					/**< @brief TIMER EXPIRED event ID */
	BOOT_CLT_EV_RECEIVED_DEVICE_START_REQ,		/**< @brief START REQUEST event ID */
	BOOT_CLT_EV_RECEIVED_DISCOVERY_CNF,			/**< @brief DISCOVERY COMPLETE event ID */
	BOOT_CLT_EV_RECEIVED_DEVICE_PANSORT_REQ,	/**< @brief PANSORT REQUEST event ID */
	BOOT_CLT_EV_RECEIVED_NETWORK_JOIN_CNF,		/**< @brief NETWORK JOIN CONFIRM event ID */
	BOOT_CLT_EV_RECEIVED_ROUTE_DISCOVERY_CNF,	/**< @brief ROUTE DISCOVERY CONFIRM event ID */
	BOOT_CLT_EV_RECEIVED_DEVICE_LEAVE_REQ,		/**< @brief LEAVE REQUEST event ID */
	BOOT_CLT_EV_RECEIVED_NETWORK_LEAVE_CNF,		/**< @brief NETWORK LEAVE CONFIRM event ID */
	BOOT_CLT_EV_RECEIVED_NETWORK_LEAVE_IND,		/**< @brief NETWORK LEAVE INDICATION event ID */
	BOOT_CLT_EV_CNT								/**< @brief Number of events */
} boot_clt_event_t;

typedef enum boot_clt_state_enum
{
  BOOT_CLT_ST_DISCONNECTED,					/**< @brief DISCONNECTED state ID */
  BOOT_CLT_ST_WAIT_TIMER,					/**< @brief WAIT TIMER state ID */
  BOOT_CLT_ST_WAIT_DISCOVER_CNF,			/**< @brief DISCOVER state ID */
  BOOT_CLT_ST_WAIT_PANSORT_REQ,				/**< @brief PANSORT state ID */
  BOOT_CLT_ST_BOOTSTRAPPING,		        /**< @brief NETWORK JOIN/BOOTSTRAPPING state ID */
  BOOT_CLT_ST_ROUTING,						/**< @brief ROUTING state ID */
  BOOT_CLT_ST_CONNECTED,					/**< @brief CONNECTED state ID */
  BOOT_CLT_ST_LEAVING,						/**< @brief LEAVING state ID */
  BOOT_CLT_ST_CNT							/**< @brief Number of states */
} boot_clt_state_t;

/**
  * @brief Set of events for node connecting or connected to the bootstrap client
  */
typedef enum boot_clt_eap_event_enum
{
	BOOT_CLT_EAP_EV_NONE = 0x00,		/**< @brief No event ID */
	BOOT_CLT_EAP_EV_RECEIVED_FIRST,		/**< @brief EAP #1 Message event ID */
	BOOT_CLT_EAP_EV_RECEIVED_THIRD,		/**< @brief EAP #3 Message event ID */
	BOOT_CLT_EAP_EV_RECEIVED_ACCEPT,	/**< @brief EAP Accept event ID */
	BOOT_CLT_EAP_EV_RECEIVED_DECLINE,	/**< @brief EAP Accept event ID */
	BOOT_CLT_EAP_EV_RECEIVED_PARAM,		/**< @brief EAP Accept event ID */
	BOOT_CLT_EAP_EV_RECEIVED_KICK,		/**< @brief KICK message event ID (no effect in handshake) */
	BOOT_CLT_EAP_EV_CNT					/**< @brief Number of events */
} boot_clt_eap_event_t;

/**
  * @brief Set of states for node connecting or connected to the bootstrap client
  */
typedef enum boot_srv_eap_state_enum
{
    BOOT_CLT_EAP_ST_WAIT_FIRST = 0,		/**< @brief Waiting for EAP #1 message */
    BOOT_CLT_EAP_ST_WAIT_THIRD,			/**< @brief Waiting for EAP #3 message */
	BOOT_CLT_EAP_ST_WAIT_ACCEPT,		/**< @brief Waiting for EAP Accept message */
    BOOT_CLT_EAP_ST_CNT					/**< @brief Number of states */
} boot_clt_eap_state_t;


/* Custom types */
#pragma pack(push, 1)

/**
  * @brief  Boot client internal data structure
  */
typedef struct boot_client_str
{
    boot_clt_event_t		curr_event;					/**< @brief The current event of the FSM */
    boot_clt_state_t		curr_state;					/**< @brief The current state of the FSM */

    /* Bootstrap */
    uint8_t					pan_count;
    ADP_PanDescriptor_t 	pan_descriptor[ADP_MAX_NUM_PANDESCR];
    uint8_t					pan_index;
    uint8_t 				join_tries;

    /* Connection info */
    uint16_t 				pan_id;
    uint16_t 				short_address;
    uint16_t				lba_addr;
    uint8_t					media_type;

    /* Fast Restore */
    bool					fast_restore;
} boot_client_t;

#pragma pack(pop)

/* Public functions */
void g3_app_boot_clt_init(void);
void g3_app_boot_clt(void *payload);

bool g3_app_boot_clt_msg_needed( const g3_msg_t *g3_msg);
void g3_app_boot_clt_msg_handler(const g3_msg_t *g3_msg);
void g3_app_boot_clt_req_handler(const g3_msg_t *g3_msg);

void g3_app_boot_clt_start(void);

void g3_app_boot_clt_timeoutCallback(void *argument);

#endif /* ENABLE_BOOT_CLIENT_ON_HOST */
#endif /* !IS_COORD */

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

#endif /* G3_APP_BOOT_CLT_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
