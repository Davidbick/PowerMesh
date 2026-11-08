/*****************************************************************************
*   @file    hi_mac_sap_interface.h
*   @author  AMG/IPC Application Team
*   @brief   Include file with the definitions related to the interface between the MAC layer and ADP layers
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/** @defgroup MAC MAC
  * @brief    G3 MAC Management Module
  * @{
  */

/** @defgroup MAC_SAP_Interface MAC-SAP Interface
  * @brief    Interface between MAC and ADP (or upper) layers
  * @details  This module contains the implementation of the interface between the MAC and ADP Layers.
  * @{
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HI_MAC_SAP_INTERFACE_H__
#define HI_MAC_SAP_INTERFACE_H__

#include <stdint.h>
#include <hi_mac_message_catalog.h>
#include <hi_mac_constants.h>
#include <hi_g3lib_attributes.h>

/** @defgroup MAC_SAP_IF_TypesDefinitions MAC-SAP Interface Types Definitions
  * @brief  MAC Sap Interface types definitions
  * @{
  */

/* MAC Commands parameters */
#define MAC_ACTIVE_SCAN                     ((uint8_t)0x01) /**< @brief Active Scan Type */
#define MAC_MAX_SCANDURATION                ((uint8_t)60)   /**< @brief Maximum Scan Duration (in seconds) */
#define MAC_SETPIBATTRIBUTE_DEFAULT_OFF     ((uint8_t)0x00) /**< @brief Property used by the MLME-RESET.Request SAP: do not reset PIB Attributes */
#define MAC_SETPIBATTRIBUTE_DEFAULT_ON      ((uint8_t)0x01) /**< @brief Property used by the MLME-RESET.Request SAP: reset PIB Attributes     */
#define MAC_QOS_MAX_VALUE                   ((uint8_t)0x01) /**< @brief The maximum value for the Quality of Service field  */
#define MAC_KEYINDEX_MAX_VALUE              ((uint8_t)0x01) /**< @brief The maximum value for the KeyIndex field  */

#define MAC_LQI_TO_SNR_OFFSET_PLC			(10)
#define MAC_LQI_TO_SNR_STEP_PLC				(0.25)
#define MAC_LQI_TO_RSSI_OFFSET_RF			(174)
#define MAC_NOTMEASURED_LQI_RF              (0xFF) /**< @brief The LQI value 0xFF represents a "not measured" LQI */

#define MAC_DBUV_TO_V(dbuv)					((float) (pow(10, ((((double) (dbuv)) / 20) - 6))))
#define MAC_V_TO_DBUV(volt)					((float) (20 * (log10((double) (volt)) + 6)))

#define V_PEAK_TO_AVG_POWER_RATION			10 /* In dB */

#pragma pack(1)

/**
  * @struct  MAC_McpsDataReq_t
  * @brief   Structure of the MCPS-DATA.Request Command
  * @var     MAC_McpsDataReq_t::SrcAddrMode
  * @details The source addressing mode for this primitive and subsequent MPDU. This value can take one of the following values:
  *          @n 0x00 = no address (addressing fields omitted)
  *          @n 0x01 = reserved
  *          @n 0x02 = 16-bit short address
  *          @n 0x03 = 64-bit extended address
  * @var     MAC_McpsDataReq_t::DstAddr
  * @details @li The destination addressing mode for this primitive and subsequent MPDU. This value can take one of the following values:
  *          @n 0x00 = no address (addressing fields omitted)
  *          @n 0x01 = reserved
  *          @n 0x02 = 16-bit short address
  *          @n 0x03 = 64-bit extended address
  *          @li The 16-bit PAN identifier of the entity to which the MSDU is being transferred
  *          @li The individual device address of the entity to which the MSDU is being transferred
  * @var     MAC_McpsDataReq_t::TxOptions
  * @details b0: 0 transmission without ACK, 1 transmission with ACK
  * @var     MAC_McpsDataReq_t::SecurityLevel
  * @details 0x00: none, 0x05: "ENC-MIC-32"
  * @var     MAC_McpsDataReq_t::QoS
  * @details 0: Normal Priority, 1: High Priority, 2: Contention free
  * @var     MAC_McpsDataReq_t::MediaType
  * @details 0: PLC, 1: RF, 2: PLC 1st option, RF 2nd option. 3: RF 1st option, PLC 2nd option
  */
