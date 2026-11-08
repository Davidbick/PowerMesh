/******************************************************************************
*   @file    g3_app_boot_srv.c
*   @author  AMG/IPC Application Team
*   @brief   This code includes the state machines and the functions related to LoWPAN Bootstrap
*            server implementation.
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
#include <g3_app_boot_srv.h>
#include <g3_app_boot.h>
#include <g3_boot_srv_eap.h>
#include <main.h>


/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot_Srv
  * @{
  */

/** @addtogroup G3_App_Boot_Srv_Private_Code
  * @{
  */

#if IS_COORD

/* Macros */
#if ENABLE_BOOT_SERVER_ON_HOST
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
#define HANDLE_CNF_ERROR(cnf_id, status) g3_boot_srv_cnf_error(cnf_id, status)
#define HANDLE_SRV_ERROR(error_id)		 g3_boot_srv_error(error_id)
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#define HANDLE_SRV_ERROR(error_id)
#endif

typedef enum boot_srv_err_enum
{
	srv_err_none = 0,
	srv_err_unexpected_lbp_eap_psk_T,
	srv_err_unexpected_lbp_eap_psk_type,
	srv_err_unexpected_lbp_eap_len,
	srv_err_unexpected_lbp_eap_code,
	srv_err_unexpected_lbp_code,
	srv_err_unexpected_lbp_len,
	srv_err_cannot_start_PAN,
	srv_err_cannot_start,
	srv_err_cannot_stop,
	srv_err_failed_discovery,
	srv_err_failed_start,
	srv_err_invalid_lbp,
	srv_err_invalid_lbp_id,
	srv_err_no_entry,
	srv_err_cannot_kick_tables,
	srv_err_cannot_kick_busy,
} boot_srv_err_t;

/* External variables */
extern osTimerId_t bootTimerHandle;

extern osTimerId_t serverTimerHandle;

#if BOOT_SERVER_IDS_LEN == 0
extern uint8_t mac_address[MAC_ADDR64_SIZE];
#endif

