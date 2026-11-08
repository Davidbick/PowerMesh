/******************************************************************************
*   @file    g3_app_config.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains the implementation of g3 configuration functionalities.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Inclusions */
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <mem_pool.h>
#include <utils.h>
#include <debug_print.h>
#include <hi_g3lib_sap_interface.h>
#include <hi_msgs_impl.h>
#include <hi_adp_lbp.h>
#include <g3_app_config.h>
#include <g3_app_config_rf_params.h>
#include <g3_app_boot.h>
#include <g3_app_boot_clt.h>
#include <user_modbus.h>
#include <settings.h>
#include <version.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Config
  * @{
  */

/** @addtogroup G3_App_Config_Private_Code
  * @{
  */

/* Definitions */
#define COUNTER_EXTRA_VALUE					0x400

/* attribute configuration steps */
#if IS_COORD
/* Mandatory */
#define MAC_PANID_ATTRIBUTE_STEP					0
#define MAC_SHORTADDRESS_ATTRIBUTE_STEP				1
/* Suggested */
#define MAC_KEYTABLE_ATTRIBUTE_STEP					2
#define ADP_ACTIVEKEYINDEX_ATTRIBUTE_STEP			3
#define ADP_PROBINGINTERVAL_ATTRIBUTE_STEP			4
#define ADP_BLACKLISTTABLEENTRYTTL_ATTRIBUTE_STEP	5
#define G3_LIB_PEEVENTINDICATION_ATTRIBUTE_STEP		6
#define ATTRIBUTE_N									7	/* Number of different attributes to set  */
#else
/* Mandatory */
#define ADP_EAPPSKKEY_ATTRIBUTE_STEP				0
/* Suggested */
#define ADP_COORDSHORTADDRESS_ATTRIBUTE_STEP		1
#define ADP_PROBINGINTERVAL_ATTRIBUTE_STEP			2
#define ADP_BLACKLISTTABLEENTRYTTL_ATTRIBUTE_STEP	3
#define G3_LIB_PEEVENTINDICATION_ATTRIBUTE_STEP		4
#define ATTRIBUTE_N									5	/* Number of different attributes to set  */
#endif

/* RF configuration steps */
#define RF_POWERDBM_ATTRIBUTE_STEP						0
#define MAC_FREQUENCYBAND_RF_ATTRIBUTE_STEP				1
#define MAC_OPERATINGMODE_RF_ATTRIBUTE_STEP				2
#define RF_FREQOFFSET_ATTRIBUTE_STEP					3
#define MAC_CHANNELNUMBER_RF_ATTRIBUTE_STEP				4
#define RF_FEMCSDGPIOCFG_ATTRIBUTE_STEP					5
#define RF_FEMCPSGPIOCFG_ATTRIBUTE_STEP					6
#define RF_FEMCTXGPIOCFG_ATTRIBUTE_STEP					7
#define RF_FEMRXGAIN_ATTRIBUTE_STEP						8
#if ENABLE_FREQUENCY_HOPPING
#define MAC_UNICASTSLOTDURATION_RF_ATTRIBUTE_STEP		9
#define MAC_BROADCASTSLOTDURATION_RF_ATTRIBUTE_STEP		10
#define MAC_BROADCASTINTERVALDURATION_RF_ATTRIBUTE_STEP	11
#define MAC_EXTENDEDBITMAP_RF_ATTRIBUTE_STEP			12
#define MAC_BCNRANDWINLEN_RF_ATTRIBUTE_STEP				13
#define MAC_ADDITIONALCHSCANTIME_RF_ATTRIBUTE_STEP		14
#define MAC_MAXCLOCKDRIFT_RF_ATTRIBUTE_STEP				15
#define MAC_HOPPINGENABLED_RF_ATTRIBUTE_STEP			16
#define RF_ATTRIBUTE_N									17	/* Number of different attributes to set during RF configuration */
#else
#define RF_ATTRIBUTE_N									9	/* Number of different attributes to set during RF configuration */
#endif /* ENABLE_FREQUENCY_HOPPING */

#if ENABLE_FAST_RESTORE
/* Fast restore steps */
#define FAST_RESTORE_MAC_PANID_ATTRIBUTE_STEP			0
#define FAST_RESTORE_MAC_SHORTADDRESS_ATTRIBUTE_STEP	1
#define FAST_RESTORE_ADP_GROUPTABLE_1_ATTRIBUTE_STEP	2
#define FAST_RESTORE_ADP_GROUPTABLE_2_ATTRIBUTE_STEP	3
#define FAST_RESTORE_ADP_GROUPTABLE_3_ATTRIBUTE_STEP	4
#define FAST_RESTORE_MAC_KEYTABLE_0_ATTRIBUTE_STEP		5
#define FAST_RESTORE_MAC_KEYTABLE_1_ATTRIBUTE_STEP		6
#define FAST_RESTORE_MAC_ACTIVEKEYINDEX_ATTRIBUTE_STEP	7
#if NVM_IS_ABSENT
#define FAST_RESTORE_MAC_FRAMECOUNTER_ATTRIBUTE_STEP	8
#define FAST_RESTORE_MAC_FRAMECOUNTER_RF_ATTRIBUTE_STEP	9
#define FAST_RESTORE_ADP_LOADngSEQNUM_ATTRIBUTE_STEP	10
#define FAST_RESTORE_ATTRIBUTE_N						11	/* Number of different attributes to set during fast restore, including the MAC/ADP counters */
#else
#define FAST_RESTORE_ATTRIBUTE_N						8	/* Number of different attributes to set during fast restore */
#endif /* NVM_IS_ABSENT */
#endif /* ENABLE_FAST_RESTORE */

/* Macros */
#if DEBUG_G3_CONF
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_G3_CONF_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* Private types */
typedef enum conf_event_enum
{
    CONF_EV_NONE,
    CONF_EV_RECEIVED_SWRESET_CNF,
    CONF_EV_RECEIVED_SET_CNF,
	CONF_EV_RECEIVED_DBGTOOL_CNF,
    CONF_EV_RECEIVED_RFCONFIGSET_CNF,
	CONF_EV_RECEIVED_NVM_CNF,
	CONF_EV_RECEIVED_SRV_STOP_CNF,
	CONF_EV_RECEIVED_SRV_START_CNF,
    CONF_EV_CNT
} conf_event_t;

typedef enum conf_state_enum
{
    CONF_ST_WAIT_SWRESET_CNF,
    CONF_ST_WAIT_SET_CNF,
	CONF_ST_WAIT_DBGTOOL_CNF,
	CONF_ST_WAIT_RFCONFIGSET_CNF,
	CONF_ST_WAIT_RFG3LIBSET_CNF,
	CONF_ST_FAST_RESTORE,
#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
	CONF_ST_WAIT_SRV_STOP_CNF,
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
	CONF_ST_WAIT_SRV_START_CNF,
#endif /* IS_COORD */
    CONF_ST_CNT
} g3_conf_state_t;

typedef struct g3_conf_fsm_info_str
{
    g3_conf_state_t curr_state;
    conf_event_t	curr_event;
    uint32_t		peVersion;
    bool			rf_connected;
    bool			g3_ready;
    /* PIBs configuration */
    uint32_t		config_index;
    /* RF PIBs configuration */
    uint32_t		rf_config_index;
    /* Fast Restore */
    bool			fast_restore;
    uint32_t		fast_restore_index;
} g3_conf_fsm_info_t;

/* Private variables */
static g3_conf_fsm_info_t	g3_conf_fsm_info;

#if !IS_COORD && ENABLE_FAST_RESTORE
fast_restore_data_t fast_restore_data;
/* this variable must be in STM32 FLASH */
flash_data_t	__attribute__((section(".Fast_Restore")))	usr_flash_data;
adp_lbd_eap_psk_data_t	*eap_psk_data;
eap_msg_t 				*eap_msg_3;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

/**
  * @}
  */

/** @addtogroup G3_App_Config_Exported_Code
  * @{
  */

/* Global variables */
rf_type_t		 rf_type;
BOOT_Bandplan_t	 working_plc_band = BOOT_BANDPLAN_CENELEC_A;
plc_mode_t		 working_plc_mode = PLC_MODE_PHY;
modbus_role_t    modbus_role;
uint8_t 		 mac_address[MAC_ADDR64_SIZE] = {0};

/* External variables */
extern bool fast_restore_enabled;


#if !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
extern boot_client_t boot_client;
#endif /* !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST */
/**
  * @}
  */

/** @addtogroup G3_App_Config_Private_Code
  * @{
  */

/* Private function pointer type */
typedef g3_conf_state_t g3_conf_fsm_func(void);

/* Private FSM function prototypes */
static g3_conf_state_t g3_conf_fsm_default(void);
static g3_conf_state_t g3_conf_fsm_set_attribute(void);
static g3_conf_state_t g3_conf_fsm_received_dbgtool_cnf(void);
static g3_conf_state_t g3_conf_fsm_set_rf_attribute(void);
static g3_conf_state_t g3_conf_fsm_fast_restore(void);
#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
static g3_conf_state_t g3_conf_fsm_received_srvstop_cnf(void);
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
static g3_conf_state_t g3_conf_fsm_received_srvstart_cnf(void);
#endif /* IS_COORD */

/* Private FSM function pointer array */

/**
  * @brief Calls a different function, depending on the current state and event.
  */
static g3_conf_fsm_func *g3_conf_fsm_func_tbl[CONF_ST_CNT][CONF_EV_CNT] = {
/*                               NONE,                RECEIVED_SWRESET_CNF,      RECEIVED_G3LIBSET_CNF,     	RECEIVED_DBGTOOL_CNF,			  RECEIVED_RFCONFIGSET_CNF,     RECEIVED_NVM_CNF		  RECEIVED_SRVSTOP_CNF,				RECEIVED_SRVSTART_CNF,  		  */
/* WAIT_SWRESET_CNF          */ {g3_conf_fsm_default, g3_conf_fsm_set_attribute, g3_conf_fsm_default,			g3_conf_fsm_default,              g3_conf_fsm_default,        	g3_conf_fsm_default,	  g3_conf_fsm_default,				g3_conf_fsm_default				  },
/* WAIT_G3LIBSET_CNF         */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_set_attribute,		g3_conf_fsm_default,			  g3_conf_fsm_default,          g3_conf_fsm_default,	  g3_conf_fsm_default,				g3_conf_fsm_default     		  },
/* WAIT_DBGTOOL_CNF          */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_default,			g3_conf_fsm_received_dbgtool_cnf, g3_conf_fsm_default, 			g3_conf_fsm_default,	  g3_conf_fsm_default,				g3_conf_fsm_default     		  },
/* WAIT_RFCONFIGSET_CNF      */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_default,			g3_conf_fsm_default,			  g3_conf_fsm_set_rf_attribute,	g3_conf_fsm_default,	  g3_conf_fsm_default,				g3_conf_fsm_default     		  },
/* WAIT_RFG3LIBSET_CNF		 */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_set_rf_attribute,	g3_conf_fsm_default,			  g3_conf_fsm_default,			g3_conf_fsm_default,	  g3_conf_fsm_default,				g3_conf_fsm_default     		  },
/* FAST_RESTORE				 */ {g3_conf_fsm_default, g3_conf_fsm_default,		 g3_conf_fsm_fast_restore,		g3_conf_fsm_default,      		  g3_conf_fsm_default,   		g3_conf_fsm_fast_restore, g3_conf_fsm_default,				g3_conf_fsm_default				  },
#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
/* WAIT_SRVSTOP_CNF          */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_default,			g3_conf_fsm_default,			  g3_conf_fsm_default,          g3_conf_fsm_default,	  g3_conf_fsm_received_srvstop_cnf,	g3_conf_fsm_default      		  },
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
/* WAIT_SRVSTART_CNF         */ {g3_conf_fsm_default, g3_conf_fsm_default,       g3_conf_fsm_default,			g3_conf_fsm_default,			  g3_conf_fsm_default,          g3_conf_fsm_default,	  g3_conf_fsm_default,        	  	g3_conf_fsm_received_srvstart_cnf },
#endif /* IS_COORD */
};


