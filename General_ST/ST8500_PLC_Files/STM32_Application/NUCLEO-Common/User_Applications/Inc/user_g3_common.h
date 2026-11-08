/**
  ******************************************************************************
  * @file    user_g3_common.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define the interface for the Common HI functions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef USER_G3_COMMON_H
#define USER_G3_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup User_G3_Common User G3 Common
  * @{
  */

/**
  * @}
  */

/** @defgroup User_G3_Common_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup User_G3_Common_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Include files -------------------------------------------------------------*/
#include <g3_comm.h>
#include <hi_msgs_impl.h>
#include <hi_ip_sap_interface.h>

/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_G3_Common
  * @{
  */

/** @addtogroup User_G3_Common_Exported_Code
  * @{
  */

/* User event macros */
#define USER_EVENT_RISEN(event_pos)	((user_term_fsm.user_events  	   & (1 << event_pos)) ==  (1 << event_pos))
#define USER_EVENT_OK(event_pos)	((user_term_fsm.user_events_status & (1 << event_pos)) ==  (1 << event_pos))

/* User G3 events */
typedef enum usrevt_g3_str
{
	USEREVT_G3_ERROR_IND = 0,
	USEREVT_G3_DBGTOOLS_CNF,
	USEREVT_G3_RFCONFIGSET_CNF,
	USEREVT_G3_SET_CNF,
	USEREVT_G3_G3LIBSWRESETCNF,
#if IS_COORD
	USEREVT_G3_BOOT_SRV_START_CNF,
	USEREVT_G3_BOOT_SRV_STOP_CNF,
	USEREVT_G3_BOOT_SRV_LEAVE_IND,
	USEREVT_G3_BOOT_SRV_KICK_CNF,
	USEREVT_G3_BOOT_SRV_JOIN_IND,
	USEREVT_G3_BOOT_SRV_GETPSK_IND,
	USEREVT_G3_BOOT_SRV_SETPSK_CNF,
#else
	USEREVT_G3_BOOT_DEV_START_CNF,
	USEREVT_G3_BOOT_DEV_LEAVE_CNF,
	USEREVT_G3_BOOT_DEV_LEAVE_IND,
	USEREVT_G3_BOOT_DEV_PANSORT_IND,
	USEREVT_G3_BOOT_DEV_PANSORT_CNF,
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
	USEREVT_G3_BOOT_SRV_REKEYING_CNF,
#endif
	USEREVT_G3_UDP_DATA_CNF,
	USEREVT_G3_UDP_DATA_IND,
	USEREVT_G3_UDP_CONN_SET_CNF,
	USEREVT_G3_ICMP_ECHO_CNF,
	USEREVT_G3_ICMP_ECHOREP_IND,
	USEREVT_G3_ICMP_ECHOREQ_IND,
	USEREVT_G3_NUMBER	/* Must be last */
} usrevt_g3_t;

/* User connection data */
typedef struct user_conn_str
{
	bool		 applied;				/*!<  Indicates if this connection has been set on the platform */
    uint8_t      connection_id;			/*!<  Connection ID, similar to socket handle */
    uint16_t     remote_port;			/*!<  Port of the remote sender */
    uint16_t     local_port;			/*!<  Port of the local receiver */
    ip6_addr_t   remote_address;		/*!<  IPv6 address of the remote sender */
} user_conn_t;

/* Loop-back data for UDP tests */
typedef struct loop_info_str
{
	ip6_addr_t   dest_ip_addr;       	/*!<  Only used in loopMode */
    uint32_t     remote_port;           /*!<  Destination port */
    bool         requested;           	/*!<  Decides which destination address is used */
} loop_info_t;

/* Data structure related to the next UDP packet */
typedef struct udp_packet_tx_str
{
  uint8_t  		connection_id;	/* Selected connection ID */
  ip6_addr_t	ip_addr;		/* Destination/Source IPv6 address */
  uint16_t 		port;			/* Selected UDP port */
  uint16_t 		length;			/* Length of the payload, in bytes */
  void     		*payload;		/* Pointer to the payload buffer */
} udp_packet_t;

typedef struct userg3_common_data_str
{
	uint8_t      		connection_number;     		/*!<  Counter of connections that have been set */
	uint8_t      		connection_total_number;	/*!<  Total number of defined connection */
	bool		 		online;    					/*!<  Boolean about the connection with the coordinator/server online */
	uint16_t     		pan_id;						/*!<  PAN ID of the device */
#if !IS_COORD
	uint16_t     		device_addr;				/*!<  For the device, its assigned short address */
#endif
	uint32_t 			user_events;				/*!<  User events */
	uint32_t 			user_events_status;			/*!<  Results associated to user events */

	uint32_t 			ts_start;					/*!<   Timestamp for the start */
	uint32_t 			ts_end;						/*!<   Timestamp for the end */

	hi_dbgtool_cnf_t 	platform_info;				/*!<   Platform info */
} userg3_common_t;

typedef struct connection_table_str
{
	user_conn_t  		TestConn;              		/*!<  Stores info about connection setup for UDP tests */
	user_conn_t  		TransferConn;				/*!<  Stores info about connection setup for UDP transfers */
#if ENABLE_LAST_GASP
	user_conn_t  		LastGaspConn;              	/*!<  Stores info about connection setup for UDP Last Gasp connection (of the G3 module) */
#endif
	/* Add more connections to set here (update connection_list as well) */
} connection_table_t;

/* Public functions */
void UserG3_Init(void);

/* Message handler and FSM */
bool UserG3_MsgNeeded( const g3_msg_t *msg);
void UserG3_MsgHandler(const g3_msg_t *msg);
void UserG3_FsmManager(void);

/* Debug tool info */
void UserG3_PlatformInfoRequest(void);

/* UDP connection */
const user_conn_t*	UserG3_GetUdpConnection(const uint8_t connection_index);
bool 				UserG3_ModifyUdpConnection(const uint8_t connection_id, const ip6_addr_t *ip_address, const uint16_t local_port, const uint16_t remote_port);
void 				UserG3_StartUdpConnSetup(void);

/* UDP loopback */
void UserG3_RequestUdpDataLoopback(void);
void UserG3_AbortUdpDataLoopback(void);

/* UDP data */
uint8_t 	 	UserG3_SendUdpDataToShortAddress(const uint8_t connection_id, const uint16_t dest_short_addr, void *data, const uint32_t length);
uint8_t 		UserG3_SendUdpData(              const uint8_t connection_id, const ip6_addr_t dest_ip_addr,  void *data, const uint32_t length);
udp_packet_t	UserG3_GetUdpData(               const uint8_t connection_id);
void 			UserG3_DiscardUdpData(           const uint8_t connection_id);

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
/* Re-keying */
void UserG3_SendRekeyingRequest(const uint8_t* gmk);
void UserG3_SendRekeyingAbortRequest(void);
#endif

/* Time measures */
void 		UserG3_StartTimeCounter(void);
void 		UserG3_StopTimecounter(void);
uint32_t 	UserG3_GetTimecounter(void);

/* Timer callback */
void UserG3_TimeoutCallback(void *argument);

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

#endif /* USER_G3_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
