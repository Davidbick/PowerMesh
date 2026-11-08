/*****************************************************************************
*   @file    g3_app_last_gasp.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains the implementation of the Last Gasp activity.
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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <debug_print.h>
#include <g3_comm.h>
#include <mem_pool.h>
#include <utils.h>
#include <hi_msgs_impl.h>
#include <hif_g3_common.h>
#include <g3_app_config.h>
#include <g3_app_boot.h>
#include <g3_app_last_gasp.h>
#include <main.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_LastGasp
  * @{
  */

/** @addtogroup G3_App_LastGasp_Exported_Code
  * @{
  */

#if ENABLE_LAST_GASP

/* External variables */

/**
  * @}
  */

/** @addtogroup G3_App_LastGasp_Private_Code
  * @{
  */

/* Definitions */

#if DEBUG_G3_LAST_GASP
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_G3_LAST_GASP_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* Multicast address (used for broadcast) */
#define IPV6_MULTICAST_ADDR 	{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}

/* Last Gasp message ID */
#define LAST_GASP_MSG_ID		0x1000

#define LAST_GASP_MAXHOPS		15	/* Maximum number of hops for broadcast messages */

/* Custom types */
typedef enum last_gasp_dest_mode_enum
{
	last_gasp_broadcast,
	last_gasp_unicast,
} last_gasp_dest_mode_t;

typedef enum last_gasp_event_enum
{
    LAST_GASP_EV_NONE = 0,
	LAST_GASP_EV_JOINED_PAN,
	LAST_GASP_EV_RECEIVED_SET_CNF,
	LAST_GASP_EV_RECEIVED_DATA_CNF,
	LAST_GASP_EV_LEFT_PAN,
	LAST_GASP_EV_CNT
} last_gasp_event_t;

typedef enum last_gasp_state_enum
{
	LAST_GASP_ST_DISCONNECTED = 0,
	LAST_GASP_ST_NORMAL,
	LAST_GASP_ST_GET_NEXT_HOP,
	LAST_GASP_ST_WAIT_DATA_CNF,
	LAST_GASP_ST_DEAD,
	LAST_GASP_ST_CNT
} last_gasp_state_t;

typedef struct last_gasp_info_str
{
	last_gasp_state_t	curr_state;
	last_gasp_event_t	curr_event;

	/* This device info */
	uint8_t				last_gasp_activated;
    uint16_t    		pan_id;
    uint16_t    		short_address;
    uint8_t				handle;
} last_gasp_fsm_t;

#pragma pack(push, 1)

typedef struct last_gasp_msg_str
{
    uint16_t    	id;
    uint16_t    	gasped_short_addr;
    uint16_t    	hop_count;
    uint16_t		visited_device[LAST_GASP_MAXHOPS];
} last_gasp_msg_t;

#pragma pack(pop)


/* External Variables */
extern osMessageQueueId_t g3_queueHandle;

extern uint8_t udp_handle;         /*!<  Number of sent messages */

#if !IS_COORD

/* Private variables */
static last_gasp_fsm_t	last_gasp_fsm;

/* Private function pointer type */
typedef last_gasp_state_t g3_last_gasp_fsm_func(void);

/* Private FSM function prototypes */
static last_gasp_state_t g3_last_gasp_fsm_default(void);
static last_gasp_state_t g3_last_gasp_fsm_connect(void);
static last_gasp_state_t g3_last_gasp_fsm_send_gasp(void);
static last_gasp_state_t g3_last_gasp_fsm_depart(void);
static last_gasp_state_t g3_last_gasp_fsm_disconnect(void);

