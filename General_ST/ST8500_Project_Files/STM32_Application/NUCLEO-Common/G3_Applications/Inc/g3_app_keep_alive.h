/*****************************************************************************
*   @file    g3_app_keep_alive.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Keep-Alive application.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef G3_APP_KEEP_ALIVE_H_
#define G3_APP_KEEP_ALIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <settings.h>
#include <g3_comm.h>

/** @addtogroup G3_App
  * @{
  */

/** @defgroup G3_App_KeepAlive G3 Keep-Alive Application
  * @{
  */

/** @defgroup G3_App_KeepAlive_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_KeepAlive_Exported_Code Exported Code
  * @{
  */

/* Definitions */

#if ENABLE_ICMP_KEEP_ALIVE

/* Public functions */
bool	 g3_app_ka_msg_needed( const g3_msg_t *g3_msg);
void	 g3_app_ka_msg_handler(const g3_msg_t *g3_msg);
void	 g3_app_ka_init(void);
bool 	 g3_app_ka_start(void);
bool 	 g3_app_ka_stop(void);
#if IS_COORD
void	 g3_app_ka_wait(void);
bool 	 g3_app_ka_in_progress(void);
void 	 g3_app_ka(void);
#else
uint32_t g3_app_ka_time_to_leave(void);
#endif
void 	 g3_app_ka_timer_callback(void *argument);

#endif /* ENABLE_ICMP_KEEP_ALIVE */

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

#endif /* G3_APP_KEEP_ALIVE_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
