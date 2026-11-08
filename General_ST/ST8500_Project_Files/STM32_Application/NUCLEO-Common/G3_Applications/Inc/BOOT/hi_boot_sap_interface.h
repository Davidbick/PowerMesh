/*****************************************************************************
*   @file    hi_boot_sap_interface.h
*   @author  AMG/IPC Application Team
*   @brief   This code include all functionalities of the interface between Bootstrap Handler and SAP levels.
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
#ifndef HI_BOOT_SAP_INTERFACE_H
#define HI_BOOT_SAP_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_boot_drv HIF BOOT DRV
  * @{
  */

/* Definitions */

/** @defgroup Boot_SAP_Interface BOOT SAP Interface
  * @brief    Interface between Bootstrap Layer and upper layers
  * @details  This module contains the implementation of the interface between the Bootstrap Handler Application and upper layers.
  * @{
  */
#include <hi_adp_lbp_message_catalog.h>
#include <hi_adp_sap_interface.h>
#include <hi_g3lib_attributes.h>

/** @defgroup BOOT_SAP_DEFINE BOOT SAP Interface Definition
  * @brief    Bootstrap Layer Interface Definition
  * @details  These values are used in the BOOT SAP Interface
  * @{
  */

#define BOOT_EAP_PSK_KEY_LEN            ((uint8_t) 0x10)  /**< @brief Length of the EAP-PSK Pre Shared Key */
/**
  * @}
  */


/** @struct BOOT_States_t
  * @brief    enumeration for the various states the boostrap process can take for the device
  * @brief    Bootstrap Layer Interface Len Values
  * @details  These values are used to handle SAP Primitives parameters
  * @{
  */
typedef enum BOOT_States_enum
{
  BOOT_DEV_STATE_IDLE = 0,    /**< @brief The device is not connected and not starting a bootstrap process */
  BOOT_DEV_STATE_WAITING,     /**< @brief The device has started a bootstrap process and it is waiting the proper time to perfom active scan */
  BOOT_DEV_STATE_DISCOVERING, /**< @brief The device is perfoming the active scan */
  BOOT_DEV_STATE_SORTING,     /**< @brief The device is sorting the received PAN descriptors */
  BOOT_DEV_STATE_JOINING,     /**< @brief The device has performed the active scan and it is waiting the proper time to join the network */
  BOOT_DEV_STATE_CONNECTED,   /**< @brief The device is connected */
} BOOT_States_t;
/**
  * @}
  */

/** @struct BOOT_CBKResult_t
  * @brief    Bootstrap Layer Interface Callback Return Values
  * @details  These values are used as return value from the external callbacks called by the Bootstrap Layer
  * @{
  */
typedef enum BOOT_CBKResult_enum
{
    BOOT_CBK_SUCCESS = 0,             /**< @brief The requested operation was completed successfully. */
    BOOT_CBK_EUI64_BLACKLISTED,       /**< @brief The Node is not allowed to join the PAN.  */
    BOOT_CBK_IDP_BLACKLISTED,         /**< @brief The Node is not allowed to join the PAN.  */
    BOOT_CBK_NODE_TABLE_FULL,         /**< @brief The Nodes Table is full and new joining attempts cannot be handled.  */
    BOOT_CBK_NODE_UNKNOWN             /**< @brief The Node cannot be found in the Nodes Table and its request cannot be handled.  */
} BOOT_CBKResult_t;
/**
  * @}
  */

/** @struct BOOT_StartType_t
  * @brief The Bootstrap Layer SAP Start Type Values
  * @details These values are given as parameter to BOOT-SERVER-START.request or BOOT-DEVICE-START.request commands to decide how the bootstrap procedure should start
  * @{
  */
typedef enum BOOT_StartType_enum
{
    BOOT_START_NORMAL = 0,    /**< @brief Normal bootstrap type: create a new PAN (for PAN Coordinator) or discover and connect to existing PAN (for Device) */
    BOOT_START_FAST_RESTORE   /**< @brief Fast Restore bootstrap type: Fast Restore algorithm restores network information of the nodes if the restart happens when the node was already in the network */
} BOOT_StartType_t;
/**
  * @}
  */

/** @struct BOOT_Bandplan_enum
  * @brief The bandplan type values
  * @details These values are given as parameter to G3LIB-SWRESET.request command to decide which bandplan shall be used
  * @{
  */
