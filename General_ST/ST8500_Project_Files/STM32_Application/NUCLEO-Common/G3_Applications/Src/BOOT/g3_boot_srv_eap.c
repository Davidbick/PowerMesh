/*****************************************************************************
*   @file    g3_boot_srv_eap.c
*   @author  AMG/IPC Application Team
*   @brief   This file includes the functions used by the Bootstrap Server to handle EAP-PSK messages.
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
#include <stdlib.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <utils.h>
#include <g3_comm.h>
#include <hi_msgs_impl.h>
#include <g3_app_config.h>
#include <g3_app_boot_constants.h>
#include <g3_app_boot.h>
#include <g3_boot_srv_eap.h>
#include <main.h>


/** @addtogroup G3_ADP
  * @{
  */

/** @addtogroup G3_ADP_LBP
  * @{
  */

/** @addtogroup G3_ADP_LBP_EAP
  * @{
  */

/** @addtogroup G3_ADP_LBP_EAP_Private_Code
  * @{
  */

/* Definitions */

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
#define HANDLE_SRV_ERROR(error_id)		 g3_boot_srv_eap_error(error_id)
#else
#define HANDLE_SRV_ERROR(error_id)
#endif

typedef enum boot_srv_err_enum
{
	srv_err_none = 0,
	srv_err_disconnected,
	srv_err_no_entry,
	srv_err_rekeying_index_oor_no_proceed,
	srv_err_rekeying_index_oor_no_rollback,
	srv_err_entry_table_full_no_rollback,
} boot_srv_err_t;

/* External variables */
extern boot_server_t 		boot_server;

extern osMessageQueueId_t	g3_queueHandle;

extern osTimerId_t			bootTimerHandle;

extern osTimerId_t			serverTimerHandle;

/* State Functions */
static void g3_boot_srv_eap_fsm_default(          const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_send_1(           const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_recv_2(           const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_send_3(           const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_recv_4(           const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_accepted(         const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);
static void g3_boot_srv_eap_fsm_recv_param_result(const lbp_ind_t* lbp_eap_msg, boot_join_entry_t* join_entry);

/**
  * @brief State function for Bootstrap Procedure Handling
  */
typedef void g3_boot_srv_eap_fsm_func(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry);

/**
 * @brief  Event Handlers Table
 */
static g3_boot_srv_eap_fsm_func* const g3_boot_eap_fsm_func_tbl[BOOT_SRV_EAP_ST_CNT][BOOT_SRV_EAP_EV_CNT] =
{
/* 				   NONE,  					    RECEIVED_JOIN, 		         RECEIVED_SECOND,             PSK_ACQUIRED,		           RECEIVED_FOURTH              RECEIVED_CNF,                 RECEIVED_PARAM                         RECEIVED_KICK,			     */
/* WAIT_JOIN   */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_send_1,  g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default,  g3_boot_srv_eap_fsm_default,           g3_boot_srv_eap_fsm_default },
/* WAIT_SECOND */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_send_1,  g3_boot_srv_eap_fsm_recv_2,  g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default,  g3_boot_srv_eap_fsm_default,           g3_boot_srv_eap_fsm_default },
/* WAIT_PSK    */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_send_1,  g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_send_3,  g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default,  g3_boot_srv_eap_fsm_default,           g3_boot_srv_eap_fsm_default },
/* WAIT_FOURTH */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_send_1,  g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_recv_4,  g3_boot_srv_eap_fsm_default,  g3_boot_srv_eap_fsm_default,           g3_boot_srv_eap_fsm_default },
/* WAIT_CNF    */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_accepted, g3_boot_srv_eap_fsm_default,           g3_boot_srv_eap_fsm_default },
/* WAIT_PARAM  */ {g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default, g3_boot_srv_eap_fsm_default,  g3_boot_srv_eap_fsm_recv_param_result, g3_boot_srv_eap_fsm_default }
};

/* Private Functions */

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)
static void g3_boot_srv_eap_error(boot_srv_err_t error_id)
{
	PRINT_G3_BOOT_SRV_CRITICAL("Boot server EAP error: %u\n", error_id);
}
#endif

/**
 * @brief   Completes the bootstrap procedure by sending the BOOT-SERVER-JOIN.indication, registering the device in the connected devices list
 * 			and removing the given joining entry
 * @param   [in/out] join_entry The joining entry performing the bootstrap
 * @return  None
 */