/* Private functions */

#if (DEBUG_G3_CONF >= DEBUG_LEVEL_FULL)
/**
  * @brief Function that translates an attribute of the G3 Configuration FSM to a string.
  * @param attribute Structure of the attribute to translate.
  * @return Pointer to a string representing the attribute of the G3 Configuration FSM.
  */
static char* g3_conf_translate_attribute(G3_LIB_PIB_ID_t attribute)
{
    switch(attribute.id)
    {
    case MAC_PANID_ID:							return "MAC_PANID";
    case MAC_SHORTADDRESS_ID:					return "MAC_SHORTADDRESS";
	case MAC_KEYTABLE_ID:						return "MAC_KEYTABLE";
	case ADP_ACTIVEKEYINDEX_ID:					return "ADP_ACTIVEKEYINDEX";
	case ADP_EAPPSKKEY_ID:						return "ADP_EAPPSKKEY";
	case ADP_COORDSHORTADDRESS_ID:				return "ADP_COORDSHORTADDRESS";
	case ADP_PROBINGINTERVAL_ID:				return "ADP_PROBINGINTERVAL";
	case ADP_BLACKLISTTABLEENTRYTTL_ID:			return "ADP_BLACKLISTTABLEENTRYTTL";
	case G3_LIB_PEEVENTINDICATION_ID:			return "G3_LIB_PEEVENTINDICATION";
    case ADP_LASTGASP_ID:						return "ADP_LASTGASP";
    case G3_LIB_RTEGPIO_ID:						return "G3_LIB_RTEGPIO";
    // RF attributes
    case RF_POWERDBM_ID:						return "RF_POWERDBM";
    case MAC_FREQUENCYBAND_RF_ID:				return "MAC_FREQUENCYBAND_RF";
    case MAC_OPERATINGMODE_RF_ID:				return "MAC_OPERATINGMODE_RF";
    case RF_FREQOFFSET_ID:						return "RF_FREQOFFSET";
    case MAC_CHANNELNUMBER_RF_ID:				return "MAC_CHANNELNUMBER_RF";
    case RF_FEMCSDGPIOCFG_ID:					return "RF_FEMCSDGPIOCFG";
    case RF_FEMCPSGPIOCFG_ID:					return "RF_FEMCPSGPIOCFG";
    case RF_FEMCTXGPIOCFG_ID:					return "RF_FEMCTXGPIOCFG";
    case RF_FEMRXGAIN_ID:						return "RF_FEMRXGAIN_RF";
    // Frequency Hopping attributes
    case MAC_UNICASTSLOTDURATION_RF_ID:			return "MAC_UNICASTSLOTDURATION_RF";
    case MAC_BROADCASTSLOTDURATION_RF_ID:		return "MAC_BROADCASTSLOTDURATION_RF";
    case MAC_BROADCASTINTERVALDURATION_RF_ID:	return "MAC_BROADCASTINTERVALDURATION_RF";
    case MAC_EXTENDEDBITMAP_RF_ID:				return "MAC_EXTENDEDBITMAP_RF";
    case MAC_BCNRANDWINLEN_RF_ID:				return "MAC_BCNRANDWINLEN_RF";
    case MAC_ADDITIONALCHSCANTIME_RF_ID:		return "MAC_ADDITIONALCHSCANTIME_RF";
    case MAC_MAXCLOCKDRIFT_RF_ID:				return "MAC_MAXCLOCKDRIFT_RF";
    case MAC_HOPPINGENABLED_RF_ID:				return "MAC_HOPPINGENABLED_RF";
	/* Add more... */
    default:									return "UNKNOWN_ATTRIBUTE";
    }
}
#endif

/* Private functions */

/**
 * @brief Builds the bit map for the G3 event notifications to enable.
 * @param None
 * @return the event notification enable bit map (64 bit integer).
 */
static uint64_t g3_conf_build_event_notifications(void)
{
	uint64_t pe_event_indication = 0;

#if ENABLE_EVENTINDICATION_ERROR
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_ERROR;
#endif
#if ENABLE_EVENTINDICATION_GMK
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_GMK;
#endif
#if ENABLE_EVENTINDICATION_CONTEXT
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_CONTEXT;
#endif

	/* Bit 3 is reserved */

#if ENABLE_EVENTINDICATION_ACTKEY
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_ACTKEY;
#endif
#if ENABLE_EVENTINDICATION_PANID
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PANID;
#endif
#if ENABLE_EVENTINDICATION_SHORTADDR
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_SHORTADDR;
#endif
#if ENABLE_EVENTINDICATION_THERMEV
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_THERMEV;
#endif
#if ENABLE_EVENTINDICATION_TMRTX
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_TMRTX;
#endif
#if ENABLE_EVENTINDICATION_TMRRX
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_TMRRX;
#endif
#if ENABLE_EVENTINDICATION_ROUTE
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_ROUTE;
#endif
#if ENABLE_EVENTINDICATION_PREQ
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PREQ;
#endif
#if ENABLE_EVENTINDICATION_SURGEEV
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_SURGEEV;
#endif
#if ENABLE_EVENTINDICATION_RTEWARN
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_RTEWARN;
#endif
#if ENABLE_EVENTINDICATION_RTEPOLL
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_RTEPOLL;
#endif
#if ENABLE_EVENTINDICATION_BOOTMAXRETIES
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_BOOTMAXRETIES;
#endif
#if ENABLE_EVENTINDICATION_BANDMISMATCH
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_BANDMISMATCH;
#endif
#if ENABLE_EVENTINDICATION_PHY_RX_QI
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PHYQUALITY_RX;
#endif
#if ENABLE_EVENTINDICATION_PHY_TX_QI
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PHYQUALITY_TX;
#endif
#if ENABLE_EVENTINDICATION_MACUPDATE
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_MACUPDATE;
#endif
#if ENABLE_EVENTINDICATION_MEMLEAK
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_MEMLEAK;
#endif
#if ENABLE_EVENTINDICATION_TIMERERR
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_TIMERERR;
#endif
#if ENABLE_EVENTINDICATION_PHY_RX_RF_QI
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PHYQUALITY_RX_RF;
#endif
#if ENABLE_EVENTINDICATION_PHY_TX_RF_QI
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_PHYQUALITY_TX_RF;
#endif
#if ENABLE_EVENTINDICATION_ADPDATAFORWARD
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_ADPDATAFORWARD;
#endif
#if ENABLE_EVENTINDICATION_FASTRESTORE
	pe_event_indication |= G3_LIB_PEEVENTINDICATION_FASTRESTORE;
#endif

	return pe_event_indication;
}


/**
  * @brief Init the RF configuration struct
  * @param None
  * @return The next state of the G3 Configuration FSM (the starting one).
  */
static void g3_conf_rf_config_init(void)
{

}

/**
  * @brief Completes the configuration.
  * @param None
  * @return The next state of the G3 Configuration FSM (the starting one).
  */
