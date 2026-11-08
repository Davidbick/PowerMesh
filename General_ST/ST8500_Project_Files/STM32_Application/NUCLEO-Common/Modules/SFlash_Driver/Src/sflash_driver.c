/**
  ******************************************************************************
  *   @file    sflash_driver.c
  *   @author  AMG/IPC Application Team
  *   @brief   This file contains code that implements the SFLASH driver.
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
#include <stdint.h>
#include <string.h>
#include <cmsis_os.h>
#include <spi.h>
#include <debug_print.h>
#include <mem_pool.h>
#include <utils.h>
#include <main.h>
#include <sflash_info.h>
#include <sflash_driver.h>


/* Definitions */
#define SFLASH_MANU_WINBOND_ID		0xEF			/* Manufacturer ID of Winbond */
#define SFLASH_MANU_MICRON_ID		0x20			/* Manufacturer ID of Micron */

/* SFlash command bytes */
#define SPI_FLASH_CMD_SE            0xD8            /*!< Sector Erase instruction */
#define SPI_FLASH_CMD_RDSR          0x05            /*!< Status Register Read instruction */
#define SPI_FLASH_CMD_MMPP          0x02            /*!< Main Memory Page Program Through Buffer 1 instruction */
#define SPI_FLASH_CMD_MMPR          0x0B            /*!< Read instruction */
#define SPI_FLASH_CMD_WENBL         0x06            /*!< Write Enable instruction */
#define SPI_FLASH_CMD_BE            0xC7            /*!< Sector Bulk instruction */
#define SPI_FLASH_CMD_MDR           0x9F 			/*!< Manufacturer and Device ID Read instruction */

#define SPI_FLASH_STATUS_SRWD_BIT	(0x80)			/* Mask of the 'status register write protect bit' of the status register */
#define SPI_FLASH_STATUS_BP_2_BIT	(0x10)			/* Mask of the 'block protect 2 bit' of the status register */
#define SPI_FLASH_STATUS_BP_1_BIT	(0x08)			/* Mask of the 'block protect 1 bit' of the status register */
#define SPI_FLASH_STATUS_BP_0_BIT	(0x04)			/* Mask of the 'block protect 0 bit'  of the status register */
#define SPI_FLASH_STATUS_WEL_BIT	(0x02)			/* Mask of the 'write enable latch bit' of the status register */
#define SPI_FLASH_STATUS_WIP_BIT	(0x01)			/* Mask of the 'write in progress bit' of the status register */

#define SPI_FLASH_READ_HEADER		5
#define SPI_FLASH_READ_MAX          SPI_FLASH_PAGE_SIZE

#define SPI_FLASH_WRITE_HEADER		4
#define SPI_FLASH_WRITE_MAX         SPI_FLASH_PAGE_SIZE



#define SPI_FLASH_READ_PREVIEW_SIZE	8

#define SPI_TIMEOUT                 1000	/* In ms */
#define SFLASH_TIMEOUT              30000	/* In ms */
#define SPI_WAIT_TIME_NOTICE		5		/* In ms */

#define CHECK_WRITE_ENABLE			1 /* Set to 1 to check write enable */


/* External functions */
extern osSemaphoreId_t semSPIHandle;

/* Private functions */


/**
 * @brief  Handles transaction through the SPI interface.
 * @param  buff_txrx Buffer for transmission/reception.
 * @param  size Number of bytes to transfer.
 * @return Boolean value which says if the operation completed successfully or not.
 */
static bool SFLASH_SPI_Transfer(uint8_t *buff_txrx, uint16_t size)
{
	HAL_StatusTypeDef hal_status;

	/* Handle transfer (Fulll-Duplex), NSS is managed by software */
	HAL_GPIO_WritePin(SFLASH_CS_N_GPIO_Port, SFLASH_CS_N_Pin, GPIO_PIN_RESET);

	/* Uses interrupt transfer when FreeRTOS is running */
	if (osKernelGetState() == osKernelRunning)
	{
		/* Interrupt transfer with task blocking semaphore*/
		hal_status = HAL_SPI_TransmitReceive_DMA(&hspiSFlash, buff_txrx, buff_txrx, size);

		if (hal_status == HAL_OK)
		{
			if (osSemaphoreAcquire(semSPIHandle, SPI_TIMEOUT) != osOK)
			{
				hal_status = HAL_TIMEOUT;
			}
		}
	}
	else
	{
		/* Polling transfer */
		hal_status = HAL_SPI_TransmitReceive(&hspiSFlash, buff_txrx, buff_txrx, size, SPI_TIMEOUT);
	}

	/* Deactivate Chip Select (CS) */
	HAL_GPIO_WritePin(SFLASH_CS_N_GPIO_Port, SFLASH_CS_N_Pin, GPIO_PIN_SET);

	if (hal_status != HAL_OK)
	{
		PRINT_SFLASH_CRITICAL("Error in SPI transfer: %u\n", hal_status);
	}

	return (hal_status == HAL_OK);
}
/**
 * @brief  Get status register of the SPI Flash
 * @param  status_ptr Pointer to the variable where the SPI Flash status is stored
 * @return Boolean value which says if the operation completed successfully or not
 */
