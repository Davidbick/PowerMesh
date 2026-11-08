/**
  ******************************************************************************
  * @file    user_g3_common.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that can be used by both g3 and user tasks.
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
#include <assert.h>
#include <mem_pool.h>
#include <utils.h>
#include <debug_print.h>
#include <g3_app_config.h>
#include <g3_app_keep_alive.h>
#include <g3_app_last_gasp.h>
#include <user_g3_common.h>

#include <hi_adp_eap_psk.h>
/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_G3_Common
  * @{
  */

/** @addtogroup User_G3_Common_Private_Code
  * @{
  */

/* Macros */
#if DEBUG_USER_G3
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_USER_G3_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* Timing */
#define SET_CONN_TIMEOUT                        (5000U)		/* In ms */
#define CONFIRM_TIMEOUT                         (30000U)	/* In ms */
#ifdef  SEND_UDP_TIMEOUT
#define	READY_TIMEOUT							(5000U)		/* In ms */
#endif

/* Private constants and macros */

/* User event private macros */
#define RAISE_USER_EVENT(event_pos, msg_ok) 	userg3_common.user_events 			|= (            1UL    << event_pos);	\
												userg3_common.user_events_status	|= (((uint8_t) msg_ok) << event_pos)

/* Local types */

/* User G3 UDP FSM states */
typedef enum userg3_state_enum
{
	USER_G3_ST_UDP_INIT = 0,
	USER_G3_ST_UDP_READY,
	USER_G3_ST_WAIT_FOR_UDP_SET_CNF,
	USER_G3_ST_WAIT_FOR_UDP_DATA_CNF,
	USER_G3_ST_CNT
} userg3_state_t;

/* User G3 UDP FSM events */
typedef enum userg3_event_enum
{
	USER_G3_EV_NONE = 0,
	USER_G3_EV_SET_CONNECTION,
	USER_G3_EV_RECEIVED_SET_CNF,
	USER_G3_EV_SEND_UDP_DATA,
	USER_G3_EV_RECEIVED_UDP_DATA_CNF,
	USER_G3_EV_RECEIVED_UDP_DATA_IND,
	USER_G3_EV_CNF_TIMEOUT,
	USER_G3_EV_CNT
} userg3_event_t;

/* Private structures */

/* User G3 UDP FSM */
typedef struct userg3_fsm_info_str
{
	userg3_state_t 		curr_state;							/*!<  Current UserG3 FSM state */
	userg3_event_t 		curr_event;							/*!<  Current UserG3 FSM event */
	uint32_t			operation_counter;					/*!<  Operation counter of the User G3 FSM */
	loop_info_t  		loopback;							/*!<  Stores info about UDP loop-back */
	udp_packet_t 		data_received[CONN_NUMBER];			/*!<  Each element contains the data related to the last G3-UDP packet received */
	udp_packet_t		data_to_send;						/*!<  Contains the data related to G3UDP packet to send */
	uint8_t 			handle;								/*!<  Handle of the last G3UDP packet sent */
	uint8_t 			connection_handle[CONN_NUMBER]; 	/*!<  Handle of the last G3UDP packet sent, for each connection */
} userg3_fsm_t;

/* Private variables */
static userg3_fsm_t 	userg3_fsm;				/*!<  Contains the FSM data for the User G3 */

/**
  * @}
  */

/** @addtogroup User_G3_Common_Exported_Code
  * @{
  */

/* External Variables */
extern uint32_t user_task_operation_counter;

extern osMessageQueueId_t user_queueHandle;

extern osTimerId_t commTimerHandle;

/* Global variables */
uint8_t      		udp_handle;         /*!<  Number of sent messages */

userg3_common_t 	userg3_common;		/* Data shared with the rest of the user application */

connection_table_t	connection_table;	/* Table of user UDP connections */

user_conn_t			*connection_list[CONN_NUMBER]; /* List of user UDP connections */

#ifdef  SEND_UDP_TIMEOUT
bool	mallocFlag;
void	*mallocPtrLatch;
#endif

/**
  * @}
  */

/** @addtogroup User_G3_Common_Private_Code
  * @{
  */

/* Private function pointer type */
typedef userg3_state_t userg3_fsm_func(void);

/* Private FSM function prototypes */
static userg3_state_t userg3_fsm_default(void);
static userg3_state_t userg3_fsm_set_connection(void);
static userg3_state_t userg3_fsm_send_data(void);
static userg3_state_t userg3_fsm_next_connection(void);
static userg3_state_t userg3_fsm_set_ready(void);

/* Private FSM function pointer array */
static userg3_fsm_func *userg3_fsm_func_tbl[USER_G3_ST_CNT][USER_G3_EV_CNT] = {
/*                            NONE,               SET_CONNECTION,            RECEIVED_SET_CNF,           SEND_UDP_DATA,        RECEIVED_UDP_DATA_CNF,	RECEIVED_UDP_DATA_IND,	CNF_TIMEOUT */
/* UDP_INIT              */ { userg3_fsm_default, userg3_fsm_set_connection, userg3_fsm_default,         userg3_fsm_default,   userg3_fsm_default,		userg3_fsm_default,		userg3_fsm_default			},
/* UDP_READY             */ { userg3_fsm_default, userg3_fsm_set_connection, userg3_fsm_default,         userg3_fsm_send_data, userg3_fsm_default,		userg3_fsm_default,		userg3_fsm_default			},
/* WAIT_FOR_UDP_SET_CNF  */ { userg3_fsm_default, userg3_fsm_default,        userg3_fsm_next_connection, userg3_fsm_default,   userg3_fsm_default,		userg3_fsm_default,		userg3_fsm_next_connection	},
/* WAIT_FOR_UDP_DATA_CNF */ { userg3_fsm_default, userg3_fsm_default,        userg3_fsm_default,         userg3_fsm_default,   userg3_fsm_set_ready,	userg3_fsm_default,		userg3_fsm_set_ready		}
};

