/**
  ******************************************************************************
  * @file    settings.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the application settings.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef SETTINGS_H_
#define SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */
#define COMPILE_7_X_VERSION 1

#if IS_COORD
#define PLC_DEVICE_TYPE		PLC_G3_COORD
#else
#define PLC_DEVICE_TYPE		PLC_G3_DEVICE
#endif

#if IS_COORD
/* Device access policy mode */
#define BLACK_LIST_MODE				(1)	/* In this mode, only the extended addresses out of the  main table are allowed to connect,	*
										 * the short address assigned to each device is incremental (starting from 1),				*
										 * the PSK used for each device is the default one.											*/

#define WHITE_LIST_MODE				(2)	/* In this mode, only the extended addresses in the main table are allowed to connect,		*
										 * the short address assigned to each device is the one specified in the access table,		*
										 * the PSK used for each device is the one specified in the access table. 					*/
#else
/* PAN sorting criteria */
#define PANSORT_CRITERION_LINK_QUALITY		(1)  /* Sort on link quality */
#define PANSORT_CRITERION_SHORT_ADDRESS		(2)  /* Sort on short address */
#define PANSORT_CRITERION_ROUTE_COST		(3)  /* Sort on link quality */

#define PANSORT_LOWER_FIRST					(-1) /* Lower values are put at the start of the list */
#define PANSORT_HIGHER_FIRST				(1)  /* Higher values are put at the start of the list */
#endif

/* Debug related */
#define DEBUG_LEVEL_NONE			0 /* Disabled */
#define DEBUG_LEVEL_CRITICAL		1 /* Enabled for critical messages */
#define DEBUG_LEVEL_WARNING			2 /* Enabled for critical/warning messages */
#define DEBUG_LEVEL_INFO			3 /* Enabled for critical/warning/info messages */
#define DEBUG_LEVEL_FULL			4 /* Enabled for all messages */

/* Memory pool Debug */
#define MEMPOOL_DEBUG_NONE			0	/* No debug */
#define MEMPOOL_DEBUG_LOW			1	/* Includes alloc/free counters */
#define MEMPOOL_DEBUG_MEDIUM		2	/* Includes free block check, 3 = record counters */
#define MEMPOOL_DEBUG_HIGH			3	/* Includes record counters */
#define MEMPOOL_DEBUG_MAX			4	/* Registers the C code location of each malloc/free */

/* Settings */

/* Host Interface */
#define HIF_BAUDRATE				921600	/* Baudrate to set for the HIF serial communication (recommended value = 921600) */
#define USERIF_BAUDRATE				115200	/* Baudrate to set for the HIF serial terminal (recommended value = 115200) */

/* User Interface */
#define ENABLE_TIMESTAMP			1  	/*!< Define to 1 to display the timestamp in the prints on the User Interface, define to 0 to remove the timestamp */
#define ENABLE_TIMESTAMP_MICRO		1  	/*!< Define to 1 to add microseconds to the timestamp */
#define ENABLE_COLORS				1	/*!< Define to 1 to enable colored prints on the User Interface, define to 0 to disable colors */

/* Options */
#define RESET_AT_START				1	/* Resets the ST8500 at startup (strongly recommended to be set to 1) */
#define USE_POOL_IN_USER_TERMINAL 	1	/*!< Define to 1 to use memory pools for the User Interface buffers (instead of the stack of the calling task) */
#define USE_BIGGER_POOL_IF_NEEDED	1	/*!< Define to 1 to enable the use of a bigger pool if all pools of the best-fit type are occupied */
#define ENABLE_REKEYING_DELAYS		1	/*!< Define to 1 to separate each re-keying phase with a delay > */
#define SEND_UDP_TIMEOUT			1	/*!< Define to 1 to enable timeout on send_udp (to avoid mempool overflow in very high traffic conditions) > */

/* RF options */
#define USE_STANDARD_ETSI_RF		1	/* Selects the frequency and power gain values to be compliant with ETSI standard */
#define USE_STANDARD_FCC_RF			0	/* Selects the frequency and power gain values to be compliant with FCC standard */

#if !(USE_STANDARD_ETSI_RF ^ USE_STANDARD_FCC_RF)
#error "Only one between USE_STANDARD_ETSI_RF and USE_STANDARD_FCC_RF can be set to 1"
#endif

/* Enable/disable features */
#define ENABLE_ICMP_KEEP_ALIVE		1	/* Enable the keep alive feature */
#define ENABLE_DOWNLOAD				1	/* Enable the image download feature */
#define ENABLE_IMAGE_TRANSFER		1	/* Enable the image transfer feature */
#define ENABLE_SFLASH_MANAGEMENT	1	/* Enable the SPI FLASH management feature */
#define ENABLE_DEVICE_MANAGEMENT	1	/* Enable the device management feature */
#define ENABLE_REKEYING				1	/* Enable the GMK Re-keying feature */
#define ENABLE_LAST_GASP			1 	/* Enable the Last Gasp feature */
#define ENABLE_FAST_RESTORE			0 	/* Enable the Fast Restore feature */
#define ENABLE_FREQUENCY_HOPPING	0	/* Enable the Frequency Hopping feature */

