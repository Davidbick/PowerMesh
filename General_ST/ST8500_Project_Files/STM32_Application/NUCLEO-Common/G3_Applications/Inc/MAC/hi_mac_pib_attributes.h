/*****************************************************************************
*   @file    hi_mac_pib_attributes.h
*   @author  AMG/IPC Application Team
*   @brief   Header file with the definitions related to the MAC PIB Attributes.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef HI_MAC_PIB_ATTRIBUTES_H
#define HI_MAC_PIB_ATTRIBUTES_H

#include <hi_mac_pib_types.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_mac_drv HIF MAC DRV
  * @{
  */

/** @defgroup MAC_PIB_Attributes MAC PIB Attributes
  * @brief    MAC PIB Attributes module
  * @details  This module contains the definition/handling of the MAC PIB Attributes, which are defined by the G3 and IEEE 802.15.4 standards. The module contains also implementation defined attributes
  * @{
  */

/** @defgroup MAC_PIB_Attributes_TypesDefinitions MAC PIB Attributes Types Definitions
  * @brief    MAC PIB Attributes types definitions
  * @{
  */
/* Standard IEEE 802.15.4 PIB attribute ID definitions */
#define MAC_RESERVED1_ID                ((uint32_t)0x0040) /**< @brief Reserved by ITU-T */
#define MAC_MAXBE_ID                    ((uint32_t)0x0047) /**< @brief macMaxBE Attribute ID */
#define MAC_BSN_ID                      ((uint32_t)0x0049) /**< @brief macBSN Attribute ID */
#define MAC_DSN_ID                      ((uint32_t)0x004C) /**< @brief macDSN Attribute ID */
#define MAC_MAXCSMABACKOFFS_ID          ((uint32_t)0x004E) /**< @brief macMaxCSMABackoffs Attribute ID */
#define MAC_MINBE_ID                    ((uint32_t)0x004F) /**< @brief macMinBE Attribute ID */
#define MAC_PANID_ID                    ((uint32_t)0x0050) /**< @brief macPanId Attribute ID */
#define MAC_PROMISCUOUSMODE_ID          ((uint32_t)0x0051) /**< @brief macPromiscuousMode Attribute ID */
#define MAC_SHORTADDRESS_ID             ((uint32_t)0x0053) /**< @brief macShortAddress Attribute ID */
#define MAC_MAXFRAMERETRIES_ID          ((uint32_t)0x0059) /**< @brief macMaxFrameRetries Attribute ID */
#define MAC_TIMESTAMPSUPP_ID            ((uint32_t)0x005C) /**< @brief macTimeStampSupport Attribute ID */
#define MAC_SECURITYENABLED_ID          ((uint32_t)0x005D) /**< @brief macSecurityEnabled Attribute ID */
#define MAC_KEYTABLE_ID                 ((uint32_t)0x0071) /**< @brief macKeyTable Attribute ID */
#define MAC_FRAMECOUNTER_ID             ((uint32_t)0x0077) /**< @brief macFrameCounter Attribute ID */
#define MAC_DUPLICATEDETECTIONTTL_ID    ((uint32_t)0x0078) /**< @brief macDuplicateDetectionTTL Attribute ID */
/* Standard G.9903 PIB attribute ID definitions */
#define MAC_HIGHPRIORITYWS_ID           ((uint32_t)0x0100) /**< @brief macHighPriorityWindowSize Attribute ID */
#define MAC_TXDATAPACKETCOUNT_ID        ((uint32_t)0x0101) /**< @brief macTxDataPacketCount Attribute ID */
#define MAC_RXDATAPACKETCOUNT_ID        ((uint32_t)0x0102) /**< @brief macRxDataPacketCount Attribute ID */
#define MAC_TXCMDPACKETCOUNT_ID         ((uint32_t)0x0103) /**< @brief macTxCmdPacketCount Attribute ID */
#define MAC_RXCMDPACKETCOUNT_ID         ((uint32_t)0x0104) /**< @brief macRxCmdPacketCount Attribute ID */
#define MAC_CSMAFAILCOUNT_ID            ((uint32_t)0x0105) /**< @brief macCSMAFailCount Attribute ID */
#define MAC_CSMANOACKCOUNT_ID           ((uint32_t)0x0106) /**< @brief macCSMAnoACKCount Attribute ID */
#define MAC_RXDATABROADCASTCOUNT_ID     ((uint32_t)0x0107) /**< @brief macRxDataBroadcastCount Attribute ID */
#define MAC_TXDATABROADCASTCOUNT_ID     ((uint32_t)0x0108) /**< @brief macTxDataBroadcastCount Attribute ID */
#define MAC_BADCRCCOUNT_ID              ((uint32_t)0x0109) /**< @brief macBadCRCCount Attribute ID */
#define MAC_NEIGHBORTABLE_ID            ((uint32_t)0x010A) /**< @brief macNeighborTable Attribute ID */
#define MAC_RESERVED2_ID                ((uint32_t)0x010B) /**< @brief Reserved by ITU-T */
#define MAC_CSMAFAIRNESSLIMIT_ID        ((uint32_t)0x010C) /**< @brief macCSMAFairnessLimit Attribute ID */
#define MAC_TMRTTL_ID                   ((uint32_t)0x010D) /**< @brief macTMRTTL Attribute ID */
#define MAC_POSTABLEENTRYTTL_ID         ((uint32_t)0x010E) /**< @brief macPOSTableEntryTTL Attribute ID */
#define MAC_RCCOORD_ID                  ((uint32_t)0x010F) /**< @brief macRCCoord Attribute ID */
#define MAC_TONEMASK_ID                 ((uint32_t)0x0110) /**< @brief macToneMask Attribute ID */
#define MAC_BCNRANDWINLEN_ID            ((uint32_t)0x0111) /**< @brief macBeaconRandomizationWindowLengh Attribute ID */
#define MAC_A_ID                        ((uint32_t)0x0112) /**< @brief macA Attribute ID */
#define MAC_K_ID                        ((uint32_t)0x0113) /**< @brief macK Attribute ID */
#define MAC_MINCWATTEMPTS_ID            ((uint32_t)0x0114) /**< @brief macMinCWAttempts Attribute ID */
#define MAC_CENELECLEGACYMODE_ID        ((uint32_t)0x0115) /**< @brief macCENELECLegacyMode Attribute ID */
#define MAC_FCCLEGACYMODE_ID            ((uint32_t)0x0116) /**< @brief macFCCLegacyMode Attribute ID */
#define MAC_BROADCASTMAXCWENABLE_ID     ((uint32_t)0x011E) /**< @brief macBroadcastMaxCWEnable If enabled (set to 1) MAC uses maximum contention window for transmitting normal priority broadcast packets */
#define MAC_TRANSMITATTEN_ID            ((uint16_t)0x011F) /**< @brief macTransmitAtten Attribute ID */
#define MAC_POSTABLE_ID                 ((uint32_t)0x0120) /**< @brief macPOSTable Attribute ID */
#define MAC_POSRECENTENTRYTHRESHOLD_ID  ((uint32_t)0x0121) /**< @brief macPOSRecentEntryThreshold Attribute ID */
#define MAC_POSRECENTENTRIES_ID         ((uint32_t)0x0122) /**< @brief macPOSRecentEntries Attribute ID */
#define MAC_PREAMBLELENGHT_ID           ((uint32_t)0x0124) /**< @brief macPreambleLength: Number of SyncP in the transmitted preamble - 8 SyncP; 12 SyncP - only FCC */
#define MAC_PANIDIGNORETABLE_ID         ((uint32_t)0x0125) /**< @brief macPanIdIgnoreTable Attribute ID: table of 16 bit PanIds that shall be ignored in Beacon scans */
#define MAC_SCANPANID_ID                ((uint32_t)0x0126) /**< @brief macScanPanId Attribute ID: panId to be used in Enh Beacons, allows limiting scan procedure to a preconfigured PanId */

