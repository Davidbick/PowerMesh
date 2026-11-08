/*****************************************************************************
*   @file    g3_app_boot.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the boot application.
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
#ifndef G3_APP_BOOT_H_
#define G3_APP_BOOT_H_

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
#include <settings.h>
#include <g3_boot_types.h>
#include <g3_app_boot_srv.h>
#include <g3_app_boot_clt.h>
#include <hi_boot_sap_interface.h>

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

#if IS_COORD
#define G3_APP_MAX_DEVICES_IN_PAN 			16U			/* Maximum number of devices that the coordinator can manage */
#define KEEP_ALIVE_LIVES_N					2U			/* Number of pings that must fail before the kick-out */
#else
#define DEVICE_RECONN_TIME               	(30000U)	/* Reconnection time for the device */
#endif

typedef struct lba_info_struct
{
	uint16_t short_address;
	uint8_t media_type;
} lba_info_t;


/* Public functions */

#if IS_COORD

void 			g3_app_boot_tbl_init(void);

/* Connected device list */
boot_device_t* g3_app_boot_find_first_device(const boot_conn_state_t status_mask);
boot_device_t* g3_app_boot_find_device(const uint8_t* ext_addr, const uint16_t short_addr, const boot_conn_state_t status_mask);
boot_device_t* g3_app_boot_get_device_by_index(const uint16_t index, const boot_conn_state_t status_mask);
boot_device_t* g3_app_boot_add_bootstrapping_device(const uint8_t* ext_addr, uint16_t short_address);
#if ENABLE_BOOT_SERVER_ON_HOST
boot_device_t* g3_app_boot_add_connected_device(boot_join_entry_t* join);
#else
boot_device_t* g3_app_boot_add_connected_device(const BOOT_ServerJoinIndication_t* join);
#endif /* ENABLE_BOOT_SERVER_ON_HOST */

bool 		   g3_app_boot_remove_connected_device(const uint8_t* ext_addr);

#endif /* IS_COORD */

/* Device Management */
#if IS_COORD
void 		   g3_app_boot_kick_device(boot_device_t *device);
#else
char* 		   g3_app_pansort_translate_media_type(uint8_t media);
void 		   g3_app_boot_leave();
void 		   g3_app_boot_restart();
#endif /* IS_COORD */

/* Boot module */
void 		   g3_app_boot_init(void);
bool 		   g3_app_boot_msg_needed(const g3_msg_t *g3_msg);
void 		   g3_app_boot_msg_handler(const g3_msg_t *g3_msg);

#if !IS_COORD
void g3_app_boot_dev_timeoutCallback(void *argument);
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

#ifdef __cplusplus
}
#endif

#endif /* G3_APP_BOOT_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
