/******************************************************************************
*   @file    g3_app_boot_clt.c
*   @author  AMG/IPC Application Team
*   @brief   This code includes the state machines and the functions related to LoWPAN Bootstrap
*            client implementation.
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
#include <mem_pool.h>
#include <utils.h>
#include <debug_print.h>
#include <hi_g3lib_sap_interface.h>
#include <hi_msgs_impl.h>
#include <g3_app_config.h>
#include <g3_app_boot_constants.h>
#include <g3_app_boot_clt.h>
#include <g3_app_boot.h>
#include <g3_boot_clt_eap.h>
#include <main.h>


/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot_Clt
  * @{
  */

/** @addtogroup G3_App_Boot_Clt_Private_Code
  * @{
  */

#if !IS_COORD
#if ENABLE_BOOT_CLIENT_ON_HOST

/* Definitions */

/* Macros */
#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)
#define HANDLE_CNF_ERROR(cnf_id, status) g3_boot_clt_cnf_error(cnf_id, status)
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* Custom types */
/**
  * @}
  */

/** @addtogroup G3_App_Boot_Clt_Exported_Code
  * @{
  */

/* Global variables */
boot_client_t boot_client;
lba_info_t lba_info;

/* External Variables */
extern osTimerId_t bootTimerHandle;
extern osMessageQueueId_t g3_queueHandle;
extern bool fast_restore_enabled;