static g3_conf_state_t g3_conf_complete(void)
{
	g3_conf_fsm_info.g3_ready = true;

	if (working_plc_mode == PLC_MODE_MAC)
	{
#if defined(DEBUG)
		PRINT("DEBUG MODE\n\n");
#endif /* defined(DEBUG) */
		PRINT("Kit name: %s\n", EVALKIT_NAME);
#if defined(STM32G070xx)
		PRINT("STM32G070 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#elif defined(STM32G474xx)
		PRINT("STM32G474 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#elif defined(STM32L476xx)
		PRINT("STM32L476 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#else
		PRINT("STM32 FW version: %u.%u.%u\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_SUB);
#endif
		PRINT("Bandplan: %s\n", hi_boot_sap_translate_bandplan(working_plc_band));
		PRINT("PAN ID: %X\n", PAN_ID);

		ALLOC_STATIC_HEX_STRING(mac_address_str, mac_address, MAC_ADDR64_SIZE);
		PRINT("MAC address: %s\n", mac_address_str);

		/* Role not relevant in MAC mode */

#if ENABLE_MODBUS_TEST
		UserModbus_start();
#endif /* ENABLE_MODBUS_TEST */

		if (modbus_role == modbus_master)
		{
			PRINT("DUT: MAC test ready, press the ");
			PRINT_COLOR_NOTS("BLUE", color_blue);
			PRINT_NOTS(" push-button or enter the DUT extended address to start\n");
			PRINT("Format: EXT_ADDR.PLCRF|PLC|RF.MSG_N\n");
		}
		else
		{
			PRINT("Tester: MAC test ready, please start it from the DUT\n");

#if ENABLE_MODBUS_TEST
			UserModbus_exec();
#endif /* ENABLE_MODBUS_TEST */
		}
	}
#if !IS_COORD
#if ENABLE_BOOT_CLIENT_ON_HOST
	else
	{
		/* Starts the Boot Client (on Host) */
		g3_app_boot_clt_start();
	}
#else
#if ENABLE_FAST_RESTORE
	else
	{
		/* Starts the Boot Client (on modem) */
		if (fast_restore_enabled != false)
		{
			uint16_t len;
			BOOT_DeviceStartRequest_t *boot_start_req = MEMPOOL_MALLOC(sizeof(BOOT_DeviceStartRequest_t));

			len = hi_boot_devstartreq_fill(boot_start_req, BOOT_START_FAST_RESTORE);
			g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_START_REQ, boot_start_req, len);
		}
	}
#endif /* ENABLE_FAST_RESTORE */
#endif /* ENABLE_BOOT_CLIENT_ON_HOST */
#endif /* !IS_COORD */

	return CONF_ST_WAIT_SWRESET_CNF;
}

/**
  * @brief G3 Configuration FSM function that maintains the current state, with no further action.
  * @param None
  * @return The next state of the G3 Configuration FSM (equal to the current one).
  */
static g3_conf_state_t g3_conf_fsm_default(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;

    return g3_conf_fsm_info.curr_state;
}

/**
  * @brief G3 Configuration FSM function that sets another attribute of the list.
  * @param None
  * @return The next state of the G3 Configuration FSM.
  */
static g3_conf_state_t g3_conf_fsm_set_attribute(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;
    g3_conf_state_t next_state = g3_conf_fsm_info.curr_state;

    G3_LIB_SetAttributeRequest_t *set_attr_req = NULL;
	uint16_t len = 0;

	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
		/* Selects next attribute */
		switch (g3_conf_fsm_info.config_index)
		{
	#if IS_COORD
		/* Mandatory attributes for Coordinator */
		case MAC_PANID_ATTRIBUTE_STEP:
			/* Converts from 16 bit integer to byte array */
			const uint8_t pan_id[] = {LOW_BYTE(PAN_ID), HIGH_BYTE(PAN_ID)};
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(pan_id)));
			len = hi_g3lib_setreq_fill(set_attr_req, MAC_PANID_ID, 0, (uint8_t*) &pan_id, sizeof(pan_id));
			break;
		case MAC_SHORTADDRESS_ATTRIBUTE_STEP:
			/* Converts from 16 bit integer to byte array */
			const uint8_t coord_addr[] = {LOW_BYTE(COORD_ADDRESS), HIGH_BYTE(COORD_ADDRESS)};
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(coord_addr)));
			len = hi_g3lib_setreq_fill(set_attr_req, MAC_SHORTADDRESS_ID, 0, (uint8_t*) &coord_addr, sizeof(coord_addr));
			break;
		/* Suggested attributes for Coordinator */
		case MAC_KEYTABLE_ATTRIBUTE_STEP:
			const uint8_t default_gmk[MAC_KEY_SIZE] = DEFAULT_GMK;
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(default_gmk)));
			len = hi_g3lib_setreq_fill(set_attr_req, MAC_KEYTABLE_ID, 0, (uint8_t*) &default_gmk, sizeof(default_gmk));
			break;
		case ADP_ACTIVEKEYINDEX_ATTRIBUTE_STEP:
			const uint8_t active_gmk_index = DEFAULT_GMK_INDEX;
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(active_gmk_index)));
			len = hi_g3lib_setreq_fill(set_attr_req, ADP_ACTIVEKEYINDEX_ID, 0, (uint8_t*) &active_gmk_index, sizeof(active_gmk_index));
			break;
#else
		/* Mandatory attributes for Device */
		case ADP_EAPPSKKEY_ATTRIBUTE_STEP:
			const uint8_t eap_psk[]  = DEFAULT_PSK; /* PSK value used to connect to the coordinator */
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(eap_psk)));
			len = hi_g3lib_setreq_fill(set_attr_req, ADP_EAPPSKKEY_ID, 0, (uint8_t*) &eap_psk, sizeof(eap_psk));
			break;
		/* Suggested attributes for Device */
		/* Please note: MAC_PANID_ID, MAC_SHORTADDRESS_ID, MAC_KEYTABLE_ID and ADP_ACTIVEKEYINDEX_ID
		 * must not be set on DEVICE side since they are acquired through the bootstrap procedure */
		case ADP_COORDSHORTADDRESS_ATTRIBUTE_STEP:
			/* Converts from 16 bit integer to byte array */
			const uint8_t coord_addr[] = {LOW_BYTE(COORD_ADDRESS), HIGH_BYTE(COORD_ADDRESS)};
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(coord_addr)));
			len = hi_g3lib_setreq_fill(set_attr_req, ADP_COORDSHORTADDRESS_ID, 0, (uint8_t*) &coord_addr, sizeof(coord_addr));
			break;
#endif /* IS_COORD */
		/* Common attributes for Coordinator/Device */
		case ADP_PROBINGINTERVAL_ATTRIBUTE_STEP:
			const uint8_t adp_probing_interval = 2;
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(adp_probing_interval)));
			len = hi_g3lib_setreq_fill(set_attr_req, ADP_PROBINGINTERVAL_ID, 0, (uint8_t*) &adp_probing_interval, sizeof(adp_probing_interval));
			break;
		case ADP_BLACKLISTTABLEENTRYTTL_ATTRIBUTE_STEP:
			const uint8_t adp_blacklisttableentry_ttl[]	= {1, 0};
			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(adp_blacklisttableentry_ttl)));
			len = hi_g3lib_setreq_fill(set_attr_req, ADP_BLACKLISTTABLEENTRYTTL_ID, 0, (uint8_t*) &adp_blacklisttableentry_ttl, sizeof(adp_blacklisttableentry_ttl));
			break;
		case G3_LIB_PEEVENTINDICATION_ATTRIBUTE_STEP:
			uint64_t pe_event_indication = g3_conf_build_event_notifications();

			const uint8_t pe_event_indication_array[]     = {	TAKE_BYTE(pe_event_indication, 0),
																TAKE_BYTE(pe_event_indication, 1),
																TAKE_BYTE(pe_event_indication, 2),
																TAKE_BYTE(pe_event_indication, 3),
																TAKE_BYTE(pe_event_indication, 4),
																TAKE_BYTE(pe_event_indication, 5),
																TAKE_BYTE(pe_event_indication, 6),
																TAKE_BYTE(pe_event_indication, 7) };

			set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(pe_event_indication_array)));
			len = hi_g3lib_setreq_fill(set_attr_req, G3_LIB_PEEVENTINDICATION_ID, 0, (uint8_t*) &pe_event_indication_array, sizeof(pe_event_indication_array));
			break;
		default:
			break;
		}
	}
	else if (working_plc_mode == PLC_MODE_MAC)
	{
		/* Mandatory attribute if working in MAC */
		/* Converts from 16 bit integer to byte array */
		const uint8_t pan_id[] = {LOW_BYTE(PAN_ID), HIGH_BYTE(PAN_ID)};
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(pan_id)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_PANID_ID, 0, (uint8_t*) &pan_id, sizeof(pan_id));
	}
	else
	{
		Error_Handler(); /* Unsupported modes */
	}

	/* If there are no more attributes left to set */
	if (g3_conf_fsm_info.config_index >= ATTRIBUTE_N)
	{
    	hi_dbgtool_req_t *dbgtool_req = MEMPOOL_MALLOC(sizeof(hi_dbgtool_req_t));

    	len = hi_hostif_dbgtoolreq_fill(dbgtool_req, HI_TOOL_INFO, HI_TOOL_NO_CONF);
    	g3_send_message(HIF_TX_MSG, HIF_HI_DBGTOOL_REQ, dbgtool_req, len);

    	next_state = CONF_ST_WAIT_DBGTOOL_CNF;
	}
	else if (len != 0)
	{
		/* Set selected attribute */
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

		if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
			g3_conf_fsm_info.config_index++;
		else if (working_plc_mode == PLC_MODE_MAC)
			g3_conf_fsm_info.config_index = ATTRIBUTE_N;

#if (DEBUG_G3_CONF >= DEBUG_LEVEL_FULL)
		/* uint16 attributes shall be displayed swapped (MAC_PANID_ID, MAC_SHORTADDRESS_ID...)*/
		ALLOC_DYNAMIC_HEX_STRING(attribute_value_str, set_attr_req->attribute.value, set_attr_req->attribute.len);
		PRINT_G3_CONF_INFO("Setting attribute %s = 0x%s\n", g3_conf_translate_attribute(set_attr_req->attribute.attribute_id), attribute_value_str);
		FREE_DYNAMIC_HEX_STRING(attribute_value_str)
#endif

	    next_state = CONF_ST_WAIT_SET_CNF;
	}
	else
	{
		Error_Handler();
	}

    return next_state;
}

/**
  * @brief G3 Configuration FSM function that executes after the reception of a DBGTOOL confirmation.
  * @note The behaviour of this function depends on the device type, the version of the ST8500 FW
  * and the presence of the RF module.
  * @param None
  * @return The next state of the G3 Configuration FSM.
  */
