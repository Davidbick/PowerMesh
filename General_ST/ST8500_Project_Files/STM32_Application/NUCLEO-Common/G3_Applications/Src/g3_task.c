/*****************************************************************************
*   @file    g3_task.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains code that implements the G3 task.
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
#include <assert.h>
#include <debug_print.h>
#include <crc.h>
#include <mem_pool.h>
#include <utils.h>
#include <host_if.h>
#include <pin_management.h>
#include <hi_msgs_impl.h>
#include <g3_comm.h>
#include <sflash.h>
#include <g3_app_config.h>
#include <g3_app_boot.h>
#include <g3_app_keep_alive.h>
#include <g3_app_last_gasp.h>
#include <g3_task.h>
#include <user_g3_common.h>
#include <user_image_transfer.h>
#include <user_mac.h>
#include <user_modbus.h>
#include <print_task.h>
#include <main.h>


/* Definitions */
#define TIMEOUT_CNF			90000 	/* Timeout for CNF reception, in ms */


#define CHANGE_BAUDRATE     (HIF_BAUDRATE != 115200) /* Determines if the baudrate has to be changed */

/* Debug Traces */
#define SET_TRACES_FILTER				0	/* Enables the filter setup for the debug traces */

#define DEBUG_TRACE_BIT_PHY_DATA		7 /* Trace data at PHY layer */
#define DEBUG_TRACE_BIT_PHY_DEBUG		8 /* Trace detailed information at PHY Layer */
#define DEBUG_TRACE_BIT_PHY_INFO		9 /* Trace high level information at PHY Layer */
#define DEBUG_TRACE_BIT_RTEI_DEBUG		10 /* Trace detailed information on the Cortex<->RTE interface */
#define DEBUG_TRACE_BIT_RTEI_INFO		11 /* Trace high level information on the Cortex<->RTE interface */
#define DEBUG_TRACE_BIT_MAC_DATA		12 /* Trace data at MAC layer */
#define DEBUG_TRACE_BIT_MAC_DEBUG 		13 /* Trace detailed information at MAC Layer */
#define DEBUG_TRACE_BIT_MAC_INFO 		14 /* Trace high level information at MAC Layer */
#define DEBUG_TRACE_BIT_ADP_DATA 		15 /* Trace data at ADP layer */
#define DEBUG_TRACE_BIT_ADP_DEBUG 		16 /* Trace detailed information at ADP Layer */
#define DEBUG_TRACE_BIT_ADP_INFO 		17 /* Trace high level information at ADP Layer */
#define DEBUG_TRACE_BIT_BOOT_DEBUG 		18 /* Trace detailed information at BOOT Layer */
#define DEBUG_TRACE_BIT_BOOT_INFO 		19 /* Trace high level information at BOOT Layer */
#define DEBUG_TRACE_BIT_G3LIB INFO 		20 /* Trace information regarding the G3-PLC stack configuration */
#define DEBUG_TRACE_BIT_RTEWDG DEBUG 	21 /* Trace information regarding the status of Cortex<->RTE interface */
#define DEBUG_TRACE_BIT_RESERVED		22 /* Reserved. Leave to 0 */
#define DEBUG_TRACE_BIT_FH_SCHEDULE_INFO 23 /* Trace information about frequency hopping schedule */
#define DEBUG_TRACE_BIT_GENERIC 		24 /* Trace information on the generic platform event */
#define DEBUG_TRACE_BIT_HI 				25 /* Trace information on the host interface */
#define DEBUG_TRACE_BIT_HI_DATA 		26 /* Trace data commands on the host interface */
#define DEBUG_TRACE_BIT_IPV6 			27 /* Trace information at Ipv6/UDP layers */
#define DEBUG_TRACE_BIT_APP				28 /* Trace information at application layer */
#define DEBUG_TRACE_BIT_RF				29	/* Trace information about RF interface */

/* Global Variables */
bool fast_restore_enabled = false;

/* External Variables */
extern rf_type_t        rf_type;
extern BOOT_Bandplan_t	working_plc_band;
extern plc_mode_t       working_plc_mode;
extern modbus_role_t    modbus_role;
#if !IS_COORD && ENABLE_FAST_RESTORE
extern flash_data_t		usr_flash_data;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