typedef struct
{
    uint8_t  src_addr_mode;             	/**< @brief Source Address Mode  */
    MAC_DeviceAddress_t dst_addr;       	/**< @brief Destination address information  */
    uint16_t msdu_len;               		/**< @brief The length of the data to transmit  */
    uint8_t  msdu[MAC_PAYLOAD_MAX_SIZE];	/**< @brief The set of octets forming the MSDU to be transmitted  */
    uint8_t  msdu_handle;               	/**< @brief The handle associated with the data to transmit  */
    uint8_t  tx_options;                	/**< @brief Transmission Options  */
    uint8_t  security_level;            	/**< @brief The security level to be used: 0x00 no Security - 0x05 ENC-MIC-32 with CCM*  */
    uint8_t  key_index;                 	/**< @brief The index of the Key to be used: 0x00 or 0x01 (not present if there is no security) */
    uint8_t  qos;                      		/**< @brief Quality of Service  */
    uint8_t  media_type;                	/**< @brief The channel over which the frame has to be transmitted */
} MAC_DataReq_t;

/**
  * @struct  MAC_McpsDataConfirm_t
  * @brief   Structure of the MCPS-DATA.Confirm Command
  * @var     MAC_McpsDataConfirm_t::Status
  * @details Could be: @n 0: SUCCESS @n 1: TRANSACTION_OVERFLOW @n 2: TRANSACTION_EXPIRED
  *          @n 3: CHANNEL_ACCESS_FAILURE @n 4: INVALID_ADDRESS @n 5: INVALID_GTS
  *          @n 6: NO_ACK @n 7: COUNTER_ERROR @n 8: FRAME_TOO_LONG
  *          @n 9: UNAVAILABLE_KEY @n 10: UNSUPPORTED_SECURITY @n 11: INVALID_PARAMETER
  * @var     MAC_McpsDataConfirm_t::MediaType
  * @details 0: PLC, 1: RF
  */
typedef struct
{
    uint8_t  msdu_handle;              /**< @brief The msduHandle specified in the MCPS-DATA.Request  */
    uint8_t  status;                  /**< @brief The status of the Data Transfer operation */
    uint32_t timestamp;               /**< @brief The time at which the data were transmitted */
    uint8_t  media_type;               /**< @brief The media type over which the frame has been transmitted */
} MAC_DataConfirm_t;

/**
  * @struct  MAC_McpsDataIndication_t
  * @brief   Structure of the MCPS-DATA.Indication Command
  * @var     MAC_McpsDataIndication_t::SrcAddr
  * @details @li The source addressing mode for this primitive and subsequent MPDU. This value can take one of the following values:
  *          @n 0x00 = no address (addressing fields omitted)
  *          @n 0x01 = reserved
  *          @n 0x02 = 16-bit short address
  *          @n 0x03 = 64-bit extended address
  *          @li The 16-bit PAN identifier of the entity to which the MSDU is being transferred
  *          @li The individual device address of the entity to which the MSDU is being transferred
  * @var     MAC_McpsDataIndication_t::DstAddr
  * @details @li The destination addressing mode for this primitive and subsequent MPDU. This value can take one of the following values:
  *          @n 0x00 = no address (addressing fields omitted)
  *          @n 0x01 = reserved
  *          @n 0x02 = 16-bit short address
  *          @n 0x03 = 64-bit extended address
  *          @li The 16-bit PAN identifier of the entity to which the MSDU is being transferred
  *          @li The individual device address of the entity to which the MSDU is being transferred
  * @var     MAC_McpsDataIndication_t::QoS
  * @details 0: Normal Priority, 1: High Priority, 2: Contention free
  * @var     MAC_McpsDataIndication_t::SecurityLevel
  * @details 0x00 no Secuirity, 0x05 ENC-MIC-32 with CCM*
  * @var     MAC_McpsDataIndication_t::MediaType
  * @details 0: PLC, 1: RF
  */