static bool SFLASH_GetStatusReg(uint8_t *status_ptr)
{
	bool success;

	uint8_t buffer_txrx[2];

	memset(buffer_txrx, 0, sizeof(buffer_txrx));

	buffer_txrx[0] = SPI_FLASH_CMD_RDSR;
	buffer_txrx[1] = 0; /* Dummy */

	success = SFLASH_SPI_Transfer(buffer_txrx, sizeof(buffer_txrx));

	if (success)
	{
		*status_ptr = (buffer_txrx[1]);
	}

	return success;
}

/**
 * @brief  Waits for the flash memory to be ready.
 * @param  None
 * @return Boolean value which says if the memory is ready or not.
 */
static bool SFLASH_WaitReady(void)
{
	bool     success;
	uint8_t  status;
	uint32_t tickstart = HAL_GetTick();
	uint32_t elapsed_time;

	for(;;)
	{
		success = SFLASH_GetStatusReg(&status);
		elapsed_time = HAL_GetTick() - tickstart;

		if (success)
		{
			if (MASK_IS_CLEAR(status, SPI_FLASH_STATUS_WIP_BIT))
			{
				break;
			}
			else if (elapsed_time > SFLASH_TIMEOUT)
			{
				success = false;
				break;
			}
			else
			{
				utils_delay_ms(1);
			}
		}
		else
		{
			break;
		}
	}

	if (elapsed_time > SPI_WAIT_TIME_NOTICE)
	{
		PRINT_SFLASH_INFO("Waited SFLASH for %u ms.\n", elapsed_time);
	}

	return success;
}

/**
 * @brief  Enables the write operation on the flash memory.
 * @param  None
 * @return Boolean value which says if the operation completed successfully or not.
 */
static bool SFLASH_EnableWrite(void)
{
	bool success;
	uint8_t buffer_txrx[1];

	/* Write Enable */
	buffer_txrx[0] = SPI_FLASH_CMD_WENBL;

	success = SFLASH_SPI_Transfer(buffer_txrx, sizeof(buffer_txrx));

#if CHECK_WRITE_ENABLE
	if (success)
	{
		uint8_t status;

		success = SFLASH_GetStatusReg(&status);

		if (success)
		{
			if (MASK_IS_CLEAR(status, SPI_FLASH_STATUS_WEL_BIT))
			{
				success = false;
				Error_Handler();
			}
		}
	}
#endif

	return success;
}

/**
 * @brief  Erase a sector in serial flash.
 * @param  sector_id Number of the sector to erase.
 * @return Boolean value which says if the operation completed successfully or not.
 */
static bool SFLASH_SectorErase(uint16_t sector_id)
{
	bool success = false;
	uint32_t start_address;
	uint8_t buffer_txrx[4];

	memset(buffer_txrx, 0, sizeof(buffer_txrx));

	start_address = sector_id * SPI_FLASH_SECTOR_SIZE;

	if (start_address < SPI_FLASH_SIZE)
	{
		/* Wait until SFLASH is ready */
		success = SFLASH_WaitReady();

		if (success)
		{
			/* Write Enable */
			success = SFLASH_EnableWrite();

			if (success)
			{
				/* Sector Erase */
				buffer_txrx[0] = SPI_FLASH_CMD_SE;
				buffer_txrx[1] = TAKE_BYTE_MASK(start_address, 2, 0x1F);
				buffer_txrx[2] = TAKE_BYTE(     start_address, 1);
				buffer_txrx[3] = TAKE_BYTE(     start_address, 0);

				success = SFLASH_SPI_Transfer(buffer_txrx, sizeof(buffer_txrx));

				if (success)
				{
					success = SFLASH_WaitReady();
				}
			}
		}
	}

	if (success)
	{
		PRINT_SFLASH_INFO("Erase operation (sector=0x%u) successful\n", sector_id);
	}
	else
	{
		PRINT_SFLASH_CRITICAL("Erase operation (sector=%u) failed\n", sector_id);
	}

	return success;
}