extern osMessageQueueId_t g3_queueHandle;
extern osMessageQueueId_t user_queueHandle;

extern osSemaphoreId_t 	semConfirmationHandle;

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_Task
  * @{
  */

/** @addtogroup G3_Task_Private_Code
  * @{
  */

#if RESET_AT_START
/**
 * @brief This functions handles the reception of the initial HWRESET confirmation.
 * @param None
 * @return Boolean that indicates if the HWRESET confirmation was received within the timeout expiration.
 */
static void g3_wait_for_hw_reset_cnf(void)
{
	task_msg_t 		task_msg;

	PRINT_G3_MSG_INFO("Waiting for %s...\n", translateG3cmd(HIF_HI_HWRESET_CNF));

	/* Waits for HOSTIF-HWRESET.Confirm, blocked until a message is available or timeout is reached */
	if (RTOS_GET_MSG_TIMEOUT(g3_queueHandle, &task_msg, TIMEOUT_HWRESET))
	{
		g3_msg_t	 *g3_msg = task_msg.data;

		if (g3_msg->command_id != HIF_HI_HWRESET_CNF)
		{
			Error_Handler();
		}

		PRINT_G3_MSG_INFO("Received %s\n", translateG3cmd(g3_msg->command_id));

		g3_discard_message(g3_msg); /* Message discarded (memory has to be freed) */
	}
	else
	{
		PRINT_G3_MSG_CRITICAL("No %s, check PE/RTE images, connection with ST8500 and if pass-through is active.\n", translateG3cmd(HIF_HI_HWRESET_CNF));
		PRINT("Going to sleep...");
		while (print_app_is_busy())
		{
			utils_delay_ms(1);
		}
		LED_SET_ALL_OFF();
		LED_SET_ON(LED_SLEEP);
		HAL_SuspendTick();
		HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		for(;;)
		{
			__WFI();
		}
	}
}
#endif /* RESET_AT_START */

#if CHANGE_BAUDRATE
/**
 * @brief This functions changes the baudrate of the host IF serial communication.
 * @param baudrate The new baudrate to set.
 * @return None.
 */
static void g3_change_hif_baudrate(const uint32_t baudrate)
{
	task_msg_t 		task_msg;

	/* Send HI-BAUDRATE-SET.request */
	hi_hwconfig_req_t hwconfig_req;

	hwconfig_req.type = BAUDRATE_CONFIG;
	hwconfig_req.payload_len = sizeof(hi_hwconfig_baudrateconfig_req_t);
	hwconfig_req.baudrate_conf.baud_rate = baudrate;
	host_if_send_message(HIF_HI_HWCONFIG_REQ, &hwconfig_req, sizeof(hwconfig_req));

	PRINT_G3_MSG_INFO("Waiting for %s...\n", translateG3cmd(HIF_HI_HWCONFIG_CNF));

	/* Waits for HOSTIF-HWCONFIG.Confirm, Blocked until a message is available or timeout is reached */
	if (!RTOS_GET_MSG_TIMEOUT(g3_queueHandle, &task_msg, TIMEOUT_BAUDRATE_SET))
	{
		Error_Handler();
	}

	g3_msg_t *g3_msg = task_msg.data;

	/* Check confirm message ID */
	if (g3_msg->command_id != HIF_HI_HWCONFIG_CNF)
	{
		PRINT_G3_MSG_CRITICAL("Received %s instead of %s\n", translateG3cmd(g3_msg->command_id), translateG3cmd(HIF_HI_HWCONFIG_CNF));
		Error_Handler();
	}

	hi_hwconfig_cnf_t *hwconfig_cnf = g3_msg->payload;

	if (	(hwconfig_cnf->status != G3_SUCCESS)									||	/* Checks result of operation */
			(hwconfig_cnf->type != BAUDRATE_CONFIG)				||	/* Checks operation type */
			(hwconfig_cnf->payload_len != sizeof(hi_hwconfig_baudrateconfig_cnf_t))	||	/* Checks length */
			(hwconfig_cnf->baudrate_conf.baud_rate != HIF_BAUDRATE	)						)	/* Checks new baudrate */
	{
		Error_Handler();
	}

	PRINT_G3_MSG_INFO("Received %s\n", translateG3cmd(g3_msg->command_id));

	g3_discard_message(g3_msg); /* Message discarded (memory has to be freed) */

	/* Reception must be aborted... */
	host_if_rx_stop();

	USART_changeBaudrate(&huartHostIf, baudrate); /* ...to change the baudrate... */

	utils_delay_ms(100); /* Delay necessary for the baudrate to be changed on platform side */

	/* ...and restarted */
	host_if_rx_start();
}
#endif

#if SET_TRACES_FILTER
/**
 * @brief This functions sets the filter for debug traces
 * @param None
 * @return None.
 */
static void g3_set_traces_filter(void)
{
	task_msg_t 		task_msg;

	/* Send HOSTIF-TRACE.Request */
	hif_trace_req_t trace_req;

	trace_req.control = TRACE_CONTROL_FIELD_VALUE;
	trace_req.data_1 =	(1 << DEBUG_TRACE_BIT_MAC_DEBUG)	|	\
						(1 << DEBUG_TRACE_BIT_MAC_INFO)		|	\
						(1 << DEBUG_TRACE_BIT_ADP_DEBUG)	|	\
						(1 << DEBUG_TRACE_BIT_ADP_INFO)		|	\
						(1 << DEBUG_TRACE_BIT_BOOT_DEBUG)	|	\
						(1 << DEBUG_TRACE_BIT_BOOT_INFO);
	//trace_req.data_1 = 0x3FBFFF80; // enable ALL traces
	trace_req.data_2 = 0;

	PRINT_G3_MSG_INFO("Waiting for %s...\n", translateG3cmd(HIF_HI_TRACE_CNF));

	host_if_send_message(HIF_HI_TRACE_REQ, &trace_req, sizeof(trace_req));

	/* Waits for HOSTIF-TRACE.Confirm, Blocked until a message is available or timeout is reached */
	if (!RTOS_GET_MSG_TIMEOUT(g3_queueHandle, &task_msg, TIMEOUT_TRACE))
	{
		Error_Handler();
	}

	g3_msg_t *g3_msg = task_msg.data;

	/* Check confirm message ID */
	if (g3_msg->command_id != HIF_HI_TRACE_CNF)
	{
		PRINT_G3_MSG_CRITICAL("Received %s instead of %s\n", translateG3cmd(g3_msg->command_id), translateG3cmd(HIF_HI_TRACE_CNF));
		Error_Handler();
	}

	PRINT_G3_MSG_INFO("Received %s\n", translateG3cmd(g3_msg->command_id));

	g3_discard_message(g3_msg); /* Message discarded (memory has to be freed) */
}
#endif

/**
 * @brief This functions handles the preliminary parsing of a G3 message, checks its integrity and forwards it to the user task, if needed.
 * @param msg Pointer to the G3 message structure
 * @retval None
 */
static void g3_msg_handler(const g3_msg_t *g3_msg)
{
	PRINT_G3_MSG_INFO("Recv <- %s (0x%X), %u bytes\n", translateG3cmd(g3_msg->command_id), g3_msg->command_id, g3_msg->size);

	/* Parses the message in the modules that need it */
	if (g3_app_conf_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Configuration module */
		g3_app_conf_msg_handler(g3_msg);

		/* Triggers its FSM */
		g3_app_conf();
	}

	if (g3_app_boot_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Boot module */
		g3_app_boot_msg_handler(g3_msg);
	}

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
	if (g3_app_boot_srv_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Boot Server module */
		g3_app_boot_srv_msg_handler(g3_msg);

		/* Triggers its FSM */
		g3_app_boot_srv(g3_msg->payload);
	}
#elif !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
	if (g3_app_boot_clt_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Boot Client module */
		g3_app_boot_clt_msg_handler(g3_msg);

		/* Triggers its FSM */
		g3_app_boot_clt(g3_msg->payload);
	}
#endif

#if ENABLE_ICMP_KEEP_ALIVE
	if (g3_app_ka_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Keep-Alive module */
		g3_app_ka_msg_handler(g3_msg);
	}
#endif

#if ENABLE_LAST_GASP
	if (g3_app_last_gasp_msg_needed(g3_msg))
	{
		/* Parses the message in the G3 Last Gasp module */
		g3_app_last_gasp_msg_handler(g3_msg);
#if !IS_COORD
		/* Triggers its FSM */
		g3_app_last_gasp();
#endif
	}
#endif
}

/**
 * @brief This functions forwards a G3 message to the User task, if needed.
 * @param g3_msg Pointer to the G3 message structure.
 * @retval "true" if the message is forwarded, "false" otherwise
 */
static bool g3_msg_forward(g3_msg_t *g3_msg)
{
	bool forward_needed = false;
	/* Checks if the user task needs to process the message as well */
	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		if (UserG3_MsgNeeded(g3_msg))
			forward_needed = true;
#if ENABLE_IMAGE_TRANSFER
		if (UserImgTransfer_MsgNeeded(g3_msg))
			forward_needed |= true;
#endif
					forward_needed = true;
	}
	else if (working_plc_mode == PLC_MODE_MAC)
	{
		if (UserMac_MsgNeeded(g3_msg))
		{
			forward_needed = true;
		}
	}

	if (forward_needed)
	{
		/* Forwards the G3 message to the User task */
		RTOS_PUT_MSG(user_queueHandle, G3_RX_MSG, g3_msg);
	}

	return forward_needed;
}

