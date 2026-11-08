/*****************************************************************************
*   @file    g3_app_keep_alive.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains the implementation of the Keep-Alive activity.
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
#include <mem_pool.h>
#include <utils.h>
#include <hi_msgs_impl.h>
#include <g3_app_config.h>
#include <g3_app_boot.h>
#include <g3_app_keep_alive.h>
#include <main.h>

#if ENABLE_ICMP_KEEP_ALIVE

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_KeepAlive
  * @{
  */

/** @addtogroup G3_App_KeepAlive_Private_Code
  * @{
  */

/**
  * @}
  */

/** @addtogroup G3_App_KeepAlive_Private_Code
  * @{
  */

/* Definitions */

#if DEBUG_G3_KA
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_G3_KA_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* KEEP-ALIVE TIMING */

/* Time between Keep-Alive checks */
#define KEEP_ALIVE_CHECK_PERIOD             (120000U)	/* In ms, using a too short period can clog up the communication */

/* For 2 or more devices, time between each device Keep-Alive check (ping/kick) */
#define KEEP_ALIVE_CHECK_NEXT_DELAY         (1000U)		/* In ms */

/* Time between Keep-Alive check retries */
#define KEEP_ALIVE_CHECK_RETRY_DELAY         (500U)		/* In ms */

/* Timeout for ICMP echo CNF/IND reception */
#define KEEP_ALIVE_CNF_TIMEOUT              (90000U)	/* In ms */
#define KEEP_ALIVE_IND_TIMEOUT              (20000U)	/* In ms */

/* Timeouts for kick (coord) / leave (device) request */
#define KA_DEVICE_LEAVE_TIME                (KEEP_ALIVE_CNF_TIMEOUT + KEEP_ALIVE_IND_TIMEOUT + KEEP_ALIVE_CHECK_PERIOD) /* In ms */

/* KA message ID */
#define KA_MSG_ID		0x10		/* ID value of the Keep-Alive message */

/* Custom types */
typedef enum ka_panc_event_enum
{
    KA_EV_PANC_NONE = 0,
    KA_EV_PANC_PING_DEVICE,
    KA_EV_PANC_RECEIVED_ECHO_CNF,
    KA_EV_PANC_RECEIVED_ECHO_IND,
    KA_EV_PANC_TIMEOUT_ON_ECHO_IND,
    KA_EV_PANC_CNT
} ka_event_t;

typedef enum ka_panc_state_enum
{
    KA_ST_PANC_READY_TO_SEND_ECHO = 0,
    KA_ST_PANC_WAIT_FOR_ECHO_ANSWER,
    KA_ST_PANC_WAIT_FOR_ECHO_IND,
    KA_ST_PANC_CNT
} ka_state_t;

typedef struct ka_info_str
{
    uint16_t    	ka_pan_id;
#if IS_COORD
    ka_state_t		curr_state;
	ka_event_t		curr_event;
	uint16_t        ping_index;
	uint8_t         ping_handle;
	g3_result_t     ping_status;
#else
    uint16_t    	ka_short_addr;
    uint32_t		last_ka_ts;
#endif
	uint8_t			icmp_handle;
} ka_info_t;

#pragma pack(push, 1)

typedef struct ka_msg_str
{
    uint8_t     	id;
    uint16_t    	pan_id;
    uint16_t    	short_addr;
} ka_msg_t;

#pragma pack(pop)

/* External variables */
#if IS_COORD
extern boot_server_t boot_server;
#endif

extern osMessageQueueId_t	g3_queueHandle;

extern osTimerId_t			kaTimerHandle;

/* Private variables */
static ka_info_t	ka_info;

#if IS_COORD

/* Private function pointer type */
typedef ka_state_t g3_ka_coord_fsm_func(ka_event_t);

/* Private FSM function prototypes */
static ka_state_t g3_ka_fsm_default(          	ka_event_t event);
static ka_state_t g3_ka_fsm_ping_device(	    ka_event_t event);
static ka_state_t g3_ka_fsm_received_echo_cnf(	ka_event_t event);
static ka_state_t g3_ka_fsm_received_echo_ind(	ka_event_t event);
static ka_state_t g3_ka_fsm_timeout_on_echo_ind(ka_event_t event);