/* Implementation defined PIB attribute ID definitions */
#define MAC_EXTENDEDADDRESS_ID          ((uint32_t)0x3000) /**< @brief aExtendedAddress Attribute ID: EUI-64 address to be used */
#define MAC_RESERVED3_ID                ((uint32_t)0x3001) /**< @brief Reserved for future use */
#define MAC_TRXSTATENOTCORRECT_ID       ((uint32_t)0x3002) /**< @brief macTrxStateNotCorrect Attribute ID: Counter of discarded frame for RTE state not correct */
#define MAC_PSDUTOLONG_ID	            ((uint32_t)0x3003) /**< @brief macPsduToLong Attribute ID: Counter of discarded frame for PDU too long */
#define MAC_INVALIDFRAMETYPE_ID         ((uint32_t)0x3004) /**< @brief macInvalidFrameType Attribute ID: Counter of discarded frame for Invalid Frame Type field  */
#define MAC_INVALIDFRAMEVERSION_ID      ((uint32_t)0x3005) /**< @brief macInvalidFrameVersion Attribute ID: Counter of discarded frame for invalid Frame Version field  */
#define MAC_INVALIDDAM_ID               ((uint32_t)0x3006) /**< @brief macInvalidDAM Attribute ID: Counter of discarded frame for invalid Destination Address Mode  */
#define MAC_INVALIDSAM_ID               ((uint32_t)0x3007) /**< @brief macInvalidSAM Attribute ID: Counter of discarded frame for invalid Source Address Mode  */
#define MAC_INVALIDPANID_ID             ((uint32_t)0x3008) /**< @brief macInvalidPanID Attribute ID: Counter of discarded frame for invalid PanID  */
#define MAC_INVALIDSHORTADDR_ID         ((uint32_t)0x3009) /**< @brief macInvalidShortAddr Attribute ID: Counter of discarded frame for invalid Short Address  */
#define MAC_INVALIDEXTADDR_ID           ((uint32_t)0x300A) /**< @brief macInvalidExtAddr Attribute ID: Counter of discarded frame for invalid Extended Address  */
#define MAC_INVALIDBEACON_ID            ((uint32_t)0x300B) /**< @brief macInvalidBeacon Attribute ID: Counter of discarded frame for invalid Beacon  */
#define MAC_INVALIDDATACMD_ID           ((uint32_t)0x300C) /**< @brief macInvalidDataCmd Attribute ID: Counter of discarded frame for invalid Command  */
#define MAC_INTERNALSTATENOTCORRECT_ID  ((uint32_t)0x300D) /**< @brief macInternalStateNotCorrect Attribute ID: Counter of discarded frame for invalid state of MAC at data indication */
#define MAC_FRAMEDISCARDED_ID           ((uint32_t)0x300E) /**< @brief macFrameDiscarded Attribute ID: Counter of discarded frame for any of the above reason */
#define MAC_ACKTRXSTATENOTCORRECT_ID    ((uint32_t)0x300F) /**< @brief macAckTrxStateNotCorrect attribute ID: Counter of discarded ACK frames for PHY State not correct  */
#define MAC_ACKNOTCORRECT_ID            ((uint32_t)0x3010) /**< @brief macAckNotCorrect attribute ID: Counter of discarded ACK frames for wrong FCH received */
#define MAC_ACKINTSTATENOTCORRECT_ID    ((uint32_t)0x3011) /**< @brief macAckIntStateNotCorrect attribute ID: Counter of discarded ACK frames for invalid state of the MAC */
#define MAC_ACKCRCNOTEXPECTED_ID        ((uint32_t)0x3012) /**< @brief macAckCrcNotExpected attribute ID: Counter of discarded ACK frames for invalid CRC expected */
#define MAC_ACKDISCARDED_ID             ((uint32_t)0x3013) /**< @brief macAckDiscarded attribute ID: Counter of total discarded ACK frames */
#define MAC_CSMASEED_ID                 ((uint32_t)0x3014) /**< @brief macCSMASeed attribute ID: Seed value in function that generates random numbers */
#define MAC_INVALIDFRAMEPENDING_ID      ((uint32_t)0x3015) /**< @brief macInvalidFramePending attribute ID: Counter of discarded frame for invalid Frame Pending field detected */
#define MAC_INVALIDPIC_ID               ((uint32_t)0x3016) /**< @brief macInvalidPIC attribute ID: Counter of discarded frame for invalid PIC field detected */
#define MAC_PSDUTOOSHORT_ID             ((uint32_t)0x3017) /**< @brief macPsduTooShort Attribute ID: Counter of discarded frame for PDU too short */
#define MAC_CNTFIXDT_ID                 ((uint32_t)0x3018) /**< @brief macCntFixDT Attrifbute ID: This attribute (b31 b30 ... b2 b1 b0) contain two fields:
                                                                      b28-0:  represent a counter indicating the number of time the DT indicated in bits b31-29 must be inserted in the FCH composed by rtMAC and passed to phy
                                                                      b31-29: represent a DT value that must be used by rtMAC to compose an FCH passed to PHY */