#endif /* ENABLE_BOOT_SERVER_ON_HOST */

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Srv_Exported_Code
  * @{
  */

/* Global variables */
boot_server_t boot_server;

/* External Variables */
extern osMessageQueueId_t g3_queueHandle;

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Srv_Private_Code
  * @{
  */

#if ENABLE_BOOT_SERVER_ON_HOST

/* Private functions */
static boot_srv_state_t g3_boot_srv_fsm_default(      const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_discover_req( const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_discover_cnf( const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_net_start_cnf(const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_lbp_cnf(      const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_lbp_ind(      const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_timeout(      const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_setpsk_req(   const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_stop_req(     const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_kick_req(     const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_start_rk_req( const void *payload);
static boot_srv_state_t g3_boot_srv_fsm_abort_rk_req( const void *payload);

/* Function pointer prototype */
typedef boot_srv_state_t g3_boot_srv_fsm_func(const void *payload);

/**
  * @brief  State Functions Table
  */
static g3_boot_srv_fsm_func* const g3_boot_srv_fsm_func_tbl[BOOT_SRV_ST_CNT][BOOT_SRV_EV_CNT] =
{
/*				              NONE,		               TIMER_EXPIRED,                RECEIVED_DISCOVERY_CNF,       RECEIVED_NETWORK_START_CNF,    RECEIVED_LBP_CNF,        RECEIVED_LBP_IND,		RECEIVED_SETPSK_REQ,        RECEIVED_STOP_REQ,	      RECEIVED_KICK_REQ		    RECEIVED_REKEYING_REQ         RECEIVED_ABORT_RK_REQ        */
/* INACTIVE  		      */ {g3_boot_srv_fsm_default, g3_boot_srv_fsm_discover_req, g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default,       g3_boot_srv_fsm_default, g3_boot_srv_fsm_default, g3_boot_srv_fsm_default,    g3_boot_srv_fsm_stop_req, g3_boot_srv_fsm_default,  g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default      },
/* WAIT_DISCOVER_CNF 	  */ {g3_boot_srv_fsm_default, g3_boot_srv_fsm_default,      g3_boot_srv_fsm_discover_cnf, g3_boot_srv_fsm_default,       g3_boot_srv_fsm_default, g3_boot_srv_fsm_default, g3_boot_srv_fsm_default,    g3_boot_srv_fsm_default,  g3_boot_srv_fsm_default,  g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default      },
/* WAIT_NETWORK_START_CNF */ {g3_boot_srv_fsm_default, g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default,      g3_boot_srv_fsm_net_start_cnf, g3_boot_srv_fsm_default, g3_boot_srv_fsm_default, g3_boot_srv_fsm_default,    g3_boot_srv_fsm_default,  g3_boot_srv_fsm_default,  g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default      },
/* ACTIVE				  */ {g3_boot_srv_fsm_default, g3_boot_srv_fsm_timeout,      g3_boot_srv_fsm_default,      g3_boot_srv_fsm_default,       g3_boot_srv_fsm_lbp_cnf, g3_boot_srv_fsm_lbp_ind, g3_boot_srv_fsm_setpsk_req, g3_boot_srv_fsm_stop_req, g3_boot_srv_fsm_kick_req, g3_boot_srv_fsm_start_rk_req, g3_boot_srv_fsm_abort_rk_req }
};

/* Private functions */

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
static void g3_boot_srv_cnf_error(hif_cmd_id_t cnf_id, g3_result_t status)
{
	if (status != G3_SUCCESS)
	{
		PRINT_G3_BOOT_SRV_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
	}
}

static void g3_boot_srv_error(boot_srv_err_t error_id)
{
	PRINT_G3_BOOT_SRV_CRITICAL("Boot server error: %u\n", error_id);
}
#endif

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
/**
 * @brief Translates EAP message events in strings.
 * @param [in] msg_event The EAP Message to translate
 * @return msg_event The Event represented from the EAP message
 */
char* g3_adp_lbp_eap_translate_event(boot_srv_eap_event_t msg_event)
{
	switch(msg_event)
	{
	case BOOT_SRV_EAP_EV_RECEIVED_JOIN: 	return "RECEIVED_JOIN";
	case BOOT_SRV_EAP_EV_RECEIVED_SECOND:	return "RECEIVED_SECOND";
	case BOOT_SRV_EAP_EV_RECEIVED_KICK:		return "RECEIVED_KICK";
	case BOOT_SRV_EAP_EV_PSK_ACQUIRED:		return "PSK_ACQUIRED";
	case BOOT_SRV_EAP_EV_RECEIVED_FOURTH:	return "RECEIVED_FOURTH";
	case BOOT_SRV_EAP_EV_RECEIVED_PARAM:	return "RECEIVED_PARAM";
	default:								return "UNKNOWN";
	}
}
#endif

/**
 * @brief Parse the EAP Message carried by the LBP Message sent from a Node
 *        to the Bootstrap Server
 * @param [in] eap_msg The EAP Message to process
 * @param [in] eap_msg_len The Length of the EAP message to process
 * @return msg_event The event represented from the EAP message
 */
static uint8_t g3_adp_lbp_eap_decode(const eap_msg_t* eap_msg, const uint16_t eap_msg_len)
{
	uint8_t msg_event = BOOT_SRV_EAP_EV_NONE;

	if (eap_msg_len >= (sizeof(eap_header_t) + sizeof(eap_psk_header_t)))
	{
		if (eap_msg->header.eap_header.code == adp_eap_response)
		{
			if (eap_msg->header.eap_psk_header.type == adp_eap_psk_iana_type)
			{
				switch (eap_msg->header.eap_psk_header.T)
				{
				case adp_eap_psk_msg_2:
					msg_event = BOOT_SRV_EAP_EV_RECEIVED_SECOND;
					break;
				case adp_eap_psk_msg_4:
					msg_event = BOOT_SRV_EAP_EV_RECEIVED_FOURTH;
					break;
				default:
					HANDLE_SRV_ERROR(srv_err_unexpected_lbp_eap_psk_T);
					break;
				}
			}
			else
			{
				HANDLE_SRV_ERROR(srv_err_unexpected_lbp_eap_psk_type);
			}
		}
		else
		{
			HANDLE_SRV_ERROR(srv_err_unexpected_lbp_eap_code);
		}
	}
	else
	{
		HANDLE_SRV_ERROR(srv_err_unexpected_lbp_eap_len);
	}

	return msg_event;
}

/**
 * @brief Parse the LBP Message sent from a Node to the Bootstrap Server
 * @param [in/out] lbp_eap_msg The data used from the Bootstrap Server to handle the LBP message
 * @param [in] MsgLen The lbp_eap_msg_len of the LBP message to process
 * @return msg_event The Event represented from the Bootstrap message
 */
static boot_srv_eap_event_t g3_adp_lbp_decode(lbp_ind_t* lbp_eap_msg, const uint16_t lbp_eap_msg_len)
{
	uint16_t eap_msg_len = 0;
	boot_srv_eap_event_t msg_event = BOOT_SRV_EAP_EV_NONE;

	if (	(lbp_eap_msg_len >= sizeof(lbp_header_t)			) &&
			(lbp_eap_msg->lbp_msg->header.T == adp_lbp_from_lbd	) )
	{
		eap_msg_len = lbp_eap_msg_len - sizeof(lbp_header_t);

		switch (lbp_eap_msg->lbp_msg->header.code)
		{
		case adp_lbd_joining:
			if (eap_msg_len == 0)
			{
				msg_event = BOOT_SRV_EAP_EV_RECEIVED_JOIN;
			}
			else
			{
				if (lbp_eap_msg->lbp_msg->eap.header.eap_header.is_cfg_par)
				{
					lbp_eap_msg->conf_param_msg = &lbp_eap_msg->lbp_msg->param;

					if (lbp_eap_msg->conf_param_msg->header.attr_id_type.M == conf_param_dsi)
					{
						msg_event = BOOT_SRV_EAP_EV_RECEIVED_PARAM;
					}
				}
				else
				{
					lbp_eap_msg->eap_msg = &lbp_eap_msg->lbp_msg->eap;

					msg_event = g3_adp_lbp_eap_decode(lbp_eap_msg->eap_msg, eap_msg_len);
				}
			}
			break;
		case adp_lbd_kick:
			msg_event = BOOT_SRV_EAP_EV_RECEIVED_KICK;
			break;
		default:
			HANDLE_SRV_ERROR(srv_err_unexpected_lbp_code);
			break;
		}
	}
	else
	{
		HANDLE_SRV_ERROR(srv_err_unexpected_lbp_len);
	}

	return msg_event;
}

/**
  * @brief G3 Boot Server FSM function that maintains the current state, with no further action
  * @param [in] payload Not used in this function.
  * @return The Boot Server current state
  */
static boot_srv_state_t g3_boot_srv_fsm_default(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return boot_server.curr_state;
}

/**
  * @brief G3 Boot Server FSM function that sends the request to scan the area in search of existing PANs
  * @param [in] payload Not used in this function.
  * @return The new FSM state
  */
static boot_srv_state_t g3_boot_srv_fsm_discover_req(const void *payload)
{
	UNUSED(payload);

	uint8_t duration = BOOT_SERVER_DISCOVERY_TIME;

	PRINT_G3_BOOT_SRV_INFO("Discovering network (%u s)...\n", duration);

	ADP_AdpmDiscoveryRequest_t *discovery_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmDiscoveryRequest_t));

    uint16_t len = hi_adp_discovery_fill(discovery_req, duration);
	g3_send_message(HIF_TX_MSG, HIF_ADPM_DISCOVERY_REQ, discovery_req, len);

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return BOOT_SRV_ST_WAIT_DISCOVER_CNF;
}

/**
  * @brief G3 Boot Server FSM function that evaluates the discovery and starts the network, depending on the result
  * @param [in] payload The Discovery confirm message payload data
  * @return The next Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_discover_cnf(const void *payload)
{
	boot_srv_state_t next_state;

	bool network_available = true;
	const ADP_AdpmDiscoveryConfirm_t *discovery_cnf = payload;

	if (discovery_cnf->status == G3_NO_BEACON)
	{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		PRINT_G3_BOOT_SRV_INFO("Discovery successful: no beacon detected\n");
#endif
	}
	else if (discovery_cnf->status == G3_SUCCESS)
	{
		assert(discovery_cnf->pan_count > 0);

		/* At least another PAN was found */
		for (uint16_t i = 0; i < discovery_cnf->pan_count; i++)
		{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
			PRINT_G3_BOOT_SRV_WARNING("Another PAN was found with ID: %04X\n", discovery_cnf->pan_descriptor[i].pan_id);
#endif
			if (discovery_cnf->pan_descriptor[i].pan_id == boot_server.pan_id)
			{
				network_available = false;
				PRINT_G3_BOOT_SRV_WARNING("This PAN has the same ID as the one to start: %04X\n", boot_server.pan_id);
			}
		}
#if BOOT_SERVER_FORCE_PAN_START
		if (network_available)
		{
			PRINT_G3_BOOT_SRV_WARNING("Can start PAN %04X anyway (there are %u existing PAN)\n", boot_server.pan_id, discovery_cnf->pan_count);
		}
#else
		network_available = false;
		HANDLE_SRV_ERROR(srv_err_cannot_start_PAN);
#endif /* BOOT_SERVER_FORCE_PAN_START */
	}
	else
	{
		network_available = false;
		HANDLE_SRV_ERROR(srv_err_failed_discovery);
	}

	if (network_available)
	{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		PRINT_G3_BOOT_SRV_INFO("Starting PAN %X...\n", boot_server.pan_id);
#endif
		ADP_AdpmNetworkStartRequest_t *network_start_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmNetworkStartRequest_t));

		uint16_t len = hi_adp_network_start_fill(network_start_req, boot_server.pan_id);
		g3_send_message(HIF_TX_MSG, HIF_ADPM_NTWSTART_REQ, network_start_req, len);

		next_state = BOOT_SRV_ST_WAIT_NETWORK_START_CNF;
	}
	else
	{
		PRINT_G3_BOOT_SRV_WARNING("Restarting Boot Server...\n");

		BOOT_ServerStartRequest_t *srvstart_req = MEMPOOL_MALLOC(sizeof(ADP_AdpmNetworkStartRequest_t));

		uint16_t len = hi_boot_srvstartreq_fill(srvstart_req, BOOT_START_NORMAL, boot_server.pan_id, boot_server.short_addr);
		g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_START_REQ, srvstart_req, len);

		/*Return to starting state */
		next_state = BOOT_SRV_ST_INACTIVE;
	}

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return next_state;
}

/**
  * @brief   G3 Boot Server FSM function that evaluates the network start and starts the network, confirming the server start, if successful
  * @param   [in] payload The Network Start confirm message payload data
  * @return  The next Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_net_start_cnf(const void *payload)
{
	boot_srv_state_t next_state;

	const ADP_AdpmNetworkStartConfirm_t *network_start_cnf = (ADP_AdpmNetworkStartConfirm_t*) payload;

	if (network_start_cnf->status == G3_SUCCESS)
	{
		PRINT_G3_BOOT_SRV_INFO("PAN %X started\n", boot_server.pan_id);

		next_state = BOOT_SRV_ST_ACTIVE;
	}
	else
	{
		HANDLE_SRV_ERROR(srv_err_failed_start);

		g3_boot_srv_init_tables();

		next_state = BOOT_SRV_ST_INACTIVE;
	}

	BOOT_ServerStartConfirm_t *srvstart_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerStartConfirm_t));

	srvstart_cnf->status = network_start_cnf->status;
	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_START_CNF, srvstart_cnf, sizeof(*srvstart_cnf));

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return next_state;
}

/**
  * @brief   G3 Boot Server FSM function that handles the ADP-LBP confirms
  * @param   [in] payload The ADP-LBP confirm message payload data
  * @return  The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_lbp_cnf(const void *payload)
{
	const ADP_AdpmLbpConfirm_t *lbp_cnf = (ADP_AdpmLbpConfirm_t *) payload;

	if (	(boot_server.curr_substate == boot_srv_kickig	) &&
			(lbp_cnf->nsdu_handle == boot_server.kick_handle) )
	{
		BOOT_ServerKickConfirm_t *kick_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerKickConfirm_t));
		kick_cnf->status = lbp_cnf->status;
		g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_KICK_CNF, kick_cnf, sizeof(*kick_cnf));

		boot_server.curr_substate = boot_srv_idle;
	}
	else
	{
		boot_join_entry_t* join_entry = g3_boot_srv_join_entry_find_handle(lbp_cnf->nsdu_handle);

		if (join_entry != NULL)
		{
			/* A positive confirm has been received for the accept of the join entry waiting for it */
			if (lbp_cnf->status == G3_SUCCESS)
			{
				/* Handle the Event generated by the LBP message depending on the Node's current state */
				join_entry->curr_event = BOOT_SRV_EAP_EV_RECEIVED_CNF;

				g3_boot_srv_eap_fsm_manager(NULL, join_entry);
			}
		}
	}

	boot_server.curr_event = BOOT_SRV_EV_NONE;

	return boot_server.curr_state;
}

/**
  * @brief   G3 Boot Server FSM function that handles the ADP-LBP indications (for bootstrap and re-keying)
  * @param   [in] payload The ADP-LBP indication message payload data
  * @return  The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_lbp_ind(const void *payload)
{
	uint8_t 			media_type;
	boot_join_entry_t* 	join_entry = NULL;
	lbp_ind_t 			lbp_eap_msg;
	boot_srv_eap_event_t 	eap_event;						/**< @brief The Event generated from the message received from the Node */
	const ADP_AdpmLbpIndication_t *lbp_ind = payload;

	uint32_t offset = 0;
	uint32_t nsdu_len;
	uint8_t security_enabled;

	// Save LBAAddress in the LBP Message container
	if (lbp_ind->src_addr.addr_mode == MAC_ADDR_MODE_64)
	{
		// The LBD is bootstrapping directly from the LBS
		lbp_eap_msg.lba_addr = MAC_BROADCAST_SHORT_ADDR;
	}
	else if (lbp_ind->src_addr.addr_mode == MAC_ADDR_MODE_16)
	{
		// The LBD is bootstrapping through an LBA
		lbp_eap_msg.lba_addr = lbp_ind->src_addr.short_addr;

		offset += sizeof(lbp_ind->src_addr.ext_addr) - sizeof(lbp_ind->src_addr.short_addr);
	}

	if ((lbp_ind->src_addr.addr_mode == MAC_ADDR_MODE_64) || (lbp_ind->src_addr.addr_mode == MAC_ADDR_MODE_16))
	{

		nsdu_len =  VAR_SIZE_PAYLOAD_OFFSET(lbp_ind->nsdu_len,   offset    );
		nsdu_len += VAR_SIZE_PAYLOAD_OFFSET(lbp_ind->nsdu_len,   (offset-1)) << 8;

		lbp_eap_msg.lbp_msg = (lbp_msg_t*) (lbp_ind->nsdu - offset);
		lbp_eap_msg.eap_msg = NULL;

		// Get the Event generated from the message - Parse the LBP message
		eap_event = g3_adp_lbp_decode(&lbp_eap_msg, nsdu_len);

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		PRINT_G3_BOOT_SRV_INFO("Received LBP indication (event: %s)\n", g3_adp_lbp_eap_translate_event(eap_event));
#endif
		offset += sizeof(lbp_ind->nsdu) - nsdu_len;

		if (lbp_ind->src_addr.addr_mode == MAC_ADDR_MODE_64)
		{
			media_type = VAR_SIZE_PAYLOAD_OFFSET(lbp_ind->media_type, offset);
		}
		else
		{
			media_type = lbp_eap_msg.lbp_msg->header.media_type;
		}

		security_enabled = VAR_SIZE_PAYLOAD_OFFSET(lbp_ind->security_enabled, offset);

		// Discard messages from LBAs sent without encryption
		if (	(lbp_eap_msg.lba_addr != MAC_BROADCAST_SHORT_ADDR	) &&
				(!security_enabled									) &&
				(eap_event != BOOT_SRV_EAP_EV_RECEIVED_PARAM		) )
		{
			HANDLE_SRV_ERROR(srv_err_invalid_lbp);
		}
		else
		{
			BOOT_ServerLeaveIndication_t *leave_ind;
			uint16_t len;

			join_entry = g3_boot_srv_join_entry_find(lbp_eap_msg.lbp_msg->header.lbd_addr);

			switch(eap_event)
			{
			case BOOT_SRV_EAP_EV_RECEIVED_JOIN:

				/* If it is rejoining, the device is disconnected */
				g3_app_boot_remove_connected_device(lbp_eap_msg.lbp_msg->header.lbd_addr);

				/* A device that is already in the joining entry table has sent a new join request */
				if (join_entry != NULL)
				{
					// Ignore Joining if the previous one (from the same LBD) was recently received
					if ((HAL_GetTick() - join_entry->join_time) < BOOT_SERVER_JOINING_IGNORE_TIME)
					{
						ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
						PRINT_G3_BOOT_SRV_WARNING("Join request from %s discarded (ignore time not elapsed)\n", ext_addr_str);
					}
					else
					{
						/* Remove the old Joining Entry, to reallocate it */
						boot_device_t* boot_device = g3_app_boot_find_device(join_entry->ext_addr, MAC_BROADCAST_SHORT_ADDR, boot_state_bootstrapping);

						if (boot_device != NULL)
						{
							boot_device->conn_state = boot_state_disconnected;
						}

						g3_boot_srv_join_entry_remove(join_entry);
						join_entry = NULL;

					}
				}

				if (join_entry == NULL)
				{
					/* Find and initialize the entry */
					join_entry = g3_boot_srv_join_entry_add(lbp_eap_msg.lbp_msg->header.lbd_addr, lbp_eap_msg.lba_addr, media_type, lbp_eap_msg.lbp_msg->header.disable_bkp, false);

					if (join_entry != NULL)
					{
						// Handle the Event generated by the LBP message depending on the Node's current state
						join_entry->curr_event = eap_event;

						g3_boot_srv_eap_fsm_manager(&lbp_eap_msg, join_entry);
					}
					else
					{
						Error_Handler(); /* Joining Entry Table full */
					}
				}
				break;
			case BOOT_SRV_EAP_EV_RECEIVED_SECOND:
			case BOOT_SRV_EAP_EV_RECEIVED_FOURTH:
			case BOOT_SRV_EAP_EV_RECEIVED_PARAM:
				/* Entry found or created */
				if (join_entry != NULL)
				{
					/* Handle the Event generated by the LBP message depending on the Node's current state */
					join_entry->curr_event = eap_event;

					g3_boot_srv_eap_fsm_manager(&lbp_eap_msg, join_entry);
				}
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
				else
				{
					HANDLE_SRV_ERROR(srv_err_no_entry);
				}
#endif
				break;
			case BOOT_SRV_EAP_EV_RECEIVED_KICK:
				/* Send Leave Indication, the device is disconnected as soon as the indication is received */
				leave_ind = MEMPOOL_MALLOC(sizeof(BOOT_ServerLeaveIndication_t));
				len = hi_boot_srvleaveind_fill(leave_ind, lbp_eap_msg.lbp_msg->header.lbd_addr);
				g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_LEAVE_IND, leave_ind, len);
				break;
			case BOOT_SRV_EAP_EV_NONE:
			default:
				HANDLE_SRV_ERROR(srv_err_invalid_lbp_id);
				break;
			}
		}
	}

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return boot_server.curr_state;
}

