/*****************************************************************************
*   @file    hi_g3lib_sap_interface.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the G3_LIB-SAP Interface.
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
#ifndef HI_G3LIB_SAP_INTERFACE_H
#define HI_G3LIB_SAP_INTERFACE_H

#include <stdint.h>
#include <hi_g3lib_attributes.h>
#include <hi_mac_message_catalog.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_g3lib_drv HIF G3LIB DRV
  * @{
  */

/** @defgroup G3_LIB G3_LIB
  * @brief    G3 LIB Management Module
  * @{
  */

/** @defgroup G3_LIB_Sap_Interface G3_LIB-SAP Interface
  * @brief This module defines the SAP between the G3_LIB and the APPLICATION Layers
  * @{
  */

/** @defgroup G3_LIB_SAP_IF_TypesDefinitions G3_LIB-SAP Interface Types Definitions
  * @brief  G3_LIB Sap Interface types definitions
  * @{
  */

typedef enum eventnot_type_enum
{
	rte_error 				= 0x00,
	gmk_update 				= 0x01,
	context_table_update 	= 0x02,
	group_table_update 		= 0x03,
	active_index_update 	= 0x04,
	short_addr_update 		= 0x05,
	pan_id_update 			= 0x06,
	tonemap_tx 				= 0x07,
	tonemap_rx 				= 0x08,
	route_update 			= 0x09,
	path_req_rx 			= 0x0A,
	thermal_event 			= 0x0B,
	surge_event 			= 0x0C,
	rte_warning_event 		= 0x0D,
	rte_polling_error 		= 0x0E,
	boot_failure 			= 0x0F,
	rte_band_mismatch 		= 0x10,
	phy_quality_rx 			= 0x11,
	phy_quality_tx 			= 0x12,
	ext_addr_update 		= 0x13,
	mem_leak_info 			= 0x14,
	timer_error 			= 0x15,
	phy_quality_rx_rf 		= 0x16,
	phy_quality_tx_rf 		= 0x17,
	adp_data_fw 			= 0x18,
	fast_restore			= 0x19,
} eventnot_type_t;

#pragma pack(1)

typedef struct LOADng_PREX_Hop_Info_str
{
	uint16_t Address;
	uint8_t Reserved 	: 2;
	uint8_t MTx 		: 1;
	uint8_t MRx 		: 1;
	uint8_t PhaseDiff 	: 3;
	uint8_t MNS 		: 1;
	uint8_t LinkCost;
} LOADng_PREX_Hop_Info_t; /* 4 bytes */

typedef struct eventnot_tonemap_str
{
	MAC_DeviceAddress_t mac_addr;
	union
	{
		mac_tonemap_resp_payload_cena_t		CEN_ToneMapRspPayload;  /**< @brief Tone Map Response payload for G3 CEN-A and CEN-B */
		mac_tonemap_resp_payload_fcc_t		FCC_ToneMapRspPayload;  /**< @brief Tone Map Response payload for G3 FCC */
	} tonemap;
} eventnot_tonemap_t;

typedef struct eventnot_routeupdate_str
{
	ADP_RoutingTableEntry_t routing_table_entry;
	uint8_t  originator;
} eventnot_routeupdate_t;

typedef struct eventnot_pathrequest_str
{
	uint16_t preq_len;
	uint8_t  preq_type;
	uint16_t preq_dst;
	uint16_t preq_orig;
	uint8_t  preq_extdata[4];
	LOADng_PREX_Hop_Info_t preq_hopinfos[];
} eventnot_pathrequest_t;

typedef struct eventnot_thermalevent_str
{
	uint8_t thermal_info;
} eventnot_thermalevent_t;

typedef struct eventnot_bootfailure_str
{
	uint16_t pan_id;
	uint8_t lqi;
	uint16_t lba_addr;
	uint16_t route_cost;
	uint8_t media_type;
	uint8_t lba_join_attempts;
	uint8_t status_code;
} eventnot_bootfailure_t;

typedef struct eventnot_rtebandmismatch_str
{
	uint8_t rte_band;
} eventnot_rtebandmismatch_t;

typedef struct eventnot_phyqualityrx_str
{
	MAC_DeviceAddress_t mac_addr;
	PHY_IndParams_t phy_params;
} eventnot_phyqualityrx_t;

typedef struct eventnot_phyqualitytx_str
{
	MAC_DeviceAddress_t mac_addr;
	uint8_t signal[PHY_NUM_OF_CARRIERS];
} eventnot_phyqualitytx_t;

typedef struct eventnot_error_str
{
	uint8_t debug_data[1024];
} eventnot_error_t;

typedef struct eventnot_phyqualityrx_rf_str
{
	MAC_DeviceAddress_t mac_addr;
	uint8_t lqi;
	uint8_t background_noise;
} eventnot_phyqualityrx_rf_t;

typedef struct eventnot_phyqualitytx_rf_str
{
	MAC_DeviceAddress_t mac_addr;
	uint8_t rf_transmitpower;
	uint8_t duty_cycle;
} eventnot_phyqualitytx_rf_t;

