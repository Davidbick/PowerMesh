/*****************************************************************************
*   @file    g3_boot_types.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the G3 Boot types.
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
#ifndef G3_BOOT_TYPES_H_
#define G3_BOOT_TYPES_H_

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
#include <stdint.h>
#include <stdbool.h>
#include <hi_mac_constants.h>
#include <hi_adp_lbp_message_catalog.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot
  * @{
  */

/** @addtogroup G3_App_Boot_Exported_Code
  * @{
  */


/* Custom Types */
typedef struct g3_boot_data_str
{
  uint16_t short_addr;						/*!< Short address */
  uint8_t  ext_addr[MAC_ADDR64_SIZE];		/*!< Extended address */
  uint8_t  psk[ADP_EAP_PSK_KEY_LEN];		/*!< PSK value */
} g3_boot_data_t;

typedef struct g3_boot_tbl_data_str
{
    uint16_t next_short_addr;
    uint16_t entry_number;
} g3_boot_tbl_data_t;

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

#endif /* G3_BOOT_TYPES_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
