/**
 ******************************************************************************
 * @file    user_image_transfer.c
 * @author  AMG/IPC Application Team
 * @brief   Implementation that handles the User Image Transfer.
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
#include <string.h>
#include <assert.h>
#include <utils.h>
#include <main.h>
#include <crc.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <image_management.h>
#include <g3_app_config.h>
#include <user_g3_common.h>
#include <user_image_transfer.h>

/** @addtogroup User_App
 * @{
 */

/** @addtogroup User_Img_Transfer
 * @{
 */

/** @addtogroup User_Img_Transfer_Private_Code
 * @{
 */

#if ENABLE_IMAGE_TRANSFER

/* Definitions */
#if DEBUG_USER_IT
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_USER_IT_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

#define TRANSFER_IMAGE_INFO_HEAD					0xA5A5A5A5
#define TRANSFER_IMAGE_INFO_END						0x5A5A5A5A

#define TRANSFER_IMAGE_DATA_SIZE					(1024)
#define TRANSFER_IMAGE_DATA_MSG_SIZE(size)			(sizeof(image_data_t) - TRANSFER_IMAGE_DATA_SIZE + size)
#define TRANSFER_IMAGE_DATA_PREVIEW_SIZE			8

#define TRANSFER_PERCENTAGE_UPDATE					(10)
#define TRANSFER_ERROR_N_MAX						(3)

#define TRANSFER_UPDATE_MARK(current_size, total_size)		((current_size != 0) && ((current_size/TRANSFER_IMAGE_DATA_SIZE) % ((total_size/TRANSFER_IMAGE_DATA_SIZE) / TRANSFER_PERCENTAGE_UPDATE) == 0))
#define TRANSFER_SHOW_PROGRESS(current_size, total_size)	((current_size == total_size) || TRANSFER_UPDATE_MARK(current_size, total_size))

#if TRANSFER_PERCENTAGE_UPDATE==0
#error "TRANSFER_PERCENTAGE_UPDATE must be > 0"
#endif /* TRANSFER_PERCENTAGE_UPDATE==0 */

#define CONVERT_B_PER_MS_TO_KB_PER_S(bpms)			(((bpms) * 1000) / KB_SIZE)

/* Timing */
#define TRANSFER_INFO_TIMEOUT                		(120000U)	/* In ms */
#define TRANSFER_DATA_TIMEOUT                		(90000U)	/* In ms */

/* Private structure */

/* Custom types */

typedef enum user_image_transfer_ack_enum
{
	TRANSFER_IMAGE_ACK		 = 1,
	TRANSFER_IMAGE_NACK		 = 2
} user_image_transfer_ack_t;

typedef enum user_image_transfer_event_enum
{
	USER_IMG_TRANSFER_EV_NONE = 0,
	USER_IMG_TRANSFER_EV_START,
	USER_IMG_TRANSFER_EV_UDP_CNF,
	USER_IMG_TRANSFER_EV_UDP_IND,
	USER_IMG_TRANSFER_EV_TIMEOUT,
	USER_IMG_TRANSFER_EV_STOP,
	USER_IMG_TRANSFER_EV_CNT
} user_img_transfer_event_t;

typedef enum user_image_transfer_state_enum
{
	USER_IMG_TRANSFER_ST_READY,
	USER_IMG_TRANSFER_ST_SHARE_INFO,
	USER_IMG_TRANSFER_ST_IN_PROGRESS,
	USER_IMG_TRANSFER_ST_CNT
} user_img_transfer_state_t;

typedef struct user_img_transfer_fsm_str
{
	/* FSM */
	user_img_transfer_state_t			curr_state;	/*!<  Current state of the image transfer */
	user_img_transfer_event_t			curr_event;	/*!<  Current event of the image transfer */
	user_image_transfer_error_code_t	error_code;	/*!<  Error code of the image transfer */

	uint8_t handle;

	uint32_t operation_counter;

	/* SFLASH */
	int8_t		image_slot;

	/* Image information */
	uint32_t	image_type;
	uint32_t	image_validity;
	uint32_t	image_size;
	uint16_t	image_crc16;

	/* Transfer details */
#if IS_COORD
	uint16_t 	dest_short_addr;
#endif
	uint32_t	transfered_bytes;
	uint32_t	last_block_size;
	uint32_t	retry_count;
	bool		transfer_complete;
	uint32_t	last_timestamp;
	uint32_t	last_transfered_bytes;
} user_img_transfer_fsm_t;

/* Message structures used for the transfer */
#pragma pack(push, 1)

typedef struct image_info_str
{
	uint32_t head;
	uint32_t validity;
	uint32_t type;
	uint32_t size;
	uint16_t crc16;
	uint32_t foot;
} image_info_t;

typedef struct image_data_str
{
	uint32_t offset;
	uint32_t size;
	uint8_t  data[TRANSFER_IMAGE_DATA_SIZE];
} image_data_t;

