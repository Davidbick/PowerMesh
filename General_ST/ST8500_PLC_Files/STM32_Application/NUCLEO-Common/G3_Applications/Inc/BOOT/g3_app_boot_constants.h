/*****************************************************************************
*   @file    g3_app_boot_contants.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Boot Server/device constants.
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
#ifndef G3_APP_BOOT_CONSTANTS_H_
#define G3_APP_BOOT_CONSTANTS_H_

/** @addtogroup G3_App
  * @{
  */

/** @defgroup G3_App_Boot_Srv G3 Boot Server
  * @{
  */

/** @defgroup G3_App_Boot_Srv_Private_Code Private code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Boot_Srv_Exported_Code Exported code
  * @{
  */

/* Boot Attributes definitions */

#if IS_COORD

/* Server attributes */
#define BOOT_SERVER_START_WAIT_TIME         		1		/**< @brief bootServerStartWaitTime: The time to wait before starting the discovery process, in seconds. If more PAN are present set this to 16 or more */
#define BOOT_SERVER_DEFAULT_MAX_HOPS        		14  	/**< @brief bootServerDefaultMaxHops: The default MaxHops value used in LBP messages sent from the Server */
#define BOOT_SERVER_DISCOVERY_TIME          		1  		/**< @brief bootServerDiscoveryTime: The duration in seconds of the active scan */
#define BOOT_SERVER_FORCE_PAN_START         		1		/**< @brief bootServerForcePanStart: If True (0x01) force the start of the LBS, without taking into account the beacon caught during the discovery */

/* IDS (EAP server NAI) */
#define BOOT_SERVER_IDS                     		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  /**< @brief bootServer_ID_S: the NAI */
#define BOOT_SERVER_IDS_LEN                 		0  		/**< @brief bootServer_ID_S: the NAI length */

/* Timing */
#define BOOT_SERVER_PSK_GET_TIMEOUT          		1000	/**< @brief bootServerPSKGetTimeout: The time to wait for the Host to send the PSK to be used, in milliseconds */
#define BOOT_SERVER_JOINING_IGNORE_TIME				500  	/**< @brief bootServerJoiningIgnoreTime: After the first Joining message from an LBD, ignore the following ones for the duration of this time, in milliseconds */
#define BOOT_SERVER_JOINING_TABLE_ENTRY_TTL			70		/**< @brief bootServerJoiningTableEntryTTL: Time, in seconds, after which an entry in the Joining Table is considered expired and can be reset */

#define BOOT_SERVER_BOOTSTRAP_TIMEOUT_CHECK_PERIOD	(1000U)	/* Period of the bootstrap timeout check, in ms */

#else

/* Client/Device attributes */
#define BOOT_CLIENT_DISCOVERY_TIME          		16		/**< @brief bootDeviceDiscoveryTime: The duration in seconds of the active scan */
#define BOOT_CLIENT_LQI_THRESHOLD           		52		/**< @brief bootDeviceLQIThreshold: The LQI value below which the received PAN descriptors are ignored */
#define BOOT_CLIENT_START_WAIT_TIME         		2		/**< @brief bootDeviceStartWaitTime: The time to wait before starting the discovery process, in seconds */
#define BOOT_CLIENT_PANSORT_TIMEOUT					3000	/**< @brief bootDevicePANSortTimeout: The time (in milliseconds) that the internal bootstrap application waits for the external host the device PAN description sorting results before sorting by itself the descriptors */
#define BOOT_CLIENT_ASSOCIATION_MAX_RETRIES   		3		/**< @brief bootDeviceAssociationMaxRetries: the number of time a device tries to associate with the same LBA */
#define BOOT_CLIENT_ASSOCIATION_RAND_WAIT_TIME 		30		/**< @brief bootDeviceAssociationRandWaitTime: the maximum of the random time (in seconds) window before trying to attempt to the same LBA */

#define BOOT_CLIENT_DEFAULT_MAX_HOPS        		14  	/**< @brief The default MaxHops value used in LBP messages sent from the Device */

#define BOOT_CLIENT_DISCOVER_ROUTE					0  		/**< @brief If enabled, discovers the route to the coordinator during the bootstrap */

/* IDP (EAP peer NAI) */
#define BOOT_CLIENT_IDP                     		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  /**< @brief bootDevice_ID_P: the NAI */
#define BOOT_CLIENT_IDP_LEN                 		0  		/**< @brief bootDevice_ID_P: the NAI length */

/* Timing */
#define BOOT_CLIENT_PANSORT_TIMEOUT         		3000	/**< @brief bootDevicePANSortTimeout: The time to wait for the Host to sort the PAN descriptors, in milliseconds */

#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /*G3_APP_BOOT_CONSTANTS_H_*/

/******************* (C) COPYRIGHT 2021 STMicroelectronics *****END OF FILE****/
