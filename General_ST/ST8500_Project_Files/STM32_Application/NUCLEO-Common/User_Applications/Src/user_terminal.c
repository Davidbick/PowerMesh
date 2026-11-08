/**
 ******************************************************************************
 * @file    user_terminal.c
 * @author  AMG/IPC Application Team
 * @brief   Implementation of the User Terminal, used as application input/output.
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
#include <settings.h>
#include <debug_print.h>
#include <sflash_info.h>
#include <sflash.h>
#include <image_management.h>
#include <mem_pool.h>
#include <utils.h>
#include <g3_app_boot.h>
#include <g3_app_boot_clt.h>
#include <g3_app_config.h>
#include <g3_app_config_rf_params.h>
#include <g3_app_keep_alive.h>
#include <g3_app_last_gasp.h>
#include <user_g3_common.h>
#include <user_image_transfer.h>
#include <user_mac.h>
#include <user_terminal.h>
#include <main.h>
#include <version.h>

/** @addtogroup User_App
 * @{
 */

/** @addtogroup User_Terminal
 * @{
 */

/** @addtogroup User_Terminal_Private_Code
 * @{
 */

/* Definitions */
#define ESC_ASCII_CODE                                                   (0x1B)
#define ASCII_OFFSET                                                     (0x30)

/* MAC test */
#define ASCII_DELIMITER													    "."
#define STR_PLCRF_LEN														5
#define STR_PLC_LEN															3
#define STR_RF_LEN															2
#define DEFAULT_TEST_MSG_NUMBER												10

/* UDP tests */

/* Multicast address (used for broadcast) */
#define IPV6_MULTICAST_ADDR 	{ 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }

/* Timing */
#define INDICATION_TIMEOUT_DELAY                                       (40000)	/* In ms */
#define LOOPBACK_TIMEOUT_DELAY                                         (40000)	/* In ms */

#define KICK_TIMEOUT_DELAY                                             (15000)	/* In ms */
#define DISCONNECT_TIMEOUT_DELAY                                       (15000)	/* In ms */
#define RECONNECT_TIMEOUT_DELAY                                        (42000)	/* In ms */
#define REKEYING_TIMEOUT_DELAY                                         (120000)	/* In ms */


/* Private macros -----------------------------------------------------------*/
#define PARSE_CMD_CHAR(ch) 			(	(	user_input != NULL			 	 	) && \
										(	user_input->payload[0U] == ch 	    ) && \
										(	(user_input->payload[1U] == '\r')	||   \
											(user_input->payload[1U] == '\n')	) )

#define PARSE_CMD_IN_RANGE(min, max) (	(	user_input != NULL			 	 	) && \
										(	user_input->payload[0U] >= min 	) && \
										(	user_input->payload[0U] <= max 	) && \
										(	(user_input->payload[1U] == '\r')	||   \
											(user_input->payload[1U] == '\n')	) )

#define GET_CMD_DIGIT()			(user_input->payload[0U] - '0')

#define IS_ASCII_DIGIT(x) 		((x >= ASCII_OFFSET) && (x <= (ASCII_OFFSET + 9)))
#define IS_NOT_ASCII_DIGIT(x) 	((x <  ASCII_OFFSET) || (x >  (ASCII_OFFSET + 9)))
#define IS_HEX_LOWER(x) 		((x == 'a') || (x == 'b') || (x == 'c') || (x == 'd') || (x == 'e') || (x == 'f'))
#define IS_HEX_UPPER(x) 		((x == 'A') || (x == 'B') || (x == 'C') || (x == 'D') || (x == 'E') || (x == 'F'))
#define IS_HEX_DIGIT(x) 		(IS_ASCII_DIGIT(x) || IS_HEX_LOWER(x) || IS_HEX_UPPER(x))
#define CONVERT_ASCII_TO_HEX(x) ((IS_ASCII_DIGIT(x)) ? (x - ASCII_OFFSET) : ((IS_HEX_UPPER(x)) ? (x - 'A' + 10) : (x - 'a' + 10)))

#define PARSE_CMD_ANY_CHAR          (user_input != NULL)

#define PRINT_CLEAR_SCREEN()		PRINT_NOTS(RESET_DISPLAY_STRING)	/* Move cursor home, then clear the screen */
#define PRINT_BLANK_LINE()     		PRINT_NOTS(pString_CR)

/* Private types */
typedef enum user_term_option_enum
{
	user_term_opt_none,
	user_term_opt_tests,
	user_term_opt_display_connection,
	user_term_opt_display_devices,
	user_term_opt_udp_transfer,
	user_term_opt_erase_flash,
	user_term_opt_device_management,
#if !IS_COORD
	user_term_opt_fast_restore,
#else
	user_term_opt_rekeying,
#endif
	user_term_opt_reset,
	user_term_opt_count
} user_term_option_t;

typedef enum user_term_state_enum
{ 
	USER_TERM_ST_ENTRY,        		/*!< User Terminal state for terminal entry */
	USER_TERM_ST_CONNECTION,		/*!< User Terminal state for UDP connection setup */
	USER_TERM_ST_MAIN,       		/*!< User Terminal state for test main */
	USER_TERM_ST_TESTS,       		/*!< User Terminal state for UDP test selection */
	USER_TERM_ST_TEST_EXEC,			/*!< User Terminal state for UDP test execution */
	USER_TERM_ST_DIPLAY_CONNECTION,	/*!< User Terminal state for UDP connection setup */
	USER_TERM_ST_DIPLAY_DEVICES,	/*!< User Terminal state for connected device */
#if ENABLE_IMAGE_TRANSFER
	USER_TERM_ST_TRANSFER,			/*!< User Terminal state for UDP image transfer */
#endif
#if ENABLE_SFLASH_MANAGEMENT
	USER_TERM_ST_SFLASH,			/*!< User Terminal state for SFLASH erasure */
#endif
#if ENABLE_DEVICE_MANAGEMENT
	USER_TERM_ST_DEVICE_MANAGEMENT,	/*!< User Terminal state for device management */
#endif
#if !IS_COORD && ENABLE_FAST_RESTORE
	USER_TERM_ST_FAST_RESTORE,		/*!< User Terminal state for enabling/disabling Fast Restore  */
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING
	USER_TERM_ST_REKEYING,			/*!< User Terminal state for GMK update/Re-keying */
#endif
	USER_TERM_ST_RESET,				/*!< User Terminal state for system reset */
	USER_TERM_ST_CNT
} user_term_state_t;

typedef enum user_term_action_str
{
	USER_TERM_ACT_DISPMENU,	/* Menu display */
	USER_TERM_ACT_PROCSEL,		/* Process selection */
	USER_TERM_ACT_CNT
} user_term_action_t;

/* For re-keying and device management */
typedef enum user_term_request_step_enum
{
	USER_TERM_REQUEST,
	USER_TERM_EXECUTE,
} user_term_request_step_t;

/* For tests */
typedef enum user_term_test_step_enum
{
	USER_TERM_TESTSTEP_0,
	USER_TERM_TESTSTEP_1,
	USER_TERM_TESTSTEP_2,
	USER_TERM_TESTSTEP_3,
	USER_TERM_TESTSTEP_4,
} user_term_test_step_t;

/* For image transfer */
typedef enum user_term_transfer_step_enum
{
	USER_TERM_TRANSFERSTEP_0,		/* Select TX/RX | Memory check */
	USER_TERM_TRANSFERSTEP_0bis,	/* Display instruction */
	USER_TERM_TRANSFERSTEP_1,		/* TX | Select SFLASH slot */
	USER_TERM_TRANSFERSTEP_2, 		/* RX | Ask for confirmation */
	USER_TERM_TRANSFERSTEP_3,		/* Get confirmation */
	USER_TERM_TRANSFERSTEP_4,		/* Send/receive image info REQ */
	USER_TERM_TRANSFERSTEP_5,		/* Ask for confirmation */
	USER_TERM_TRANSFERSTEP_cnf,	/* Wait image info CNF */
	USER_TERM_TRANSFERSTEP_ind,	/* Wait image info IND */
	USER_TERM_TRANSFERSTEP_prep,	/* Prepare image data */
	USER_TERM_TRANSFERSTEP_req,	/* Send/receive image data */
	USER_TERM_TRANSFERSTEP_ack,	/* Sends acknowledge (recipient only) */
} user_term_transfer_step_t;

/* For multiple test */
typedef enum user_term_test_type_enum
{
	test_type_none,
	test_type_basic,
	test_type_multi,
	test_type_loopback,
	test_type_multicast,
} user_term_test_type_t;

/* Private structures and unions */
typedef struct user_term_fsm_str
{
	user_term_action_t 			action;
	user_term_state_t 			state;
	user_term_request_step_t	req_step;
	uint32_t 					user_events;
	uint32_t 					user_events_status;
	uint32_t     				dest_short_addr;       	/*!<  Selected destination for UDP packets */
	uint32_t 					start_time;  			/* in milliseconds */
	uint32_t 					timeout_value;			/* in milliseconds */
#if ENABLE_ICMP_KEEP_ALIVE
	bool						ka_was_suspended;
#endif /* ENABLE_ICMP_KEEP_ALIVE */
} user_term_fsm_t;

typedef struct user_if_test
{
	user_term_test_type_t	test_type;
	user_term_test_step_t 	test_step;
	user_term_test_step_t 	test_sub_step;
} user_term_test_t;

typedef struct user_term_displayed_event_str
{
	char* 		string;
	color_t 	color;
	uint32_t 	displayed;
} user_term_displayed_event_t;

typedef struct user_term_mult_test_str
{
	uint32_t packet_length;
	uint32_t expected_test_packet_n;
	uint32_t test_packet_n;
	uint8_t  packet_payload[USERIF_INPUT_MAX_SIZE];
} user_term_mult_test_t;

typedef struct user_term_transfer_str
{
	user_term_transfer_step_t	transfer_sub_step;

	/* SFLASH */
	slot_info_t	slot[IMAGE_SLOTS_NUM];

	/* Image information */
	uint32_t	image_type;
	uint32_t	image_validity;
	uint32_t	image_size;
	uint16_t	image_crc16;
} user_term_transfer_t;