typedef struct image_ack_str
{
	uint8_t  ack;
	uint32_t next_offset;
	uint32_t next_size;
} image_ack_t;

#pragma pack(pop)

/* External Variables */
extern uint32_t 			user_task_operation_counter;

extern uint8_t      		udp_handle;         /*!<  Number of sent messages */

extern osMessageQueueId_t	user_queueHandle;

extern osTimerId_t 			transferTimerHandle;

/* Private variables */
static user_img_transfer_fsm_t		user_img_transfer_fsm;

/* Private function pointer type */
typedef user_img_transfer_state_t user_img_transfer_fsm_func(void);

/* Private FSM function prototypes */
static user_img_transfer_state_t user_img_transfer_fsm_default(void);
static user_img_transfer_state_t user_img_transfer_fsm_timeout(void);
static user_img_transfer_state_t user_img_transfer_fsm_abort(void);

#if IS_COORD
static user_img_transfer_state_t user_img_transfer_fsm_send_info(void);
static user_img_transfer_state_t user_img_transfer_fsm_parse_info_ack();
static user_img_transfer_state_t user_img_transfer_fsm_parse_ack(void);
#else
static user_img_transfer_state_t user_img_transfer_fsm_erase_flash(void);
static user_img_transfer_state_t user_img_transfer_fsm_parse_info(void);
static user_img_transfer_state_t user_img_transfer_fsm_parse_block(void);
static user_img_transfer_state_t user_img_transfer_fsm_ack_sent(void);
#endif

/* Private FSM function pointer array */
#if IS_COORD
static user_img_transfer_fsm_func *user_img_transfer_fsm_func_tbl[USER_IMG_TRANSFER_ST_CNT][USER_IMG_TRANSFER_EV_CNT] = {
/*                   NONE,              			START,           			     UDP_CNF,           			UDP_IND						          TIMEOUT,                       STOP */
/* READY 	     */	{user_img_transfer_fsm_default, user_img_transfer_fsm_send_info, user_img_transfer_fsm_default, user_img_transfer_fsm_default,        user_img_transfer_fsm_default, user_img_transfer_fsm_default  },
/* SHARE_INFO    */	{user_img_transfer_fsm_default, user_img_transfer_fsm_default,   user_img_transfer_fsm_default, user_img_transfer_fsm_parse_info_ack, user_img_transfer_fsm_timeout, user_img_transfer_fsm_abort    },
/* IN_PROGRESS   */	{user_img_transfer_fsm_default, user_img_transfer_fsm_default,   user_img_transfer_fsm_default, user_img_transfer_fsm_parse_ack,      user_img_transfer_fsm_timeout, user_img_transfer_fsm_abort    }
};
#else
static user_img_transfer_fsm_func *user_img_transfer_fsm_func_tbl[USER_IMG_TRANSFER_ST_CNT][USER_IMG_TRANSFER_EV_CNT] = {
/*                 NONE,		                  START,           					 UDP_CNF						 UDP_IND,           				TIMEOUT,					   STOP */
/* READY 	   */ {user_img_transfer_fsm_default, user_img_transfer_fsm_erase_flash, user_img_transfer_fsm_default,	 user_img_transfer_fsm_default,	    user_img_transfer_fsm_default, user_img_transfer_fsm_default },
/* SHARE_INFO  */ {user_img_transfer_fsm_default, user_img_transfer_fsm_default,     user_img_transfer_fsm_default,	 user_img_transfer_fsm_parse_info,	user_img_transfer_fsm_timeout, user_img_transfer_fsm_abort   },
/* IN_PROGRESS */ {user_img_transfer_fsm_default, user_img_transfer_fsm_default,	 user_img_transfer_fsm_ack_sent, user_img_transfer_fsm_parse_block, user_img_transfer_fsm_timeout, user_img_transfer_fsm_abort   }
};
#endif

/**
 * @brief Starts counting for the timeout of a transfer operation.
 * @param timeout The timeout value for the operation.
 * @retval none.
 */
static void user_img_transfer_set_timeout(uint32_t timeout)
{
	osStatus_t os_result = osTimerStart(transferTimerHandle, timeout);

	if (os_result != osOK)
	{
		PRINT_USER_IT_WARNING("Timer could not be started\n\r");
	}
}

/**
 * @brief Stops counting for the timeout of a transfer operation.
 * @param None
 * @retval None
 */
static void user_img_transfer_remove_timeout(void)
{
	osTimerStop(transferTimerHandle);
}

/**
 * @brief Function that ends the image transfer.
 * @param error_code Error code of the transfer (0: success)
 * @retval None
 */
static void user_img_transfer_end(user_image_transfer_error_code_t error_code)
{
	user_img_transfer_fsm.transfer_complete = true;
	user_img_transfer_fsm.error_code = error_code;

	LED_SET_OFF(LED_TRANSFER);

	RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
}

/**
 * @brief Function that updates the progress of the image transfer.
 * @param None
 * @retval None
 */
