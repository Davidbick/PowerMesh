/**
*   @file    hi_adp_pib_attributes.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for ADP PIB attributes.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HI_BOOT_PIB_ATTRIBUTES_H
#define HI_BOOT_PIB_ATTRIBUTES_H

#include <stdint.h>

/** @addtogroup BOOT
  * @{
  */

/** @defgroup BOOT_PIB_Attributes BOOT PIB Attributes
  * @brief    BOOT PIB Attributes module
  * @details  This module contains the definition/handling of the BOOT PIB Attributes, which are defined by the G3. The module contains also implementation defined attributes
  * @{
  */

/** @defgroup BOOT_PIB_Attributes_IDDefinitions BOOT PIB Attributes Definitions of the IDs
  * @brief    BOOT PIB Attributes definitions of IDs
  * @{
  */

/* Attribute ID definition */
#define BOOT_DEVICE_DISCOVERY_TIME_ID          ((uint32_t) 0x1000)  /**< @brief bootDeviceDiscoveryTime: The duration in seconds of the active scan */
#define BOOT_DEVICE_RESERVED1_ID               ((uint32_t) 0x1001)  /**< @brief bootDeviceReserverd1: Unused attribute */
#define BOOT_DEVICE_RESERVED2_ID               ((uint32_t) 0x1002)  /**< @brief bootDeviceReserverd2: Unused attribute */
#define BOOT_DEVICE_RESERVED3_ID               ((uint32_t) 0x1003)  /**< @brief bootDeviceReserverd3: Unused attribute */
#define BOOT_DEVICE_RESERVED4_ID               ((uint32_t) 0x1004)  /**< @brief bootDeviceReserverd4: Unused attribute */
#define BOOT_DEVICE_RESERVED5_ID               ((uint32_t) 0x1005)  /**< @brief bootDeviceReserverd5: Unused attribute */
#define BOOT_DEVICE_RESERVED6_ID               ((uint32_t) 0x1006)  /**< @brief bootDeviceReserverd6: Unused attribute */
#define BOOT_DEVICE_LQI_THRESHOLD_ID           ((uint32_t) 0x1007)  /**< @brief bootDeviceLQIThreshold: The LQI value below which the received PAN descriptors are ignored */
#define BOOT_DEVICE_START_WAIT_TIME_ID         ((uint32_t) 0x1008)  /**< @brief bootDeviceStartWaitTime: The time to wait before starting the discovery process, in seconds */
#define BOOT_DEVICE_OPTIONS_ID                 ((uint32_t) 0x1009)  /**< @brief bootDeviceOptions: 32-bit bitmask to define some options in the bootstrap process
                                                                             b0: start a route discovery process to the coordinator just after the network join
                                                                             b1: Exclude PanId adding in macPanIdIgnoreTable_RF on bootstrap failure
                                                                             b2: LBP header management as PLC only
                                                                             b3: reserved
                                                                 */
#define BOOT_DEVICE_ASSOCIATIONMAXRETRIES_ID   ((uint32_t) 0x100A)  /**< @brief bootDeviceAssociationMaxRetries: the number of time a device tries to associate with the same LBA */
#define BOOT_DEVICE_ASSOCIATIONRANDWAITTIME_ID ((uint32_t) 0x100B)  /**< @brief bootDeviceAssociationRandWaitTime: the random time (in mins) window before trying to attempt to the same LBA */
#define BOOT_DEVICE_ASSOCIATIONALTPAN_ID       ((uint32_t) 0x100C)  /**< @brief bootDeviceAssociationAltPan: Indicate if the device can join any PAN (1) or only the first found (0) */
#define BOOT_DEVICE_LASTUSEDLBA_ID             ((uint32_t) 0x100D)  /**< @brief bootDeviceLastUsedLBA: The 16-bits Short Address of the last used LBA */
#define BOOT_DEVICE_PROCESS_STATE_ID           ((uint32_t) 0x100F)  /**< @brief bootDeviceProcessState: Indicates the state of the bootstrap process */
#define BOOT_SERVER_START_WAIT_TIME_ID         ((uint32_t) 0x1010)  /**< @brief bootServerStartWaitTime: The time to wait before starting the discovery process, in seconds */
#define BOOT_SERVER_DEFAULT_MAX_HOPS_ID        ((uint32_t) 0x1011)  /**< @brief bootServerDefaultMaxHops: The default MaxHops value used in LBP messages sent from the Server */
#define BOOT_SERVER_DISCOVERY_TIME_ID          ((uint32_t) 0x1012)  /**< @brief bootServerDiscoveryTime: The duration in seconds of the active scan */
#define BOOT_SERVER_FORCE_PAN_START_ID         ((uint32_t) 0x1013)  /**< @brief bootServerForcePanStart: If True (0x01) force the start of the LBS, without taking into account the beacon caught during the discovery */
#define BOOT_SERVER_IDS_ID                     ((uint32_t) 0x1014)  /**< @brief bootServer_ID_S: the NAI */
#define BOOT_SERVER_IDS_LEN_ID                 ((uint32_t) 0x1015)  /**< @brief bootServer_ID_S: the NAI length */
#define BOOT_DEVICE_PANSORT_TIMEOUT_ID         ((uint32_t) 0x1016)  /**< @brief bootDevicePANSortTimeout: The time to wait for the Host to sort the PAN descriptors, in milliseconds */
#define BOOT_SERVER_PSKGET_TIMEOUT_ID          ((uint32_t) 0x1017)  /**< @brief bootServerPSKGetTimeout: The time to wait for the Host to send the PSK to be used, in milliseconds */
#define BOOT_SERVER_JOININGIGNORETIME_ID       ((uint32_t) 0x1018)  /**< @brief bootServerJoiningIgnoreTime: After the first Joining message from an LBD, ignore the following ones for the duration of this time, in milliseconds */
#define BOOT_SERVER_JOININGTABLEENTRYTTL_ID    ((uint32_t) 0x1019)  /**< @brief bootServerJoiningTableEntryTTL: Time, in seconds, after which an entry in the Joining Table is considered expired and can be reset */

/* Bitmask definition*/
#define BOOT_DEVICE_OPTIONS_RREQSTART       ((uint32_t)0x00000001) /**< @brief Start route discovery process option bit mask */
#define BOOT_DEVICE_OPTIONS_EXCLUDEPANID    ((uint32_t)0x00000002) /**< @brief Exclude PanId option bit mask */
#define BOOT_DEVICE_OPTIONS_LBPHEADERPLC    ((uint32_t)0x00000004) /**< @brief LBP Header PLC option bit mask */


#define BOOT_SERVER_JOININGTABLEENTRYTTL_BATT   ((uint16_t)0x0708) /**< @brief Joining Table entry TTL for battery powered devices (30 min) */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /*HI_BOOT_PIB_ATTRIBUTES_H*/

/******************* (C) COPYRIGHT 2014 STMicroelectronics *****END OF FILE****/