/**
  * @brief   G3 Boot Server FSM function that handles timeout events
  * @param   [in] payload Not used in this functions
  * @return  The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_timeout(const void *payload)
{
	UNUSED(payload);

	const uint8_t free_entry[MAC_ADDR64_SIZE] = {0};

	bool timer_restart_needed = false;
	boot_join_entry_t*  join_entry;
	uint32_t elapsed_time;

	for (uint32_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		join_entry = g3_boot_srv_join_entry_get(i);

		/* Check the timestamp only for used entries */
		if (memcmp(join_entry->ext_addr, free_entry, MAC_ADDR64_SIZE) != 0)
		{
			/* There is at least a joining entry in the table */
			timer_restart_needed = true;

			if (	(join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_SECOND) ||
					(join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_FOURTH) ||
					(join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_PARAM ) )
			{
				elapsed_time = HAL_GetTick() - join_entry->join_time;

				/* Bootstrap timeout */
				if (elapsed_time > (BOOT_SERVER_JOINING_TABLE_ENTRY_TTL * configTICK_RATE_HZ))
				{
					ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
					PRINT_G3_BOOT_SRV_WARNING("Bootstrap timeout for device %s\n", ext_addr_str);

					boot_device_t* boot_device = g3_app_boot_find_device(join_entry->ext_addr, MAC_BROADCAST_SHORT_ADDR, boot_state_bootstrapping);

					if (boot_device != NULL)
					{
						boot_device->conn_state = boot_state_disconnected;
					}

					if (join_entry->rekeying)
					{
						if (join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_SECOND)
						{
							boot_server.rekeying_error = rekeying_error_msg_2;
						}
						else if (join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_FOURTH)
						{
							boot_server.rekeying_error = rekeying_error_msg_4;
						}
						else if (join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_PARAM)
						{
							boot_server.rekeying_error = rekeying_error_param;
						}

						g3_boot_srv_eap_rekeying_fsm();
					}
					else
					{
						g3_boot_srv_join_entry_remove(join_entry);
					}

				}
			}
			else if (join_entry->curr_state == BOOT_SRV_EAP_ST_WAIT_PSK)
			{
				elapsed_time = HAL_GetTick() - join_entry->getpsk_timestamp;

				/* SETPSK timeout */
				if (elapsed_time > BOOT_SERVER_PSK_GET_TIMEOUT)
				{
					const uint8_t psk_default[] = DEFAULT_PSK;

					PRINT_G3_BOOT_SRV_WARNING("Using default PSK due to SETPSK timeout. Assigned short address = %u\n", join_entry->short_addr);

					/* In case SETPSK is not received, the default PSK is used (has to match the entry's PSK to succeed) */
					eap_psk_initialize_psk(psk_default, join_entry->eap_psk_data.psk_context);

					join_entry->short_addr = i;
					join_entry->curr_event = BOOT_SRV_EAP_EV_PSK_ACQUIRED;

					g3_boot_srv_eap_fsm_manager(NULL, join_entry);
				}
			}
		}
	}

	if (timer_restart_needed)
	{
		osTimerStart(bootTimerHandle, BOOT_SERVER_BOOTSTRAP_TIMEOUT_CHECK_PERIOD);
	}

	boot_server.curr_event = BOOT_SRV_EV_NONE;

	return boot_server.curr_state;
}