/* Private FSM function pointer array */
static g3_last_gasp_fsm_func *g3_last_gasp_fsm_func_tbl[LAST_GASP_ST_CNT][LAST_GASP_EV_CNT] = {
/*                  NONE,                     JOINED_PAN,                 RECEIVED_SET_CNF,              RECEIVED_DATA_CNF,        LEFT_PAN                   */
/* DISCONNECTED  */{g3_last_gasp_fsm_default, g3_last_gasp_fsm_connect,   g3_last_gasp_fsm_default,      g3_last_gasp_fsm_default, g3_last_gasp_fsm_default    },
/* NORMAL        */{g3_last_gasp_fsm_default, g3_last_gasp_fsm_default,   g3_last_gasp_fsm_send_gasp,	 g3_last_gasp_fsm_default, g3_last_gasp_fsm_disconnect },
/* GET_NEXT_HOP  */{g3_last_gasp_fsm_default, g3_last_gasp_fsm_default,   g3_last_gasp_fsm_default,      g3_last_gasp_fsm_default, g3_last_gasp_fsm_default    },
/* WAIT_DATA_CNF */{g3_last_gasp_fsm_default, g3_last_gasp_fsm_default,   g3_last_gasp_fsm_default,      g3_last_gasp_fsm_depart,  g3_last_gasp_fsm_default    },
/* DEAD          */{g3_last_gasp_fsm_default, g3_last_gasp_fsm_default,   g3_last_gasp_fsm_default,      g3_last_gasp_fsm_default, g3_last_gasp_fsm_default    }
};

/* Private functions */

/**
  * @brief Sends a G3UDP-DATA.Request in broadcast or to the LBA in unicast
  * @param dest_mode Determines the transmission mode (broadcast or unicast)
  * @param dest_short_addr Short address of the device the next destination device, unused in case of broadcast
  * @param gasped_short_addr Short address of the device that detected the power supply shortage
  * @param hop_count Number of hops of the request
  * @param device_list The array with the list of short addresses the message has already passed through, NULL for the device starting the Last Gasp.
  * @return None
  */
static void g3_last_gasp_send_data_req(last_gasp_dest_mode_t dest_mode, uint16_t dest_short_addr, uint16_t gasped_short_addr, uint16_t hop_count, uint16_t* device_list)
{
	uint16_t 				len;
	ip6_addr_t          	ip_dst_addr;

	IP_G3UdpDataRequest_t	*udpdata_req = MEMPOOL_MALLOC(sizeof(IP_G3UdpDataRequest_t)); /* Uses memory pool due to big structure size */

	last_gasp_msg_t last_gasp_msg;

	if (dest_mode == last_gasp_broadcast)
	{
		uint8_t broadcast_arr[IP_IPV6_ADDR128_UINT8_LEN] = IPV6_MULTICAST_ADDR;
		memcpy(ip_dst_addr.u8, broadcast_arr, IP_IPV6_ADDR128_UINT8_LEN);

		PRINT_G3_LAST_GASP_INFO("Sending Last Gasp in broadcast\n");
	}
	else
	{
		/* Compute the IPv6 destination address, from the short address of the next hop */
		hi_ipv6_set_ipaddr(&ip_dst_addr, last_gasp_fsm.pan_id, dest_short_addr);

		PRINT_G3_LAST_GASP_INFO("Sending Last Gasp to device %u in unicast\n", dest_short_addr);
	}

	last_gasp_msg.id = LAST_GASP_MSG_ID;
	last_gasp_msg.gasped_short_addr = gasped_short_addr;
	last_gasp_msg.hop_count = hop_count;

	if (device_list != NULL)
	{
		/* Forwarded message case, copies the given list */
		memcpy(last_gasp_msg.visited_device, device_list, sizeof(last_gasp_msg.visited_device));
	}
	else
	{
		/* Starting message case */
		last_gasp_msg.visited_device[0] = gasped_short_addr; /* The gasped short address is equal to the device short address, in this case */

		for (uint32_t i = 1; i < LAST_GASP_MAXHOPS; i++)
		{
			last_gasp_msg.visited_device[i] = 0xFF; /* Empty */
		}
	}

	if (osKernelLock() == osOK)
	{
		last_gasp_fsm.handle = ++udp_handle;

		osKernelUnlock();
	}
	else
	{
		Error_Handler();
	}

	/* Send the message to ST8500 */
	len = hi_ipv6_udpdatareq_fill(udpdata_req, LAST_GASP_CONN_ID, ip_dst_addr, last_gasp_fsm.handle, LAST_GASP_REMOTE_PORT, sizeof(last_gasp_msg), (uint8_t*) &last_gasp_msg);
	g3_send_message(HIF_TX_MSG, HIF_UDP_DATA_REQ, udpdata_req, len);
}

/**
  * @brief G3 Last Gasp FSM function that maintains the current state, with no further action.
  * @param None
  * @return The next state of the G3 Last Gasp FSM (equal to the current one).
  */
static last_gasp_state_t g3_last_gasp_fsm_default(void)
{
	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	return last_gasp_fsm.curr_state;
}

