/*****************************************************************************
*   @file    hi_mac_pib_types.h
*   @author  AMG/IPC Application Team
*   @brief   Header file with the Types used in Attribute and MAC management.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef HI_MAC_PIB_TYPES_H
#define HI_MAC_PIB_TYPES_H

/* Inclusions */
#include <hi_mac_constants.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_mac_drv HIF MAC DRV
  * @{
  */

/** @defgroup MAC_PIB_Attributes_TypesDefinitions HIF MAC PIB Types Definitions
  * @{
  */

#pragma pack(push, 1)

/**
  * @brief  MAC Device Addressing Information
  */
typedef struct
{
    uint8_t      addr_mode;                 /**< @brief The Address Mode - 0: Address not present, 1: Reserved, 2: 16-bit Address present, 3: 64-bit Address present  */
    uint16_t     pan_id;                    /**< @brief The Pan Identifier  */
    union
    {
        uint16_t short_addr;                /**< @brief 16-bit Address */
        uint8_t  ext_addr[MAC_ADDR64_SIZE]; /**< @brief 64-bit Address */
    }; /**< @brief The address of the device (16 or 64 bit) */
} MAC_DeviceAddress_t;

#pragma pack(pop)

/**
  * @brief  A key for secured communications
  */
typedef struct
{
    uint8_t FreeElement;           /**< @brief indicates if the entry is valid or not */
    uint8_t KeyData[MAC_KEY_SIZE]; /**< @brief The buffer in which the Key is stored */
} MAC_Key_t;

/**
  * @brief  The number of key contained in the KeyTable
  */
#define MAC_KEY_TBL_LEN ((uint8_t)2)

/**
  * @brief  A table containing keys required for secured communications
  */
typedef struct
{
    MAC_Key_t CurrentKey;   /**< @brief The Current Key */
    MAC_Key_t PrecedingKey; /**< @brief The Previous Key */
} MAC_KeyTable_t;




/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* HI_MAC_PIB_TYPES_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