/**
  * @brief   G3 Boot Server FSM function that handles the server set-psk request
  * @param   [in] payload The server set-psk request message payload data
  * @return  The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_setpsk_req(const void *payload)
{
	const BOOT_ServerSetPSKRequest_t *setpsk_req = payload;
	BOOT_ServerSetPSKConfirm_t *setpsk_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerSetPSKConfirm_t));

	boot_join_entry_t* entry_to_set = g3_boot_srv_join_entry_find(setpsk_req->ext_addr);

	if (entry_to_set != NULL)
	{
		entry_to_set->short_addr = setpsk_req->short_addr;

		if (entry_to_set->short_addr != MAC_BROADCAST_SHORT_ADDR)
		{
			PRINT_G3_BOOT_SRV_INFO("Using PSK from the SETPSK request. Assigned short address = %u\n", entry_to_set->short_addr);

			/* In case SETPSK is received, the given PSK is used (has to match the entry's PSK to succeed) */
			eap_psk_initialize_psk(setpsk_req->psk, entry_to_set->eap_psk_data.psk_context);

			setpsk_cnf->status = G3_SUCCESS;
		}
		else
		{
			PRINT_G3_BOOT_SRV_WARNING("SETPSK request denied the authentication, sending decline...\n", entry_to_set->short_addr);

			setpsk_cnf->status = G3_NOT_ACCEPTABLE;
		}

		/* Generate event for the FSM of the entry */
		entry_to_set->curr_event = BOOT_SRV_EAP_EV_PSK_ACQUIRED;

		g3_boot_srv_eap_fsm_manager(NULL, entry_to_set);
	}
	else
	{
		setpsk_cnf->status = G3_FAILED;
	}

	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_SETPSK_CNF, setpsk_cnf, sizeof(*setpsk_cnf));

	boot_server.curr_event = BOOT_SRV_EV_NONE;

    return boot_server.curr_state;
}