#if (ENABLE_FAST_RESTORE && ENABLE_FREQUENCY_HOPPING)
#error "ENABLE_FAST_RESTORE and ENABLE_FREQUENCY_HOPPING cannot be set both to 1"
#endif

#if (ENABLE_FREQUENCY_HOPPING && !COMPILE_7_X_VERSION)
#error "ENABLE_FREQUENCY_HOPPING is available on G3_7_x version only"
#endif

#if IS_COORD
#define ENABLE_BOOT_SERVER_ON_HOST	0	/* If set to 1, the Boot Server of the coordinator is embedded in the host application (this FW) */
#else
#define ENABLE_BOOT_CLIENT_ON_HOST	0	/* If set to 1, the Boot Client of the device is embedded in the host application (this FW) */
#endif

/* Other settings */
#define ENABLE_LPMODE				0	/* Set to 1 to set the LPMODE as external input, 0 to force its de-assertion (recommended at 0) */

/* Enable/disable tests */
#if IS_COORD
/* In order to save FLASH memory, coordinator version does not include by default FATFS, MODUBUS, SPI FLASH tests */
#define ENABLE_MODBUS				0 	/* Enable the Modbus feature */
#define ENABLE_FATFS_TEST			0 	/* Enable the uSD card test (inside MAC test) */
#define ENABLE_MODBUS_TEST			0 	/* Enable the Modbus test (inside MAC test), requires ENABLE_MODBUS */
#define ENABLE_SFLASH_TEST			0 	/* Enable the SFLASH test (inside MAC test) */
#else
#if (ENABLE_FAST_RESTORE || ENABLE_FREQUENCY_HOPPING)
/* In order to save FLASH memory, device version does not include FATFS, MODUBUS, SPI FLASH tests together with fast_restore or frequency_hopping */
#define ENABLE_MODBUS				0 	/* Enable the Modbus feature */
#define ENABLE_FATFS_TEST			0 	/* Enable the uSD card test (inside MAC test) */
#define ENABLE_MODBUS_TEST			0 	/* Enable the Modbus test (inside MAC test), requires ENABLE_MODBUS */
#define ENABLE_SFLASH_TEST			0 	/* Enable the SFLASH test (inside MAC test) */
#else
#define ENABLE_MODBUS				1 	/* Enable the Modbus feature */
#define ENABLE_FATFS_TEST			1 	/* Enable the uSD card test (inside MAC test) */
#define ENABLE_MODBUS_TEST			1 	/* Enable the Modbus test (inside MAC test), requires ENABLE_MODBUS */
#define ENABLE_SFLASH_TEST			1 	/* Enable the SFLASH test (inside MAC test) */
#endif /* (ENABLE_FAST_RESTORE || ENABLE_FREQUENCY_HOPPING) */
#endif /* IS_COORD */

#if IS_COORD

/* Access mode */
#define SELECTED_LIST_MODE			BLACK_LIST_MODE	/* Selects black or white list mode	*/

#if (SELECTED_LIST_MODE != BLACK_LIST_MODE) && (SELECTED_LIST_MODE != WHITE_LIST_MODE)
#error "Black list mode or White list mode must be selected"
#endif

#else

/* PAN sorting criteria */

/* Select the PAN-sorting criterion: PANSORT_CRITERION_LINK_QUALITY, PANSORT_CRITERION_SHORT_ADDRESS, PANSORT_CRITERION_ROUTE_COST */
/* Select the PAN-sorting order: PANSORT_LOWER_FIRST, PANSORT_HIGHER_FIRST */

/* Selects the primary criterion for the PAN sorting */
#define PANSORT_CRITERION_PRIMARY			PANSORT_CRITERION_ROUTE_COST
#define PANSORT_ORDER_PRIMARY				PANSORT_HIGHER_FIRST

/* Selects the secondary criterion for the PAN sorting (evaluated after the primary criterion) */
#define PANSORT_CRITERION_SECONDARY			PANSORT_CRITERION_LINK_QUALITY
#define PANSORT_ORDER_SECONDARY				PANSORT_HIGHER_FIRST

#if (PANSORT_CRITERION_PRIMARY != PANSORT_CRITERION_LINK_QUALITY) && (PANSORT_CRITERION_PRIMARY != PANSORT_CRITERION_SHORT_ADDRESS) && (PANSORT_CRITERION_PRIMARY != PANSORT_CRITERION_ROUTE_COST)
#error "Primary PAN-sorting criteria must be selected (link quality, short address or route cost)"
#endif