/* Private variables ---------------------------------------------------------*/
static const char *pString_NoSuitableAnswerFound = "Invalid entry! Please retry...\n";
static const char *pString_Offline               = "You need to connect the device to a PAN to use this feature\n";
static const char *pString_CR                    = "\n";  /* Carriage Return string */
static const char *pString_TypeDestinationAddr   = "Type the destination short address, then ENTER (press only ENTER for default: %u)\n";
static const char *pString_DestinationAddrOOR    = "Destination short address out of range\n";
static const char *pString_TypeStringToSend      = "Type the string to send over UDP link (max size = %u), then ENTER.\n";
static const char *pString_TypedStringIs         = "Typed string is: ";
static const char *pString_UdpDataIndContent     = "G3UDP-DATA.Ind message content is: ";
static const char *pString_PlayTestOnceMore      = "Play test once more or press ESC. key then ENTER to go back to main menu.\n";
static const char *pString_ReturnToMainMenu      = "To return to the main menu, press ENTER\n";
#if IS_COORD
static const char *pString_DeviceNotFound        = "Error, device not found in table.\n";
static const char *pString_NoDeviceFound         = "No connected device found\n";
#endif

static user_term_fsm_t			user_term_fsm;
static user_term_test_t			user_term_test;
static user_term_mult_test_t	user_term_mult_test;
static user_term_transfer_t		user_term_transfer;

static user_term_displayed_event_t user_term_displayed_event[USEREVT_G3_NUMBER] = {
		/* string,                      color, 			displayed */
		{"HOSTIF-ERROR----------Ind", color_red,	    1},
		{"HOSTIF-TOOLS----------Cnf", color_green,		1},
		{"HOSTIF-RF-CONFIG-SET--Cnf", color_green,		1},
		{"G3LIB--SET------------Cnf", color_green,		0},
		{"G3LIB--SW-RESET-------Cnf", color_green,		1},
#if IS_COORD
		{"G3BOOT-SERVER-START---Cnf", color_green,		1},
		{"G3BOOT-SERVER-STOP----Cnf", color_green,		1},
		{"G3BOOT-SERVER-LEAVE---Ind", color_magenta,	1},
		{"G3BOOT-SERVER-KICK----Cnf", color_green,		1},
		{"G3BOOT-SERVER-JOIN----Ind", color_blue,		1},
		{"G3BOOT-SERVER-GETPSK--Ind", color_cyan,		1},
		{"G3BOOT-SERVER-SETPSK--Cnf", color_green,		1},
#else
		{"G3BOOT-DEVICE-START---Cnf", color_blue,		1},
		{"G3BOOT-DEVICE-LEAVE---Cnf", color_green,		1},
		{"G3BOOT-DEVICE-LEAVE---Ind", color_magenta,	1},
		{"G3BOOT-DEVICE-PANSORT-Ind", color_cyan,		1},
		{"G3BOOT-DEVICE-PANSORT-Cnf", color_green,		1},
#endif /* IS_COORD */
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
		{"G3BOOT-SERVER-REKEY---Cnf", color_green,		1},
#endif
		{"G3UDP--UDP-DATA-------Cnf", color_green,		1},
		{"G3UDP--UDP-DATA-------Ind", color_cyan,		1},
		{"G3UDP--CONNECTION-SET-Cnf", color_green,		1},
		{"G3ICMP-ICMP-ECHO------Cnf", color_green,		0},
		{"G3ICMP-ICMP-ECHO------Ind", color_cyan,		0},
		{"G3ICMP-ICMP-ECHO-REQ--Ind", color_cyan,		0},
};

/**
 * @}
 */

/** @addtogroup User_Terminal_Exported_Code
 * @{
 */

/* External Variables */
extern rf_type_t			rf_type;
extern BOOT_Bandplan_t		working_plc_band;

#if !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
extern lba_info_t 			lba_info;
#endif

extern userg3_common_t		userg3_common;
extern bool 				fast_restore_enabled;

extern osMessageQueueId_t	user_queueHandle;

extern osTimerId_t 			userTimeoutTimerHandle;

#if !IS_COORD && ENABLE_FAST_RESTORE
extern flash_data_t		usr_flash_data;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

/**
 * @}
 */

/** @addtogroup User_Terminal_Private_Code
 * @{
 */

/* Private functions */

/**
 * @brief Converts a user input terminal input string into an hex array.
 * @param dst_buff Indicates the destination buffer.
 * @param dst_buff_len Indicates the size of the destination buffer.
 * @param user_input Indicates the user terminal command to convert.
 * @retval The number of hex digits converted.
 */
static uint16_t user_term_convert_str_to_hex(uint8_t* dst_buff, uint16_t dst_buff_len, user_input_t * user_input)
{
	uint32_t len;

	PRINT(pString_TypedStringIs);
	PRINT_RAW((char*) user_input->payload, user_input->length);
	PRINT_BLANK_LINE();

	PRINT("Resulting hex string is: ");
	for (len = 0; len < dst_buff_len; len++)
	{
		if ((2*len+1) < user_input->length)
		{
			uint8_t hbyte = user_input->payload[2*len];
			uint8_t lbyte = user_input->payload[2*len+1];

			if (IS_HEX_DIGIT(hbyte) && IS_HEX_DIGIT(lbyte))
			{
				hbyte = CONVERT_ASCII_TO_HEX(hbyte);
				lbyte = CONVERT_ASCII_TO_HEX(lbyte);

				dst_buff[len] = ASSEMBLE_U8(hbyte, lbyte);
				PRINT_NOTS("%02X ", dst_buff[len]);
			}
			else
			{
				len = 0;
				PRINT_NOTS("%c%c -> INVALID.", hbyte, lbyte);
				break;
			}
		}
		else
		{
			break;
		}
	}
	PRINT_BLANK_LINE();

	return len;
}

/**
 * @brief Starts counting for the timeout of a transfer operation.
 * @param timeout The timeout value for the operation.
 * @retval Estimated elapsed time.
 */
static void user_term_set_timeout(uint32_t timeout)
{
	user_term_fsm.start_time 		= HAL_GetTick();
	user_term_fsm.timeout_value 	= timeout;
	if (!osTimerIsRunning(userTimeoutTimerHandle))
	{
		osTimerStart(userTimeoutTimerHandle, timeout);
	}
}

/**
 * @brief Stops counting for the timeout of a transfer operation.
 * @param None
 * @retval None
 */
static void user_term_remove_timeout(void)
{
	osTimerStop(userTimeoutTimerHandle);
}

/**
 * @brief Measures the time elapsed since the transfer timeout has been set.
 * @retval Estimated elapsed time.
 */
static bool user_term_timeout_reached(void)
{
	/* Since these are uint32_t, the difference is computed correctly even in case of roll-over */
	return ((HAL_GetTick() - user_term_fsm.start_time) >= user_term_fsm.timeout_value);
}

/**
 * @brief Sets the User Terminal FSM to a specific state, making it update afterwards.
 * @param new_state State on which to set to.
 * @retval None
 */
static void user_term_set_state(user_term_state_t new_state)
{
	user_term_fsm.state    = new_state;
	user_term_fsm.req_step = USER_TERM_REQUEST;
	user_term_fsm.action   = USER_TERM_ACT_DISPMENU;
	RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
}

/**
 * @brief Resets the User Terminal FSM to a specific state.
 * @param new_state State to set.
 * @retval None
 */
static void user_term_reset_to_state(user_term_state_t new_state)
{
	user_if_init_rx_fifo();  /* Flush UART Rx FIFO */

	user_term_test.test_step         	 = USER_TERM_TESTSTEP_0;
	user_term_test.test_sub_step     	 = USER_TERM_TESTSTEP_0;

#if ENABLE_IMAGE_TRANSFER
	if (user_term_transfer.transfer_sub_step == USER_TERM_TRANSFERSTEP_2)
	{
		UserImgTransfer_Stop();
	}
#endif

	user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_0;

	user_term_fsm.user_events            = 0U;
	user_term_fsm.user_events_status     = 0U;

	user_term_displayed_event[USEREVT_G3_UDP_DATA_CNF].displayed = 1;
	user_term_displayed_event[USEREVT_G3_UDP_DATA_IND].displayed = 1;

	UserG3_AbortUdpDataLoopback();

	if (osTimerIsRunning(userTimeoutTimerHandle))
	{
		user_term_remove_timeout();
	}

#if ENABLE_ICMP_KEEP_ALIVE
	if (user_term_fsm.ka_was_suspended)
	{
		user_term_fsm.ka_was_suspended = false;
		g3_app_ka_start();
	}
#endif /* ENABLE_ICMP_KEEP_ALIVE */
	user_term_set_state(new_state);

	memset(&user_term_transfer, 0, sizeof(user_term_transfer));
}

/**
 * @brief Print utility function displaying platform information.
 * @param None
 * @retval None
 */
static void user_term_print_platform_info(void)
{
	PRINT_BLANK_LINE();
	PRINT("Platform info:\n");

	uint8_t bandplan   = GET_PE_VERSION_BAND(userg3_common.platform_info.peVersion);

	uint8_t dev_type   = GET_PE_VERSION_PROFILE(userg3_common.platform_info.peVersion);

	uint8_t pe_ver_maj = GET_PE_VERSION_MAJOR(userg3_common.platform_info.peVersion);
	uint8_t pe_ver_min = GET_PE_VERSION_MINOR(userg3_common.platform_info.peVersion);
	uint8_t pe_ver_sub = GET_PE_VERSION_PATCH(userg3_common.platform_info.peVersion);

	uint8_t rte_ver_maj = GET_RTE_VERSION_MAJOR(userg3_common.platform_info.rteVersion);
	uint8_t rte_ver_min = GET_RTE_VERSION_MINOR(userg3_common.platform_info.rteVersion);
	uint8_t rte_ver_sub = GET_RTE_VERSION_PATCH(userg3_common.platform_info.rteVersion);

	PRINT("- Platform: %s\n",  hi_boot_sap_translate_platform(userg3_common.platform_info.platform));
	PRINT("- FW version: PE v%u.%u.%u, RTE v%u.%urc%u, G3LIB v%08X\n",	pe_ver_maj, pe_ver_min, pe_ver_sub, rte_ver_maj, rte_ver_min, rte_ver_sub, userg3_common.platform_info.libVersion);

	PRINT("- Role: %s\n", 				hi_boot_sap_translate_dev_type(dev_type));
	PRINT("- Bandplan: %s\n", 			hi_boot_sap_translate_bandplan(bandplan));
	PRINT("- Mode: %s\n",     		    hi_boot_sap_translate_mode(userg3_common.platform_info.PlcMode));

	PRINT("- RF connected: ");
	if (userg3_common.platform_info.RfConf != 0)
	{
		PRINT_COLOR_NOTS("YES\n", color_green);
	}
	else
	{
		PRINT_COLOR_NOTS("NO\n", color_red);
	}

	PRINT("- Platform SFlash ID: ");
	if (userg3_common.platform_info.SFlashId == SPI_FLASH_ST8500_ID)
	{
		PRINT_COLOR_NOTS("0x%X\n", color_green, userg3_common.platform_info.SFlashId);
	}
	else
	{
		PRINT_COLOR_NOTS("0x%X\n", color_red, userg3_common.platform_info.SFlashId);
	}

	PRINT("- USART configuration: %02X\n", userg3_common.platform_info.UsartNum);
	PRINT("- LED configuration: %02X\n",   userg3_common.platform_info.LedConf);

	PRINT("- MAC address: ");
	for (int32_t i = sizeof(userg3_common.platform_info.eui64) - 1; i >= 0; i--)
	{
		PRINT_NOTS("%02X", TAKE_BYTE(userg3_common.platform_info.eui64[i / sizeof(uint32_t)], (i % sizeof(uint32_t))));
	}
	PRINT_BLANK_LINE();

	PRINT_BLANK_LINE();
}