/**
  * @brief G3 Last Gasp FSM function that changes state to normal.
  * @param None
  * @return The next state of the G3 Last Gasp FSM.
  */
static last_gasp_state_t g3_last_gasp_fsm_connect(void)
{
	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	return LAST_GASP_ST_NORMAL;
}

/**
  * @brief G3 Last Gasp FSM function that sends the first Last Gasp message (sent in case of power supply shortage).
  * @param None
  * @return The next state of the G3 Last Gasp FSM.
  */
static last_gasp_state_t g3_last_gasp_fsm_send_gasp(void)
{
	g3_last_gasp_send_data_req(last_gasp_broadcast, 0, last_gasp_fsm.short_address, 0, NULL);

	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	return LAST_GASP_ST_WAIT_DATA_CNF;
}

/**
  * @brief G3 Last Gasp FSM function that de-activates the FSM of Last Gasp.
  * @param None
  * @return The next state of the G3 Last Gasp FSM.
  */
static last_gasp_state_t g3_last_gasp_fsm_depart(void)
{
	PRINT_G3_LAST_GASP_WARNING("Last Gasp activated\n");

	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	return LAST_GASP_ST_DEAD;
}

/**
  * @brief G3 Last Gasp FSM function that changes state to disconnected.
  * @param None
  * @return The next state of the G3 Last Gasp FSM.
  */
static last_gasp_state_t g3_last_gasp_fsm_disconnect(void)
{
	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	return LAST_GASP_ST_DISCONNECTED;
}

/**
  * @brief Function that handles the FSM of the Last Gasp activity.
  * @param None
  * @retval None
  */
static void g3_last_gasp_fsm_manager(void)
{
    last_gasp_fsm.curr_state = g3_last_gasp_fsm_func_tbl[last_gasp_fsm.curr_state][last_gasp_fsm.curr_event]();
}

/**
  * @brief Function that handles the reception of a G3LIB-SET.Confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_last_gasp_handle_set_cnf(const void *payload)
{
	const G3_LIB_SetAttributeConfirm_t *set_attr_cnf = payload;

	if ((set_attr_cnf->attribute_id.id == ADP_LASTGASP_ID) && (set_attr_cnf->attribute_id.index == 0))
	{
		if (set_attr_cnf->status == G3_SUCCESS)
		{
			last_gasp_fsm.curr_event = LAST_GASP_EV_RECEIVED_SET_CNF;
		}
		else
		{
#if (DEBUG_G3_LAST_GASP >= DEBUG_LEVEL_CRITICAL)
			HANDLE_CNF_ERROR(HIF_G3LIB_SET_CNF, set_attr_cnf->status);
#endif
		}
	}
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-START.Confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_last_gasp_handle_dev_start_cnf(const void *payload)
{
    const BOOT_DeviceStartConfirm_t *dev_start = payload;

    if (dev_start->status == G3_SUCCESS)
    {
    	last_gasp_fsm.pan_id        = dev_start->pan_id;
		last_gasp_fsm.short_address = dev_start->network_addr;

		last_gasp_fsm.curr_event = LAST_GASP_EV_JOINED_PAN;
    }
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-LEAVE.Confirm (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_last_gasp_handle_dev_leave_cnf(const void *payload)
{
    const BOOT_DeviceLeaveConfirm_t *dev_leave = payload;

	last_gasp_fsm.curr_event = LAST_GASP_EV_LEFT_PAN;

    if (dev_leave->status == G3_SUCCESS)
    {
    	last_gasp_fsm.pan_id 		= MAC_BROADCAST_PAN_ID;
    	last_gasp_fsm.short_address = MAC_BROADCAST_SHORT_ADDR;
    }
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-LEAVE.Indication (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_last_gasp_handle_dev_leave_ind(const void *payload)
{
    UNUSED(payload);

	last_gasp_fsm.curr_event = LAST_GASP_EV_LEFT_PAN;

	last_gasp_fsm.pan_id 		= MAC_BROADCAST_PAN_ID;
	last_gasp_fsm.short_address = MAC_BROADCAST_SHORT_ADDR;
}


/**
  * @brief Function that handles the G3UDP-DATA.Confirm coming from the G3 task.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_last_gasp_handle_data_cnf(const void *payload)
{
    const IP_G3UdpDataConfirm_t *udp_data_cnf = payload;
    assert(udp_data_cnf != NULL);

    if (udp_data_cnf->handle == last_gasp_fsm.handle)
    {
		HANDLE_CNF_ERROR(HIF_UDP_DATA_CNF, udp_data_cnf->status);

		if (last_gasp_fsm.curr_state == LAST_GASP_ST_WAIT_DATA_CNF)
		{
			last_gasp_fsm.curr_event = LAST_GASP_EV_RECEIVED_DATA_CNF;
		}
    }
}

#endif /* !IS_COORD */