/* Private FSM function pointer array */
static g3_ka_coord_fsm_func *g3_ka_coord_fsm_func_tbl[KA_ST_PANC_CNT][KA_EV_PANC_CNT] = {
/*                         NONE,              PING_DEVICE,           RECEIVED_ECHO_CNF,           RECEIVED_ECHO_IND,           TIMEOUT_ON_ECHO_IND*/
/* READY_TO_SEND_ECHO   */{g3_ka_fsm_default, g3_ka_fsm_ping_device, g3_ka_fsm_default,           g3_ka_fsm_default,           g3_ka_fsm_default            },
/* WAIT_FOR_ECHO_ANSWER */{g3_ka_fsm_default, g3_ka_fsm_default,     g3_ka_fsm_received_echo_cnf, g3_ka_fsm_received_echo_ind, g3_ka_fsm_timeout_on_echo_ind},
/* WAIT_FOR_ECHO_IND    */{g3_ka_fsm_default, g3_ka_fsm_default,     g3_ka_fsm_default,           g3_ka_fsm_received_echo_ind, g3_ka_fsm_timeout_on_echo_ind}
};

/* Private functions */

/**
  * @brief Prepares to ping the next device, if there are still more to ping, or sets the next K.A. event (coordinator only).
  * @retval None
  */
static void g3_ka_coord_load_next_device()
{
	uint32_t time_to_next_check;
	bool roundtable_completed = false;

	uint16_t initial_index = ka_info.ping_index;
	boot_device_t  *device = NULL;

	do
	{
		ka_info.ping_index++;

		if (ka_info.ping_index >= BOOT_MAX_NUM_JOINING_NODES)
		{
			/* When the index restarts from 0, more time is waited to trigger the next keep-alive event */
			ka_info.ping_index = 0;
			roundtable_completed = true;
		}

		if (boot_server.connected_devices[ka_info.ping_index].conn_state == boot_state_connected)
		{
			device = &boot_server.connected_devices[ka_info.ping_index];
			break;
		}
	}
	while (ka_info.ping_index != initial_index);

	if (device != NULL)
	{
		/* Checks if all connected devices have been pinged */
		if (roundtable_completed)
		{
			LED_SET_OFF(LED_KEEPALIVE);

			/* No more devices, sets standard amount of time, if not urgent */
			if ((HAL_GetTick() - device->last_ka_ts + KEEP_ALIVE_CHECK_PERIOD) < KA_DEVICE_LEAVE_TIME)
			{
				time_to_next_check =  KEEP_ALIVE_CHECK_PERIOD;
			}
			else
			{
				time_to_next_check = KEEP_ALIVE_CHECK_NEXT_DELAY;
			}
		}
		else
		{
			/* If there are more devices to ping, sets a small amount of time before the next Keep-Alive ping event */
			time_to_next_check = KEEP_ALIVE_CHECK_NEXT_DELAY;
		}

#if (DEBUG_G3_KA >= DEBUG_LEVEL_FULL)
		PRINT_G3_KA_INFO("Will ping device %u in %u seconds.\n", device->short_addr, time_to_next_check/configTICK_RATE_HZ);
#endif
		osTimerStart(kaTimerHandle, time_to_next_check);
	}
	else
	{
		LED_SET_OFF(LED_KEEPALIVE);

		PRINT_G3_KA_WARNING("No device to ping\n");
	}
}

/**
  * @brief Starts the check for the next device in the K.A. device table, by triggering the KA_EV_PANC_CHECK_DEVICES event (coordinator only).
  * @param unused Unused parameter (can be set as NULL).
  * @retval None
  */
