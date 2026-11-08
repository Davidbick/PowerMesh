/*****************************************************************************
*   @file    hi_msgs_impl.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for message filling functions.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef HI_MSGS_IMPL_H_
#define HI_MSGS_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdbool.h>
#include <settings.h>
#include <hi_mac_pib_types.h>
#include <hi_adp_pib_types.h>
#include <hi_ip_pib_attributes.h>
#include <hi_g3lib_sap_interface.h>
#include <hi_adp_sap_interface.h>
#include <hi_boot_sap_interface.h>
#include <hi_ip_sap_interface.h>
#include <g3_comm.h>
#include <g3_app_boot_srv.h>
#include <g3_boot_srv_join_entry_tbl.h>


/** @addtogroup G3_App
  * @{
  */

/** @defgroup G3_App_Msg_Def G3 Message Definitions
  * @{
  */

/* Definitions */

/* SFLASH */
#define SFLASH_OP_MAX_SIZE				256

/* RF */
#define RF_POWER_DBM_OFFSET				31			/* PowerdBm field = 0 -> -31 dBm, PowerdBm field = 45 -> 14 dBm) */

/* Debug Traces */
#define TRACE_CONTROL_FIELD_VALUE		5

/* S2LP */
#define S2LP_MAX_REG_LEN  ((uint8_t) 255)  // Maximum number of bytes that can be write/read to/from S2LP registers

/* NVM */
#define NVM_OP_MAX_SIZE					1024

/*CONFIG field: */
/* 1: disable, 0: enable
 * b0: not used in this version
 * b1: not used in this version
 * b2: use fast restore
 * b3: apply NVM PLC attributes
 * b4-b7: not used in this version
*/
#define NVM_CONFIG_OFFSET				7
#define NVM_CONFIG_SIZE					1
#define NVM_CONFIG_FAST_RESTORE_MASK	0xFB /* use fast restore (1 = disable, 0 = enable) */
#define NVM_CONFIG_APPLY_ATTR_MASK		0xF7 /* apply NVM PLC attributes (1 = disable, 0 = enable)  */

#define NVM_FAST_RESTORE_DATA_OFFSET	16
#define NVM_FAST_RESTORE_DATA_SIZE		30

#define NVM_PAN_SHORT_ADDR_OFFSET		44
#define NVM_PAN_SHORT_ADDR_SIZE			4

#define PE_VERSION_BAND_MASK            0xE0000000 /* Bandplan value bitmask: field can be CENA (0), CENB (1), ARIB (2), FCC (3), FCC_LOW (4), FCC_LOW (5) */
#define PE_VERSION_PROFILE_MASK         0x10000000 /* Device type value bitmask: field can be Device (0) or Coordinator (1) */
#define PE_VERSION_MAJOR_MASK           0x00FF0000 /* Major value bitmask of the PE version */
#define PE_VERSION_MINOR_MASK           0x0000FF00 /* Minor value bitmask of the PE version */
#define PE_VERSION_PATCH_MASK           0x000000FF /* Patch value bitmask of the PE version */

#define PE_VERSION_BAND_SHIFT           29	/* Bandplan value bit offset */
#define PE_VERSION_PROFILE_SHIFT        28	/* Device type value bit offset */
#define PE_VERSION_MAJOR_SHIFT          16	/* Major value bit offset of the PE version */
#define PE_VERSION_MINOR_SHIFT          8	/* Minor value bit offset of the PE version */
#define PE_VERSION_PATCH_SHIFT          0	/* Patch value bit offset of the PE version */

#define RTE_VERSION_MAJOR_MASK          0x00FF0000 /* Major value bitmask of the RTE version */
#define RTE_VERSION_MINOR_MASK          0x0000FF00 /* Minor value bitmask of the RTE version */
#define RTE_VERSION_PATCH_MASK          0x000000F0 /* Patch value bitmask of the RTE version */

