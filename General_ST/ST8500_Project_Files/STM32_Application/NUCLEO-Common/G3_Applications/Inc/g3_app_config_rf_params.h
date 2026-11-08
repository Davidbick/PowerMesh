/*****************************************************************************
*   @file    g3_app_config_rf_params.h
*   @author  AMG/IPC Application Team
*   @brief   This file contains the RF parameters structures.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef G3_APP_CONFIG_RF_PARAMS_H_
#define G3_APP_CONFIG_RF_PARAMS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <settings.h>
#include <g3_app_config.h>

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
#define RF_PARAM_868_BASE_FREQ        	863100000
#define RF_PARAM_868_POWER_DBM        	35
#define RF_PARAM_868_IRQ_GPIO_PIN     	0x00
#define RF_PARAM_868_FEM_ENABLED      	0x00
#define RF_PARAM_868_FEM_GPIO_PIN_CSD 	0xFF
#define RF_PARAM_868_FEM_GPIO_PIN_CPS 	0xFF
#define RF_PARAM_868_FEM_GPIO_PIN_CTX 	0xFF


#if USE_STANDARD_ETSI_RF
#define RF_PARAM_915_BASE_FREQ        	863100000	/* In Hz */
#define RF_PARAM_915_POWER_DBM        	17			/* (17 - 31) dBm + 28 dB = +14 dBm */
#elif USE_STANDARD_FCC_RF
#define RF_PARAM_915_BASE_FREQ        	915000000	/* In Hz */
#define RF_PARAM_915_POWER_DBM        	30			/* (30 - 31) dBm + 28 dB = +27 dBm */
#endif

#define RF_PARAM_915_IRQ_GPIO_PIN     	0x03
#define RF_PARAM_915_FEM_ENABLED      	0x01
#define RF_PARAM_915_FEM_GPIO_PIN_CSD 	0x00
#define RF_PARAM_915_FEM_GPIO_PIN_CPS 	0x01
#define RF_PARAM_915_FEM_GPIO_PIN_CTX 	0x02

/* Define the RF type names */
#define RF_TYPE_STRING_915		"915 MHz"
#define RF_TYPE_STRING_868		"868 MHz"

#define RF_TYPE_STRING(rf_type)		(rf_type == RF_TYPE_915) ? RF_TYPE_STRING_915 : RF_TYPE_STRING_868

/* 868 minimum/maximum frequency */
#define RF_868_FREQ_MIN             863000000
#define RF_868_FREQ_MAX             870000000

/* 915 minimum/maximum frequency */
#define RF_915_FREQ_MIN             860000000
#define RF_915_FREQ_MAX             940000000

/* default values */
#define RF_PARAM_RF_POWERDBM				0x23	// Power amplifier value (4dBm)
#define RF_PARAM_MAC_FREQUENCYBAND_RF		0x04	// RF band 863MHz (IEEE 802154-2020 Table 7-21)
#define RF_PARAM_MAC_OPERATINGMODE_RF		0x01	// Operating Mode #1 (ITU-T G.9903 Table H.5-1)
#define RF_PARAM_RF_FREQOFFSET				0x0000	// Offset applied to RADIO_BASEFREQ
#define RF_PARAM_MAC_CHANNELNUMBER_RF		0x1D	// Channel #29 (866MHz)
/* EvalS2868 default values (no FEM) */
#define RF_PARAM_868_RF_FEMCSDGPIOCONFIG	0xA2	// GPIO in GND
#define RF_PARAM_868_RF_FEMCPSGPIOCONFIG	0xA2	// GPIO in GND
#define RF_PARAM_868_RF_FEMCTXGPIOCONFIG	0xA2	// GPIO in GND
#define RF_PARAM_868_RF_FEMRXGAIN			0x00	// No compensation on RSSI calculation
/* EvalS2915A2 default values (FEM mounted) */
#define RF_PARAM_915_RF_FEMCSDGPIOCONFIG	0x92	// GPIO as output
#define RF_PARAM_915_RF_FEMCPSGPIOCONFIG	0x52	// GPIO as output, LNA is NOT bypassed
#define RF_PARAM_915_RF_FEMCTXGPIOCONFIG	0x2A	// GPIO as output
#define RF_PARAM_915_RF_FEMRXGAIN			0x10	// 16dB compensation on RSSI calculation