/* External functions */
extern char* g3_app_pansort_translate_media_type(uint8_t media);

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Clt_Private_Code
  * @{
  */

/* Private Variables */

/* Private functions */
static boot_clt_state_t g3_boot_clt_fsm_default(        const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_start(          const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_discover_req(   const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_pansort_ind(    const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_join_start(     const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_join_retry(     const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_join_result(    const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_routed(         const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_disconnect(     const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_kicked(			const void *payload);
static boot_clt_state_t g3_boot_clt_fsm_left(			const void *payload);

/* Function pointer prototype */
typedef boot_clt_state_t g3_boot_clt_fsm_func(const void *payload);

/**
  * @brief  State Functions Table
  */
static g3_boot_clt_fsm_func* const g3_boot_clt_fsm_func_tbl[BOOT_CLT_ST_CNT][BOOT_CLT_EV_CNT] =
{
/*				              NONE,		               TIMER_EXPIRED,                RECEIVED_DEVICE_START_REQ, RECEIVED_DISCOVERY_CNF,      RECEIVED_DEVICE_PANSORT_REQ, RECEIVED_NETWORK_JOIN_CNF,   RECEIVED_ROUTE_DISCOVERY_CNF, RECEIVED_DEVICE_LEAVE_REQ,  RECEIVED_NETWORK_LEAVE_CNF, RECEIVED_NETWORK_LEAVE_IND */
/* DISCONNECTED			  */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_start,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* WAIT_TIMER			  */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_discover_req, g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* WAIT_DISCOVER_CNF 	  */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,   g3_boot_clt_fsm_pansort_ind, g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* WAIT_PANSORT_REQ       */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_join_start,   g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_join_start,  g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* BOOTSTRAPPING          */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_join_retry,   g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_join_result, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* ROUTING                */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_routed,       g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default,    g3_boot_clt_fsm_default    },
/* CONNECTED              */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_disconnect, g3_boot_clt_fsm_default,    g3_boot_clt_fsm_kicked     },
/* LEAVING                */ {g3_boot_clt_fsm_default, g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,   g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,     g3_boot_clt_fsm_default,      g3_boot_clt_fsm_default,    g3_boot_clt_fsm_left,	     g3_boot_clt_fsm_default    }
};

/* Internal functions prototypes */

#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)
static void g3_boot_clt_cnf_error(hif_cmd_id_t cnf_id, g3_result_t status)
{
	if (status != G3_SUCCESS)
	{
		PRINT_G3_BOOT_CLT_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
	}
}
#endif

#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_FULL)
/**
 * @brief Translates EAP message events in strings.
 * @param [in] msg_event The EAP Message to translate
 * @return msg_event The Event represented from the EAP message
 */
char* g3_adp_lbp_eap_translate_event(boot_clt_eap_event_t msg_event)
{
	switch(msg_event)
	{
	case BOOT_CLT_EAP_EV_RECEIVED_FIRST: 	return "RECEIVED_FIRST";
	case BOOT_CLT_EAP_EV_RECEIVED_THIRD:	return "RECEIVED_THIRD";
	case BOOT_CLT_EAP_EV_RECEIVED_ACCEPT:	return "RECEIVED_ACCEPT";
	case BOOT_CLT_EAP_EV_RECEIVED_DECLINE:	return "RECEIVED_DECLINE";
	case BOOT_CLT_EAP_EV_RECEIVED_PARAM:	return "RECEIVED_PARAM";
	case BOOT_CLT_EAP_EV_RECEIVED_KICK:		return "RECEIVED_KICK";
	default:								return "UNKNOWN";
	}
}
#endif

#if 0
/**
 * @brief Parse the EAP Message carried by the LBS Message sent from a node to the bootstrap client/peer
 * @param [in] eap_msg The EAP Message to process
 * @param [in] eap_msg_len The Length of the EAP message to process
 * @return msg_event The event represented from the EAP message
 */
static uint8_t g3_adp_lbp_eap_decode(const eap_msg_t* eap_msg, const uint16_t eap_msg_len)
{
	uint8_t msg_event = BOOT_CLT_EAP_EV_NONE;

	if ((eap_msg_len >= (sizeof(eap_header_t) + sizeof(eap_psk_header_t))) &&
		(eap_msg->header.eap_header.code == adp_eap_request			     ) &&
		(eap_msg->header.eap_psk_header.type == adp_eap_psk_iana_type	 ) )
	{
		switch (eap_msg->header.eap_psk_header.T)
		{
		case adp_eap_psk_msg_1:
			msg_event = BOOT_CLT_EAP_EV_RECEIVED_FIRST;
			break;
		case adp_eap_psk_msg_3:
			msg_event = BOOT_CLT_EAP_EV_RECEIVED_THIRD;
			break;
		default:
			PRINT_G3_BOOT_CLT_CRITICAL("LBP-EAP: unexpected message PSK type (%u)\n", eap_msg->header.eap_psk_header.T);
			break;
		}
	}
	else
	{
		PRINT_G3_BOOT_CLT_CRITICAL("LBP-EAP: unexpected length (%u) code (%u) or type (%u)\n", eap_msg_len, eap_msg->header.eap_header.code, eap_msg->header.eap_psk_header.type);
	}

	return msg_event;
}

/**
 * @brief Parse the LBP Message sent from a Node to the Bootstrap Server
 * @param [in/out] lbp_eap_msg The data used from the Bootstrap Server to handle the LBP message
 * @param [in] MsgLen The lbp_eap_msg_len of the LBP message to process
 * @return msg_event The Event represented from the Bootstrap message
 */
static boot_clt_eap_event_t g3_adp_lbp_decode(lbp_ind_t* lbp_eap_msg, const uint16_t lbp_eap_msg_len)
{
	uint16_t eap_msg_len = 0;
	boot_clt_eap_event_t msg_event = BOOT_CLT_EAP_EV_NONE;

	if (	(lbp_eap_msg_len >= sizeof(lbp_header_t)			) &&
			(lbp_eap_msg->lbp_msg->header.T == adp_lbp_to_lbd	) )
	{
		eap_msg_len = lbp_eap_msg_len - sizeof(lbp_header_t);

		switch (lbp_eap_msg->lbp_msg->header.code)
		{
		case adp_lbs_accepted:
			msg_event = BOOT_CLT_EAP_EV_RECEIVED_ACCEPT;
			break;
		case adp_lbs_challange:
			if (lbp_eap_msg->lbp_msg->eap.header.eap_header.is_cfg_par)
			{
				lbp_eap_msg->conf_param_msg = &lbp_eap_msg->lbp_msg->param;

				if (lbp_eap_msg->conf_param_msg->header.attr_id_type.M == conf_param_dsi)
				{
					msg_event = BOOT_CLT_EAP_EV_RECEIVED_PARAM;
				}
			}
			else
			{
				lbp_eap_msg->eap_msg = &lbp_eap_msg->lbp_msg->eap;

				msg_event = g3_adp_lbp_eap_decode(lbp_eap_msg->eap_msg, eap_msg_len);
			}
			break;
		case adp_lbs_decline:
			msg_event = BOOT_CLT_EAP_EV_RECEIVED_DECLINE;
			break;
		case adp_lbs_kick:
			msg_event = BOOT_CLT_EAP_EV_RECEIVED_KICK;
			break;
		default:
			PRINT_G3_BOOT_CLT_CRITICAL("LBP: unexpected code (%u)\n", lbp_eap_msg->lbp_msg->header.code);
			break;
		}
	}
	else
	{
		PRINT_G3_BOOT_CLT_CRITICAL("LBP: unexpected length (%u) or type (%u)\n", lbp_eap_msg_len, lbp_eap_msg->lbp_msg->header.T);
	}

	return msg_event;
}
#endif

/**
  * @brief G3 Boot Client function that sends the G3BOOT-DEVICE-START.Confirm to the G3 task.
  * @param status The result of the operation
  * @return None
  */
static void g3_boot_clt_send_start_cnf(uint8_t status, uint16_t network_addr, uint16_t pan_id)
{
	BOOT_DeviceStartConfirm_t *start_cnf = MEMPOOL_MALLOC(sizeof(BOOT_DeviceStartConfirm_t));
	uint16_t len = hi_boot_devstartcnf_fill(start_cnf, status, network_addr, pan_id);
	g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_START_CNF, start_cnf, len);
}

/**
  * @brief G3 Boot Client function that sends the G3BOOT-DEVICE-LEAVE.Confirm to the G3 task.
  * @param status The result of the operation
  * @return None
  */
static void g3_boot_clt_send_leave_cnf(uint8_t status)
{
	BOOT_DeviceLeaveConfirm_t *leave_cnf = MEMPOOL_MALLOC(sizeof(BOOT_DeviceLeaveConfirm_t));
	uint16_t len = hi_boot_devleavecnf_fill(leave_cnf, status);
	g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_LEAVE_CNF, leave_cnf, len);
}

/**
  * @brief G3 Boot Client function that sends the G3BOOT-DEVICE-PANSORT.Confirm to the G3 task.
  * @param status The result of the operation
  * @return None
  */
static void g3_boot_clt_send_pansort_cnf(uint8_t status)
{
	BOOT_DevicePANSortConfirm_t* pansort_cnf = MEMPOOL_MALLOC(sizeof(BOOT_DevicePANSortConfirm_t));
	pansort_cnf->status = status;
	g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_PANSORT_CNF, pansort_cnf, sizeof(BOOT_DevicePANSortConfirm_t));
}

/**
  * @brief G3 Boot Client function that tries to join the current/next chosen PAN LBA in the list, depending on the retries/LQI.
  * @param None
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_join_chosen_pan(void)
{
	boot_clt_state_t next_state;

	if (boot_client.join_tries >= BOOT_CLIENT_ASSOCIATION_MAX_RETRIES)
	{
		uint16_t pan_id 	= boot_client.pan_descriptor[boot_client.pan_index].pan_id;
		uint16_t lba_addr 	= boot_client.pan_descriptor[boot_client.pan_index].lba_addr;
		uint16_t media_type = boot_client.pan_descriptor[boot_client.pan_index].media_type;

		PRINT_G3_BOOT_CLT_WARNING("Maximum number of retries reached for joining PAN %X, address %u, media %s\n", pan_id, lba_addr, g3_app_pansort_translate_media_type(media_type));

		boot_client.join_tries = 0;
		boot_client.pan_index++;
	}

	/* Skip PAN descriptors with LQI lower than the LQI threshold */
	while ((boot_client.pan_index < boot_client.pan_count) && (boot_client.pan_descriptor[boot_client.pan_index].lq < BOOT_CLIENT_LQI_THRESHOLD))
	{
		uint16_t pan_id 	= boot_client.pan_descriptor[boot_client.pan_index].pan_id;
		uint16_t lba_addr 	= boot_client.pan_descriptor[boot_client.pan_index].lba_addr;
		uint16_t media_type = boot_client.pan_descriptor[boot_client.pan_index].media_type;
		uint8_t lqi 		= boot_client.pan_descriptor[boot_client.pan_index].lq;

		PRINT_G3_BOOT_CLT_WARNING("Skipped PAN %X, address %u, media %s due to LQI = %u/%u\n", pan_id, lba_addr, g3_app_pansort_translate_media_type(media_type), lqi, BOOT_CLIENT_LQI_THRESHOLD);

		boot_client.pan_index++;
	}

	if (boot_client.pan_index < boot_client.pan_count)
	{
		/* Selects the pan descriptor*/
		boot_client.pan_id		= boot_client.pan_descriptor[boot_client.pan_index].pan_id;
		boot_client.lba_addr	= boot_client.pan_descriptor[boot_client.pan_index].lba_addr;
		boot_client.media_type	= boot_client.pan_descriptor[boot_client.pan_index].media_type;

		PRINT_G3_BOOT_CLT_INFO("Joining PAN %X, address %u, media %s...\n", boot_client.pan_id,boot_client.lba_addr, g3_app_pansort_translate_media_type(boot_client.media_type));

		/* Sends request to join the PAN */
		ADP_AdpmNetworkJoinRequest_t *network_join_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmNetworkJoinRequest_t));

		uint16_t len = hi_adp_network_join_fill(network_join_req, &boot_client.pan_descriptor[boot_client.pan_index]);
		g3_send_message(HIF_TX_MSG, HIF_ADPM_NTWJOIN_REQ, network_join_req, len);

		boot_client.join_tries++;

		next_state = BOOT_CLT_ST_BOOTSTRAPPING;
	}
	else
	{
		PRINT_G3_BOOT_CLT_CRITICAL("End of PAN list reached, resetting Boot Client FSM\n");

		next_state = BOOT_CLT_ST_DISCONNECTED;

		g3_boot_clt_send_start_cnf(G3_LIMIT_REACHED, 0, 0);
	}

	return next_state;
}

/**
  * @brief G3 Boot Client function that finishes the bootstrap procedure.
  * @param None
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t boot_clt_finish_join(void)
{
	boot_clt_state_t next_state;
#if BOOT_CLIENT_DISCOVER_ROUTE
	/* A G3ADP-ROUTEDISCOVERY.Request is sent to discover the route to the Coordiantor */
	ADP_AdpdRouteDiscoveryRequest_t *route_discovery_req = MEMPOOL_MALLOC(sizeof(ADP_AdpdRouteDiscoveryRequest_t));

	uint16_t len = hi_adp_routediscovery_fill(route_discovery_req, COORD_ADDRESS, BOOT_CLIENT_DEFAULT_MAX_HOPS, boot_client.lba_addr, boot_client.media_type);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_ROUTEDISCO_REQ, route_discovery_req, len);

	next_state = BOOT_CLT_ST_ROUTING;
#else
	PRINT_G3_BOOT_CLT_INFO("Joined PAN %X as device %u\n", boot_client.pan_id, boot_client.short_address);

	g3_boot_clt_send_start_cnf(G3_SUCCESS, boot_client.short_address, boot_client.pan_id);

	next_state = BOOT_CLT_ST_CONNECTED;
#endif
	return next_state;
}