/* Private functions */

static char* userg3_translate_event(eventnot_type_t event_notification)
{
	switch (event_notification)
	{
	case rte_error 				: return "RTE error";
	case gmk_update				: return "GMK update";
	case context_table_update	: return "Context table update";
	case group_table_update 	: return "Group table update";
	case active_index_update 	: return "Active index update";
	case short_addr_update		: return "Short address update";
	case pan_id_update			: return "PAN ID update";
	case tonemap_tx				: return "Tonemap TX";
	case tonemap_rx				: return "Tonemap RX";
	case route_update			: return "Route update";
	case path_req_rx			: return "Path request RX";
	case thermal_event			: return "Thermal event";
	case surge_event			: return "Surge event";
	case rte_warning_event		: return "RTE warning event";
	case rte_polling_error		: return "RTE polling error";
	case boot_failure 			: return "Boot failure";
	case rte_band_mismatch		: return "RTE band mismatch";
	case phy_quality_rx			: return "PHY quality RX";
	case phy_quality_tx			: return "PHY quality TX";
	case ext_addr_update		: return "Extended address update";
	case mem_leak_info			: return "Memory leak";
	case timer_error			: return "Timer error";
	case phy_quality_rx_rf		: return "PHY quality RX RF";
	case phy_quality_tx_rf		: return "PHY quality TX RF";
	case adp_data_fw			: return "ADP data forward";
	case fast_restore			: return "Fast restore event";
	default						: return "Unknown";
	}
}

/**
  * @brief User G3 function that finds and sets a connection in the list to update/set.
  * @param None
  * @return True if a connection to set was found/set, false otherwise.
  */
static bool userg3_set_next_connection()
{
	bool found = false;
	uint16_t    len;
	user_conn_t *connection;

	while (userg3_common.connection_number < userg3_common.connection_total_number)
	{
		connection = connection_list[userg3_common.connection_number];

		if (connection->applied)
		{
			userg3_common.connection_number++; /* Connection already up to date, skip to the next */
		}
		else
		{
			found = true;

			/* Connection not up to date, need to set it */
			IP_UdpConnSetRequest_t *udpconnset_req = MEMPOOL_MALLOC(sizeof(IP_UdpConnSetRequest_t));

			len = hi_ipv6_udpconnsetreq_fill(udpconnset_req, connection->connection_id, connection->remote_address, connection->remote_port, connection->local_port);
			g3_send_message(HIF_TX_MSG, HIF_UDP_CONN_SET_REQ, udpconnset_req, len);

#if (DEBUG_USER_G3 >= DEBUG_LEVEL_FULL)
			ALLOC_STATIC_HEX_STRING(ip_addr_str, udpconnset_req->remote_address.u8, sizeof(udpconnset_req->remote_address.u8));
			PRINT_USER_G3_INFO( "UDP connection %d setup:\n\r"
								"\t\tIPv6 address: %s\n\r"
								"\t\tLocal port: %d\n\r"
								"\t\tRemote port: %d\n", udpconnset_req->connection_id, ip_addr_str, udpconnset_req->local_port, udpconnset_req->remote_port);
#endif
			/* Start timer to handle eventual timeout */
			osTimerStart(commTimerHandle, SET_CONN_TIMEOUT);
			break;
		}
	}

	return found;
}

/**
  * @brief User G3 FSM function that maintains the current state, with no further action.
  * @param None
  * @return The next state of the User G3 FSM (equal to the current one).
  */
static userg3_state_t userg3_fsm_default(void)
{
    userg3_fsm.curr_event = USER_G3_EV_NONE;

    return userg3_fsm.curr_state;
}

/**
  * @brief Function that handles the preparation and sending of UDP_CONN_SET_REQ message to ST8500.
  * @param None
  * @retval None
  */
static userg3_state_t userg3_fsm_set_connection(void)
{
	userg3_set_next_connection();

	userg3_fsm.curr_event = USER_G3_EV_NONE;

    return USER_G3_ST_WAIT_FOR_UDP_SET_CNF;
}

/**
  * @brief Function that handles the reset of the FSM for UDP.
  * @param None
  * @retval None
  */
static userg3_state_t userg3_fsm_next_connection(void)
{
	userg3_state_t next_state;

    if (userg3_set_next_connection())
	{
    	/* If another connection has to be set, waits for its confirm */
    	next_state = USER_G3_ST_WAIT_FOR_UDP_SET_CNF;
	}
    else
    {
    	/* Otherwise, it has finished */
    	osTimerStop(commTimerHandle);

    	next_state = USER_G3_ST_UDP_READY; /* All connections are set */
    }

	userg3_fsm.curr_event = USER_G3_EV_NONE;

    return next_state;
}

/**
  * @brief Function that handles the preparation and sending of HIF_UDP_DATA_REQ message to ST8500.
  * @param None
  * @retval None
  */
