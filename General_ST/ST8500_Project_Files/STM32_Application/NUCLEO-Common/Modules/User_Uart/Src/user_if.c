/**
  ******************************************************************************
  * @file    user_if.c
  * @author  AMG/IPC Application Team
  * @brief   Implementation of the User Interface (ASCII input/output).
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
#include <stdarg.h>
#include <stdio.h>
#include <task_comm.h>
#include <stream_buffer.h>
#include <utils.h>
#include <mem_pool.h>
#include <user_if.h>
#include <settings.h>

/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_If
  * @{
  */

 /** @addtogroup User_If_Private_Code
  * @{
  */

/* Private definitions */

/* @brief Buffer sizes and masks */
#define USERIF_RX_FIFO_NUM			(4U) 						/*!< Number of input elements (beware: power of 2 value expected!) */
#define USERIF_RX_FIFO_NUM_MASK		(USERIF_RX_FIFO_NUM - 1U)	/*!< Mask of the number of user input elements */

#if defined(STM32G474xx) || defined(STM32L476xx)
#define USERIF_TX_FIFO_SIZE			(5120U) 					/*!< Size of the User Interface stream buffer, in bytes */
#else
#define USERIF_TX_FIFO_SIZE			(1024U) 					/*!< Size of the User Interface stream buffer, in bytes */
#endif

/* @brief ASCII values */
#define USERIF_BS_ASCII_CODE		(0x08) /*!< Backspace ('\b') */
#define USERIF_LF_ASCII_CODE		(0x0A) /*!< Line Feed ('\n') */
#define USERIF_CR_ASCII_CODE		(0x0D) /*!< Carriage Return ('\r') */

/* @brief Timeout values */
#define USERIF_PRINT_TIMEOUT		(500) /*!< Timeout value for the User Interface print, in ms */


/* Private types */
typedef struct user_if_fifo_rx_str
{
  uint32_t read_index;		/* Used by UserIf_GetInput */
  uint32_t search_index;	/* Used by UserIf_SearchChar */
  uint32_t write_index;
  user_input_t data[USERIF_RX_FIFO_NUM];
} user_if_fifo_rx_t;

/* External Variables */
extern osMessageQueueId_t	user_queueHandle;

#if ENABLE_TIMESTAMP_MICRO
extern TIM_HandleTypeDef    htimSys; /* Systick Timer */
#endif

/* Private variables */
static user_if_fifo_rx_t 	user_if_fifo_rx;      							/*!< User IF Rx data circular FIFO  */
static uint8_t              hostStreamBufferStorage[USERIF_TX_FIFO_SIZE];	/*!< User IF Tx stream buffer */
static StaticStreamBuffer_t hostStreamBufferCtrl;							/*!< User IF Tx stream buffer control */

static uint8_t 				huartUserIf_data;
/**
  * @}
  */

/** @addtogroup User_If_Exported_Code
  * @{
  */

/* External variables */
extern osMutexId_t		mutexPrintHandle;

extern osSemaphoreId_t	semStartPrintHandle;
extern osSemaphoreId_t	semUserIfTxCompleteHandle;

/* Global variables */
user_input_t			user_input;

StreamBufferHandle_t	printStreamBufferHandle;

/**
  * @}
  */

/** @addtogroup User_If_Private_Code
  * @{
  */

/* Private functions */

/**
  * @brief This function resets the User Interface structure.
  * @retval None
  */
static void user_if_reset_input(void)
{
	user_input.length = 0U;
	memset(&user_input.payload[0], 0, sizeof(user_input.payload));
}

#if ENABLE_TIMESTAMP
/**
  * @brief This function inserts the timestamp in format [d:hh:mm:ss] in a destination string.
  * @param buffer_dst Destination buffer for the string.
  * @param buffer_dst_size Destination buffer received_characters.
  * @retval None.
  */