/**
  * @brief G3 Boot Client FSM function that maintains the current state, with no further action
  * @param payload Not used in this function.
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_default(const void *payload)
{
	UNUSED(payload);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return boot_client.curr_state;
}

/**
  * @brief G3 Boot Client FSM function that starts the Boot Client in normal or fast restore mode
  * @param payload Not used in this function
  * @return  The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_start(const void *payload)
{
	UNUSED(payload);

	boot_clt_state_t next_state = boot_client.curr_state;

	if (boot_client.fast_restore != true)
	{
		/* Normal start */
		srand(HAL_GetTick()); /* Needed to randomize the random wait time between join retries */

		osTimerStart(bootTimerHandle, BOOT_CLIENT_START_WAIT_TIME*configTICK_RATE_HZ); /* Starts the device connection procedure after this time */

		PRINT_G3_BOOT_CLT_INFO("Normal start\n");

		next_state = BOOT_CLT_ST_WAIT_TIMER;
	}
#if ENABLE_FAST_RESTORE
	else
	{
		PRINT_G3_BOOT_CLT_INFO("Fast restore start\n");

		/* Completes the bootstrap after all attributes have been set */
		next_state = boot_clt_finish_join();
	}
#endif /* ENABLE_FAST_RESTORE */

	boot_client.curr_event = BOOT_CLT_EV_NONE;

	return next_state;
}