static userg3_state_t userg3_fsm_send_data(void)
{
	userg3_state_t next_state;

#ifdef  SEND_UDP_TIMEOUT
	osTimerStop(commTimerHandle);
#endif

#if !IS_COORD && ENABLE_LAST_GASP
	if (g3_app_last_gasp_is_active())
	{
		/* Data cannot be sent in Last Gasp mode */
		next_state = USER_G3_ST_UDP_READY;

		PRINT_USER_G3_CRITICAL("Cannot sent UDP data in Last Gasp");
	}
	else
	{
#endif /* !IS_COORD && ENABLE_LAST_GASP */

		ip6_addr_t          	dst_ip_addr;
		uint16_t            	dest_port;
		IP_G3UdpDataRequest_t	*udpdata_req = MEMPOOL_MALLOC(sizeof(IP_G3UdpDataRequest_t)); /* Use memory pool due to big structure size */

		if (userg3_fsm.loopback.requested == true)
		{
			userg3_fsm.loopback.requested = false; /* must be set to true everytime */

			dst_ip_addr = userg3_fsm.loopback.dest_ip_addr;
			dest_port	= userg3_fsm.loopback.remote_port;
		}
		else
		{
			dst_ip_addr = userg3_fsm.data_to_send.ip_addr;
			dest_port	= userg3_fsm.data_to_send.port;
		}

#if (DEBUG_USER_G3 >= DEBUG_LEVEL_INFO)
		ALLOC_DYNAMIC_HEX_STRING(dst_ip_addr_str, dst_ip_addr.u8, sizeof(dst_ip_addr.u8));
		PRINT_USER_G3_INFO("Sending UDP data to connection %u, IPv6 %s, destination port %u.\n", connection_id, dst_ip_addr_str, dest_port);
		FREE_DYNAMIC_HEX_STRING(dst_ip_addr_str);
#endif

		bool found = false;

		for (uint8_t i = 0; i < userg3_common.connection_total_number; i++)
		{
			if (connection_list[i]->connection_id == userg3_fsm.data_to_send.connection_id)
			{
				userg3_fsm.handle = userg3_fsm.connection_handle[i];
				found = true;
				break;
			}
		}

		assert(found);

#if IS_COORD && ENABLE_ICMP_KEEP_ALIVE
		/* Wait for Keep-Alive */
		if (g3_app_ka_in_progress())
		{
			PRINT_USER_G3_WARNING("Waiting for Keep-Alive...\n");
			while (g3_app_ka_in_progress())
			{
				utils_delay_ms(1);
			}
			PRINT_USER_G3_WARNING("Resumed\n");
		}
#endif

		/* Send the message to ST8500 */
		uint16_t len = hi_ipv6_udpdatareq_fill(udpdata_req, userg3_fsm.data_to_send.connection_id, dst_ip_addr, userg3_fsm.handle, dest_port, userg3_fsm.data_to_send.length, userg3_fsm.data_to_send.payload);
		g3_send_message(HIF_TX_MSG, HIF_UDP_DATA_REQ, udpdata_req, len);

		/* Start timer to handle eventual timeout */
		osTimerStart(commTimerHandle, CONFIRM_TIMEOUT);

		/* Free memory pool used for payload */
#ifdef  SEND_UDP_TIMEOUT
		if (mallocFlag != false)
		{
			mallocFlag = false;
		}
#endif
		MEMPOOL_FREE(userg3_fsm.data_to_send.payload);

		next_state = USER_G3_ST_WAIT_FOR_UDP_DATA_CNF;
#if !IS_COORD && ENABLE_LAST_GASP
	}
#endif

	userg3_fsm.curr_event = USER_G3_EV_NONE;

    return next_state;
}

/**
  * @brief Function that handles sets ready the FSM for UDP.
  * @param None
  * @retval None
  */
static userg3_state_t userg3_fsm_set_ready(void)
{
    userg3_fsm.curr_event = USER_G3_EV_NONE;
      
    return USER_G3_ST_UDP_READY;
}

/**
  * @brief Function that handles the G3LIB-DBGTOOL.Cnf coming from the G3 task
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_dbg_tool_cnf(const void* payload)
{
	const hi_dbgtool_cnf_t *hi_tools_cnf = payload;
	assert(hi_tools_cnf != NULL);
	userg3_common.platform_info = *hi_tools_cnf; /* Gets platform info */
}

/**
  * @brief Function that handles the G3LIB-EVENTNOTIFICATION.Ind coming from the G3 task
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_event_notification_ind(const void* payload)
{
	const G3_LIB_EventNotificationIndicationt *event_notification_ind = payload;
	assert(event_notification_ind != NULL);

	PRINT_USER_G3_WARNING("Event notification type %s (0x%X)\n", userg3_translate_event(event_notification_ind->type), event_notification_ind->type);
}

#if IS_COORD

/**
  * @brief Handles the reception of a G3BOOT-SRV-START.Cnf coming from the G3 task.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void userg3_handle_server_start_cnf(const void *payload)
{
	const BOOT_ServerStartConfirm_t *start_cnf = payload;

	if (start_cnf->status == G3_SUCCESS)
	{
		/* Sets the PAN ID for Coordinator */
		userg3_common.pan_id = PAN_ID;

		/* Goes online */
		userg3_common.online = true;
		LED_SET_ON(LED_ONLINE);
	}
}

/**
  * @brief Handles the reception of a G3BOOT-SRV-STOP.Cnf coming from the G3 task.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void userg3_handle_server_stop_cnf(const void *payload)
{
	const BOOT_ServerStopConfirm_t *stop_cnf = payload;

	if (stop_cnf->status == G3_SUCCESS)
	{
		/* Goes offline */
		userg3_common.online = false;
		LED_SET_OFF(LED_ONLINE);
	}
}

