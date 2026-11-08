/**
  ******************************************************************************
  * @file    debug_print.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements debug log prints.
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
#include <cmsis_os.h>
#include <main.h>
#include <debug_print.h>
#include <hif_g3_common.h>
#include <user_if.h>
#include <user_terminal.h>

#if DEBUG_G3_MSG
const char label_g3_msg[]         = "G3_MSG:      ";
#endif

#if DEBUG_G3_CONF
const char label_g3_conf[]        = "G3_CONF:     ";
#endif

#if DEBUG_G3_BOOT
const char label_g3_boot[]        = "G3_BOOT:     ";
#endif

#if DEBUG_G3_BOOT_SRV
const char label_g3_boot_srv[]    = "G3_BOOT_SRV: ";
#endif

#if DEBUG_G3_BOOT_CLT
const char label_g3_boot_clt[]    = "G3_BOOT_CLT: ";
#endif

#if DEBUG_G3_PANSORT
const char label_g3_pansort[]     = "G3_PANSORT:  ";
#endif

#if DEBUG_G3_KA
const char label_g3_ka[]          = "G3_KA:       ";
#endif

#if DEBUG_G3_LAST_GASP
const char label_g3_last_gasp[]   = "G3_LAST_GASP:";
#endif

#if DEBUG_USER_G3
const char label_user_g3[]        = "USER_G3:     ";
#endif

#if DEBUG_USER_IT
const char label_user_it[]        = "USER_IT:     ";
#endif

#if DEBUG_SFLASH
const char label_sflash[]         = "SFLASH:      ";
#endif

#if (DEBUG_G3_MSG || DEBUG_G3_BOOT || DEBUG_G3_BOOT_SRV || DEBUG_G3_BOOT_CLT || DEBUG_G3_CONF || DEBUG_G3_KA || DEBUG_G3_LAST_GASP)
/**
  * @brief  Translated a command ID value to its name string.
  * @param  cmd_id: command ID value to translate.
  * @retval String of the command ID name.
  */