typedef enum BOOT_Bandplan_enum
{
	BOOT_BANDPLAN_CENELEC_A 		= G3_LIB_G3PLC_CENA,
	BOOT_BANDPLAN_CENELEC_B 		= G3_LIB_G3PLC_CENB,
	BOOT_BANDPLAN_ARIB 				= G3_LIB_G3PLC_ARIB, 		/* Not supported by ST8500 */
	BOOT_BANDPLAN_FCC 				= G3_LIB_G3PLC_FCC,
	BOOT_BANDPLAN_FCC_LOW           = G3_LIB_G3PLC_FCC_LOW,
	BOOT_BANDPLAN_FCC_HIGH          = G3_LIB_G3PLC_FCC_HIGH
} BOOT_Bandplan_t;

/** @struct plcType_enum
  * @brief The dDevice type values
  * @details These values are given as parameter to G3LIB-SWRESET.request command to decide which role shall be used
  * @{
  */
typedef enum plcType_enum
{
  PLC_G3_DEVICE	= ADP_DEVICETYPE_DEVICE,
  PLC_G3_COORD 	= ADP_DEVICETYPE_COORD,
  PLC_G3_NONE 	= ADP_DEVICETYPE_UNKNOWN,
} plcType_t;

/** @struct platform_enum
  * @brief The Platform Type values
  * @details These value is associated to the type of hardware platform.
  * @{
  */
typedef enum platform_enum
{
	PLAT_INVALID = 0x00,
	PLAT_STCOMET_STCOM = 0x01,
	PLAT_ST8500 = 0x02,
	PLAT_UNSPEC = 0xFF
} platform_t;

/** @struct plc_mode_enum
  * @brief The PLC mode values
  * @details These value is associated to the current mode of the platform.
  * @{
  */
typedef enum plc_mode_enum
{
	PLC_MODE_PHY       = 0x0,
	PLC_MODE_MAC       = 0x1,
	PLC_MODE_ADP       = 0x2,
	PLC_MODE_ADP_BOOT  = 0x3,
	PLC_MODE_IPV6_ADP  = 0x4,
	PLC_MODE_IPV6_BOOT = 0x5
} plc_mode_t;

/**
  * @}
  */

#pragma pack(push, 1)

/**
  * @brief  BOOT-SERVER-START.Request parameters
  */
typedef struct BOOT_ServerStartRequest_str
{
    uint16_t server_addr;		/**< @brief The short address used by the server */
    uint16_t pan_id;          	/**< @brief The PAN ID */
    BOOT_StartType_t req_type;	/**< @brief The type of BOOT-SERVER-START.request (#BOOT_StartType_t) */
} BOOT_ServerStartRequest_t;

/**
  * @brief  BOOT-SERVER-START.Confirm parameters
  */
typedef struct BOOT_ServerStartConfirm_str
{
    uint8_t status; /**< @brief Result of the operation */
} BOOT_ServerStartConfirm_t;

/*   BOOT-SERVER-STOP.Request has no payload */

/**
  * @brief  BOOT-SERVER-STOP.Confirm parameters
  */
typedef struct BOOT_ServerStopConfirm_str
{
    uint8_t status; /**< @brief Result of the operation */
} BOOT_ServerStopConfirm_t;

/**
  * @brief  BOOT-SERVER-KICK.Request parameters
  */
typedef struct BOOT_ServerKickRequest_str
{
    uint8_t ext_addr[MAC_ADDR64_SIZE];	/**< @brief The extended address of the node to be kicked */
    uint16_t short_addr;             	/**< @brief The 16-bit address of the node to be kicked */
} BOOT_ServerKickRequest_t;

/**
  * @brief  BOOT-SERVER-KICK.Confirm parameters
  */
typedef struct BOOT_ServerKickConfirm_str
{
    uint8_t status; /**< @brief Result of the operation */
} BOOT_ServerKickConfirm_t;

/**
  * @brief  BOOT-SERVER-LEAVE.Indication parameters
  */
typedef struct BOOT_ServerLeaveIndication_str
{
    uint8_t ext_addr[MAC_ADDR64_SIZE]; /**< @brief The extended address (EUI-64)of the node leaving the network*/
} BOOT_ServerLeaveIndication_t;

/**
  * @brief  BOOT-SERVER-JOIN.Indication parameters
  */
typedef struct BOOT_ServerJoinIndication_str
{
    uint8_t  ext_addr[MAC_ADDR64_SIZE];	/**< @brief The Node extended address (EUI-64)of the node that has joint the network */
    uint16_t short_addr;               	/**< @brief The assigned 16-bit Short Addresss */
} BOOT_ServerJoinIndication_t;

/**
  * @brief  BOOT-SERVER-REKEYING.Request parameters
  */
typedef struct BOOT_ServerRekeyingRequest_str
{
	uint8_t gmk[MAC_KEY_SIZE];
} BOOT_ServerRekeyingRequest_t;

/**
  * @brief  BOOT-SERVER-REKEYING.Confirm parameters
  */