static void user_if_insert_timestamp(char* buffer_dst, uint32_t buffer_dst_size)
{
#if ENABLE_TIMESTAMP_MICRO
	char buffer_src[18];
#else
	char buffer_src[11];
#endif
    uint32_t cur_timestamp;
	uint16_t seconds, minutes, hours, days;

	cur_timestamp = HAL_GetTick();

#if ENABLE_TIMESTAMP_MICRO
	uint32_t micro_seconds = ((cur_timestamp % 1000) * 1000) + htimSys.Instance->CNT;
#endif

	seconds = cur_timestamp / 1000U;
	minutes = seconds / 60U;
	seconds %= 60U;

	hours = minutes / 60U;
	minutes %= 60U;

	days = (hours / 24U) % 365;
	hours %= 24;

#if ENABLE_TIMESTAMP_MICRO
	sprintf(buffer_src, "%.1u:%.2u:%.2u:%.2u.%.6lu", days, hours, minutes, seconds, micro_seconds);
#else
	sprintf(buffer_src, "%.1u:%.2u:%.2u:%.2u", days, hours, minutes, seconds);
#endif
	strcat(buffer_src, "> ");

	/* Overflow check */
	assert(strlen(buffer_src) <= (buffer_dst_size - strlen(buffer_dst)));

	strncat(buffer_dst, buffer_src, strlen(buffer_src));
}
#endif

/**
  * @brief This function prints out a number of data bytes indicated in its parameter
  * @param data_to_print Pointer on data payload to send
  * @param length Data payload received_characters
  * @retval Length of final string to print
  */
static uint32_t user_if_low_level_print(const char *data_to_print, uint32_t length)
{
	uint32_t lengt_printed = 0;

	/* Check arguments */
	if ((data_to_print != NULL) && (length > 0))
    {
		if (OS_IS_ACTIVE())
		{
			/* During FreeRTOS execution */
			if (osMutexAcquire(mutexPrintHandle, USERIF_PRINT_TIMEOUT) == osOK)
			{
				lengt_printed = xStreamBufferSend(printStreamBufferHandle, data_to_print, length, USERIF_PRINT_TIMEOUT);

				osMutexRelease(mutexPrintHandle);
			}

			osSemaphoreRelease(semStartPrintHandle);
		}
		else
		{
			/* While FreeRTOS has not started yet */
			HAL_UART_Transmit(&huartUserIf, (uint8_t*) data_to_print, length, HAL_MAX_DELAY);
			lengt_printed = length;
		}
    }

    return lengt_printed;
}

/**
  * @}
  */

/** @addtogroup User_If_Exported_Code
  * @{
  */

/* Public functions */
/**
  * @brief This function initializes the User Interface.
  * @retval None
  */
void user_if_init(void)
{
	/* Initialize User Interface Output */
	printStreamBufferHandle = xStreamBufferCreateStatic(sizeof(hostStreamBufferStorage), 1, hostStreamBufferStorage, &hostStreamBufferCtrl);

	assert(printStreamBufferHandle != NULL);

	/* Initialize User Interface Input */
	user_if_init_rx_fifo();
}

/**
  * @brief This function initializes the User Interface RX FIFO.
  * @retval None
  */
void user_if_init_rx_fifo(void)
{
    /* Init Terminal Rx part: */
    user_if_fifo_rx.read_index   = 0U;
    user_if_fifo_rx.search_index = 0U;
    user_if_fifo_rx.write_index  = 0U;
    memset(&user_if_fifo_rx.data[0], 0x0U, sizeof(user_if_fifo_rx.data));
    user_if_reset_input();
}

/**
  * @brief This functions starts interrupt reception on the User Interface.
  * @param None
  * @retval None
  */
inline void user_if_rx_start(void)
{
	/* Starts listening for the first byte */
	HAL_UART_Receive_IT(&huartUserIf, &huartUserIf_data, sizeof(huartUserIf_data));
}

/**
  * @brief This functions stops interrupt reception on the User Interface.
  * @param None
  * @retval None
  */
inline void user_if_rx_stop(void)
{
	/* Stops listening */
	HAL_UART_Abort_IT(&huartUserIf);
}

