/**
  ******************************************************************************
  * @file    user_mac.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code for the user MAC/test mode.
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
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <cmsis_os.h>
#include <assert.h>
#include <utils.h>
#include <pin_management.h>
#include <mem_pool.h>
#include <debug_print.h>
#include <sflash_info.h>
#include <sflash.h>
#include <g3_app_config.h>
#include <hi_mac_sap_interface.h>
#include <hi_msgs_impl.h>
#include <user_if.h>
#include <user_modbus.h>
#include <user_mac.h>
#include <ff.h>

/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_MAC
  * @{
  */

/** @addtogroup User_MAC_Private_Code
  * @{
  */

/* Definitions */
#define VERBOSE_LOG									0

#define TEST_MSG_SIZE								(10)		/* Size of the dummy payload */
#define REQUEST_MAX_RETRIES							(3)			/* Retries in case of CNF error */

/* Timing */
#define MAC_CNF_TIMEOUT								(3000)	/* In ms */
#define MAC_IND_TIMEOUT								(5000)	/* In ms */

#define MAC_RF_DELAY								(50)	/* In ms */

/* Macros */
#define PRINT_CNF_ERROR(cnf_id, status) 	PRINT("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id))


/* Private constants and macros */

/* Local types */

/* User MAC FSM states */
typedef enum usermac_state_enum
{
	USER_MAC_ST_READY,
	USER_MAC_ST_RX_ONGOING,
	USER_MAC_ST_TX_ONGOING,
	USER_MAC_ST_CNT
} usermac_state_t;

/* User MAC FSM events */
typedef enum usermac_event_enum
{
	USER_MAC_EV_NONE = 0,
	USER_MAC_EV_START_TX,
	USER_MAC_EV_RECEIVED_NEG_DATA_CNF,
	USER_MAC_EV_RECEIVED_DATA_IND,
	USER_MAC_EV_TIMEOUT,
	USER_MAC_EV_CNT
} usermac_event_t;

typedef enum usermac_msg_type_enum
{
	IS_REQUEST = 1,
	IS_ANSWER  = 2
} usermac_msg_type_t;

/* Private structures */

#pragma pack(push, 1)

/* Structure of the message used for testing */
typedef struct usermac_ping_msg_str
{
	uint8_t 	type;		/* Request or answer */
	uint8_t 	media_type; /* PLC and RF messages must be different to avoid  misunderstandings (the modem can mistake equal frames for duplicates) */
	uint32_t	msg_i;		/* Number ID of the message */
	uint32_t	msg_n;		/* Total number of messages to expect on RX side */
	uint8_t		test_payload[TEST_MSG_SIZE]; /* Additional bytes to enlarge the size of the ping message */
} usermac_ping_msg_t;

#pragma pack(pop)

typedef struct mac_ind_info_str
{
	uint16_t 				msdu_len;
	uint8_t 				msdu[MAC_PAYLOAD_MAX_SIZE];
	uint8_t  				dsn;
	uint32_t 				timestamp;
	mac_security_level_t	security_level;
	uint8_t					key_index;
	mac_qos_t				qos;
	uint8_t 				lqi;
	mac_mediatype_req_t 	media_type;
} mac_ind_info_t;

/* User MAC FSM */
typedef struct usermac_fsm_info_str
{
	/* Test state/event */
	usermac_state_t			curr_state;				/*!<  Current User MAC FSM state */
	usermac_event_t			curr_event;				/*!<  Current User MAC FSM event */

	uint32_t				operation_counter;

	/* Internal test variables */
	uint8_t 				mac_handle;
	uint32_t 				req_counter;
	uint32_t 				ind_counter;
	bool					received_cnf;
	bool					received_ind;

	/* Test parameters */
	uint8_t					dst_addr[MAC_ADDR64_SIZE];
	usermac_test_type_t		test_type;
	uint32_t				test_msg_n;

	/* Message information */
	mac_ind_info_t			last_req;
	mac_ind_info_t			last_ind;

	/* Errors */
	uint32_t				cnf_errors;
} usermac_fsm_t;

/* Private variables */
static usermac_fsm_t 	usermac_fsm;			/*!<  Contains the FSM data for the User MAC */


/* External Variables */
extern uint32_t 			user_task_operation_counter;

extern BOOT_Bandplan_t		working_plc_band;
extern modbus_role_t    	modbus_role;

extern osMessageQueueId_t	user_queueHandle;

extern osTimerId_t			commTimerHandle;

/**
  * @}
  */

/** @addtogroup User_MAC_Exported_Code
  * @{
  */

/* External variables */
extern modbus_role_t   modbus_role;


/**
  * @}
  */

/** @addtogroup User_MAC_Private_Code
  * @{
  */

/* Private function pointer type */
typedef usermac_state_t usermac_fsm_func(void);

/* Private FSM function prototypes */
static usermac_state_t usermac_fsm_default(void);
static usermac_state_t usermac_fsm_send_frame(void);
static usermac_state_t usermac_fsm_reply_frame(void);
static usermac_state_t usermac_fsm_timeout_reached(void);

