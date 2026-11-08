/**
*   @file      S2LP_Attributes.h
*   @author    AMS - Analog sGroup - IPC 
*   @version   V1.0.0
*   @date      13/12/2019
*   @brief     Include file with the definitions related to the S2LP PIB Attributes
*   @note      (C) COPYRIGHT 2019 STMicroelectronics
*   @par
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HI_S2LP_PIB_ATTRIBUTE_H_
#define HI_S2LP_PIB_ATTRIBUTE_H_

/* S2LP Attributes */
#define RF_RSSINOISEINTERVAL_ID		((uint32_t)0x5000)  /**< @brief rfRSSINoiseInterval: Time interval (in secs) between two consecutive background noise measurements */
#define RF_RSSINOISEMARGINRX_ID		((uint32_t)0x5001)  /**< @brief rfRSSINoiseMarginRX: The margin (in dB), to be added to last background noise measurement when setting RSSI threshold used in RX */
#define RF_DISABLEPHY_ID			((uint32_t)0x5002)  /**< @brief rfDisablePHY: Disable RF PHY Tx and Rx */
#define RF_RSSINOISEMARGINTX_ID		((uint32_t)0x5003)  /**< @brief rfRSSINoiseMarginTX: The margin (in dB), to be added to last background noise measurement when setting CS threshold used by CSMA in TX */
#define RF_FREQOFFSET_ID			((uint32_t)0x5004)  /**< @brief rfFreqOffset: Control the frequency offset (in Hz) to be applied wheh setting RF base frequency. The Most Significant Bit represents the sign (0 positive, 1 negative)  */
#define RF_RSSINOISEEWMAALPHA_ID	((uint32_t)0x5005)  /**< @brief rfRSSINoiseEWMAAlpha: The weight of Exponentially Weighted Moving Average used to average noise measurements. It is defined as Alpha = 1 / rfRSSINoiseEWMAAlpha (i.e. when rfRSSINoiseEWMAAlpha = 1 no averaging is applied) */
#define RF_WMBUSENABLE_ID			((uint32_t)0x5006)  /**< @brief rfWmBusEnable: gives the control of RF interface to WmBus protocol, it's a special use case where 2*S2LP share the Rf interface */
#define RF_ACTIVESTATES_MAXTIME_ID	((uint32_t)0x5007)  /**< @brief rfActiveStatesMaxTime: maximum time the tranceiver is allowed to stay in active states (TX, RX), in seconds */
#define RF_INITNOISE_ID				((uint32_t)0x5008)  /**< @brief rfInitNoise: initial value reference for background noise measurement */
#define RF_TXPACKETCOUNT_ID			((uint32_t)0x5009)  /**< @brief rfTxPacketCount: counter of packets transmitted by S2LP */
#define RF_RXPACKETCOUNT_ID			((uint32_t)0x500A)  /**< @brief rfRxPacketCount: counter of packets received by S2LP */
#define RF_FEMCSDGPIOCFG_ID			((uint32_t)0x500B)  /**< @brief rfFemCSDGpioConfig: configuration of GPIO FEM CSD */
#define RF_FEMCPSGPIOCFG_ID			((uint32_t)0x500C)  /**< @brief rfFemCPSGpioConfig: configuration of GPIO FEM CPS */
#define RF_FEMCTXGPIOCFG_ID			((uint32_t)0x500D)  /**< @brief rfFemCTXGpioConfig: configuration of GPIO FEM CTX */
#define RF_FEMRXGAIN_ID				((uint32_t)0x500E)  /**< @brief rfFemRxGain: Rx gain for FEM. The Most Significant Bit represents the sign (0 positive, 1 negative)  */
#define RF_BASEFREQUENCY_ID			((uint32_t)0x500F)  /**< @brief rfBaseFrequency: specifies the base carrier frequency in Hz */
#define RF_DATARATE_ID				((uint32_t)0x5010)  /**< @brief rfDataRate: specifies the data rate in bps */
#define RF_FREQDEVIATION_ID        	((uint32_t)0x5011)  /**< @brief rfFreqDeviation: specifies the frequency deviation in Hz */
#define RF_BANDWIDTH_ID            	((uint32_t)0x5012)  /**< @brief rfBandwidth: specifies the channel filter bandwidth in Hz */
#define RF_POWERDBM_ID             	((uint32_t)0x5013)  /**< @brief rfPowerDbm: power amplifier value expressed in dBm (0 is -31 dBm, 45 is 14 dBm) */

#endif /*HI_S2LP_PIB_ATTRIBUTE_H_*/

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