static void user_img_transfer_update_progress(void)
{
	if (user_img_transfer_fsm.transfered_bytes == 0)
	{
		user_img_transfer_fsm.last_timestamp = HAL_GetTick();
		user_img_transfer_fsm.last_transfered_bytes = 0;
	}
	else if (TRANSFER_SHOW_PROGRESS(user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size))
	{
		uint32_t progress = (100 * user_img_transfer_fsm.transfered_bytes) / user_img_transfer_fsm.image_size;

		uint32_t timestamp = HAL_GetTick();
		float delta_time = timestamp - user_img_transfer_fsm.last_timestamp;
		float delta_bytes = user_img_transfer_fsm.transfered_bytes - user_img_transfer_fsm.last_transfered_bytes;

		float transfer_speed = CONVERT_B_PER_MS_TO_KB_PER_S(delta_bytes / delta_time);

#if IS_COORD
		PRINT("Sent %u/%u bytes - %u%% - %.2f kB/s\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size, progress, transfer_speed);
#else
		PRINT("%u/%u bytes received - %u%% - %.2f kB/s\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size, progress, transfer_speed);
#endif
		user_img_transfer_fsm.last_transfered_bytes = user_img_transfer_fsm.transfered_bytes;
		user_img_transfer_fsm.last_timestamp = timestamp;
	}
}

#if IS_COORD

/**
 * @brief Load memory content into a slot.
 * @param slot_ptr Pointer to the slot info structure.
 * @param slot_index Index of the SPI FLASH memory slot
 * @retval None
 */
static void user_img_transfer_load_image_info(slot_info_t *slot_ptr, int8_t slot_index)
{
	assert_param(slot_ptr != NULL);

	if ((slot_index >= 0) && (slot_index < IMAGE_SLOTS_NUM))
	{
		slot_ptr->address = SFLASH_SLOT(slot_index);

		slot_ptr->type = getImageType(slot_ptr->address);

		if (slot_ptr->type != IMAGE_NOT_FOUND)
		{
			slot_ptr->free = false;

			slot_ptr->validity = getImageValidity(slot_ptr->address);

			if (IMG_VALIDITY_NOT_CORRUPTED(slot_ptr->validity))
			{
				slot_ptr->size = getImageSize(slot_ptr->address);

				if (slot_ptr->size <= IMAGE_SIZE)
				{
					slot_ptr->crc16 = calculateImageCRC(slot_ptr->address, slot_ptr->size);
				}
				else
				{
					downgradeImageValidity(slot_ptr->address);
				}
			}
			else
			{
				downgradeImageValidity(slot_ptr->address);
			}
		}
		else
		{
			slot_ptr->free = true;
		}
	}
	else
	{
		PRINT_USER_IT_CRITICAL("Slot %u does not exist (possible slots: 1-%u)\n", slot_index + 1, IMAGE_SLOTS_NUM + 1);
	}
}

static bool user_img_transfer_send_info(void)
{
	bool success = false;
	slot_info_t	slot_info = { 0 };

	PRINT_USER_IT_INFO("Starting image transmission...\n");

	user_img_transfer_load_image_info(&slot_info, user_img_transfer_fsm.image_slot);

	if ((IMG_VALIDITY_NOT_CORRUPTED(slot_info.validity)) && (slot_info.size > 0) && (!slot_info.free))
	{
		user_img_transfer_fsm.image_type           = slot_info.type; //Unused in tx
		user_img_transfer_fsm.image_validity       = slot_info.validity; //Unused in tx
		user_img_transfer_fsm.image_size           = slot_info.size;
		user_img_transfer_fsm.image_crc16	  	   = slot_info.crc16;

		/* Send G3UDP-DATA.Request with image info */
		image_info_t image_info;

		image_info.head     = TRANSFER_IMAGE_INFO_HEAD;
		image_info.validity = slot_info.validity;
		image_info.type	    = slot_info.type;
		image_info.size	    = slot_info.size;
		image_info.crc16	= slot_info.crc16;
		image_info.foot     = TRANSFER_IMAGE_INFO_END;

		PRINT_USER_IT_INFO("Image type: %s\n",   translateImageType(user_img_transfer_fsm.image_type));
		PRINT_USER_IT_INFO("Image size: %u\n",   user_img_transfer_fsm.image_size);
		PRINT_USER_IT_INFO("Image CRC : 0x%X\n", user_img_transfer_fsm.image_crc16);

		PRINT_USER_IT_INFO("Destination short address: %u\n", user_img_transfer_fsm.dest_short_addr);

		user_img_transfer_fsm.handle = UserG3_SendUdpDataToShortAddress(TRANSFER_CONN_ID, user_img_transfer_fsm.dest_short_addr, &image_info, sizeof(image_info));

		user_img_transfer_set_timeout(TRANSFER_INFO_TIMEOUT); /* Timeout indication */

		success = true;
	}

	return success;
}

/**
 * @brief Functions that reads and sends the next block of data of the image as UDP packet
 * @param offset Memory offset of the data block
 * @param size Size of the data block, in bytes
 * @return None
 */
static void user_img_transfer_send_block(uint32_t offset, uint32_t size)
{
	uint32_t image_data_len = TRANSFER_IMAGE_DATA_MSG_SIZE(size);
	image_data_t *image_data = MEMPOOL_MALLOC(image_data_len);

	image_data->offset = offset;
	image_data->size = size;

	LED_TOGGLE(LED_TRANSFER);

	/* Read flash memory and downloads block to ST8500 */
	bool result = getDataBlock(SFLASH_SLOT(user_img_transfer_fsm.image_slot), image_data->data, image_data->size, image_data->offset);

	if (result)
	{
		user_img_transfer_fsm.transfered_bytes = image_data->offset + image_data->size;

#if (DEBUG_USER_IT >= DEBUG_LEVEL_FULL)
		ALLOC_DYNAMIC_HEX_STRING(block_str, image_data->data, TRANSFER_IMAGE_DATA_PREVIEW_SIZE);
		PRINT_USER_IT_INFO("Sent data: %u/%u, raw data: %s\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size, block_str);
		FREE_DYNAMIC_HEX_STRING(block_str)
#else
		PRINT_USER_IT_INFO("Sent data: %u/%u, raw data: %s\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size);

		user_img_transfer_fsm.last_block_size = image_data->size;
#endif
		/* Send UDP data request with image data */
		user_img_transfer_fsm.handle = UserG3_SendUdpDataToShortAddress(TRANSFER_CONN_ID, user_img_transfer_fsm.dest_short_addr, image_data, image_data_len);

		user_img_transfer_fsm.last_block_size = image_data->size;

		user_img_transfer_set_timeout(TRANSFER_DATA_TIMEOUT); /* Timeout indication */
	}
	else
	{
		Error_Handler();
	}
}

#else

/**
 * @brief Function that sends the image transfer acknowledge to a specific address.
 * @param dest_addr Destination short address of the acknowledge
 * @param ack Type of acknowledge
 * @retval None
 */
static void user_img_transfer_send_ack(uint16_t dest_addr, uint8_t ack)
{
	image_ack_t	 *image_ack = MEMPOOL_MALLOC(sizeof(image_ack_t));
	uint32_t bytes_left = user_img_transfer_fsm.image_size - user_img_transfer_fsm.transfered_bytes;

	/* Result of the last operation */
	image_ack->ack 		  = ack;

	/* Specify next block offset */
	image_ack->next_offset = user_img_transfer_fsm.transfered_bytes;

	/* Specify next block size */
	if (bytes_left >= TRANSFER_IMAGE_DATA_SIZE)
	{
		image_ack->next_size = TRANSFER_IMAGE_DATA_SIZE; /* Standard block size */
	}
	else
	{
		image_ack->next_size = bytes_left; /* Last block size */
	}

	user_img_transfer_fsm.handle = UserG3_SendUdpDataToShortAddress(TRANSFER_CONN_ID, dest_addr, image_ack, sizeof(image_ack_t));
}

#endif /*COORD */

/**
 * @brief Function that resets the User Image Transfer FSM.
 * @param None
 * @retval None
 */
static void user_img_transfer_reset_state()
{
	user_img_transfer_fsm.curr_state = USER_IMG_TRANSFER_ST_READY;
	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;
	user_img_transfer_fsm.error_code = 0;

	/* SFLASH */
	user_img_transfer_fsm.image_slot = 0;

	/* Image information */
	user_img_transfer_fsm.image_type = 0;
	user_img_transfer_fsm.image_validity = 0;
	user_img_transfer_fsm.image_size = 0;
	user_img_transfer_fsm.image_crc16 = 0;

	/* Transfer details */
#if IS_COORD
	user_img_transfer_fsm.dest_short_addr = 0;
#endif
	user_img_transfer_fsm.transfered_bytes = 0;
	user_img_transfer_fsm.last_block_size = 0;
	user_img_transfer_fsm.retry_count = 0;
	user_img_transfer_fsm.transfer_complete = 0;
	user_img_transfer_fsm.last_timestamp = 0;
	user_img_transfer_fsm.last_transfered_bytes= 0;
}

/**
 * @brief Function that maintains the current state, with no further action.
 * @return The next state of the User Image Transfer FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_default(void)
{
	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return user_img_transfer_fsm.curr_state;
}

/**
 * @brief Function that handles the timeout during the User Image Transfer, including retransmission
 * @return The next state of the User Image Transfer FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_timeout(void)
{
	user_img_transfer_state_t next_state = USER_IMG_TRANSFER_ST_IN_PROGRESS;

	PRINT_USER_IT_WARNING("Timeout on Image Transfer\n");

	if (	(user_img_transfer_fsm.curr_state == USER_IMG_TRANSFER_ST_SHARE_INFO) ||
			(user_img_transfer_fsm.retry_count >= TRANSFER_ERROR_N_MAX			) )
	{
		/* Finished, failing */
		PRINT_USER_IT_CRITICAL("Transfer failed\n");

		next_state = USER_IMG_TRANSFER_ST_READY;

		user_img_transfer_end(uit_info_timeout_error);
	}
	else
	{
		user_img_transfer_fsm.retry_count++;
#if IS_COORD
		user_img_transfer_send_block(user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.last_block_size);

		user_img_transfer_update_progress();
#else
		user_img_transfer_set_timeout(TRANSFER_DATA_TIMEOUT); /* Timeout indication */
#endif
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

/**
 * @brief Function that handles the abort during the User Image Transfer
 * @return The next state of the User Image Transfer FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_abort(void)
{
	/* Finished, failing */
	PRINT_USER_IT_WARNING("Transfer aborted\n");

	user_img_transfer_end(uit_aborted);

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return USER_IMG_TRANSFER_ST_READY;
}

#if IS_COORD

/**
 * @brief Function that handles the start-up of the User Image Transfer.
 * @param None
 * @return The next state of the FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_send_info(void)
{
	user_img_transfer_state_t next_state;

	if (user_img_transfer_send_info())
	{
		next_state = USER_IMG_TRANSFER_ST_SHARE_INFO;
	}
	else
	{
		next_state = USER_IMG_TRANSFER_ST_READY;
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

/**
 * @brief User Image Transfer FSM function that checks the ACK received for the image info and sends the first block.
 * @note
 * @return The next state of the User Image Transfer FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_parse_info_ack(void)
{
	user_img_transfer_state_t next_state = user_img_transfer_fsm.curr_state;

	/* Gets UDP packet received, containing the image ACK */
	udp_packet_t udp_packet_rx = UserG3_GetUdpData(TRANSFER_CONN_ID);

	if (udp_packet_rx.payload != NULL)
	{
		/* Reset timer */
		user_img_transfer_remove_timeout();

		assert(udp_packet_rx.length == sizeof(image_ack_t));

		image_ack_t* image_ack = udp_packet_rx.payload;

		if (image_ack->ack == TRANSFER_IMAGE_ACK)
		{
			PRINT_USER_IT_INFO("Info ACK received\n");

			user_img_transfer_fsm.retry_count = 0;

			user_img_transfer_update_progress();

			user_img_transfer_send_block(image_ack->next_offset, image_ack->next_size);

			next_state = USER_IMG_TRANSFER_ST_IN_PROGRESS;
		}
		else
		{
			PRINT_USER_IT_WARNING("Info NACK received\n");

			user_img_transfer_fsm.retry_count++;

			if (user_img_transfer_fsm.retry_count >= TRANSFER_ERROR_N_MAX)
			{
				/* Finished, failing */
				PRINT_USER_IT_CRITICAL("Could not start transfer\n");

				user_img_transfer_end(uit_info_timeout_error);

				next_state = USER_IMG_TRANSFER_ST_READY;
			}
			else
			{
				user_img_transfer_send_info();
			}
		}

		UserG3_DiscardUdpData(TRANSFER_CONN_ID);
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

/* @brief User Image Transfer FSM function that checks the ACK received and sends the next block.
 * @note
 * @return The next state of the User Image Transfer FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_parse_ack(void)
{
	user_img_transfer_state_t next_state = user_img_transfer_fsm.curr_state;

	/* Gets UDP packet received, containing the image ACK */
	udp_packet_t udp_packet_rx = UserG3_GetUdpData(TRANSFER_CONN_ID); // Sistema!

	if (udp_packet_rx.payload != NULL)
	{
		/* Reset timer */
		user_img_transfer_remove_timeout();

		assert(udp_packet_rx.length == sizeof(image_ack_t));

		image_ack_t *image_ack = udp_packet_rx.payload;

		/* Handle errors */
		if (image_ack->ack == TRANSFER_IMAGE_NACK)
		{
			PRINT_USER_IT_WARNING("NACK received\n");
			user_img_transfer_fsm.retry_count++;
			user_img_transfer_fsm.transfered_bytes = image_ack->next_offset;
		}
		else if (image_ack->next_offset != user_img_transfer_fsm.transfered_bytes)
		{
			PRINT_USER_IT_WARNING("Wrong next offset, got %u instead of %u\n", image_ack->next_offset, user_img_transfer_fsm.transfered_bytes);
			user_img_transfer_fsm.retry_count++;
			user_img_transfer_fsm.transfered_bytes = image_ack->next_offset;
		}
		else
		{
			user_img_transfer_fsm.retry_count = 0;
		}

		assert(user_img_transfer_fsm.transfered_bytes <= user_img_transfer_fsm.image_size);

		if (user_img_transfer_fsm.retry_count >= TRANSFER_ERROR_N_MAX)
		{
			/* Finished, failing */
			PRINT_USER_IT_CRITICAL("Transfer failed\n");

			next_state = USER_IMG_TRANSFER_ST_READY;

			user_img_transfer_end(uit_data_timeout_error);
		}
		else if (user_img_transfer_fsm.transfered_bytes == user_img_transfer_fsm.image_size)
		{
			/* Finished */
			PRINT_USER_IT_INFO("Transfer completed (CRC16: 0x%X)\n", user_img_transfer_fsm.image_crc16);

			next_state = USER_IMG_TRANSFER_ST_READY;

			user_img_transfer_end(uit_no_error);
		}
		else
		{
			user_img_transfer_send_block(image_ack->next_offset, image_ack->next_size);

			user_img_transfer_update_progress();
		}

		UserG3_DiscardUdpData(TRANSFER_CONN_ID);
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

#else /* IS_COORD */

/**
 * @brief User Image Transfer function that erase flash and initialize the Image reception
 * @note device FSM Function
 * @param none.
 * @return img_transfer_coord_state_t next state of the FSM.
 */

static user_img_transfer_state_t user_img_transfer_fsm_erase_flash(void)
{
	PRINT_USER_IT_INFO("Erasing slot...\n");

	prepareImageSlot(SFLASH_SLOT(user_img_transfer_fsm.image_slot));

	PRINT_USER_IT_INFO("Starting image reception...\n");

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return USER_IMG_TRANSFER_ST_SHARE_INFO;
}

/**
 * @brief User Image Transfer function that wait for the Image info reception
 * @note device FSM Function
 * @param none.
 * @return img_transfer_coord_state_t next state of the FSM.
 */
static user_img_transfer_state_t user_img_transfer_fsm_parse_info(void)
{
	user_img_transfer_state_t next_state = user_img_transfer_fsm.curr_state;

	/* Gets UDP packet received, containing the image info */
	udp_packet_t udp_packet_rx = UserG3_GetUdpData(TRANSFER_CONN_ID);

	if (udp_packet_rx.payload != NULL)
	{
		uint16_t pan_id, short_addr;

		hi_ipv6_get_saddr_panid(udp_packet_rx.ip_addr, &pan_id, &short_addr);

		PRINT_USER_IT_INFO("Receive image info from short address %u\n", short_addr);

		uint8_t ack = TRANSFER_IMAGE_NACK;

		if (udp_packet_rx.length == sizeof(image_info_t))
		{
			image_info_t *image_info = udp_packet_rx.payload;

			if ((image_info->head == TRANSFER_IMAGE_INFO_HEAD) && (image_info->foot == TRANSFER_IMAGE_INFO_END))
			{
				user_img_transfer_fsm.image_type     		= image_info->type;
				user_img_transfer_fsm.image_validity 		= image_info->validity;
				user_img_transfer_fsm.image_size     		= image_info->size;
				user_img_transfer_fsm.image_crc16    		= image_info->crc16;

				PRINT_USER_IT_INFO("Image type: %s\n",   translateImageType(user_img_transfer_fsm.image_type));
				PRINT_USER_IT_INFO("Image size: %u\n",   user_img_transfer_fsm.image_size);
				PRINT_USER_IT_INFO("Image CRC : 0x%X\n", user_img_transfer_fsm.image_crc16);

				if (user_img_transfer_fsm.image_size <= IMAGE_SIZE)
				{
					ack = TRANSFER_IMAGE_ACK;

					user_img_transfer_update_progress();

					next_state = USER_IMG_TRANSFER_ST_IN_PROGRESS;
				}
				else
				{
					PRINT_USER_IT_WARNING("Incoming image is too big: %u kB (max size: %u kB)\n", user_img_transfer_fsm.image_size/KB_SIZE, IMAGE_SIZE/KB_SIZE);
				}
			}
			else
			{
				PRINT_USER_IT_WARNING("Wrong head or foot for image info message: %08X vs %08X, %08X vs %08X\n", image_info->head, TRANSFER_IMAGE_INFO_HEAD,image_info->foot, TRANSFER_IMAGE_INFO_END);
			}
		}
		else
		{
			PRINT_USER_IT_CRITICAL("Wrong image info length (%u instead of %u)\n", udp_packet_rx.length, sizeof(image_info_t));
		}

		user_img_transfer_send_ack(short_addr, ack);

		UserG3_DiscardUdpData(TRANSFER_CONN_ID);
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

/**
 * @brief User Image Transfer function that read the UDP message block and write it into Flash memory
 * @note device FSM Function
 * @param none.
 * @return img_transfer_coord_state_t next state of the FSM.
 */

static user_img_transfer_state_t user_img_transfer_fsm_parse_block(void)
{
	user_img_transfer_state_t next_state  = user_img_transfer_fsm.curr_state;

	/* Gets UDP packet received, containing the image data */
	udp_packet_t udp_packet_rx = UserG3_GetUdpData(TRANSFER_CONN_ID);

	if (udp_packet_rx.payload != NULL)
	{
		uint8_t ack = TRANSFER_IMAGE_NACK;

		LED_TOGGLE(LED_TRANSFER);

		image_data_t *image_data = udp_packet_rx.payload;

		assert(udp_packet_rx.length == TRANSFER_IMAGE_DATA_MSG_SIZE(image_data->size));

		if (image_data->offset == user_img_transfer_fsm.transfered_bytes)
		{
			bool result = setDataBlock(SFLASH_SLOT(user_img_transfer_fsm.image_slot), image_data->data, image_data->size, image_data->offset);

			if (result)
			{
				user_img_transfer_fsm.retry_count = 0;

				user_img_transfer_fsm.transfered_bytes += image_data->size;

#if (DEBUG_USER_IT >= DEBUG_LEVEL_FULL)
				ALLOC_DYNAMIC_HEX_STRING(block_str, image_data->data, TRANSFER_IMAGE_DATA_PREVIEW_SIZE);
				PRINT_USER_IT_INFO("Received data: %u/%u, raw data: %s\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size, block_str);
				FREE_DYNAMIC_HEX_STRING(block_str)
#else
				PRINT_USER_IT_INFO("Received data: %u/%u\n", user_img_transfer_fsm.transfered_bytes, user_img_transfer_fsm.image_size);
#endif
				user_img_transfer_update_progress();

				ack = TRANSFER_IMAGE_ACK;
			}
			else
			{
				Error_Handler();
			}
		}
		else
		{
			PRINT_USER_IT_CRITICAL("Error, data offset is %u instead of %u\n", image_data->offset, user_img_transfer_fsm.transfered_bytes);

			ack = TRANSFER_IMAGE_NACK;
		}

		UserG3_DiscardUdpData(TRANSFER_CONN_ID);

		uint16_t pan_id, short_addr;

		hi_ipv6_get_saddr_panid(udp_packet_rx.ip_addr, &pan_id, &short_addr);

		user_img_transfer_send_ack(short_addr, ack);
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}

/**
 * @brief User Image Transfer function that
 * @note device FSM Function
 * @param None
 * @return The next state of the FSM
 */
static user_img_transfer_state_t user_img_transfer_fsm_ack_sent(void)
{
	user_img_transfer_state_t next_state = USER_IMG_TRANSFER_ST_IN_PROGRESS;

	if (user_img_transfer_fsm.transfered_bytes < user_img_transfer_fsm.image_size)
	{
		/* More data is coming */
		user_img_transfer_set_timeout(TRANSFER_DATA_TIMEOUT); /* Timeout indication */
	}
	else if (user_img_transfer_fsm.transfered_bytes == user_img_transfer_fsm.image_size)
	{
		user_image_transfer_error_code_t error_code;

		/* If no more data is coming, handles the completion of the transfer */
		user_img_transfer_remove_timeout();

		/* Use image data block as buffer for calculating CRC16 */
		uint16_t crc_calc = calculateImageCRC(SFLASH_SLOT(user_img_transfer_fsm.image_slot), user_img_transfer_fsm.image_size);
		PRINT_USER_IT_INFO("  Transfer completed (CRC16: 0x%X)\n", crc_calc);

		if (crc_calc == user_img_transfer_fsm.image_crc16)
		{
			error_code = uit_no_error;
		}
		else
		{
			error_code = uit_crc_error;
			user_img_transfer_fsm.image_validity = IMG_INVALIDATED;
			PRINT_USER_IT_CRITICAL("  Error, expected CRC16: 0x%04X instead of 0x%04X\n", user_img_transfer_fsm.image_crc16, crc_calc);
		}

		/* Validity isn't considered for CRC16 calculation */
		setImageValidity(SFLASH_SLOT(user_img_transfer_fsm.image_slot), user_img_transfer_fsm.image_validity);

		regradeImagesInMemory(SFLASH_SLOT(user_img_transfer_fsm.image_slot), user_img_transfer_fsm.image_validity);

		next_state = USER_IMG_TRANSFER_ST_READY;

		user_img_transfer_end(error_code);
	}
	else
	{
		/* Must never exceed expected data */
		Error_Handler();
	}

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_NONE;

	return next_state;
}
#endif

/**
 * @brief Function that handles the G3UDP-DATA.Confirm coming from the G3 task.
 * @param payload Pointer to the payload of the received message.
 * @retval None
 */
static void user_img_transfer_handle_udp_data_cnf(const void *payload)
{
	const IP_G3UdpDataConfirm_t *udp_data_cnf = payload;
	assert(udp_data_cnf != NULL);

	if (udp_data_cnf->handle == user_img_transfer_fsm.handle)
	{
		if (udp_data_cnf->status == G3_SUCCESS)
		{
			user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_UDP_CNF;

			HANDLE_CNF_ERROR(HIF_UDP_DATA_CNF, udp_data_cnf->status);
		}
	}
}

/**
 * @brief Function that handles the G3UDP-DATA.Indication coming from the G3 task.
 * @param payload Pointer to the payload of the received message.
 * @retval None
 */
static void user_img_transfer_handle_udp_data_ind(const void *payload)
{
	const IP_DataIndication_t    *udp_ind   = payload;
	assert(udp_ind != NULL);

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_UDP_IND;
}

/**
 * @}
 */

/** @addtogroup User_Img_Transfer_Exported_Code
 * @{
 */

/**
 * @brief Function that handles the initialization of User Image Transfer FSM.
 * @param None
 * @retval None
 */
void UserImgTransfer_Init(void)
{
	user_img_transfer_reset_state();

	user_img_transfer_fsm.operation_counter = 0;
}

/**
 * @brief Function that checks if a message is needed by the User Image Transfer application.
 * @param g3_msg Pointer to the G3 message structure to evaluate
 * @return 'true' if the message is needed, 'false' otherwise.
 */
bool UserImgTransfer_MsgNeeded(const g3_msg_t *g3_msg)
{
	switch(g3_msg->command_id)
	{
	case HIF_UDP_DATA_CNF:
	case HIF_UDP_DATA_IND:
		return true;
		break;
	default:
		return false;
		break;
	}
}


/**
 * @brief Function that handles the G3 messages coming from the G3 task.
 * @param g3_msg Pointer to the received G3 message
 * @retval None
 */
void UserImgTransfer_MsgHandler(const g3_msg_t *g3_msg)
{
	assert(g3_msg->payload != NULL); /* All expected messages have payload */

	switch (g3_msg->command_id)
	{
	case HIF_UDP_DATA_CNF:
		user_img_transfer_handle_udp_data_cnf(g3_msg->payload);
		break;
	case HIF_UDP_DATA_IND :
		user_img_transfer_handle_udp_data_ind(g3_msg->payload);
		break;
	default:
		break;
	}
}

/**
 * @brief Function that handles the state of the User Image Transfer FSM.
 * @param None
 * @retval None
 * @note Entry function called from User task infinite execution loop.
 */
void UserImgTransfer_FsmManager(void)
{
	user_img_transfer_fsm.operation_counter++;

	user_img_transfer_fsm.curr_state = user_img_transfer_fsm_func_tbl[user_img_transfer_fsm.curr_state][user_img_transfer_fsm.curr_event]();
}

#if IS_COORD
/**
 * @brief Function that starts the User Image Transfer FSM.
 * @param image_slot Slot of the image to send
 * @param dest_short_addr Short address of the device that will receive the image
 * @retval None
 */
void UserImgTransfer_StartSend(uint8_t image_slot, uint16_t dest_short_addr)
{
	assert(image_slot < IMAGE_SLOTS_NUM);

	user_img_transfer_reset_state();

	user_img_transfer_fsm.dest_short_addr = dest_short_addr;
	user_img_transfer_fsm.image_slot      = image_slot;

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_START;

	if (user_task_operation_counter == user_img_transfer_fsm.operation_counter)
	{
		/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
	}
}
#else
/**
 * @brief Function that starts the User Image Transfer FSM.
 * @param image_slot Slot where the image will be saved
 * @retval None
 */
void UserImgTransfer_StartReceive(uint8_t image_slot)
{
	assert(image_slot < IMAGE_SLOTS_NUM);

	user_img_transfer_reset_state();

	user_img_transfer_fsm.image_slot = image_slot;

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_START;

	if (user_task_operation_counter == user_img_transfer_fsm.operation_counter)
	{
		/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
	}
}
#endif

/**
 * @brief Function that aborts the User Image Transfer FSM.
 * @param None
 * @retval None
 */
void UserImgTransfer_Stop(void)
{
	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_STOP;

	if (user_task_operation_counter == user_img_transfer_fsm.operation_counter)
	{
		/* If the operation counter of the FSM is equal to the user task operation counter, the user task needs to be executed again */
		RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
	}
}

/**
 * @brief Function that returns the completion state of the image transfer.
 * @param None
 * @retval Boolean that indicates if the image transfer has yet been completed
 */
bool UserImgTransfer_IsComplete(void)
{
	return (user_img_transfer_fsm.transfer_complete);
}

/**
 * @brief Function that returns the error code of the image transfer.
 * @param None
 * @retval Error code of the image transfer
 */
user_image_transfer_error_code_t UserImgTransfer_GetError(void)
{
	return user_img_transfer_fsm.error_code;
}

/**
 * @brief Callback function of the transferTimer timer.
 * @param argument Unused argument.
 * @retval None
 */
void UserImgTransfer_TimeoutCallback(void *argument)
{
	UNUSED(argument);

	user_img_transfer_fsm.curr_event = USER_IMG_TRANSFER_EV_TIMEOUT;

	RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
}

#endif /* ENABLE_IMAGE_TRANSFER */

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