/**
 * @brief Print utility function displaying connection information.
 * @param None
 * @retval None
 */
static void user_term_print_connection_info(void)
{
	PRINT_BLANK_LINE();
	PRINT("Connection info:\n");

	PRINT("- PAN ID: %04X\n",    userg3_common.pan_id);
#if IS_COORD
	PRINT("- Short address: %u\n", COORD_ADDRESS);
#else
	PRINT("- Short address: %u\n", userg3_common.device_addr);
#if ENABLE_BOOT_CLIENT_ON_HOST
	PRINT("- LBA address: %u\n", lba_info.short_address);
	PRINT("- LBA media type: %s\n", g3_app_pansort_translate_media_type(lba_info.media_type));
#endif
#endif
	PRINT_BLANK_LINE();
}

/**
 * @brief Printf utility function centralizing "waiting for message" printing.
 * @param msg_index Index of awaited message.
 * @retval None
 */
static void user_term_print_waiting_for_msg(uint32_t msg_index)
{
	PRINT("Waiting for %s message...\n", user_term_displayed_event[msg_index].string);
}

/**
 * @brief This function parses the events upcoming from user G3 common interface.
 * @param None
 * @retval None
 */
static void user_term_parse_user_events(void)
{
	uint32_t event_pos;
	uint32_t event_mask;

	user_term_fsm.user_events 		 = userg3_common.user_events; 			/* Locally store instant event bitmap */
	user_term_fsm.user_events_status  = userg3_common.user_events_status;	/* Locally store event result bitmap */

	userg3_common.user_events = 0U; /* Read-clear user events */

	/* Manage display of User Task events */
	for (event_pos = 0U; event_pos < USEREVT_G3_NUMBER; event_pos++)
	{
		event_mask = (1UL << event_pos);
		if (((user_term_fsm.user_events & event_mask)       == event_mask) &&
			( user_term_displayed_event[event_pos].displayed != 0U  ) )
		{
			color_t color;

			if ((user_term_fsm.user_events_status & event_mask)  == event_mask)
			{
				color = user_term_displayed_event[event_pos].color;
			}
			else
			{
				color = color_red;
			}

			PRINT_COLOR("%s received\n", color, user_term_displayed_event[event_pos].string);

			switch(event_pos)
			{
#if IS_COORD
			case USEREVT_G3_DBGTOOLS_CNF:
				user_term_print_platform_info();
				break;
			case USEREVT_G3_BOOT_SRV_START_CNF:
				user_term_print_waiting_for_msg(USEREVT_G3_BOOT_SRV_JOIN_IND);
				break;
			case USEREVT_G3_BOOT_SRV_STOP_CNF:
				if (!userg3_common.online)
				{
					user_term_print_waiting_for_msg(USEREVT_G3_BOOT_SRV_START_CNF);
				}
				break;
#else
			case USEREVT_G3_DBGTOOLS_CNF:
				user_term_print_platform_info();

				if ((!userg3_common.online) && (userg3_common.platform_info.RfConf == 0))
				{
					user_term_print_waiting_for_msg(USEREVT_G3_BOOT_DEV_START_CNF);
				}
				break;
			case USEREVT_G3_RFCONFIGSET_CNF:
				if ((!userg3_common.online) && (userg3_common.platform_info.RfConf != 0))
				{
					user_term_print_waiting_for_msg(USEREVT_G3_BOOT_DEV_START_CNF);
				}
				break;
#endif /* IS_COORD */
			default:
				break;
			}
		}
	}
}

/**
 * @brief This function parses specific commands (like ESC) coming from user terminal interface.
 * @param None
 * @retval None
 */
static void user_term_parse_specific_command(void)
{
	if (user_if_search_char(ESC_ASCII_CODE))
	{
		PRINT_NOTS("[["); /* Undo the ESC of the local echo, on the terminal */
		user_term_reset_to_state(USER_TERM_ST_MAIN);
	}
}

/**
 * @brief This functions gets the number entered by the user on the terminal. It should be btw 1 and 99.
 * @param cmd Command typed by the user.
 * @retval Number typed by the user, in decimal format.
 */
static uint32_t user_term_read_char(user_input_t * cmd)
{
	uint32_t output = 0U;
	/* Check if the number is in the correct range. First make the check if it is on 1 digit */
	if ((cmd != NULL) && (cmd->payload[1U] == '\r'))
	{
		/* The number got is in ASCII format. check it is well btw 1 and 9 */
		if (IS_ASCII_DIGIT(cmd->payload[0U]))
		{
			/* This means the number is between 0 and 9 */
			/* RcvCommand->Payload[i] comes in ASCII format. If we subtract ASCII_OFFSET (=0x30) it is in decimal */
			output = (cmd->payload[0U] - ASCII_OFFSET);
		}
		else
		{
			PRINT(pString_NoSuitableAnswerFound);
		}
	}
	else if ((cmd != NULL) && (cmd->payload[2U] == '\r'))
	{
		/* The user has entered 2 digits. Check they are both in the correct range */
		if (IS_ASCII_DIGIT(cmd->payload[0U]) &&
				IS_ASCII_DIGIT(cmd->payload[1U]) )
		{
			/* This means the number is between 01 and 99 */
			output = 10U * (cmd->payload[0U] - ASCII_OFFSET) + (cmd->payload[1U] - ASCII_OFFSET);
		}
		else
		{
			PRINT(pString_NoSuitableAnswerFound);
		}
	}
	else if ((cmd != NULL) && ((cmd->payload[1U] != '\r') || (cmd->payload[2U] != '\r')))
	{
		PRINT(pString_NoSuitableAnswerFound);
	}

	return (output);
}

/**
 * @brief Assigns a value given by the user, converting the string to an unsigned integer.
 * @param user_input Pointer to user input structure.
 * @param default_value Default value to use if the input is empty.
 * @param value_label String that contains the name of the value to assign.
 * @param is_hex Boolean value that indicates if the value is to be displayed as hexadecimal.
 * @retval Value to assign
 */
static uint32_t user_term_assign_user_value(user_input_t *user_input, uint32_t default_value, const char* value_label, bool is_hex)
{
	uint32_t value_to_assign;
	assert(user_input != NULL);

	/* must be at least 2 char (a digit and ENTER) */
	if (strlen((char*) user_input->payload) > 1)
	{
		value_to_assign = (uint32_t) strtol((char*) user_input->payload, (char**) NULL, 10 + (6*is_hex));

		char string[24] = "Selected %s is: ";

		if (is_hex)
		{
			strcat(string, "%X.\n");
		}
		else
		{
			strcat(string, "%d.\n");
		}

		PRINT(string, value_label, value_to_assign);
	}
	else
	{
		value_to_assign = default_value;

		char string[36] = "No input, using default %s: ";

		if (is_hex)
		{
			strcat(string, "%X.\n");
		}
		else
		{
			strcat(string, "%d.\n");
		}

		PRINT(string, value_label, value_to_assign);
	}

	return value_to_assign;
}

#if IS_COORD
/**
 * @brief Prints the list of connected devices, for the coordinator.
 * @param None
 * @retval The number of connected devices
 */
static uint16_t user_term_print_device_list()
{
	uint16_t device_count = 0;
	boot_device_t *boot_device;

	PRINT("Connected devices:\n");
	do
	{
		boot_device = g3_app_boot_get_device_by_index(device_count, boot_state_connected);

		if (boot_device != NULL)
		{
			ALLOC_DYNAMIC_HEX_STRING(ext_addr_str, boot_device->ext_addr, MAC_ADDR64_SIZE);
			PRINT("\tDevice %s - short address %u\n", ext_addr_str, boot_device->short_addr);
			FREE_DYNAMIC_HEX_STRING(ext_addr_str);

			device_count++;
		}
	}
	while (boot_device != NULL);
	PRINT_BLANK_LINE();

	return device_count;
}
#endif /* IS_COORD */

/**
 * @brief This function implements the single UDP-DATA request test execution from originator side.
 * @param None
 * @retval None
 */