/**
  * @brief G3 Boot Server FSM function that handles the server stop request
  * @param [in] payload The stop request message payload data
  * @return The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_stop_req(const void *payload)
{
	boot_server.curr_event = BOOT_SRV_EV_NONE;

	UNUSED(payload);

	boot_srv_state_t next_state = boot_server.curr_state;
	BOOT_ServerStopConfirm_t *srvstop_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerStopConfirm_t));

	if (boot_server.curr_substate == boot_srv_idle)
	{
		PRINT_G3_BOOT_SRV_INFO("Received STOP request, resetting Boot Server FSM\n");

		osTimerStop(bootTimerHandle);

		g3_boot_srv_init_tables();

		srvstop_cnf->status = G3_SUCCESS;

		next_state = BOOT_SRV_ST_INACTIVE;
	}
	else
	{
		HANDLE_SRV_ERROR(srv_err_cannot_stop);

		srvstop_cnf->status = G3_BUSY;
	}

	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_STOP_CNF, srvstop_cnf, sizeof(*srvstop_cnf));

    return next_state;
}

/**
  * @brief G3 Boot Server FSM function that handles the server kick request
  * @param [in] payload The kick request message payload data
  * @return The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_kick_req(const void *payload)
{
	BOOT_ServerKickRequest_t *kick_req = (BOOT_ServerKickRequest_t*) payload;

	boot_server.curr_event = BOOT_SRV_EV_NONE;
	uint8_t cnf_status = G3_SUCCESS;

	if (boot_server.curr_substate == boot_srv_idle)
	{
		boot_join_entry_t* entry_to_kick = NULL;
		boot_device_t* device_to_kick = g3_app_boot_find_device(kick_req->ext_addr, kick_req->short_addr, boot_state_connected);

		if (device_to_kick != NULL)
		{
			/* Proceed only if the entry is not present in the joining entry table */
			if (g3_boot_srv_join_entry_find(kick_req->ext_addr) == NULL)
			{
				// Initialize the new entry, with short address (needed to cipher messages)
				entry_to_kick = g3_boot_srv_join_entry_add(device_to_kick->ext_addr, device_to_kick->short_addr, device_to_kick->media_type, device_to_kick->disable_bkp, false);

				if (entry_to_kick != NULL)
				{
					/* LBA = LBD in this case */
					entry_to_kick->short_addr = device_to_kick->short_addr;

					PRINT_G3_BOOT_SRV_INFO("Received KICK request for device %u\n", entry_to_kick->short_addr);

					boot_server.kick_handle = boot_server.nsdu_handle;
					g3_adp_lbp_send_kick(entry_to_kick, boot_server.pan_id, boot_server.nsdu_handle++);

					boot_server.curr_substate = boot_srv_kickig;

					g3_app_boot_remove_connected_device(device_to_kick->ext_addr);
				}
			}
		}

		if (entry_to_kick == NULL)
		{
			cnf_status = G3_FAILED;
			HANDLE_SRV_ERROR(srv_err_cannot_kick_tables);
		}
	}
	else
	{
		cnf_status = G3_BUSY;
		HANDLE_SRV_ERROR(srv_err_cannot_kick_busy);
	}

	if (cnf_status != G3_SUCCESS)
	{
		BOOT_ServerKickConfirm_t *kick_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerKickConfirm_t));
		kick_cnf->status = cnf_status;
		g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_KICK_CNF, kick_cnf, sizeof(*kick_cnf));
	}

    return boot_server.curr_state;
}