/**
  * @brief G3 Boot Client FSM function that sends the request to scan the area in search of existing PANs
  * @param payload Not used in this function.
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_discover_req(const void *payload)
{
	UNUSED(payload);

	uint8_t duration = BOOT_CLIENT_DISCOVERY_TIME;

	PRINT_G3_BOOT_CLT_INFO("Discovering network (%u s)...\n", duration);

	ADP_AdpmDiscoveryRequest_t *discovery_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmDiscoveryRequest_t));

	// Send ADPM-DISCOVERY.Request to ADP
    uint16_t len = hi_adp_discovery_fill(discovery_req, duration);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_DISCOVERY_REQ, discovery_req, len);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return BOOT_CLT_ST_WAIT_DISCOVER_CNF;
}

/**
  * @brief G3 Boot Client FSM function that sends the list of discovered PAN accesses
  * @param payload The G3ADP-DISCOVERY.Confirm message payload data
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_pansort_ind(const void *payload)
{
	const ADP_AdpmDiscoveryConfirm_t *discovery_cnf = payload;

	boot_clt_state_t next_state;

	if (discovery_cnf->status == G3_SUCCESS)
	{
		/* Saves the PAN agent list, in case no PANSORT request is received later */
		boot_client.pan_count = discovery_cnf->pan_count;
		memcpy(boot_client.pan_descriptor, discovery_cnf->pan_descriptor, boot_client.pan_count*sizeof(boot_client.pan_descriptor[0]));

#if SEND_PANSORT_REQUEST
		PRINT_G3_BOOT_CLT_INFO("Requesting PAN sorting...\n");

		BOOT_DevicePANSortIndication_t *pansort_ind = MEMPOOL_MALLOC(sizeof(BOOT_DevicePANSortIndication_t)); /* Uses memory pool due to big structure size */

		/* Sends G3BOOT-PANSORT.Indication to the host */
		uint16_t len = hi_boot_devpansortind_fill(pansort_ind, discovery_cnf);
		g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_PANSORT_IND, pansort_ind, len);
#else
		PRINT_G3_BOOT_CLT_INFO("PAN sorting disabled\n");