static void g3_boot_complete_bootstrap(boot_join_entry_t* join_entry)
{
	/* Send Join Indication */
	BOOT_ServerJoinIndication_t *srvjoin_ind = MEMPOOL_MALLOC(sizeof(BOOT_ServerJoinIndication_t));

	uint16_t len = hi_boot_srvjoinind_fill(srvjoin_ind, join_entry);
	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_JOIN_IND, srvjoin_ind, len);

	/* Sets the device to the connected state */
	g3_app_boot_add_connected_device(join_entry);

	/* Remove entry when done */
	g3_boot_srv_join_entry_remove(join_entry);
}

/**
  * @brief   G3 Boot EAP FSM function that maintains the current state, with no further action.
  * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer (not used)
  * @param   [in/out] join_entry The joining entry for which the FSM is executed (not used)
  * @return  None
  */
static void g3_boot_srv_eap_fsm_default(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry)
{
	UNUSED(lbp_eap_msg);
	UNUSED(join_entry);
}

/**
 * @brief   G3 Boot EAP FSM function that sends the EAP message #1 to the given joining entry
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer
 * @param   [in/out] join_entry The joining entry for which the FSM is executed
 * @return  None
 */
static void g3_boot_srv_eap_fsm_send_1(const lbp_ind_t* lbp_eap_msg, boot_join_entry_t* join_entry)
{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, lbp_eap_msg->lbp_msg->header.lbd_addr, sizeof(lbp_eap_msg->lbp_msg->header.lbd_addr));
	PRINT_G3_BOOT_SRV_INFO("Received join request from %s\n", ext_addr_str);
#else
	UNUSED(lbp_eap_msg);
#endif

	/* Send Message 1 */
	g3_adp_lbp_eap_send_1(join_entry, boot_server.pan_id, boot_server.nsdu_handle++, boot_server.ids, sizeof(boot_server.ids));

	/* Starts the timer to handle timeouts */
	if (!osTimerIsRunning(bootTimerHandle))
	{
		osTimerStart(bootTimerHandle, BOOT_SERVER_BOOTSTRAP_TIMEOUT_CHECK_PERIOD);
	}
}

/**
 * @brief   G3 Boot EAP FSM function that parses the EAP message #2 from the given joining entry
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer
 * @param   [in/out] join_entry The joining entry for which the FSM is executed
 * @return  None
 */
static void g3_boot_srv_eap_fsm_recv_2(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry)
{
	bool success = false;
	bool send_decline = false;

	do
	{
		/* Check that LBA is not changed from last message */
		if (lbp_eap_msg->lba_addr != join_entry->lba_addr)
		{
			send_decline = true; /* Fatal error, LBA addresses do not match */
			break;
		}

		/* Decode EAP-PSK message #2 (Step 1): get the RAND_P and the ID_P fields */
		if (!eap_psk_decode_2_step1(lbp_eap_msg->eap_msg, &(join_entry->eap_psk_data)))
		{
			break; /* RFC 4764 - If a validity check fails, the message is silently discarded. */
		}

		/* Check the EAP Identifier to avoid duplicate messages */
		if (join_entry->eap_psk_data.eap_id != lbp_eap_msg->eap_msg->header.eap_header.id)
		{
			break; /* The Node remains in the same state */
		}

		success = true;
	} while(0);

	if (success)
	{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
		PRINT_G3_BOOT_SRV_INFO("Received bootstrap message #2 from %s (LBA=0x%04X)\n", ext_addr_str, join_entry->lba_addr);
#endif

		BOOT_ServerGetPSKIndication_t 	*getpsk_ind = MEMPOOL_MALLOC(sizeof(BOOT_ServerGetPSKIndication_t));

		uint16_t len = hi_boot_srvgetpskind_fill(getpsk_ind, join_entry);
		g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_GETPSK_IND, getpsk_ind, len);

		join_entry->getpsk_timestamp = HAL_GetTick();
		join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_PSK;
	}
	else
	{
		if (send_decline)
		{
			g3_adp_lbp_send_decline(join_entry, boot_server.pan_id, boot_server.nsdu_handle++); /* Send LBP Message and disconnect the Node */
		}

		if (join_entry->rekeying)
		{
			boot_server.rekeying_error = rekeying_error_msg_2;

			g3_boot_srv_eap_rekeying_fsm();
		}
		else
		{
			join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_JOIN;
		}
	}
}