/**
  * @brief This function handles user input extraction from the User Interface RX FIFO.
  * @retval Pointer on current user input received
  */
user_input_t* user_if_get_input(void)
{
	user_input_t *user_input = NULL;

	if (user_if_fifo_rx.read_index != user_if_fifo_rx.write_index)
	{
		user_input = &user_if_fifo_rx.data[user_if_fifo_rx.read_index];
		user_if_fifo_rx.read_index = ((user_if_fifo_rx.read_index + 1) & USERIF_RX_FIFO_NUM_MASK);
	}

	return user_input;
}

/**
  * @brief This function looks for a specific initial character byte in User Interface RX FIFO payloads.
  * @param byte_char byte data character to look-up.
  * @retval 'true' if the character was found, 'false' otherwise.
  */
bool user_if_search_char(uint8_t byte_char)
{
	bool found = false;

	while (user_if_fifo_rx.search_index != user_if_fifo_rx.write_index)
	{
		if (user_if_fifo_rx.data[user_if_fifo_rx.search_index].payload[0] == byte_char)
		{
			found = true;
		}

		user_if_fifo_rx.search_index = ((user_if_fifo_rx.search_index + 1) & USERIF_RX_FIFO_NUM_MASK);

		if (found)
		{
			break;
		}
	}

	return found;
}

/**
  * @brief This function prints a formatted string, with optional time stamp, label and color
  * @param include_ts If true, includes the time stamp. If false, the time stamp is omitted
  * @param color Color for the entire print (set with an escape sequence), use 'color_default' to avoid inserting the escape sequence related to the color
  * @param label Label string to add before the formatted string
  * @param format String that contains the text to be printed
  * @param ... Additional arguments: variables to be used to replace format specifiers in the 'format' string
  * @retval Length of final string to print
  */
uint32_t user_if_printf(bool include_ts, color_t color, const char *label, const char *format, ...)
{
#if USE_POOL_IN_USER_TERMINAL
	char *buffer_dst = MEMPOOL_MALLOC(USERIF_PRINT_MAX_SIZE);

	assert(buffer_dst != NULL);
#else
	uint8_t buffer_dst[USERIF_PRINT_MAX_SIZE];
#endif
    va_list args;
    uint32_t length         = 0;
    uint32_t length_printed = 0;

	memset(buffer_dst, 0, USERIF_PRINT_MAX_SIZE);

#if ENABLE_TIMESTAMP
	if (include_ts)
	{
		user_if_insert_timestamp(buffer_dst, USERIF_PRINT_MAX_SIZE);

		length = strlen(buffer_dst);
	}
#else
	UNUSED(include_ts);
#endif

#if ENABLE_COLORS
	/* Changes color */
	if (color != color_default)
	{
		char buffer_src[10];

		switch (color)
		{
		case color_red:
			strncpy(buffer_src, COLOR_RED_STR, sizeof(buffer_src));
			break;
		case color_green:
			strncpy(buffer_src, COLOR_GREEN_STR, sizeof(buffer_src));
			break;
		case color_yellow:
			strncpy(buffer_src, COLOR_YELLOW_STR, sizeof(buffer_src));
			break;
		case color_blue:
			strncpy(buffer_src, COLOR_BLUE_STR, sizeof(buffer_src));
			break;
		case color_magenta:
			strncpy(buffer_src, COLOR_MAGENTA_STR, sizeof(buffer_src));
			break;
		case color_cyan:
			strncpy(buffer_src, COLOR_CYAN_STR, sizeof(buffer_src));
			break;
		case color_white:
			strncpy(buffer_src, COLOR_WHITE_STR, sizeof(buffer_src));
			break;
		case color_default:
		default:
			assert(0);
		}

		/* Overflow check */
		assert(strlen(buffer_src) <= (USERIF_PRINT_MAX_SIZE - length));

		strncat(buffer_dst, buffer_src, strlen(buffer_src));

		length = strlen(buffer_dst);
	}
#else
	UNUSED(color);
#endif

	if (label != NULL)
	{
		/* Overflow check */
		assert(strlen(label) <= (USERIF_PRINT_MAX_SIZE - strlen((char*) buffer_dst)));

		strncat( buffer_dst, label, strlen(label));
		length = strlen(buffer_dst);
	}

	va_start(args, format);
	vsprintf(&buffer_dst[length], format, args);
	va_end(args);

	length = strlen(buffer_dst);

	/* Converts \n -> \r\n */
	if (buffer_dst[length-1U] == '\n')
	{
		buffer_dst[length] = '\r';
		length++;
	}

#if ENABLE_COLORS
	/* Changes color back to normal */
	if (color != color_default)
	{
		char buffer_src[10];

		strncpy(buffer_src, COLOR_DEFAULT_STR, sizeof(buffer_src));

		/* Overflow check */
		assert(strlen(buffer_src) <= (USERIF_PRINT_MAX_SIZE - length));

		strncat(buffer_dst, buffer_src, strlen(buffer_src));

		length = strlen(buffer_dst);
	}
#endif

	assert(length <= USERIF_PRINT_MAX_SIZE);

	length_printed = user_if_low_level_print(buffer_dst, length);

#if USE_POOL_IN_USER_TERMINAL
	MEMPOOL_FREE(buffer_dst);
#endif

    return length_printed;
}