#if (PANSORT_ORDER_PRIMARY != PANSORT_LOWER_FIRST) && (PANSORT_ORDER_PRIMARY != PANSORT_HIGHER_FIRST)
#error "Primary PAN-sorting criteria must be selected (higher first or lower first)"
#endif

#if (PANSORT_CRITERION_SECONDARY != PANSORT_CRITERION_LINK_QUALITY) && (PANSORT_CRITERION_SECONDARY != PANSORT_CRITERION_SHORT_ADDRESS) && (PANSORT_CRITERION_SECONDARY != PANSORT_CRITERION_ROUTE_COST)
#error "Secondary PAN-sorting criteria must be selected (link quality, short address or route cost)"
#endif

#if (PANSORT_ORDER_SECONDARY != PANSORT_LOWER_FIRST) && (PANSORT_ORDER_SECONDARY != PANSORT_HIGHER_FIRST)
#error "Secondary PAN-sorting criteria must be selected (higher first or lower first)"
#endif

#endif

/* Event Notifications */
#define ENABLE_EVENTINDICATION_ERROR			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_GMK				1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_CONTEXT			0	/* Default: 0 */
/* Reserved                                            Default: 0 */

#define ENABLE_EVENTINDICATION_ACTKEY			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_PANID			1	/* Default: 1, must be set to 1 for Fast Restore */
#define ENABLE_EVENTINDICATION_SHORTADDR		1	/* Default: 1, must be set to 1 for Fast Restore */
#define ENABLE_EVENTINDICATION_THERMEV			0	/* Default: 0 */

#define ENABLE_EVENTINDICATION_TMRTX			0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_TMRRX			0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_ROUTE			0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_PREQ				1	/* Default: 1 */

#define ENABLE_EVENTINDICATION_SURGEEV			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_RTEWARN			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_RTEPOLL			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_BOOTMAXRETIES	1	/* Default: 1 */

#define ENABLE_EVENTINDICATION_BANDMISMATCH		1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_PHY_RX_QI		0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_PHY_TX_QI		0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_MACUPDATE		1	/* Default: 1 */

#define ENABLE_EVENTINDICATION_MEMLEAK			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_TIMERERR			1	/* Default: 1 */
#define ENABLE_EVENTINDICATION_PHY_RX_RF_QI		0	/* Default: 0 */
#define ENABLE_EVENTINDICATION_PHY_TX_RF_QI		0	/* Default: 0 */

#define ENABLE_EVENTINDICATION_ADPDATAFORWARD	0	/* Default: 0 */
#if ENABLE_FAST_RESTORE
#define ENABLE_EVENTINDICATION_FASTRESTORE		1	/* Default: 0, must be set to 1 for Fast Restore */
#else
#define ENABLE_EVENTINDICATION_FASTRESTORE		0	/* Default: 0, must be set to 1 for Fast Restore */
#endif /* ENABLE_FAST_RESTORE */

/* Settings for Debug prints */
/* Enable specific debug prints and sets the verbose level: DEBUG_LEVEL_NONE, DEBUG_LEVEL_CRITICAL, DEBUG_LEVEL_WARNING, DEBUG_LEVEL_INFO or DEBUG_LEVEL_FULL */

/* G3 protocol debug prints */
#define DEBUG_G3_MSG            DEBUG_LEVEL_WARNING	/* G3 message exchange */
#define DEBUG_G3_CONF           DEBUG_LEVEL_WARNING	/* G3 Configuration */
#define DEBUG_G3_BOOT           DEBUG_LEVEL_WARNING	/* G3 Boot */
#define DEBUG_G3_BOOT_SRV       DEBUG_LEVEL_WARNING	/* G3 Boot Server (Coordinator only) */
#define DEBUG_G3_BOOT_CLT		DEBUG_LEVEL_WARNING	/* G3 Boot Client (Device only) */
#define DEBUG_G3_PANSORT        DEBUG_LEVEL_WARNING	/* G3 Boot PAN sorting (Device only) */
#define DEBUG_G3_KA             DEBUG_LEVEL_WARNING	/* G3 Keep-Alive */
#define DEBUG_G3_LAST_GASP      DEBUG_LEVEL_WARNING	/* G3 Last Gasp */

/* User application debug prints */
#define DEBUG_USER_G3           DEBUG_LEVEL_WARNING	/* User G3 */
#define DEBUG_USER_IT			DEBUG_LEVEL_WARNING	/* User Image Transfer */

/* Other debug prints */
#define DEBUG_SFLASH            DEBUG_LEVEL_WARNING	/* External SPI FLASH */


/* Settings for Memory pool debug */

/* Enable specific debug prints and sets the level: MEMPOOL_DEBUG_NONE, MEMPOOL_DEBUG_LOW, MEMPOOL_DEBUG_MEDIUM or MEMPOOL_DEBUG_HIGH, MEMPOOL_DEBUG_MAX */
#define MEMPOOL_DEBUG			MEMPOOL_DEBUG_NONE

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
