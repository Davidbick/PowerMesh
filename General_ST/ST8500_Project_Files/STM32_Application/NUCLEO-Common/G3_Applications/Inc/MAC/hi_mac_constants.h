/*****************************************************************************
*   @file    hi_mac_constants.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for MAC constants.
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
#ifndef HI_MAC_CONSTANTS_H
#define HI_MAC_CONSTANTS_H

/* Inclusions */
#include <stdint.h>
#include <hi_phy_support.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_mac_drv HIF MAC DRV
  * @{
  */

/** @defgroup MAC_Constants MAC Constants
  * @brief MAC Constants
  * @{
  */

/* G3-PLC MAC constants */
#define MAC_PAYLOAD_MAX_SIZE       ((uint16_t) 400)  /**< @brief The max number of octets that can be transmitted in the MAC Payload field. */
#define aMinFrameSize              ((uint16_t)   4)  /**< @brief The minimum MAC frame size in data symbols. */
#define aMaxFrameSize              ((uint16_t) 252)  /**< @brief The maximum MAC frame size in data symbols. */

#define aCIFS_FCC                  ((uint16_t)  10)  /**< @brief The Contention Inter Frame Space (in data symbols). */
#define aRIFS_FCC                  ((uint16_t)  10)  /**< @brief The response interframe space (in data symbols). */
#define aPreamSymbolTime_FCC       ((uint16_t) 213)  /**< @brief The duration of one preamble symbol on physical layer (in microseconds). */
#define aSymbolTime_FCC            ((uint16_t) 232)  /**< @brief The duration of one data symbol on physical layer. */
#define aAckTime_FCC               ((uint16_t) ((PHY_NPRE * aPreamSymbolTime_FCC) + (PHY_NFCH * aSymbolTime_FCC)))  /**< @brief The duration of acknowledgement. */
#define aEIFS_FCC                  ((uint16_t) (aSymbolTime_FCC * (aMaxFrameSize + aRIFS_FCC + aCIFS_FCC) + aAckTime_FCC))  /**< @brief The extended interframe space. */

#define aCIFS_CENA                 ((uint16_t)   8)  /**< @brief The Contention Inter Frame Space (in data symbols). */
#define aRIFS_CENA                 ((uint16_t)   8)  /**< @brief The response interframe space (in data symbols). */
#define aPreamSymbolTime_CENA      ((uint16_t) 640)  /**< @brief The duration of one preamble symbol on physical layer (in microseconds). */
#define aSymbolTime_CENA           ((uint16_t) 695)  /**< @brief The duration of one data symbol on physical layer. */
#define aAckTime_CENA              ((uint16_t) ((PHY_NPRE * aPreamSymbolTime_CENA) + (PHY_NFCH * aSymbolTime_CENA)))  /**< @brief The duration of acknowledgement. */
#define aEIFS_CENA                 ((uint16_t) (aSymbolTime_CENA * (aMaxFrameSize + aRIFS_CENA + aCIFS_CENA) + aAckTime_CENA))  /**< @brief The extended interframe space. */

#define aSlotTime                  ((uint16_t)   2)  /**< @brief The duration of contention slot time (in data symbols). */
#define aBaseSlotDuration          ((uint16_t)   0)  /**< @brief The number of symbols forming a superframe slot when the superframe order is equal to 0 (unused). */
#define aBaseSuperframeDuration    ((uint16_t)   0)  /**< @brief The number of symbols forming a superframe when the superframe order is equal to 0 (unused). */
#define aGTSDescPersistenceTime    ((uint16_t)   0)  /**< @brief The number of superframes in which a GTS descriptor exists in the beacon frame (unused). */
#define aMaxBeaconOverhead         ((uint16_t)   0)  /**< @brief The max number of octects added by the MAC sublayer to the MAC payload of a beacon frame (unused). */
#define aMaxBeaconPayloadLength    ((uint16_t)   0)  /**< @brief The max size, in octets, of a beacon payload (unused). */
#define aMaxLostBeacon             ((uint16_t)   0)  /**< @brief The number of consecutive lost beacons that will cause the MAC sublayer to declare a loss of synch (unused). */
#define aMaxMACSafePayloadSize     ((uint16_t)   0)  /**< @brief The maximum number of octets that can be transmitted in the MAC Payload field of an unsecured MAC frame (unused). */
#define aMaxMPDUUnsecuredOverhead  ((uint16_t)   0)  /**< @brief The max number of octects added by the MAC sublayer to the PSDU without security (unused). */
#define aMaxSIFSFrameSize          ((uint16_t)   0)  /**< @brief The maximum size of an MPDU, in octets, that can be followed by a SIFS period. (unused). */
#define aMinCAPLength              ((uint16_t)   0)  /**< @brief The minimum number of symbols forming the CAP. (unused). */
#define aMinMPDUOverhead           ((uint16_t)   9)  /**< @brief The min number of octets added by the MAC sublayer to the PSDU. */
#define aNumSuperframeSlots        ((uint16_t)   0)  /**< @brief The number of slots contained in any superframe. (unused). */
#define aUnitBackoffPeriod         ((uint16_t)   2)  /**< @brief The number of symbols forming the basic time period used by the CSMA-CA algorithm. */

// Addresses Sizes
#define MAC_PAN_ID_SIZE            ((uint16_t) 0x02)  /**< @brief The size in bytes of the MAC PAN Id. */
#define MAC_ADDR16_SIZE            ((uint16_t) 0x02)  /**< @brief The size in bytes of the MAC Short Address. */
#define MAC_ADDR64_SIZE            ((uint16_t) 0x08)  /**< @brief The size in bytes of the MAC Extended Address. */
#define MAC_MIC32_SIZE             ((uint16_t) 0x04)  /**< @brief Security MIC-32 tag size (in bytes). */
#define MAC_TOTALPAYLOAD_SIZE      ((uint16_t) (MAC_PAYLOAD_MAX_SIZE + MAC_MIC32_SIZE)) /**< @brief The max number of octets that can be transmitted in the MAC Payload field. */

// Key Size
#define MAC_KEY_NUMBER				((uint16_t)2)
#define MAC_KEY_SIZE           		((uint16_t)16)      /**< @brief The size of the key used to secure communication */

// Nonce Size
#define MAC_NONCE_SIZE             	((uint16_t)0x0D)  /**< @brief Size (in bytes) of the Nonce  */

#endif /*HI_MAC_CONSTANTS_H*/

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