#define MAC_DISABLEPLC_ID               ((uint32_t)0x3019) /**< @brief macDisablePLC Attribute ID: This attribute indicates if modem is enabled (0) or disabled(1) when modem is disabled all counter managed by MAC are frozen, counters managed by phy are not frozen */
#define MAC_STARTLOWERINGRETRIES_ID     ((uint32_t)0x301A) /**< @brief macStartLoweringRetries Attribute ID: If this attribute has value different from 0 indicate in which retransmission start lowering modulation mechanism, value 0 indicate that lowering modulation mechanism is disabled. */
#define MAC_TIMERWABEND_ID              ((uint32_t)0x301B) /**< @brief macTimerWABEnd Attribute ID: Counter of number of times after channel sensed busy was received nothing from phy */
#define MAC_DONTSENDNACK_ID             ((uint32_t)0x301C) /**< @brief macDontSendNACK Attribute ID: Never send NACK frame */
#define MAC_ENABLECCTT230_ID            ((uint32_t)0x301D) /**< @brief macEnableCCTT230Feature Attribute ID: If this attribute is set to 1 is enabled the feature indicated in CCTT230 */
#define MAC_LOWERINGBEHAVIOR_ID         ((uint32_t)0x301E) /**< @brief macBehaviorWhenLowering Attribute ID: This attribute determines the behavior of rtMAC when the lowering of modulation order in retransmissions is executed.  */
#define MAC_SNRDBROBO_ID                ((uint32_t)0x3280) /**< @brief macSNRdBROBO Attribute ID: tone map algorithm thresold for ROBO */
#define MAC_SNRDBDBPSK_ID               ((uint32_t)0x3281) /**< @brief macSNRdBDBPSK Attribute ID: tone map algorithm thresold for xBPSK */
#define MAC_SNRDBDQPSK_ID               ((uint32_t)0x3282) /**< @brief macSNRdBDQPSK Attribute ID: tone map algorithm thresold for xQPSK */
#define MAC_SNRDBD8PSK_ID               ((uint32_t)0x3283) /**< @brief macSNRdBD8PSK Attribute ID: tone map algorithm thresold for x8PSK */
#define MAC_RXBCNLASTSCAN_ID            ((uint32_t)0x3284) /**< @brief macRXBeaconReceivedFromLastScan: Number of received beacon from the start of the scan */
#define MAC_MODULATIONCONTROL_ID        ((uint32_t)0x3285) /**< @brief macModulationControl: Field mask to control the modulation used by the MAC Layer where
                                                                      Bit [0..3]: defines the modulation to be used for the data transmission (0: ROBO, 1: DBPSK or BPSK, 2: DQPSK or QPSK, 3: D8PSK or 8PSK, F use neighbor table)
                                                                      Bit [4..7]: defines the modulation scheme to be asked in the tone map response (0: ROBO, 1: DBPSK or BPSK, 2: DQPSK or QPSK, 3: D8PSK or 8PSK, F use the internal algorithm)
                                                                      Bit 8: if 1 force a scheme selection in the tone map response. Scheme is selected through bit 9
                                                                      Bit 9: if bit 8 indicate to foce the scheme this bit indicates 0 for differential 1 for coherent
                                                                      Bit [10..11]: defines the modulation to be used for the broadcast data transmission (0: ROBO, 1: DBPSK or BPSK, 2: DQPSK or QPSK, 3: D8PSK or 8PSK)
                                                                      Bit 12: defines the modulation scheme to be used for broadcast transmission (0: differential, 1: coherent)
                                                                      Bit 13: if set to 1 force the scheme specified in Bit 14
                                                                      Bit 14: if set to 0 force differential, if set to 1 force coherent for unicast data transmission
                                                                      Bit [15..31]: not used */
#define MAC_OPTIONALFEATURES_ID         ((uint32_t)0x3286) /**< @brief macOptionalFeatures: Field mask to control optional features provided by the MAC Layer where
                                                                      Bit 0: if 1 enable the TMR bit in the tone map response
                                                                      Bit 1: if 1 send the ToneMap Response message cyphered
                                                                      Bit 2: if 1 set the TMR bit also if the source address is 64bit
                                                                      Bit 3: set the ToneMap Response priority (0: Normal priority, 1: High priority)
                                                                      Bit 4: if 1 export the MAC frames with both 64 bit src and dst addresses through the MCPS-DATA-EXTADDR.ind
                                                                      Bit 5: if 1 ignore beacon request frames and do not reply to them
                                                                      Bit 6: if 1 do not send ToneMapResponse framesif 1
                                                                      Bit 7: if 1 do not invoke MLME-COMM-STATUS.Indication for PAN_ID_CONFILCT
                                                                      Bit 8: if 1 do not invoke MLME-COMM-STATUS.Indication for security errors (COUNTER_ERROR, SECURITY_ERROR, UNAVAILABLE_KEY, UNSUPPORTED_SECURITY, IMPROPER_KEY_TYPE)
                                                                      Bit 9: if 1 do not invoke MLME-COMM-STATUS.Indication for other errors
                                                                      Bit 10: if 1 filter the MLME-COMM-STATUS.Indications for PAN_ID_CONFILCT (only one indication every macPanIdConflictTableTTL is issued for the same alternate PANId) */