static void user_term_udp_test_basic_tx(void)
{
	user_input_t * user_input = NULL;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		PRINT(pString_TypeStringToSend, (USERIF_INPUT_MAX_SIZE - 1U));
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			PRINT(pString_TypedStringIs);
			PRINT_RAW((char*) user_input->payload, user_input->length);
			PRINT_BLANK_LINE();

			UserG3_SendUdpDataToShortAddress(TEST_CONN_ID, user_term_fsm.dest_short_addr, user_input->payload, user_input->length);

			user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_CNF);  /* Now waiting for G3UDP-DATA.Cnf message... */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_2;
		}
		break;
	case USER_TERM_TESTSTEP_2:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_CNF))
		{
			user_term_remove_timeout();

			PRINT(pString_PlayTestOnceMore);

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the single UDP-DATA request test execution from recipient side
 * @retval None
 */
static void user_term_udp_test_basic_rx(void)
{
	udp_packet_t udp_packet_rx;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_IND);  /* Now waiting for G3UDP-DATA.Ind message... */
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_IND)) /* Wait without timeout */
		{
			udp_packet_rx = UserG3_GetUdpData(TEST_CONN_ID);	/* Get received UDP data */

			PRINT(pString_UdpDataIndContent);
			PRINT_RAW(udp_packet_rx.payload, udp_packet_rx.length);
			PRINT_BLANK_LINE();
			PRINT(pString_PlayTestOnceMore);

			UserG3_DiscardUdpData(TEST_CONN_ID);	/* Not needed anymore */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the multiple basic UDP-DATA request test execution from originator side.
 * @param None
 * @retval None
 */
static void user_term_udp_test_multi_basic_tx(void)
{
	user_input_t * user_input = NULL;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		PRINT("Enter the number of packets to send (between 1 and 99) then ENTER.\n");
		user_term_mult_test.test_packet_n          = 0U;
		user_term_mult_test.expected_test_packet_n = 0U;
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			user_term_mult_test.expected_test_packet_n = user_term_read_char(user_input);

			if (	(user_term_mult_test.expected_test_packet_n > 0U  ) &&
					(user_term_mult_test.expected_test_packet_n < 100U) )
			{
				user_term_test.test_sub_step = USER_TERM_TESTSTEP_2;
			}
			else
			{
				user_term_test.test_sub_step = USER_TERM_TESTSTEP_0; /* Re-acquire input */
			}
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	case USER_TERM_TESTSTEP_2:
		PRINT_BLANK_LINE();
		PRINT(pString_TypeStringToSend, (USERIF_INPUT_MAX_SIZE - 1U));
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_3;
		break;
	case USER_TERM_TESTSTEP_3:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			/* Store the command received from the user for the next UDP transfers */
			if (user_input->length > USERIF_INPUT_MAX_SIZE)
			{
				user_input->length = USERIF_INPUT_MAX_SIZE;
			}

			user_term_mult_test.packet_length = user_input->length;
			memcpy(user_term_mult_test.packet_payload, user_input->payload, user_term_mult_test.packet_length);

			PRINT(pString_TypedStringIs);
			PRINT_RAW((char*) user_term_mult_test.packet_payload, user_term_mult_test.packet_length);
			PRINT_BLANK_LINE();

			/* Send the 1st UDP with the nb of tests (1 byte) as data so that the receiver knows when to finish the test */
			user_input->payload[0] = (uint8_t)user_term_mult_test.expected_test_packet_n;
			UserG3_SendUdpDataToShortAddress(TEST_CONN_ID, user_term_fsm.dest_short_addr, user_input->payload, 1);

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_4;
		}
		break;
	case USER_TERM_TESTSTEP_4:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_CNF))
		{
			user_term_mult_test.test_packet_n++;

			if (user_term_mult_test.test_packet_n < user_term_mult_test.expected_test_packet_n)
			{
				UserG3_SendUdpDataToShortAddress(TEST_CONN_ID, user_term_fsm.dest_short_addr, user_term_mult_test.packet_payload, user_term_mult_test.packet_length);
			}
			else
			{
				user_term_remove_timeout(); /* Stop timeout timer */

				PRINT_BLANK_LINE();
				PRINT("Test completed, %d UDP packets have been sent with the specified data.\n", user_term_mult_test.expected_test_packet_n);
				PRINT(pString_PlayTestOnceMore);

				user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
		}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the multiple basic UDP-DATA request test execution from recipient side
 * @retval None
 */
static void user_term_udp_test_multi_rx(void)
{
	udp_packet_t udp_packet_rx;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_IND);  /* Now waiting for G3UDP-DATA.Ind message... */
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		user_term_mult_test.test_packet_n          = 0U;
		user_term_mult_test.expected_test_packet_n = 0U;
		break;
	case USER_TERM_TESTSTEP_1:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_IND)) /* Wait with timeout */
		{
			user_term_mult_test.test_packet_n++;

			udp_packet_rx = UserG3_GetUdpData(TEST_CONN_ID);	/* Get received UDP data */

			if (user_term_mult_test.test_packet_n == user_term_mult_test.expected_test_packet_n)
			{
				user_term_remove_timeout(); /* Stop timeout timer */

				PRINT(pString_UdpDataIndContent);
				PRINT_RAW(udp_packet_rx.payload, udp_packet_rx.length);
				PRINT_BLANK_LINE();
				PRINT("Test completed, G3UDP-DATA.Ind received %u times.\n", user_term_mult_test.expected_test_packet_n);
				PRINT(pString_PlayTestOnceMore);

				user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
			else
			{
				user_term_set_timeout(INDICATION_TIMEOUT_DELAY); /* Refresh reception timeout time */

				if (user_term_mult_test.test_packet_n == 1U)
				{
					/* The 1st data received is the expected iteration number */
					user_term_mult_test.expected_test_packet_n = ((uint8_t*) udp_packet_rx.payload)[0];
				}
			}

			UserG3_DiscardUdpData(TEST_CONN_ID);	/* Not needed anymore */
		}
		else if (	(user_term_timeout_reached()			) &&
					(user_term_mult_test.test_packet_n > 1U) )
		{
			/* If timeout is detected AND a 1st data + the data for Required_Nb_Tests_Rcv_Side has already being received THEN quit */
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else {}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the loopback UDP-DATA REQUEST test execution from originator node side.
 * @param None
 * @retval None
 */
static void user_term_udp_test_loop_tx(void)
{
	user_input_t * user_input = NULL;

	udp_packet_t udp_packet_rx;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		PRINT(pString_TypeStringToSend, (USERIF_INPUT_MAX_SIZE - 1U));
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			PRINT(pString_TypedStringIs);
			PRINT_RAW((char*) user_input->payload, user_input->length);
			PRINT_BLANK_LINE();

			UserG3_SendUdpDataToShortAddress(TEST_CONN_ID, user_term_fsm.dest_short_addr, user_input->payload, user_input->length);
			UserG3_StartTimeCounter();  /* Start timer in order to evaluate data transfer loopback duration */
			user_term_set_timeout(LOOPBACK_TIMEOUT_DELAY);

			user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_IND);  /* Now waiting for G3UDP-DATA.Ind message... */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_2;
		}
		break;
	case USER_TERM_TESTSTEP_2:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_IND)) /* Wait with timeout */
		{
			user_term_remove_timeout();
			UserG3_StopTimecounter();

			udp_packet_rx = UserG3_GetUdpData(TEST_CONN_ID);	/* Get received UDP data */

			PRINT("Echo content: ");
			PRINT_RAW(udp_packet_rx.payload, udp_packet_rx.length);
			PRINT_BLANK_LINE();
			PRINT("%u characters echoed in %d ms.\n", (udp_packet_rx.length - 1U), UserG3_GetTimecounter());
			PRINT(pString_PlayTestOnceMore);

			UserG3_DiscardUdpData(TEST_CONN_ID);	/* Not needed anymore */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		else if (user_term_timeout_reached())
		{
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else {}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the loopback UDP DATA REQUEST test execution from mirror node side.
 * @param None
 * @retval None
 */
static void user_term_udp_test_loop_rx(void)
{
	udp_packet_t udp_packet_rx;

	uint8_t answer_buffer[USERIF_INPUT_MAX_SIZE];

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		UserG3_RequestUdpDataLoopback(); /* Now will get destination from the next packet */
		user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_IND);  /* Now waiting for G3UDP-DATA.Ind message... */
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_IND)) /* Wait without timeout */
		{
			udp_packet_rx = UserG3_GetUdpData(TEST_CONN_ID);	/* Get received UDP data */

			PRINT(pString_UdpDataIndContent);
			PRINT_RAW(udp_packet_rx.payload, udp_packet_rx.length);
			PRINT_BLANK_LINE();

			/* The memory pool with the received message will be discarded at the end of the User Task cycle, so we need another copy of the message */
			if ((udp_packet_rx.length > 0) && (udp_packet_rx.length <= sizeof(answer_buffer)))
			{
				memcpy(answer_buffer, udp_packet_rx.payload, udp_packet_rx.length);

				UserG3_SendUdpDataToShortAddress(TEST_CONN_ID, user_term_fsm.dest_short_addr, answer_buffer, udp_packet_rx.length);
			}
			else
			{
				PRINT("Error, invalid packet size: %u", udp_packet_rx.length);
			}

			PRINT(pString_PlayTestOnceMore);

			UserG3_DiscardUdpData(TEST_CONN_ID);	/* Not needed anymore */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_2;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	case USER_TERM_TESTSTEP_2:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_CNF))
		{
			user_term_remove_timeout();

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	default:
		break;
	}
}

/**
 * @brief This function implements the single multicast G3UDP-DATA.Request test execution from originator side.
 * @param None
 * @retval None
 */
static void user_term_udp_test_multicast_tx(void)
{
	user_input_t * user_input = NULL;

	switch (user_term_test.test_sub_step)
	{
	case USER_TERM_TESTSTEP_0:
		PRINT_BLANK_LINE();
		PRINT(pString_TypeStringToSend, (USERIF_INPUT_MAX_SIZE - 1U));
		user_term_test.test_sub_step = USER_TERM_TESTSTEP_1;
		break;
	case USER_TERM_TESTSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			PRINT(pString_TypedStringIs);
			PRINT_RAW((char*) user_input->payload, user_input->length);
			PRINT_BLANK_LINE();

			ip6_addr_t dst_ip_addr;

			uint8_t broadcast_arr[IP_IPV6_ADDR128_UINT8_LEN] = IPV6_MULTICAST_ADDR;
			memcpy(dst_ip_addr.u8, broadcast_arr, IP_IPV6_ADDR128_UINT8_LEN);

			UserG3_SendUdpData(TEST_CONN_ID, dst_ip_addr, user_input->payload, user_input->length);

			user_term_print_waiting_for_msg(USEREVT_G3_UDP_DATA_CNF);  /* Now waiting for G3UDP-DATA.Cnf message... */

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_2;
		}
		break;
	case USER_TERM_TESTSTEP_2:
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_DATA_CNF))
		{
			user_term_remove_timeout();

			PRINT(pString_PlayTestOnceMore);

			user_term_test.test_sub_step = USER_TERM_TESTSTEP_0;
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
		break;
	default:
		break;
	}
}

#if ENABLE_IMAGE_TRANSFER
#if IS_COORD
/**
 * @brief This function implements the UDP image transfer execution from originator side.
 * @param None
 * @retval None
 */