static g3_conf_state_t g3_conf_fsm_received_dbgtool_cnf(void)
{
	g3_conf_fsm_info.curr_event = CONF_EV_NONE;

	g3_conf_state_t next_state;

	if (g3_conf_fsm_info.rf_connected)
	{
		uint32_t base_frequency;

		/* Sets different parameters, depending on the module type (868 or 915) and frequency */

		if (rf_type == RF_TYPE_915)
		{
			base_frequency = RF_PARAM_915_BASE_FREQ;
		}
		else
		{
			base_frequency = RF_PARAM_868_BASE_FREQ;
		}

		hif_rfconfigset_req_t *rfconfig_req = MEMPOOL_MALLOC(sizeof(hif_rfconfigset_req_t));

		uint16_t len = hi_hostif_rfconfigsetreq_fill(rfconfig_req, base_frequency, rf_type);
		g3_send_message(HIF_TX_MSG, HIF_HI_RFCONFIGSET_REQ, rfconfig_req, len);

		PRINT_G3_CONF_INFO("Configured RF with frequency %u Hz, power gain %d dBm\n", rfconfig_req->s2lp_configdata.RadioBaseFreq, rfconfig_req->s2lp_configdata.PowerdBm - RF_POWER_DBM_OFFSET);

		g3_conf_rf_config_init();
		g3_conf_fsm_info.rf_config_index = 0;
		next_state = CONF_ST_WAIT_RFCONFIGSET_CNF;
	}
	else
	{
		PRINT_G3_CONF_WARNING("S2LP module is not connected, RF configuration skipped\n");

#if IS_COORD
		if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
		{
#if !ENABLE_BOOT_SERVER_ON_HOST
			/* In order to start a custom server, it is necessary to stop the default one first */
			g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_STOP_REQ, NULL, 0);

			next_state = CONF_ST_WAIT_SRV_STOP_CNF;
#else
			BOOT_ServerStartRequest_t *srvstart_req = MEMPOOL_MALLOC(sizeof(sizeof(BOOT_ServerStartRequest_t)));

			uint16_t len = hi_boot_srvstartreq_fill(srvstart_req, BOOT_START_NORMAL, PAN_ID, COORD_ADDRESS);
			g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_START_REQ, srvstart_req, len);

			next_state = CONF_ST_WAIT_SRV_START_CNF;
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
		}
		else
		{
			/* Device ha completed configuration */
			next_state = g3_conf_complete();
		}
#else
		if (fast_restore_enabled != true)
		{
			/* Device ha completed configuration */
			next_state = g3_conf_complete();
		}
#if !IS_COORD && ENABLE_FAST_RESTORE
		else
		{
			/* Fast Restore */
			hif_nvm_req_t *hif_nvm_req = MEMPOOL_MALLOC(sizeof(hif_nvm_req_t)); /* Uses memory pool due to big structure size */

			uint16_t len = hi_hostif_nvm_fill(hif_nvm_req, flash_op_read, nvm_plc, NVM_FAST_RESTORE_DATA_OFFSET, NVM_FAST_RESTORE_DATA_SIZE, NULL);
			g3_send_message(HIF_TX_MSG, HIF_HI_NVM_REQ, hif_nvm_req, len);

			next_state = CONF_ST_FAST_RESTORE;
		}
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

#endif /* IS_COORD */
	}

	return next_state;
}

/**
  * @brief G3 Configuration FSM function that executes after the reception of a RFCONFIGSET confirmation.
  * @note The behavior of this function depends on the device type.
  * @param None
  * @return The next state of the G3 Configuration FSM.
  */
static g3_conf_state_t g3_conf_fsm_set_rf_attribute(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;
    g3_conf_state_t next_state = g3_conf_fsm_info.curr_state;

    G3_LIB_SetAttributeRequest_t *set_attr_req = NULL;
	uint16_t len = 0;

	/* Selects next attribute */
	switch (g3_conf_fsm_info.rf_config_index)
	{
	case RF_POWERDBM_ATTRIBUTE_STEP:
		uint8_t rf_powerdbm = RF_PARAM_RF_POWERDBM;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_powerdbm)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_POWERDBM_ID, 0, (uint8_t*) &rf_powerdbm, sizeof(rf_powerdbm));
		break;
	case MAC_FREQUENCYBAND_RF_ATTRIBUTE_STEP:
		uint8_t mac_frequency_band_rf = RF_PARAM_MAC_FREQUENCYBAND_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_frequency_band_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_FREQUENCYBAND_RF_ID, 0, (uint8_t*) &mac_frequency_band_rf, sizeof(mac_frequency_band_rf));
		break;
	case MAC_OPERATINGMODE_RF_ATTRIBUTE_STEP:
		uint8_t mac_operating_mode_rf = RF_PARAM_MAC_OPERATINGMODE_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_operating_mode_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_OPERATINGMODE_RF_ID, 0, (uint8_t*) &mac_operating_mode_rf, sizeof(mac_operating_mode_rf));
		break;
	case RF_FREQOFFSET_ATTRIBUTE_STEP:
		uint8_t rf_frequency_offset[] = {LOW_BYTE(RF_PARAM_RF_FREQOFFSET), HIGH_BYTE(RF_PARAM_RF_FREQOFFSET)};
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_frequency_offset)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_FREQOFFSET_ID, 0, (uint8_t*) &rf_frequency_offset, sizeof(rf_frequency_offset));
		break;
	case MAC_CHANNELNUMBER_RF_ATTRIBUTE_STEP:
		uint8_t mac_channel_number_rf = RF_PARAM_MAC_CHANNELNUMBER_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_channel_number_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_CHANNELNUMBER_RF_ID, 0, (uint8_t*) &mac_channel_number_rf, sizeof(mac_channel_number_rf));
		break;
	case RF_FEMCSDGPIOCFG_ATTRIBUTE_STEP:
		uint8_t rf_fem_csd_gpio_config;
		rf_fem_csd_gpio_config = (rf_type == RF_TYPE_915) ? RF_PARAM_915_RF_FEMCSDGPIOCONFIG : RF_PARAM_868_RF_FEMCSDGPIOCONFIG;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_fem_csd_gpio_config)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_FEMCSDGPIOCFG_ID, 0, (uint8_t*) &rf_fem_csd_gpio_config, sizeof(rf_fem_csd_gpio_config));
		break;
	case RF_FEMCPSGPIOCFG_ATTRIBUTE_STEP:
		uint8_t rf_fem_cps_gpio_config;
		rf_fem_cps_gpio_config = (rf_type == RF_TYPE_915) ? RF_PARAM_915_RF_FEMCPSGPIOCONFIG : RF_PARAM_868_RF_FEMCPSGPIOCONFIG;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_fem_cps_gpio_config)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_FEMCPSGPIOCFG_ID, 0, (uint8_t*) &rf_fem_cps_gpio_config, sizeof(rf_fem_cps_gpio_config));
		break;
	case RF_FEMCTXGPIOCFG_ATTRIBUTE_STEP:
		uint8_t rf_fem_ctx_gpio_config;
		rf_fem_ctx_gpio_config = (rf_type == RF_TYPE_915) ? RF_PARAM_915_RF_FEMCTXGPIOCONFIG : RF_PARAM_868_RF_FEMCTXGPIOCONFIG;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_fem_ctx_gpio_config)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_FEMCTXGPIOCFG_ID, 0, (uint8_t*) &rf_fem_ctx_gpio_config, sizeof(rf_fem_ctx_gpio_config));
		break;
	case RF_FEMRXGAIN_ATTRIBUTE_STEP:
		uint8_t rf_fem_rx_gain;
		rf_fem_rx_gain = (rf_type == RF_TYPE_915) ? RF_PARAM_915_RF_FEMRXGAIN : RF_PARAM_868_RF_FEMRXGAIN;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(rf_fem_rx_gain)));
		len = hi_g3lib_setreq_fill(set_attr_req, RF_FEMRXGAIN_ID, 0, (uint8_t*) &rf_fem_rx_gain, sizeof(rf_fem_rx_gain));
		break;
#if ENABLE_FREQUENCY_HOPPING
	case MAC_UNICASTSLOTDURATION_RF_ATTRIBUTE_STEP:
		uint8_t mac_unicastslotduration_rf = RF_PARAM_MAC_UNICASTSLOTDURATION_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_unicastslotduration_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_UNICASTSLOTDURATION_RF_ID, 0, (uint8_t*) &mac_unicastslotduration_rf, sizeof(mac_unicastslotduration_rf));
		break;
	case MAC_BROADCASTSLOTDURATION_RF_ATTRIBUTE_STEP:
		uint8_t mac_broadcastslotduration_rf = RF_PARAM_MAC_BROADCASTSLOTDURATION_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_broadcastslotduration_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_BROADCASTSLOTDURATION_RF_ID, 0, (uint8_t*) &mac_broadcastslotduration_rf, sizeof(mac_broadcastslotduration_rf));
		break;
	case MAC_BROADCASTINTERVALDURATION_RF_ATTRIBUTE_STEP:
		uint8_t mac_broadcastintervalduration_rf[] = {LOW_BYTE(RF_PARAM_MAC_BROADCASTINTERVALDURATION_RF), HIGH_BYTE(RF_PARAM_MAC_BROADCASTINTERVALDURATION_RF)};
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_broadcastintervalduration_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_BROADCASTINTERVALDURATION_RF_ID, 0, (uint8_t*) &mac_broadcastintervalduration_rf, sizeof(mac_broadcastintervalduration_rf));
		break;
	case MAC_EXTENDEDBITMAP_RF_ATTRIBUTE_STEP:
		uint8_t mac_extendedbitmap_rf[] = RF_PARAM_MAC_EXTENDEDBITMAP_RF;
		utils_reverse_array(mac_extendedbitmap_rf, sizeof(mac_extendedbitmap_rf));
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_extendedbitmap_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_EXTENDEDBITMAP_RF_ID, 0, (uint8_t*) &mac_extendedbitmap_rf, sizeof(mac_extendedbitmap_rf));
		break;
	case MAC_BCNRANDWINLEN_RF_ATTRIBUTE_STEP:
		uint8_t mac_bcnrandwinlen_rf[] = {LOW_BYTE(RF_PARAM_MAC_BCNRANDWINLEN_RF), HIGH_BYTE(RF_PARAM_MAC_BCNRANDWINLEN_RF)};
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_bcnrandwinlen_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_BCNRANDWINLEN_RF_ID, 0, (uint8_t*) &mac_bcnrandwinlen_rf, sizeof(mac_bcnrandwinlen_rf));
		break;
	case MAC_ADDITIONALCHSCANTIME_RF_ATTRIBUTE_STEP:
		uint8_t mac_additionalchscantime_rf = RF_PARAM_MAC_ADDITIONALCHSCANTIME_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_additionalchscantime_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_ADDITIONALCHSCANTIME_RF_ID, 0, (uint8_t*) &mac_additionalchscantime_rf, sizeof(mac_additionalchscantime_rf));
		break;
	case MAC_MAXCLOCKDRIFT_RF_ATTRIBUTE_STEP:
		uint8_t mac_maxclockdrift_rf = RF_PARAM_MAC_MAXCLOCKDRIFT_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_maxclockdrift_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_MAXCLOCKDRIFT_RF_ID, 0, (uint8_t*) &mac_maxclockdrift_rf, sizeof(mac_maxclockdrift_rf));
		break;
	case MAC_HOPPINGENABLED_RF_ATTRIBUTE_STEP:
		uint8_t mac_hoppingenabled_rf = RF_PARAM_MAC_HOPPINGENABLED_RF;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(mac_hoppingenabled_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_HOPPINGENABLED_RF_ID, 0, (uint8_t*) &mac_hoppingenabled_rf, sizeof(mac_hoppingenabled_rf));
		break;
