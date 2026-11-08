/*****************************************************************************
*   @file    g3_app_last_gasp.h
*   @author  AMG/IPC Application Team
*   @brief   Header file for the Last Gasp application.
*
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/

#ifndef G3_APP_LAST_GASP_H_
#define G3_APP_LAST_GASP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <settings.h>
#include <g3_comm.h>

/** @addtogroup G3_App
  * @{
  */

/** @defgroup G3_App_LastGasp G3 Last Gasp Application
  * @{
  */

/** @defgroup G3_App_LastGasp_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/** @defgroup G3_App_LastGasp_Exported_Code Exported Code
  * @{
  */

/* Definitions */

#if ENABLE_LAST_GASP

/* Public functions */
bool	 g3_app_last_gasp_msg_needed( const g3_msg_t *g3_msg);
void	 g3_app_last_gasp_msg_handler(const g3_msg_t *g3_msg);
void	 g3_app_last_gasp_init(void);

#if !IS_COORD
void 	 g3_app_last_gasp_start(void);
void 	 g3_app_last_gasp_activate(void);
uint8_t	 g3_app_last_gasp_is_active(void);
void 	 g3_app_last_gasp(void);
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* G3_APP_KEEP_ALIVE_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