#define RTE_VERSION_MAJOR_SHIFT         16	/* Major value bit offset of the RTE version */
#define RTE_VERSION_MINOR_SHIFT         8	/* Minor value bit offset of the RTE version */
#define RTE_VERSION_PATCH_SHIFT         4	/* Patch value bit offset of the RTE version */

#define IP6_HTONS(n) ((uint16_t) ((((uint16_t)         (n)) << 8) |        (((uint16_t) (n)) >> 8)))
#define IP6_HTONL(n) (             ((uint32_t)IP6_HTONS(n) << 16) | IP6_HTONS((uint32_t)(n)  >> 16))

#define GET_PE_VERSION_BAND(ver)		MASK_AND_SHIFTR(ver, PE_VERSION_BAND_MASK,   	PE_VERSION_BAND_SHIFT)
#define GET_PE_VERSION_PROFILE(ver)		MASK_AND_SHIFTR(ver, PE_VERSION_PROFILE_MASK,	PE_VERSION_PROFILE_SHIFT)

#define GET_PE_VERSION_MAJOR(ver)		MASK_AND_SHIFTR(ver, PE_VERSION_MAJOR_MASK,   	PE_VERSION_MAJOR_SHIFT)
#define GET_PE_VERSION_MINOR(ver)		MASK_AND_SHIFTR(ver, PE_VERSION_MINOR_MASK,   	PE_VERSION_MINOR_SHIFT)
#define GET_PE_VERSION_PATCH(ver)		MASK_AND_SHIFTR(ver, PE_VERSION_PATCH_MASK,   	PE_VERSION_PATCH_SHIFT)

#define GET_RTE_VERSION_MAJOR(ver)		MASK_AND_SHIFTR(ver, RTE_VERSION_MAJOR_MASK,   	RTE_VERSION_MAJOR_SHIFT)
#define GET_RTE_VERSION_MINOR(ver)		MASK_AND_SHIFTR(ver, RTE_VERSION_MINOR_MASK,   	RTE_VERSION_MINOR_SHIFT)
#define GET_RTE_VERSION_PATCH(ver)		MASK_AND_SHIFTR(ver, RTE_VERSION_PATCH_MASK,   	RTE_VERSION_PATCH_SHIFT)

#define SWAP_VECT(dst, src, len)    for (uint16_t _i = 0; _i < len; _i++) dst[len - 1 - _i] = src[_i]

typedef enum flash_op_enum
{
	flash_op_erase = 0,
	flash_op_read = 1,
	flash_op_write = 2
} flash_op_t;

typedef enum nvm_section_enum
{
	nvm_none = 0,
	nvm_gen = 1,
	nvm_plc = 2,
	nvm_plcc = 3,
	nvm_rf = 4
} nvm_section_t;

typedef enum rf_type_enum
{
	RF_TYPE_868 = 10U,
	RF_TYPE_915 = 20U,
} rf_type_t;

typedef enum hi_hwconfig_enum
{
    GPIO_CONFIG,
    GPIO_WRITE,
    GPIO_READ,
    BAUDRATE_CONFIG,
    S2LP_REG_WRITE,
    S2LP_REG_READ,
} hi_hwconfig_type_t;

#pragma pack(push, 1)

typedef struct {
    uint8_t port;
    uint8_t pin;
    uint8_t mode;
} hi_hwconfig_gpioconfig_req_t;

typedef struct {
    uint8_t port;
    uint8_t pin;
    uint8_t value;
} hi_hwconfig_gpiowrite_req_t;

typedef struct {
    uint8_t port;
    uint8_t pin;
} hi_hwconfig_gpioread_req_t;

typedef struct
{
    uint32_t baud_rate;
} hi_hwconfig_baudrateconfig_req_t;

typedef struct {
    uint8_t addr;
    uint8_t len;
    uint8_t data[S2LP_MAX_REG_LEN];
} hi_hwconfig_s2lpregwrite_req_t;

typedef struct {
    uint8_t addr;
    uint8_t len;
} hi_hwconfig_s2lpregread_req_t;