#endif /* ENABLE_FREQUENCY_HOPPING */
	default:
		break;
	}

	if (g3_conf_fsm_info.rf_config_index >= RF_ATTRIBUTE_N)
	{
#if IS_COORD
		if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
		{
#if !ENABLE_BOOT_SERVER_ON_HOST
			/* In order to start a custom server, it is necessary to stop the default one first */
			g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_STOP_REQ, NULL, 0);

			next_state = CONF_ST_WAIT_SRV_STOP_CNF;
#else
			BOOT_ServerStartRequest_t *srvstart_req = MEMPOOL_MALLOC(sizeof(sizeof(BOOT_ServerStartRequest_t)));

			uint16_t len = hi_boot_srvstartreq_fill(srvstart_req, BOOT_START_NORMAL, PAN_ID, COORD_ADDRESS);
			g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_START_REQ, srvstart_req, len);

			next_state = CONF_ST_WAIT_SRV_START_CNF;
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
		}
		else
		{
			/* Device ha completed configuration */
			next_state = g3_conf_complete();
		}
#else
		if (fast_restore_enabled != true)
		{
			/* Device ha completed configuration */
			next_state = g3_conf_complete();
		}
#if !IS_COORD && ENABLE_FAST_RESTORE
		else
		{
			/* Fast Restore */
			hif_nvm_req_t *hif_nvm_req = MEMPOOL_MALLOC(sizeof(hif_nvm_req_t)); /* Uses memory pool due to big structure size */

			uint16_t len = hi_hostif_nvm_fill(hif_nvm_req, flash_op_read, nvm_plc, NVM_FAST_RESTORE_DATA_OFFSET, NVM_FAST_RESTORE_DATA_SIZE, NULL);
			g3_send_message(HIF_TX_MSG, HIF_HI_NVM_REQ, hif_nvm_req, len);

			next_state = CONF_ST_FAST_RESTORE;
		}
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

#endif /* IS_COORD */
	}
	else if (len != 0)
	{
		/* Set selected attribute */
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

		g3_conf_fsm_info.rf_config_index++;

#if (DEBUG_G3_CONF >= DEBUG_LEVEL_FULL)
		/* uint16 attributes shall be displayed swapped (MAC_PANID_ID, MAC_SHORTADDRESS_ID...)*/
		ALLOC_DYNAMIC_HEX_STRING(attribute_value_str, set_attr_req->attribute.value, set_attr_req->attribute.len);
		PRINT_G3_CONF_INFO("Setting RF attribute %s = 0x%s\n", g3_conf_translate_attribute(set_attr_req->attribute.attribute_id), attribute_value_str);
		FREE_DYNAMIC_HEX_STRING(attribute_value_str)
#endif

	    next_state = CONF_ST_WAIT_RFG3LIBSET_CNF;
	}
	else
	{
		Error_Handler();
	}

    return next_state;
}

/**
  * @brief G3 conf function that handles the fast restore, setting the attributes that would be get by the bootstrap with a normal start
  * @param payload Not used in this function
  * @return The next Boot Client FSM state
  */
static g3_conf_state_t g3_conf_fsm_fast_restore(void)
{
    g3_conf_state_t next_state = g3_conf_fsm_info.curr_state;

    g3_conf_fsm_info.curr_event = CONF_EV_NONE;

#if !IS_COORD && ENABLE_FAST_RESTORE

	G3_LIB_SetAttributeRequest_t *set_attr_req = NULL;
	uint16_t len = 0;

	/* Selects next attribute */
	switch (g3_conf_fsm_info.fast_restore_index)
	{
	case FAST_RESTORE_MAC_PANID_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.pan_id)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_PANID_ID, 0, (uint8_t*) &fast_restore_data.pan_id, sizeof(fast_restore_data.pan_id));
		break;
	case FAST_RESTORE_MAC_SHORTADDRESS_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.short_addr)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_SHORTADDRESS_ID, 0, (uint8_t*) &fast_restore_data.short_addr, sizeof(fast_restore_data.short_addr));
		break;
	case FAST_RESTORE_ADP_GROUPTABLE_1_ATTRIBUTE_STEP:
		const uint16_t group_table_1 = ADP_GROUP_TABLE_DEFAULT_1;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(group_table_1)));
		len = hi_g3lib_setreq_fill(set_attr_req, ADP_GROUPTABLE_ID, ADP_GROUP_TABLE_DEFAULT_1_IDX, (uint8_t*) &group_table_1, sizeof(group_table_1));
		break;
	case FAST_RESTORE_ADP_GROUPTABLE_2_ATTRIBUTE_STEP:
		const uint16_t group_table_2 = ADP_GROUP_TABLE_DEFAULT_2;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(group_table_2)));
		len = hi_g3lib_setreq_fill(set_attr_req, ADP_GROUPTABLE_ID, ADP_GROUP_TABLE_DEFAULT_2_IDX, (uint8_t*) &group_table_2, sizeof(group_table_2));
		break;
	case FAST_RESTORE_ADP_GROUPTABLE_3_ATTRIBUTE_STEP:
		const uint16_t group_table_3 = ADP_GROUP_TABLE_UDP_RESPONDER;
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(group_table_3)));
		len = hi_g3lib_setreq_fill(set_attr_req, ADP_GROUPTABLE_ID, ADP_GROUP_TABLE_UDP_RESPONDER_IDX, (uint8_t*) &group_table_3, sizeof(group_table_3));
		break;
	case FAST_RESTORE_MAC_KEYTABLE_0_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.key_data[0].key)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_KEYTABLE_ID, 0, fast_restore_data.key_data[0].key, sizeof(fast_restore_data.key_data[0].key));
		break;
	case FAST_RESTORE_MAC_KEYTABLE_1_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.key_data[1].key)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_KEYTABLE_ID, 1, fast_restore_data.key_data[1].key, sizeof(fast_restore_data.key_data[1].key));
		break;
	case FAST_RESTORE_MAC_ACTIVEKEYINDEX_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.active_key_index)));
		len = hi_g3lib_setreq_fill(set_attr_req, ADP_ACTIVEKEYINDEX_ID, 0, &fast_restore_data.active_key_index, sizeof(fast_restore_data.active_key_index));
		break;
#if NVM_IS_ABSENT
	case FAST_RESTORE_MAC_FRAMECOUNTER_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.fc)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_FRAMECOUNTER_ID, 0, (uint8_t*) &fast_restore_data.fc, sizeof(fast_restore_data.fc));
		break;
	case FAST_RESTORE_MAC_FRAMECOUNTER_RF_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.fc_rf)));
		len = hi_g3lib_setreq_fill(set_attr_req, MAC_FRAMECOUNTER_RF_ID, 0, (uint8_t*) &fast_restore_data.fc_rf, sizeof(fast_restore_data.fc_rf));
		break;
	case FAST_RESTORE_ADP_LOADngSEQNUM_ATTRIBUTE_STEP:
		set_attr_req = MEMPOOL_MALLOC(G3_LIB_PIB_SIZE(sizeof(fast_restore_data.lg_sn)));
		len = hi_g3lib_setreq_fill(set_attr_req, ADP_LOADngSEQNUM_ID, 0, (uint8_t*) &fast_restore_data.lg_sn, sizeof(fast_restore_data.lg_sn));
		break;
#endif /* NVM_IS_ABSENT */
	default:
		break;
	}

	if (g3_conf_fsm_info.fast_restore_index >= FAST_RESTORE_ATTRIBUTE_N)
	{
#if !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
		boot_client.pan_id        = fast_restore_data.pan_id;
		boot_client.short_address = fast_restore_data.short_addr;
#endif /* !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST */

		/* Device ha completed configuration */
		next_state = g3_conf_complete();
	}
	else if (len != 0)
	{
		/* Set selected attribute */
		g3_send_message(HIF_TX_MSG, HIF_G3LIB_SET_REQ, set_attr_req, len);

		g3_conf_fsm_info.fast_restore_index++;

		PRINT_G3_CONF_INFO("Setting attribute #%u...\n", g3_conf_fsm_info.fast_restore_index);
	}
	else
	{
		Error_Handler();
	}
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

	return next_state;
}


#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST

/**
  * @brief G3 Configuration FSM function that executes after the reception of the SRVSTOP confirmation.
  * @param None
  * @return The next state of the G3 Configuration FSM.
  */
static g3_conf_state_t g3_conf_fsm_received_srvstop_cnf(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;

	BOOT_ServerStartRequest_t *srvstart_req = MEMPOOL_MALLOC(sizeof(sizeof(BOOT_ServerStartRequest_t)));

	uint16_t len = hi_boot_srvstartreq_fill(srvstart_req, BOOT_START_NORMAL, PAN_ID, COORD_ADDRESS);
	g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_START_REQ, srvstart_req, len);

	return CONF_ST_WAIT_SRV_START_CNF;
}
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */

/**
  * @brief G3 Configuration FSM function that executes after the reception of the SRVSTART confirmation.
  * @param None
  * @return The next state of the G3 Configuration FSM
  */
