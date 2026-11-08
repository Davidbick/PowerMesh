/*****************************************************************************
*   @file    hi_adp_sap_interface.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the interface between ADP and SAP levels.
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
#ifndef HI_ADP_SAP_INTERFACE_H
#define HI_ADP_SAP_INTERFACE_H

#include <stdint.h>
#include <hi_adp_pib_attributes.h>
#include <hi_g3lib_attributes.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_adp_drv HIF ADP DRV
  * @{
  */

/** @defgroup ADP_SAP_Interface ADP-SAP Interface
  * @brief    Interface between ADP and upper layers
  * @details  This module contains the implementation of the interface between the 6LoWPAN based ADP and upper layers.
  * @{
  */

/** @defgroup ADP_SAP_IF_TypesDefinitions ADP-SAP Interface Types Definitions
  * @brief  ADP Sap Interface types definitions
  * @{
  */

/* ADP Commands parameters */
#define ADP_MAX_SDU_SIZE                ((uint16_t)1600) /**< @brief ADP<->IPv6 Layer Primitive maximum size of supported IPv6 packet  */
#define ADP_MAX_CTRL_PKT_SIZE           ((uint16_t)400)  /**< @brief ADP<->IPv6 Layer Primitive maximum size of supported internal/control packet  */
#define ADP_MAX_NUM_PANDESCR            ((uint8_t)64)    /**< @brief ADP<->IPv6 Layer Primitive maximum number of supported PAN Descriptors  */
#define ADP_MAX_STR_LEN                 ((uint8_t)128)   /**< @brief ADP<->IPv6 Layer Primitive maximum length of the string returned by ADPM-NETWORK-STATUS.Indication */
#define ADP_MAX_NUM_BC0                 ((uint8_t)6)     /**< @brief ADP<->IPv6 Layer Primitive maximum number of BC0 header(s) */
#define ADP_RF_CHANNEL                  ((uint8_t)0x01)
#define ADP_HOP_INFO_TABLE_N			((uint8_t)28) /* Two times the maximum number of hops (14 = 0x0E) (round trip) */

#define ADP_SHORT_ADDR_MULTICAST_MASK   ((uint16_t)0x8000) /**< @brief ADP short address multicast bit mask */


/* Discover Route */
typedef enum adp_discover_route_enum
{
	ADP_DISCOVERY_ROUTE_FALSE = 0x00,   /**< @brief ADP<->IPv6 Layer Primitive no route discovery is performed */
	ADP_DISCOVERY_ROUTE_TRUE  = 0x01    /**< @brief ADP<->IPv6 Layer Primitive a route discovery procedure will be performed prior to sending the frame if a route to the destination is not available in the routing table */
} adp_discover_route_t;

/* Quality Of Service (Priority) */
typedef enum adp_qos_enum
{
	ADP_QOS_STANDARD_PRIORITY = 0x00,   /**< @brief ADP<->IPv6 Layer Primitive quality of service (QoS) of the frame to send: standard priority */
	ADP_QOS_HIGH_PRIORITY     = 0x01    /**< @brief ADP<->IPv6 Layer Primitive quality of service (QoS) of the frame to send: high priority */
} adp_qos_t;

/* Security Enable */
typedef enum adp_security_enabled_enum
{
	ADP_SECURIY_DISABLED = 0x00,  /**< @brief ADP<->IPv6 Layer Primitive the frame was received with a security level lower than adpSecurityLevel */
	ADP_SECURIY_ENABLED  = 0x01   /**< @brief ADP<->IPv6 Layer Primitive the frame was received with a security level greater or equal to adpSecurityLevel */
} adp_security_enabled_t;

/* Media type */
typedef enum adp_mediatype_enum
{
	ADP_MEDIATYPE_PLC = 0x00, /**< @brief Media Type is PLC */
	ADP_MEDIATYPE_RF  = 0x01  /**< @brief Media Type is RF */
} adp_mediatype_t;

#pragma pack(push, 1)

