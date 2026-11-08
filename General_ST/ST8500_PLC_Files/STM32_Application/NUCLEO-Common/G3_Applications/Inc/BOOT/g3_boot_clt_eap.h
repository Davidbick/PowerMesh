/*****************************************************************************
*   @file    g3_boot_clt_eap.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the bootstrap client EAP-PSK messages handling.
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
#ifndef G3_APP_BOOT_CLT_EAP_H_
#define G3_APP_BOOT_CLT_EAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions*/
#include <stdint.h>
#include <settings.h>
#include <hi_adp_lbp.h>

/** @addtogroup G3_ADP
  * @{
  */

/** @addtogroup G3_ADP_LBP
  * @{
  */

/** @defgroup G3_ADP_LBP_EAP EAP layer
  * @{
  */

/** @defgroup G3_ADP_LBP_EAP_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_ADP_LBP_EAP_Exported_Code Exported Code
  * @{
  */

#if !IS_COORD && ENABLE_BOOT_CLIENT_ON_HOST
void g3_boot_clt_eap_fsm_manager(const lbp_ind_t* lbp_eap_msg);
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* G3_APP_BOOT_CLT_EAP_H_ */

/******************* (C) COPYRIGHT 2021 STMicroelectronics *****END OF FILE****/