/* Frequency Hopping */
#define RF_PARAM_MAC_UNICASTSLOTDURATION_RF			0x64	// duration of a unicast slot in ms
#define RF_PARAM_MAC_BROADCASTSLOTDURATION_RF		0x64	// duration of a unicast slot in ms
#define RF_PARAM_MAC_BROADCASTINTERVALDURATION_RF	0x0190	// duration of the broadcast interval (time between the start of 2 broadcast slots) in ms

// a bitmap of 129 bits for tracing active channels in FH
// at least 2 channels must be active for the selected FREQUENCYBAND/OPERATINGMODE)
#define RF_PARAM_MAC_EXTENDEDBITMAP_RF				{0xFF, 0xFF, 0xFF, 0xFF, \
													 0xFF, 0xFF, 0xFF, 0xFF, \
													 0xFF, 0xFF, 0xFF, 0xFF, \
													 0xFF, 0xFF, 0xFF, 0xFF, 0x01}
/*
(IEEE 802154-2020 Table 7-21)
(ITU-T G.9903 Table H.5-1)
(IEEE 802154-2020 Table 10-14)
|==================|====================================================================================|
ID	| BAND  | Mode |base Freq	|Data rate  |ch spac	|freq dev	|ChMin1	|ChMax1		|ChMin2	|ChMax2 |
|===|==============|====================================================================================|
4	|  863  |  #1   {863100000	,50000		,100000		,12500		,0		,68			,0		,0   	},
	|   .   |  #2   {863100000	,100000		,200000		,25000		,0		,34			,0		,0   	},

14	|  866  |  #1   {865100000	,50000		,100000		,12500		,0		,18			,0		,0   	},
	|   .   |  #2   {865100000	,100000		,200000		,25000		,0		,9			,0		,0   	},

15	|  870  |  #1   {870100000	,50000		,100000		,12500		,0		,58			,0		,0   	},
	|   .   |  #2   {870200000	,100000		,200000		,25000		,0		,28			,0		,0   	},

7	|  915  |  #1   {902200000	,50000		,200000		,25000		,0		,128		,0		,0   	},
	|   .   |  #3   {902400000	,200000		,400000		,50000		,0		,63			,0		,0   	},

16	|  915a |  #1   {902200000	,50000		,200000		,25000		,0		,128		,0		,0   	},
	|   .   |  #4   {902400000	,200000		,400000		,50000		,0		,63			,0		,0   	},

17	|  915b |  #1   {902200000	,50000		,200000		,25000		,0		,26			,65		,128 	},
	|   .   |  #4   {902400000	,200000		,400000		,50000		,0		,12			,32		,63  	},

18	|  915c |  #1   {902200000	,50000		,200000		,25000		,65		,128		,0		,0   	},
	|   .   |  #4   {902400000	,200000		,400000		,50000		,32		,63			,0		,0   	},

9	|  920  |  #1   {920600000	,50000		,200000		,25000		,0		,37			,0		,0   	},
	|   .   |  #6   {920900000	,150000		,400000		,37500		,0		,17			,0		,0   	},

21	|  919  |  #1   {902200000	,50000		,200000		,25000		,85		,103		,0		,0   	},
	|   .   |  #4   {902400000	,200000		,400000		,50000		,42		,51			,0		,0   	},

23	|  920b |  #1   {902200000	,50000		,200000		,25000		,90		,113		,0		,0   	},
	|   .   |  #4   {902400000	,200000		,400000		,50000		,45		,56			,0		,0   	},
*/

#define RF_PARAM_MAC_BCNRANDWINLEN_RF				0x2EE0	// duration time in ms for beacon randomization
#define RF_PARAM_MAC_ADDITIONALCHSCANTIME_RF		0x32	// additional time in ms to beacon randomization
#define RF_PARAM_MAC_MAXCLOCKDRIFT_RF				0x32	// clock drift of the device

#define RF_PARAM_MAC_HOPPINGENABLED_RF				0x01	// controls if the frequency hopping mechanism is enabled

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

#endif /* G3_APP_CONFIG_RF_PARAMS_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