/**
  * @brief  ADPD-DATA.Request parameters
  */
typedef struct ADP_AdpdDataRequest_str
{
  uint16_t NsduLength;             /**< @brief The Length of the IPv6 Packet  */
  uint8_t  NsduHandle;             /**< @brief The Handle of the IPv6 Packet  */
  uint8_t  DiscoverRoute;          /**< @brief Discovery Route Flag: 0x01 to start a discovery route if route not present in routing table, 0x00 in the other cases */
  uint8_t  QualityOfService;       /**< @brief The Quality of service: 0x00 for normal priority, 0x01 for high priority  */
  uint8_t  Nsdu[ADP_MAX_SDU_SIZE]; /**< @brief The IPv6 Packet  */
} ADP_AdpdDataRequest_t;

#define ADP_DATA_REQ_MIN_LEN  ((uint16_t)sizeof(ADP_AdpdDataRequest_t) - ADP_MAX_SDU_SIZE)  /**< @brief Minimum length of one ADP Data Request (no payload) */

/**
  * @brief  ADPD-DATA.Confirm parameters
  */
typedef struct ADP_AdpdDataConfirm_str
{
  uint8_t  Status;     /**< @brief The Status of the operation (result)  */
  uint8_t  NsduHandle; /**< @brief The Handle specified in the correspondent .Request  */
  uint8_t  NexthopMedia; /**< @brief The Next Hop Media Type */
} ADP_AdpdDataConfirm_t;

/**
  * @brief  ADPD-DATA.Indication parameters
  */
typedef struct ADP_AdpdDataIndication_str
{
  uint16_t NsduLength;                 /**< @brief The Length of the IPv6 Packet  */
  MAC_DeviceAddress_t PreviousHop;     /**< @brief The MAC address of the previous hop  */
  uint8_t  LQI;                        /**< @brief The Link Quality Indicator  */
  uint8_t  PhaseDifferential;          /**< @brief The phase difference in multiples of 60 degrees of the last received MAC frame  */
  uint8_t  PayloadModType;             /**< @brief The modulation type used in the last received MAC frame  */
  uint8_t  PayloadModScheme;           /**< @brief The payload modulation scheme used in the last received MAC frame */
  uint8_t  ToneMap[PHY_TONEMAP_SIZE];  /**< @brief The Tone map parameter used in the last received MAC frame */
  uint8_t  BC0Num;                     /**< @brief Number of the BC0 headers of the 6LoWPAN broadcast corresponding to the received IPv6 multicast packet */
  uint8_t  BC0List[ADP_MAX_NUM_BC0];   /**< @brief List of BC0 headers of the 6LoWPAN broadcast packets corresponding to the received IPv6 multicast packet */
  uint8_t  PrevhopMedia;               /**< @brief The MediaType */
  uint8_t  Nsdu[ADP_MAX_SDU_SIZE];     /**< @brief The IPv6 Packet  */
} ADP_AdpdDataIndication_t;

/**
* @brief  ADPM-DISCOVERY.Request parameters
*/
typedef struct ADP_AdpmDiscoveryRequest_str
{
    uint8_t duration;       /**< @brief The duration in seconds of the discovery procedure  */
} ADP_AdpmDiscoveryRequest_t;

/**
* @brief  ADPM-DISCOVERY.Confirm parameters
*/
typedef struct ADP_AdpmDiscoveryConfirm_str
{
    uint8_t 			status;       							/**< @brief The status of the discovery operation */
    uint8_t 			pan_count;     							/**< @brief The number of returned PAN Descriptors  */
    ADP_PanDescriptor_t pan_descriptor[ADP_MAX_NUM_PANDESCR]; 	/**< @brief The array of returned PAN Descriptors */
} ADP_AdpmDiscoveryConfirm_t;

/**
  * @brief  ADPM-NETWORK-START.Request parameters
  */