/**
 * @brief   G3 Boot EAP FSM function that sends the EAP message #3 to the given joining entry
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer (empty for this state)
 * @param   [in/out] join_entry The joining entry for which the FSM is executed
 * @return  None
 */
static void g3_boot_srv_eap_fsm_send_3(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry)
{
	UNUSED(lbp_eap_msg); /* Empty, this state is triggered by the reception of the SETPSK.Request */
	bool success = false;
	bool send_decline = false;

	do
	{
		/* Verify short address */
		if (join_entry->short_addr == MAC_BROADCAST_SHORT_ADDR)
		{
			send_decline = true;
			break;
		}

		/* Decode EAP-PSK message #2 (Step 2): verify MAC_P (and thus, the PSK) */
		if (!eap_psk_decode_2_step2(&join_entry->eap_psk_data, boot_server.ids, sizeof(boot_server.ids)))
		{
			break; /* RFC 4764 - If a validity check fails, the message is silently discarded. */
		}

		success = true;
	} while (0);

	if (success)
	{
		uint16_t short_address;
		uint8_t *gmk0;
		uint8_t *gmk1;
		uint8_t gmk_index;

		assert(boot_server.gmk_index < 2);
		assert(boot_server.gmk_index_new < 2);

		/* Parameters are chosen depending on the current situation/case */
		if (join_entry->rekeying)
		{
			/* Re-keying case, short address is excluded */
			short_address = MAC_BROADCAST_SHORT_ADDR;

			/* Only the new GMK and its index are sent */
			if (boot_server.gmk_index_new == 1)
			{
				gmk0 = NULL;
				gmk1 = boot_server.gmk[1];
			}
			else
			{
				gmk0 = boot_server.gmk[0];
				gmk1 = NULL;
			}

			gmk_index = NO_ACTIVE_INDEX;
		}
		else
		{
			/* Bootstrap case, short address is included */
			short_address = join_entry->short_addr;

			if (boot_server.curr_substate == boot_srv_rekeying_step_send_gmk)
			{
				/* If bootstrap happens while re-keying, during the distribution, sends the  GMK index directly and both GMK */
				gmk0 = boot_server.gmk[0];
				gmk1 = boot_server.gmk[1];
				gmk_index = boot_server.gmk_index;
			}
			else if (boot_server.curr_substate == boot_srv_rekeying_step_activate_gmk)
			{
				/* If bootstrap happens while re-keying, during the activation, sends the new GMK index directly and both GMK */
				gmk0 = boot_server.gmk[0];
				gmk1 = boot_server.gmk[1];
				gmk_index = boot_server.gmk_index_new;
			}
			else if (	(boot_server.curr_substate == boot_srv_rekeying_step_set_gmk_index			) ||
						(boot_server.curr_substate == boot_srv_rekeying_step_set_gmk_index_wait_cnf	) )
			{
				/* If bootstrap happens while re-keying, after activation, sends the new GMK index directly and only the new GMK */
				if (boot_server.gmk_index_new == 1)
				{
					gmk0 = NULL;
					gmk1 = boot_server.gmk[1];
				}
				else
				{
					gmk0 = boot_server.gmk[0];
					gmk1 = NULL;
				}

				gmk_index = boot_server.gmk_index_new;
			}
			else
			{
				/* If bootstrap happens in all other cases, sends only the active GMK and its index */
				if (boot_server.gmk_index == 1)
				{
					gmk0 = NULL;
					gmk1 = boot_server.gmk[1];
				}
				else
				{
					gmk0 = boot_server.gmk[0];
					gmk1 = NULL;
				}

				gmk_index = boot_server.gmk_index;
			}
		}

		/* For bootstrap and re-keying, even the old GMK is needed to communicate with bootstrapping devices, so both are sent */
		g3_adp_lbp_eap_send_3(join_entry, boot_server.pan_id, boot_server.nsdu_handle++, boot_server.ids, sizeof(boot_server.ids), short_address, gmk0, gmk1, gmk_index);
	}
	else
	{
		if (send_decline)
		{
			/* If the assigned short address is 0xFFFF, it means the Node is not allowed to join the PAN */
			g3_adp_lbp_send_decline(join_entry, boot_server.pan_id, boot_server.nsdu_handle++);
		}

		if (join_entry->rekeying)
		{
			boot_server.rekeying_error = rekeying_error_msg_2;
		}

		join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_JOIN;
	}
}