/**
  * @brief Function that handles the G3BOOT-SRV-JOIN.Ind coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_server_join_ind(const void *payload)
{
    const BOOT_ServerJoinIndication_t *join_ind = payload;
    assert(join_ind != NULL);

    PRINT_USER_G3_INFO("Received join indication from %d.\n", join_ind->short_addr);
}

#else

/**
  * @brief Function that handles the G3BOOT-DEV-START.Cnf coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_device_start_cnf(const void *payload)
{
    const BOOT_DeviceStartConfirm_t *dev_start_cnf = payload;
    assert(dev_start_cnf != NULL);

    if (dev_start_cnf->status == G3_SUCCESS)
    {
        /* Get the PAN ID and destination address for Device */
    	userg3_common.pan_id 	  = dev_start_cnf->pan_id;
    	userg3_common.device_addr = dev_start_cnf->network_addr;

        /* Goes online */
    	userg3_common.online = true;
		LED_SET_ON(LED_ONLINE);
    }
    else
    {
    	HANDLE_CNF_ERROR(HIF_BOOT_DEV_START_CNF, dev_start_cnf->status);
    }
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-LEAVE.Cnf coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_device_leave_cnf(const void *payload)
{
    const BOOT_DeviceLeaveConfirm_t *dev_leave = payload;

    if (dev_leave->status == G3_SUCCESS)
    {
    	userg3_common.online = false;
    	LED_SET_OFF(LED_ONLINE);
    }
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-LEAVE.Ind coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_device_leave_ind(const void *payload)
{
    UNUSED(payload);
    userg3_common.online = false;
    LED_SET_OFF(LED_ONLINE);
    PRINT_USER_G3_INFO("Received leave IND\n");
}

#endif

/**
  * @brief Function that handles the G3UDP-CONN-SET.Confirm coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_connection_set_cnf(const void *payload)
{
    const IP_UdpConnSetConfirm_t *conn_set_cnf = payload;
    assert(conn_set_cnf != NULL);
    HANDLE_CNF_ERROR(HIF_UDP_CONN_SET_CNF, conn_set_cnf->status);

    if (conn_set_cnf->status == G3_SUCCESS)
    {
    	connection_list[userg3_common.connection_number]->applied = true; /* Connection now up to date */
    }

    userg3_fsm.curr_event = USER_G3_EV_RECEIVED_SET_CNF;
}

/**
  * @brief Function that handles the G3UDP-DATA.Cnf coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_udp_data_cnf(const void *payload)
{
    const IP_G3UdpDataConfirm_t *udp_data_cnf = payload;
    assert(udp_data_cnf != NULL);
    
    if (udp_data_cnf->handle == userg3_fsm.handle)
	{
        userg3_fsm.curr_event = USER_G3_EV_RECEIVED_UDP_DATA_CNF;

        osTimerStop(commTimerHandle);

        HANDLE_CNF_ERROR(HIF_UDP_DATA_CNF, udp_data_cnf->status);
	}
}

/**
  * @brief Function that handles the G3UDP-DATA.Ind coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval True if the message is related the the User G3, false otherwise
  */
static bool userg3_handle_udp_data_ind(const void *payload)
{
	bool userg3_related = false;
	const IP_DataIndication_t    *ip_udp_ind   = payload;
	const IP_UdpDataIndication_t *udp_data_ind = hi_ipv6_extract_udp_from_ip(ip_udp_ind);

	userg3_fsm.curr_event = USER_G3_EV_RECEIVED_UDP_DATA_IND;

	assert(udp_data_ind != NULL);

	for (uint8_t i = 0; i < userg3_common.connection_total_number; i++)
	{
		if (	(connection_list[i]->connection_id == udp_data_ind->connection_id)
#if ENABLE_LAST_GASP
				&& (connection_list[i]->connection_id != LAST_GASP_CONN_ID		 )
#endif
		   )
		{
			userg3_related = true;
			userg3_fsm.data_received[i].length = udp_data_ind->data_len;

			if (userg3_fsm.data_received[i].payload != NULL)
			{
				MEMPOOL_FREE(userg3_fsm.data_received[i].payload); /* Free previous payload, if it wasn't freed by calling  */
				PRINT_USER_G3_WARNING("Discarded last UDP data\n");
			}

			if (userg3_fsm.data_received[i].length > 0)
			{
				/* Only in case of loopback test */
				if (userg3_fsm.loopback.requested == true)
				{
					/* The source PAN ID is not needed */
					userg3_fsm.loopback.dest_ip_addr	= udp_data_ind->source_address;
					userg3_fsm.loopback.remote_port     = udp_data_ind->source_port;
				}

				userg3_fsm.data_received[i].connection_id = udp_data_ind->connection_id;
				userg3_fsm.data_received[i].ip_addr       = udp_data_ind->source_address;
				userg3_fsm.data_received[i].port		  = udp_data_ind->source_port;

				userg3_fsm.data_received[i].payload = MEMPOOL_MALLOC(userg3_fsm.data_received[i].length);


#if (DEBUG_USER_G3 >= DEBUG_LEVEL_INFO)
				ALLOC_DYNAMIC_HEX_STRING(src_ip_addr_str, udp_data_ind->source_address.u8, sizeof(udp_data_ind->source_address.u8));
				PRINT_USER_G3_INFO("Received UDP packet of %u bytes from connection %u, IPv6 %s, remote port %u\n", udp_data_ind->data_len, udp_data_ind->connection_id, src_ip_addr_str, udp_data_ind->source_port);
				FREE_DYNAMIC_HEX_STRING(src_ip_addr_str);
#endif
				if (userg3_fsm.data_received[i].payload != NULL)
				{
					memcpy(userg3_fsm.data_received[i].payload, udp_data_ind->data, userg3_fsm.data_received[i].length);
				}
				else
				{
					PRINT_USER_G3_INFO("Received UDP packet is too long (%u bytes)\n", udp_data_ind->data_len);
				}
			}
			break;
		}
	}

	return userg3_related;
}

/**
  * @brief Function that handles the G3ICMP-ECHOREP.Ind coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_icmp_echorep_ind(const void *payload)
{
	UNUSED(payload);
}

/**
  * @brief Function that handles the G3ICMP-ECHOREQ.Indication coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void userg3_handle_icmp_echoreq_ind(const void *payload)
{
	UNUSED(payload); /* Do nothing */
}