typedef struct ADP_AdpmNetworkStartRequest_str
{
    uint16_t pan_id; /**< @brief The Pan ID to be used for the new PAN */
} ADP_AdpmNetworkStartRequest_t;

/**
  * @brief  ADPM-NETWORK-START.Confirm parameters
  */
typedef struct ADP_AdpmNetworkStartConfirm_str
{
    uint8_t status; /**< @brief The status of the request */
} ADP_AdpmNetworkStartConfirm_t;

/**
* @brief  ADPM-NETWORK-JOIN.Request parameters
*/
typedef struct ADP_AdpmNetworkJoinRequest_str
{
    uint16_t pan_id;       /**< @brief The 16-bit PAN identifier of the network to join  */
    uint16_t lba_addr;  	/**< @brief The 16-bit short address of the device acting as a LoWPAN bootstrap agent  */
    uint8_t  media_type;   /**< @brief The Media Type */
#if COMPILE_7_X_VERSION
    uint32_t FH_LBAScheduleStartTime;
#endif /* COMPILE_7_X_VERSION */
} ADP_AdpmNetworkJoinRequest_t;

/**
* @brief  ADPM-NETWORK-JOIN.Confirm parameters
*/
typedef struct ADP_AdpmNetworkJoinConfirm_str
{
    uint8_t  status;          /**< @brief The result of the attempt to join the network */
    uint16_t network_addr;	  /**< @brief The 16-bit network address that was allocated to the device  */
    uint16_t pan_id;          /**< @brief TThe 16-bit address of the PAN of which the device is now a member  */
} ADP_AdpmNetworkJoinConfirm_t;

/**
* @brief  ADPM-NETWORK-LEAVE.Confirm parameters
* @details The ADPM-NETWORK-LEAVE.Request has no parameters
*/
typedef struct ADP_AdpmNetworkLeaveConfirm_str
{
    uint8_t status;           /**< @brief The result of the leaving operation */
} ADP_AdpmNetworkLeaveConfirm_t;

/**
* @brief  ADPM-NETWORK-LEAVE.Indication parameters
*/
 /* No payload */

/**
 * @brief  ADPM-RESET.Confirm parameters
 * @details Note that ADPM-RESET.Request has no parameters
 */
typedef struct ADP_AdpmResetConfirm_str
{
   uint8_t status; /**< @brief The status of the requested reset */
} ADP_AdpmResetConfirm_t;

/**
  * @brief  ADPM-NETWORK-STATUS.Indication parameters
  */
typedef struct ADP_AdpmNetworkStatusIndication_str
{
    uint8_t   Status;                            /**< @brief The communications status  */
    MAC_DeviceAddress_t SrcAddr;                 /**< @brief The individual device address of the entity from which the frame causing the error originated  */
    MAC_DeviceAddress_t DstAddr;                 /**< @brief The individual device address of the device for which the frame was intended */
    uint8_t   SecurityLevel;                     /**< @brief The security level used by the originator of the received frame */
    uint8_t   KeyIdMode;                         /**< @brief The mode used to identify the key used by the originator of the received frame  */
    uint8_t   KeyIndex;                          /**< @brief The index of the key used by the originator of the received frame  */
    uint8_t   SNRCarriers[PHY_NUM_OF_CARRIERS];  /**< @brief The SNR measurements of each carrier */
    uint8_t   lqi;                               /**< @brief The Link Quality Indicator */
} ADP_AdpmNetworkStatusIndication_t;

/**
  * @brief  ADPD-ROUTE-DISCOVERY.Request parameters
  */
typedef struct ADP_AdpdRouteDiscoveryRequest_str
{
    uint16_t 	dst_addr;                		/**< @brief The 16 bits short address of the final destination node */
    uint8_t		max_hops;            			/**< @brief The maximum number of hops */
    uint16_t 	next_hop;                		/**< @brief The Next Hop - if already known - to which the RREQ will be transmitted. If the NextHop is set to the 0xFFFF the standard procedure will take place */
    uint8_t		media_type;          			/**< @brief The Next Hop media type (0x00 Power Line interface, 0x01 Radio Frequency interface). Ignored if NextHop is 0xFFFF. */
} ADP_AdpdRouteDiscoveryRequest_t;