static void g3_ka_coord_timer_event(void *unused)
{
	UNUSED(unused);

	if (ka_info.curr_state == KA_ST_PANC_READY_TO_SEND_ECHO)
	{
		/* Triggers ping */
		ka_info.curr_event = KA_EV_PANC_PING_DEVICE;
	}
	else
	{
		/* Triggers timeout */
		ka_info.curr_event = KA_EV_PANC_TIMEOUT_ON_ECHO_IND;
	}

	RTOS_PUT_MSG(g3_queueHandle, KA_MSG, NULL);
}

/**
  * @brief G3 Keep-Alive FSM function that maintains the current state, with no further action.
  * @param event Current G3 Keep-Alive event.
  * @return The next state of the G3 Keep-Alive FSM (equal to the current one).
  */
static ka_state_t g3_ka_fsm_default(ka_event_t event)
{
	UNUSED(event);

	/* Resets event */
    ka_info.curr_event = KA_EV_PANC_NONE;

    return ka_info.curr_state;
}

/**
  * @brief G3 Keep-Alive FSM function that pings the current device, depending on how many times it was pinged unsuccessfully before.
  * @param event Current G3 Keep-Alive event.
  * @return The next state of the G3 Keep-Alive FSM.
  */
static ka_state_t g3_ka_fsm_ping_device(ka_event_t event)
{
	UNUSED(event);

	ka_state_t next_state;
	ip6_addr_t ip_dst_addr;
	uint16_t len;
	ka_msg_t ka_payload;
	IP_G3IcmpDataRequest_t	*icmp_data_req = MEMPOOL_MALLOC(sizeof(IP_G3IcmpDataRequest_t));
    
    boot_device_t * device = &boot_server.connected_devices[ka_info.ping_index];

    if (device->conn_state == boot_state_connected)
	{
    	LED_SET_ON(LED_KEEPALIVE);

		/* Sends new ping to the device */
		ka_payload.id = KA_MSG_ID;
		ka_payload.pan_id = ka_info.ka_pan_id;
		ka_payload.short_addr = device->short_addr;

#if (DEBUG_G3_KA >= DEBUG_LEVEL_FULL)
		PRINT_G3_KA_INFO("Pinging device %u (PAN: %X).\n", device->short_addr, ka_info.ka_pan_id);
#endif
		/* Compute the IPv6 remote address, from the short address */
		hi_ipv6_set_ipaddr(&ip_dst_addr, ka_info.ka_pan_id, device->short_addr);

		ka_info.ping_handle = ++ka_info.icmp_handle;

		device->last_ka_ts = HAL_GetTick();

		/* Prepare and send request ST8500 */
		len = hi_ipv6_echoreq_fill(icmp_data_req, ip_dst_addr, ka_info.ping_handle, sizeof(ka_payload), (uint8_t*) &ka_payload);
		g3_send_message(HIF_TX_MSG, HIF_ICMP_ECHO_REQ, icmp_data_req, len);

		/* Now both IND and CNF can be received in any order so we go to a state that can receive both */
		next_state = KA_ST_PANC_WAIT_FOR_ECHO_ANSWER;
	}
    else
    {
    	/* If the device is missing at the given index, silently loads the next one */
		g3_ka_coord_load_next_device();

		next_state = KA_ST_PANC_READY_TO_SEND_ECHO;
    }

	/* Resets event */
	ka_info.curr_event = KA_EV_PANC_NONE;

    return next_state;
}

/**
  * @brief G3 Keep-Alive FSM function that acknowledges the reception of the ICMP echo confirmation.
  * @param event Current G3 Keep-Alive event.
  * @return The next state of the G3 Keep-Alive FSM.
  */