/**
  * @}
  */

/** @addtogroup User_G3_Common_Exported_Code
  * @{
  */

/* Public functions */

/**
  * @brief Function that initializes the User G3 variables.
  * @param None
  * @retval None
  */
void UserG3_Init(void)
{
	userg3_fsm.curr_state = USER_G3_ST_UDP_INIT;
	userg3_fsm.curr_event = USER_G3_EV_NONE;

    memset(&userg3_fsm.loopback.dest_ip_addr , 0, sizeof(userg3_fsm.loopback.dest_ip_addr ));
	userg3_fsm.loopback.remote_port 	    = TEST_REMOTE_PORT;
	userg3_fsm.loopback.requested         	= false;

    /* UDP packets */
	memset(&userg3_fsm.data_received, 0, sizeof(userg3_fsm.data_received));
	memset(&userg3_fsm.data_to_send,  0, sizeof(userg3_fsm.data_to_send));

	/* No set connection at startup */
	userg3_common.connection_number       = 0;
	userg3_common.connection_total_number = NUM_OF_ELEM(connection_list);

	/* Initially disconnected */
	userg3_common.online = false;

	userg3_common.pan_id = 0;

    /* Initialize G3-UDP shared data structure */
    userg3_common.user_events       	= 0;
    userg3_common.user_events_status	= 0;

    userg3_common.ts_start 			= 0;
    userg3_common.ts_end   			= 0;
    memset(&userg3_common.platform_info, 0, sizeof(userg3_common.platform_info));

    /* Test connection parameters */
	connection_table.TestConn.applied			 = false;
	connection_table.TestConn.connection_id		 = TEST_CONN_ID;
	memset(&connection_table.TestConn.remote_address, 0, sizeof(ip6_addr_t));
	connection_table.TestConn.local_port		 = TEST_LOCAL_PORT;
	connection_table.TestConn.remote_port		 = TEST_REMOTE_PORT;

	/* Transfer connection parameters */
	connection_table.TransferConn.applied		 = false;
	connection_table.TransferConn.connection_id  = TRANSFER_CONN_ID;
	memset(&connection_table.TransferConn.remote_address, 0, sizeof(ip6_addr_t));
	connection_table.TransferConn.remote_port 	 = TRANSFER_REMOTE_PORT;
	connection_table.TransferConn.local_port  	 = TRANSFER_LOCAL_PORT;

	/* Fill connection list */
	uint8_t list_index = 0;
	connection_list[list_index++] = &connection_table.TestConn;
	connection_list[list_index++] = &connection_table.TransferConn;

#if ENABLE_LAST_GASP
	/* Transfer connection parameters */
	connection_table.LastGaspConn.applied		 = false;
	connection_table.LastGaspConn.connection_id  = LAST_GASP_CONN_ID;
	memset(&connection_table.LastGaspConn.remote_address, 0, sizeof(ip6_addr_t));
	connection_table.LastGaspConn.remote_port 	 = LAST_GASP_LOCAL_PORT;
	connection_table.LastGaspConn.local_port  	 = LAST_GASP_REMOTE_PORT;

	connection_list[list_index++] = &connection_table.LastGaspConn;
#endif
	/* Add all connections from connection_table */
}



/**
  * @brief Function that checks if a message is needed by the User G3 application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed, 'false' otherwise.
  */
bool UserG3_MsgNeeded(const g3_msg_t *g3_msg)
{
	switch(g3_msg->command_id)
	{
	case HIF_ERROR_IND:
	case HIF_HI_DBGTOOL_CNF:
	case HIF_HI_RFCONFIGSET_CNF:
	case HIF_G3LIB_SET_CNF:
	case HIF_G3LIB_SWRESET_CNF:
	case HIF_G3LIB_EVENT_IND:
#if IS_COORD
	case HIF_BOOT_SRV_START_CNF:
	case HIF_BOOT_SRV_STOP_CNF:
	case HIF_BOOT_SRV_LEAVE_IND:
	case HIF_BOOT_SRV_KICK_CNF:
	case HIF_BOOT_SRV_JOIN_IND:
	case HIF_BOOT_SRV_GETPSK_IND:
	case HIF_BOOT_SRV_SETPSK_CNF:
#else
	case HIF_BOOT_DEV_START_CNF:
	case HIF_BOOT_DEV_LEAVE_CNF:
    case HIF_BOOT_DEV_LEAVE_IND:
	case HIF_BOOT_DEV_PANSORT_IND:
	case HIF_BOOT_DEV_PANSORT_CNF:
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
	case HIF_BOOT_SRV_REKEYING_CNF:
#endif
	case HIF_UDP_DATA_CNF:
	case HIF_UDP_DATA_IND:
	case HIF_UDP_CONN_SET_CNF:
	case HIF_ICMP_ECHO_CNF:
	case HIF_ICMP_ECHO_REP_IND:
	case HIF_ICMP_ECHO_REQ_IND:
		return true;
		break;
	default:
		return false;
		break;
	}
}

/**
  * @brief Function that handles the messages coming from the G3 task.
  * @param g3_msg Pointer to the received G3 message
  * @retval None
  */