#define MAC_TMRMMODULATIONCONTROL_ID    ((uint32_t)0x3287) /**< @brief macTMRModulationControl: Field mask to control the modulation used by the MAC Layer during data transmission when TMR bit is set
                                                                      Bit [0..2]: defines the modulation to be used to the first data tranmission (0: ROBO, 1: DBPSK or BPSK, 2: DQPSK or QPSK, 3: D8PSK or 8PSK)
                                                                      Bit 3: defines the modulation scheme to be used to the first data tranmission (when TMR bit is set) (0: differential, 1: coherent)
                                                                      Bit [4..6]: defines the number of negative step with respect to maximum tx power (only negative values of TxGain)
                                                                      Bit 7: defines the TxRes(0: 3 dB, 1: 6 dB)
                                                                      Bit [8..31]: defines the tone map field */
#define MAC_TMRMINNUMCARRIER_ID         ((uint32_t)0x3288) /**< @brief macTMRMinNumCarrier: minimum number of carrier that the must be used when selecting a modulation in the tone map response algorithm */
#define MAC_SNRDBSUPERROBO_ID           ((uint32_t)0x3289) /**< @brief macSNRdBSuperROBO Attribute ID: tone map algorithm thresold for SuperROBO */
#define MAC_MAXTXPOWER_ID               ((uint32_t)0x328A) /**< @brief macMaxTxPower Attribute ID: default value for the maxium power */
#define MAC_DATATXTIMEOUT_ID            ((uint32_t)0x328B) /**< @brief macDataTxTimeout Attribute ID: timeout for a MCPS-DATA.confirm generation (seconds) */
#define MAC_DEVICETABLE_ID              ((uint32_t)0x328C) /**< @brief macDeviceTable Attribute ID: this is used to get the MAC device table entry by entry */
#define MAC_NEIGHBORTABLE_BYSHORT_ID    ((uint32_t)0x328D) /**< @brief macNeighborTable Attribute ID, index is used as short address */
#define MAC_NEIGHBORTABLE_VALIDENTRY_ID ((uint32_t)0x328E) /**< @brief macNeighborTable Attribute ID but the entry returned is the next one valid */
#define MAC_SNRDB_USEDIFF8PSK_ID        ((uint32_t)0x328F) /**< @brief macSNRdBUseDiff8PSK Attribute ID: indicates the number of dB with respect to the macSNRdBD8PSK to force the switch to differential */
#define MAC_POSTABLE_VALIDENTRY_ID      ((uint32_t)0x3290) /**< @brief macPOSTable Attribute ID, but the entry returned is the next one valid */
#define MAC_POSTABLE_RF_S_ID            ((uint32_t)0x3291) /**< @brief macPOSTable_RF_S Attribute ID */
#define MAC_NEIGHBORTABLE_S_ID          ((uint32_t)0x3292) /**< @brief macNeighborTable_S Attribute ID */
#define MAC_RESERVED6_ID                ((uint32_t)0x3293) /**< @brief Reserved for future use */
#define MAC_IMPULSIVENOISELOWERING_ID   ((uint32_t)0x3294) /**< @brief macImpulsiveNoiseLowering Attribute ID that specify how many modulation step we have to lower when impuslive noise is present */
#define MAC_DELAYTXAFTERTMR_ID          ((uint32_t)0x3295) /**< @brief macDelayTxAfterTMR Attribute ID that specifies the maxiumum time (in ms) the node has to wait before sending a next MAC frame after the TMR bit was set in a previous transmission */
#define MAC_TMRLQILIMITS_ID             ((uint32_t)0x3296) /**< @brief macTMRLQILimits Attribute ID: upper limit (b0…b7) and lower limit (b8…b15) for LQI value to be inserted in ToneMapResponse frame */
#define MAC_DEVICETABLE_BYSHORT_ID      ((uint32_t)0x3297) /**< @brief macDeviceTable Attribute ID, index is used as short address */
#define MAC_DEVICETABLE_VALIDENTRY_ID   ((uint32_t)0x3298) /**< @brief macDeviceTable Attribute ID but the entry returned is the next one valid */
#define MAC_ACKTIMEOUT_RF_ID            ((uint32_t)0x3299) /**< @brief macAckTimeout_RF Attribute ID: maximum time (in ms) within which the complete reception of an ACK frame is expected. After this time a retransmission is carried out  */
#define MAC_MAXBACKOFFTIME_RF_ID        ((uint32_t)0x329A) /**< @brief macMaxBackoffTime_RF Attribute ID: maximum backoff time (in ms) for RF transmission 1st attempt (not done from S2LP CSMA) */
#define MAC_POSTABLE_VALIDENTRY_RF_ID   ((uint32_t)0x329B) /**< @brief macPOSTableValidEntry_RF: macPOSTable_RF Attribute ID, but the entry returned is the next one valid */
#define MAC_DEVICETABLE_BYSHORT_RF_ID   ((uint32_t)0x329C) /**< @brief macDeviceTableByShortAddr_RF: macDeviceTable_RF Attribute ID, index is used as short address */
#define MAC_DEVICETABLE_VALIDENTRY_RF_ID ((uint32_t)0x329D) /**< @brief macDeviceTableValidEntry_RF: macDeviceTable_RF Attribute ID but the entry returned is the next one valid */
#define MAC_PANIDCONFLICTTABLETTL_ID    ((uint32_t)0x329E) /**< @brief macPanIdConflictTableTTL: Time to Live (in minutes) of an entry in the macPanIdConflictTable */
#define MAC_DATAREQTIMEOUT_ID           ((uint32_t)0x329F) /**< @brief macDataReqTimeout Attribute ID: timeout for a HyAL-DATA.confirm generation (seconds) */
#define MAC_PANIDCONFLICTTABLETTL_RF_ID ((uint32_t)0x32A0) /**< @brief macPanIdConflictTableTTL_RF: Time to Live (in minutes) of an entry in the macPanIdConflictTable_RF */
#define MAC_NOISE_RF_ID                 ((uint32_t)0x32A1) /**< @brief macNoise_RF: Noise measured on last RF packet received */
#define MAC_PHYQUALITYRXTABLE_ID        ((uint32_t)0x32A2) /**< @brief macPhyQualityRxTable: this is used to get the PHY quality table entry by entry */
#define MAC_PHYQUALITYRXTABLE_BYSHORT_ID ((uint32_t)0x32A3) /**< @brief macPhyQualityRxTableByShortAddr: index is used as short address */
#define MAC_PHYQUALITYRXTABLETTL_ID      ((uint32_t)0x32A4) /**< @brief macPhyQualityRxTableTTL: index is used as short address */
#define MAC_PHYQUALITYRXTABLE_VALIDENTRY_ID ((uint32_t)0x32A5) /**< @brief macPhyQualityRxTableValidEntry: index but the entry returned is the next one valid */
#define MAC_FHTIMINGACCURACY_RF_ID       ((uint32_t)0x32A6) /**< @brief macFHTimingAccuracy_RF: timing accuracy of the local node */
#define MAC_FHUNICASTSCHEDULESTART_RF_ID ((uint32_t)0x32A7) /**< @brief macFHUnicastScheduleStart_RF: absolute time of the start of the unicast schedule */
#define MAC_COORDSHORTADDRESS_ID         ((uint32_t)0x32A8) /**< @brief macCoordShortAddress: duplicated of adpCoordShortAddress */
#define MAC_FHTIMEDEVENTTXMARGIN_RF_ID   ((uint32_t)0x32A9) /**< @brief macFHTimedEventTxMargin_RF: time in ms for timed event Tx margin */
#define MAC_FHPROPAGATIONTIME_RF_ID      ((uint32_t)0x32AA) /**< @brief macFHPropagationTime_RF: time in ms for sending ack/beacon from MAC to RF-Task */
#define MAC_FHPREAMBLEDURATION_RF_ID     ((uint32_t)0x32AB) /**< @brief macFHPreambleDuration_RF: additional left guard time in ms used to take into account the message preamble + sync duration */
#define MAC_RTE_DISABLE_ID               ((uint32_t)0x32AC) /**< @brief macRTEDisable: used for manually switching off RTE */
#define MAC_POSTABLE_BYSHORT_ID          ((uint32_t)0x32AD) /**< @brief macPOSTableByShortAddr: index is used as short address */
#define MAC_POSTABLE_BYSHORT_RF_ID       ((uint32_t)0x32AE) /**< @brief macPOSTableByShortAddr_RF: index is used as short address */
#define MAC_FHBROADCASTTIMINGTABLE_RF_ID ((uint32_t)0x32AF) /**< @brief macFHBroadcastTimingTable_RF: attribute for get/set the broadcast domain for FH */
#define MAC_LNCSLSLOTLENGHT_RF_ID        ((uint32_t)0x32B0) /**< @brief macLNCSLSlotLenght_RF: Duration in ms of a CSL slot */
#define MAC_KEY_REMOVAL_ID               ((uint32_t)0x32B1) /**< @brief macKeyRemoval: index is used to set a key as invalid */
#define MAC_FHUNICASTSCHEDULEPANID_RF_ID ((uint32_t)0x32B2) /**< @brief macFHUnicastSchedulePanId_RF: PanId of the LBA used to compute the unicast listening channel of the device during Join */
#define MAC_FHACKPROPAGATIONTIME_RF_ID   ((uint32_t)0x32B3) /**< @brief macFHAckPropagationTime_RF time in ms for sending ack from MAC to S2LP Tx cmd*/
#define MAC_LNPOSTABLE_BYSHORT_RF_ID     ((uint32_t)0x32B4) /**< @brief macLNPOSTableByShortAddr_RF: macLNPOSTable_RF Attribute ID, index is used as short address */
#define MAC_LNPOSTABLE_VALIDENTRY_RF_ID  ((uint32_t)0x32B5) /**< @brief macLNPOSTableValidEntry_RF: macLNPOSTable_RF Attribute ID, but the entry returned is the next one valid */
#define MAC_LNCSLSLOTEXP_RF_ID           ((uint32_t)0x32B6) /**< @brief macLNCSLSlotExp_RF: the exponent N used to compute the number of CSL slots */