static ka_state_t g3_ka_fsm_received_echo_cnf(ka_event_t event)
{
	UNUSED(event);

	ka_state_t next_state;
    
    boot_device_t *device = &boot_server.connected_devices[ka_info.ping_index];

	if (device->conn_state == boot_state_connected)
	{
		if (ka_info.ping_status == G3_SUCCESS)
		{
			next_state = KA_ST_PANC_WAIT_FOR_ECHO_IND;

			/* If the ping was successful and it received only the ECHO CNF, waits for the ECHO IND */
			osTimerStart(kaTimerHandle, KEEP_ALIVE_IND_TIMEOUT);
		}
		else
		{
			next_state = KA_ST_PANC_READY_TO_SEND_ECHO;

			if (ka_info.ping_status == G3_BUSY)
			{
				PRINT_G3_KA_WARNING("Platform busy while pinging device %u, retrying in %u ms\n", device->short_addr, KEEP_ALIVE_CHECK_RETRY_DELAY);
				osTimerStart(kaTimerHandle, KEEP_ALIVE_CHECK_RETRY_DELAY);
			}
			else
			{
				/* Necessary but not sufficient for determinate death */
				PRINT_G3_KA_WARNING("Error on echo CNF for device %u\n", device->short_addr);

				device->lives--;

				if (device->lives == 0)
				{
					/* If the ping failed and the device has no lives left, kicks out the device */
					g3_app_boot_kick_device(device);
				}

				g3_ka_coord_load_next_device();
			}
		}
	}
	else
	{
		next_state = KA_ST_PANC_READY_TO_SEND_ECHO;

		g3_ka_coord_load_next_device();
	}

	/* Resets the event */
	ka_info.curr_event = KA_EV_PANC_NONE;

    return next_state;
}

/**
  * @brief G3 Keep-Alive FSM function that acknowledges the reception of the ICMP echo indication.
  * @param event Current G3 Keep-Alive event.
  * @return The next state of the G3 Keep-Alive FSM.
  */
static ka_state_t g3_ka_fsm_received_echo_ind(ka_event_t event)
{
	UNUSED(event);
    
    boot_device_t *device = &boot_server.connected_devices[ka_info.ping_index];

	if (device->conn_state == boot_state_connected)
	{
		if (device->lives != KEEP_ALIVE_LIVES_N)
		{
			PRINT_G3_KA_WARNING("Device with short address %u restored (lives: %u->%u)\n", device->short_addr, device->lives, KEEP_ALIVE_LIVES_N);
			device->lives = KEEP_ALIVE_LIVES_N;
		}
#if (DEBUG_G3_KA >= DEBUG_LEVEL_FULL)
		else
		{
			PRINT_G3_KA_INFO("Device with short address %u is alive\n", device->short_addr);
		}
#endif
	}
	else
	{
		PRINT_G3_KA_WARNING("Echo IND from disconnected device\n");
	}

	g3_ka_coord_load_next_device();

    /* Resets event */
    ka_info.curr_event = KA_EV_PANC_NONE;
    
    return KA_ST_PANC_READY_TO_SEND_ECHO;
}

/**
 * @brief G3 Keep-Alive FSM function that acknowledges the timeout of the ICMP echo indication.
 * @param event Current G3 Keep-Alive event.
 * @return The next state of the G3 Keep-Alive FSM.
 */
static ka_state_t g3_ka_fsm_timeout_on_echo_ind(ka_event_t event)
{
	UNUSED(event);

	boot_device_t * device = &boot_server.connected_devices[ka_info.ping_index];

	if (device->conn_state == boot_state_connected)
	{
		/* Necessary but not sufficient for determinate death */
		PRINT_G3_KA_WARNING("Did not receive echo IND for device %u\n", device->short_addr);

		device->lives--;

		if (device->lives == 0)
		{
			/* If the ping failed and the device has no lives left, kicks out the device */
			g3_app_boot_kick_device(device);
		}
	}

	/* Proceeds to the next device in list */
	g3_ka_coord_load_next_device();

	/* Resets event */
	ka_info.curr_event = KA_EV_PANC_NONE;

	return KA_ST_PANC_READY_TO_SEND_ECHO;
}


/**
  * @brief Function that handles the FSM of the keep-alive activity.
  * @param None
  * @retval None
  */
static void g3_ka_fsm_manager(void)
{
    ka_info.curr_state = g3_ka_coord_fsm_func_tbl[ka_info.curr_state][ka_info.curr_event](ka_info.curr_event);
}