/* Private FSM function pointer array */
static usermac_fsm_func *usermac_fsm_func_tbl[USER_MAC_ST_CNT][USER_MAC_EV_CNT] = {
/*                 NONE,                START_TX,               RECEIVED_NEG_DATA_CNF,   RECEIVED_DATA_IND,       TIMEOUT                     */
/* READY      */ { usermac_fsm_default, usermac_fsm_send_frame, usermac_fsm_default,     usermac_fsm_reply_frame, usermac_fsm_default         },
/* RX_ONGOING */ { usermac_fsm_default, usermac_fsm_default,    usermac_fsm_reply_frame, usermac_fsm_reply_frame, usermac_fsm_timeout_reached },
/* TX_ONGOING */ { usermac_fsm_default, usermac_fsm_default,    usermac_fsm_send_frame,  usermac_fsm_send_frame,  usermac_fsm_timeout_reached },
};

/* Private functions */

/**
  * @brief Sets the LEDs according to the test state.
  * @param test_state The test state.
  * @retval None.
  */
static void usermac_set_leds(const mac_test_state_t test_state)
{
	LED_SET(LED0, (test_state & 0x01));
	LED_SET(LED1, (test_state & 0x02));
	LED_SET(LED2, (test_state & 0x04));
	LED_SET(LED3, (test_state & 0x08));
}

/**
  * @brief Toggle the LEDs according to the test state.
  * @param test_state The test state.
  * @retval None.
  */
static void usermac_toggle_leds(const mac_test_state_t test_state)
{
	if (test_state & 0x01) LED_TOGGLE(LED0); else LED_SET_OFF(LED0);
	if (test_state & 0x02) LED_TOGGLE(LED1); else LED_SET_OFF(LED1);
	if (test_state & 0x04) LED_TOGGLE(LED2); else LED_SET_OFF(LED2);
	if (test_state & 0x08) LED_TOGGLE(LED3); else LED_SET_OFF(LED3);
}

/**
  * @brief Starts counting for the timeout of a transfer operation.
  * @param timeout The timeout value for the operation.
  * @retval Estimated elapsed time.
  */
static void usermac_set_timeout(uint32_t timeout)
{
	osStatus_t os_result = osTimerStart(commTimerHandle, timeout);

	if (os_result != osOK)
	{
		PRINT("Timer could not be started\n\r");
	}
}

/**
  * @brief Stops counting for the timeout of a transfer operation.
  * @param None
  * @retval None
  */
static void usermac_remove_timeout(void)
{
	osTimerStop(commTimerHandle);
}

/* Test functions */

#if ENABLE_FATFS_TEST
/**
  * @brief Tests the FAT File System and the SD card reader.
  * @param None
  * @retval None
  */
static void usermac_test_fs(void)
{
	FATFS   *fat_fs = MEMPOOL_MALLOC(sizeof(FATFS)); 	/* FAT FS handle. Allocated with memory pool due to big size */
	FIL     file; 										/* File handle */
	FRESULT fres;										/* File operation result */

	PRINT("Testing uSD card slot...\n");

	BYTE write_buffer[] = "Sample test for testing.\n";
	BYTE read_buffer[sizeof(write_buffer)] = "";

	//Open the file system (note: a semaphore is allocated while the FS is mounted)
	fres = f_mount(fat_fs, "", 1); //1=mount now

	if (fres == FR_OK)
	{
		/* Gets statistics from the SD card */
		DWORD free_clusters, free_sectors, total_sectors;

		FATFS* getFreeFs;

		fres = f_getfree("", &free_clusters, &getFreeFs);

		if (fres == FR_OK)
		{
			/* Calculate total/available sectors */
			total_sectors = (getFreeFs->n_fatent - 2) * getFreeFs->csize;
			free_sectors = free_clusters * getFreeFs->csize;

			/* Divide by 2 to convert from sectors to kB (each sectors contains 512 bytes) */
			PRINT("uSD card: %lu/%lu kB available\n", free_sectors / 2, total_sectors / 2);

			char file_name[] = "test.txt";

			/* Create file */
			fres = f_open(&file, file_name, FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);

			if (fres != FR_OK)
			{
				PRINT("Open error (%i)\r\n", fres);
			}

			UINT bytes_written, bytes_read;

			fres = f_write(&file, write_buffer, sizeof(write_buffer), &bytes_written);

			if (fres != FR_OK)
			{
				PRINT("Write error (%i)\r\n", fres);
			}

			/* Close file */
			f_close(&file);

			/* Re-open file in read mode */
			fres = f_open(&file, file_name, FA_READ | FA_OPEN_EXISTING);

			if (fres != FR_OK)
			{
				PRINT("Open error (%i)\r\n", fres);
			}

			fres = f_read(&file, read_buffer, sizeof(write_buffer), &bytes_read);

			if (fres == FR_OK)
			{
				if (memcmp(read_buffer, write_buffer, sizeof(write_buffer)) != 0)
				{
					PRINT("Read mismatch: %s vs %s\n", read_buffer, write_buffer);
				}
			}
			else
			{
				PRINT("Read error (%u)\r\n", fres);
			}

			/* Close file */
			f_close(&file);

			/* Unmount drive after */
			f_mount(NULL, "", 0);
		}
		else
		{
			PRINT("Get Free error (%u)\r\n", fres);
		}
	}
	else
	{
		PRINT("Mount error (%u), insert uSD card\r\n", fres);
	}

	if (memcmp(read_buffer, write_buffer, sizeof(write_buffer)) == 0)
	{
		PRINT_COLOR("uSD test ok\n", color_green);
	}
	else
	{
		PRINT_COLOR("uSD test fail\n", color_red);
	}

	MEMPOOL_FREE(fat_fs);
}
#endif