/* G3-PLC RF Hybrid Profile defined PIB attribute ID definitions */
#define MAC_DSN_RF_ID                   ((uint32_t)0x0200) /**< @brief macDsn_RF Attribute ID */
#define MAC_MAXBE_RF_ID                 ((uint32_t)0x0201) /**< @brief macMaxBE_RF Attribute ID */
#define MAC_MAXCSMABACKOFFS_RF_ID       ((uint32_t)0x0202) /**< @brief macMaxCSMABackoffs_RF Attribute ID */
#define MAC_MAXFRAMERETRIES_RF_ID       ((uint32_t)0x0203) /**< @brief macMaxFrameRetries_RF Attribute ID */
#define MAC_MINBE_RF_ID                 ((uint32_t)0x0204) /**< @brief macMinBE_RF Attribute ID */
#define MAC_TIMESTAMPSUPPORTED_RF_ID    ((uint32_t)0x0205) /**< @brief macTimestampSupported_RF Attribute ID */
#define MAC_DEVICETABLE_RF_ID           ((uint32_t)0x0206) /**< @brief macDeviceTable_RF Attribute ID */
#define MAC_FRAMECOUNTER_RF_ID          ((uint32_t)0x0207) /**< @brief macFrameCounter_RF Attribute ID */
#define MAC_DUPLICATEDETECTIONTTL_RF_ID ((uint32_t)0x0208) /**< @brief macDuplicateDetectionTTL_RF Attribute ID */
#define MAC_COUNTEROCTETS_RF_ID         ((uint32_t)0x0209) /**< @brief macCounterOctets_RF Attribute ID */
#define MAC_RETRYCOUNT_RF_ID            ((uint32_t)0x020A) /**< @brief macRetryCount_RF Attribute ID */
#define MAC_MULTIPLERETRYCOUNT_RF_ID    ((uint32_t)0x020B) /**< @brief macMultipleRetryCount_RF Attribute ID */
#define MAC_TXFAILCOUNT_RF_ID           ((uint32_t)0x020C) /**< @brief macTxFailCount_RF Attribute ID */
#define MAC_TXSUCCESSCOUNT_RF_ID        ((uint32_t)0x020D) /**< @brief macTxSuccessCount_RF Attribute ID */
#define MAC_FCSERRORCOUNT_RF_ID         ((uint32_t)0x020E) /**< @brief macFcsErrorCount_RF Attribute ID */
#define MAC_SECURITYFAILURE_RF_ID       ((uint32_t)0x020F) /**< @brief macSecurityFailure_RF Attribute ID */
#define MAC_DUPLICATEFRAMECOUNT_RF_ID   ((uint32_t)0x0210) /**< @brief macDuplicateFrameCount_RF Attribute ID */
#define MAC_RXSUCCESSCOUNT_RF_ID        ((uint32_t)0x0211) /**< @brief macRxSuccessCount_RF Attribute ID */
#define MAC_EBRPERMITJOINING_RF_ID      ((uint32_t)0x0213) /**< @brief macEbrPermitJoining_RF Attribute ID */
#define MAC_EBRFILTERS_RF_ID            ((uint32_t)0x0214) /**< @brief macEbrFilters_RF Attribute ID */
#define MAC_EBRATTRIBUTELIST_RF_ID      ((uint32_t)0x0215) /**< @brief macEbrAttributeList_RF Attribute ID */
#define MAC_BEACONAUTORESPOND_RF_ID     ((uint32_t)0x0216) /**< @brief macBeaconAutoRespond_RF Attribute ID */
#define MAC_USEENHANCEDBEACON_RF_ID     ((uint32_t)0x0217) /**< @brief macUseEnhancedBeacon_RF Attribute ID */
#define MAC_EBHEADERIELIST_RF_ID        ((uint32_t)0x0218) /**< @brief macEbHeaderIeList_RF Attribute ID */
#define MAC_EBPAYLOADIELIST_RF_ID       ((uint32_t)0x0219) /**< @brief macEbPayloadIeList_RF Attribute ID */
#define MAC_EBFILTERINGENABLED_RF_ID    ((uint32_t)0x021A) /**< @brief macEbFilteringEnabled_RF Attribute ID */
#define MAC_EBSN_RF_ID                  ((uint32_t)0x021B) /**< @brief macEbsn_RF Attribute ID */
#define MAC_EBAUTOSA_RF_ID              ((uint32_t)0x021C) /**< @brief macEbAutoSa_RF Attribute ID */
#define MAC_POSTABLE_RF_ID              ((uint32_t)0x021D) /**< @brief macPOSTable_RF Attribute ID */
#define MAC_OPERATINGMODE_RF_ID         ((uint32_t)0x021E) /**< @brief macOperatingMode_RF Attribute ID: The RF operating mode as defined in clause 20.3 of [IEEE 802.15.4v] */
#define MAC_CHANNELNUMBER_RF_ID         ((uint32_t)0x021F) /**< @brief macChannelNumber_RF Attribute ID: The channel number as defined in cause 10.1.2.8 of [IEEE 802.15.4v] to be used for channel scanning and data transmission */
#define MAC_DUTYCYCLEUSAGE_RF_ID        ((uint32_t)0x0220) /**< @brief macDutyCycleUsage_RF Attribute ID: Current usage of maximum allowed duty cycle in percent over the current sliding-window measurement period, i.e. ( t_on / macDutyCycleLimit_RF) * 100 */
#define MAC_DUTYCYCLEPERIOD_RF_ID       ((uint32_t)0x0221) /**< @brief macDutyCyclePeriod_RF Attribute ID: Duration of the measurement period in seconds, e.g. 3600 seconds as default for [ETSI EN 303 204] */
#define MAC_DUTYCYCLELIMIT_RF_ID        ((uint32_t)0x0222) /**< @brief macDutyCycleLimit_RF Attribute ID: Duration of the allowed transmission time in seconds, e.g. 2,5%/10% of 3600 seconds as default for [ETSI EN 303 204]. If this attribute is set equal to macDutyCyclePeriod_RF the value of macDutyCycleUsage_RF will be fixed to zero. */
#define MAC_DUTYCYCLETHRESHOLD_RF_ID    ((uint32_t)0x0223) /**< @brief macDutyCycleThreshold_RF Attribute ID: Duty cycle threshold for stopping RF transmissions. */
#define MAC_RESERVED7_ID                ((uint32_t)0x0224) /**< @brief Reserved by ITU-T  */
#define MAC_FREQUENCYBAND_RF_ID         ((uint32_t)0x0225) /**< @brief macFrequencyBand_RF Attribute ID:  */
#define MAC_TRANSMITATTEN_RF_ID         ((uint32_t)0x0226) /**< @brief macTransmitAtten_RF Attribute ID:  */
#define MAC_ADAPTIVEPOWERSTEP_RF_ID     ((uint32_t)0x0227) /**< @brief macAdaptivePowerStep_RF Attribute ID:  */
#define MAC_ADAPTIVEPOWERHIGHBOUND_RF_ID ((uint32_t)0x0228) /**< @brief macAdaptivePowerHighBound_RF Attribute ID:  */
#define MAC_ADAPTIVEPOWERLOWBOUND_RF_ID ((uint32_t)0x0229) /**< @brief macAdaptivePowerLowBound_RF Attribute ID:  */
#define MAC_POSRECENTENTRIES_RF_ID      ((uint32_t)0x022A) /**< @brief macPOSRecentEntries_RF Attribute ID */
#define MAC_HOPPINGENABLED_RF_ID             ((uint32_t)0x022B) /**< @brief macHoppingEnabled_RF Attribute ID: controls if the frequency hopping mechanism is enabled */
#define MAC_BROADCASTINTERVALDURATION_RF_ID  ((uint32_t)0x022C) /**< @brief macBroadcastIntervalDuration_RF Attribute ID: duration of the broadcast interval (time between the start of 2 broadcast slots) in ms */
#define MAC_BROADCASTSLOTDURATION_RF_ID      ((uint32_t)0x022D) /**< @brief macBroadcastSlotDuration_RF Attribute ID: duration of the broadcast slot in ms */
#define MAC_UNICASTSLOTDURATION_RF_ID        ((uint32_t)0x022E) /**< @brief macUnicastSlotDuration_RF Attribute ID: duration of a unicast slot in ms */
#define MAC_EXTENDEDBITMAP_RF_ID             ((uint32_t)0x022F) /**< @brief macExtendedBitmap_RF Attribute ID: abitmap of 129 bits for tracing active channels in FH */
#define MAC_BCNRANDWINLEN_RF_ID              ((uint32_t)0x0230) /**< @brief macBeaconRandomizationWindowLength_RF  Attribute ID: duration time in ms for beacon randomization */
#define MAC_ADDITIONALCHSCANTIME_RF_ID       ((uint32_t)0x0231) /**< @brief macAdditionalChannelScanTime_RF Attribute ID: additional time  in ms to beacon randomization */
#define MAC_MAXCCAATTEMPSRETRIES_RF_ID       ((uint32_t)0x0232) /**< @brief macMaxCcaAttemptsRetries_RF Attribute ID: number of retry attempts after CCA failure */
#define MAC_MININTERTXINTERVAL_RF_ID         ((uint32_t)0x0233) /**< @brief macMinInterTxInterval_RF Attribute ID: minimum interval in ms in a channel between two transmissions by the same device */
#define MAC_INITIALRETRYTIME_RF_ID           ((uint32_t)0x0234) /**< @brief macInitialRetryTime_RF Attribute ID: initial frame retransmission time in milliseconds */
#define MAC_MAXIMUMRETRYTIME_RF_ID           ((uint32_t)0x0235) /**< @brief macMaximumRetryTime_RF Attribute ID: maximum frame retransmission time in milliseconds */
#define MAC_MAXCLOCKDRIFT_RF_ID              ((uint32_t)0x0236) /**< @brief macMaxClockDrift_RF Attribute ID: clock drift of the device */
#define MAC_FHUNICASTSCHEDULEADDRESS_RF_ID   ((uint32_t)0x0237) /**< @brief macFHUnicastScheduleAddress_RF Attribute ID: short address used to compute the unicast listening channel of the device */
#define MAC_FHUNICASTSLOTNUMBER_RF_ID        ((uint32_t)0x0238) /**< @brief macFHUnicastSlotNumber_RF Attribute ID: current slot number for the unicast listening schedule */
#define MAC_FHUNICASTOFFSET_RF_ID            ((uint32_t)0x0239) /**< @brief macFHUnicastOffset_RF Attribute ID: offset in milliseconds from the start of the current unicast slot */
#define MAC_MINGUARDTIME_RF_ID               ((uint32_t)0x023A) /**< @brief macMinGuardTime_RF Attribute ID: minimum guard time in ms to be used for FH communication */
#define MAC_MAXBCASTRESYNCHWAITUNIT_RF_ID    ((uint32_t)0x023B) /**< @brief macMaxBcastResynchWaitUnit_RF Attribute ID: max num of broadcast intervals to wait for the broadcast frame during the resynch procedure */
#define MAC_SCANTYPE_RF_ID                   ((uint32_t)0x023C) /**< @brief macScanType_RF Attribute ID: Configure the MLME-Scan behavior in FH mode (0: default scan; 1: halt on 1st PanId found) */
#define MAC_BEACONRATIOMODIFIER_RF_ID        ((uint32_t)0x023D) /**< @brief macBeaconRatioModifier_RF Attribute ID: used to modify the Beacon randomization window, set to 100% to disable */
#define MAC_LQIFILTER_RF_ID                  ((uint32_t)0x023E) /**< @brief macLQIFilter_RF Attribute ID: minimum LQI value used in BF-IE, set to 0 to disable */
#define MAC_PERCENTFILTER_RF_ID              ((uint32_t)0x023F) /**< @brief macPercentFilter_RF Attribute ID: probability in BF-IE, set to 100% to disable */
#define MAC_LNSYNCPERIOD_RF_ID               ((uint32_t)0x0240) /**< @brief macLNSyncPeriod_RF Attribute ID: number of LN bcast intervals between bcast synch message sent by PN */
#define MAC_LNBCINTERVALLENGHT_RF_ID         ((uint32_t)0x0241) /**< @brief macLNBCIntervalLenght_RF Attribute ID: duration of LN bcast interval in seconds */
#define MAC_MAXWAKEUPFRAMES_RF_ID            ((uint32_t)0x0242) /**< @brief macMaxWakeupFrames_RF Attribute ID: maximum number of wakeup frames to be sent from PN to LN before considering the link broken */
#define MAC_LNPOSTABLE_RF_ID                 ((uint32_t)0x0243) /**< @brief macLNPOSTable_RF Attribute ID: POS table for storage of LN information into PN */
#define MAC_LNTIMINGTABLE_RF_ID              ((uint32_t)0x0244) /**< @brief macLNTimingTable_RF Attribute ID: Table of one element for storing tiing info into the LN (LN Only) */
#define MAC_LNPOSTABLEENTRYTTL_RF_ID         ((uint32_t)0x0245) /**< @brief macLNPOSTableEntryTTL_RF Attribute ID: default lifetime of a macLNPOSTable_RF entry in minutes */
#define MAC_LNUCINTERVALLENGHT_RF_ID         ((uint32_t)0x0246) /**< @brief macLNUCIntervalLenght_RF Attribute ID: requested duration of LN ucast interval in seconds (LN Only) */
#define MAC_EMPTYFRAMESECURITYLEVEL_RF_ID    ((uint32_t)0x0247) /**< @brief macEmptyFrameSecurityLevel_RF Attribute ID: minimum security level used for incoming and outgoing frames with empty MAC payload */
#define MAC_EMPTYFRAMEKEYINDEX_RF_ID         ((uint32_t)0x0248) /**< @brief macEmptyFrameKeyIndex_RF Attribute ID: index of the GMK used for transmission of frames with empty MAC payload */