/**
  * @}
  */


/** @addtogroup G3_Task_Exported_Code
  * @{
  */

/**
 * @brief This functions initializes the G3 task and its applications.
 * @param None
 * @retval None
 */
void g3_task_init(void)
{
	/* Detects the RF module type */
	if (readRfSelect())
	{
		rf_type = RF_TYPE_915;
	}
	else
	{
		rf_type = RF_TYPE_868;
	}

	if (readFCCSelect())
	{
		working_plc_band = BOOT_BANDPLAN_FCC;
	}
	else
	{
		working_plc_band = BOOT_BANDPLAN_CENELEC_A;
	}

	/* Detects the selected mode */
	if (readMacSelect())
	{
		working_plc_mode = PLC_MODE_MAC;

		modbus_role = readModbusMasterSelect() ? modbus_master : modbus_slave;
	}
	else
	{
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
		working_plc_mode = PLC_MODE_IPV6_ADP; /* The BOOT layer is located on the host */
#elif !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
		working_plc_mode = PLC_MODE_IPV6_ADP; /* The BOOT layer is located on the host */
#else
		working_plc_mode = PLC_MODE_IPV6_BOOT; /* The BOOT layer is located on the platform */
#endif
	}

	/* Initializes Configuration module */
	g3_app_conf_init();

	if (working_plc_mode != PLC_MODE_MAC)
	{
		/* Initializes Boot module */
		g3_app_boot_init();

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
		/* Initializes Boot Server module */
		g3_app_boot_srv_init();
#elif !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
		/* Initializes Boot Client module */
		g3_app_boot_clt_init();
#endif

#if ENABLE_ICMP_KEEP_ALIVE
		/* Initializes Keep-Alive module */
		g3_app_ka_init();
#endif

#if !IS_COORD && ENABLE_LAST_GASP
		/* Initializes Last Gasp module */
		g3_app_last_gasp_init();
#endif
	}

	/* Initializes Host Interface */
	host_if_init();

	/* Add here more initializations for the G3 task */
}