char* translateG3cmd(uint8_t cmd_id)
{
    switch(cmd_id)
    {
    case   HIF_HI_HWRESET_REQ:          return "HI_HWRESET_REQ";
    case   HIF_HI_HWRESET_CNF:          return "HI_HWRESET_CNF";
#if 0
    case   HIF_HI_MODE_SET_REQ:         return "HI_MODE_SET_REQ";		/* deprecated, do not use */
    case   HIF_HI_MODE_SET_CNF:         return "HI_MODE_SET_CNF";		/* deprecated, do not use */
    case   HIF_HI_MODE_GET_REQ:         return "HI_MODE_GET_REQ";		/* deprecated, do not use */
    case   HIF_HI_MODE_GET_CNF:         return "HI_MODE_GET_CNF";		/* deprecated, do not use */
    case   HIF_HI_BAUDRATE_SET_REQ:     return "HI_BAUDRATE_SET_REQ";	/* deprecated, do not use */
    case   HIF_HI_BAUDRATE_SET_CNF:     return "HI_BAUDRATE_SET_CNF";	/* deprecated, do not use */
#endif

#if 0
    case   HIF_HI_RESETSTATE_REQ:       return "HI_RESETSTATE_REQ";
    case   HIF_HI_RESETSTATE_CNF:       return "HI_RESETSTATE_CNF";
    case   HIF_HI_TEST_SET_REQ:         return "HI_TEST_SET_REQ";
    case   HIF_HI_TEST_SET_CNF:         return "HI_TEST_SET_CNF";
    case   HIF_HI_TEST_GET_REQ:         return "HI_TEST_GET_REQ";
    case   HIF_HI_TEST_GET_CNF:         return "HI_TEST_GET_CNF";
#endif
    case   HIF_HI_SFLASH_REQ:           return "HI_SFLASH_REQ";
    case   HIF_HI_SFLASH_CNF:           return "HI_SFLASH_CNF";
    case   HIF_HI_NVM_REQ:              return "HI_NVM_REQ";
    case   HIF_HI_NVM_CNF:              return "HI_NVM_CNF";
#if 1
    case   HIF_HI_TRACE_REQ:            return "HI_TRACE_REQ";
    case   HIF_HI_TRACE_CNF:            return "HI_TRACE_CNF";
#endif
    case   HIF_HI_DBGTOOL_REQ:          return "HI_DBGTOOL_REQ";
    case   HIF_HI_DBGTOOL_CNF:          return "HI_DBGTOOL_CNF";
#if 0
    case   HIF_HI_SEC_INIT_REQ:         return "HI_SEC_INIT_REQ";
    case   HIF_HI_SEC_INIT_CNF:         return "HI_SEC_INIT_CNF";
#endif
    case   HIF_HI_RFCONFIGSET_REQ:      return "HI_RFCONFIGSET_REQ";
    case   HIF_HI_RFCONFIGSET_CNF:      return "HI_RFCONFIGSET_CNF";
    case   HIF_HI_RFCONFIGGET_REQ:      return "HI_RFCONFIGGET_REQ";
    case   HIF_HI_RFCONFIGGET_CNF:      return "HI_RFCONFIGGET_CNF";

    case   HIF_HI_HWCONFIG_REQ:      	return "HI_HWCONFIG_REQ";
    case   HIF_HI_HWCONFIG_CNF:      	return "HI_HWCONFIG_CNF";
#if 0
    case   HIF_HI_OTP_REQ:              return "HI_OTP_REQ";
    case   HIF_HI_OTP_CNF:              return "HI_OTP_CNF";
    case   HIF_HI_READY_IND:            return "HI_READY_IND";
#endif
    case   HIF_G3LIB_GET_REQ:           return "G3LIB_GET_REQ";
    case   HIF_G3LIB_GET_CNF:           return "G3LIB_GET_CNF";
    case   HIF_G3LIB_SET_REQ:           return "G3LIB_SET_REQ";
    case   HIF_G3LIB_SET_CNF:           return "G3LIB_SET_CNF";
    case   HIF_G3LIB_SWRESET_REQ:       return "G3LIB_SWRESET_REQ";
    case   HIF_G3LIB_SWRESET_CNF:       return "G3LIB_SWRESET_CNF";
#if 0
    case   HIF_G3LIB_TESTMODE_REQ:      return "G3LIB_TESTMODE_REQ";	/* deprecated, do not use */
    case   HIF_G3LIB_TESTMODE_CNF:      return "G3LIB_TESTMODE_CNF";	/* deprecated, do not use */
#endif
    case   HIF_G3LIB_EVENT_IND:    return "HIF_G3LIB_EVENT_IND";
#if 0
    case   HIF_G3LIB_MULTIPLEGET_REQ:   return "G3LIB_MULTIPLEGET_REQ";
    case   HIF_G3LIB_MULTIPLEGET_CNF:   return "G3LIB_MULTIPLEGET_CNF";

    case   HIF_PD_DATA_REQ:             return "PD_DATA_REQ";
    case   HIF_PD_DATA_CNF:             return "PD_DATA_CNF";
    case   HIF_PD_DATA_IND:             return "PD_DATA_IND";
    case   HIF_PD_ACK_REQ:              return "PD_ACK_REQ";
    case   HIF_PD_ACK_CNF:              return "PD_ACK_CNF";
    case   HIF_PD_ACK_IND:              return "PD_ACK_IND";
    case   HIF_PHY_RF_REQ:              return "PHY_RF_REQ";
    case   HIF_PHY_RF_CNF:              return "PHY_RF_CNF";
    case   HIF_PHY_RF_IND:              return "PHY_RF_IND";
    case   HIF_PLME_GET_CNF:            return "PLME_GET_CNF";
    case   HIF_PLME_SET_TRX_STATE_REQ:  return "PLME_SET_TRX_STATE_REQ";
    case   HIF_PLME_SET_TRX_STATE_CNF:  return "PLME_SET_TRX_STATE_CNF";
    case   HIF_PLME_CS_REQ:             return "PLME_CS_REQ";
    case   HIF_PLME_CS_CNF:             return "PLME_CS_CNF";
#endif
    case   HIF_MCPS_DATA_REQ:           return "MCPS_DATA_REQ";
    case   HIF_MCPS_DATA_CNF:           return "MCPS_DATA_CNF";
    case   HIF_MCPS_DATA_IND:           return "MCPS_DATA_IND";
#if 0
    case   HIF_MLME_BEACON_NOTIFY_IND:  return "MLME_BEACON_NOTIFY_IND";
    case   HIF_MLME_GET_REQ:            return "MLME_GET_REQ";
    case   HIF_MLME_GET_CNF:            return "MLME_GET_CNF";
    case   HIF_MLME_SET_REQ:            return "MLME_SET_REQ";
    case   HIF_MLME_SET_CNF:            return "MLME_SET_CNF";
    case   HIF_MLME_RESET_REQ:          return "MLME_RESET_REQ";
    case   HIF_MLME_RESET_CNF:          return "MLME_RESET_CNF";
    case   HIF_MLME_SCAN_REQ:           return "MLME_SCAN_REQ";
    case   HIF_MLME_SCAN_CNF:           return "MLME_SCAN_CNF";
    case   HIF_MLME_COMM_STATUS_IND:    return "MLME_COMM_STATUS_IND";
    case   HIF_MLME_START_REQ:          return "MLME_START_REQ";
    case   HIF_MLME_START_CNF:          return "MLME_START_CNF";

    case   HIF_ADPD_DATA_REQ:           return "ADPD_DATA_REQ";
    case   HIF_ADPD_DATA_CNF:           return "ADPD_DATA_CNF";
    case   HIF_ADPD_DATA_IND:           return "ADPD_DATA_IND";
#endif
    case   HIF_ADPM_DISCOVERY_REQ:      return "ADPM_DISCOVERY_REQ";
    case   HIF_ADPM_DISCOVERY_CNF:      return "ADPM_DISCOVERY_CNF";
    case   HIF_ADPM_NTWSTART_REQ:       return "ADPM_NTWSTART_REQ";
    case   HIF_ADPM_NTWSTART_CNF:       return "ADPM_NTWSTART_CNF";
    case   HIF_ADPM_NTWJOIN_REQ:        return "ADPM_NTWJOIN_REQ";
    case   HIF_ADPM_NTWJOIN_CNF:        return "ADPM_NTWJOIN_CNF";
    case   HIF_ADPM_NTWLEAVE_REQ:       return "ADPM_NTWLEAVE_REQ";
    case   HIF_ADPM_NTWLEAVE_CNF:       return "ADPM_NTWLEAVE_CNF";
    case   HIF_ADPM_NTWLEAVE_IND:       return "ADPM_NTWLEAVE_IND";
#if 0
    case   HIF_ADPM_RESET_REQ:          return "ADPM_RESET_REQ";
    case   HIF_ADPM_RESET_CNF:          return "ADPM_RESET_CNF";
    case   HIF_ADPM_GET_REQ:            return "ADPM_GET_REQ";
    case   HIF_ADPM_GET_CNF:            return "ADPM_GET_CNF";
    case   HIF_ADPM_SET_REQ:            return "ADPM_SET_REQ";
    case   HIF_ADPM_SET_CNF:            return "ADPM_SET_CNF";
#endif
    case   HIF_ADPM_NETSTATUS_IND:      return "ADPM_NETSTATUS_IND";
    case   HIF_ADPM_ROUTEDISCO_REQ:     return "ADPM_ROUTEDISCO_REQ";
    case   HIF_ADPM_ROUTEDISCO_CNF:     return "ADPM_ROUTEDISCO_CNF";
#if 0
    case   HIF_ADPM_PATHDISCO_REQ:      return "ADPM_PATHDISCO_REQ";
    case   HIF_ADPM_PATHDISCO_CNF:      return "ADPM_PATHDISCO_CNF";
#endif
    case   HIF_ADPM_LBP_REQ:            return "ADPM_LBP_REQ";
    case   HIF_ADPM_LBP_CNF:            return "ADPM_LBP_CNF";
    case   HIF_ADPM_LBP_IND:            return "ADPM_LBP_IND";
#if 0
    case   HIF_ADPD_ROUTEOVER_REQ:      return "ADPD_ROUTEOVER_REQ";
    case   HIF_ADPD_ROUTEDEL_REQ:       return "ADPD_ROUTEDEL_REQ";
    case   HIF_ADPD_ROUTEDEL_CNF:       return "ADPD_ROUTEDEL_CNF";
#endif
    case   HIF_BOOT_SRV_START_REQ:   return "BOOT_SRV_START_REQ";
    case   HIF_BOOT_SRV_START_CNF:   return "BOOT_SRV_START_CNF";
    case   HIF_BOOT_SRV_STOP_REQ:    return "BOOT_SRV_STOP_REQ";
    case   HIF_BOOT_SRV_STOP_CNF:    return "BOOT_SRV_STOP_CNF";
    case   HIF_BOOT_SRV_LEAVE_IND:   return "BOOT_SRV_LEAVE_IND";
    case   HIF_BOOT_SRV_KICK_REQ:    return "BOOT_SRV_KICK_REQ";
    case   HIF_BOOT_SRV_KICK_CNF:    return "BOOT_SRV_KICK_CNF";
    case   HIF_BOOT_SRV_JOIN_IND:    return "BOOT_SRV_JOIN_IND";
    case   HIF_BOOT_DEV_START_REQ:   return "BOOT_DEV_START_REQ";
    case   HIF_BOOT_DEV_START_CNF:   return "BOOT_DEV_START_CNF";
    case   HIF_BOOT_DEV_LEAVE_REQ:   return "BOOT_DEV_LEAVE_REQ";
    case   HIF_BOOT_DEV_LEAVE_CNF:   return "BOOT_DEV_LEAVE_CNF";
    case   HIF_BOOT_DEV_LEAVE_IND:   return "BOOT_DEV_LEAVE_IND";
    
    case   HIF_BOOT_SRV_REKEYING_REQ: return "BOOT_SRV_REKEYING_REQ";	/* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
    case   HIF_BOOT_SRV_REKEYING_CNF: return "BOOT_SRV_REKEYING_CNF";	/* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
    case   HIF_BOOT_SRV_ABORT_RK_REQ: return "BOOT_SRV_ABORT_RK_REQ";	/* CUSTOM ID, can only be used with the Boot Server embedded in the host application */
    case   HIF_BOOT_SRV_ABORT_RK_CNF: return "BOOT_SRV_ABORT_RK_CNF";	/* CUSTOM ID, can only be used with the Boot Server embedded in the host application */

    case   HIF_BOOT_DEV_PANSORT_IND: return "BOOT_DEV_PANSORT_IND";
    case   HIF_BOOT_DEV_PANSORT_REQ: return "BOOT_DEV_PANSORT_REQ";
    case   HIF_BOOT_DEV_PANSORT_CNF: return "BOOT_DEV_PANSORT_CNF";
    case   HIF_BOOT_SRV_GETPSK_IND:  return "BOOT_SRV_GETPSK_IND";
    case   HIF_BOOT_SRV_SETPSK_REQ:  return "BOOT_SRV_SETPSK_REQ";
    case   HIF_BOOT_SRV_SETPSK_CNF:  return "BOOT_SRV_SETPSK_CNF";
    
    case   HIF_UDP_DATA_REQ:         return "UDP_DATA_REQ";
    case   HIF_UDP_DATA_CNF:         return "UDP_DATA_CNF";
    case   HIF_UDP_DATA_IND:         return "UDP_DATA_IND";
    case   HIF_UDP_CONN_SET_REQ:     return "UDP_CONN_SET_REQ";
    case   HIF_UDP_CONN_SET_CNF:     return "UDP_CONN_SET_CNF";
    case   HIF_UDP_CONN_GET_REQ:     return "UDP_CONN_GET_REQ";
    case   HIF_UDP_CONN_GET_CNF:     return "UDP_CONN_GET_CNF";
    case   HIF_ICMP_ECHO_REQ:        return "ICMP_ECHO_REQ";
    case   HIF_ICMP_ECHO_CNF:        return "ICMP_ECHO_CNF";
    case   HIF_ICMP_ECHO_REP_IND:    return "ICMP_ECHO_REP_IND";
    case   HIF_ICMP_ECHO_REQ_IND:    return "ICMP_ECHO_REQ_IND";
    case   HIF_ERROR_IND:            return "ERROR_IND";
    default: 						 return "UNKOWN/UNMAPPED";
    }
}

#endif

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
