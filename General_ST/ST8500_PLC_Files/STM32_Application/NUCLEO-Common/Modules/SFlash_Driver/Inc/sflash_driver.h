/**
  ******************************************************************************
  *   @file    sflash_driver.h
  *   @author  AMG/IPC Application Team
  *   @brief   Header file of SFLASH driver.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef SFLASH_DRIVER_H_
#define SFLASH_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>

/* Public functions */
bool SFLASH_GetDeviceId(uint32_t* device_id);
bool SFLASH_Read( uint8_t *buf,     uint32_t address, uint32_t size);
bool SFLASH_Write(uint32_t address, uint8_t *buf,     uint32_t size);
bool SFLASH_Erase(uint32_t address, uint32_t size);
bool SFLASH_BulkErase(void);

#ifdef __cplusplus
}
#endif

#endif /* SFLASH_DRIVER_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