/**
  * @brief This function prints a specific ASCII string, without formatting
  * @param string String to print
  * @param length Length of the string
  * @retval Length of printed string
  */
uint32_t user_if_print_raw(const char * string, const uint16_t length)
{
	uint32_t length_printed = 0;

	assert(length < USERIF_PRINT_MAX_SIZE-1);

#if USE_POOL_IN_USER_TERMINAL
	char *buffer_dst = MEMPOOL_MALLOC(length + 1);

	assert(buffer_dst != NULL);
#else
	uint8_t buffer_dst[USERIF_PRINT_MAX_SIZE];
#endif

	memcpy(buffer_dst, string, length);

	buffer_dst[length] = 0;

	length_printed = user_if_low_level_print(buffer_dst, length);

#if USE_POOL_IN_USER_TERMINAL
	MEMPOOL_FREE(buffer_dst);
#endif

    return length_printed;
}

/**
  * @brief  This function handles User Interface RX ISR
  * @retval None
  */
void user_if_rx_handler(void)
{
	if (huartUserIf_data != USERIF_BS_ASCII_CODE) /* Character different from backspace */
	{
		if (user_input.length < sizeof(user_input.payload))
		{
			user_input.payload[user_input.length++] = huartUserIf_data;
		}
	}
	else
	{
		if (user_input.length > 0U)
		{
			user_input.payload[--user_input.length] = 0U; /* Deletes previous input byte */
		}
	}

	if (user_input.length > 0U)
	{
		if ((huartUserIf_data == USERIF_CR_ASCII_CODE) || (huartUserIf_data == USERIF_LF_ASCII_CODE)) /* CR or LF character */
		{
			/* Parses input end conditions (CR or LF) */

			/* When input is ready, it is sent to the User Interface RX FIFO */
			memcpy(&(user_if_fifo_rx.data[user_if_fifo_rx.write_index]), &(user_input), sizeof(user_input));
			user_if_fifo_rx.write_index = ((user_if_fifo_rx.write_index+1) & USERIF_RX_FIFO_NUM_MASK);
			user_if_reset_input();

			RTOS_PUT_MSG(user_queueHandle, USER_MSG, NULL);
		}
	}

	/* To enable the next call to HAL_UART_RxCpltCallback for a "frame" of one byte for the HOST UART */
	user_if_rx_start();
}

/**
  * @brief This function handles User Interface TX ISR
  * @retval None
  */
void user_if_tx_handler(void)
{
	/* Unblocks the Print task after the data has been sent on User Interface UART */
	osSemaphoreRelease(semUserIfTxCompleteHandle);
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