#endif
		next_state = BOOT_CLT_ST_WAIT_PANSORT_REQ;

		/* Sets timeout for the reception of the G3BOOT-PANSORT.Request */
		osTimerStart(bootTimerHandle, BOOT_CLIENT_PANSORT_TIMEOUT);
	}
	else
	{
		if (discovery_cnf->status == G3_NO_BEACON)
		{
			PRINT_G3_BOOT_CLT_WARNING("No PAN found, retrying in %u s...\n", BOOT_CLIENT_START_WAIT_TIME);
		}
		else
		{
			PRINT_G3_BOOT_CLT_WARNING("Discovery error (%u=%s), retrying in %u s...\n", discovery_cnf->status, g3_app_translate_g3_result(discovery_cnf->status), BOOT_CLIENT_START_WAIT_TIME);
		}

		next_state = BOOT_CLT_ST_WAIT_TIMER;

		osTimerStart(bootTimerHandle, BOOT_CLIENT_START_WAIT_TIME*configTICK_RATE_HZ); /* Starts the device connection procedure after this time */
	}

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return next_state;
}

/**
  * @brief G3 Boot Client FSM function that joins the first or next PAN agent of the PANSORT request list
  * @param payload The G3BOOT-PANSORT.Request message payload data
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_join_start(const void *payload)
{
	boot_clt_state_t next_state;

	const BOOT_DevicePANSortRequest_t *pansort_req = payload;

	/* Get the PAN agent list from the PANSORT request, if it was received. Otherwise, the old list from the ADPM-DISCOVERY.Confirm is used */
	if (pansort_req != NULL)
	{
		/* Revokes the timeout for the reception of the G3BOOT-PANSORT.Request */
		osTimerStop(bootTimerHandle);

		if (pansort_req->pan_count <= ADP_MAX_NUM_PANDESCR)
		{
			boot_client.pan_count = pansort_req->pan_count;
			memcpy(boot_client.pan_descriptor, pansort_req->pan_descriptor, boot_client.pan_count*sizeof(boot_client.pan_descriptor[0]));

			g3_boot_clt_send_pansort_cnf(G3_SUCCESS);
		}
		else
		{
			g3_boot_clt_send_pansort_cnf(G3_PARAMETER_ERROR);
		}
	}
	else
	{
		PRINT_G3_BOOT_CLT_WARNING("No PANSORT request received, using PAN descriptor list as is\n");
	}

#if DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_INFO
	uint16_t pan_id 	= boot_client.pan_descriptor[boot_client.pan_index].pan_id;
	uint16_t lba_addr 	= boot_client.pan_descriptor[boot_client.pan_index].lba_addr;
	uint16_t media_type = boot_client.pan_descriptor[boot_client.pan_index].media_type;

	PRINT_G3_BOOT_CLT_INFO("Starting from PAN %X, address %u, media %s\n", pan_id, lba_addr, g3_app_pansort_translate_media_type(media_type));
#endif

	next_state = g3_boot_clt_join_chosen_pan();

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return next_state;
}

/**
  * @brief G3 Boot Client FSM function that handles the join retry
  * @param payload Not used in this function
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_join_retry(const void *payload)
{
	boot_clt_state_t next_state;

	UNUSED(payload);

	next_state = g3_boot_clt_join_chosen_pan();

	uint16_t pan_id 	= boot_client.pan_descriptor[boot_client.pan_index].pan_id;
	uint16_t lba_addr 	= boot_client.pan_descriptor[boot_client.pan_index].lba_addr;
	uint16_t media_type = boot_client.pan_descriptor[boot_client.pan_index].media_type;

	PRINT_G3_BOOT_CLT_WARNING("Trying to join PAN %X, address %u, media %s\n", pan_id, lba_addr, g3_app_pansort_translate_media_type(media_type));

	boot_client.curr_event = BOOT_CLT_EV_NONE;

	return next_state;
}


/**
  * @brief G3 Boot Client FSM function that evaluates the network join, indicating its result
  * @param payload The G3ADP-NETWORK-JOIN.Confirm message payload data
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_join_result(const void *payload)
{
	const ADP_AdpmNetworkJoinConfirm_t *network_join_cnf = payload;

	boot_clt_state_t next_state = boot_client.curr_state;

	if (network_join_cnf->status == G3_SUCCESS)
	{
		/* Boot client PAN ID has already been set in the PAN descriptor selection, sets the short address */
		boot_client.short_address = network_join_cnf->network_addr;

		/* Set LBA info */
		lba_info.short_address = boot_client.lba_addr;
		lba_info.media_type    = boot_client.media_type;

		/* Save connection info (PAN ID and Short address) */
		hif_nvm_req_t *hif_nvm_req = MEMPOOL_MALLOC(sizeof(hif_nvm_req_t)); /* Uses memory pool due to big structure size */

		uint16_t len = hi_hostif_nvm_fill(hif_nvm_req, flash_op_write, nvm_plc, NVM_PAN_SHORT_ADDR_OFFSET, NVM_PAN_SHORT_ADDR_SIZE, (uint8_t*) &boot_client.pan_id);
		g3_send_message(HIF_TX_MSG, HIF_HI_NVM_REQ, hif_nvm_req, len);

		PRINT_G3_BOOT_CLT_INFO("PAN ID and short address saved\n");

		next_state = boot_clt_finish_join();
	}
	else
	{
		uint32_t retry_time = rand() % (BOOT_CLIENT_ASSOCIATION_RAND_WAIT_TIME*configTICK_RATE_HZ);

		PRINT_G3_BOOT_CLT_WARNING("Network join failed. Retrying in %u ms\n", retry_time);

		osTimerStart(bootTimerHandle, retry_time);
	}

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return next_state;
}