/**
 * @brief   G3 Boot EAP FSM function that parses the EAP #4 message from the given joining entry
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer
 * @param   [in/out] join_entry The data used by the server to complete the Joining procedure with the Node
 * @return  None
 */
static void g3_boot_srv_eap_fsm_recv_4(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry)
{
	uint8_t success  = false;
	uint8_t send_decline = 0;

	uint16_t p_channel_len = 0;
	eap_psk_channel_t* p_channel;

	do {
		if (lbp_eap_msg->lba_addr != join_entry->lba_addr)
		{
			send_decline = 1; /* Fatal error, LBA addresses do not match (has changed from the last message) */
			break;
		}

		/* Decode EAP-PSK message #4 */
		if (!eap_psk_decode_4(lbp_eap_msg->eap_msg, &(join_entry->eap_psk_data), &p_channel_len))
		{
			break; /* RFC 4764 - If a validity check fails, the message is silently discarded. */
		}

		// Check the EAP Identifier to avoid duplicate messages
		if (join_entry->eap_psk_data.eap_id != lbp_eap_msg->eap_msg->header.eap_header.id)
		{
			break; /* The Node remains in the same state */
		}

		/* Check the PCHANNEL length */
		if (p_channel_len <= 0)
		{
			break; /* The Node remains in the same state */
		}

		p_channel = &(lbp_eap_msg->eap_msg->msg.n4.p_channel);

		// Check the result indication Flag R and the extension flag
		if ((p_channel->E_R.R != p_channel_result_success) || (!p_channel->E_R.E))
		{
			send_decline = 2;
			break;
		}

		p_channel_len--; /* Don't need flag R, flag E and 5-bit Reserved field anymore */

		// Check if protected data carryies EXT-Type field
		if ((p_channel_len <= 0) || (p_channel->ext.type != adp_config_param_ext_type))
		{
			send_decline = 3;
			break;
		}

		p_channel_len--; /* Don't need EXT_Type field anymore */

		if (!g3_adp_lbp_decode_4_params(p_channel->ext.payload, p_channel_len))
		{
			send_decline = 4;
			break;
		}

		success = true;
	} while(0);

	if (success)
	{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, sizeof(join_entry->ext_addr));
		PRINT_G3_BOOT_SRV_INFO("Received bootstrap message #4 from %s (LBA=0x%04X)\n", ext_addr_str, join_entry->lba_addr);
#endif

		// Increment the EAP Identifier to be used for the next Request
		join_entry->eap_psk_data.eap_id++;

		/* Assiegns tha accept handle */
		join_entry->accept_handle = boot_server.nsdu_handle;

		/* Send success message */
		g3_adp_lbp_send_accept(join_entry, boot_server.pan_id, boot_server.nsdu_handle++);

		/* Need to wait for the LBP confirm to verify the completion of the bootstrap */
		join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_CNF;
	}
	else
	{
		if (send_decline)
		{
			PRINT_G3_BOOT_SRV_WARNING("Decline code %u\n", send_decline);
			g3_adp_lbp_send_decline(join_entry, boot_server.pan_id, boot_server.nsdu_handle++); /* Send LBP Message and disconnect the Node */
		}

		if (join_entry->rekeying)
		{
			boot_server.rekeying_error = rekeying_error_msg_4;

			g3_boot_srv_eap_rekeying_fsm();
		}
		else
		{
			join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_JOIN;
		}
	}
}

/**
 * @brief   G3 Boot EAP FSM function that completes the bootstrap after the accept for the given joining entry has successfully been sent
 * @param   lbp_eap_msg Unused, can be left to NULL
 * @param   join_entry The data used by the server to complete the Joining procedure with the Node
 * @return  None
 */
static void g3_boot_srv_eap_fsm_accepted(const lbp_ind_t *lbp_eap_msg, boot_join_entry_t *join_entry)
{
	UNUSED(lbp_eap_msg);

	if (join_entry->rekeying)
	{
		/* Set device to the next re-keying step */
		join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_PARAM;

#if ENABLE_REKEYING_DELAYS
		if (boot_server.rekeyed_count < boot_server.rekeying_count)
		{
			/* Continues the "Send GMK" phase */
			g3_boot_srv_eap_rekeying_fsm();
		}
		else
		{
			/* Proceeds to the "Activate GMK" phase */
			osTimerStart(serverTimerHandle, REKEYING_ACTIVATE_GMK_PHASE_DELAY);
		}
#else
		/* Continues the "Send GMK" phase or proceeds to the "Activate GMK" phase */
		g3_boot_srv_eap_rekeying_fsm();
#endif
	}
	else
	{
		/* Bootstrap complete */
		g3_boot_complete_bootstrap(join_entry);
	}
}