#if ENABLE_SFLASH_TEST
/**
  * @brief Tests the SPI FLASH memory connected to the STM32.
  * @param None
  * @retval None
  */
static void usermac_test_sflash(void)
{
	uint32_t mem_size = 0;
	uint32_t test_size = 4 * SPI_FLASH_SECTOR_SIZE;

	uint32_t sflash_id;

	PRINT("Testing SFLASH...\n");

	if (SFLASH_GET_ID(&sflash_id))
	{
		PRINT("ID: %X \n", sflash_id);

		if ((sflash_id == SPI_FLASH_STM32_ID) || (sflash_id == SPI_FLASH_STM32_ALT_ID))
		{
			PRINT("Testing SFLASH erase...\n");

			uint32_t chunk_size = SPI_FLASH_PAGE_SIZE;

			if (SFLASH_ERASE(0, test_size))
			{
				uint8_t *buf_wr = MEMPOOL_MALLOC(chunk_size);
				uint8_t *buf_rd = MEMPOOL_MALLOC(chunk_size);

				memset(buf_wr, 0xA5, chunk_size);
				memset(buf_rd, 0x00, chunk_size);

				PRINT("Testing SFLASH write/read...\n");

				while (mem_size < test_size)
				{
					if (SFLASH_WRITE(mem_size, buf_wr, chunk_size))
					{
						if (SFLASH_READ(buf_rd, mem_size, chunk_size))
						{
							if (memcmp(buf_rd, buf_wr, chunk_size) == 0)
							{
								mem_size += chunk_size;
							}
							else
							{
								PRINT_COLOR("Compare error\n", color_red);
								break;
							}
						}
						else
						{
							PRINT_COLOR("Read error\n", color_red);
							break;
						}
					}
					else
					{
						PRINT_COLOR("Write error\n", color_red);
						break;
					}
				}

				MEMPOOL_FREE(buf_wr);
				MEMPOOL_FREE(buf_rd);
			}

			PRINT("Testing SFLASH bulk erase...\n");
			SFLASH_BULK_ERASE();

			uint8_t *buf_chk = MEMPOOL_MALLOC(chunk_size);
			uint8_t *buf_rd = MEMPOOL_MALLOC(chunk_size);

			if (SFLASH_READ(buf_rd, 0, chunk_size))
			{
				memset(buf_chk, 0xFF, chunk_size);

				if (memcmp(buf_rd, buf_chk, chunk_size) != 0)
				{
					PRINT_COLOR("Compare error after bulk erase\n", color_red);
					mem_size = 0;
				}
			}

			MEMPOOL_FREE(buf_chk);
			MEMPOOL_FREE(buf_rd);
		}
	}

	if (mem_size == test_size)
	{
		PRINT_COLOR("SFLASH test ok\n", color_green);
	}
	else
	{
		PRINT_COLOR("SFLASH test fail\n", color_red);
	}
}
#endif

/**
  * @brief Function that resets the User MAC variables and sets the test status/error code.
  * @param error_code The error code of the last test.
  * @retval None
  */
static void usermac_reset_fsm(const mac_test_state_t test_state)
{
	/* Remove eventual timeout */
	usermac_remove_timeout();

	/* Set LEDs according to the state/result */
	usermac_set_leds(test_state);

	/* Prints the test result code. Ready state omitted. */
	if (test_state != mac_ready)
	{
		PRINT("Test result: %u\n", test_state);
	}

	/* Print test result (triggers terminal sound with '\a') */
	if (test_state == mac_ok)
	{
		PRINT_COLOR("MAC test ok\a\n", color_green);
	}
	else if (test_state >= mac_cnf_plc_error)
	{
		PRINT_COLOR("MAC test fail\a\n", color_red);
	}

	/* Reset the FSM */
	usermac_fsm.curr_state = USER_MAC_ST_READY;
	usermac_fsm.curr_event = USER_MAC_EV_NONE;

	usermac_fsm.mac_handle   = 0;
	usermac_fsm.req_counter  = 0;
	usermac_fsm.ind_counter  = 0;
	usermac_fsm.received_cnf = false;
	usermac_fsm.received_ind = false;

	memset(&usermac_fsm.dst_addr, 0xFF, sizeof(usermac_fsm.dst_addr));
	usermac_fsm.test_type  = mac_test_plc;
	usermac_fsm.test_msg_n = 0;

	memset(&usermac_fsm.last_req, 0, sizeof(usermac_fsm.last_ind));
	memset(&usermac_fsm.last_ind, 0, sizeof(usermac_fsm.last_ind));

	usermac_fsm.cnf_errors = 0;

	/* Enable/disable the push-button */
	if (modbus_role == modbus_master)
	{
		/* Clear pending EXTI (anti-bouncing) */
		__HAL_GPIO_EXTI_CLEAR_IT(USER_PUSHBUTTON_Pin);

		/* EXTI interrupt enable for push button */
		HAL_NVIC_EnableIRQ(USER_PUSHBUTTON_EXTI_IRQn);
	}
	else
	{
		/* EXTI interrupt enable for push button */
		HAL_NVIC_DisableIRQ(USER_PUSHBUTTON_EXTI_IRQn);
	}
}