/**
  * @brief Function that handles the reception of a device leave indication (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_server_leave_ind(const void *payload)
{
    UNUSED(payload);

    /* Stops Keep Alive if no device is left */
    if (boot_server.connected_devices_number == 0)
	{
    	g3_app_ka_stop();
	}
}

/**
  * @brief Function that handles the reception of a server kick confirm (coordinator only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_server_kick_cnf(const void *payload)
{
	UNUSED(payload);

	/* Stops Keep Alive if no device is left */
	if (boot_server.connected_devices_number == 0)
	{
		g3_app_ka_stop();
	}
}

/**
  * @brief Function that handles the reception of a server join indication (coordinator only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_server_join_ind(const void *payload)
{
	UNUSED(payload);

    /* Starts timer for keep-alive, if not already running */
    g3_app_ka_start();
}

/**
  * @brief Function that handles the reception of a ICMP echo confirmation (coordinator only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_echo_cnf(const void *payload)
{
    const IP_G3IcmpDataConfirm_t *echo_cnf = payload;

    if (echo_cnf->handle == ka_info.ping_handle)
    {
    	HANDLE_CNF_ERROR(HIF_ICMP_ECHO_CNF, echo_cnf->status);

		ka_info.ping_status = echo_cnf->status;
        ka_info.curr_event = KA_EV_PANC_RECEIVED_ECHO_CNF;
        RTOS_PUT_MSG(g3_queueHandle, KA_MSG, NULL);
    }
}

/**
  * @brief Function that handles the reception of a ICMP echo indication (coordinator only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_echorep_ind(const void *payload)
{
    const IP_DataIndication_t *echo_ind = payload;
    const IP_IcmpDataIndication_t *data_ind = hi_ipv6_extract_icmp_from_ip(echo_ind);
    uint16_t src_pan_id, src_short_addr;
    
    boot_device_t * device = &boot_server.connected_devices[ka_info.ping_index];

	if (device->conn_state == boot_state_connected)
	{
		ka_msg_t* ka_msg = (ka_msg_t*) data_ind->data;

		 /* If for the Keep-Alive */
		if (ka_msg->id == KA_MSG_ID)
		{
			hi_ipv6_get_saddr_panid(data_ind->source_address, &src_pan_id, &src_short_addr);

			/* Checks the PAN ID and short address */
			if ((src_pan_id == ka_info.ka_pan_id) && (src_short_addr == device->short_addr))
			{
				ka_info.curr_event = KA_EV_PANC_RECEIVED_ECHO_IND;
				RTOS_PUT_MSG(g3_queueHandle, KA_MSG, NULL);
			}
			else
			{
				PRINT_G3_KA_WARNING("Ping from unknown device (PAN %X, address %d).\n", src_pan_id, src_short_addr);
			}
		}
	}

}

#else

/**
  * @brief Checks if the timeout for the ping reception from the coordinator is expired (device only).
  * @param unused Unused parameter (can be set as NULL).
  * @retval None.
  */
static void g3_ka_device_timeout_expired(void *unused)
{
	UNUSED(unused);
	PRINT_G3_KA_CRITICAL("KA expired, leaving PAN...\n");
	g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_LEAVE_REQ, NULL, 0);
}

/**
  * @brief Function that handles the reception of a device start confirmation (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_device_start_cnf(const void *payload)
{
    const BOOT_DeviceStartConfirm_t *dev_start = payload;

    if (dev_start->status == G3_SUCCESS)
    {
    	/* Sets PAN ID for DEVICE */
		ka_info.ka_pan_id 		= dev_start->pan_id;
		ka_info.ka_short_addr 	= dev_start->network_addr;

        /* Starts timer for keep-alive, if not already running */
		g3_app_ka_start();
    }
}