/**
 * @brief  Gets the device ID
 * @param  device_id Pointer to the variable where the ID will be stored
 * @return Boolean that indicates the success of the operation
 */
bool SFLASH_GetDeviceId(uint32_t* device_id)
{
	bool success;
	uint8_t buffer_txrx[4]; /* CFD length (1 byte) and content (16 bytes) is ignored */

	memset(buffer_txrx, 0, sizeof(buffer_txrx));

	buffer_txrx[0] = SPI_FLASH_CMD_MDR;

	/* Wait until SFLASH is ready */
	success = SFLASH_WaitReady();

	if (success)
	{
		success = SFLASH_SPI_Transfer(buffer_txrx, sizeof(buffer_txrx));

		if (success)
		{
			*device_id = ASSEMBLE_U24(buffer_txrx[1], buffer_txrx[2], buffer_txrx[3]);

			PRINT_SFLASH_INFO("SFLASH manufacturer ID: 0x%X (%s).\n", buffer_txrx[1], (buffer_txrx[1] == SFLASH_MANU_WINBOND_ID) ? "Winbond" : "unknown");
			PRINT_SFLASH_INFO("SFLASH device ID type:  0x%X.\n", ASSEMBLE_U16(buffer_txrx[2], buffer_txrx[3]));
		}
		else
		{
			PRINT_SFLASH_CRITICAL("SFLASH error while reading device ID.\n");
		}
	}

	return success;
}

/**
 * @brief  Performs read from serial flash (aligned on pages)
 * @param  buff_rd Destination address in RAM
 * @param  address Source address in serial flash
 * @param  size Number of bytes to read
 * @retval 'true' if successful, 'false' otherwise
 */
bool SFLASH_Read(uint8_t *buff_rd, uint32_t address, uint32_t size)
{
	bool success = false;
	uint32_t current_address = address;

	if ((buff_rd != NULL) && ((current_address+size) <= SPI_FLASH_SIZE) && (size > 0))
	{
		uint32_t bytes_read = 0;
		uint16_t packet_length;
		uint8_t *buffer_txrx = MEMPOOL_MALLOC(SPI_FLASH_READ_HEADER + SPI_FLASH_READ_MAX);

		while (bytes_read < size)
		{
			/* Wait until SFLASH is ready */
			success = SFLASH_WaitReady();

			if (success)
			{
				if ((size - bytes_read) >= SPI_FLASH_READ_MAX)
				{
					packet_length = SPI_FLASH_READ_MAX;
				}
				else
				{
					packet_length = size - bytes_read;
				}

				/* Send "Main Memory Page Read Through Buffer 1" instruction */
				buffer_txrx[0] = SPI_FLASH_CMD_MMPR;
				buffer_txrx[1] = TAKE_BYTE_MASK(current_address, 2, 0x1F);
				buffer_txrx[2] = TAKE_BYTE(     current_address, 1);
				buffer_txrx[3] = TAKE_BYTE(     current_address, 0);
				buffer_txrx[4] = 0; /* Dummy */

				success = SFLASH_SPI_Transfer(buffer_txrx, SPI_FLASH_READ_HEADER + packet_length);

				if (success)
				{
					memcpy(&buff_rd[bytes_read], &buffer_txrx[SPI_FLASH_READ_HEADER], packet_length);

					bytes_read += packet_length;
					current_address    += packet_length;
					current_address    &= SPI_FLASH_TOT_MASK;
				}
				else
				{
					memset(&buff_rd[bytes_read], 0, size);
					break;
				}
			}
			else
			{
				break;
			}
		}

		MEMPOOL_FREE(buffer_txrx);
	}

	if (success)
	{


#if (DEBUG_USER_IT >= DEBUG_LEVEL_FULL)
		ALLOC_DYNAMIC_HEX_STRING(block_str, &buff_rd[0], SPI_FLASH_READ_PREVIEW_SIZE);
		PRINT_SFLASH_INFO("Read operation (address=0x%X; size=%u) successful, raw data: %s\n", address, size, block_str);
		FREE_DYNAMIC_HEX_STRING(block_str)
#else
		PRINT_SFLASH_INFO("Read operation (address=0x%X; size=%u) successful\n", address, size);
#endif
	}
	else
	{
		PRINT_SFLASH_CRITICAL("Read operation (address=0x%X; size=%u) failed\n", address, size);
	}

	return success;
}