typedef struct {
    hi_hwconfig_type_t type;
    uint8_t payload_len;
    union
    {
        hi_hwconfig_gpioconfig_req_t gpio_conf;
        hi_hwconfig_gpiowrite_req_t gpio_write;
        hi_hwconfig_gpioread_req_t gpio_read;
        hi_hwconfig_baudrateconfig_req_t baudrate_conf;
        hi_hwconfig_s2lpregwrite_req_t s2lpreg_write;
        hi_hwconfig_s2lpregread_req_t s2lpreg_read;
    };
} hi_hwconfig_req_t;

typedef struct {
    uint8_t value;
} hi_hwconfig_gpioread_cnf_t;

typedef struct
{
    uint32_t baud_rate;
} hi_hwconfig_baudrateconfig_cnf_t;

typedef struct
{
    uint8_t len;
    uint8_t data[S2LP_MAX_REG_LEN];
} hi_hwconfig_s2lpregread_cnf_t;

typedef struct {
    uint8_t status;
    hi_hwconfig_type_t type;
    uint8_t payload_len;
    union
    {
        hi_hwconfig_gpioread_cnf_t       gpio_read;
        hi_hwconfig_baudrateconfig_cnf_t baudrate_conf;
        hi_hwconfig_s2lpregread_cnf_t    s2lpreg_read;
    };
} hi_hwconfig_cnf_t;

typedef struct hif_trace_req_str
{
	uint8_t control;
	uint32_t data_1;
	uint32_t data_2;
} hif_trace_req_t;

/* HOSTIF-TRACE.Confirm has no payload */

typedef struct hif_sflash_req_str
{
	uint8_t operation;
	uint16_t size;
	uint32_t address;
	uint8_t payload[SFLASH_OP_MAX_SIZE];
} hif_sflash_req_t;

typedef struct hif_sflash_cnf_str
{
	uint8_t payload[SFLASH_OP_MAX_SIZE];
} hif_sflash_cnf_t;

typedef struct hif_nvm_req_str
{
	uint8_t operation;
	uint8_t section;
	uint16_t offset;
	uint16_t size;
	uint8_t data[NVM_OP_MAX_SIZE];
} hif_nvm_req_t;

typedef struct hif_nvm_cnf_str
{
	uint8_t status;
	uint16_t size;
	uint8_t data[NVM_OP_MAX_SIZE];
} hif_nvm_cnf_t;

typedef struct hi_dbgtool_req_str
{
  uint8_t   control;
  uint32_t  info;
} hi_dbgtool_req_t;

typedef struct hi_dbgtool_cnf_str
{
  uint32_t	peVersion;
  uint32_t	libVersion;
  uint32_t	rteVersion;
  uint32_t	eui64[2];
  uint32_t	SFlashId;
  uint8_t	UsartNum;
  uint8_t	LedConf;
  uint8_t	platform;
  uint8_t	RfConf;		/* Only for ST8500 FW version 6 or above */
  uint8_t	PlcMode;	/* Only for ST8500 FW version 6 or above */
  uint16_t	PanId;		/* Only for ST8500 FW version 6 or above */
  uint16_t	ShortAddr;	/* Only for ST8500 FW version 6 or above */
} hi_dbgtool_cnf_t;

