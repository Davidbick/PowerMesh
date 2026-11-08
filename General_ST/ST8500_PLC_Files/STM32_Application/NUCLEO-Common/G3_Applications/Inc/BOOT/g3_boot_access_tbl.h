/*****************************************************************************
*   @file    g3_boot_acces_tbl.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the boot access table management.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef G3_APP_BOOT_ACCESS_TBL_H_
#define G3_APP_BOOT_ACCESS_TBL_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup G3_App_Boot G3 Boot Application
  * @{
  */

/**
  * @}
  */


/** @defgroup G3_App_Boot_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Boot_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <settings.h>
#include <g3_boot_types.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot
  * @{
  */

/** @addtogroup G3_App_Boot_Exported_Code
  * @{
  */

/* Definitions */

/* Custom Types */

/* Public functions */

#if IS_COORD
void 			g3_boot_access_table_init(void);
g3_boot_data_t*	g3_boot_access_table_find(const uint8_t *extended_address);
#endif /* IS_COORD */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* G3_APP_BOOT_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