void UserG3_MsgHandler(const g3_msg_t *g3_msg)
{
	bool cnf_ok = false;

	if (g3_msg->size > 0)
	{
		assert(g3_msg->payload != NULL);
		cnf_ok = (*((uint8_t*) g3_msg->payload) == G3_SUCCESS); /*The first byte is always the status, if present */
	}

	/* Indications and DBG-TOOL confirm are always ok, the other confirms are ok only if the first byte is equal to 0 (G3_SUCCESS) */
    switch (g3_msg->command_id)
    {
    case HIF_ERROR_IND:
        RAISE_USER_EVENT(USEREVT_G3_ERROR_IND, true);
        break;
    case HIF_HI_DBGTOOL_CNF:
        userg3_handle_dbg_tool_cnf(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_DBGTOOLS_CNF, true);
        break;
    case HIF_HI_RFCONFIGSET_CNF:
        RAISE_USER_EVENT(USEREVT_G3_RFCONFIGSET_CNF, cnf_ok);
        break;
    case HIF_G3LIB_SET_CNF:
        RAISE_USER_EVENT(USEREVT_G3_SET_CNF, cnf_ok);
        break;
    case HIF_G3LIB_SWRESET_CNF:
        RAISE_USER_EVENT(USEREVT_G3_G3LIBSWRESETCNF, cnf_ok);
        break;
    case HIF_G3LIB_EVENT_IND:
    	userg3_handle_event_notification_ind(g3_msg->payload);
		break;
#if IS_COORD
    case HIF_BOOT_SRV_START_CNF:
    	userg3_handle_server_start_cnf(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_START_CNF, cnf_ok);
        break;
    case HIF_BOOT_SRV_STOP_CNF:
    	userg3_handle_server_stop_cnf(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_STOP_CNF, cnf_ok);
        break;
    case HIF_BOOT_SRV_LEAVE_IND:
        RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_LEAVE_IND, true);
        break;
    case HIF_BOOT_SRV_KICK_CNF:
        RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_KICK_CNF, cnf_ok);
        break;
    case HIF_BOOT_SRV_JOIN_IND:
        userg3_handle_server_join_ind(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_JOIN_IND, true);
        break;
    case HIF_BOOT_SRV_GETPSK_IND:
    	RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_GETPSK_IND, true);
        break;
    case HIF_BOOT_SRV_SETPSK_CNF:
    	RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_SETPSK_CNF, cnf_ok);
        break;
#else
    case HIF_BOOT_DEV_START_CNF:
        userg3_handle_device_start_cnf(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_DEV_START_CNF, cnf_ok);
        break;
    case HIF_BOOT_DEV_LEAVE_CNF:
    	userg3_handle_device_leave_cnf(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_DEV_LEAVE_CNF, cnf_ok);
        break;
    case HIF_BOOT_DEV_LEAVE_IND:
    	userg3_handle_device_leave_ind(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_BOOT_DEV_LEAVE_IND, true);
        break;
    case HIF_BOOT_DEV_PANSORT_IND:
        RAISE_USER_EVENT(USEREVT_G3_BOOT_DEV_PANSORT_IND, true);
        break;
    case HIF_BOOT_DEV_PANSORT_CNF:
		RAISE_USER_EVENT(USEREVT_G3_BOOT_DEV_PANSORT_CNF, cnf_ok);
		break;
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
    case HIF_BOOT_SRV_REKEYING_CNF:
		RAISE_USER_EVENT(USEREVT_G3_BOOT_SRV_REKEYING_CNF, cnf_ok);
		break;
#endif
    case HIF_UDP_DATA_CNF:
    	userg3_handle_udp_data_cnf(g3_msg->payload);
		RAISE_USER_EVENT(USEREVT_G3_UDP_DATA_CNF, cnf_ok);
        break;
    case HIF_UDP_DATA_IND:
    	if (userg3_handle_udp_data_ind(g3_msg->payload))
    	{
    		RAISE_USER_EVENT(USEREVT_G3_UDP_DATA_IND, true);
    	}
        break;
    case HIF_UDP_CONN_SET_CNF:
    	userg3_handle_connection_set_cnf(g3_msg->payload);
		RAISE_USER_EVENT(USEREVT_G3_UDP_CONN_SET_CNF, cnf_ok);
        break;
    case HIF_ICMP_ECHO_CNF:
        RAISE_USER_EVENT(USEREVT_G3_ICMP_ECHO_CNF, cnf_ok);
        break;
    case HIF_ICMP_ECHO_REP_IND:
        userg3_handle_icmp_echorep_ind(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_ICMP_ECHOREP_IND, true);
        break;
    case HIF_ICMP_ECHO_REQ_IND:
        userg3_handle_icmp_echoreq_ind(g3_msg->payload);
        RAISE_USER_EVENT(USEREVT_G3_ICMP_ECHOREQ_IND, true);
        break;
    default:
        break;
    }
}

/**
  * @brief Function that handles the state of the User G3 FSM.
  * @param None
  * @retval None
  * @note Entry function called from User task infinite execution loop.
  */
void UserG3_FsmManager(void)
{
	userg3_fsm.operation_counter++;

    userg3_fsm.curr_state = userg3_fsm_func_tbl[userg3_fsm.curr_state][userg3_fsm.curr_event]();
}

/**
  * @brief Function that provides board platform information (ST8500 FW version, PLC info, ...).
  * @note This function prepares and sends the USER_HI_TOOLS_REQ message to the ST8500.
  * @param None
  * @retval None
  */
void UserG3_PlatformInfoRequest(void)
{
    PRINT_USER_G3_INFO("Requesting platform info...\n");

    hi_dbgtool_req_t *dbgtool_req = MEMPOOL_MALLOC(sizeof(hi_dbgtool_req_t));

    uint16_t len = hi_hostif_dbgtoolreq_fill(dbgtool_req, HI_TOOL_INFO, HI_TOOL_NO_CONF);
    g3_send_message(HIF_TX_MSG, HIF_HI_DBGTOOL_REQ, dbgtool_req, len);
}

/**
  * @brief Function that returns the pointer to the connection structure at a given index, to read its data.
  * @param connection_index The ID of the connection to modify
  * @retval None
  */
const user_conn_t* UserG3_GetUdpConnection(const uint8_t connection_index)
{
	return connection_list[connection_index];
}