static g3_conf_state_t g3_conf_fsm_received_srvstart_cnf(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;

    /* Device ha completed configuration */
    return g3_conf_complete();
}
#endif /* IS_COORD */

/**
  * @brief Function that handles the reception of a HOSTIF-NVM.Cofirm.
  * @param payload The payload of the received message
  * @retval None
  */
static void g3_conf_handle_nvm_cnf(const void *payload)
{
#if !IS_COORD && ENABLE_FAST_RESTORE
	const hif_nvm_cnf_t *nvm_cnf = payload;

	if (g3_conf_fsm_info.curr_state == CONF_ST_FAST_RESTORE)
	{
		if (nvm_cnf->size == NVM_FAST_RESTORE_DATA_SIZE)
		{
			if (nvm_cnf->status == G3_SUCCESS)
			{
				/* Copy str for fast restore */
				fast_restore_data_nvm_t* nvm_data = (fast_restore_data_nvm_t*) nvm_cnf->data;

				fast_restore_data.fc				= nvm_data->fc;
				fast_restore_data.fc_rf				= nvm_data->fc_rf;
				fast_restore_data.lg_sn				= nvm_data->lg_sn;
				fast_restore_data.pan_id			= nvm_data->pan_id;

				fast_restore_data.short_addr		= usr_flash_data.str.short_addr;
				fast_restore_data.active_key_index	= usr_flash_data.str.active_key_index;
				fast_restore_data.key_data[0]		= usr_flash_data.str.key_data[0];
				fast_restore_data.key_data[1]		= usr_flash_data.str.key_data[1];

				g3_conf_fsm_info.fast_restore_index = 0;
			}
			else
			{
				/* Failure */
				memset(&fast_restore_data, 0, sizeof(fast_restore_data));

				g3_conf_fsm_info.fast_restore_index = FAST_RESTORE_ATTRIBUTE_N;
			}

			g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_NVM_CNF;
		}
	}
#else
	UNUSED(payload);
	g3_conf_fsm_info.curr_event = CONF_EV_NONE;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */
}

/**
  * @brief Function that handles the reception of a SWRESET confirmation.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_conf_handle_swreset_cnf(const void *payload)
{
#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
	const G3_LIB_SWResetConfirm_t *sw_reset_cnf = payload;
    HANDLE_CNF_ERROR(HIF_G3LIB_SWRESET_CNF, sw_reset_cnf->status);
#else
    UNUSED(payload);
#endif

    g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_SWRESET_CNF;
}

/**
  * @brief Function that handles the reception of a G3LIB-SET.Confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_conf_handle_set_cnf(const void *payload)
{
	const G3_LIB_SetAttributeConfirm_t *set_attr_cnf = payload;
#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
	HANDLE_CNF_ERROR(HIF_G3LIB_SET_CNF, set_attr_cnf->status);
#else
	UNUSED(payload);
#endif

    g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_SET_CNF;

}

/**
  * @brief Function that handles the reception of a HOSTIF-DBGTOOL.Confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_conf_handle_dbgtool_cnf(const void *payload)
{
	const hi_dbgtool_cnf_t *hi_dbg_tools_cnf = payload;

	g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_DBGTOOL_CNF;

	g3_conf_fsm_info.peVersion	  = hi_dbg_tools_cnf->peVersion;
	g3_conf_fsm_info.rf_connected = hi_dbg_tools_cnf->RfConf;
	memcpy(mac_address, hi_dbg_tools_cnf->eui64, sizeof(mac_address));

	/* MAC address needs to be reversed */
	utils_reverse_array(mac_address, sizeof(mac_address));
}

/**
  * @brief Function that handles the reception of a RFCONFIGSET confirmation.
  * @param payload Pointer to the payload of the received message.
  * @retval None.
  */
static void g3_conf_handle_rfconfigset_cnf(const void *payload)
{
#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
    const hif_rfconfigset_cnf_t *rfconfigset_cnf = payload;
    HANDLE_CNF_ERROR(HIF_HI_RFCONFIGSET_CNF, rfconfigset_cnf->status);
#else
    UNUSED(payload);
#endif

    g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_RFCONFIGSET_CNF;
}

#if !IS_COORD && ENABLE_FAST_RESTORE
/**
  * @brief   Process the Configuration Parameters received from the LBS in EAP-PSK #3 message
  * @param   [in] DataLength The length of the buffer containing the Configuration Parameters
  * @param   [in] Data The buffer containing the Configuration Parameters
  * @param   [in] fast_restore_data_flash The buffer containing the Configuration Parameters
  * @return  0 (ADP_RESULT_PARAMETER_SUCCESS) if all the necessary parameters are received and correctly decoded,
  * 		 error code otherwise
  */
uint8_t g3_conf_parse_msg_3_payload(uint16_t DataLength,
                                        uint8_t* Data,
		   							    fast_restore_data_flash_t* fast_restore_data_flash)
{
    uint16_t Offset = 0;
    uint8_t AttrId = 0;
    uint8_t AttrLen = 0;
    uint8_t Result = ADP_RESULT_PARAMETER_SUCCESS;

    // Bootstrapping data carries the configuration parameters. Decode and process
    // configuration parameters
    while (Offset < DataLength)
    {
        AttrId = Data[Offset++];
        AttrLen = Data[Offset++];

        if (Offset + AttrLen > DataLength)
        {
            // P_CHANNEL is not long enough
            Result = ADP_RESULT_INVALID_PARAMETER_VALUE;
            // Exit from the while loop
            break;
        }

        switch (AttrId)
        {
          // Short Address
          case ADP_MSG3_PARAM_SHORT_ADDR:
            if (ADP_CONF_PARAM_SHORT_ADDR_LEN == AttrLen)
            {
            	uint16_t short_addr = (Data[Offset] << 8) | Data[Offset+1];
            	if (short_addr != 0xFFFF)
            		fast_restore_data_flash->short_addr = short_addr;
            }
            else
                Result = ADP_RESULT_INVALID_PARAMETER_VALUE;
            break;

          // Provide a GMK key. On reception, the key is installed in the provided Key Identifier slot.
          case ADP_MSG3_PARAM_GMK:
            if (ADP_CONF_PARAM_GMK_LEN == AttrLen)
            {
            	uint8_t key_index = Data[Offset];
				fast_restore_data_flash->key_data[key_index].key_free = false; /* Key free indication (0: key0 present, other: key not preset) */
				memcpy(fast_restore_data_flash->key_data[key_index].key, &Data[Offset+1], MAC_KEY_SIZE);
            }
            else
                Result = ADP_RESULT_INVALID_PARAMETER_VALUE;
            break;

          // Indicate the GMK to use for outgoing messages
          case ADP_MSG3_PARAM_GMK_ACTIVATION:
            if (ADP_CONF_PARAM_GMK_ACTIVATION_LEN == AttrLen)
                fast_restore_data_flash->active_key_index = Data[Offset];
            else
                Result = ADP_RESULT_INVALID_PARAMETER_VALUE;
            break;

          // Unknown parameter received
          default:
            Result = ADP_RESULT_UNKNOWN_PARAMETER_ID;
            break;
        }

        // if (Result != SUCCESS) exit from the while loop
        if (ADP_RESULT_PARAMETER_SUCCESS != Result)
            break;

        /* increase offset */
        Offset += AttrLen;
    }

    return Result;
}

/**
  * @brief Function that handles the reception of a event indication (EAP-PSK MSG#2-#3).
  * @param payload Unused (no payload)
  * @retval None
  */
static void g3_conf_handle_event_notification_ind(const void *payload)
{
	const G3_LIB_EventNotificationIndicationt *event_notification_ind = payload;
	assert(event_notification_ind != NULL);
	if(event_notification_ind->type == fast_restore)
	{
		switch(event_notification_ind->info.fast_restore_info.EAP_msg)
		{
		case 2:
			/* allocate memory for eap_psk_data */
			eap_psk_data = MEMPOOL_MALLOC(sizeof(adp_lbd_eap_psk_data_t));
			/* initialize eap_psk_data */
			memset(eap_psk_data, 0, sizeof(adp_lbd_eap_psk_data_t));
			/* Extract randP, randS from msg2*/
			memcpy(eap_psk_data->rand_p, event_notification_ind->info.fast_restore_info.eap_msg.msg.n2.rand_p,	ADP_EAP_PSK_RAND_LEN);
			memcpy(eap_psk_data->rand_s, event_notification_ind->info.fast_restore_info.eap_msg.msg.n2.rand_s,	ADP_EAP_PSK_RAND_LEN);
			break;

		case 3:
			/* allocate memory for eap_msg_3 */
			eap_msg_3 = MEMPOOL_MALLOC(sizeof(eap_msg_t));
			/* initialize eap_msg_3 */
			memset(eap_msg_3, 0, sizeof(eap_msg_t));
			/* save msg3 */
			memcpy(eap_msg_3, &(event_notification_ind->info.fast_restore_info.eap_msg), event_notification_ind->info.fast_restore_info.lenght);
			/* Req a PIB read for ID_S_LEN */
			uint16_t len = 0;
			G3_LIB_GetAttributeRequest_t *get_attr_req = MEMPOOL_MALLOC(sizeof(G3_LIB_GetAttributeRequest_t));
			len = hi_g3lib_getreq_fill(get_attr_req, BOOT_SERVER_IDS_ID, 0);
			g3_send_message(HIF_TX_MSG, HIF_G3LIB_GET_REQ, get_attr_req, len);
			break;

		case ADP_MSG3_PARAM_GMK_ACTIVATION:
		case ADP_MSG3_PARAM_GMK_REMOVAL:
		    /* Intialize storage buffer */
			flash_data_t flash_data;
		    /* read data stored in STM32 Internal Flash */
		    /* this step is due to the need to Erase the Page Before a Write Operation */
		    flash_data = usr_flash_data;

		    if (event_notification_ind->info.fast_restore_info.EAP_msg == ADP_MSG3_PARAM_GMK_ACTIVATION)
			{
				/* update active gmk */
				memcpy(&flash_data.str.active_key_index, &event_notification_ind->info.fast_restore_info.eap_msg, sizeof(uint8_t));
			}
		    else
		    {
		    	/* remove key */
				uint8_t remove_index;
				memcpy(&remove_index, &event_notification_ind->info.fast_restore_info.eap_msg, sizeof(uint8_t));
				memset(&flash_data.str.key_data[remove_index], 0xFF, sizeof(flash_data.str.key_data));
		    }

			/* Write data to STM32 Internal Flash Memory */
			uint32_t PageError;
			HAL_StatusTypeDef status;
			FLASH_EraseInitTypeDef EraseInitStruct;
			EraseInitStruct.TypeErase	= FLASH_TYPEERASE_PAGES;
			EraseInitStruct.Banks       = (((uint32_t)&usr_flash_data - FLASH_BASE) / FLASH_BANK_SIZE) + 1;
			EraseInitStruct.Page        = (((uint32_t)&usr_flash_data - FLASH_BASE) % FLASH_BANK_SIZE) / FLASH_PAGE_SIZE ;
			EraseInitStruct.NbPages 	= 1;

			HAL_FLASH_Unlock();
			/* Erase the Page Before a Write Operation */
			status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
			/* Write data to STM32 Internal Flash Memory */
#if defined(STM32G070xx) || defined(STM32G474xx) || defined(STM32L476xx)
			for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 8;  j++)
				status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)&usr_flash_data + j*8, flash_data.array[j]);