typedef struct
{
    MAC_DeviceAddress_t src_addr;            	/**< @brief Address information of the source node  */
    MAC_DeviceAddress_t dst_addr;             	/**< @brief Address information of the destination node  */
    uint16_t msdu_len;                        	/**< @brief The length of the received data  */
    uint8_t  msdu[MAC_PAYLOAD_MAX_SIZE];        /**< @brief The received data  */
    uint8_t  dsn;                               /**< @brief The received Data Sequence Number  */
    uint32_t timestamp;                         /**< @brief The time at which the frame was received  */
    uint8_t  security_level;                  	/**< @brief The security level used: 0x00 no Security - 0x05 ENC-MIC-32 with CCM*   */
    uint8_t  key_index;                       	/**< @brief The index of the key used  */
    uint8_t  qos;                               /**< @brief Quality of Service  */
    PHY_IndParams_t phy_params;               	/**< @brief Channel quality parameters estimated by the PHY Layer */
    uint8_t  media_type;                       	/**< @brief The channel over which the frame has been received */
} MAC_DataIndication_t;

/**
  * @brief  Structure of the MLME-BEACON-NOTIFY.Indication Command
  */
typedef struct
{
    uint16_t PanID;             /**< @brief The 16-bit PAN identifier */
    uint16_t LBAAddress;        /**< @brief The 16 bit short address of a device in this PAN to be used as the LBA by the associating device */
    uint16_t RC_COORD;          /**< @brief The estimated route cost from LBA to the coordinator */
    PHY_IndParams_t PhyParams;  /**< @brief Channel quality parameters estimated by the PHY Layer */
#if COMPILE_7_X_VERSION
    uint32_t FH_ScheduleStartTime;  /**< @brief Schedule Start Time to be used in FH mode for calculating Slot Number and offset */
#endif /* COMPILE_7_X_VERSION */
    uint8_t  MediaType;			/**< @brief The channel over which the frame has been received */
} MAC_MlmeBeaconNotify_t;

/**
  * @brief  Structure of the MLME-RESET.Request Command
  */
typedef struct
{
  uint8_t    SetDefaultPIB;   /**< @brief 0x00: Maintain the current PIB Attribute values, 0x01: Reset PIB Attributes to their default values  */
} MAC_MlmeResetReq_t;

/**
  * @brief  Structure of the MLME-RESET.Confirm Command
  */
typedef struct
{
    uint8_t   Status;        /**< @brief The state of the reset operation  */
} MAC_MlmeResetConfirm_t;

/**
  * @brief  Structure of the MLME-SCAN.Request Command
  * @details Scan support only Active mode and no security is used
  */
typedef struct
{
    uint8_t   ScanType;         /**< @brief Scan Type: Only active scan allowed  */
    uint8_t   ScanDuration;     /**< @brief The duration of the scan (in seconds)  */
} MAC_MlmeScanReq_t;

/**
  * @struct  MAC_MlmeScanConfirm_t
  * @brief   Structure of the MLME-SCAN.Confirm Command
  * @var     MAC_MlmeScanConfirm_t::Status
  * @details The status of MlmeScanConfirm could be: @n SUCCESS, LIMIT_REACHED, NO_BEACON,
  *          SCAN_IN_PROGRESS, COUNTER_ERROR, FRAME_TOO_LONG, UNAVAILABLE_KEY, UNSUPPORTED_SECURITY, INVALID_PARAMETER
  */
typedef struct
{
    uint8_t   Status;                          /**< @brief The state of the scan operation  */
    uint8_t   ScanType;                        /**< @brief Scan Type: Only active scan allowed  */
} MAC_MlmeScanConfirm_t;

/**
  * @brief  Structure of the MLME-COMM-STATUS.Indication Command
  */
typedef struct
{
    uint8_t   Status;                            /**< @brief The communications status  */
    MAC_DeviceAddress_t SrcAddr;                 /**< @brief The individual device address of the entity from which the frame causing the error originated  */
    MAC_DeviceAddress_t DstAddr;                 /**< @brief The individual device address of the device for which the frame was intended */
    uint8_t   SecurityLevel;                     /**< @brief The security level used by the originator of the received frame */
    uint8_t   KeyIdMode;                         /**< @brief The mode used to identify the key used by the originator of the received frame  */
    uint8_t   KeyIndex;                          /**< @brief The index of the key used by the originator of the received frame  */
    uint8_t   SNRCarriers[PHY_NUM_OF_CARRIERS];  /**< @brief The SNR measurements of each carrier */
    uint8_t   Lqi;                               /**< @brief The Link Quality Indicator */
    uint8_t   MediaType;                       /**< @brief The channel over which the frame has been received */
} MAC_MlmeCommStatusIndication_t;

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

/**
  * @}
  */
#endif /* HI_MAC_SAP_INTERFACE_H__ */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