typedef struct eventnot_adpdatafw_str
{
	MAC_DeviceAddress_t mac_addr;
	uint8_t data_type;
} eventnot_adpdatafw_t;

typedef struct eventnot_fastrestoreinfo_str
{
    uint8_t EAP_msg;		/**< @brief Tracks which EAP message (#2 or #3) or ConfigParameter is shared (#43: gmk-act or #47: gmk-rem) */
    uint16_t lenght;		/**< @brief The EAP#x packet lenght (or Param lenght) */
	eap_msg_t eap_msg;		/**< @brief The EAP#x packet (or ConfigParam) to be shared with application */
} eventnot_fastrestoreinfo_t;

/**
  * @brief  Structure of the G3LIB-GET.Request
  */
typedef struct G3_LIB_GetAttributeRequest_str
{
    G3_LIB_PIB_ID_t attribute_id;      /**< @brief The ID+Index */
} G3_LIB_GetAttributeRequest_t;

/**
  * @brief  Structure of the G3LIB-GET.Confirm
  */
typedef struct G3_LIB_GetAttributeConfirm_str
{
    uint8_t status;                 /**< @brief The result of the operation  */
    G3_LIB_PIB_t attribute;         /**< @brief The attribute information */
} G3_LIB_GetAttributeConfirm_t;

/**
  * @brief  Structure of the G3LIB-SET.Request command
  */
typedef struct G3_LIB_SetAttributeRequest_str
{
    G3_LIB_PIB_t attribute;          /**< @brief The attribute information */
} G3_LIB_SetAttributeRequest_t;

/**
  * @brief  Structure of the G3LIB-SET.Confirm command
  */
typedef struct G3_LIB_SetAttributeConfirm_str
{
    uint8_t  status;                 /**< @brief The result of the operation  */
    G3_LIB_PIB_ID_t attribute_id;       	 /**< @brief The ID+Index */
} G3_LIB_SetAttributeConfirm_t;

/**
  * @brief  Structure of the G3LIB-SWRESET.Request
  */
typedef struct G3_LIB_SWResetRequest_str
{
    uint8_t bandplan;       /**< @brief The version of the G3 Library  */
    uint8_t device_type;    /**< @brief The G3 Device type  */
    uint8_t mode	 : 7;	/**< @brief The G3 Mode (PHY, MAC...)  */
    uint8_t req_type : 1;	/**< @brief The request type (could be normal reset or fast restore reset, applicable only if mode is BOOT or IPv6_BOOT  */
} G3_LIB_SWResetRequest_t;

/**
  * @brief  Structure of the G3LIB-SWRESET.Confirm
  */
typedef struct G3_LIB_SWResetConfirm_str
{
    uint8_t status;			/**< @brief The State of the G3LIB-SWRESET operation  */
    uint8_t bandplan;		/**< @brief The version of the G3 Library */
    uint8_t device_type;	/**< @brief The G3 Device type */
    uint8_t mode	 : 7;	/**< @brief The G3 Mode (PHY, MAC...)  */
    uint8_t req_type : 1;	/**< @brief The request type (could be normal reset or fast restore reset, applicable only if mode is BOOT or IPv6_BOOT  */
} G3_LIB_SWResetConfirm_t;

/**
  * @brief  Structure of the G3LIB-TESTMODEENABLE.Request
  */
typedef struct G3_LIB_TestModeRequest_str
{
    uint8_t ID;                     /**< @brief The ID of the Test Mode to set  */
} G3_LIB_TestModeRequest_t;

/**
  * @brief  Structure of the G3LIB-TESTMODEENABLE.Confirm
  */
typedef struct G3_LIB_TestModeConfirm_str
{
    uint8_t status;                 /**< @brief The State of the G3LIB-TESTMODEENABLE-CNF  */
} G3_LIB_TestModeConfirm_t;

/**
  * @brief  Structure of the G3LIB-EVENTNOTIFICATION-IND
  */
typedef struct G3_LIB_EventNotificationIndication_str
{
    uint8_t type;                 /**< @brief The State of the G3LIB-EVENTNOTIFICATION-IND  */
    union
	{
    	eventnot_tonemap_t 			tonemap;
    	eventnot_routeupdate_t		route_update;
    	eventnot_pathrequest_t		path_request;
    	eventnot_thermalevent_t 	thermal_event;
    	eventnot_bootfailure_t		boot_failure;
    	eventnot_rtebandmismatch_t	rte_band_mismatch;
    	eventnot_phyqualityrx_t		phy_quality_rx;
    	eventnot_phyqualitytx_t		phy_quality_tx;
    	eventnot_error_t			error;
    	eventnot_phyqualityrx_rf_t	phy_quality_rx_rf;
    	eventnot_phyqualitytx_rf_t	phy_quality_tx_rf;
    	eventnot_adpdatafw_t		adp_data_fw;
    	eventnot_fastrestoreinfo_t	fast_restore_info;
    } info;
} G3_LIB_EventNotificationIndicationt;




/**
  * @}
  */

/**
  * @}
  */

#pragma pack()

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#endif //HI_G3LIB_SAP_INTERFACE_H

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