#elif defined(STM32U545xx)
			for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 16;  j++)
				status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, (uint32_t)&usr_flash_data + j*16, flash_data.array[j]);
#endif
			HAL_FLASH_Lock();

		    if (event_notification_ind->info.fast_restore_info.EAP_msg == ADP_MSG3_PARAM_GMK_ACTIVATION)
			{
				if (status == HAL_OK)
					PRINT_G3_CONF_INFO("Active key written to STM32 Internal Flash\n");
				else
					PRINT_G3_CONF_INFO("ERROR in writing Active key to STM32 Internal Flash\n");
			}
		    else
		    {
				if (status == HAL_OK)
					PRINT_G3_CONF_INFO("Remove key from STM32 Internal Flash\n");
				else
					PRINT_G3_CONF_INFO("ERROR in removing key from STM32 Internal Flash\n");
		    }
		    break;

		default:
			break;
		}
	}
}

/**
  * @brief Function that handles the reception of a g3lib_get_cnf.
  * @param payload Unused (no payload)
  * @retval None
  */
static void g3_conf_handle_g3lib_get_cnf(const void *payload)
{
	const G3_LIB_GetAttributeConfirm_t* get_attribute_confirm = payload;
	if(get_attribute_confirm->status == G3_SUCCESS)
	{
		if(get_attribute_confirm->attribute.attribute_id.id == BOOT_SERVER_IDS_ID)
		{
			/* get id_s value */
			memcpy(eap_psk_data->id_s, get_attribute_confirm->attribute.value, ADP_EAP_PSK_ID_MAX_LEN);
			/* Req a PIB read for ID_S_LEN */
			uint16_t len = 0;
			G3_LIB_GetAttributeRequest_t *get_attr_req = MEMPOOL_MALLOC(sizeof(G3_LIB_GetAttributeRequest_t));
			len = hi_g3lib_getreq_fill(get_attr_req, BOOT_SERVER_IDS_LEN_ID, 0);
			g3_send_message(HIF_TX_MSG, HIF_G3LIB_GET_REQ, get_attr_req, len);
		}
		else if(get_attribute_confirm->attribute.attribute_id.id == BOOT_SERVER_IDS_LEN_ID)
		{
			/* get id_s_len value */
			eap_psk_data->id_s_len = get_attribute_confirm->attribute.value[0];
			/* initialize psk/tek */
			const uint8_t default_psk[] = DEFAULT_PSK;
			eap_psk_initialize_psk(default_psk, eap_psk_data->psk_context);
			eap_psk_initialize_tek(eap_psk_data->rand_p, eap_psk_data->psk_context);
			/* decode msg3 */
			uint16_t p_channel_len = 0;
			if (eap_psk_decode_3(eap_msg_3, eap_psk_data, &p_channel_len) == true)
			{
			    /* Intialize storage buffer */
				flash_data_t flash_data;
			    memset(flash_data.array, 0xFF, sizeof(flash_data.array));
			    /* this step to manage gmk-update case) */
			    flash_data = usr_flash_data;
				/* payload parsing */
				if (g3_conf_parse_msg_3_payload(p_channel_len-2, eap_msg_3->msg.n3.p_channel.ext.payload, &flash_data.str) == ADP_RESULT_PARAMETER_SUCCESS)
				{
					/* fast_restore_enabled needs to be saved too */
					flash_data.str.fast_restore_enabled = (uint8_t)fast_restore_enabled;
					/* Write data to STM32 Internal Flash Memory 											*/
					/* Please Note: for the sake of simplicity the GMKs are stored in a non-ciphered form	*/
					/* The choice about how to protect sensitive data is on CTM's duty						*/
					uint32_t PageError;
					HAL_StatusTypeDef status;
					FLASH_EraseInitTypeDef EraseInitStruct;
					EraseInitStruct.TypeErase	= FLASH_TYPEERASE_PAGES;
					EraseInitStruct.Banks       = (((uint32_t)&usr_flash_data - FLASH_BASE) / FLASH_BANK_SIZE) + 1;
					EraseInitStruct.Page        = (((uint32_t)&usr_flash_data - FLASH_BASE) % FLASH_BANK_SIZE) / FLASH_PAGE_SIZE ;
					EraseInitStruct.NbPages 	= 1;

					HAL_FLASH_Unlock();
					/* Erase the Page Before a Write Operation */
					status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
					/* Write data to STM32 Internal Flash Memory */
#if defined(STM32G070xx) || defined(STM32G474xx) || defined(STM32L476xx)
					for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 8;  j++)
						status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)&usr_flash_data + j*8, flash_data.array[j]);
#elif defined(STM32U545xx)
					for(uint32_t j = 0; j != sizeof(usr_flash_data.array) / 16;  j++)
						status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, (uint32_t)&usr_flash_data + j*16, flash_data.array[j]);
#endif
					HAL_FLASH_Lock();

					if (status == HAL_OK)
					{
						PRINT_G3_CONF_INFO("GMK written to STM32 Internal Flash\n");
					}
					else
					{
						PRINT_G3_CONF_INFO("ERROR in writing GMK to STM32 Internal Flash\n");
					}
				}
			}
			/* free malloc(s) */
			MEMPOOL_FREE(eap_psk_data);
			MEMPOOL_FREE(eap_msg_3);
		}
		else
		{
			/* do nothing */
		}
	}
	else
	{
		/* do nothing */
	}
}
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */

#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Function that handles the reception of a SRVSTOP confirmation.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_conf_handle_srvstop_cnf(const void *payload)
{
#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
	const BOOT_ServerStopConfirm_t *srv_stop_cnf = payload;
	HANDLE_CNF_ERROR(HIF_BOOT_SRV_STOP_CNF, srv_stop_cnf->status);
#else
	UNUSED(payload);
#endif

    g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_SRV_STOP_CNF;
}
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */

/**
  * @brief Function that handles the reception of a SRVSTART confirmation.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_conf_handle_srvstart_cnf(const void *payload)
{
#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
	const BOOT_ServerStartConfirm_t *srv_start_cnf = payload;
    HANDLE_CNF_ERROR(HIF_BOOT_SRV_START_CNF, srv_start_cnf->status);
#else
    UNUSED(payload);
#endif

    g3_conf_fsm_info.curr_event = CONF_EV_RECEIVED_SRV_START_CNF;
}

#endif /* IS_COORD */

/**
  * @brief Function that handles the FSM of the initial configuration of the device.
  * @param None
  * @retval None
  */
static void g3_conf_fsm_manager(void)
{
    g3_conf_fsm_info.curr_state = g3_conf_fsm_func_tbl[g3_conf_fsm_info.curr_state][g3_conf_fsm_info.curr_event]();
}

/**
  * @}
  */

/** @addtogroup G3_App_Config_Exported_Code
  * @{
  */