/**
 * @brief This functions executes the G3 task routine.
 * @param None
 * @retval None
 */
void g3_task_exec(void)
{
	g3_msg_t *g3_msg;
	task_msg_t task_msg;

	osStatus_t	os_status;
#if RESET_AT_START
	g3_wait_for_hw_reset_cnf(); /* HW reset confirm must always be received */
#endif /* RESET_AT_START */

#if CHANGE_BAUDRATE
	g3_change_hif_baudrate(HIF_BAUDRATE);
#endif /* CHANGE_BAUDRATE */

#if SET_TRACES_FILTER
	g3_set_traces_filter();
#endif /* CHANGE_BAUDRATE */

#if !IS_COORD && ENABLE_FAST_RESTORE
	/* read fast_restore_enabled from STM32 internal Flash */
	fast_restore_enabled = (bool)usr_flash_data.str.fast_restore_enabled;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

	/* Starts G3 configuration by default */
	g3_app_conf_start(fast_restore_enabled);

	for(;;)
	{
		/* Reception of messages */
		if (RTOS_GET_MSG(g3_queueHandle, &task_msg))
		{
			g3_msg = task_msg.data; /* The payload of the message is a G3 message, or a Host Interface message (HIF) */

			switch (task_msg.message_type)
			{
			case G3_RX_MSG:	/* Message received from HIF UART */
				g3_msg_handler(g3_msg); /* Message handler for G3 messages */

				if (!g3_msg_forward(g3_msg)) 			/* Forwards to the User Task, if needed */
				{
					g3_discard_message(g3_msg); 		/* Discards it if there is no need to forward it to the User Task */
				}
				break;
			case HIF_TX_MSG: /* Checks if a user message is to be sent through the Host Interface */
#if !IS_COORD
				/* The start request does not need to wait for confirm, immediately */
				if (g3_msg->command_id != HIF_BOOT_DEV_START_REQ)
				{
#endif
					/* The ST8500 can handle a maximum of 2 requests at the same time, therefore a counting semaphore of 2 is used */
					os_status = osSemaphoreAcquire(semConfirmationHandle, TIMEOUT_CNF);
					if (os_status == osErrorTimeout)
					{
						PRINT_G3_MSG_WARNING("CNF timeout (%u ms)\n", TIMEOUT_CNF);
						osSemaphoreRelease(semConfirmationHandle);
					}

					/* Need no other request in case of G3ICMP-ECHO or G3UDP-DATA requests */
					if ((g3_msg->command_id == HIF_ICMP_ECHO_REQ) || (g3_msg->command_id == HIF_UDP_DATA_REQ))
					{
						if (osSemaphoreGetCount(semConfirmationHandle) == 0)
						{
							osSemaphoreAcquire(semConfirmationHandle, TIMEOUT_CNF);
							osSemaphoreRelease(semConfirmationHandle);
						}
					}
#if !IS_COORD
				}
#endif
				/* Transmission of messages through Host Interface */
				if (host_if_send_message(g3_msg->command_id, g3_msg->payload, g3_msg->size) != 0)
				{
					g3_discard_message(g3_msg); 			/* Message sent successfully, no forward */
				}
				else
				{
					Error_Handler();
				}
				break;
#if IS_COORD && ENABLE_ICMP_KEEP_ALIVE
			case KA_MSG: 								/* Internal messages for Keep-Alive module */
				g3_app_ka(); 							/* Keep-Alive module (Coordinator only) */
				g3_discard_message(g3_msg); 			/* No forward */
				break;
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
			case BOOT_SRV_MSG:	 						/* Internal messages for Boot Server module */
				g3_app_boot_srv_req_handler(g3_msg);	/* Boot Server request handler (Boot Server module) */
				g3_discard_message(g3_msg); 			/* No forward */
				break;
			case BOOT_REKEY_MSG:
				g3_app_boot_srv_rekeying(g3_msg);		/* Triggers execution of the Re-keying procedure inside Boot Server module */
				g3_discard_message(g3_msg); 			/* No forward */
				break;
#elif !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
			case BOOT_CLT_MSG:	 						/* Internal messages for Boot Client module */
				g3_app_boot_clt_req_handler(g3_msg);	/* Boot Client request handler (Boot Client module) */
				g3_discard_message(g3_msg); 			/* No forward */
				break;
#endif
#if !IS_COORD && ENABLE_LAST_GASP
			case LAST_GASP_MSG:
				g3_app_last_gasp_activate();
				g3_discard_message(g3_msg); 			/* No forward */
				break;
#endif
			default:
				Error_Handler(); /* Unexpected message type */
			}
		}
		else
		{
			Error_Handler(); /* Should never get here, either a message is available in the g3_queue or the g3_task is blocked */
		}
	}
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

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
