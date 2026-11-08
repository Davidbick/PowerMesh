/*****************************************************************************
*   @file    g3_app_config.h
*   @author  AMG/IPC Application Team
*   @brief   Header file that contains declarations for G3 configuration functionalities.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef G3_APP_CONFIG_H_
#define G3_APP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup G3_App_Config G3 Configuration Application
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Config_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_Config_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <g3_comm.h>
#include <hi_boot_sap_interface.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Config
  * @{
  */

/** @addtogroup G3_App_Config_Exported_Code
  * @{
  */

/* Definitions */
/* Attribute values */
#define PAN_ID          0x781D	/* PAN ID to set */
#define COORD_ADDRESS   0x0000	/* Coordinator short address to set, must be set to 0 */

/* Default PSK value */
#define DEFAULT_PSK {	0xABU, 0x10U, 0x34U, 0x11U, \
						0x45U, 0x11U, 0x1BU, 0xC3U, \
						0xC1U, 0x2DU, 0xE8U, 0xFFU, \
						0x11U, 0x14U, 0x22U, 0x04U }

/* Default GMK value */
#define DEFAULT_GMK	{	0xAF, 0x4D, 0x6D, 0xCC, \
		                0xF1, 0x4D, 0xE7, 0xC1, \
						0xC4, 0x23, 0x5E, 0x6F, \
						0xEF, 0x6C, 0x15, 0x1F }

/* Default GMK index */
#define DEFAULT_GMK_INDEX	0

/* Custom types */
#pragma pack(push, 1)
typedef struct fast_restore_data_nvm_str
{
	uint32_t 	fc; 							/* macFrameCounter */
	uint32_t 	fc_rf; 							/* macFrameCounter_RF */
	uint16_t 	lg_sn;							/* adpLOADngSequenceNumber */
	uint8_t  	pad[2];
	uint32_t 	lg_sn_extra;
	uint32_t 	fc_extra;
	uint32_t 	fc_rf_extra;
	uint8_t  	reserved3[2];
	uint16_t 	crc;
	uint16_t 	pan_id;
} fast_restore_data_nvm_t;

typedef struct key_data_str
{
	uint8_t  key_free;			/* Key free indication (0: key0 present, other: key not preset) */
	uint8_t  key[MAC_KEY_SIZE];
} key_data_t;

typedef struct fast_restore_data_flash_str
{
	uint16_t 	short_addr;
	uint8_t  	active_key_index;
	uint8_t		fast_restore_enabled;
	uint8_t  	dummy1;
	uint8_t  	dummy2;
	key_data_t	key_data[MAC_KEY_NUMBER];
} fast_restore_data_flash_t;

typedef union flash_data_uni
{
	uint64_t	 				array[sizeof(fast_restore_data_flash_t)/8];
	fast_restore_data_flash_t	str;
} flash_data_t;

typedef struct fast_restore_data_str
{
	uint32_t 	fc; 		/* macFrameCounter */
	uint32_t 	fc_rf; 		/* macFrameCounter_RF */
	uint16_t 	lg_sn;		/* adpLOADngSequenceNumber */
	uint16_t 	pan_id;
	uint16_t 	short_addr;
	uint8_t  	active_key_index;
	key_data_t	key_data[MAC_KEY_NUMBER];
} fast_restore_data_t;
#pragma pack(pop)



/* Public Functions */
char* g3_app_translate_g3_result(g3_result_t g3_result);

void g3_app_conf_init(void);
void g3_app_conf_start(bool fast_restore_en);
void g3_app_conf(void);

bool g3_app_conf_msg_needed( const g3_msg_t *g3_msg);
void g3_app_conf_msg_handler(const g3_msg_t *g3_msg);

bool g3_app_conf_ready(void);

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

#endif /* G3_APP_CONFIG_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
