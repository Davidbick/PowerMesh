/*****************************************************************************
*   @file    hi_ip_sap_interface.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for all functionalities of the interface between IP and SAP levels.
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
#ifndef HI_IP_SAP_INTERFACE_H
#define HI_IP_SAP_INTERFACE_H

#include <stdint.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_phy_drv HIF PHY DRV
  * @{
  */

/** @defgroup IP_SAP_IF_TypesDefinitions IP-SAP Interface Types Definitions
  * @brief  IP Sap Interface types definitions
  * @{
  */

/* IP Commands parameters */

/**
  * @brief    ADP<->IPv6 Layer Interface Enumeration Values (Results and Status fields)
  * @details  These values are used to signal the results of ADP-SAP interface operations
  * @{
  */

#pragma pack(1)

/**
  * @}
  */

/**
  * @brief    ADP<->IPv6 Layer Interface Enumeration Values (Results and Status fields)
  * @details  These values are used to signal the results of ADP-SAP interface operations
  * @{
  */

/**
  * @brief Meta Data received from ADP Layer
  */
typedef struct IP_AdpMetaData_str
{
	MAC_DeviceAddress_t  previous_hop;		/**< @brief The MAC address of the previous hop  */
	uint8_t  lqi;                      		/**< @brief The Link Quality Indicator  */
	uint8_t  phase_differential;            /**< @brief The phase difference in multiples of 60 degrees of the last received MAC frame  */
	uint8_t  payload_modulation_type; 		/**< @brief The modulation type used in the last received MAC frame  */
	uint8_t  payload_modulation_scheme; 	/**< @brief The payload modulation scheme used in the last received MAC frame */
	uint8_t  tone_map[PHY_TONEMAP_SIZE];	/**< @brief The Tone map parameter used in the last received MAC frame */
	uint8_t  bc0_num;                       /**< @brief Number of the BC0 headers of the 6LoWPAN broadcast corresponding to the received IPv6 multicast packet */
	uint8_t  bc0_list[ADP_MAX_NUM_BC0];     /**< @brief List of BC0 headers of the 6LoWPAN broadcast packets corresponding to the received IPv6 multicast packet */
} IP_AdpMetaData_t;

/**
  * @brief  ICMP-DATA.Request parameters
  */
typedef struct IP_G3IcmpDataRequest_str
{
	ip6_addr_t  dest_address;       	/**< @brief The destination address for the ICMPv6 Packet  */
	uint8_t   	handle;               	/**< @brief The Handle of the ICMPv6 Packet  */
	uint16_t  	data_len;              	/**< @brief The Length of the ICMPv6 Packet  */
	uint8_t   	data[IP_MAX_SDU_SIZE];	/**< @brief The ICMPv6 Packet  */
} IP_G3IcmpDataRequest_t;

/**< @brief Minimum length of one ADP Data Request (no payload) */
#define G3_ICMP_DATA_REQ_MIN_LEN            (sizeof(IP_G3IcmpDataRequest_t) - IP_MAX_SDU_SIZE)
#define G3_ICMP_DATA_REQ_CURR_LEN(data_req) (sizeof(IP_G3IcmpDataRequest_t) - sizeof((data_req)->data) + (data_req)->data_len)

/**
  * @brief  ICMP-DATA.Confirm parameters
  */
typedef struct IP_G3IcmpDataConfirm_str
{
	uint8_t       status;			/**< @brief The Status of the operation */
	uint8_t       handle;			/**< @brief The Handle of the ICMPv6 Packet */
	uint8_t       next_hop_medium;	/**< @brief The MediaType */
} IP_G3IcmpDataConfirm_t;

/**
  * @brief  ICMP-DATA.Indication parameters
  */
typedef struct IP_IcmpDataIndication_str
{
	uint8_t          prev_hop_media;        	/**< @brief The MediaType */
	ip6_addr_t       source_address;         	/**< @brief The IPv6 source address of the ICMPv6 Packet  */
	uint16_t         data_len;              	/**< @brief The Length of the ICMP Echo Reply payload  */
	uint8_t          data[IP_MAX_SDU_SIZE];		/**< @brief The ICMP Echo Reply payload  */
} IP_IcmpDataIndication_t;

/**< @brief Minimum length of one IP Data Indication (no payload) */
#define ICMP_DATA_IND_MIN_LEN  ((uint16_t)sizeof(IP_IcmpDataIndication_t) - IP_MAX_SDU_SIZE)

/**
  * @brief  UDP-DATA.Confirm and ICMP-DATA.Confirm parameters
  */
typedef struct IP_DataConfirm_str
{
	uint8_t  status;  /**< @brief The Status of the last Request operation */
	uint8_t  handle;  /**< @brief The Handle specified in the correspondent .Request  */
} IP_DataConfirm_t;