/**
  * @brief G3 Boot Client FSM function that evaluates the route discovery result
  * @param payload The G3ADP-ROUTE-DISCOVERY.Confirm message payload data
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_routed(const void *payload)
{
	const ADP_AdpdRouteDiscoveryConfirm_t *route_discoveryn_cnf = payload;

	if (route_discoveryn_cnf->status == G3_SUCCESS)
	{
		PRINT_G3_BOOT_CLT_INFO("Found route to device %u with cost %u\n", route_discoveryn_cnf->dst_addr, route_discoveryn_cnf->route_cost);
	}
	else
	{
		PRINT_G3_BOOT_CLT_WARNING("Error while routing (%u=%s)\n", route_discoveryn_cnf->status, g3_app_translate_g3_result(route_discoveryn_cnf->status));
	}

	PRINT_G3_BOOT_CLT_INFO("Joined PAN %X as device %u\n", boot_client.pan_id, boot_client.short_address);

	g3_boot_clt_send_start_cnf(G3_SUCCESS, boot_client.short_address, boot_client.pan_id);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return BOOT_CLT_ST_CONNECTED;
}

/**
  * @brief G3 Boot Client FSM function that disconnects from the PAN, by leaving
  * @param payload Not used in this function
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_disconnect(const void *payload)
{
	UNUSED(payload); /* No payload */

	/* Request forwarded to ADP layer */
	g3_send_message(HIF_TX_MSG, HIF_ADPM_NTWLEAVE_REQ, NULL, 0);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return BOOT_CLT_ST_LEAVING;
}

/**
  * @brief G3 Boot Client FSM function that acknowledges the disconnection, when device is kicked out
  * @param payload Not used in this function
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_kicked(const void *payload)
{
	UNUSED(payload);

	/* Sends the G3BOOT-DEV-LEAVE.Indication if the device was kicked */
	g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_LEAVE_IND, NULL, 0);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return BOOT_CLT_ST_DISCONNECTED;
}

/**
  * @brief G3 Boot Client FSM function that handles the disconnection.
  * @param payload The payload of the ADP-NETWORK-LEAVE.Confirm
  * @return The next Boot Client FSM state
  */