static void user_term_udp_transfer_tx(void)
{
	uint8_t selected_slot;
	user_input_t * user_input = NULL;

	switch (user_term_transfer.transfer_sub_step)
	{
	case USER_TERM_TRANSFERSTEP_0:
		PRINT_BLANK_LINE();
		checkMemoryContent(user_term_transfer.slot);
		user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_0bis;
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		break;
	case USER_TERM_TRANSFERSTEP_0bis:
		PRINT("Select the slot to transfer (1 - %u).\n", IMAGE_SLOTS_NUM);
		user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_1;
		break;
	case USER_TERM_TRANSFERSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			selected_slot = (user_term_read_char(user_input) - 1);

			bool selection_valid = false;

			if (selected_slot <  IMAGE_SLOTS_NUM)
			{
				if (!user_term_transfer.slot[selected_slot].free)
				{
					user_term_transfer.image_type     = user_term_transfer.slot[selected_slot].type;
					user_term_transfer.image_validity = user_term_transfer.slot[selected_slot].validity;
					user_term_transfer.image_size     = user_term_transfer.slot[selected_slot].size;

					if (	IMG_TYPE_IS_VALID(    user_term_transfer.image_type    ) &&
							IMG_VALIDITY_IS_VALID(user_term_transfer.image_validity) &&
							IMG_SIZE_IS_VALID(    user_term_transfer.image_size    ) )
					{
						selection_valid = true;

						user_term_transfer.image_crc16 = calculateImageCRC(SFLASH_SLOT(selected_slot), user_term_transfer.image_size);
					}
				}
			}

			if (selection_valid)
			{
				PRINT_BLANK_LINE();
				PRINT("Selected slot %u, starting image transfer...\n", selected_slot + 1);

				/* Silences event display */
				user_term_displayed_event[USEREVT_G3_UDP_DATA_CNF].displayed = 0;
				user_term_displayed_event[USEREVT_G3_UDP_DATA_IND].displayed = 0;

				user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_2;
				UserImgTransfer_StartSend(selected_slot, user_term_fsm.dest_short_addr);
			}
			else
			{
				user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_0bis;
				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
		}
		break;
	case USER_TERM_TRANSFERSTEP_2:
		if (UserImgTransfer_IsComplete())
		{
			user_image_transfer_error_code_t error_code = UserImgTransfer_GetError();

			if (error_code == uit_no_error)
			{
				PRINT_COLOR("Image transmitted successfully", color_green);
			}
			else
			{
				PRINT_COLOR("Error during image transmission", color_red);
			}

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		break;
	default:
		Error_Handler();
		break;
	}
}

#else /* IS_COORD */

/**
 * @brief This function implements the UDP image transfer execution from recipient side.
 * @param None
 * @retval None
 */
static void user_term_udp_transfer_rx(void)
{
	user_input_t * user_input = NULL;

	uint8_t selected_slot;

	switch (user_term_transfer.transfer_sub_step)
	{
	case USER_TERM_TRANSFERSTEP_0:
		PRINT_BLANK_LINE();
		checkMemoryContent(user_term_transfer.slot);
		user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_0bis;
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		break;
	case USER_TERM_TRANSFERSTEP_0bis:
		PRINT("Select the slot for the incoming image (1 - %u):\n", IMAGE_SLOTS_NUM);
		user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_1;
		break;
	case USER_TERM_TRANSFERSTEP_1:
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			selected_slot = user_term_read_char(user_input) - 1;

			if (selected_slot < IMAGE_SLOTS_NUM)
			{
				PRINT_BLANK_LINE();
				if (!user_term_transfer.slot[selected_slot].free)
				{
					PRINT("Note: slot not empty. The transfer will overwrite the slot\n");
				}
				PRINT("Selected slot %u, waiting for image transfer...\n", selected_slot + 1);

				/* Silences event display */
				user_term_displayed_event[USEREVT_G3_UDP_DATA_CNF].displayed = 0;
				user_term_displayed_event[USEREVT_G3_UDP_DATA_IND].displayed = 0;

				/* Clear previous events */
				user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_2;

				UserImgTransfer_StartReceive(selected_slot);
			}
			else
			{
				user_term_transfer.transfer_sub_step = USER_TERM_TRANSFERSTEP_0bis;
				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
		}
		break;
	case USER_TERM_TRANSFERSTEP_2:
		if (UserImgTransfer_IsComplete())
		{
			user_image_transfer_error_code_t error_code = UserImgTransfer_GetError();

			if (error_code == uit_no_error)
			{
				PRINT_COLOR("Image received successfully", color_green);
			}
			else
			{
				PRINT_COLOR("Error during image reception", color_red);
			}

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
#if !IS_COORD && ENABLE_LAST_GASP
		else if (g3_app_last_gasp_is_active())
		{
			UserImgTransfer_Stop();
		}
#endif /* !IS_COORD && ENABLE_LAST_GASP */
		break;
	default:
		Error_Handler();
		break;
	}
}

#endif /* IS_COORD */
#endif /* ENABLE_IMAGE_TRANSFER */

/**
 * @brief User Terminal implementation for the initial start-up.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_terminal_startup(user_term_action_t action)
{
	if (action == USER_TERM_ACT_DISPMENU)
	{
#if defined(DEBUG)
		PRINT("DEBUG BUILD\n\n");
#endif
		PRINT("Kit name: %s\n", EVALKIT_NAME);
#if defined(STM32G070xx)
		PRINT("STM32G070 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#elif defined(STM32G474xx)
		PRINT("STM32G474 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#elif defined(STM32L476xx)
		PRINT("STM32L476 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#else
		PRINT("STM32 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#endif
		PRINT("Bandplan: %s\n", hi_boot_sap_translate_bandplan(working_plc_band));
		PRINT("Role: %s, RF: %s\n", hi_boot_sap_translate_dev_type(PLC_DEVICE_TYPE), RF_TYPE_STRING(rf_type));

		uint32_t sflash_id;

		if (SFLASH_GET_ID(&sflash_id))
		{
			PRINT("STM32 SFlash ID: ");

			if (sflash_id == SPI_FLASH_STM32_ID)
			{
				PRINT_COLOR_NOTS("0x%X\n", color_green, sflash_id);
			}
			else if (sflash_id == SPI_FLASH_STM32_ALT_ID)
			{
				PRINT_COLOR_NOTS("0x%X\n", color_yellow, sflash_id);
			}
			else
			{
				PRINT_COLOR_NOTS("0x%X\n", color_red, sflash_id);
			}
		}
		else
		{
			PRINT_COLOR("Could not read STM32 SFlash ID\n", color_red);
		}

		PRINT_BLANK_LINE();
		user_term_print_waiting_for_msg(USEREVT_G3_G3LIBSWRESETCNF);
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
#if IS_COORD
		if (USER_EVENT_RISEN(USEREVT_G3_BOOT_SRV_JOIN_IND))
#else
		if (USER_EVENT_RISEN(USEREVT_G3_BOOT_DEV_START_CNF))
#endif /* IS_COORD */
		{
#if 0
			user_term_set_state(USER_TERM_ST_PLATFORMINFO);
#else
			user_term_print_connection_info();
			user_term_set_state(USER_TERM_ST_CONNECTION);
#endif
		}
	}
}

/**
 * @brief User Terminal implementation for the UDP connections setup.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_set_connection(user_term_action_t action)
{
	if (action == USER_TERM_ACT_DISPMENU)
	{
		/* Prepare for UDP data transfer by launching an UDP Connection set */
		UserG3_StartUdpConnSetup();

		/* Wait for all connections to be set */
		user_term_print_waiting_for_msg(USEREVT_G3_UDP_CONN_SET_CNF);
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		if (USER_EVENT_RISEN(USEREVT_G3_UDP_CONN_SET_CNF))
		{
			/* Goes to the main menu when all connection have been set */
			if (userg3_common.connection_number == userg3_common.connection_total_number)
			{
				user_term_set_state(USER_TERM_ST_MAIN);
			}
		}
	}
}

/**
 * @brief User Terminal implementation for the main menu.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_main_menu(user_term_action_t action)
{
	user_input_t *user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		PRINT("< Main menu >\n\n");

		PRINT("%u) UDP tests\n",               		user_term_opt_tests);
		PRINT("%u) UDP connections\n",          	user_term_opt_display_connection);
		PRINT("%u) Connected devices\n",        	user_term_opt_display_devices);
#if ENABLE_IMAGE_TRANSFER
		PRINT("%u) UDP image transfer\n",			user_term_opt_udp_transfer);
#endif
#if ENABLE_SFLASH_MANAGEMENT
		PRINT("%u) Manage SFLASH memory\n",			user_term_opt_erase_flash);
#endif
#if ENABLE_DEVICE_MANAGEMENT
#if IS_COORD
		PRINT("%u) Kick connected device\n",    	user_term_opt_device_management);
#else
		PRINT("%u) Disconnect/reconnect\n",			user_term_opt_device_management);
#endif
#endif
#if !IS_COORD && ENABLE_FAST_RESTORE
		PRINT("%u) Enable/disable Fast Restore\n",	user_term_opt_fast_restore);
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING
		PRINT("%u) Update GMK\n",            		user_term_opt_rekeying);
#endif
		PRINT("%u) Reset\n",						user_term_opt_reset);
		/* Append other tests to the related menu here... */

		PRINT_BLANK_LINE();
		PRINT("Press ESCAPE key then ENTER at any time to go back to this menu\n");
		PRINT("Digit the number of an option and then ENTER to select it\n");
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		user_input = user_if_get_input();

		if (PARSE_CMD_ANY_CHAR)
		{
			uint8_t option = GET_CMD_DIGIT();

			switch(option)
			{
			case user_term_opt_tests:
				user_term_set_state(USER_TERM_ST_TESTS);
				break;
			case user_term_opt_display_connection:
				user_term_set_state(USER_TERM_ST_DIPLAY_CONNECTION);
				break;
			case user_term_opt_display_devices:
				user_term_set_state(USER_TERM_ST_DIPLAY_DEVICES);
				break;
#if ENABLE_IMAGE_TRANSFER
			case user_term_opt_udp_transfer:
				user_term_set_state(USER_TERM_ST_TRANSFER);
				break;
#endif
#if ENABLE_SFLASH_MANAGEMENT
			case user_term_opt_erase_flash:
				user_term_set_state(USER_TERM_ST_SFLASH);
				break;
#endif
#if ENABLE_DEVICE_MANAGEMENT
			case user_term_opt_device_management:
				user_term_set_state(USER_TERM_ST_DEVICE_MANAGEMENT);
				break;
#endif
#if !IS_COORD && ENABLE_FAST_RESTORE
			case user_term_opt_fast_restore:
				user_term_set_state(USER_TERM_ST_FAST_RESTORE);
				break;
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING
			case user_term_opt_rekeying:
				user_term_set_state(USER_TERM_ST_REKEYING);
				break;
#endif
			case user_term_opt_reset:
				user_term_set_state(USER_TERM_ST_RESET);
				break;
			default:
				PRINT(pString_NoSuitableAnswerFound);
				break;
			}
		}
	}
}