/* Bitmask definitions */
#define MAC_MODULATIONCONTROL_TXMOD                 ((uint32_t)0x0000000F) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_TMMOD                 ((uint32_t)0x000000F0) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_FORCETMSCHEME         ((uint32_t)0x00000100) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_TMSCHEME              ((uint32_t)0x00000200) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_BRMOD                 ((uint32_t)0x00000C00) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_BRSCHEME              ((uint32_t)0x00001000) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_FORCEUNISCHEME        ((uint32_t)0x00002000) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_UNISCHEME_COHE        ((uint32_t)0x00004000) /**< @brief mask for macModulationControl parameter */
#define MAC_MODULATIONCONTROL_MOD_DISABLE           ((uint32_t)0x0000000F) /**< @brief mask for macModulationControl parameter */

#define MAC_OPTIONALFEATURES_SETTMR                 ((uint32_t)0x00000001) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_TMRESPENCR             ((uint32_t)0x00000002) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_TMREXTADDR             ((uint32_t)0x00000004) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_TMRESPPRIO             ((uint32_t)0x00000008) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_EXPEXTADDRS            ((uint32_t)0x00000010) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_IGNOREBCNREQ           ((uint32_t)0x00000020) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_STOPTMR                ((uint32_t)0x00000040) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_NOCOMMIND_PANCONFLICT  ((uint32_t)0x00000080) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_NOCOMMIND_SECURITYERR  ((uint32_t)0x00000100) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_NOCOMMIND_OTHER        ((uint32_t)0x00000200) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_PANCONFLICTFILTER      ((uint32_t)0x00000400) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_BCASTTIMINGTABLE       ((uint32_t)0x00000800) /**< @brief mask for macOptionalFeatures parameter */
#define MAC_OPTIONALFEATURES_ADDLNUCTRIE            ((uint32_t)0x00001000) /**< @brief mask for macOptionalFeatures parameter */