/**
  * @brief Function that modifies the parameters of a given connection.
  * @param connection_id The ID of the connection to modify
  * @param ip_address Poiter to the remote IPv6 address to receive from (set all zeroes to receive from all IPv6 addresses)
  * @param local_port The local UDP port for this connection (cannot be 0)
  * @param remote_port The remote UDP port to receive from (set 0 to receive from any remote port)
  * @retval "true" if the connection ID was found and the parameters were modified, "false" otherwise
  * @note Call UserG3_StartUdpConnSetup to apply the modification
  */
bool UserG3_ModifyUdpConnection(const uint8_t connection_id, const ip6_addr_t *ip_address, const uint16_t local_port, const uint16_t remote_port)
{
	bool found = false;

	/* Local port cannot be 0 */
	assert_param(local_port != 0);

	for (uint8_t i = 0; i < userg3_common.connection_total_number; i++)
	{
		if (connection_list[i]->connection_id == connection_id)
		{
			found = true;

			/* The connection now must be updated on the platform */
			connection_list[i]->applied		= false;

			/* Sets the ports of the UDP connection */
			connection_list[i]->remote_port	= remote_port;
			connection_list[i]->local_port	= local_port;

			/* Sets the IPv6 remote address */
			memcpy(&connection_list[i]->remote_address, ip_address, sizeof(ip6_addr_t));

			break;
		}
	}

	if (!found)
	{
		PRINT_USER_G3_CRITICAL("Connection with ID=%u not found, cannot modify\n", connection_id);
	}

	return found;
}

/**
  * @brief This function starts the setup of all UDP connections
  * and triggers the SET_CONNECTION event.
  * @param None
  * @retval None
  */
void UserG3_StartUdpConnSetup(void)
{
	userg3_common.connection_number = 0;

    /* Makes the FSM request the connection setup */
    userg3_fsm.curr_event = USER_G3_EV_SET_CONNECTION;

	if (user_task_operation_counter == userg3_fsm.operation_counter)
	{
		/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
    }
}

/**
  * @brief This function enables the loopback mode (destination = source address of last received udp data ind)
  * @param None
  * @retval None
  */
void UserG3_RequestUdpDataLoopback(void)
{
	userg3_fsm.loopback.requested = true;
}

/**
  * @brief This function disables the loopback mode (destination = source address of last received udp data ind).
  * @param None
  * @retval None
  */
void UserG3_AbortUdpDataLoopback(void)
{
	userg3_fsm.loopback.requested = false;
}

/**
  * @brief This function prepares the buffer with the data to send before sending a G3UDP-DATA.Request message to the platform.
  * @Note Short address version.
  * @param connection_id The ID of the connection to use.
  * @param dest_short_addr Destination short address.
  * @param data Pointer to the buffer containing the data to send as payload. If it points to a memory pool, it is freed in case of error.
  * @param length Size of the buffer containing the data to send as payload.
  * @retval UDP handle of the G3UDP packet sent
  */
uint8_t UserG3_SendUdpDataToShortAddress(const uint8_t connection_id, const uint16_t dest_short_addr, void *data, const uint32_t length)
{
	ip6_addr_t dest_ip_addr;

	/* Compute the IPv6 remote address, from the short address */
	hi_ipv6_set_ipaddr(&dest_ip_addr, userg3_common.pan_id, dest_short_addr);

    return UserG3_SendUdpData(connection_id, dest_ip_addr, data, length);
}

/**
  * @brief This function prepares the buffer with the data to send before sending a G3UDP-DATA.Request message to the platform.
  * @Note Allocates a memory pool if the data argument is not pointing a memory pool already. IPv6 address version.
  * @param connection_id The ID of the connection to use.
  * @param dest_ip_addr Destination IPv6 address.
  * @param data Pointer to the buffer containing the data to send as payload. If it points to a memory pool, it is freed in case of error.
  * @param length Size of the buffer containing the data to send as payload.
  * @retval UDP handle of the G3UDP packet sent
  */
uint8_t UserG3_SendUdpData(const uint8_t connection_id, const ip6_addr_t dest_ip_addr, void *data, const uint32_t length)
{
	bool success = false;
	bool found = false;
	bool buffer_is_mem_pool = mem_pool_check(data);
	uint8_t handle = 0;

	for (uint8_t i = 0; i < userg3_common.connection_total_number; i++)
	{
		if (connection_list[i]->connection_id == connection_id)
		{
			found = true;

			if (userg3_fsm.curr_state == USER_G3_ST_UDP_READY)
			{
				void *payload_data;

				if (buffer_is_mem_pool)
				{
					/* If a memory pool was already allocated for the buffer (data), then it is forwarded without copying it */
					payload_data = data;
				}
				else
				{
					/* Otherwise, provides to allocate the memory pool and copies the data from the buffer */
					payload_data = MEMPOOL_MALLOC(length);
#ifdef  SEND_UDP_TIMEOUT
					mallocFlag = true;
					mallocPtrLatch = payload_data;
#endif
					if (payload_data != NULL)
					{
						memcpy(payload_data, data, length);
					}
					else
					{
						PRINT_USER_G3_CRITICAL("Error during memory pool allocation.\n");
					}
				}

				if (payload_data != NULL)
				{
					/* Fill UDP FSM data */
					userg3_fsm.data_to_send.connection_id = connection_list[i]->connection_id;
					userg3_fsm.data_to_send.ip_addr       = dest_ip_addr;
					userg3_fsm.data_to_send.port		  = connection_list[i]->remote_port;
					userg3_fsm.data_to_send.length		  = length;
					userg3_fsm.data_to_send.payload		  = payload_data;

					if (osKernelLock() == osOK)
					{
						userg3_fsm.connection_handle[connection_id] = ++udp_handle;

						osKernelUnlock();
					}
					else
					{
						Error_Handler();
					}

					handle = userg3_fsm.connection_handle[connection_id];

					success = true;

					userg3_fsm.curr_event = USER_G3_EV_SEND_UDP_DATA;
#ifdef  SEND_UDP_TIMEOUT
					/* Start timer to handle eventual timeout */
					osTimerStart(commTimerHandle, READY_TIMEOUT);
#endif
					if (user_task_operation_counter == userg3_fsm.operation_counter)
					{
						/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
						RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
					}
				}
			}
			else
			{
				PRINT_USER_G3_CRITICAL("Cannot send UDP data, not ready (state: %u).\n", userg3_fsm.curr_state);
			}
			break;
		}
	}

	if (!found)
	{
		PRINT_USER_G3_CRITICAL("Cannot send UDP data, connection %u not found.\n", connection_id);
	}

	if ((buffer_is_mem_pool) && (!success))
	{
		/* If failed and a memory pool was allocated, frees it */
		MEMPOOL_FREE(data);
	}

    return handle;
}