static boot_clt_state_t g3_boot_clt_fsm_left(const void *payload)
{
	/* Sends the G3BOOT-DEV-LEAVE.Confirm if the device left the PAN on its own */
	const ADP_AdpmNetworkLeaveConfirm_t *network_leave_cnf = payload;

	BOOT_DeviceLeaveConfirm_t *device_leave_cnf = MEMPOOL_MALLOC(sizeof(BOOT_DeviceLeaveConfirm_t));

	uint16_t len = hi_boot_devleavecnf_fill(device_leave_cnf, network_leave_cnf->status);
	g3_send_message(G3_RX_MSG, HIF_BOOT_DEV_LEAVE_CNF, device_leave_cnf, len);

	boot_client.curr_event = BOOT_CLT_EV_NONE;

    return BOOT_CLT_ST_DISCONNECTED;
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-START.Request
  * @param payload The payload of the received message
  * @return None
  */
static void g3_boot_clt_handle_device_start_req(const void *payload)
{
	const BOOT_DeviceStartRequest_t *device_start_req = payload;

	if (boot_client.curr_state == BOOT_CLT_ST_DISCONNECTED)
	{
		if (device_start_req->req_type == BOOT_START_NORMAL)
		{
			/* Normal Start */
			boot_client.fast_restore = false;
			fast_restore_enabled = false;
		}
#if ENABLE_FAST_RESTORE
		else
		{
			/* Fast restore */
			boot_client.fast_restore = true;
			fast_restore_enabled = true;
		}
#endif /* ENABLE_FAST_RESTORE */

		boot_client.curr_event = BOOT_CLT_EV_RECEIVED_DEVICE_START_REQ;
	}
	else
	{
		g3_boot_clt_send_start_cnf(G3_WRONG_STATE, 0, 0);
	}
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-LEAVE.Request
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_boot_clt_handle_device_leave_req(const void *payload)
{
	UNUSED(payload);

	if (boot_client.curr_state == BOOT_CLT_ST_CONNECTED)
	{
		boot_client.curr_event = BOOT_CLT_EV_RECEIVED_DEVICE_LEAVE_REQ;
	}
	else
	{
		g3_boot_clt_send_leave_cnf(G3_WRONG_STATE);
	}
}

/**
  * @brief Function that handles the reception of a G3BOOT-DEV-PANSORT.Request
  * @param payload The payload of the received message
  * @return None
  */
static void g3_boot_clt_handle_device_pansort_req(const void *payload)
{
	UNUSED(payload);

	if (boot_client.curr_state == BOOT_CLT_ST_WAIT_PANSORT_REQ)
	{
		boot_client.curr_event = BOOT_CLT_EV_RECEIVED_DEVICE_PANSORT_REQ;
	}
	else
	{
		g3_boot_clt_send_pansort_cnf(G3_WRONG_STATE);
	}
}


/**
  * @brief Function that handles the reception of a G3ADP-DISCOVERY.Confirm
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_boot_clt_handle_discovery_cnf(const void *payload)
{
	UNUSED(payload);

	boot_client.curr_event = BOOT_CLT_EV_RECEIVED_DISCOVERY_CNF;
}

/**
  * @brief Function that handles the reception of a G3ADP-NETWORK-JOIN.Confirm
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_boot_clt_handle_network_join_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)
	const ADP_AdpmNetworkJoinConfirm_t *network_join_cnf = payload;
	HANDLE_CNF_ERROR(HIF_ADPM_NTWJOIN_CNF, network_join_cnf->status);
#else
	UNUSED(payload);
#endif

	boot_client.curr_event = BOOT_CLT_EV_RECEIVED_NETWORK_JOIN_CNF;
}

/**
  * @brief Function that handles the reception of a G3ADP-ROUTE-DISCOVERY.Confirm
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_boot_clt_handle_route_discovery_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)
	const ADP_AdpdRouteDiscoveryConfirm_t *route_discovery_cnf = payload;
	HANDLE_CNF_ERROR(HIF_ADPM_ROUTEDISCO_CNF, route_discovery_cnf->status);
#else
	UNUSED(payload);
#endif

	boot_client.curr_event = BOOT_CLT_EV_RECEIVED_ROUTE_DISCOVERY_CNF;
}

/**
  * @brief Function that handles the reception of a G3ADP-NETWORK-LEAVE.Confirm
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_boot_clt_handle_network_leave_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)
	const ADP_AdpmNetworkLeaveConfirm_t *network_leave_cnf = payload;
	HANDLE_CNF_ERROR(HIF_ADPM_NTWLEAVE_CNF, network_leave_cnf->status);
#else
	UNUSED(payload);
#endif

	boot_client.curr_event = BOOT_CLT_EV_RECEIVED_NETWORK_LEAVE_CNF;
}
/**
  * @brief Function that handles the reception of a ADPM-NETWORK-LEAVE.Indication
  * @param payload The payload of the received message (empty)
  * @retval None
  */
static void g3_boot_clt_handle_network_leave_ind(const void *payload)
{
	UNUSED(payload);

	boot_client.curr_event = BOOT_CLT_EV_RECEIVED_NETWORK_LEAVE_IND;
}

/**
  * @brief Checks if an internal request is needed by the G3 Boot Client application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed by the boot application, 'false' otherwise.
  */
static bool g3_boot_clt_req_needed(const g3_msg_t *g3_msg)
{
	switch (g3_msg->command_id)
	{
	case HIF_BOOT_DEV_START_REQ:
	case HIF_BOOT_DEV_LEAVE_REQ:
	case HIF_BOOT_DEV_PANSORT_REQ:
		return true;
	default:
		return false;
	}
}

/**
  * @brief Handles the reception of internal G3 Boot Client application requests.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
static void g3_boot_clt_req_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
    case HIF_BOOT_DEV_START_REQ:
		g3_boot_clt_handle_device_start_req(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_REQ:
    	g3_boot_clt_handle_device_leave_req(g3_msg->payload);
    	break;
    case HIF_BOOT_DEV_PANSORT_REQ:
    	g3_boot_clt_handle_device_pansort_req(g3_msg->payload);
		break;
    default:
        break;
    }
}

/**
  * @brief Function that handles the FSM of the G3 Boot Client application.
  * @param payload The payload of the received G3 message
  * @retval None
  */
static void g3_boot_clt_fsm_manager(const void *payload)
{
    boot_client.curr_state = g3_boot_clt_fsm_func_tbl[boot_client.curr_state][boot_client.curr_event](payload);
}

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Clt_Exported_Code
  * @{
  */

/* Public functions */

/**
  * @brief Initializes the G3 Boot Client application.
  * @param None
  * @retval None
  */
void g3_app_boot_clt_init(void)
{
	boot_client.curr_state			= BOOT_CLT_ST_DISCONNECTED;
	boot_client.curr_event			= BOOT_CLT_EV_NONE;

	boot_client.pan_count			= 0;
	memset(&boot_client.pan_descriptor, 0, sizeof(boot_client.pan_descriptor));
	boot_client.pan_index			= 0;
	boot_client.join_tries			= 0;

	boot_client.pan_id				= 0;
	boot_client.short_address		= 0;
	boot_client.lba_addr 			= 0;
	boot_client.media_type			= 0;

	boot_client.fast_restore      	= false;
}

/**
  * @brief Executes the G3 Boot Client application.
  * @param payload Payload of the received G3 message
  * @retval None
  */
void g3_app_boot_clt(void *payload)
{
	/* Manages the current event, depending on the current state */
	g3_boot_clt_fsm_manager(payload);
}

/**
  * @brief Checks if a message is needed by the G3 Boot Client application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed by the boot application, 'false' otherwise
  */
bool g3_app_boot_clt_msg_needed(const g3_msg_t *g3_msg)
{
	switch (g3_msg->command_id)
	{
	case HIF_ADPM_DISCOVERY_CNF:
	case HIF_ADPM_NTWJOIN_CNF:
	case HIF_ADPM_NTWLEAVE_CNF:
    case HIF_ADPM_NTWLEAVE_IND:
    case HIF_ADPM_ROUTEDISCO_CNF:
		return true;
	default:
		return false;
	}
}

/**
  * @brief Handles the reception of G3 Boot Client application messages.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_boot_clt_msg_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
    /* Messages received by the coordinator only when the Boot Client is on the application layer */
    case HIF_ADPM_DISCOVERY_CNF:
    	g3_boot_clt_handle_discovery_cnf(g3_msg->payload);
    	break;
    case HIF_ADPM_NTWJOIN_CNF:
    	g3_boot_clt_handle_network_join_cnf(g3_msg->payload);
    	break;
    case HIF_ADPM_NTWLEAVE_CNF:
    	g3_boot_clt_handle_network_leave_cnf(g3_msg->payload);
    	break;
    case HIF_ADPM_NTWLEAVE_IND:
    	g3_boot_clt_handle_network_leave_ind(g3_msg->payload);
    	break;
    case HIF_ADPM_ROUTEDISCO_CNF:
    	g3_boot_clt_handle_route_discovery_cnf(g3_msg->payload);
    	break;

    default:
        break;
    }
}

/**
  * @brief Executes the G3 Boot Client application (internal requests).
  * @param g3_msg Pointer to the G3 request message structure
  * @retval None
  */
void g3_app_boot_clt_req_handler(const g3_msg_t *g3_msg)
{
	void *payload = NULL;

	if (g3_msg != NULL)
	{
		if (g3_boot_clt_req_needed(g3_msg))
		{
			g3_boot_clt_req_handler(g3_msg);
			payload = g3_msg->payload;
		}
	}

	/* Manages the current event, depending on the current state. The payload can be null (timer event) */
	g3_boot_clt_fsm_manager(payload);
}

/**
  * @brief Starts the G3 Boot Client application.
  * @param None
  * @retval None
  */
void g3_app_boot_clt_start(void)
{
	uint16_t len;
	BOOT_DeviceStartRequest_t *boot_start_req = MEMPOOL_MALLOC(sizeof(BOOT_DeviceStartRequest_t));

#if ENABLE_FAST_RESTORE
	boot_client.fast_restore = fast_restore_enabled;
	if (boot_client.fast_restore != true)
		len = hi_boot_devstartreq_fill(boot_start_req, BOOT_START_NORMAL);
	else
		len = hi_boot_devstartreq_fill(boot_start_req, BOOT_START_FAST_RESTORE);
#else
	len = hi_boot_devstartreq_fill(boot_start_req, BOOT_START_NORMAL);
#endif /* ENABLE_FAST_RESTORE */

	g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_START_REQ, boot_start_req, len);
}

/**
  * @brief Callback function of the bootTimer FreeRTOStimer for the PAN client device.
  *        Triggers the timeout event for the Boot Server FSM
  * @param argument Unused argument.
  * @retval None
  */
void g3_app_boot_clt_timeoutCallback(void *argument)
{
	UNUSED(argument);
	boot_client.curr_event = BOOT_CLT_EV_TIMER_EXPIRED;

	RTOS_PUT_MSG(g3_queueHandle, BOOT_CLT_MSG, NULL); /* Needed to execute the boot application */
}

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

/******************* (C) COPYRIGHT 2021 STMicroelectronics *******************/