/**
 * @brief   G3 Boot EAP FSM function that parses the Parameter Result message from the given joining entry.
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer
 * @param   [in/out] join_entry The joining entry for which the FSM is executed
 * @return  None
 */
static void g3_boot_srv_eap_fsm_recv_param_result(const lbp_ind_t* lbp_eap_msg, boot_join_entry_t* join_entry)
{
	bool success = false;
	char result_str[8];

	if (	(lbp_eap_msg->conf_param_msg->header.attr_id_type.attr_id	  == ADP_CONF_PARAM_RESULT_ID	 			) &&
			(lbp_eap_msg->conf_param_msg->header.length 			  	  == sizeof(adp_param_result_param_value_t)	) &&
			(lbp_eap_msg->conf_param_msg->value.param_result_param.result == ADP_RESULT_PARAMETER_SUCCESS			) )
	{
		success = true;
		strncpy(result_str, "SUCCESS", sizeof(result_str));
	}
	else
	{
		strncpy(result_str, "FAILURE", sizeof(result_str));
	}

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, lbp_eap_msg->lbp_msg->header.lbd_addr, sizeof(lbp_eap_msg->lbp_msg->header.lbd_addr));
	PRINT_G3_BOOT_SRV_INFO("Received parameter result from %s: %s\n", ext_addr_str, result_str);
#else
	UNUSED(lbp_eap_msg);
#endif
	if (success)
	{
		if (join_entry->rekeying)
		{
			join_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_JOIN;
#if ENABLE_REKEYING_DELAYS
			if (boot_server.activated_count < boot_server.rekeying_count)
			{
				/* Continues the "Activate GMK" phase */
				g3_boot_srv_eap_rekeying_fsm();
			}
			else
			{
				/* Proceeds to the "Set GMK index" phase */
				osTimerStart(serverTimerHandle, REKEYING_SET_GMK_INDEX_PHASE_DELAY);
			}
#else
			/* Continues the "Activate GMK" phase or proceeds to the "Set GMK index" phase */
			g3_boot_srv_eap_rekeying_fsm();
#endif
		}
		else
		{
			g3_boot_complete_bootstrap(join_entry);
		}
	}
	else
	{
		if (join_entry->rekeying)
		{
			/* Handles the error of the parameter configuration */
			boot_server.rekeying_error = rekeying_error_param;

			g3_boot_srv_eap_rekeying_fsm();
		}
	}
}

/**
 * @brief   G3 Boot EAP function that handles the completion of the re-keying procedure, in case of success or failure
 * @param   None
 * @return  None
 */
static void g3_boot_srv_eap_complete_rekeying(void)
{
	g3_result_t result;

	if (boot_server.rekeying_error == rekeying_error_none)
	{
		/* In case of success, synchronizes variable to the attribute value */
		boot_server.gmk_index = boot_server.gmk_index_new;

		result = G3_SUCCESS;
	}
	else
	{
		result = G3_FAILED;
	}

	boot_join_entry_t* join_entry;

	/* Removes all entries related to re-keying */
	for (uint32_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		join_entry = g3_boot_srv_join_entry_get(i);

		if (join_entry->rekeying)
		{
			g3_boot_srv_join_entry_remove(join_entry);
		}
	}

	/* Prepare and send SRV-REKEYING-Confirm */
	BOOT_ServerRekeyingConfirm_t *rekeying_cnf = MEMPOOL_MALLOC(sizeof(BOOT_ServerRekeyingConfirm_t));

	uint16_t len = hi_boot_srvrekeyingcnf_fill(rekeying_cnf, result, boot_server.rekeying_error);
	g3_send_message(G3_RX_MSG, HIF_BOOT_SRV_REKEYING_CNF, rekeying_cnf, len);

	/* Reset re-keying status */
	boot_server.curr_substate = boot_srv_idle;
}

/**
 * @brief   G3 Boot EAP function that creates/updates the list of re-keying entries
 * @param   None
 * @return  None
 */