typedef struct s2lp_configdata_str
{
    uint32_t RadioBaseFreq;      /*!< Specify the base carrier frequency (in Hz) */
    uint8_t  RadioModSelect;     /*!< Specify the modulation */
    uint32_t RadioDataRate;      /*!< Specify the datarate expressed in bps */
    uint32_t RadioFreqDeviation; /*!< Specify the frequency deviation expressed in Hz */
    uint32_t RadioBandwidth;     /*!< Specify the channel filter bandwidth expressed in Hz */
    uint8_t  RadioCsBlanking;    /*!< Prevent data to be received if the RSSI level on the air is below the RSSI threshold */
    uint32_t RadioXtalFreq;      /*!< Set the XTAL frequency */
    uint8_t  RadioRssiGain;      /*!< Set the RSSI filter gain */
    uint8_t  PowerdBm;           /*!< Power Amplifier value to write expressed in dBm (0 is -31 dBm, 45 is 14 dBm). In case of FEM this value is used to set the raw register */
    uint8_t  PktCRCMode;         /*!< Set the CRC type */
    uint8_t  PktEnFEC;           /*!< Enable the FEC/Viterbi */
    uint8_t  PktEnWhitening;     /*!< Enable the data whitening */
    uint8_t  PktEnInterleaving;  /*!< Enable the interleaving of 802.15.4g packet */
    uint8_t  IrqGpioPin;         /*!< Specify the S2LP GPIO pin used to route the interrupt flag */
    uint8_t  MCUClkEnable;       /*!< Enable the S2LP Clock Output (0x00 Disabled, 0x01 Enabled) */
    uint8_t  MCUClkGpioPin;      /*!< Specify the S2LP GPIO pin used to export S2LP Clock to MCU */
    uint8_t  MCUClkXORatio;      /*!< Set the S2LP clock output XO prescaler */
    uint8_t  MCUClkRCORatio;     /*!< Set the S2LP Clock Output RCO prescaler */
    uint8_t  MCUClkClockCycles;  /*!< Set the S2LP ExtraClockCycles */
    uint8_t  ExtSmpsEnable;      /*!< Enable the external SMPS (0x00 Disabled, 0x01 Enabled) */
    uint8_t  FEMEnabled;         /*!< Enable the S2LP GPIO signals to control the Front End Module (0x00 Disabled, 0x01 Enabled) */
    uint8_t  FEMGpioPinCSD;      /*!< Specify the S2LP GPIO pin to control CSD */
    uint8_t  FEMGpioPinCPS;      /*!< Specify the S2LP GPIO pin to control CPS */
    uint8_t  FEMGpioPinCTX;      /*!< Specify the S2LP GPIO pin to control CTX */
    uint8_t  FEMTxBypassEn;      /*!< Enable the FEM TX Bypass Mode (0x00 Disabled, 0x01 Enabled) */
} s2lp_configdata_t;

typedef struct hif_rfconfigset_req_str
{
    s2lp_configdata_t s2lp_configdata;
} hif_rfconfigset_req_t;

typedef struct hif_rfconfigset_cnf_str
{
    uint8_t status;
} hif_rfconfigset_cnf_t;

#pragma pack(pop)

/* UTILITY */
void hi_ipv6_set_ipaddr(           ip6_addr_t *addr, const uint16_t pan_id,  const uint16_t short_addr);
void hi_ipv6_get_saddr_panid(const ip6_addr_t addr,        uint16_t *pan_id,       uint16_t *short_addr);

IP_UdpDataIndication_t* 	hi_ipv6_extract_udp_from_ip(const IP_DataIndication_t *data_ind);
IP_IcmpDataIndication_t* 	hi_ipv6_extract_icmp_from_ip(const IP_DataIndication_t *data_ind);

/* HOSTIF */
uint16_t hi_hostif_sflash_fill(        void *msg_, flash_op_t operation, uint16_t size, uint32_t address, uint8_t *payload);
uint16_t hi_hostif_nvm_fill(           void *msg_, flash_op_t operation, nvm_section_t section, uint16_t offset, uint16_t size, uint8_t *data);
uint16_t hi_hostif_dbgtoolreq_fill(    void *msg_, uint8_t control, uint32_t info);
uint16_t hi_hostif_rfconfigsetreq_fill(void *msg_, uint32_t base_frequency, rf_type_t rf_type);

/* G3LIB */
uint16_t hi_g3lib_swresetreq_fill(void *msg_, BOOT_Bandplan_t bandplan, plcType_t device_type, plc_mode_t mode, BOOT_StartType_t req_type);
uint16_t hi_g3lib_getreq_fill(    void *msg_, uint32_t id, uint16_t index);
uint16_t hi_g3lib_setreq_fill(    void *msg_, uint32_t id, uint16_t index, uint8_t* value, uint16_t length);