/**
 * @brief User Terminal implementation for selection of the UDP test (single/multi/loopback).
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_test(user_term_action_t action)
{
	if (userg3_common.online)
	{
		user_input_t * user_input = NULL;

		if (action == USER_TERM_ACT_DISPMENU)
		{
			PRINT_BLANK_LINE();
			PRINT("<< UDP test menu >>\n\n");

			PRINT("1) Single basic UDP test\n");
			PRINT("2) Multiple basic UDP tests\n");
			PRINT("3) Loopback UDP test\n");
			PRINT("4) Multicast UDP test\n");

			user_term_test.test_type = test_type_none;
		}
		else /* if (action == USER_TERM_ACT_PROCSEL) */
		{
			user_input = user_if_get_input();

			/* Parse received command: */
			if (     PARSE_CMD_CHAR('1'))
			{
				user_term_test.test_type = test_type_basic;
			}
			else if (PARSE_CMD_CHAR('2'))
			{
				user_term_test.test_type = test_type_multi;
			}
			else if (PARSE_CMD_CHAR('3'))
			{
				user_term_test.test_type = test_type_loopback;
			}
			else if (PARSE_CMD_CHAR('4'))
			{
				user_term_test.test_type = test_type_multicast;
			}
			else if (PARSE_CMD_ANY_CHAR)
			{
				PRINT(pString_NoSuitableAnswerFound);
			}
			else {}

			if (user_term_test.test_type != test_type_none)
			{
				user_term_set_state(USER_TERM_ST_TEST_EXEC);
			}
		}
	}
	else
	{
		PRINT(pString_Offline);
		user_term_reset_to_state(USER_TERM_ST_MAIN);
	}
}

/**
 * @brief User Terminal implementation for execution of UDP tests (single/multi/loopback).
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_test_exec(user_term_action_t action)
{
	user_input_t * user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		switch(user_term_test.test_type)
		{
		case test_type_basic:
			PRINT("<<< Basic UDP test >>>\n\n");
			break;
		case test_type_multi:
			PRINT("<<< Multiple basic UDP tests >>>\n\n");
			break;
		case test_type_loopback:
			PRINT("<<< Loopback UDP test >>>\n\n");
			break;
		case test_type_multicast:
			PRINT("<<< Multicast UDP test >>>\n\n");
			break;
		default:
			break;
		}

		PRINT("1) Transmit\n");
		PRINT("2) Receive\n");
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		uint32_t default_value;
		bool value_in_range;

#if IS_COORD
		/* Sets the first device in the sorted table as destination */
		default_value = (uint32_t) g3_app_boot_find_first_device(boot_state_connected)->short_addr;
#else /* IS_COORD */
		default_value = COORD_ADDRESS;