static boot_srv_rk_err_t g3_boot_srv_eap_update_rekeying_list(void)
{
	boot_srv_rk_err_t result = rekeying_error_none;

	boot_join_entry_t *rekeying_entry;
	uint8_t *ext_addr;
	uint16_t short_addr;
	uint8_t media_type;
	uint8_t disable_bkp;

	for (uint32_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		if (boot_server.connected_devices[i].conn_state == boot_state_connected)
		{
			rekeying_entry = g3_boot_srv_join_entry_find(boot_server.connected_devices[i].ext_addr);

			/* If not found in the joining entries */
			if (rekeying_entry == NULL)
			{
				ext_addr    = boot_server.connected_devices[i].ext_addr;
				short_addr  = boot_server.connected_devices[i].short_addr;
				media_type  = boot_server.connected_devices[i].media_type;
				disable_bkp = boot_server.connected_devices[i].disable_bkp;

				/* Initialize the new entry, with short address (needed to cipher messages), LBA = LBD in this case */
				rekeying_entry = g3_boot_srv_join_entry_add(ext_addr, short_addr, media_type, disable_bkp, true);

				if (rekeying_entry != NULL)
				{
					rekeying_entry->short_addr = short_addr;

					boot_server.rekeying_count++;

					/* If the current step is the (de)activation of the GMK, the entry is set in the status where it waits for the config. parameter */
					if (	(boot_server.curr_substate == boot_srv_rekeying_step_activate_gmk	) ||
							(boot_server.curr_substate == boot_srv_rekeying_step_deactivate_gmk	) )
					{
						rekeying_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_PARAM;
					}
				}
				else
				{
					result = rekeying_error_table_full;
				}
			}
			else
			{
				/* If it is found, then it is bootstrapping or already in list, and it won't be added to the list */
			}
		}
	}

	return result;
}

/**
  * @}
  */

/** @addtogroup G3_ADP_LBP_EAP_Exported_Code
  * @{
  */

/**
 * @brief   G3 Boot EAP FSM function that handles the full re-keying procedure and, in case of error, its roll-back.
 * @param   None
 * @return  None
 * @details This function is called:
 * 				- At the reception of the BOOT-SRV-REKEYING.request.
 * 				- At the reception of the EAP-PSK message #2 and #4 (in case of error).
 * 				- At the reception of the EAP accept message.
 * 				- At the reception of the configuration parameter result message.
 * 				- At the reception of the G3LIB-SET.confirm.
 * 			The steps of the re-keying procedure are the following:
 * 				1. Write the new GMK in the not active index of the internal GMK array of the coordinator, by setting the relative attribute.
 * 				2. Perform the bootstrap procedure with each connected device, to distribute the new GMK.
 * 				3. Send a configuration parameter message to each connected device, to switch their index of the active GMK.
 * 				4. Switch the index of the active GMK of the coordinator, by setting the relative attribute.
 * 				5. Align the Boot Server variable to the set attribute and send back a positive BOOT-SERVER-REKEYING.confirm.
 * 			The steps of the roll-back procedure are the following (the initial step depends on the failed step of the re-keying procedure):
 *				1. Switch the index of the active GMK of the coordinator to its original value, by setting the relative attribute.
 *				2. Send a configuration parameter message to each connected device, to switch their index of the active GMK to its original value.
 *				3. Send back a negative BOOT-SERVER-REKEYING.confirm.
 * @note	For the re-keying procedure, additional steps (after step 4.) to remove the previous GMK on the connected devices and the coordinator could be added.
 * 			For the roll-back procedure, additional steps (after step 2.) to remove the new GMK on the connected devices and the coordinator could be added.
 */