/**
  * @brief  UDP-CONNECTION-SET.Request
  */
typedef struct IP_UdpConnSetRequest_str
{
	uint8_t       connection_id;          /**< @brief The Index of the UDP connection to set */
	ip6_addr_t    remote_address;   /**< @brief The remote IPv6 address (if set to all 0s it accepts UPD datagrams from any address)  */
	uint16_t      remote_port;      /**< @brief The remote UDP port (if set to 0 it accepts UPD datagrams on any port)  */
	uint16_t      local_port;       /**< @brief The local UDP port  */
} IP_UdpConnSetRequest_t;

/**
  * @brief  UDP-CONNECTION-SET.Confirm
  */
typedef struct IP_UdpConnSetConfirm_str
{
	uint8_t       status;          /**< @brief The Status of the operation */
} IP_UdpConnSetConfirm_t;

/**
  * @brief  UDP-CONNECTION-GET.Confirm
  */
typedef struct IP_UdpConnGetRequest_str
{
	uint8_t       connection_id;          /**< @brief The Index of the UDP connection to get */
} IP_UdpConnGetRequest_t;

/**
  * @brief  UDP-CONNECTION-GET.Request
  */
typedef struct IP_UdpConnGetConfirm_str
{
	uint8_t       status;        	/**< @brief The Status of the operation */
	uint8_t       connection_id;	/**< @brief The Index of the UDP connection to get */
	ip6_addr_t    remote_address;	/**< @brief The remote IPv6 address (if set to all 0s it accepts UPD datagrams from any address)  */
	uint16_t      remote_port;    	/**< @brief The remote UDP port (if set to 0 it accepts UPD datagrams on any port)  */
	uint16_t      local_port;     	/**< @brief The local UDP port  */
} IP_UdpConnGetConfirm_t;

/**
  * @brief  UDP-DATA.Request parameters
  */
typedef struct IP_G3UdpDataRequest_str
{
	uint8_t       handle;               	/**< @brief The Handle of the UDP transmission */
	uint8_t       connection_id;            /**< @brief The ID of the UDP connection to be used to send this datagram */
	ip6_addr_t    dest_address;          	/**< @brief The IPv6 destination address (if set to all 0s the remote address of the UDP connection is used)  */
	uint16_t      dest_port;             	/**< @brief The ID of the UDP connection to be used to send this datagram */
	uint16_t      data_len;             	/**< @brief The Length of the UDP payload  */
	uint8_t       data[IP_MAX_SDU_SIZE];	/**< @brief The UDP payload  */
} IP_G3UdpDataRequest_t;

/**< @brief Minimum length of one ADP Data Request (no payload) */
#define G3_UDP_DATA_REQ_CURR_LEN(data_req) (sizeof(IP_G3UdpDataRequest_t) - sizeof((data_req)->data) + (data_req)->data_len)



/**
  * @brief  UDP-DATA.Confirm parameters
  */
typedef struct
{
	uint8_t       status;                /**< @brief The Status of the operation */
	uint8_t       handle;                /**< @brief The Handle of the UDP transmission */
	uint8_t       next_hop_medium;        /**< @brief The MediaType */
} IP_G3UdpDataConfirm_t;

/**
  * @brief  UDP-DATA.Indication parameters
  */
typedef struct
{
	uint8_t          prev_hop_media;           /**< @brief The MediaType */
	uint8_t          connection_id;            /**< @brief The ID of the UDP connection  */
	ip6_addr_t       source_address;           /**< @brief The source IPv6 address */
	uint16_t         source_port;              /**< @brief The source UDP port */
	ip6_addr_t       dest_address;             /**< @brief The destination IPv6 address */
	uint16_t         dest_port;                /**< @brief The destination UDP port */
	uint16_t         data_len;                 /**< @brief The Length of the UDP data payload  */
	uint8_t          data[IP_MAX_SDU_SIZE];    /**< @brief The UDP data payload  */
} IP_UdpDataIndication_t;

/**< @brief Minimum length of one IP Data Indication (no payload) */
#define UDP_DATA_IND_MIN_LEN  ((uint16_t)sizeof(IP_UdpDataIndication_t) - IP_MAX_SDU_SIZE)

typedef struct
{
	IP_AdpMetaData_t meta_data;
	union
	{
		IP_IcmpDataIndication_t icmp_data_ind;   /**< @brief The received ICMP Packet  */
		IP_UdpDataIndication_t  udp_data_ind;    /**< @brief The received UDP Packet  */
	};
} IP_DataIndication_t;

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

#endif // HI_IP_SAP_INTERFACE_H

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