/* MAC */
uint16_t hi_mac_data_fill(void *msg_, const mac_addr_mode_t src_addr_mode, const uint16_t pan_id, const uint16_t dst_short_addr, const uint8_t *dst_ext_addr, const uint16_t msdu_len, const uint8_t* msdu, const uint8_t handle, const mac_tx_options_t tx_options, const mac_key_index_t key_index, const mac_mediatype_req_t media_type);

/* ADP */
#if (IS_COORD && ENABLE_BOOT_SERVER_ON_HOST) || (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST)
uint16_t hi_adp_discovery_fill(void *msg_, uint8_t duration);
uint16_t hi_adp_lbp_fill(void *msg_, const uint16_t nsdu_len, const uint8_t* nsdu, const uint8_t* lbd_ext_addr, const uint16_t pan_id, const uint16_t lba_short_addr, const adp_mediatype_t media_type, const uint8_t handle);
#endif

#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
uint16_t hi_adp_network_start_fill(void *msg_, uint16_t pan_id);
#endif

#if (!IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST)
uint16_t hi_adp_network_join_fill(void *msg_, const ADP_PanDescriptor_t *pan_descriptor);
#if BOOT_CLIENT_DISCOVER_ROUTE
uint16_t hi_adp_routediscovery_fill(void *msg_, uint16_t dst_addr, uint8_t max_hops, uint16_t next_hop, uint8_t media_type);
#endif
#endif

/* G3BOOT */
#if IS_COORD
uint16_t hi_boot_srvstartreq_fill(void *msg_, BOOT_StartType_t type, uint16_t pan_id, uint16_t short_addr);
uint16_t hi_boot_srvkickreq_fill(void *msg_, boot_device_t *device);
uint16_t hi_boot_srvsetpskreq_fill( void *msg_, const uint8_t *ext_addr,const uint8_t *psk, const uint16_t short_addr);

#if ENABLE_BOOT_SERVER_ON_HOST
uint16_t hi_boot_srvjoinind_fill(void *msg_, const boot_join_entry_t *join);
uint16_t hi_boot_srvleaveind_fill(void *msg_, const uint8_t *ext_addr);
uint16_t hi_boot_srvrekeyingreq_fill(void *msg_, const uint8_t *gmk);
uint16_t hi_boot_srvrekeyingcnf_fill(void *msg_, const uint8_t status, const uint8_t error);
uint16_t hi_boot_srvgetpskind_fill(void *msg_, const boot_join_entry_t *join_entry);
#endif

#else
uint16_t hi_boot_devstartreq_fill(  void *msg_, BOOT_StartType_t start_type);
uint16_t hi_boot_devpansortreq_fill(void *msg_, const BOOT_DevicePANSortIndication_t *pansort_ind);

#if ENABLE_BOOT_CLIENT_ON_HOST
uint16_t hi_boot_devstartcnf_fill(void *msg_, g3_result_t status, uint16_t network_addr, uint16_t pan_id);
uint16_t hi_boot_devleavecnf_fill(void *msg_, g3_result_t status);
uint16_t hi_boot_devpansortind_fill(void *msg_, const ADP_AdpmDiscoveryConfirm_t *discovery_cnf);
#endif

#endif /* IS_COORD */

/* IPv6 ICMP */
uint16_t hi_ipv6_echoreq_fill(      void *msg_, const ip6_addr_t dst_addr, const uint8_t handle, const uint16_t data_len, const uint8_t *data);

/* IPv6 UDP */
uint16_t hi_ipv6_udpconnsetreq_fill(void *msg_, const uint8_t conn_id, const ip6_addr_t remote_addr, const uint16_t remote_port, const uint16_t local_port);
uint16_t hi_ipv6_udpconngetreq_fill(void *msg_, const uint8_t conn_id);
uint16_t hi_ipv6_udpdatareq_fill(   void *msg_, const uint8_t conn_id, const ip6_addr_t dst_addr, const uint8_t handle, const uint16_t dst_port, const uint16_t data_len,const void *data);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* HI_MSGS_IMPL_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