typedef struct BOOT_ServerRekeyingConfirm_str
{
  uint8_t status; /**< @brief Result of the operation */
  uint8_t error;	/**< @brief Error during the operation */
} BOOT_ServerRekeyingConfirm_t;

/**
  * @brief  BOOT-SERVER-ABORT-RK.Confirm parameters
  */
typedef struct BOOT_ServerAbortRekeyingConfirm_str
{
  uint8_t status; /**< @brief Result of the operation */
} BOOT_ServerAbortRekeyingConfirm_t;

/**
  * @brief  BOOT-SERVER-GETPSK.Indication parameters
  */
typedef struct BOOT_ServerGetPSKIndication_str
{
    uint8_t ext_addr[MAC_ADDR64_SIZE]; /**< @brief The extended address (EUI-64) of the node joining the network */
    uint8_t idp_len;                        /**< @brief The Length of the EAP peer NAI (IdP) */
    uint8_t idp[ADP_EAP_PSK_ID_MAX_LEN];      /**< @brief The EAP peer NAI (IdP) */
} BOOT_ServerGetPSKIndication_t;

/**
  * @brief  BOOT-SERVER-SETPSK.Request parameters
  */
typedef struct BOOT_ServerSetPSKRequest_str
{
    uint8_t  ext_addr[MAC_ADDR64_SIZE];	/**< @brief The extended address (EUI-64) of the node joining the network */
    uint8_t  psk[ADP_EAP_PSK_KEY_LEN]; 	/**< @brief The PSK to be used for EAP-PSK exchange with the peer */
    uint16_t short_addr;              	/**< @brief The short address to be assigned to the peer */
} BOOT_ServerSetPSKRequest_t;

/**
  * @brief  BOOT-SERVER-SETPSK.Confirm parameters
  */
typedef struct BOOT_ServerSetPSKConfirm_str
{
    uint8_t status; 				/**< @brief Result of the operation */
} BOOT_ServerSetPSKConfirm_t;

/**
  * @brief  BOOT-DEVICE-START.Request parameters
  */
typedef struct BOOT_DeviceStartRequest_str
{
    BOOT_StartType_t req_type; 	/**< @brief The type of BOOT-DEVICE-START.request (#BOOT_StartType_t) */
} BOOT_DeviceStartRequest_t;

/**
  * @brief  BOOT-DEVICE-START.Confirm parameters
  */
typedef struct BOOT_DeviceStartConfirm_str
{
    uint8_t  status;        /**< @brief Result of the operation */
    uint16_t network_addr; 	/**< @brief The short address used by the node  */
    uint16_t pan_id;        /**< @brief The PAN ID */
} BOOT_DeviceStartConfirm_t;

/* BOOT-DEVICE-LEAVE.Request has no payload */

/**
  * @brief  BOOT-DEVICE-LEAVE.Confirm parameters
  */
typedef struct BOOT_DeviceLeaveConfirm_str
{
    uint8_t status; /**< @brief Result of the operation */
} BOOT_DeviceLeaveConfirm_t;

/* BOOT-DEVICE-LEAVE.Indication has no payload */

/**
  * @brief  BOOT-DEVICE-PANSORT.Indication parameters
  */
typedef struct BOOT_DevicePANSortIndication_str
{
    uint8_t 			pan_count;                              /**< @brief The number of received PAN descriptors  */
    ADP_PanDescriptor_t pan_descriptor[ADP_MAX_NUM_PANDESCR];	/**< @brief The array of received PAN descriptors */
} BOOT_DevicePANSortIndication_t;

/**
  * @brief  BOOT-DEVICE-PANSORT.Request parameters
  */
typedef struct BOOT_DevicePANSortRequest_str
{
    uint8_t pan_count;                                       	/**< @brief The number of selected PAN Descriptors  */
    ADP_PanDescriptor_t pan_descriptor[ADP_MAX_NUM_PANDESCR];	/**< @brief The array of selected and ordered PAN Descriptors */
} BOOT_DevicePANSortRequest_t;

/**
  * @brief  BOOT-DEVICE-PANSORT.Confirm parameters
  */
typedef struct BOOT_DevicePANSortConfirm_str
{
    uint8_t status; /**< @brief Result of the operation */
} BOOT_DevicePANSortConfirm_t;

#pragma pack(pop)

/* Public Functions */
char* hi_boot_sap_translate_dev_type(plcType_t dev_type);
char* hi_boot_sap_translate_bandplan(BOOT_Bandplan_t bandplan);
char* hi_boot_sap_translate_platform(uint32_t platform);
char* hi_boot_sap_translate_mode(uint32_t plc_mode);

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

#endif /* HI_BOOT_SAP_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