/**
  * @brief  ADPD-ROUTE-DISCOVERY.Confirm parameters
  */
typedef struct ADP_AdpdRouteDiscoveryConfirm_str
{
	uint8_t		status;
    uint16_t 	dst_addr;                		/**< @brief The 16 bits short address of the final destination node */
    uint16_t 	next_hop;                		/**< @brief The 16 bits short address of the next hop */
    uint16_t 	route_cost;                		/**< @brief The computed route cost to reach the final destination */
    uint8_t 	weaklink	: 4;				/**< @brief Weak link count (4 MSB) */
    uint8_t 	hopcount	: 4;				/**< @brief Hop count (4 LSB) */
    uint16_t	valid_time;            			/**< @brief The remaining time for which the routing entry to the final destination is valid */
    uint8_t		media_type;          			/**< @brief The next hop media type (0x00 Power Line interface, 0x01 Radio Frequency interface) */
} ADP_AdpdRouteDiscoveryConfirm_t;

typedef struct ADP_HopInfo_str
{
	uint16_t 	addr;                			/**< @brief The 16 bits short address the hop */
	uint8_t		mns					: 1;		/**< @brief 0: metric supported by the node, 1: metric is not supported by the node */
	uint8_t		phase_differential	: 3;		/**< @brief Phase differential */
	uint8_t		mrx					: 1;		/**< @brief Medium over which a PREQ/PREP message has been received */
	uint8_t		mtx					: 1;		/**< @brief Medium over which a PREQ/PREP message should be transmitted */
	uint8_t		reserved			: 2;		/**< @brief Reserved */
	uint8_t		link_cost;                		/**< @brief The link cost for this hop */
} ADP_HopInfo_t;

/**
  * @brief  ADPD-PATH-DISCOVERY.Request parameters
  */
typedef struct ADP_AdpdPathDiscoveryRequest_str
{
    uint16_t 	dst_addr;                		/**< @brief The 16 bits short address of the final destination node */
    uint8_t		path_metric_type;            	/**< @brief The metric type to be used for link and rout cost evaluation during this path discovery: 0: hop count, 14: capacity metric type, 15: G3 composite metric type */
} ADP_AdpdPathDiscoveryRequest_t;

/**
  * @brief  ADPD-PATH-DISCOVERY.Confirm parameters
  */
typedef struct ADP_AdpdPathDiscoveryConfirm_str
{
    uint16_t 		dst_addr;                		/**< @brief The 16 bits short address of the final destination node */
    uint8_t 		status;							/**< @brief The operation result */
    uint16_t 		reserved			: 12;		/**< @brief Reserved */
    uint16_t 		path_metric_type	:  4;		/**< @brief PATHMETRICTYPE as specified in the request */
    uint16_t 		orig;							/**< @brief The 16 bits short address of the originator node */
    ADP_HopInfo_t	hop_info[ADP_HOP_INFO_TABLE_N];	/**< @brief List of the hop info */
} ADP_AdpdPathDiscoveryConfirm_t;

/**
  * @brief  ADPM-LBP.Request parameters
  */
typedef struct ADP_AdpmLbpRequest_str
{
    MAC_DeviceAddress_t dst_addr; 			/**< @brief The address of the destination device */
    uint16_t nsdu_len;         				/**< @brief The length of the NSDU */
    union {
    	uint8_t   nsdu[ADP_MAX_CTRL_PKT_SIZE]; 	/**< @brief The NSDU */
    	lbp_msg_t lbp;
    };
    uint8_t  nsdu_handle;         			/**< @brief The handle of this request */
    uint8_t  max_hops;            			/**< @brief The maximum number of hops */
    uint8_t  discover_route;     			/**< @brief A flag used to activate the route discovery */
    uint8_t  qos;                			/**< @brief The QoS of the NSDU */
    uint8_t  security_enabled;    			/**< @brief A flag used to activate the security */
    uint8_t  media_type;          			/**< @brief The MediaType */
} ADP_AdpmLbpRequest_t;

