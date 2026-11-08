/**
  ******************************************************************************
  * @file    sflash_info.h
  * @author  AMG/IPC Application Team
  * @brief   SPI FLASH information file.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef SFLASH_INFO_H_
#define SFLASH_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */
#define SPI_FLASH_PAGE_SIZE                 ((uint16_t) 0x100)         	/*!< Flash page size, in bytes (256) */
#define SPI_FLASH_PAGES_PER_SECTOR       	((uint16_t) 0x100)			/*!< Number of flash pages in a sector (256) */
#define SPI_FLASH_SECTORS_CNT               ((uint16_t) 0x20)			/*!< Number of sectors (32) */
#define SPI_FLASH_SECTOR_SIZE               ((uint32_t) 0x10000)		/*!< Sector size, in bytes (65536) */
#define SPI_FLASH_SIZE                      ((uint32_t) 0x200000) 		/*!< Flash memory size, in bytes (2097152) */

#define SPI_FLASH_PAGE_MASK                 (0xFF)						/*!< Flash page size mask */
#define SPI_FLASH_TOT_MASK                  (0x1FFFFF)					/*!< Flash memory size mask */

#define SPI_FLASH_STM32_ID 					(0xEF4015)					/*!< ID of the SPI Flash chip connected to the STM32 */
#define SPI_FLASH_STM32_ALT_ID 				(0x202015)					/*!< ID of the SPI Flash chip connected to the STM32 */
#define SPI_FLASH_ST8500_ID 				(0xEF4015)					/*!< ID of the SPI Flash chip connected to the ST8500 */

#ifdef __cplusplus
}
#endif

#endif /* SFLASH_INFO_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