/**
  * @brief G3 Boot Server FSM function that handles the server re-keying request
  * @param [in] payload The re-keying request message payload data
  * @return The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_start_rk_req(const void *payload)
{
	boot_server.curr_event = BOOT_SRV_EV_NONE;

	BOOT_ServerRekeyingRequest_t *rekeying_req = (BOOT_ServerRekeyingRequest_t*) payload;

	if (boot_server.curr_substate == boot_srv_idle)
	{
		/* The new index is always 1 minus the current */
		boot_server.gmk_index_new = 1 - boot_server.gmk_index;

		/* Copies the new GMK at the new index */
		memcpy(boot_server.gmk[boot_server.gmk_index_new], rekeying_req->gmk, MAC_KEY_SIZE);

		boot_server.rekeying_error = rekeying_error_none;

		boot_server.curr_substate = boot_srv_rekeying_step_set_gmk;

		g3_boot_srv_eap_rekeying_fsm();
	}
	else
	{
		/* Send SRV-REKEYING-Confirm */
		BOOT_ServerRekeyingConfirm_t *rekeying_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerRekeyingConfirm_t));

		uint16_t len = hi_boot_srvrekeyingcnf_fill(rekeying_cnf, G3_BUSY, rekeying_error_none);
		g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_REKEYING_CNF, rekeying_cnf, len);
	}

    return boot_server.curr_state;
}

/**
  * @brief G3 Boot Server FSM function that handles the server re-keying abort request
  * @param [in] payload The re-keying abort request message payload data
  * @return The current Boot Server state
  */
static boot_srv_state_t g3_boot_srv_fsm_abort_rk_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_NONE;

	BOOT_ServerAbortRekeyingConfirm_t *abort_rk_req = MEMPOOL_MALLOC(sizeof(BOOT_ServerAbortRekeyingConfirm_t));

	if (	(boot_server.curr_substate != boot_srv_idle	 ) &&
			(boot_server.curr_substate != boot_srv_kickig) )
	{
		boot_server.rekeying_error = rekeying_error_abort;

		abort_rk_req->status = G3_SUCCESS;
	}
	else
	{
		abort_rk_req->status = G3_WRONG_STATE;

	}

	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_ABORT_RK_CNF, abort_rk_req, sizeof(*abort_rk_req));

    return boot_server.curr_state;
}