/**
  * @brief This function returns the buffer with the data received with the last G3UDP-DATA.Indication message.
  * @param connection_id ID of the connection where the packet was received
  * @retval The last UDP packet received
  */
udp_packet_t UserG3_GetUdpData(const uint8_t connection_id)
{
	assert(connection_id < CONN_NUMBER);

	return userg3_fsm.data_received[connection_id];
}

/**
  * @brief This function discards the data received from the last G3UDP-DATA.Indication message.
  * @param connection_id ID of the connection where the packet was received
  * @retval None
  */
void UserG3_DiscardUdpData(const uint8_t connection_id)
{
	assert(connection_id < CONN_NUMBER);

	if (userg3_fsm.data_received[connection_id].payload != NULL)
	{
		MEMPOOL_FREE(userg3_fsm.data_received[connection_id].payload); /* Free previous UDP data payload received */
	}
	else
	{
		PRINT_USER_G3_WARNING("No UDP data to discard\n");
	}
}

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING
/**
  * @brief Function that starts a Re-keying procedure.
  * @note This function can only be used when the Boot Server is embedded in the application.
  * @param gmk Buffer containing the new GMK to set for the PAN
  * @retval None
  */
void UserG3_SendRekeyingRequest(const uint8_t* gmk)
{
	BOOT_ServerRekeyingRequest_t *rekeying_req = MEMPOOL_MALLOC(sizeof(BOOT_ServerRekeyingRequest_t));

	uint16_t len = hi_boot_srvrekeyingreq_fill(rekeying_req, gmk);
	g3_send_message(BOOT_SRV_MSG, HIF_BOOT_SRV_REKEYING_REQ, rekeying_req, len);
}

/**
  * @brief Function that aborts a Re-keying procedure, if in progress.
  * @note This function can only be used when the Boot Server is embedded in the application.
  * @param None
  * @retval None
  */
void UserG3_SendRekeyingAbortRequest(void)
{
	g3_send_message(BOOT_SRV_MSG, HIF_BOOT_SRV_ABORT_RK_REQ, NULL, 0);
}

#endif /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING */


bool UserG3_TxInProgress(void)
{
	return (userg3_fsm.curr_state == USER_G3_ST_WAIT_FOR_UDP_DATA_CNF);
}
/**
  * @brief Function that starts counting the time in order to measure it.
  * @param None
  * @retval None
  */
void UserG3_StartTimeCounter(void)
{
    userg3_common.ts_start = HAL_GetTick();
}

/**
  * @brief Function that stops counting the time in order to measure it.
  * @param None
  * @retval None
  */
void UserG3_StopTimecounter(void)
{
    userg3_common.ts_end = HAL_GetTick();
}

/**
  * @brief Function that computes the loopback duration.
  * @param None
  * @retval None
  */
uint32_t UserG3_GetTimecounter(void)
{
    return (userg3_common.ts_end - userg3_common.ts_start);
}

/**
  * @brief Callback function of the commTimer FreeRTOStimer. Handles all timeout events of the user interface, triggering the FSM.
  * @param argument Unused argument.
  * @retval None
  */
void UserG3_TimeoutCallback(void *argument)
{
	UNUSED(argument);
	userg3_fsm.curr_event = USER_G3_EV_CNF_TIMEOUT;

	if (userg3_fsm.curr_state == USER_G3_ST_WAIT_FOR_UDP_DATA_CNF)
	{
		PRINT_USER_G3_WARNING("UDP data confirm timeout\n");

		/* Raise negative user event for User Terminal */
		RAISE_USER_EVENT(USEREVT_G3_UDP_DATA_CNF, false);
	}
	else if (userg3_fsm.curr_state == USER_G3_ST_WAIT_FOR_UDP_SET_CNF)
	{
		PRINT_USER_G3_WARNING("UDP connection set confirm timeout\n");

		/* Raise negative user event for User Terminal */
		RAISE_USER_EVENT(USEREVT_G3_UDP_CONN_SET_CNF, false);
	}
#ifdef  SEND_UDP_TIMEOUT
	else if (userg3_fsm.curr_state == USER_G3_ST_UDP_READY)
	{
		if ((mallocFlag != false) && (mallocPtrLatch == userg3_fsm.data_to_send.payload))
		{
			MEMPOOL_FREE(userg3_fsm.data_to_send.payload);
			mallocFlag = false;
			userg3_fsm.curr_event=USER_G3_EV_NONE;
		}
	}
#endif
	else
	{
		Error_Handler();
	}

	/* Unblocks the User Task to execute UserG3 and UserIf FSMs */
	RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
