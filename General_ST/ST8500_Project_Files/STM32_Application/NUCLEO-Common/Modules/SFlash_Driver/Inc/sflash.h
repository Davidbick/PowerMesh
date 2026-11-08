/**
  *****************************************************************************
  *   @file    sflash.h
  *   @author  AMG/IPC Application Team
  *   @brief   Header file for the communication with the SPI FLASH.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef SFLASH_H_
#define SFLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup sflash_Comm
 * @{
 */

/**
 * @}
 */

/** @defgroup sflash_Comm_Exported_Code Exported Code
 * @{
 */

/**
 * @}
 */

/** @defgroup sflash_Comm_Private_Code Private Code
 * @{
 */

/* Inclusions */
#include <cmsis_os.h>

/* Definitions */
#define SFLASH_SEM_NUM 					2 		/* maximum number of tasks that can access flash memory */

/* SFLASH macros */
#define SFLASH_BULK_ERASE() 					sflash_command(SFLASH_CMD_BULK_ERASE, 0, NULL, 0, SFLASH_ERASE_WRITE_TIMEOUT)
#define SFLASH_ERASE(address, size) 			sflash_command(SFLASH_CMD_ERASE, address, NULL, size, SFLASH_ERASE_WRITE_TIMEOUT)
#define SFLASH_WRITE(address, data, size) 		sflash_command(SFLASH_CMD_WRITE, address, data, size, SFLASH_ERASE_WRITE_TIMEOUT)
#define SFLASH_READ(data, address, size) 		sflash_command(SFLASH_CMD_READ, address, data, size, SFLASH_READ_TIMEOUT)
#define SFLASH_GET_ID(data) 					sflash_command(SFLASH_CMD_GET_ID, 0, data, 0, SFLASH_READ_TIMEOUT)

/* Timing */
#define SFLASH_ERASE_WRITE_TIMEOUT		0		/* No timeout for erase/write operations */
#define SFLASH_READ_TIMEOUT				5000	/* Default timeout for read operations */

/* Custom types */

/* Semaphore assignment structure */
typedef struct sflash_task_semaphore_struct
{
	osThreadId_t    task_id;
	osSemaphoreId_t semaphore;
} sflash_task_semaphore_t;

/* Semaphore assignment structure */
typedef enum sflash_msg_cmd_enum
{
	SFLASH_CMD_BULK_ERASE,
	SFLASH_CMD_ERASE,
	SFLASH_CMD_WRITE,
	SFLASH_CMD_READ,
	SFLASH_CMD_GET_ID,
	SFLASH_CMD_CNT
} sflash_cmd_t;

/* SFLASH message Structure */
typedef struct sflash_msg_str
{
	sflash_cmd_t 	command;
	uint32_t 		address;
	void*			data;
	uint32_t 		size; 		// Using a 32 bits for the len field
	osThreadId_t 	task_id;	// Using an int8_t for the task_id field
} sflash_msg_t;

/* Public Function */
bool sflash_command(sflash_cmd_t sflash_cmd, uint32_t address, void *data, uint32_t size, uint32_t timeout);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* SFLASH_H_ */