/**
  * @brief Handler of the BOOT-SERVER-START.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_start_req(const void *payload)
{
	BOOT_ServerStartRequest_t *server_start_req = (BOOT_ServerStartRequest_t*) payload;

	if (boot_server.curr_state == BOOT_SRV_ST_INACTIVE)
	{
		boot_server.pan_id = server_start_req->pan_id;
		boot_server.short_addr = server_start_req->server_addr;

#if BOOT_SERVER_IDS_LEN == 0
		memcpy(boot_server.ids, mac_address, sizeof(boot_server.ids));
#endif

		if (osTimerStart(bootTimerHandle, BOOT_SERVER_START_WAIT_TIME * configTICK_RATE_HZ) == osOK)
		{
			PRINT_G3_BOOT_SRV_INFO("Received START request, starting server in %u seconds\n", BOOT_SERVER_START_WAIT_TIME);
		}
		else
		{
			Error_Handler();
		}
	}
	else
	{
		HANDLE_SRV_ERROR(srv_err_cannot_start);

		BOOT_ServerStartConfirm_t *srvstart_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerStartConfirm_t));

		srvstart_cnf->status = G3_WRONG_STATE;
		g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_START_CNF, srvstart_cnf, sizeof(*srvstart_cnf));
	}
}

/**
  * @brief Handler of the BOOT-SERVER-STOP.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_stop_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_STOP_REQ;
}

/**
  * @brief Handler of the BOOT-SERVER-KICK.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_kick_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_KICK_REQ;
}

/**
  * @brief Handler of the BOOT-SERVER-SETPSK.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_setpsk_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_SETPSK_REQ;
}

/**
  * @brief Handler of the BOOT-SERVER-REKEYING.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_rekeying_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_REKEYING_REQ;
}

/**
  * @brief Handler of the BOOT-SERVER-ABORT_RK.request event
  * @param [in] payload The pointer to the payload of the message
  * @return None
  */
static void g3_boot_srv_handle_server_abort_rekeying_req(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_ABORT_RK_REQ;
}

/**
  * @brief Function that handles the reception of a ADP-DISCOVERY confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_srv_handle_discovery_cnf(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_DISCOVERY_CNF;
}

/**
  * @brief Function that handles the reception of a ADP-NETWORK-START confirm (coordinator only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_srv_handle_network_start_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
	const ADP_AdpmNetworkStartConfirm_t *network_start_cnf = payload;
	HANDLE_CNF_ERROR(HIF_ADPM_NTWSTART_CNF, network_start_cnf->status);
#else
	UNUSED(payload);
#endif

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_NETWORK_START_CNF;
}

/**
  * @brief Function that handles the reception of a ADP-LBP confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_srv_handle_lbp_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
	const ADP_AdpmLbpConfirm_t *lbp_cnf = payload;
	HANDLE_CNF_ERROR(HIF_ADPM_LBP_CNF, lbp_cnf->status);
#else
	UNUSED(payload);
#endif

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_LBP_CNF;
}

/**
  * @brief Function that handles the reception of a ADP-LBP indication.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_srv_handle_lbp_ind(const void *payload)
{
	UNUSED(payload);

	boot_server.curr_event = BOOT_SRV_EV_RECEIVED_LBP_IND;
}

/**
  * @brief Function that handles the reception of a G3LIB-SET confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_srv_handle_g3libset_cnf(const void *payload)
{
	const G3_LIB_SetAttributeConfirm_t *set_attr_cnf = payload;

	if ((set_attr_cnf->attribute_id.id == MAC_KEYTABLE_ID) || (set_attr_cnf->attribute_id.id == ADP_ACTIVEKEYINDEX_ID))
	{
		if (set_attr_cnf->status != G3_SUCCESS)
		{
			HANDLE_CNF_ERROR(HIF_G3LIB_SET_CNF, set_attr_cnf->status);

			boot_server.rekeying_error = rekeying_error_set_attribute;
		}

		if (set_attr_cnf->attribute_id.id == ADP_ACTIVEKEYINDEX_ID)
		{
			/* Completes the re-keying after the new GMK index has been set */
			g3_boot_srv_eap_rekeying_fsm();
		}
		else
		{
			/* Proceed to the "Send GMK" phase */
			osTimerStart(serverTimerHandle, REKEYING_SEND_GMK_PHASE_DELAY);
		}
	}
}

/**
  * @brief Checks if an internal request is needed by the G3 Boot Server application.
  * @param g3_msg Pointer to the G3 message structure to evaluate.
  * @return 'true' if the message is needed by the boot application, 'false' otherwise.
  */
static bool g3_boot_srv_req_needed(const g3_msg_t *g3_msg)
{
	switch (g3_msg->command_id)
	{
	case HIF_BOOT_SRV_START_REQ:
	case HIF_BOOT_SRV_STOP_REQ:
	case HIF_BOOT_SRV_KICK_REQ:
	case HIF_BOOT_SRV_REKEYING_REQ:
	case HIF_BOOT_SRV_ABORT_RK_REQ:
	case HIF_BOOT_SRV_SETPSK_REQ:
		return true;
	default:
		return false;
	}
}

/**
  * @brief Handles the reception of internal G3 Boot Server application requests.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
static void g3_boot_srv_req_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
    case HIF_BOOT_SRV_START_REQ:
		g3_boot_srv_handle_server_start_req(g3_msg->payload);
		break;
	case HIF_BOOT_SRV_STOP_REQ:
    	g3_boot_srv_handle_server_stop_req(g3_msg->payload);
    	break;
    case HIF_BOOT_SRV_KICK_REQ:
    	g3_boot_srv_handle_server_kick_req(g3_msg->payload);
    	break;
    case HIF_BOOT_SRV_REKEYING_REQ:
		g3_boot_srv_handle_server_rekeying_req(g3_msg->payload);
		break;
    case HIF_BOOT_SRV_ABORT_RK_REQ:
		g3_boot_srv_handle_server_abort_rekeying_req(g3_msg->payload);
		break;
    case HIF_BOOT_SRV_SETPSK_REQ:
    	g3_boot_srv_handle_server_setpsk_req(g3_msg->payload);
		break;
    default:
        break;
    }
}

/**
  * @brief State Machine main function
  */