/**
  * @brief Function that handles the G3UDP-DATA.Indication. Handles Last Gasp forwarding.
  * @param payload Pointer to the payload of the received G3 message.
  * @retval None
  */
static void g3_last_gasp_handle_data_ind(const void *payload)
{
	const IP_DataIndication_t    *ip_udp_ind   = payload;
	const IP_UdpDataIndication_t *udp_data_ind = hi_ipv6_extract_udp_from_ip(ip_udp_ind);

	assert(udp_data_ind != NULL);

	if (udp_data_ind->connection_id == LAST_GASP_CONN_ID)
	{
		last_gasp_msg_t* last_gasp_msg = (last_gasp_msg_t*) udp_data_ind->data;

		if (last_gasp_msg->id == LAST_GASP_MSG_ID)
		{
			uint16_t udp_src_pan_id;
			uint16_t udp_src_addr;

			hi_ipv6_get_saddr_panid(udp_data_ind->source_address, &udp_src_pan_id, &udp_src_addr);

			PRINT_G3_LAST_GASP_WARNING("Received Last Gasp of device %u from device %u, hop count: %u\n", last_gasp_msg->gasped_short_addr, udp_src_addr, last_gasp_msg->hop_count);
#if !IS_COORD

			/* In order to avoid bounces, each message carries the list of visited short addresses */
			bool not_visited_yet = true;

			for (uint32_t i = 0; i < LAST_GASP_MAXHOPS; i++)
			{
				if (last_gasp_msg->visited_device[i] == last_gasp_fsm.short_address)
				{
					/* The address of this device is in the list: no forward */
					not_visited_yet = false;
					break;
				}
			}

			if (not_visited_yet)
			{
				for (uint32_t i = 0; i < LAST_GASP_MAXHOPS; i++)
				{
					if (last_gasp_msg->visited_device[i] == MAC_BROADCAST_SHORT_ADDR)
					{
						/* The address of this device is written in the first free entry of the list */
						last_gasp_msg->visited_device[i] = last_gasp_fsm.short_address;
						break;
					}
				}

				if (last_gasp_fsm.last_gasp_activated)
				{
					/* Last gasp mode, in case of power supply shortage */

					/* Since the PLC route is compromised, a broadcast (RF) message is sent */
					if (last_gasp_msg->hop_count < LAST_GASP_MAXHOPS)
					{
						last_gasp_msg->hop_count++;
						g3_last_gasp_send_data_req(last_gasp_broadcast, 0, last_gasp_msg->gasped_short_addr, last_gasp_msg->hop_count, last_gasp_msg->visited_device);
					}
					else
					{
						PRINT_G3_LAST_GASP_WARNING("Max hop count reached, Last Gasp discarded\n");
					}
				}
				else
				{
					/* Normal  mode, directly sends the message to the coordinator since the route should be intact */
					g3_last_gasp_send_data_req(last_gasp_unicast, COORD_ADDRESS, last_gasp_msg->gasped_short_addr, 0, last_gasp_msg->visited_device);
				}
			}
			else
			{
				PRINT_G3_LAST_GASP_WARNING("Already forwarded\n");
			}
#endif
		}
	}
}

/**
  * @}
  */

/** @addtogroup G3_App_LastGasp_Exported_Code
  * @{
  */

/**
  * @brief Checks if a message is needed by the G3 Last Gasp application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed, 'false' otherwise
  */
bool g3_app_last_gasp_msg_needed(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
#if !IS_COORD
    case HIF_G3LIB_SET_CNF:
    case HIF_BOOT_DEV_LEAVE_CNF:
    case HIF_BOOT_DEV_LEAVE_IND:
    case HIF_BOOT_DEV_START_CNF:
	case HIF_UDP_DATA_CNF:
#endif
	case HIF_UDP_DATA_IND:
        return true;
    default:
    	return false;
    }
}