/**
  * @brief  ADPM-LBP.Confirm parameters
  */
typedef struct ADP_AdpmLbpConfirm_str
{
    uint8_t  status;     	/**< @brief The status of the request */
    uint8_t  nsdu_handle;	/**< @brief The handle used for the request*/
    uint8_t  media_type;	/**< @brief The Media Type */
} ADP_AdpmLbpConfirm_t;

/**
  * @brief  ADPM-LBP.Indication  parameters
  */
typedef struct ADP_AdpmLbpIndication_str
{
    MAC_DeviceAddress_t src_addr;   		/**< @brief The address of the source device */
    uint16_t nsdu_len;              		/**< @brief The length of the NSDU */
    uint8_t  nsdu[ADP_MAX_CTRL_PKT_SIZE];	/**< @brief The NSDU */
    uint8_t  lqi;							/**< @brief The LQI of the current NSDU */
    uint8_t  security_enabled;      		/**< @brief A flag that indicates if the NSDU was received with or without the security */
    uint8_t  media_type; 					/**< @brief MediaType */
} ADP_AdpmLbpIndication_t;

/**
  * @brief  ADPD-ROUTEOVER-DATA.Request parameters
  */
typedef struct ADP_AdpdRouteOverDataRequest_str
{
  uint16_t            NsduLength;             /**< @brief The Length of the IPv6 Packet  */
  uint8_t             NsduHandle;             /**< @brief The Handle of the IPv6 Packet  */
  MAC_DeviceAddress_t NextHop;                /**< @brief The MAC address of the next node the packet shall be sent to  */
  uint8_t             AckRequired;            /**< @brief It allows the upper layers to handle MAC layer acknowledgement: 0x00 if the device shall not request the acknowledgement, 0x01 if the device shall request the acknowledgement */
  uint8_t             QualityOfService;       /**< @brief The Quality of service: 0x00 for normal priority, 0x01 for high priority  */
  uint8_t  			  MediaType;              /**< @brief The channel over which the frame has to betransmitted to thenext hop */
  uint8_t             Nsdu[ADP_MAX_SDU_SIZE]; /**< @brief The IPv6 Packet  */
} ADP_AdpdRouteOverDataRequest_t;

#define ADP_RO_DATA_MIN_ADDR_LEN ((uint16_t) sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t))   /**< @brief Minimum length of Next Hop destination address (Mode + PanId + Short address) */
#define ADP_RO_DATA_REQ_MIN_LEN  ((uint16_t) sizeof(ADP_AdpdRODataRequest_t) - (uint16_t)sizeof(MAC_DeviceAddress_t) + ADP_RO_DATA_MIN_ADDR_LEN - ADP_MAX_SDU_SIZE) /**< @brief Minimum length of ADPD-RO-DATA Request */
#define ADP_RO_DATA_REQ_MAX_LEN  (uint16_t) sizeof(ADP_AdpdRODataRequest_t) /**< @brief Maximim length of ADPD-RO-DATA Request */

/**
  * @brief  ADPM-ROUTE-DELETE.Request parameters
  */
typedef struct ADP_AdpmRouteDeleteRequest_str
{
    uint16_t final_dst_addr; 	/**< @brief The final destination address */
} ADP_AdpmRouteDeleteRequest_t;

/**
  * @brief  ADPM-ROUTE-DELETE.Confirm parameters
  */
typedef struct ADP_AdpmRouteDeleteConfirm_str
{
    uint8_t  status;  			/**< @brief The status of the requested reset */
    uint16_t final_dst_addr; 	/**< @brief The final destination address */
} ADP_AdpmRouteDeleteConfirm_t;

#pragma pack(pop)

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

#endif // HI_ADP_SAP_INTERFACE_H

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