static void g3_boot_srv_fsm_manager(const void *payload)
{
    // Extract last event (if any) from Event Queue
    boot_server.curr_state = g3_boot_srv_fsm_func_tbl[boot_server.curr_state][boot_server.curr_event](payload);
}

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Srv_Exported_Code
  * @{
  */

/* Public functions */

/**
  * @brief Initializes the G3 Boot Server application.
  * @param None
  * @retval None
  */
void g3_app_boot_srv_init(void)
{
	uint8_t default_gmk[MAC_KEY_SIZE] = DEFAULT_GMK;
	uint8_t empty_gmk[MAC_KEY_SIZE] = { 0 };

	/* Initial state is WAIT_TIMER */
	boot_server.curr_state	= BOOT_SRV_ST_INACTIVE;
	boot_server.nsdu_handle = 1;
	boot_server.short_addr	= 0;
	boot_server.pan_id		= 0;

#if BOOT_SERVER_IDS_LEN != 0
	memcpy(boot_server.ids, BOOT_SERVER_IDS, sizeof(boot_server.ids));
	reverse_array(boot_server.ids, sizeof(boot_server.ids));
#else
	memset(boot_server.ids, 0, sizeof(boot_server.ids));
#endif

	memcpy(boot_server.gmk[DEFAULT_GMK_INDEX], default_gmk, MAC_KEY_SIZE);
	memcpy(boot_server.gmk[1 - DEFAULT_GMK_INDEX], empty_gmk, MAC_KEY_SIZE);
	boot_server.gmk_index		= DEFAULT_GMK_INDEX;
	boot_server.gmk_index_new	= DEFAULT_GMK_INDEX;

	boot_server.curr_substate	= boot_srv_idle;
	boot_server.rekeying_error	= rekeying_error_none;
	boot_server.rekeying_index	= 0;
	boot_server.rekeying_count	= 0;
	boot_server.rekeyed_count	= 0;
	boot_server.activated_count = 0;

	/* The connected device list is initialized in 'g3_app_boot_init' */
}

/**
  * @brief Executes the G3 Boot Server application.
  * @param payload Payload of the received G3 message
  * @retval None
  */
void g3_app_boot_srv(void *payload)
{
	/* Manages the current event, depending on the current state */
	g3_boot_srv_fsm_manager(payload);
}

/**
  * @brief Checks if a message is needed by the G3 Boot Server application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed by the boot application, 'false' otherwise
  */
bool g3_app_boot_srv_msg_needed(const g3_msg_t *g3_msg)
{
	switch (g3_msg->command_id)
	{
	case HIF_ADPM_DISCOVERY_CNF:
	case HIF_ADPM_NTWSTART_CNF:
	case HIF_ADPM_LBP_CNF:
	case HIF_ADPM_LBP_IND:
	case HIF_G3LIB_SET_CNF:
		return true;
	default:
		return false;
	}
}

/**
  * @brief Handles the reception of G3 Boot Server application messages.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_boot_srv_msg_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
    /* Messages received by the coordinator only when the Boot Server is on the application layer */
    case HIF_ADPM_DISCOVERY_CNF:
    	g3_boot_srv_handle_discovery_cnf(g3_msg->payload);
    	break;
    case HIF_ADPM_NTWSTART_CNF:
    	g3_boot_srv_handle_network_start_cnf(g3_msg->payload);
    	break;
    case HIF_ADPM_LBP_CNF:
    	g3_boot_srv_handle_lbp_cnf(g3_msg->payload);
		break;
    case HIF_ADPM_LBP_IND:
    	g3_boot_srv_handle_lbp_ind(g3_msg->payload);
		break;
	case HIF_G3LIB_SET_CNF:
		g3_boot_srv_handle_g3libset_cnf(g3_msg->payload);
		break;
    default:
        break;
    }
}

/**
  * @brief Executes the G3 Boot Server application (internal requests).
  * @param g3_msg Pointer to the G3 message structure to evaluate.
  * @retval None
  */
void g3_app_boot_srv_req_handler(const g3_msg_t *g3_msg)
{
	void *payload = NULL;

	if (g3_msg != NULL)
	{
		if (g3_boot_srv_req_needed(g3_msg))
		{
			g3_boot_srv_req_handler(g3_msg);
			payload = g3_msg->payload;
		}
	}

	/* Manages the current event, depending on the current state. The payload can be null (timer event) */
	g3_boot_srv_fsm_manager(payload);
}

/**
  * @brief Executes the G3 Boot Server application (Re-keying).
  * @param g3_msg Unused (NULL)
  * @retval None
  */
void g3_app_boot_srv_rekeying(const g3_msg_t *g3_msg)
{
	UNUSED(g3_msg); /* Empty */

	g3_boot_srv_eap_rekeying_fsm();
}

/**
  * @brief Callback function of the bootTimer FreeRTOStimer for the PAN Coordinator.
  *        Triggers the timeout event for the Boot Server FSM
  * @param argument Unused argument.
  * @retval None
  */
void g3_app_boot_srv_timeoutCallback(void *argument)
{
	UNUSED(argument);

	boot_server.curr_event = BOOT_SRV_EV_TIMER_EXPIRED;

	RTOS_PUT_MSG(g3_queueHandle, BOOT_SRV_MSG, NULL); /* Needed to execute the boot application */
}

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

/******************* (C) COPYRIGHT 2021 STMicroelectronics *******************/