/**
  * @brief Function that handles the reception of a device leave confirmation (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_device_leave_cnf(const void *payload)
{
    const BOOT_DeviceLeaveConfirm_t *dev_leave = payload;

    if (dev_leave->status == G3_SUCCESS)
    {
    	g3_app_ka_stop();
    }
}

/**
  * @brief Function that handles the reception of a device leave indication (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_device_leave_ind(const void *payload)
{
    UNUSED(payload);

    g3_app_ka_stop();
}


/**
  * @brief Function that handles the reception of a ICMP echo request indication (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_ka_handle_echoreq_ind(const void *payload)
{
    const IP_DataIndication_t *echo_ind     = payload;
	const IP_IcmpDataIndication_t *data_ind = hi_ipv6_extract_icmp_from_ip(echo_ind);
	uint16_t src_pan_id, src_short_addr;

    uint8_t ka_id = data_ind->data[0];

    hi_ipv6_get_saddr_panid(data_ind->source_address, &src_pan_id, &src_short_addr);

    /* If for the Keep-Alive and from the PAN coordinator */
    if ((ka_id == KA_MSG_ID                 ) &&
    	(src_pan_id     == ka_info.ka_pan_id) &&
    	(src_short_addr == 0	            ) )
	{
		if (osTimerIsRunning(kaTimerHandle))
		{
#if (DEBUG_G3_KA >= DEBUG_LEVEL_FULL)
			PRINT_G3_KA_INFO("Coordinator is alive (PAN ID %X)\n", src_pan_id);
#endif
			ka_info.last_ka_ts = HAL_GetTick();
			osTimerStart(kaTimerHandle, KA_DEVICE_LEAVE_TIME);
		}
		else
		{
			PRINT_G3_KA_WARNING("Coordinator ping with Keep-Alive off\n");
		}
	}
	else
	{
		PRINT_G3_KA_INFO("Ping (ID=%X) from PAN ID %X, address %u (not coordinator)\n", ka_id, src_pan_id, src_short_addr);
	}
}

#endif /* IS_COORD */

/**
  * @}
  */

/** @addtogroup G3_App_KeepAlive_Exported_Code
  * @{
  */

/**
  * @brief Checks if a message is needed by the G3 Keep-Alive application.
  * @param g3_msg Pointer to the G3 message structure to evaluate.
  * @return 'true' if the message is needed, 'false' otherwise.
  */
bool g3_app_ka_msg_needed(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
#if IS_COORD
    case HIF_BOOT_SRV_LEAVE_IND:
    case HIF_BOOT_SRV_KICK_CNF:
    case HIF_BOOT_SRV_JOIN_IND:
    case HIF_ICMP_ECHO_CNF:
    case HIF_ICMP_ECHO_REP_IND:
#else
    case HIF_BOOT_DEV_LEAVE_CNF:
    case HIF_BOOT_DEV_LEAVE_IND:
    case HIF_BOOT_DEV_START_CNF:
    case HIF_ICMP_ECHO_REQ_IND:
#endif /* IS_COORD */
        return true;
    default:
    	return false;
    }
}

/**
  * @brief Handles the reception of a G3 Keep-Alive application message.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_ka_msg_handler(const g3_msg_t *g3_msg)
{
    /* If the device type is set, handles the message depending on the device type */
	switch (g3_msg->command_id)
	{
#if IS_COORD
	case HIF_BOOT_SRV_LEAVE_IND:
		g3_ka_handle_server_leave_ind(g3_msg->payload);
		break;
	case HIF_BOOT_SRV_KICK_CNF:
		g3_ka_handle_server_kick_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_SRV_JOIN_IND:
		g3_ka_handle_server_join_ind(g3_msg->payload);
		break;
	case HIF_ICMP_ECHO_CNF:
		g3_ka_handle_echo_cnf(g3_msg->payload);
		break;
	case HIF_ICMP_ECHO_REP_IND:
		g3_ka_handle_echorep_ind(g3_msg->payload);
		break;
#else
	case HIF_BOOT_DEV_START_CNF:
		g3_ka_handle_device_start_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_CNF:
		g3_ka_handle_device_leave_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_IND:
		g3_ka_handle_device_leave_ind(g3_msg->payload);
		break;
	case HIF_ICMP_ECHO_REQ_IND:
		g3_ka_handle_echoreq_ind(g3_msg->payload);
		break;
#endif /* IS_COORD */
	default:
		break;
	}
}

