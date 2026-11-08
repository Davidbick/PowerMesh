/*****************************************************************************
 *   @file    hif_g3_common.h
 *   @author  AMG/IPC Application Team
 *   @brief   Header file to define the interface for the Common HI functions.
 *
 * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 *******************************************************************************/

#ifndef HIF_G3_COMMON_H
#define HIF_G3_COMMON_H

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
 * @{
 */

/** @defgroup hif_g3_common HIF G3 COMMON
 * @{
 */

/** @defgroup hif_g3_common_Constants HIF G3 COMMON Constants
 * @{
 */

#include <settings.h>

/* Definitions */

/* Tiing */
#define TIMEOUT_HWRESET      		3000 /* In ms, timeout for the reception of the initial HI-HWRESET-CNF */
#define TIMEOUT_BAUDRATE_SET		1000 /* In ms, timeout for the reception of the HI-BAUDRATE_SET-CNF */
#define TIMEOUT_TRACE				1000 /* In ms, timeout for the reception of the HI-TRACE-CNF */

/* UDP ports used in connections for User G3 */
#define TEST_LOCAL_PORT				1000		/*!< Local port for the connection used for tests */
#define TEST_REMOTE_PORT			1000		/*!< Remote port for the connection used for tests */

#define TRANSFER_LOCAL_PORT    		2000		/*!< Local port for the connection used for transfers */
#define TRANSFER_REMOTE_PORT   		2000		/*!< Remote port for the connection used for transfers */

/* UDP ports used in connections for G3 */
#define LAST_GASP_LOCAL_PORT		50		/*!< Local port for the connection used for Last Gasp */
#define LAST_GASP_REMOTE_PORT		50		/*!< Remote port for the connection used for Last Gasp */

/* Custom types */

/* Macros */

/* Exported types */

typedef enum connection_id_enum
{
	TEST_CONN_ID		= 0U,	/*!< ID for the connection used for tests */
	TRANSFER_CONN_ID,			/*!< ID for the connection used for transfers */
#if ENABLE_LAST_GASP
	LAST_GASP_CONN_ID,			/*!< ID for the connection used for Last Gasp (cannot be used in User G3) */
#endif
	CONN_NUMBER
	/* Add more connection ID here... */
} connection_id_t;

typedef enum g3_result_enum
{
	G3_SUCCESS = (uint8_t)0x00,        /**< @brief Operation completed successfully  */
	G3_WRONG_CRC,                      /**< @brief Wrong CRC */
	G3_WRONG_MSG_LEN,                  /**< @brief Length field in the message header has a wrong value */
	G3_WRONG_CMD_ID,                   /**< @brief Command ID not supported */
	G3_WRONG_MODE,                     /**< @brief Mode not supported */
	G3_NOT_EXPECTED,                   /**< @brief The received command is not expected */
	G3_WRONG_STATE,                    /**< @brief The received command is not correct in the current state */
	G3_WRONG_TRX_STATE,                /**< @brief Wrong transceiver state */
	G3_NOT_ACCEPTABLE,                 /**< @brief The requested operation cannot be performed in the current operation mode */
	G3_WRONG_TX_POWER,                 /**< @brief Wrong TX Power value */
	G3_WRONG_MOD_SCHEME,               /**< @brief Wrong Modulation Scheme value */
	G3_WRONG_MOD_TYPE,                 /**< @brief Wrong Modulation Type value */
	G3_WRONG_PREEMPHASIS,              /**< @brief Wrong Pre-emphasis value */
	G3_WRONG_TONEMAP,                  /**< @brief Wrong Tonemap value */
	G3_WRONG_TONEMASK,                 /**< @brief Wrong Tonemask value */
	G3_WRONG_FCH,                      /**< @brief Wrong FCH */
	G3_WRONG_VALUE_LEN,                /**< @brief The length of value for the specified attribute is not in the supported range */
	G3_WRONG_TEST_MODE,                /**< @brief The test mode given is not supported */
	G3_QUEUE_FULL,                     /**< @brief Error on queue management */
	G3_DATA_LIMIT_REACHED,             /**< @brief The data passed as input to RTE exceeds the supported limit */
	G3_RESERVERD_CMD_ID,               /**< @brief The Command ID given is reserved */
	G3_RTE_INTERFACE_TIMEOUT,          /**< @brief Operation aborted due to a RTE timeout */
	G3_TEST_FAILED,                    /**< @brief Activation or deactivation of the test is not possible */
	G3_TEST_NOT_ACTIVE,                /**< @brief Deactivation not possible because a test is not active */
	G3_TEST_ALREADY_ACTIVE,            /**< @brief Activation not possible because a test is already active */
	G3_TEST_NOT_IMPLEMENTED,           /**< @brief Activation not possible because the test is not implemented yet */
	G3_TEST_NOT_IDLE,                  /**< @brief Activation not possible because the system is not idle */
	G3_TEST_NOT_ACCEPTABLE,            /**< @brief Received command cannot be managed when test is active */
	G3_FAILED,                         /**< @brief The requested operation is failed */
	G3_TX_BUSY,                        /**< @brief The transmitted is busy */
	G3_RX_BUSY,                        /**< @brief The receiver is busy */
	G3_CS_IDLE,                        /**< @brief Carrier sense is idle */
	G3_CS_BUSY,                        /**< @brief Carrier sense is busy */
	G3_PREVIOUS_REQUEST_FAILED,        /**< @brief The current operation cannot be completed as the previous request failed */
	G3_NO_INDICATION,                  /**< @brief An expected Indication is missing */
	G3_PLME_CS_CONF_TMR_EXP,           /**< @brief Transmission failed because the RTE did not receive the PLME_CS.Confirm() */
	G3_PLME_SET_TRX1_KO,               /**< @brief Transmission failed because the RTE received the first PLME_SET_TRX_STATE.Confirm() with un unexpected result */
	G3_PLME_SET_TRX1_TMR_EXP,          /**< @brief Transmission failed because the RTE did not receive the first PLME_SET_TRX_STATE.Confirm() */
	G3_PLME_SET_TRX2_KO,               /**< @brief Transmission failed because the RTE received the second PLME_SET_TRX_STATE.Confirm() with un unexpected result */
	G3_PLME_SET_TRX2_TMR_EXP,          /**< @brief Transmission failed because the RTE did not receive the second PLME_SET_TRX_STATE.Confirm()*/
	G3_PLME_SET_TRX3_KO,               /**< @brief Transmission failed because the RTE received the third PLME_SET_TRX_STATE.Confirm() with un unexpected result */
	G3_PLME_SET_TRX3_TMR_EXP,          /**< @brief Transmission failed because the RTE did not receive the third PLME_SET_TRX_STATE.Confirm() */
	G3_PLME_SET_KO,                    /**< @brief Transmission failed because the RTE received the PLME_SET.Confirm() with un unexpected result */
	G3_PLME_SET_TMR_EXP,               /**< @brief Transmission failed because the RTE did not receive the PLME_SET.Confirm() */
	G3_PD_DATA_CONF_FAILED,            /**< @brief Transmission failed because the RTE received the PLME_SET.Confirm() with un unexpected result */
	G3_PD_DATA_CONF_TMR_EXP,           /**< @brief Transmission failed because the RTE did not receive the PD_DATA.Confirm() */
	G3_SEGMENT_DISCARDED,              /**< @brief The RTE discarded the last segment due to some errors on the transmission of the previous one. */
	G3_WRONG_VALUE,                    /**< @brief The value for the specified attribute is not in the supported range */
	G3_WRONG_ATTR_NUM,                 /**< @brief The number of attributes given is out of the supported range */
	G3_WRONG_DT,                       /**< @brief Wrong DT type */
	G3_ACTION_NOT_IMPLEMENTED,         /**< @brief The required recovery action is not implemented */
	G3_ACTION_UNKNOWN,                 /**< @brief The required recovery action is unknown */
	G3_DT_ACKREQ_MISMATCH,             /**< @brief The indicated value for filed DT does not match the bit AckReq in PSD */
	G3_PLME_GET_CONF_TMR_EXP,          /**< @brief Reception failed on RTE*/
	G3_PD_ACK_CONF_FAILED,             /**< @brief Reception failed on RTE */
	G3_PD_ACK_REQ_TMR_EXP,             /**< @brief Reception failed on RTE */
	G3_RX_NACK,                        /**< @brief Received NACK after macMaxFrameRetries attempt of transmission */
	G3_ERR_SEG_COUNT,                  /**< @brief Error in SC field of MAC header */
	G3_ERR_SEG_ID,                     /**< @brief Error in SC field of MAC header */
	G3_ERR_SEG_CC,                     /**< @brief Error in SC field of MAC header */
	G3_ERR_SEG_AR,                     /**< @brief Error in SC field of MAC header */
	G3_TX_QUEUE_TO_SHORT,              /**< @brief The RTE didn't accept the segment because the data queue is to short to manage the transmission of the frame */
	G3_NO_EARLIER_SEG,                 /**< @brief The RTE didn't accept the segment because it didn't receive the previous one */
	G3_ERR_SEQ_NUM,                    /**< @brief Error in SN field of MAC header */
	G3_INTERNAL_ERROR,                 /**< @brief RTE internal error */
	G3_SEGMENT_TOO_LATE,               /**< @brief The RTE didn't accept the segment because it is hasn't been received in time for the TX */
	G3_WRONG_TWORSBLOCKS,              /**< @brief Wrong TwoRSBlocks value */
	G3_NOT_IDLE,                       /**< @brief Required operation cannot be performed because RTE is not IDLE */
	G3_WRONG_BAND,                     /**< @brief Wrong band */
	G3_NOT_SUPPORTED_BAND,             /**< @brief Not supported band */
	G3_NOT_ACCEPTABLE_LEN,             /**< @brief The length of the frame is not acceptable */
	G3_PLC_DISABLED,                   /**< @brief The RTE didn't accept the frame to because PLC is disabled */
	G3_PHY_NOT_READY,                  /**< @brief The requested action can�t be accepted by the PHY */
	G3_WRONG_LEN,                      /**< @brief Wrong length */
	G3_BANDSETOK_ATTRSETKO,            /**< @brief Bandplan set ended with success, but the default PHY setting hasn't been restored */

	G3_BUSY = (uint8_t)0x60,           /**< @brief The requested operation was not successfully completed because the stack is in busy state */
	G3_INVALID_IPv6_FRAME,             /**< @brief A wrong IPv6 frame has been provided  */
	G3_INVALID_QOS,                    /**< @brief Invalid QoS */
	G3_BAD_CCM_OUTPUT,                 /**< @brief The security processing has been attempted on an outgoing frame and it failed because the security engine produced erroneous output */
	G3_ROUTE_ERROR,                    /**< @brief The requested operation has failed due to a routing failure on the sending device  */
	G3_BT_TABLE_FULL,                  /**< @brief An attempt to send a broadcast frame or member mode multicast has failed due to the fact that there is no room in the BTT  */
	G3_FRAME_NOT_BUFFERED,             /**< @brief An frame transmission has failed due to insufficient buffering available  */
	G3_STARTUP_FAILURE,                /**< @brief An ADPM-NETWORK-START.request has failed to start a network  */
	G3_NOT_PERMITTED,                  /**< @brief An ADPM-NETWORK-JOIN.request has been disallowed  */
	G3_UNKNOWN_DEVICE,                 /**< @brief The device addressed in the parameter list is not accepted */
	G3_SENDING_DATA_ERROR,             /**< @brief The requested operation has failed because the lower layers were not able to successfully send the frame  */
	G3_PARAMETER_ERROR,                /**< @brief Error while setting or retrieving a parameter  */
	G3_WRONG_DEVICE_TYPE,              /**< @brief The requested operation has failed because of a wrong adpDeviceType for the specified operation */
	G3_ADD_EVENT_ERROR,                /**< @brief An attempt to add an event to internal FSM failed */
	G3_GET_EVENT_ERROR,                /**< @brief An attempt to get an event from internal FSM failed  */
	G3_MALLOC_FAIL,                    /**< @brief Memory buffer management error  */
	G3_RTEIPC_ERROR,                   /**< @brief Error on nRT - RTE interface */
	G3_WRONG_MEDIA_TYPE,               /**< @brief Wrong Media Type value */
	G3_UNAVAILABLE_MEDIA_TYPE,         /**< @brief The media for the transmission are blacklisted */
	G3_DUTY_CYCLE_ERR,                 /**< @brief The data frame cannot be transmitted over the RF interface due to Duty Cycle limitation */
	G3_DISABLED_MEDIA_TYPE,            /**< @brief The media type selected for the transmission is disabled */
	G3_RF_DISABLED,                    /**< @brief The frame cannot be accepted as the RF PHY layer is disabled */
	G3_JOINING_DECLINE,                /**< @brief Decline response received by Coordinator during bootstrap */
	G3_JOINING_TIMEOUT,                /**< @brief No response received by Coordinator during bootstrap */

	G3_COUNTER_ERROR = (uint8_t)0xdb, /**< @brief The frame counter of the received frame is invalid  */
	G3_IMPROPER_KEY_TYPE,             /**< @brief The key applied on the received frame is not allowed to be used with that frame type  */
	G3_IMPROPER_SECURITY_LEVEL,       /**< @brief The security level does not meet the minimum required security level  */
	G3_UNSUPPORTED_LEGACY,            /**< @brief The received frame was purportedly secured using security based on IEEE Std 802154-2003, and such security is not supported by this standard  */
	G3_UNSUPPORTED_SECURITY,          /**< @brief The security purportedly applied by the originator of the received frame is not supported  */
	G3_BEACON_LOSS,                   /**< @brief The beacon was lost following a synchronization request  */
	G3_CHANNEL_ACCESS_FAILURE,        /**< @brief A transmission could not take place due to activity on the channel  */
	G3_DENIED,                        /**< @brief The GTS request has been denied by the PAN coordinator  */
	G3_DISABLE_TRX_FAILURE,           /**< @brief The attempt to disable the transceiver has failed  */
	G3_SECURITY_ERROR,                /**< @brief Cryptographic processing of the received secured frame failed  */
	G3_FRAME_TOO_LONG,                /**< @brief Either a frame resulting from processing has a length that is greater than aMaxPHYPacketSize or a requested transaction is too large to fit in the CAP or GTS  */
	G3_INVALID_GTS,                   /**< @brief The requested GTS transmission failed because the specified GTS either did not have a transmit GTS direction or was not defined  */
	G3_INVALID_HANDLE,                /**< @brief A request to purge an MSDU from the transaction queue was made using an MSDU handle that was not found in the transaction table  */
	G3_INVALID_PARAMETER,             /**< @brief A parameter in the primitive is either not supported or is out of the valid range  */
	G3_NO_ACK,                        /**< @brief No acknowledgment was received after macMaxFrameRetries  */
	G3_NO_BEACON,                     /**< @brief A scan operation failed to find any network beacons  */
	G3_NO_DATA,                       /**< @brief No response data were available following a request  */
	G3_NO_SHORT_ADDRESS,              /**< @brief The operation failed because a 16-bit short address was not allocated  */
	G3_OUT_OF_CAP,                    /**< @brief A receiver enable request was unsuccessful because it could not be  completed within the CAP  */
	G3_PAN_ID_CONFLICT,               /**< @brief A PAN identifier conflict has been detected and communicated to the PAN coordinator  */
	G3_REALIGNMENT,                   /**< @brief A coordinator realignment command has been received  */
	G3_TRANSACTION_EXPIRED,           /**< @brief The transaction has expired and its information was discarded  */
	G3_TRANSACTION_OVERFLOW,          /**< @brief There is no capacity to store the transaction  */
	G3_TX_ACTIVE,                     /**< @brief The transceiver was in the transmitter enabled state when the receiver was requested to be enabled  */
	G3_UNAVAILABLE_KEY,               /**< @brief The key purportedly used by the originator of the received frame is not available or, if available, the originating device is not known or is blacklisted with that particular key  */
	G3_UNSUPPORTED_ATTRIBUTE,         /**< @brief A SET/GET request was issued with the identifier of a PIB attribute that is not supported  */
	G3_INVALID_ADDRESS,               /**< @brief A request to send data was unsuccessful because neither the source address parameters nor the destination address parameters were present  */
	G3_ON_TIME_TOO_LONG,              /**< @brief A receiver enable request was unsuccessful because it specified a number of symbols that was longer than the beacon interval  */
	G3_PAST_TIME,                     /**< @brief A receiver enable request was unsuccessful because it could not be completed within the current superframe and was not permitted to be deferred until the next superframe  */
	G3_TRACKING_OFF,                  /**< @brief The device was instructed to start sending beacons based on the timing of the beacon transmissions of its coordinator, but the device is not currently tracking the beacon of its coordinator  */
	G3_INVALID_INDEX,                 /**< @brief An attempt to write to a PIB attribute that is in a table failed because the specified table index was out of range  */
	G3_LIMIT_REACHED,                 /**< @brief A scan operation terminated prematurely because the number of PAN descriptors stored reached an implementation specified maximum  */
	G3_READ_ONLY,                     /**< @brief A SET/GET request was issued with the identifier of an attribute that is read only  */
	G3_SCAN_IN_PROGRESS,              /**< @brief A request to perform a scan operation failed because the MLME was in the process of performing a previously initiated scan operation  */
	G3_SUPERFRAME_OVERLAP,            /**< @brief The device was instructed to start sending beacons based on the timing of the beacon transmissions of its coordinator, but the instructed start time overlapped the transmission time of the beacon of its coordinator  */
} g3_result_t;

/* HIF command ID list for G3 */
typedef enum hif_cmd_id_enum
{
	/* Host Interface */
	HIF_HI_HWRESET_REQ        = 0x00,
	HIF_HI_HWRESET_CNF        = 0x01,
	HIF_HI_MODE_SET_REQ       = 0x02,	/* deprecated, do not use */
	HIF_HI_MODE_SET_CNF       = 0x03,	/* deprecated, do not use */
	HIF_HI_MODE_GET_REQ       = 0x04,	/* deprecated, do not use */
	HIF_HI_MODE_GET_CNF       = 0x05,	/* deprecated, do not use */
	HIF_HI_BAUDRATE_SET_REQ   = 0x06,	/* deprecated, do not use */
	HIF_HI_BAUDRATE_SET_CNF   = 0x07,	/* deprecated, do not use */
	HIF_HI_RESETSTATE_REQ     = 0x08,
	HIF_HI_RESETSTATE_CNF     = 0x09,
	HIF_HI_TEST_SET_REQ       = 0x0A,
	HIF_HI_TEST_SET_CNF       = 0x0B,
	HIF_HI_TEST_GET_REQ       = 0x0C,
	HIF_HI_TEST_GET_CNF       = 0x0D,
	HIF_HI_SFLASH_REQ         = 0x0E,
	HIF_HI_SFLASH_CNF         = 0x0F,
	HIF_HI_NVM_REQ            = 0x10,
	HIF_HI_NVM_CNF            = 0x11,
	HIF_HI_TRACE_REQ          = 0x12,
	HIF_HI_TRACE_CNF          = 0x13,
	HIF_HI_DBGTOOL_REQ        = 0x14,
	HIF_HI_DBGTOOL_CNF        = 0x15,
	HIF_HI_SEC_INIT_REQ       = 0x16,
	HIF_HI_SEC_INIT_CNF       = 0x17,
	HIF_HI_RFCONFIGSET_REQ    = 0x18,
	HIF_HI_RFCONFIGSET_CNF    = 0x19,
	HIF_HI_RFCONFIGGET_REQ    = 0x1A,
	HIF_HI_RFCONFIGGET_CNF    = 0x1B,
	HIF_HI_HWCONFIG_REQ    	  = 0x1C,
	HIF_HI_OTP_REQ            = 0x1D,
	HIF_HI_OTP_CNF            = 0x1E,
	HIF_HI_HWCONFIG_CNF       = 0x1F,

	/* G3 Library */
	HIF_G3LIB_GET_REQ         = 0x20,
	HIF_G3LIB_GET_CNF         = 0x21,
	HIF_G3LIB_SET_REQ         = 0x22,
	HIF_G3LIB_SET_CNF         = 0x23,
	HIF_G3LIB_SWRESET_REQ     = 0x24,
	HIF_G3LIB_SWRESET_CNF     = 0x25,
	HIF_G3LIB_TESTMODE_REQ    = 0x26,	/* deprecated, do not use */
	HIF_G3LIB_TESTMODE_CNF    = 0x27,	/* deprecated, do not use */
	HIF_G3LIB_EVENT_IND       = 0x28,
	HIF_G3LIB_MULTIPLEGET_REQ = 0x29,
	HIF_G3LIB_MULTIPLEGET_CNF = 0x2A,

	/* G3 Library */

	HIF_PD_DATA_REQ           = 0x40,
	HIF_PD_DATA_CNF           = 0x41,
	HIF_PD_DATA_IND           = 0x42,
	HIF_PD_ACK_REQ            = 0x43,
	HIF_PD_ACK_CNF            = 0x44,
	HIF_PD_ACK_IND            = 0x45,
	HIF_PHY_RF_REQ            = 0x46,
	HIF_PHY_RF_CNF            = 0x47,
	HIF_PHY_RF_IND            = 0x48,
	HIF_PLME_GET_CNF          = 0x49, /* not implemented */
	HIF_PLME_SET_TRX_STATE_REQ= 0x4A,
	HIF_PLME_SET_TRX_STATE_CNF= 0x4B,
	HIF_PLME_CS_REQ           = 0x4C,
	HIF_PLME_CS_CNF           = 0x4D,

	HIF_MCPS_DATA_REQ         = 0x60,
	HIF_MCPS_DATA_CNF         = 0x61,
	HIF_MCPS_DATA_IND         = 0x62,
	HIF_MLME_BEACON_NOTIFY_IND= 0x63,
	HIF_MLME_GET_REQ          = 0x64, /* not implemented */
	HIF_MLME_GET_CNF          = 0x65, /* not implemented */
	HIF_MLME_SET_REQ          = 0x66, /* not implemented */
	HIF_MLME_SET_CNF          = 0x67, /* not implemented */
	HIF_MLME_RESET_REQ        = 0x68,
	HIF_MLME_RESET_CNF        = 0x69,
	HIF_MLME_SCAN_REQ         = 0x6A,
	HIF_MLME_SCAN_CNF         = 0x6B,
	HIF_MLME_COMM_STATUS_IND  = 0x6C,
	HIF_MLME_START_REQ        = 0x6D,
	HIF_MLME_START_CNF        = 0x6E,

	HIF_ADPD_DATA_REQ         = 0x80,
	HIF_ADPD_DATA_CNF         = 0x81,
	HIF_ADPD_DATA_IND         = 0x82,
	HIF_ADPM_DISCOVERY_REQ    = 0x83,
	HIF_ADPM_DISCOVERY_CNF    = 0x84,
	HIF_ADPM_NTWSTART_REQ     = 0x85,
	HIF_ADPM_NTWSTART_CNF     = 0x86,
	HIF_ADPM_NTWJOIN_REQ      = 0x87,
	HIF_ADPM_NTWJOIN_CNF      = 0x88,
	HIF_ADPM_NTWLEAVE_REQ     = 0x89,
	HIF_ADPM_NTWLEAVE_CNF     = 0x8A,
	HIF_ADPM_NTWLEAVE_IND     = 0x8B,
	HIF_ADPM_RESET_REQ        = 0x8C,
	HIF_ADPM_RESET_CNF        = 0x8D,
	HIF_ADPM_GET_REQ          = 0x8E, /* not implemented */
	HIF_ADPM_GET_CNF          = 0x8F, /* not implemented */
	HIF_ADPM_SET_REQ          = 0x90, /* not implemented */
	HIF_ADPM_SET_CNF          = 0x91, /* not implemented */
	HIF_ADPM_NETSTATUS_IND    = 0x92,
	HIF_ADPM_ROUTEDISCO_REQ   = 0x93,
	HIF_ADPM_ROUTEDISCO_CNF   = 0x94,
	HIF_ADPM_PATHDISCO_REQ    = 0x95,
	HIF_ADPM_PATHDISCO_CNF    = 0x96,
	HIF_ADPM_LBP_REQ          = 0x97,
	HIF_ADPM_LBP_CNF          = 0x98,
	HIF_ADPM_LBP_IND          = 0x99,
	HIF_ADPD_ROUTEOVER_REQ    = 0x9A,
	HIF_ADPD_ROUTEDEL_REQ     = 0x9B,
	HIF_ADPD_ROUTEDEL_CNF     = 0x9C,

	HIF_BOOT_SRV_START_REQ 	  = 0xB0,
	HIF_BOOT_SRV_START_CNF    = 0xB1,
	HIF_BOOT_SRV_STOP_REQ     = 0xB2,
	HIF_BOOT_SRV_STOP_CNF     = 0xB3,
	HIF_BOOT_SRV_LEAVE_IND    = 0xB4,
	HIF_BOOT_SRV_KICK_REQ     = 0xB5,
	HIF_BOOT_SRV_KICK_CNF     = 0xB6,
	HIF_BOOT_SRV_JOIN_IND     = 0xB7,
	HIF_BOOT_DEV_START_REQ	  = 0xB8,
	HIF_BOOT_DEV_START_CNF    = 0xB9,
	HIF_BOOT_DEV_LEAVE_REQ    = 0xBA,
	HIF_BOOT_DEV_LEAVE_CNF    = 0xBB,
	HIF_BOOT_DEV_LEAVE_IND    = 0xBC,

	HIF_BOOT_SRV_REKEYING_REQ = 0xBD, /* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
	HIF_BOOT_SRV_REKEYING_CNF = 0xBE, /* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
	HIF_BOOT_SRV_ABORT_RK_REQ = 0xBF, /* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
	HIF_BOOT_SRV_ABORT_RK_CNF = 0xC0, /* CUSTOM ID, can only be used with the Boot Server embedded in the host application */

	HIF_BOOT_DEV_PANSORT_IND  = 0xC1,
	HIF_BOOT_DEV_PANSORT_REQ  = 0xC2,
	HIF_BOOT_DEV_PANSORT_CNF  = 0xC3,
	HIF_BOOT_SRV_GETPSK_IND   = 0xC4,
	HIF_BOOT_SRV_SETPSK_REQ   = 0xC5,
	HIF_BOOT_SRV_SETPSK_CNF	  = 0xC6,

	HIF_UDP_DATA_REQ          = 0xD0,
	HIF_UDP_DATA_CNF          = 0xD1,
	HIF_UDP_DATA_IND          = 0xD2,
	HIF_UDP_CONN_SET_REQ      = 0xD3,
	HIF_UDP_CONN_SET_CNF      = 0xD4,
	HIF_UDP_CONN_GET_REQ      = 0xD5,
	HIF_UDP_CONN_GET_CNF      = 0xD6,
	HIF_ICMP_ECHO_REQ         = 0xD7,
	HIF_ICMP_ECHO_CNF         = 0xD8,
	HIF_ICMP_ECHO_REP_IND     = 0xD9,
	HIF_ICMP_ECHO_REQ_IND     = 0xDF,
	HIF_ERROR_IND             = 0xFF,
} hif_cmd_id_t;

/* HOSTIF DBGTOOL Control field */
typedef enum hif_dbgtool_ctrl_enum
{
	HI_TOOL_INFO		= 0x00,
	HI_TOOL_UART_NUM	= 0x01,
	HI_TOOL_LED_NUM     = 0x02,
	HI_TOOL_RF_NUM      = 0x03,
	HI_TOOL_PLC_MODE    = 0x10,
	HI_TOOL_PLC_STATE   = 0x11,
	HI_TOOL_RF_CONFIG   = 0x12,
	HI_TOOL_BAUDRATE    = 0x20,
	HI_TOOL_RESET_STATE = 0x21,
	HI_TOOL_PLC_PERF	= 0x30,
	HI_TOOL_SFL_ERASE   = 0x40,
	HI_TOOL_SFL_READ    = 0x41,
	HI_TOOL_SFL_WRITE   = 0x42,
	HI_TOOL_NVM_RD      = 0x43,
	HI_TOOL_NVM_WR      = 0x44,
	HI_TOOL_NVM_SV      = 0x45,
	HI_TOOL_NVM_ER      = 0x46,
	HI_TRACE_DISABLE    = 0x50,
	HI_TRACE_ENABLE     = 0x51,
	HI_TRACE_START      = 0x52,
	HI_TRACE_STOP       = 0x53,
	HI_TRACE_DUMP       = 0x54,
	HI_TRACE_FILTER     = 0x55,
	HI_TRACE_TRIGGER    = 0x56,
	HI_TOOL_SECINIT     = 0x60,
	HI_TOOL_SECCFG      = 0x61,
	HI_TOOL_OTP_RD      = 0x70,
	HI_TOOL_OTP_WR      = 0x71,
	HI_TOOL_OTP_LOCK    = 0x72,
	HI_TOOL_RF_GET_CONF = 0x80,
	HI_TOOL_RF_SET_CONF = 0x81,
} hif_dbgtool_ctrl_t;

/* HOSTIF DBGTOOL Info field */
typedef enum hif_dbgtool_info_info_enum
{
	HI_TOOL_NO_CONF = 0,
} hif_dbgtool_info_info_t;

typedef enum hif_dbgtool_info_conf_gpio_enum
{
	HI_TOOL_UART_CONF_GPIO0	= 0x00,
	HI_TOOL_UART_CONF_GPIO1	= 0x01,
} hif_dbgtool_info_conf_gpio_t;

typedef enum hif_dbgtool_info_conf_led_enum
{
	HI_TOOL_LED_CONF_GPIO1 = 0x00,
	HI_TOOL_LED_CONF_GPIO3 = 0x01,
} hif_dbgtool_info_conf_led_t;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* HIF_G3_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