/**
  * @brief Handles the reception of a G3 Last Gasp application message.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_last_gasp_msg_handler(const g3_msg_t *g3_msg)
{
    /* If the device type is set, handles the message depending on the device type */
	switch (g3_msg->command_id)
	{
#if !IS_COORD
	case HIF_G3LIB_SET_CNF:
		g3_last_gasp_handle_set_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_START_CNF:
		g3_last_gasp_handle_dev_start_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_CNF:
		g3_last_gasp_handle_dev_leave_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_IND:
		g3_last_gasp_handle_dev_leave_ind(g3_msg->payload);
		break;
	case HIF_UDP_DATA_CNF:
		g3_last_gasp_handle_data_cnf(g3_msg->payload);
		break;
#endif /* !IS_COORD */
	case HIF_UDP_DATA_IND:
		g3_last_gasp_handle_data_ind(g3_msg->payload);
		break;
	default:
		break;
	}
}

#if !IS_COORD

/**
  * @brief Initializes the G3 Last Gasp application.
  * @param None
  * @retval None
  */
void g3_app_last_gasp_init(void)
{
	last_gasp_fsm.curr_state = LAST_GASP_ST_DISCONNECTED;
	last_gasp_fsm.curr_event = LAST_GASP_EV_NONE;

	last_gasp_fsm.last_gasp_activated 	= 0;
	last_gasp_fsm.pan_id 				= MAC_BROADCAST_PAN_ID;
	last_gasp_fsm.short_address 		= MAC_BROADCAST_SHORT_ADDR;
	last_gasp_fsm.handle				= 0;
}

/**
  * @brief Starts the G3 Last Gasp sequence. Kept light since executed inside an ISR.
  * @param None
  * @retval None
  */
void g3_app_last_gasp_start(void)
{
	/* Unblocks the G3 task to execute Last Gasp Activation */
	RTOS_PUT_MSG(g3_queueHandle, LAST_GASP_MSG, NULL);
}

/**
  * @brief Activates the G3 Last Gasp mode.
  * @param None
  * @retval None
  */
void g3_app_last_gasp_activate(void)
{
	if (last_gasp_fsm.curr_state == LAST_GASP_ST_NORMAL)
	{
		G3_LIB_SetAttributeRequest_t *set_attr_req = NULL;

		/* Activates Last Gasp mode */
		last_gasp_fsm.last_gasp_activated = 1;

		/* Sets the attribute to set the platform in Last Gasp mode */
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(last_gasp_fsm.last_gasp_activated)));
		uint16_t len = hi_g3lib_setreq_fill(set_attr_req, ADP_LASTGASP_ID, 0, &last_gasp_fsm.last_gasp_activated, sizeof(last_gasp_fsm.last_gasp_activated));
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

#if (DEBUG_G3_LAST_GASP >= DEBUG_LEVEL_FULL)
		ALLOC_DYNAMIC_HEX_STRING(attribute_value_str, set_attr_req->attribute.value, set_attr_req->attribute.len);
		PRINT_G3_LAST_GASP_INFO("Setting attribute LAST_GASP = 0x%s\n", attribute_value_str);
		FREE_DYNAMIC_HEX_STRING(attribute_value_str)
#endif
	}
	else
	{
		PRINT_G3_LAST_GASP_WARNING("Not connected to a PAN, cannot activate Last Gasp\n\r");

		HAL_NVIC_ClearPendingIRQ(USER_PUSHBUTTON_EXTI_IRQn);
		HAL_NVIC_EnableIRQ(USER_PUSHBUTTON_EXTI_IRQn);
	}
}

/**
  * @brief Returns the G3 Last Gasp mode activations status.
  * @param None
  * @retval The G3 Last Gasp mode activations status.
  */
uint8_t g3_app_last_gasp_is_active(void)
{
	return last_gasp_fsm.last_gasp_activated;
}

/**
  * @brief Executes G3 Last Gasp FSM.
  * @param None
  * @retval None
  */
void g3_app_last_gasp(void)
{
	/* Manages the current event, depending on the current state */
	g3_last_gasp_fsm_manager();
}

#endif /* !IS_COORD */


#endif /* ENABLE_LAST_GASP */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