void g3_boot_srv_eap_rekeying_fsm(void)
{
	uint16_t len;
	boot_join_entry_t *rekeying_entry;
	G3_LIB_SetAttributeRequest_t *set_attr_req = NULL;

	/* Changes state in case of exceptions */
	if (		(boot_server.curr_substate == boot_srv_rekeying_step_send_gmk) &&
				(boot_server.connected_devices_number == 0))
	{
		PRINT_G3_BOOT_SRV_WARNING("There is no device to update, will only change the GMK of the coordinator\n");
		boot_server.curr_substate = boot_srv_rekeying_step_set_gmk_index;
	}
	else if (	(boot_server.curr_substate == boot_srv_rekeying_step_activate_gmk) &&
				(boot_server.rekeying_error != rekeying_error_none				 ) )
	{
		/* In case of error, goes from activation to de-activation */
		PRINT_G3_BOOT_SRV_WARNING("Rolling-back...\n");
		boot_server.curr_substate = boot_srv_rekeying_step_deactivate_gmk;
	}

	/* Re-keying FSM */
	switch(boot_server.curr_substate)
	{
	case boot_srv_idle:
		break;
	case boot_srv_rekeying_step_set_gmk:
		boot_server.rekeying_index	= 0;
		boot_server.rekeying_count  = 0;
		boot_server.rekeyed_count	= 0;
		boot_server.activated_count = 0;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(MAC_KEY_SIZE));

		/* Sets the new GMK key in the free slot */
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_KEYTABLE_ID, boot_server.gmk_index_new, boot_server.gmk[boot_server.gmk_index_new], MAC_KEY_SIZE);
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

		boot_server.curr_substate = boot_srv_rekeying_step_send_gmk;

		/* Creates the list of devices to re-key */
		boot_server.rekeying_error = g3_boot_srv_eap_update_rekeying_list();

		/* Terminate Re-keying in case of error */
		if (boot_server.rekeying_error != rekeying_error_none)
		{
			g3_boot_srv_eap_complete_rekeying();
		}
		break;
	case boot_srv_rekeying_step_send_gmk:
		if (boot_server.rekeying_error == rekeying_error_none)
		{
			while (boot_server.rekeying_index < BOOT_MAX_NUM_JOINING_NODES)
			{
				rekeying_entry = g3_boot_srv_join_entry_get(boot_server.rekeying_index);

				if (rekeying_entry->rekeying)
				{
					PRINT_G3_BOOT_SRV_INFO("Re-keying device %u/%u, short address: %u\n", boot_server.rekeyed_count+1, boot_server.connected_devices_number, rekeying_entry->short_addr);

					/* Send Message 1 */
					g3_adp_lbp_eap_send_1(rekeying_entry, boot_server.pan_id, boot_server.nsdu_handle++, boot_server.ids, sizeof(boot_server.ids));

					boot_server.rekeyed_count++;

					if (boot_server.rekeyed_count == boot_server.rekeying_count)
					{
						boot_server.rekeying_index = 0;

						boot_server.curr_substate  = boot_srv_rekeying_step_activate_gmk;

						/* Updates the list to keep into account the devices that bootstrapped in the meanwhile */
						boot_server.rekeying_error = g3_boot_srv_eap_update_rekeying_list();
					}
					else
					{
						boot_server.rekeying_index++;
					}
					break;
				}
				else
				{
					boot_server.rekeying_index++;
				}
			}

			if (boot_server.rekeying_index >= BOOT_MAX_NUM_JOINING_NODES)
			{
				/* If the code gets here, it means that the list was fully scanned without finding the expected number of devices */
				boot_server.rekeying_error = rekeying_error_procedure;
				g3_boot_srv_eap_error(srv_err_rekeying_index_oor_no_proceed);
				g3_boot_srv_eap_complete_rekeying();
			}
		}

		/* Terminate Re-keying in case of error */
		if (boot_server.rekeying_error != rekeying_error_none)
		{
			g3_boot_srv_eap_complete_rekeying();
		}
		break;
	case boot_srv_rekeying_step_activate_gmk:
		if (boot_server.rekeying_error == rekeying_error_none)
		{
			while (boot_server.rekeying_index < BOOT_MAX_NUM_JOINING_NODES)
			{
				rekeying_entry = g3_boot_srv_join_entry_get(boot_server.rekeying_index);

				if (rekeying_entry->rekeying)
				{
					/* Print before increment, requires +1 */
					PRINT_G3_BOOT_SRV_INFO("GMK Activation for device %u/%u, short addr: %u\n", boot_server.activated_count+1, boot_server.rekeying_count, rekeying_entry->short_addr);

					/* Send Message GMK-Activation */
					g3_adp_lbp_send_gmk_activation(rekeying_entry, boot_server.pan_id, boot_server.nsdu_handle++, boot_server.gmk_index_new);

					boot_server.activated_count++;
					boot_server.rekeying_index++;

					if (boot_server.activated_count == boot_server.rekeying_count)
					{
						/* All devices received the GMK-Activation */
						boot_server.curr_substate = boot_srv_rekeying_step_set_gmk_index;
					}
					break;
				}
				else
				{
					boot_server.rekeying_index++;
				}
			}

			if (boot_server.rekeying_index >= BOOT_MAX_NUM_JOINING_NODES)
			{
				/* If the code gets here, it means that the list was fully scanned without finding the expected number of devices */
				boot_server.rekeying_error = rekeying_error_procedure;
				g3_boot_srv_eap_error(srv_err_rekeying_index_oor_no_proceed);
				g3_boot_srv_eap_complete_rekeying();
			}
		}

		/* Do NOT terminate re-keying in case of error */
		break;
	case boot_srv_rekeying_step_deactivate_gmk:
		if (boot_server.activated_count == 0)
		{
			/* All devices have been deactivated  */
			g3_boot_srv_eap_complete_rekeying();
		}
		else
		{
			/* Looks for the next device to deactivate */
			while (boot_server.rekeying_index > 0)
			{
				/* Index value is in the range 1 : BOOT_MAX_NUM_JOINING_NODES. Its value is the last used index + 1 */
				rekeying_entry = g3_boot_srv_join_entry_get(boot_server.rekeying_index - 1);

				if (rekeying_entry->rekeying)
				{
					/* Rolls-back the state of the entry */
					rekeying_entry->curr_state = BOOT_SRV_EAP_ST_WAIT_PARAM;

					/* Print before decrement, does not require +1 */
					PRINT_G3_BOOT_SRV_INFO("GMK-Deactivation for device %u/%u, short address: %u\n", boot_server.activated_count, boot_server.rekeying_count, rekeying_entry->short_addr);

					/* Send Message GMK-Activation */
					g3_adp_lbp_send_gmk_activation(rekeying_entry, boot_server.pan_id, boot_server.nsdu_handle++, boot_server.gmk_index);

					/* Decreases the active count (one device has been deactivated) */
					boot_server.activated_count--;

					/* Decreases the index if necessary */
					if (boot_server.activated_count > 0)
					{
						boot_server.rekeying_index--;
					}
					break;
				}
				else
				{
					boot_server.rekeying_index--;
				}
			}

			if (boot_server.rekeying_index == 0)
			{
				/* Should not reach here. If so, returns an error */
				g3_boot_srv_eap_error(srv_err_rekeying_index_oor_no_rollback);
				g3_boot_srv_eap_complete_rekeying();
			}
		}
		break;
	case boot_srv_rekeying_step_set_gmk_index:
		/* Sets the attribute about the index of the active GMK to the new value */
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(boot_server.gmk_index_new)));

		len = hi_g3lib_setreq_fill(set_attr_req, ADP_ACTIVEKEYINDEX_ID, 0, &boot_server.gmk_index_new, sizeof(boot_server.gmk_index_new));
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

		boot_server.curr_substate = boot_srv_rekeying_step_set_gmk_index_wait_cnf;
		break;
	case boot_srv_rekeying_step_set_gmk_index_wait_cnf:
		if (boot_server.rekeying_error == rekeying_error_none)
		{
			g3_boot_srv_eap_complete_rekeying();
		}
		else
		{
			/* Roll-backs and resets the attribute about the index of the active GMK */
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(boot_server.gmk_index)));

			len = hi_g3lib_setreq_fill(set_attr_req, ADP_ACTIVEKEYINDEX_ID, 0, &boot_server.gmk_index, sizeof(boot_server.gmk_index));
			g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

			PRINT_G3_BOOT_SRV_WARNING("Rolling-back...\n");
			boot_server.curr_substate = boot_srv_rekeying_step_deactivate_gmk;

			g3_boot_srv_eap_update_rekeying_list();
		}
		break;
	default:
		Error_Handler();
	}
}

/**
 * @brief   Main function used by the server to handle the events generated from LBP messages.
 * @param   [in] lbp_eap_msg The data containing the LBP-EAP message received from the ADP Layer
 * @param   [in/out] join_entry The joining entry for which the FSM is executed
 * @return  None
 */
void g3_boot_srv_eap_fsm_manager(const lbp_ind_t* lbp_eap_msg, boot_join_entry_t* join_entry)
{
	g3_boot_eap_fsm_func_tbl[join_entry->curr_state][join_entry->curr_event](lbp_eap_msg, join_entry);
};

/**
 * @brief Callback function of the serverTimer timer.
 * @param argument Unused argument.
 * @retval None
 */
void g3_boot_srv_eap_timeoutCallback(void *argument)
{
	UNUSED(argument);

	RTOS_PUT_MSG(g3_queueHandle, BOOT_REKEY_MSG, NULL); /* Needed to execute the boot application */
}

#endif /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */


/******************* (C) COPYRIGHT 2013 STMicroelectronics *******************/