#endif /* IS_COORD */
		switch (user_term_test.test_step)
		{
		case USER_TERM_TESTSTEP_0:
			user_input = user_if_get_input();

			/* Parse received command: */
			if (PARSE_CMD_CHAR('1'))
			{
				if ((user_term_test.test_type == test_type_multicast))
				{
					user_term_test.test_step = USER_TERM_TESTSTEP_3;
				}
				else
				{
					user_term_test.test_step = USER_TERM_TESTSTEP_1;
				}

				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
			else if (PARSE_CMD_CHAR('2'))
			{
				user_term_test.test_step = USER_TERM_TESTSTEP_4;
				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
			else if (PARSE_CMD_ANY_CHAR)
			{
				PRINT(pString_NoSuitableAnswerFound);
			}
			else {}
			break;
		case USER_TERM_TESTSTEP_1:
#if IS_COORD
			PRINT_BLANK_LINE();
			if (user_term_print_device_list() > 0)
			{
				PRINT(pString_TypeDestinationAddr, default_value);
				user_term_test.test_step = USER_TERM_TESTSTEP_2;
			}
			else
			{
				PRINT(pString_NoDeviceFound);
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
#else /* IS_COORD */
			PRINT(pString_TypeDestinationAddr, default_value);
			user_term_test.test_step = USER_TERM_TESTSTEP_2;
#endif /* IS_COORD */
			break;
		case USER_TERM_TESTSTEP_2:
			user_input = user_if_get_input();

			if (PARSE_CMD_ANY_CHAR)
			{
				user_term_fsm.dest_short_addr = user_term_assign_user_value(user_input, default_value, "destination short address", false);

				value_in_range = (user_term_fsm.dest_short_addr < MAC_BROADCAST_SHORT_ADDR);
#if IS_COORD
				value_in_range &= (user_term_fsm.dest_short_addr != COORD_ADDRESS);
#else
				value_in_range &= (user_term_fsm.dest_short_addr != userg3_common.device_addr);
#endif /* IS_COORD */
				if (value_in_range)
				{
					user_term_test.test_step = USER_TERM_TESTSTEP_3;
				}
				else
				{
					PRINT(pString_DestinationAddrOOR);
				}

				RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
			}
			break;
		case USER_TERM_TESTSTEP_3:
			switch(user_term_test.test_type)
			{
			case test_type_basic:
				user_term_udp_test_basic_tx();
				break;
			case test_type_multi:
				user_term_udp_test_multi_basic_tx();
				break;
			case test_type_loopback:
				user_term_udp_test_loop_tx();
				break;
			case test_type_multicast:
				user_term_udp_test_multicast_tx();
				break;
			default:
				break;
			}
			break;
		case USER_TERM_TESTSTEP_4:
			switch(user_term_test.test_type)
			{
			case test_type_basic:
				user_term_udp_test_basic_rx();
				break;
			case test_type_multi:
				user_term_udp_test_multi_rx();
				break;
			case test_type_loopback:
				user_term_udp_test_loop_rx();
				break;
			case test_type_multicast:
				user_term_udp_test_basic_rx(); /* Same as basic on RX side */
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}


/**
 * @brief User Terminal implementation for printing the UDP connection information.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_print_connection(user_term_action_t action)
{
	user_input_t * user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		UserG3_PlatformInfoRequest(); /* Ask for platform information */
		user_term_print_waiting_for_msg(USEREVT_G3_DBGTOOLS_CNF);
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		if (USER_EVENT_RISEN(USEREVT_G3_DBGTOOLS_CNF))
		{
			user_term_print_connection_info();

			PRINT("<< UDP Connections >>\n\n");

			for (uint32_t i = 0; i < userg3_common.connection_number; i++)
			{
				const user_conn_t* connection = UserG3_GetUdpConnection(i);
				ALLOC_DYNAMIC_HEX_STRING(rem_addr_str, connection->remote_address.u8, sizeof(connection->remote_address.u8));

				PRINT_NOTS("\t\tConnection ID       - %u\n", connection->connection_id);
				PRINT_NOTS("\t\tRemote IPv6 address - %s\n", rem_addr_str);
				PRINT_NOTS("\t\tLocal port          - %u\n", connection->local_port);
				PRINT_NOTS("\t\tRemote port         - %u\n\n", connection->remote_port);

				FREE_DYNAMIC_HEX_STRING(rem_addr_str);
			}
			PRINT(pString_ReturnToMainMenu);
		}

		user_input = user_if_get_input();

		if (PARSE_CMD_ANY_CHAR)
		{
			PRINT_BLANK_LINE();
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
	}
}

/**
 * @brief This function prints the device table of the coordinator, or the coordinator info.
 * @param action Type of action from user_term_action_t to run.
 * @retval None
 */
static void user_term_state_print_device_list(user_term_action_t action)
{
	user_input_t *user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		PRINT("<< Connected devices >>\n\n");
#if IS_COORD
		PRINT("Coordinator - PAN ID: %X, short address: 0\n\n", userg3_common.pan_id);
		user_term_print_device_list();
#else /* IS_COORD */
		PRINT("This device - PAN ID: %X, short address: %u.\n", userg3_common.pan_id, userg3_common.device_addr);
#endif /* IS_COORD */
		PRINT(pString_ReturnToMainMenu);
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		user_input = user_if_get_input();
		if (PARSE_CMD_ANY_CHAR)
		{
			PRINT_BLANK_LINE();
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
	}
}

#if ENABLE_IMAGE_TRANSFER
/**
 * @brief User Terminal implementation for the terminal UDP image transfer menu.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_transfer(user_term_action_t action)
{
	if (userg3_common.online)
	{
#if IS_COORD
		/* Default destination */
		uint32_t default_value = (uint32_t) g3_app_boot_find_first_device(boot_state_connected)->short_addr;
#endif
		if (action == USER_TERM_ACT_DISPMENU)
		{
			PRINT_BLANK_LINE();
			PRINT("<< UDP image transfer >>\n\n");
#if IS_COORD
			if (userg3_common.online)
			{
				uint16_t device_count = user_term_print_device_list();

				if (device_count > 0)
				{
					PRINT(pString_TypeDestinationAddr, default_value);
				}
				else
				{
					PRINT(pString_NoDeviceFound);
					user_term_reset_to_state(USER_TERM_ST_MAIN);
				}
			}
			else
			{
				PRINT(pString_Offline);
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
#else
			PRINT("The image has to be sent from the coordinator\n");
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
#endif
		}
		else /* if (action == USER_TERM_ACT_PROCSEL) */
		{
#if IS_COORD
			if (user_term_fsm.req_step == USER_TERM_REQUEST)
			{
				user_input_t *user_input = user_if_get_input();

				if (PARSE_CMD_ANY_CHAR)
				{
					/* Acquires destination */
					user_term_fsm.dest_short_addr = user_term_assign_user_value(user_input, default_value, "destination short address", false);

					if ((user_term_fsm.dest_short_addr != COORD_ADDRESS) && (user_term_fsm.dest_short_addr < MAC_BROADCAST_SHORT_ADDR))
					{
						/* Kick device */
						boot_device_t* recipient_device = g3_app_boot_find_device(NULL, user_term_fsm.dest_short_addr, boot_state_connected);

						if (recipient_device != NULL)
						{
							/* Proceeds */
							user_term_fsm.req_step = USER_TERM_EXECUTE;

							RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
						}
						else
						{
							PRINT(pString_DeviceNotFound);
							user_term_reset_to_state(USER_TERM_ST_MAIN);
						}
					}
					else
					{
						/* Re-acquires input destination */
						PRINT(pString_DestinationAddrOOR);
					}
				}
			}
			else if (user_term_fsm.req_step == USER_TERM_EXECUTE)
			{
				user_term_udp_transfer_tx();
			}
#else
			user_term_udp_transfer_rx();
#endif
		}
	}
	else
	{
		PRINT(pString_Offline);
		user_term_reset_to_state(USER_TERM_ST_MAIN);
	}
}
#endif /* ENABLE_IMAGE_TRANSFER */

#if ENABLE_SFLASH_MANAGEMENT
/**
 * @brief User Terminal implementation for the SPI FLASH erase menu.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_erase_sflash(user_term_action_t action)
{
	user_input_t * user_input = NULL;

	uint8_t erase_index;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		PRINT("<< SFLASH management >>");
		PRINT_BLANK_LINE();
		checkMemoryContent(user_term_transfer.slot);
		PRINT("Press 'y' then ENTER to erase all slots (1 - 4)\n");
		PRINT("Press '1', '2', '3', '4', then ENTER to erase a single slot\n");
		PRINT("Press 'm' then ENTER to erase the NVM\n");
		PRINT("Press 'n' then ENTER to abort\n");
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		user_input = user_if_get_input();

		/* Parse received command: */
		if (PARSE_CMD_CHAR('y'))
		{
			PRINT("Erasing all flash memory slots...\n");

			if (eraseMemory() == true)
			{
				PRINT("Erasure complete\n");
			}
			else
			{
				PRINT("Erasure failed\n");
			}

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else if (PARSE_CMD_IN_RANGE('1', '4'))
		{
			erase_index = GET_CMD_DIGIT();

			PRINT("Erasing slot %u...\n", erase_index);
			utils_delay_ms(10); /* Gives time slice to the print task */
			if (eraseMemorySector(erase_index - 1) == true)
			{
				PRINT("Erasure complete\n");
			}
			else
			{
				PRINT("Erasure failed for slot %u\n", erase_index);
			}

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else if (PARSE_CMD_CHAR('m'))
		{
			hif_nvm_req_t *hif_nvm_req = MEMPOOL_MALLOC(sizeof(hif_nvm_req_t)); /* Uses memory pool due to big structure size */

			uint16_t len = hi_hostif_nvm_fill(hif_nvm_req, flash_op_erase, nvm_none, 0, 0, NULL);

			g3_send_message(HIF_TX_MSG, HIF_HI_NVM_REQ, hif_nvm_req, len);

			PRINT("NVM erased\n");

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else if (PARSE_CMD_CHAR('n'))
		{
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else if (PARSE_CMD_ANY_CHAR)
		{
			PRINT(pString_NoSuitableAnswerFound);
		}
		else {}
	}
}
#endif /* ENABLE_SFLASH_MANAGEMENT */

#if ENABLE_DEVICE_MANAGEMENT
/**
 * @brief User Terminal implementation for the device management menu.
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_device_management(user_term_action_t action)
{
#if IS_COORD
	uint32_t default_value = (uint32_t) g3_app_boot_find_first_device(boot_state_connected)->short_addr;
#endif /* IS_COORD */
	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
#if IS_COORD
		PRINT("<< Coordinator management >>\n\n");

		if (userg3_common.online)
		{
			uint16_t device_count = user_term_print_device_list();

			if (device_count > 0)
			{
				PRINT("Type the short address of the device to kick (default: %u), then ENTER\n", default_value);
			}
			else
			{
				PRINT(pString_NoDeviceFound);
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
		}
		else
		{
			PRINT(pString_Offline);
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
#else /* IS_COORD */
		PRINT("<< Device management >>\n");
		if (userg3_common.online)
		{
			PRINT("Disconnect? (y/n)\n");
		}
		else
		{
			PRINT("  Reconnect? (y/n)\n");
		}
#endif /* IS_COORD */
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		if (user_term_fsm.req_step == USER_TERM_REQUEST)
		{
			user_input_t *user_input = user_if_get_input();
#if IS_COORD
			if (PARSE_CMD_ANY_CHAR)
			{
				/* Acquires destination */
				user_term_fsm.dest_short_addr = user_term_assign_user_value(user_input, default_value, "short address of the device to kick", false);

				if ((user_term_fsm.dest_short_addr != COORD_ADDRESS) && (user_term_fsm.dest_short_addr < MAC_BROADCAST_SHORT_ADDR))
				{
					/* Kick device */
					boot_device_t* device_to_kick = g3_app_boot_find_device(NULL, user_term_fsm.dest_short_addr, boot_state_connected);

					if (device_to_kick != NULL)
					{
						g3_app_boot_kick_device(device_to_kick);
						user_term_print_waiting_for_msg(USEREVT_G3_BOOT_SRV_KICK_CNF);

						user_term_fsm.req_step = USER_TERM_EXECUTE;
					}
					else
					{
						PRINT(pString_DeviceNotFound);
						user_term_reset_to_state(USER_TERM_ST_MAIN);
					}
				}
				else
				{
					/* Re-acquires input destination */
					PRINT(pString_DestinationAddrOOR);
				}
			}
#else /* IS_COORD */
			PRINT_BLANK_LINE();
			if (PARSE_CMD_CHAR('y') || PARSE_CMD_CHAR('Y'))
			{
				if (userg3_common.online)
				{
					/* Leave PAN */
					g3_app_boot_leave();
					user_term_print_waiting_for_msg(USEREVT_G3_BOOT_DEV_LEAVE_CNF);
				}
				else
				{
					/* Re-join PAN */
					g3_app_boot_restart();
					user_term_print_waiting_for_msg(USEREVT_G3_BOOT_DEV_START_CNF);
				}

				user_term_fsm.req_step = USER_TERM_EXECUTE;
			}
			else
			{
				PRINT_BLANK_LINE();
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
#endif /* IS_COORD */
		}
		else if (user_term_fsm.req_step == USER_TERM_EXECUTE)
		{
#if IS_COORD
			if (USER_EVENT_RISEN(USEREVT_G3_BOOT_SRV_KICK_CNF))
			{
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
#else /* IS_COORD */
			if ((USER_EVENT_RISEN(USEREVT_G3_BOOT_DEV_START_CNF)) || (USER_EVENT_RISEN(USEREVT_G3_BOOT_DEV_LEAVE_CNF)))
			{
				user_term_reset_to_state(USER_TERM_ST_MAIN);
			}
#endif /* IS_COORD */
		}
	}
}

#endif /* ENABLE_DEVICE_MANAGEMENT */

#if !IS_COORD && ENABLE_FAST_RESTORE
/**
 * @brief User Terminal implementation for enabling/disabling the Fast Restore boot (the platform configuration is loaded from NVM).
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_fast_restore(user_term_action_t action)
{
	user_input_t * user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		PRINT("<< Fast Restore >>\n");
		if (!fast_restore_enabled)
		{
			PRINT("Enable Fast Restore? (y/n)\n");
		}
		else
		{
			PRINT("Disable Fast Restore? (y/n)\n");
		}
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		user_input = user_if_get_input();

		/* Parse received command: */
		if (PARSE_CMD_CHAR('y'))
		{
			if (!fast_restore_enabled)
			{
				PRINT("Switch to fast restore boot\n");
				fast_restore_enabled = true;
			}
			else
			{
				PRINT("Switch to normal boot\n");
				fast_restore_enabled = false;
			}

		    /* Intialize storage buffer */
			flash_data_t flash_data;
		    /* read data stored in STM32 Internal Flash */
		    /* this step is due to the need to Erase the Page Before a Write Operation */
		    flash_data = usr_flash_data;
		    /* update fast_restore_enabled value */
			flash_data.str.fast_restore_enabled = (uint8_t)fast_restore_enabled;
			/* Write data to STM32 Internal Flash Memory */
			uint32_t PageError;
			HAL_StatusTypeDef status;
			FLASH_EraseInitTypeDef EraseInitStruct;
			EraseInitStruct.TypeErase	= FLASH_TYPEERASE_PAGES;
			EraseInitStruct.Banks       = (((uint32_t)&usr_flash_data - FLASH_BASE) / FLASH_BANK_SIZE) + 1;
			EraseInitStruct.Page        = (((uint32_t)&usr_flash_data - FLASH_BASE) % FLASH_BANK_SIZE) / FLASH_PAGE_SIZE ;
			EraseInitStruct.NbPages 	= 1;

			HAL_FLASH_Unlock();
			/* Erase the Page Before a Write Operation */
			status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
			/* Write data to STM32 Internal Flash Memory */
#if defined(STM32G070xx) || defined(STM32G474xx) || defined(STM32L476xx)
			for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 8;  j++)
				status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)&usr_flash_data + j*8, flash_data.array[j]);
#elif defined(STM32U545xx)
			for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 16;  j++)
				status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, (uint32_t)&usr_flash_data + j*16, flash_data.array[j]);
#endif
			HAL_FLASH_Lock();

			if (status == HAL_OK)
				PRINT("Fast Restore setting written to STM32 Internal Flash\n");
			else
				PRINT("ERROR in writing Fast Restore setting to STM32 Internal Flash\n");

			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
		else
		{
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
	}
}

#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING

/**
 * @brief User Terminal implementation for the trigger for the PAN GMK update, also called Re-keying (coordinator only).
 * @param action Type of action to run.
 * @retval None
 */
static void user_term_state_rekeying(user_term_action_t action)
{
	if (userg3_common.online)
	{
		uint8_t new_gmk[MAC_KEY_SIZE];
		const uint8_t default_gmk[MAC_KEY_SIZE] = DEFAULT_GMK;

		user_input_t * user_input = NULL;

		if (action == USER_TERM_ACT_DISPMENU)
		{
			ALLOC_STATIC_HEX_STRING(default_gmk_str, default_gmk, sizeof(default_gmk));

			PRINT_BLANK_LINE();
			PRINT("<< GMK update >>\n\n");

			PRINT("Insert a 16 bytes hexadecimal GMK without spaces or '0x', then press ENTER\n");
			PRINT("Default: %s\n", default_gmk_str);
		}
		else /* if (action == USER_TERM_ACT_PROCSEL) */
		{
			user_input = user_if_get_input();

			if (user_term_fsm.req_step == USER_TERM_REQUEST)
			{
				if (PARSE_CMD_ANY_CHAR)
				{
					uint16_t len = user_term_convert_str_to_hex(new_gmk, sizeof(new_gmk), user_input);

					if (len == 0)
					{
						len = sizeof(new_gmk);
						memcpy(new_gmk, default_gmk, sizeof(new_gmk));
						PRINT("Updating all PAN devices with default GMK\n");
					}

					if (len == sizeof(new_gmk))
					{
						ALLOC_STATIC_HEX_STRING(gmk_str, new_gmk, sizeof(new_gmk));
						PRINT("Updating all PAN devices with GMK: %s\n", gmk_str);
						PRINT("Press 'a' then ENTER to abort\n");
#if ENABLE_ICMP_KEEP_ALIVE
						/* Suspends Keep-alive during the re-keying, to lighten the traffic */
						if (g3_app_ka_in_progress())
						{
							g3_app_ka_wait();
							g3_app_ka_stop();
						}

						user_term_fsm.ka_was_suspended = true;
#endif /* ENABLE_ICMP_KEEP_ALIVE */
						UserG3_SendRekeyingRequest(new_gmk);
						UserG3_StartTimeCounter();  /* Start timer in order to evaluate rekeying duration */

						user_term_set_timeout(REKEYING_TIMEOUT_DELAY);

						user_term_print_waiting_for_msg(USEREVT_G3_BOOT_SRV_REKEYING_CNF);

						user_term_fsm.req_step = USER_TERM_EXECUTE;
					}
					else
					{
						PRINT("Invalid hex string (%u/%u digits)\n", len, MAC_KEY_SIZE);
						user_term_reset_to_state(USER_TERM_ST_MAIN);
					}
				}
			}
			else if (user_term_fsm.req_step == USER_TERM_EXECUTE)
			{
				if (USER_EVENT_RISEN(USEREVT_G3_BOOT_SRV_REKEYING_CNF))
				{
					user_term_remove_timeout();
					UserG3_StopTimecounter();

					uint32_t elapsed_time = UserG3_GetTimecounter();

					uint32_t elapsed_s = elapsed_time / 1000;
					uint32_t elapsed_ms = elapsed_time % 1000;

					PRINT("GMK update completed in %u.%u s\n", elapsed_s, elapsed_ms);
#if ENABLE_ICMP_KEEP_ALIVE
					/* Resumes Keep-alive */
					if (user_term_fsm.ka_was_suspended)
					{
						user_term_fsm.ka_was_suspended = false;
						g3_app_ka_start();
					}
#endif /* ENABLE_ICMP_KEEP_ALIVE */
					user_term_set_state(USER_TERM_ST_MAIN);
				}
				else if (user_term_timeout_reached())
				{
					/* Aborts re-keying and goes to the main menu */
					PRINT("Timeout, aborting GMK update...\n");
					UserG3_SendRekeyingAbortRequest();
					user_term_reset_to_state(USER_TERM_ST_MAIN);
				}
				else if (PARSE_CMD_ANY_CHAR)
				{
					PRINT_BLANK_LINE();
					if (PARSE_CMD_CHAR('a') || PARSE_CMD_CHAR('A'))
					{
						/* Aborts rekeying, remaining in this state to wait the CNF */
						PRINT("Aborting GMK update...\n");
						UserG3_SendRekeyingAbortRequest();
					}
				}
			}
		}
	}
	else
	{
		PRINT(pString_Offline);
		user_term_reset_to_state(USER_TERM_ST_MAIN);
	}
}
#endif /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING */

/**
 * @brief User Terminal implementation for the system reset.
 * @param action Type of action to run
 * @retval None
 */
static void user_term_state_reset(user_term_action_t action)
{
	user_input_t * user_input = NULL;

	if (action == USER_TERM_ACT_DISPMENU)
	{
		PRINT_BLANK_LINE();
		PRINT("<< System reset >>\n\n");

		PRINT("Reset the system? (y/n)\n");
	}
	else /* if (action == USER_TERM_ACT_PROCSEL) */
	{
		user_input = user_if_get_input();

		/* Parse received command: */
		if (PARSE_CMD_CHAR('y'))
		{
			PRINT("Waiting for reset...\n");
			HAL_NVIC_SystemReset();
			utils_delay_ms(osWaitForever);
		}
		else
		{
			user_term_reset_to_state(USER_TERM_ST_MAIN);
		}
	}
}

/* Function pointer prototype */
typedef void user_term_func(user_term_action_t action);

/**
 * @brief  State Functions Table
 */
static user_term_func* const user_term_func_tbl[USER_TERM_ST_CNT] =
{
		/* ENTRY				*/ user_term_terminal_startup,
		/* CONNECTION 			*/ user_term_state_set_connection,
		/* MAIN 				*/ user_term_state_main_menu,
		/* TESTS 				*/ user_term_state_test,
		/* TEST_EXEC 			*/ user_term_state_test_exec,
		/* DIPLAY_CONNECTION	*/ user_term_state_print_connection,
		/* DIPLAY_DEVICES		*/ user_term_state_print_device_list,

#if ENABLE_IMAGE_TRANSFER
		/* TRANSFER 			*/ user_term_state_transfer,
#endif  /* ENABLE_IMAGE_TRANSFER */

#if ENABLE_SFLASH_MANAGEMENT
		/* SFLASH				*/ user_term_state_erase_sflash,
#endif  /* ENABLE_SFLASH_MANAGEMENT */

#if ENABLE_DEVICE_MANAGEMENT
		/* DEVICE_MANAGEMENT    */ user_term_state_device_management,
#endif /* ENABLE_DEVICE_MANAGEMENT */

#if !IS_COORD && ENABLE_FAST_RESTORE
		/* FAST_RESTORE			*/ user_term_state_fast_restore,
#endif  /* !IS_COORD && ENABLE_FAST_RESTORE */

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING
		/* REKEYING				*/ user_term_state_rekeying,
#endif  /* IS_COORD && ENABLE_BOOT_SERVER_ON_HOST && ENABLE_REKEYING */

		/* RESET				*/ user_term_state_reset,
};

/**
 * @}
 */

/** @addtogroup User_Terminal_Exported_Code
 * @{
 */

/**
 * @brief This function handles User interface FSM initialization
 * @note To be called once in the User task initialization function.
 * @retval None
 */
void UserTerminal_Init(void)
{
#if ENABLE_ICMP_KEEP_ALIVE
	user_term_fsm.ka_was_suspended = false;
#endif
	user_term_fsm.dest_short_addr = COORD_ADDRESS;

	user_term_reset_to_state(USER_TERM_ST_ENTRY);
}

/**
 * @brief Function that handles the state of the User Interface FSM
 * @param None
 * @retval None
 * @note Entry function called from User task infinite execution loop.
 */
void UserTerminal_FsmManager_IPv6(void)
{
	user_term_parse_user_events();

	user_term_parse_specific_command();

	user_term_action_t prev_action = user_term_fsm.action;

	user_term_func_tbl[user_term_fsm.state](user_term_fsm.action);

	if ((prev_action == USER_TERM_ACT_DISPMENU) && (user_term_fsm.action == USER_TERM_ACT_DISPMENU))
	{
		user_term_fsm.action = USER_TERM_ACT_PROCSEL;
	}
}

/**
 * @brief Function that handles the state of the User Interface FSM in MAC mode
 * @param None
 * @retval None
 * @note Entry function called from User task infinite execution loop.
 */
void UserTerminal_FsmManager_MAC(void)
{
	user_input_t* user_input = user_if_get_input();

	if ((user_input != NULL) &&  UserMac_IsReady())
	{
		uint8_t dest_addr[MAC_ADDR64_SIZE];
		usermac_test_type_t test_type;
		uint32_t msg_number;

		/* command string format: MAC_ADDRESS.PLC|RF.MESSAGE_NUMBER*/

		/* Converts the extended address from string to array */
		uint16_t dest_addr_len = user_term_convert_str_to_hex(dest_addr, sizeof(dest_addr), user_input);

		/* Splits the input command string to acquire the test parameters */
		char * token = strtok((char*) user_input->payload, ASCII_DELIMITER);

		/* strtok called again to select the substring after the first dot */
		token = strtok(NULL, ASCII_DELIMITER);

		if (token != NULL)
		{
			/* Parses the first test parameter */
			if (strncmp(token, "PLCRF", STR_PLCRF_LEN) == 0)
			{
				test_type = mac_test_plc_rf;
			}
			else if (strncmp(token, "PLC", STR_PLC_LEN) == 0)
			{
				test_type = mac_test_plc;
			}
			else if (strncmp(token, "RF", STR_RF_LEN) == 0)
			{
				test_type = mac_test_rf;
			}
			else
			{
				PRINT("Type format error, using default: PLCRF\n");
				test_type = mac_test_plc_rf;
			}

			/* strtok called again to select the substring after the second dot */
			token = strtok(NULL, ASCII_DELIMITER);

			if (token != NULL)
			{
				/* Parses the second test parameter */
				msg_number = atoi(token);
			}
			else
			{
				PRINT("Message number format error, using default: %u\n", DEFAULT_TEST_MSG_NUMBER);
				msg_number = DEFAULT_TEST_MSG_NUMBER;
			}
		}
		else
		{
			test_type = mac_test_plc_rf;
			msg_number = DEFAULT_TEST_MSG_NUMBER;
		}

		/* Checks inserted extended address length */
		if (dest_addr_len == sizeof(dest_addr))
		{
			ALLOC_STATIC_HEX_STRING(dest_addr_str, dest_addr, sizeof(dest_addr));
			PRINT("Testing %u messages, type: %X, destination: %s\n", msg_number, test_type, dest_addr_str);

			/* Starts the MAC test */
			UserMac_StartTxCallback(dest_addr, test_type, msg_number);
		}
		else
		{
			PRINT("Insert MAC address in HEX format (e.g. '0102030405060708'):\n");
		}
	}
}

/**
 * @brief Callback function of the userTimeoutTimer FreeRTOStimer. Warns the user about the timeout event and unblocks the User Task.
 * @param argument Unused argument.
 * @retval None
 */
void UserTerminal_TimeoutCallback(void *argument)
{
	UNUSED(argument);

	PRINT("Timeout reached\n");

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