/**
  * @brief User MAC FSM function that maintains the current state, with no further action.
  * @param None
  * @return The next state of the User MAC FSM (equal to the current one).
  */
static usermac_state_t usermac_fsm_default(void)
{
	usermac_fsm.curr_event = USER_MAC_EV_NONE;

	return usermac_fsm.curr_state;
}

/**
  * @brief User MAC FSM function that sends a MAC frame
  * @param None
  * @return The next state of the User MAC FSM.
  */
static usermac_state_t usermac_fsm_send_frame(void)
{
	bool 				proceed = true;
	mac_test_state_t	result_state;
	usermac_state_t 	next_state = usermac_fsm.curr_state;

	usermac_ping_msg_t* ping_sent = (usermac_ping_msg_t*) usermac_fsm.last_req.msdu; /* Alias for the request MSDU */
	usermac_ping_msg_t* ping_recv = (usermac_ping_msg_t*) usermac_fsm.last_ind.msdu; /* Alias for the indication MSDU */

	if (!g3_app_conf_ready())
	{
		PRINT("G3 configuration not ready, please wait...\n");

		HAL_NVIC_ClearPendingIRQ(USER_PUSHBUTTON_EXTI_IRQn);
		HAL_NVIC_EnableIRQ(USER_PUSHBUTTON_EXTI_IRQn);

		proceed = false;
	}

	if (proceed)
	{
		if (usermac_fsm.cnf_errors >= REQUEST_MAX_RETRIES)
		{
			next_state = USER_MAC_ST_READY;

			/* Result based on the physical type of the last request */
			result_state = (usermac_fsm.last_req.media_type == MAC_MEDIATYPE_PLC) ? mac_cnf_plc_error : mac_cnf_rf_error;

			usermac_reset_fsm(result_state);

			proceed = false;
		}
		else if ((usermac_fsm.req_counter == 0) && (usermac_fsm.cnf_errors == 0))
		{
				usermac_set_leds(mac_ready);
	#if ENABLE_FATFS_TEST
				usermac_test_fs();
	#endif
	#if ENABLE_MODBUS && ENABLE_MODBUS_TEST
				UserModbus_exec();
	#endif
	#if ENABLE_SFLASH_TEST
				usermac_test_sflash();
	#endif
				PRINT("Starting MAC test...\n");
		}
	}

	if (proceed)
	{
		/* Check received last received frame, if there is one */
		if (usermac_fsm.received_ind)
		{
			if (ping_recv->type != IS_ANSWER)
			{
				proceed = false;

				PRINT("Wrong first byte: %u instead of %u (indicating an answer)\n", ping_recv->type, IS_ANSWER);
			}
			else if (ping_recv->msg_n != ping_sent->msg_n)
			{
				proceed = false;

				PRINT("Message number mismatch, sent: %u, received: %u\n", ping_sent->msg_n, ping_recv->msg_n);
			}
			else if (ping_recv->media_type != ping_sent->media_type)
			{
				proceed = false;

				PRINT("Message media type mismatch, sent: %u, received: %u\n", ping_sent->media_type, ping_recv->media_type);
			}
			else if (memcmp(ping_recv->test_payload, ping_sent->test_payload, TEST_MSG_SIZE) != 0)
			{
				proceed = false;

				ALLOC_DYNAMIC_HEX_STRING(last_req_mdsu_str, &usermac_fsm.last_req.msdu[1], usermac_fsm.last_req.msdu_len-1);
				ALLOC_DYNAMIC_HEX_STRING(last_ind_mdsu_str, &usermac_fsm.last_ind.msdu[1], usermac_fsm.last_req.msdu_len-1);
				PRINT("Payload mismatch, sent: %s\n", last_req_mdsu_str);
				PRINT("Wrong payload received: %s\n", last_ind_mdsu_str);
				FREE_DYNAMIC_HEX_STRING(last_req_mdsu_str);
				FREE_DYNAMIC_HEX_STRING(last_ind_mdsu_str);
			}

			/* Messages with wrong msg ID are discarded */

			if (!proceed)
			{
				next_state = USER_MAC_ST_READY;

				/* Result based on the physical type of the last request */
				result_state = (usermac_fsm.last_req.media_type == MAC_MEDIATYPE_PLC) ? mac_ind_plc_mismatch : mac_ind_rf_mismatch;

				usermac_reset_fsm(result_state);
			}
		}
	}

	if (proceed)
	{
		/* Send new frame */
		if (usermac_fsm.req_counter < usermac_fsm.test_msg_n)
		{
			mac_tx_options_t mac_tx_options;
			MAC_DataReq_t *mac_data_req = MEMPOOL_MALLOC(sizeof(MAC_DataReq_t)); /* Uses memory pool due to big structure size */

			uint8_t broadcast_addr[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

			usermac_fsm.received_cnf = false;
			usermac_fsm.received_ind = false;

			if (memcmp(usermac_fsm.dst_addr, broadcast_addr, sizeof(usermac_fsm.dst_addr)) == 0)
			{
				mac_tx_options = MAC_ACK_REQUEST_OFF;
			}
			else
			{
				mac_tx_options = MAC_ACK_REQUEST_ON;
			}

			usermac_fsm.last_req.key_index = MAC_NO_KEY;

			ping_sent->type = IS_REQUEST;

			if (usermac_fsm.test_type == mac_test_plc_rf)
			{
				/* Half messages PLC, half RF */
				if (usermac_fsm.req_counter < (usermac_fsm.test_msg_n / 2))
				{
					ping_sent->media_type = MAC_MEDIATYPE_PLC;
					usermac_fsm.last_req.media_type = MAC_MEDIATYPE_PLC;
				}
				else
				{
					ping_sent->media_type = MAC_MEDIATYPE_RF;
					usermac_fsm.last_req.media_type = MAC_MEDIATYPE_RF;
				}
			}
			else if (usermac_fsm.test_type == mac_test_plc)
			{
				/* All messages PLC */
				ping_sent->media_type = MAC_MEDIATYPE_PLC;
				usermac_fsm.last_req.media_type = MAC_MEDIATYPE_PLC;
			}
			else if (usermac_fsm.test_type == mac_test_rf)
			{
				/* All messages RF */
				ping_sent->media_type = MAC_MEDIATYPE_RF;
				usermac_fsm.last_req.media_type = MAC_MEDIATYPE_RF;
			}

			/* Increments request counter */
			usermac_fsm.req_counter++;

			ping_sent->msg_i = usermac_fsm.req_counter;

			ping_sent->msg_n = usermac_fsm.test_msg_n;

			for (uint32_t payload_index = 0; payload_index < sizeof(ping_sent->test_payload); payload_index++)
			{
				ping_sent->test_payload[payload_index] = payload_index;
			}

			usermac_fsm.last_req.msdu_len = sizeof(usermac_ping_msg_t);

#if (MAC_RF_DELAY > 0)
			/* Forcefully decrease RF duty cycle */
			if (ping_sent->media_type == MAC_MEDIATYPE_RF)
			{
				utils_delay_ms(MAC_RF_DELAY);
			}
#endif
#if VERBOSE_LOG
			PRINT("Sending request %u/%u\n", ping_sent->msg_i, ping_sent->msg_n);
#endif
			uint16_t len = hi_mac_data_fill(mac_data_req,
											MAC_ADDR_MODE_64,
											PAN_ID,
											MAC_BROADCAST_SHORT_ADDR,
											usermac_fsm.dst_addr,
											usermac_fsm.last_req.msdu_len,
											usermac_fsm.last_req.msdu,
											usermac_fsm.mac_handle++,
											mac_tx_options,
											usermac_fsm.last_req.key_index,
											usermac_fsm.last_req.media_type);

			g3_send_message(HIF_TX_MSG, HIF_MCPS_DATA_REQ, mac_data_req, len);

			usermac_set_timeout(MAC_IND_TIMEOUT);

			usermac_toggle_leds(mac_ready);

			next_state = USER_MAC_ST_TX_ONGOING;
		}
		else
		{
#if VERBOSE_LOG
			PRINT("Test completed\n");
#endif
			usermac_reset_fsm(mac_ok);

			next_state = USER_MAC_ST_READY;
		}
	}

	usermac_fsm.curr_event = USER_MAC_EV_NONE;

	return next_state;
}

/**
  * @brief User MAC FSM function that sends a MAC frame as answer
  * @param None
  * @return The next state of the User MAC FSM.
  */
static usermac_state_t usermac_fsm_reply_frame(void)
{
	usermac_state_t next_state = usermac_fsm.curr_state;

	usermac_ping_msg_t* ping_recv = (usermac_ping_msg_t*) usermac_fsm.last_ind.msdu; /* Alias for the indication MSDU */

	if (usermac_fsm.cnf_errors < REQUEST_MAX_RETRIES)
	{
		if (ping_recv->type == IS_REQUEST)
		{
			next_state = USER_MAC_ST_RX_ONGOING;

			/* Copy payload and modify request/answer field */
			usermac_fsm.last_req.msdu_len = usermac_fsm.last_ind.msdu_len;

			usermac_ping_msg_t* ping_sent = (usermac_ping_msg_t*) usermac_fsm.last_req.msdu; /* Alias for the request MSDU */

			ping_sent->type 		= IS_ANSWER;
			ping_sent->media_type	= ping_recv->media_type;
			ping_sent->msg_i        = ping_recv->msg_i;
			ping_sent->msg_n		= ping_recv->msg_n;

			memcpy(ping_sent->test_payload, ping_recv->test_payload, sizeof(ping_sent->test_payload));

#if (MAC_RF_DELAY > 0)
			if (ping_sent->media_type == MAC_MEDIATYPE_RF)
			{
				utils_delay_ms(MAC_RF_DELAY);
			}
#endif
#if VERBOSE_LOG
			PRINT("Sending answer %u/%u\n", ping_sent->msg_i, ping_sent->msg_n);
#endif
			MAC_DataReq_t *mac_data_req = MEMPOOL_MALLOC(sizeof(MAC_DataReq_t)); /* Uses memory pool due to big structure size */

			uint16_t len = hi_mac_data_fill(mac_data_req,
											MAC_ADDR_MODE_64,
											PAN_ID,
											MAC_BROADCAST_SHORT_ADDR,
											usermac_fsm.dst_addr,
											usermac_fsm.last_req.msdu_len,
											usermac_fsm.last_req.msdu,
											usermac_fsm.mac_handle++,
											MAC_ACK_REQUEST_ON,
											MAC_NO_KEY,
											usermac_fsm.last_ind.media_type);

			g3_send_message(HIF_TX_MSG, HIF_MCPS_DATA_REQ, mac_data_req, len);

			if (usermac_fsm.ind_counter < usermac_fsm.test_msg_n)
			{
				usermac_toggle_leds(mac_ready);

				usermac_set_timeout(MAC_IND_TIMEOUT);
			}
			else
			{
#if VERBOSE_LOG
				PRINT("Test completed\n");
#endif
				next_state = USER_MAC_ST_READY;

				usermac_reset_fsm(mac_ready);
			}
		}
	}
	else
	{
		next_state = USER_MAC_ST_READY;

		mac_test_state_t test_state;

		if (ping_recv->media_type == MAC_MEDIATYPE_PLC)
		{
			test_state = mac_cnf_plc_error;
		}
		else
		{
			test_state = mac_cnf_rf_error;
		}

		usermac_reset_fsm(test_state);
	}

	usermac_fsm.curr_event = USER_MAC_EV_NONE;

	return next_state;
}

/**
  * @brief User MAC FSM function that handles the timeout event
  * @param None
  * @return The next state of the User MAC FSM.
  */
static usermac_state_t usermac_fsm_timeout_reached(void)
{
	mac_test_state_t result_state;

	if (!usermac_fsm.received_cnf)
	{
		result_state = (usermac_fsm.last_req.media_type == MAC_MEDIATYPE_PLC) ? mac_cnf_plc_timeout : mac_cnf_rf_timeout;
	}
	else if (!usermac_fsm.received_ind)
	{
		result_state = (usermac_fsm.last_req.media_type == MAC_MEDIATYPE_PLC) ? mac_ind_plc_timeout : mac_ind_rf_timeout;
	}
	else
	{
		result_state = mac_internal_error;
	}

	usermac_reset_fsm(result_state);

	usermac_fsm.curr_event = USER_MAC_EV_NONE;

	return USER_MAC_ST_READY;
}

/**
  * @brief User MAC function that handles the reception of a G3MAC-DATA confirm
  * @param payload Payload of the received message
  * @return None
  */
static void usermac_handle_mac_data_cnf(const void *payload)
{
	const MAC_DataConfirm_t *mac_cnf = payload;

	usermac_fsm.received_cnf = true;

	if (mac_cnf->status != G3_SUCCESS)
	{
		usermac_fsm.cnf_errors++;
		PRINT_CNF_ERROR(HIF_MCPS_DATA_CNF, mac_cnf->status);

		if (usermac_fsm.curr_state == USER_MAC_ST_TX_ONGOING)
		{
			/* Roll-backs to the previous request count */
			usermac_fsm.req_counter--;
		}

		usermac_fsm.curr_event = USER_MAC_EV_RECEIVED_NEG_DATA_CNF;
	}
}

/**
  * @brief User MAC function that handles the reception of a G3MAC-DATA indication
  * @param payload Payload of the received message
  * @return None
  */
static void usermac_handle_mac_data_ind(const void *payload)
{
	const MAC_DataIndication_t *mac_ind = payload;

	/* Offset used to handle variable message size */
	uint32_t offset = 0;

	/* Handle different size depending of source address mode */
	if (mac_ind->src_addr.addr_mode == MAC_ADDR_MODE_64)
	{
		memcpy(usermac_fsm.dst_addr, mac_ind->src_addr.ext_addr, sizeof(usermac_fsm.dst_addr));
	}
	else if (mac_ind->src_addr.addr_mode == MAC_ADDR_MODE_16)
	{
		offset += sizeof(mac_ind->src_addr.ext_addr) - sizeof(mac_ind->src_addr.short_addr);
	}
	else
	{
		offset += sizeof(mac_ind->src_addr.ext_addr);
	}

	/* Handle different size depending of destination address mode */
	if (mac_ind->dst_addr.addr_mode == MAC_ADDR_MODE_64)
	{
		/* No action */
	}
	else if (mac_ind->dst_addr.addr_mode == MAC_ADDR_MODE_16)
	{
		offset += sizeof(mac_ind->dst_addr.ext_addr) - sizeof(mac_ind->dst_addr.short_addr);
	}
	else
	{
		offset += sizeof(mac_ind->dst_addr.ext_addr);
	}

	/* Extract MSDU length */
	usermac_fsm.last_ind.msdu_len =  VAR_SIZE_PAYLOAD_OFFSET(mac_ind->msdu_len,   offset    );
	usermac_fsm.last_ind.msdu_len += VAR_SIZE_PAYLOAD_OFFSET(mac_ind->msdu_len,   (offset-1)) << 8;

	/* Extract MSDU */
	if ((usermac_fsm.last_ind.msdu_len > 0) && (usermac_fsm.last_ind.msdu_len <= sizeof(usermac_fsm.last_ind.msdu)))
	{
		memcpy(usermac_fsm.last_ind.msdu, mac_ind->msdu - offset, usermac_fsm.last_ind.msdu_len);
	}

	offset += sizeof(mac_ind->msdu) - usermac_fsm.last_ind.msdu_len;

	/* Update test message number */
	usermac_ping_msg_t* ping_recv = (usermac_ping_msg_t*) usermac_fsm.last_ind.msdu;
	usermac_fsm.test_msg_n	= ping_recv->msg_n;

	/* Discards accidental retransmissions */
	if (ping_recv->msg_i == (usermac_fsm.ind_counter + 1))
	{
		usermac_fsm.received_ind = true;

		usermac_fsm.curr_event = USER_MAC_EV_RECEIVED_DATA_IND;

		/* Increments indication counter */
		usermac_fsm.ind_counter++;

		/* Extract security fields */
		usermac_fsm.last_ind.security_level = VAR_SIZE_PAYLOAD_OFFSET(mac_ind->security_level, offset);

		if (usermac_fsm.last_ind.security_level == MAC_SECURITY_LEVEL_5_ENCMIC32)
		{
			usermac_fsm.last_ind.key_index = VAR_SIZE_PAYLOAD_OFFSET(mac_ind->key_index, offset);

			/* offset does not need to be changed */
		}
		else
		{
			usermac_fsm.last_ind.key_index = MAC_NO_KEY;

			offset += sizeof(mac_ind->key_index); /* No key index field */
		}

		/* Extract QoS/LQI fields */
		usermac_fsm.last_ind.qos = VAR_SIZE_PAYLOAD_OFFSET(mac_ind->qos, offset);
		usermac_fsm.last_ind.lqi = VAR_SIZE_PAYLOAD_OFFSET(mac_ind->phy_params.lqi, offset);

		/* Extract media type */
		uint8_t ind_media_type = VAR_SIZE_PAYLOAD_OFFSET(mac_ind->media_type, offset);

		if (ind_media_type == MAC_MEDIATYPE_IND_PLC)
		{
			usermac_fsm.last_ind.media_type = MAC_MEDIATYPE_PLC;
		}
		else
		{
			usermac_fsm.last_ind.media_type = MAC_MEDIATYPE_RF;
		}

		/* Extract and process PHY parameters */
		if (usermac_fsm.last_ind.media_type == MAC_MEDIATYPE_PLC)
		{
			uint32_t carrier_n;

			float signal_tot_square = 0;
			float signal_tot  = 0;
			float signal_v, signal_dbuv;

			float noise_tot_square = 0;
			float noise_tot = 0;
			float noise_v, noise_dbuv;

			float snr_db;

			if (working_plc_band == BOOT_BANDPLAN_CENELEC_A)
			{
				carrier_n = PHY_NUM_OF_CARRIERS_CENA;
			}
			else if (working_plc_band == BOOT_BANDPLAN_FCC)
			{
				carrier_n = PHY_NUM_OF_CARRIERS_FCC;
			}
			else
			{
				carrier_n = PHY_NUM_OF_CARRIERS_CENB;
			}

			uint8_t signal[PHY_NUM_OF_CARRIERS];
			uint8_t noise[PHY_NUM_OF_CARRIERS];

			memset(signal, 0, sizeof(signal));
			memset(noise, 0, sizeof(noise));

			memcpy(signal, VAR_SIZE_POINTER_OFFSET(mac_ind->phy_params.signal[0], offset), carrier_n);
			memcpy(noise, VAR_SIZE_POINTER_OFFSET(mac_ind->phy_params.noise[0], offset), carrier_n);

			/* Calculate signal in V, then calculate its average */
			for (uint32_t i = 0; i < carrier_n; i++)
			{
				signal_v = MAC_DBUV_TO_V(signal[i]);

				signal_tot_square += (signal_v * signal_v);

				noise_v = MAC_DBUV_TO_V(noise[i]);

				noise_tot_square += (noise_v * noise_v);
			}

			signal_tot = sqrt(signal_tot_square);

			noise_tot = sqrt(noise_tot_square);

			/* then re-convert to dBuV */
			signal_dbuv = MAC_V_TO_DBUV(signal_tot);
			noise_dbuv  = MAC_V_TO_DBUV(noise_tot);

			/* Conversion from peak value to average value (takes into account that carriers with different phases can be destructive) */
			signal_dbuv -= V_PEAK_TO_AVG_POWER_RATION;
			noise_dbuv -= V_PEAK_TO_AVG_POWER_RATION;

			snr_db = (((float) usermac_fsm.last_ind.lqi) * MAC_LQI_TO_SNR_STEP_PLC) - MAC_LQI_TO_SNR_OFFSET_PLC;

			PRINT("Received %u/%u %s (PLC): LQI = %u (SNR = %.2f dB, signal = %.2f dBuV, noise = %.2f dBuV)\n", usermac_fsm.ind_counter, usermac_fsm.test_msg_n, translateG3cmd(HIF_MCPS_DATA_IND), usermac_fsm.last_ind.lqi, snr_db, signal_dbuv, noise_dbuv);
		}
		else
		{
			if (usermac_fsm.last_ind.lqi != MAC_NOTMEASURED_LQI_RF)
			{
				int8_t rssi = usermac_fsm.last_ind.lqi - MAC_LQI_TO_RSSI_OFFSET_RF;

				PRINT("Received %u/%u %s (RF): LQI = %u (RSSI = %d dBm)\n", usermac_fsm.ind_counter, usermac_fsm.test_msg_n, translateG3cmd(HIF_MCPS_DATA_IND), usermac_fsm.last_ind.lqi, rssi);
			}
			else
			{
				PRINT_COLOR("Received %u/%u %s (RF): LQI not measured\n", color_yellow, usermac_fsm.ind_counter, usermac_fsm.test_msg_n, translateG3cmd(HIF_MCPS_DATA_IND));
			}
		}
	}
	else
	{
		PRINT_COLOR("Received retransmitted message (ID: %u instead of %u)\n", color_yellow, ping_recv->msg_i, (usermac_fsm.ind_counter + 1));
	}
}

/**
  * @}
  */

/** @addtogroup User_MAC_Exported_Code
  * @{
  */

/* Public functions */

/**
  * @brief Function that initializes the User MAC variables.
  * @param None
  * @retval None
  */
void UserMac_Init(void)
{
#if ENABLE_MODBUS && ENABLE_MODBUS_TEST
	UserModbus_init(modbus_role);
#endif
	usermac_reset_fsm(mac_ready);

	usermac_fsm.operation_counter = 0;
}

/**
  * @brief Function that checks if a message is needed by the User MAC application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed, 'false' otherwise.
  */
bool UserMac_MsgNeeded(const g3_msg_t *g3_msg)
{
	switch(g3_msg->command_id)
	{
	case HIF_MCPS_DATA_CNF:
	case HIF_MCPS_DATA_IND:
		return true;
		break;
	default:
		return false;
		break;
	}
}

/**
  * @brief Function that handles the messages coming from the G3 task.
  * @param g3_msg Pointer to the received G3 message.
  * @retval None
  */
void UserMac_MsgHandler(const g3_msg_t *g3_msg)
{
	assert(g3_msg->payload != NULL); /* All expected messages have payload */

    switch (g3_msg->command_id)
    {
	case HIF_MCPS_DATA_CNF:
		usermac_handle_mac_data_cnf(g3_msg->payload);
		break;
    case HIF_MCPS_DATA_IND:
    	usermac_handle_mac_data_ind(g3_msg->payload);
        break;
    default:
        break;
    }
}

/**
  * @brief Function that handles the state of the User MAC FSM.
  * @param None
  * @retval None
  * @note Entry function called from User task infinite execution loop.
  */
void UserMac_FsmManager()
{
	usermac_fsm.operation_counter++;

	usermac_fsm.curr_state = usermac_fsm_func_tbl[usermac_fsm.curr_state][usermac_fsm.curr_event]();
}

/**
  * @brief Function that returns the readiness of the MAC test
  * @param None
  * @retval True if the MAC test is ready, flase otherwise
  */
bool UserMac_IsReady()
{
	return (usermac_fsm.curr_state == USER_MAC_ST_READY);
}

/**
  * @brief Callback function of the push button release. Starts the MAC frame transmission.
  * @param dst_addr Destination extended address for the test. Use NULL for autod-detection (works only with 2 devices).
  * @param test_type Indicates the test type (see the enum values)
  * @param msg_number Number of messages exchanged for each test
  * @retval None
  */
void UserMac_StartTxCallback(uint8_t *dst_addr, usermac_test_type_t test_type, uint32_t msg_number)
{
	if (usermac_fsm.curr_state == USER_MAC_ST_READY)
	{
		usermac_reset_fsm(mac_ready);

		usermac_fsm.curr_event = USER_MAC_EV_START_TX;

		if (dst_addr != NULL)
		{
			utils_reverse_array(dst_addr, MAC_ADDR64_SIZE);
			memcpy(usermac_fsm.dst_addr, dst_addr, MAC_ADDR64_SIZE);
		}
		else
		{
			uint8_t broadcast_addr[MAC_ADDR64_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

			memcpy(usermac_fsm.dst_addr, broadcast_addr, MAC_ADDR64_SIZE);
		}

		usermac_fsm.test_type  = test_type;

		if (usermac_fsm.test_type == mac_test_plc_rf)
		{
			usermac_fsm.test_msg_n = 2 * msg_number;
		}
		else
		{
			usermac_fsm.test_msg_n = msg_number;
		}

		if (user_task_operation_counter == usermac_fsm.operation_counter)
		{
			/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
	}
}

/**
  * @brief Callback function of the userMacTimeoutTimer FreeRTOStimer. Warns the user about the timeout event and unblocks the User Task.
  * @param argument Unused argument.
  * @retval None
  */
void UserMac_TimeoutCallback(void *argument)
{
	UNUSED(argument);

	usermac_fsm.curr_event = USER_MAC_EV_TIMEOUT;

	PRINT("MAC test timeout\n\r");

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
