/**
  ******************************************************************************
  * @file    user_mac.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define the MAC user function.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef USER_MAC_H
#define USER_MAC_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup User_MAC User MAC
  * @{
  */

/**
  * @}
  */

/** @defgroup User_MAC_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup User_MAC_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/* Include files -------------------------------------------------------------*/
#include <g3_comm.h>
#include <hi_msgs_impl.h>
#include <hi_ip_sap_interface.h>
#include <hi_mac_sap_interface.h>

/** @addtogroup User_App
  * @{
  */

/** @addtogroup User_MAC
  * @{
  */

/** @addtogroup User_MAC_Exported_Code
  * @{
  */

/* User event macros */

/* Macros */
#define LED_MAC_RX_GPIO_Port       LED2_GPIO_Port
#define LED_MAC_RX_Pin             LED2_Pin

#define LED_MAC_READY_GPIO_Port    LED3_GPIO_Port
#define LED_MAC_READY_Pin          LED3_Pin

/* Exported types */

/* Test types, bitmap enum */
typedef enum usermac_test_type_enum
{
	mac_test_plc	= 0x01,	/* Test only PLC */
	mac_test_rf		= 0x10,	/* Test only RF */
	mac_test_plc_rf = 0x11,	/* Test PLC, then RF */
} usermac_test_type_t;

typedef enum mac_test_state_enum
{
	mac_sleep_reserved       = 0x01,
	mac_ready                = 0x02,
	mac_ok                   = 0x03,
	mac_cnf_plc_error        = 0x04,
	mac_cnf_plc_timeout      = 0x05,
	mac_ind_plc_timeout      = 0x06,
	mac_ind_plc_mismatch     = 0x07,
	mac_cnf_rf_error         = 0x08,
	mac_cnf_rf_timeout       = 0x09,
	mac_ind_rf_timeout       = 0x0A,
	mac_ind_rf_mismatch      = 0x0B,
	mac_internal_error       = 0x0C,
} mac_test_state_t;


/* User G3 events */


/* Public functions */
void UserMac_Init(void);

/* Message handler and FSM */
bool UserMac_MsgNeeded( const g3_msg_t *g3_msg);
void UserMac_MsgHandler(const g3_msg_t *g3_msg);
void UserMac_FsmManager(void);

/* Utility */
bool UserMac_IsReady();

/* Callbacks */
void UserMac_StartTxCallback(uint8_t *dst_addr, usermac_test_type_t test_type, uint32_t msg_number);
void UserMac_TimeoutCallback(void *argument);

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

#endif /* USER_G3_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