#define MAC_OPTIONALFEATURES_DEFAULTVALUE           (MAC_OPTIONALFEATURES_TMRESPPRIO) /**< @brief optional features activated by default */
#define MAC_OPTIONALFEATURES_MAXVALUE               (MAC_OPTIONALFEATURES_SETTMR | \
                                                     MAC_OPTIONALFEATURES_TMRESPENCR | \
                                                     MAC_OPTIONALFEATURES_TMREXTADDR | \
                                                     MAC_OPTIONALFEATURES_TMRESPPRIO | \
                                                     MAC_OPTIONALFEATURES_EXPEXTADDRS | \
                                                     MAC_OPTIONALFEATURES_IGNOREBCNREQ | \
                                                     MAC_OPTIONALFEATURES_STOPTMR | \
                                                     MAC_OPTIONALFEATURES_NOCOMMIND_PANCONFLICT | \
                                                     MAC_OPTIONALFEATURES_NOCOMMIND_SECURITYERR | \
                                                     MAC_OPTIONALFEATURES_NOCOMMIND_OTHER | \
                                                     MAC_OPTIONALFEATURES_PANCONFLICTFILTER | \
                                                     MAC_OPTIONALFEATURES_BCASTTIMINGTABLE | \
                                                     MAC_OPTIONALFEATURES_ADDLNUCTRIE)

