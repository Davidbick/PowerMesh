/*****************************************************************************
*   @file    hi_ip_pib_types.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for Types definition used in IP PIB.
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
#ifndef HI_IP_PIB_TYPES_H
#define HI_IP_PIB_TYPES_H

/* Inclusions */
#include <stdint.h>
#include <hi_adp_pib_types.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_ip_drv HIF IP DRV
  * @{
  */

/** @addtogroup IP_PIB_Attributes_TypesDefinitions
  * @{
  */

/** @defgroup Internal_TypesDefinitions Internal_TypesDefinitions
  * @brief Definitions of Internal Types
  * @{
  */

/* interface typedef */
#define IP_MAX_SDU_SIZE                 (1500)

#define IP_IPV6_ADDR128_UINT8_LEN       16   /**< @brief Length (in bytes) of a 128 bits IPv6 address */
#define IP_IPV6_ADDR128_UINT16_LEN      8    /**< @brief Length (in words) of a 128 bits IPv6 address */

/**
  * @brief  IPv6 Address Structures
  */
typedef union ip6_addr_uni
{
  uint8_t  u8[IP_IPV6_ADDR128_UINT8_LEN];
  uint16_t u16[IP_IPV6_ADDR128_UINT16_LEN];
} ip6_addr_t;

typedef struct
{
    uint16_t Address[IP_IPV6_ADDR128_UINT16_LEN];    /**< @brief The Source Address */
} IP_Addr128_t;

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#endif //HI_IP_PIB_TYPES_H

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