char* g3_app_translate_g3_result(g3_result_t g3_result)
{
	switch(g3_result)
	{
	case G3_SUCCESS: 					return "SUCCESS";
	case G3_WRONG_CRC: 					return "WRONG_CRC";
	case G3_WRONG_MSG_LEN: 				return "WRONG_MSG_LEN";
	case G3_WRONG_CMD_ID: 				return "G3_WRONG_CMD_ID";

	case G3_WRONG_MODE: 				return "WRONG_MODE";
	case G3_NOT_EXPECTED: 				return "NOT_EXPECTED";
	case G3_WRONG_STATE: 				return "WRONG_STATE";
#if 0
	case G3_WRONG_TRX_STATE: 			return "WRONG_TRX_STATE";
#endif
	case G3_NOT_ACCEPTABLE: 			return "NOT_ACCEPTABLE";
#if 0
	case G3_WRONG_TX_POWER: 			return "WRONG_TX_POWER";
	case G3_WRONG_MOD_SCHEME: 			return "WRONG_MOD_SCHEME";
	case G3_WRONG_MOD_TYPE: 			return "WRONG_MOD_TYPE";
	case G3_WRONG_PREEMPHASIS: 			return "WRONG_PREEMPHASIS";
	case G3_WRONG_TONEMAP: 				return "WRONG_TONEMAP";
	case G3_WRONG_TONEMASK: 			return "WRONG_TONEMASK";
	case G3_WRONG_FCH: 					return "WRONG_FCH";
	case G3_WRONG_VALUE_LEN: 			return "WRONG_VALUE_LEN";
	case G3_WRONG_TEST_MODE: 			return "WRONG_TEST_MODE";
	case G3_QUEUE_FULL: 				return "QUEUE_FULL";
#endif
	case G3_FAILED: 					return "FAILED";
#if 0
	case G3_TX_BUSY: 					return "TX_BUSY";
	case G3_RX_BUSY: 					return "RX_BUSY";
	case G3_PREVIOUS_REQUEST_FAILED: 	return "PREVIOUS_REQUEST_FAILED";
	case G3_NO_INDICATION: 				return "NO_INDICATION";
	case G3_WRONG_VALUE: 				return "WRONG_VALUE";
	case G3_WRONG_ATTR_NUM: 			return "WRONG_ATTR_NUM";
	case G3_WRONG_DT: 					return "WRONG_DT";
	case G3_RX_NACK: 					return "RX_NACK";
	case G3_TX_QUEUE_TO_SHORT: 			return "TX_QUEUE_TO_SHORT";
#endif
	case G3_INTERNAL_ERROR: 			return "INTERNAL_ERROR";
	case G3_NOT_IDLE: 					return "NOT_IDLE";
	case G3_WRONG_BAND: 				return "WRONG_BAND";
	case G3_NOT_SUPPORTED_BAND: 		return "NOT_SUPPORTED_BAND";
	case G3_NOT_ACCEPTABLE_LEN: 		return "NOT_ACCEPTABLE_LEN";
	case G3_PLC_DISABLED: 				return "PLC_DISABLED";
	case G3_WRONG_LEN: 					return "WRONG_LEN";
	case G3_BUSY: 						return "BUSY";
	case G3_INVALID_IPv6_FRAME: 		return "INVALID_IPv6_FRAME";
	case G3_INVALID_QOS: 				return "INVALID_QOS";

	case G3_ROUTE_ERROR:				return "ROUTE_ERROR";
#if 0
	case G3_BT_TABLE_FULL:				return "BT_TABLE_FULL";
	case G3_FRAME_NOT_BUFFERED:			return "FRAME_NOT_BUFFERED";
	case G3_STARTUP_FAILURE: 			return "STARTUP_FAILURE";
	case G3_NOT_PERMITTED: 				return "NOT_PERMITTED";
	case G3_UNKNOWN_DEVICE: 			return "UNKNOWN_DEVICE";
	case G3_PARAMETER_ERROR: 			return "PARAMETER_ERROR";
	case G3_WRONG_DEVICE_TYPE: 			return "WRONG_DEVICE_TYPE";
	case G3_MALLOC_FAIL: 				return "MALLOC_FAIL";
	case G3_RTEIPC_ERROR: 				return "RTEIPC_ERROR";
	case G3_WRONG_MEDIA_TYPE: 			return "WRONG_MEDIA_TYPE";
	case G3_UNAVAILABLE_MEDIA_TYPE: 	return "UNAVAILABLE_MEDIA_TYPE";
#endif
	case G3_DISABLED_MEDIA_TYPE: 		return "DISABLED_MEDIA_TYPE";
	case G3_JOINING_DECLINE: 			return "JOINING_DECLINE";
	case G3_JOINING_TIMEOUT: 			return "JOINING_TIMEOUT";
#if 0
	case G3_COUNTER_ERROR: 				return "COUNTER_ERROR";
	case G3_IMPROPER_KEY_TYPE: 			return "IMPROPER_KEY_TYPE";
	case G3_IMPROPER_SECURITY_LEVEL:	return "IMPROPER_SECURITY_LEVEL";
	case G3_UNSUPPORTED_LEGACY:			return "UNSUPPORTED_LEGACY";
	case G3_UNSUPPORTED_SECURITY: 		return "UNSUPPORTED_SECURITY";
	case G3_BEACON_LOSS: 				return "BEACON_LOSS";
	case G3_CHANNEL_ACCESS_FAILURE: 	return "CHANNEL_ACCESS_FAILURE";
	case G3_DENIED: 					return "DENIED";
	case G3_DISABLE_TRX_FAILURE: 		return "DISABLE_TRX_FAILURE";
	case G3_SECURITY_ERROR: 			return "SECURITY_ERROR";
	case G3_FRAME_TOO_LONG: 			return "FRAME_TOO_LONG";
	case G3_INVALID_HANDLE: 			return "INVALID_HANDLE";
#endif
	case G3_INVALID_PARAMETER: 			return "INVALID_PARAMETER";
	case G3_NO_ACK: 					return "NO_ACK";
	case G3_NO_BEACON: 					return "NO_BEACON";
	case G3_NO_DATA: 					return "NO_DATA";
	case G3_NO_SHORT_ADDRESS: 			return "NO_SHORT_ADDRESS";
	case G3_PAN_ID_CONFLICT: 			return "PAN_ID_CONFLICT";
	case G3_REALIGNMENT: 				return "REALIGNMENT";

	case G3_TRANSACTION_EXPIRED: 		return "TRANSACTION_EXPIRED";
	case G3_TRANSACTION_OVERFLOW: 		return "TRANSACTION_OVERFLOW";
#if 0
	case G3_TX_ACTIVE: 					return "TX_ACTIVE";
	case G3_UNAVAILABLE_KEY: 			return "UNAVAILABLE_KEY";
	case G3_UNSUPPORTED_ATTRIBUTE: 		return "UNSUPPORTED_ATTRIBUTE";
	case G3_INVALID_ADDRESS: 			return "INVALID_ADDRESS";
	case G3_INVALID_INDEX: 				return "INVALID_INDEX";
	case G3_LIMIT_REACHED: 				return "LIMIT_REACHED";
	case G3_READ_ONLY: 					return "READ_ONLY";
#endif
 	case G3_SCAN_IN_PROGRESS: 			return "SCAN_IN_PROGRESS";
 	default: 							return "OTHER";
	}
}

/**
  * @brief Initializes the G3 configuration application.
  * @param None
  * @retval None
  */
void g3_app_conf_init(void)
{
    g3_conf_fsm_info.curr_event = CONF_EV_NONE;
	g3_conf_fsm_info.curr_state = CONF_ST_WAIT_SWRESET_CNF;

	g3_conf_fsm_info.peVersion = 0x00000000;
	g3_conf_fsm_info.rf_connected = false;
	g3_conf_fsm_info.g3_ready = false;

	g3_conf_fsm_info.config_index = 0;
	g3_conf_fsm_info.rf_config_index = 0;

}

/**
  * @brief Starts the G3 configuration application in a specified mode (working_plc_mode).
  * @param None
  * @retval None
  */
void g3_app_conf_start(bool fast_restore_en)
{
	/* Sends SW reset request, selecting:
	 * - the bandplan (Cenelec A, Cenelec B or FCC),
	 * - the device type (coordinator or device),
	 * - the boot PLC mode
	 * - the req type (normal or fast restore) */

	BOOT_StartType_t working_boot_start_type = BOOT_START_NORMAL;

	if ((working_plc_mode == PLC_MODE_IPV6_BOOT) || (working_plc_mode == PLC_MODE_IPV6_ADP))
	{
#if !IS_COORD && ENABLE_FAST_RESTORE
		/* select boot_start_type */
		if (fast_restore_en != false)
		{
			working_boot_start_type = BOOT_START_FAST_RESTORE;
		}
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */
		/* Checks the presence of mandatory attributes inside the attribute table */
	}

	g3_conf_fsm_info.g3_ready = false;

	G3_LIB_SWResetRequest_t	*swreset_req = MEMPOOL_MALLOC(sizeof(G3_LIB_SWResetRequest_t));

    uint16_t len = hi_g3lib_swresetreq_fill(swreset_req, working_plc_band, PLC_DEVICE_TYPE, working_plc_mode, working_boot_start_type);
    g3_send_message(HIF_TX_MSG, HIF_G3LIB_SWRESET_REQ, swreset_req, len);
}

/**
  * @brief G3 configuration task routine.
  * @param None
  * @retval None
  */
void g3_app_conf(void)
{
	g3_conf_fsm_manager();
}

/**
  * @brief Checks if a message is needed by the G3 Configuration application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed, 'false' otherwise.
  */
bool g3_app_conf_msg_needed(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
	case HIF_HI_NVM_CNF:
    case HIF_HI_DBGTOOL_CNF:
    case HIF_HI_RFCONFIGSET_CNF:
    case HIF_G3LIB_SET_CNF:
    case HIF_G3LIB_SWRESET_CNF:
#if !IS_COORD && ENABLE_FAST_RESTORE
    case HIF_G3LIB_GET_CNF:
    case HIF_G3LIB_EVENT_IND:
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */
#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
    case HIF_BOOT_SRV_STOP_CNF:
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
    case HIF_BOOT_SRV_START_CNF:
#endif /* IS_COORD */
        return true;
    default:
        return false;
    }
}

/**
  * @brief Handles message reception for the G3 Configuration application.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_conf_msg_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
	case HIF_HI_NVM_CNF:
		g3_conf_handle_nvm_cnf(g3_msg->payload);
		break;
    case HIF_HI_DBGTOOL_CNF:
    	g3_conf_handle_dbgtool_cnf(g3_msg->payload);
        break;
    case HIF_HI_RFCONFIGSET_CNF:
        g3_conf_handle_rfconfigset_cnf(g3_msg->payload);
        break;
    case HIF_G3LIB_SET_CNF:
        g3_conf_handle_set_cnf(g3_msg->payload);
        break;
    case HIF_G3LIB_SWRESET_CNF:
        g3_conf_handle_swreset_cnf(g3_msg->payload);
        break;
#if !IS_COORD && ENABLE_FAST_RESTORE
    case HIF_G3LIB_GET_CNF:
    	g3_conf_handle_g3lib_get_cnf(g3_msg->payload);
		break;
    case HIF_G3LIB_EVENT_IND:
    	g3_conf_handle_event_notification_ind(g3_msg->payload);
		break;
#endif /* !IS_COORD && ENABLE_FAST_RESTORE */
#if IS_COORD
#if !ENABLE_BOOT_SERVER_ON_HOST
    case HIF_BOOT_SRV_STOP_CNF:
        g3_conf_handle_srvstop_cnf(g3_msg->payload);
        break;
#endif /* !ENABLE_BOOT_SERVER_ON_HOST */
    case HIF_BOOT_SRV_START_CNF:
        g3_conf_handle_srvstart_cnf(g3_msg->payload);
        break;
#endif /* IS_COORD */
    default:
        break;
    }
}

/**
  * @brief Returns the completion the G3 configuration application.
  * @param None
  * @retval True if the G3 configuration has completed its procedure, false otherwise.
  */
bool g3_app_conf_ready(void)
{
	return g3_conf_fsm_info.g3_ready;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