/**
  * @brief Initializes the G3 keep-alive application.
  * @param None
  * @retval None
  */
void g3_app_ka_init(void)
{
	memset(&ka_info, 0, sizeof(ka_info));

#if IS_COORD
	ka_info.curr_state = KA_ST_PANC_READY_TO_SEND_ECHO;
	ka_info.curr_event = KA_EV_PANC_NONE;

	ka_info.ka_pan_id = PAN_ID;
#endif /* IS_COORD */
}

/**
  * @brief Starts the G3 keep-alive application.
  * @param None
  * @retval True if the application was successfully started
  */
bool g3_app_ka_start(void)
{
	bool started = false;

	if (!osTimerIsRunning(kaTimerHandle))
	{
#if IS_COORD
		started = true;
		PRINT_G3_KA_INFO("Starting Keep-Alive (PAN ID: %X)\n", ka_info.ka_pan_id);
		ka_info.ping_index = 0;
		osTimerStart(kaTimerHandle, KEEP_ALIVE_CHECK_PERIOD);
#else
		PRINT_G3_KA_INFO("Starting Keep-Alive (PAN ID: %X, short address: %u)\n", ka_info.ka_pan_id, ka_info.ka_short_addr);
		ka_info.last_ka_ts = HAL_GetTick();
		osTimerStart(kaTimerHandle, KA_DEVICE_LEAVE_TIME);
#endif
	}

	return started;
}

/**
  * @brief Stops the G3 keep-alive application.
  * @param None
  * @retval True if the application was successfully stopped
  */
bool g3_app_ka_stop(void)
{
	bool stopped = false;

	if (osTimerIsRunning(kaTimerHandle))
	{
		stopped = true;
		LED_SET_OFF(LED_KEEPALIVE);
		PRINT_G3_KA_INFO("Stopped Keep-Alive\n");
		osTimerStop(kaTimerHandle);
	}

	return stopped;
}


#if IS_COORD

/**
  * @brief Waits until the G3 keep-alive has finished the round-table ping.
  * @param None
  * @retval None
  */
void g3_app_ka_wait(void)
{
	while (g3_app_ka_in_progress())
	{
		utils_delay_ms(1);
	}
}

/**
  * @brief Returns the status of the G3 keep-alive current activity.
  * @param None
  * @retval Returns true if the G3 keep-alive is executing the round-table ping (status of the Keep-Alive LED).
  */
bool g3_app_ka_in_progress(void)
{
	return LED_IS_ON(LED_KEEPALIVE);
}

/**
  * @brief G3 keep-alive task routine.
  * @param None
  * @retval None
  */
void g3_app_ka(void)
{
    /* For coordinator only */

	/* Manages the current event, depending on the current state */
	g3_ka_fsm_manager();

}
#else

/**
  * @brief Returns the time left after which the device will leave the PAN.
  * @param None
  * @retval None
  */
uint32_t g3_app_ka_time_to_leave(void)
{
	uint32_t time_to_leave;
	uint32_t time_elapsed = HAL_GetTick() - ka_info.last_ka_ts;

	if (time_elapsed < KA_DEVICE_LEAVE_TIME)
	{
		time_to_leave = KA_DEVICE_LEAVE_TIME - time_elapsed;
	}
	else
	{
		time_to_leave = 0;
	}

	return time_to_leave;
}

#endif /* IS_COORD */

/**
  * @brief Callback function of the kaTimer FreeRTOStimer. Handles the timed events (pings for the PAN Coordinator, disconnection for the PAN Device)
  * @param argument Unused argument.
  * @retval None
  */
void g3_app_ka_timer_callback(void *argument)
{
	UNUSED(argument);

	/* For coordinator only: */
#if IS_COORD
	g3_ka_coord_timer_event(NULL);
#else
	g3_ka_device_timeout_expired(NULL);
#endif /* IS_COORD */
}

#endif /* ENABLE_ICMP_KEEP_ALIVE */

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