#define MAC_TMRMODULATIONCONTROL_MOD                ((uint32_t)0x00000007) /**< @brief mask for macTMRModulationControl parameter */
#define MAC_TMRMODULATIONCONTROL_SCHEME             ((uint32_t)0x00000008) /**< @brief mask for macTMRModulationControl parameter */
#define MAC_TMRMODULATIONCONTROL_TXGAIN             ((uint32_t)0x00000070) /**< @brief mask for macTMRModulationControl parameter */
#define MAC_TMRMODULATIONCONTROL_TXRESP             ((uint32_t)0x00000080) /**< @brief mask for macTMRModulationControl parameter */

#define MAC_TONEMASK_LEN                (PHY_TONEMASK_SIZE) /**< @brief The size of the macToneMask parameter in Bytes */

#define MAC_RCCOORD_COORD_DEF       ((uint16_t)0x0000)  /**< @brief The PAN coordinator shall set its RC_COORD to 0x0000  */
#define MAC_RCCOORD_DEV_NOROUTE     ((uint16_t)0x7FFF)  /**< @brief A device shall initialize RC_COORD to 0x7FFF on association or it is at adpMaxHops  */
#define MAC_RCCOORD_DEV_COMM_FAILED ((uint16_t)0xFFFF)  /**< @brief If a device has failed to communicate with the PAN coordinator it shall set RC_COORD to its maximum value of 0xFFFF  */

#define MAC_EXTENDEDBITMAP_RF_SIZE  ((uint8_t)17)       /**< @brief Size of macExtendedBitmap_RF (129bits + 7 padding)  */
#define MAC_EXTENDEDBITMAP_RF_MIN   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /**< @brief Min value of macExtendedBitmap_RF  */
#define MAC_EXTENDEDBITMAP_RF_MAX   {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /**< @brief Max value of macExtendedBitmap_RF  */
#define MAC_BCNRANDWINLEN_RF_DEF_SF ((uint16_t)0x2EE0)  /**< @brief Default value for the PIB macBeaconRandomizationWindowLength_RF to be set when single freq is set  */
#define MAC_BCNRANDWINLEN_RF_DEF_FH ((uint16_t)0x0032)  /**< @brief Default value for the PIB macBeaconRandomizationWindowLength_RF to be set when FH is enabled  */

#define MAC_UCASTSLOTDUR_DEF        ((uint8_t)100)      /**< @brief Default value for the PIB macUnicastSlotDuration_RF in milliseconds */
#define MAC_BCASTSLOTDUR_DEF        ((uint8_t)100)      /**< @brief Default value for the PIB macBroadcastSlotDuration_RF in milliseconds */
#define MAC_BCASTINTVDUR_DEF        ((uint16_t)400)     /**< @brief Default value for the PIB macBroadcastIntervalDuration_RF in milliseconds */
#define MAC_UCASTSCHADDR_DEF        ((uint16_t)0xFFFF)  /**< @brief Default value for the PIB macFHUnicastScheduleAddress_RF */

#define MAC_DATATXTIMEOUT_RF        ((uint8_t)5)        /**< @brief Max time in seconds to wait for a confirm on DataTx_RF  */

#define MAC_FH_SEQLEN               ((uint32_t)0x00010000)  /**< @brief Number of slots in the channel hopping frequency */
#define MAC_FHTIE_UFSI_MASK         ((uint32_t)0x00FFFFFF)  /**< @brief Bitmask for selecting UFSI from FHT-IE */
#define MAC_FHTIE_CD_MASK           ((uint32_t)0xFF000000)  /**< @brief Bitmask for selecting CD from FHT-IE */
#define MAC_FHUTIE_UFSI_MASK        ((uint32_t)0x00FFFFFF)  /**< @brief Bitmask for selecting UFSI from FHUT-IE */
#define MAC_FH_BSI_DEF              ((uint8_t)0xFF)

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */


#endif /*HI_MAC_PIB_ATTRIBUTES_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