/**
 * @brief  Performs write to serial flash (aligned on pages)
 * @param  address Destination address in serial flash
 * @param  buff_wr Source address in RAM
 * @param  size Number of bytes to write
 * @retval 'true' if successful, 'false' otherwise
 */
bool SFLASH_Write(uint32_t address, uint8_t *buff_wr, uint32_t size)
{
	bool success = false;
	uint32_t current_address = address;

	if ((buff_wr != NULL) && ((current_address+size) <= SPI_FLASH_SIZE) && (size > 0))
	{
		uint32_t bytes_written = 0;
		uint16_t packet_length;
		uint16_t bytes_to_page_end;
		uint8_t  *buffer_txrx = MEMPOOL_MALLOC(SPI_FLASH_WRITE_HEADER + SPI_FLASH_WRITE_MAX);

		while (bytes_written < size)
		{
			/* Wait until SFLASH is ready */
			success = SFLASH_WaitReady();

			if (success)
			{
				/* Write Enable */
				success = SFLASH_EnableWrite();

				if (success)
				{
					/* It is necessary to reamin in the same page,
					 * otherwise, bytes shall be written from the start again,
					 * instead of be written in the next page */
					bytes_to_page_end = SPI_FLASH_WRITE_MAX - (current_address % SPI_FLASH_WRITE_MAX);

					if ((size - bytes_written) >= bytes_to_page_end)
					{
						packet_length = bytes_to_page_end;
					}
					else
					{
						packet_length = size - bytes_written;
					}

					/* Send "Main Memory Page Program Through Buffer 1" instruction */
					buffer_txrx[0] = SPI_FLASH_CMD_MMPP;
					buffer_txrx[1] = TAKE_BYTE_MASK(current_address, 2, 0x1F);
					buffer_txrx[2] = TAKE_BYTE(     current_address, 1);
					buffer_txrx[3] = TAKE_BYTE(     current_address, 0);

					memcpy(&buffer_txrx[SPI_FLASH_WRITE_HEADER], &buff_wr[bytes_written], packet_length);

					success = SFLASH_SPI_Transfer(buffer_txrx, SPI_FLASH_WRITE_HEADER + packet_length);

					if (success)
					{
						bytes_written   += packet_length;
						current_address += packet_length;
						current_address &= SPI_FLASH_TOT_MASK;
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		MEMPOOL_FREE(buffer_txrx);
	}

	if (success)
	{
		PRINT_SFLASH_INFO("Write operation (address=0x%X; size=%u) successful\n", address, size);
	}
	else
	{
		PRINT_SFLASH_CRITICAL("Write operation (address=0x%X; size=%u) failed\n", address, size);
	}

	return success;
}

/**
 * @brief  Performs erase in serial flash (aligned on sectors)
 * @param  address Address in serial flash of the sector to erase
 * @param  size Number of bytes that must be erased (shall cancel all sectors that include them)
 * @retval 'true' if successful, 'false' otherwise
 */
bool SFLASH_Erase(uint32_t address, uint32_t size)
{
	bool success = true;

	uint16_t sector_start;
	uint16_t sector_number;
	sector_start  = (uint16_t)(address / SPI_FLASH_SECTOR_SIZE);
	sector_number = (uint16_t)(size    / SPI_FLASH_SECTOR_SIZE);  /* sectors fully used */

	if ((size % SPI_FLASH_SECTOR_SIZE) != 0)
	{
		sector_number++; /* sector partially used */
	}

	for (uint16_t i = sector_start; i < sector_start + sector_number; i++)
	{
		success = (success && SFLASH_SectorErase(i));

		if (!success)
		{
			break;
		}
	}

	if (success)
	{
		PRINT_SFLASH_INFO("Erase operation (address=0x%X; size=%u) successful\n", address, size);
	}
	else
	{
		PRINT_SFLASH_CRITICAL("Write operation (address=0x%X; size=%u) failed\n", address, size);
	}

	return success;
}

/**
 * @brief  Erase all sectors in serial flash
 * @param  None
 * @return Boolean value which says if the operation completed successfully or not
 */
bool SFLASH_BulkErase(void)
{
	bool success = false;

	uint8_t buffer_txrx[1];

	/* Wait until SFLASH is ready */
	success = SFLASH_WaitReady();

	if (success)
	{
		/* Write Enable */
		success = SFLASH_EnableWrite();

		if (success)
		{
			buffer_txrx[0] = SPI_FLASH_CMD_BE;

			success = SFLASH_SPI_Transfer(buffer_txrx, sizeof(buffer_txrx));

			if (success)
			{
				success = SFLASH_WaitReady();
			}
		}
	}

	return success;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
