/*****************************************************************************
 *   @file    sflash_task.h
 *   @author  AMG/IPC Application Team
 *   @brief   This file contains code that implements the sflash task.
 *
 * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 *******************************************************************************/


#ifndef SFLASH_TASK_H_
#define SFLASH_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup sflash_Task
 * @{
 */

/**
 * @}
 */

/** @defgroup sflash_Task_Exported_Code Exported Code
 * @{
 */

/**
 * @}
 */

/** @defgroup sflash_Task_Private_Code Private Code
 * @{
 */

/**
 * @}
 */

/* Public Functions */
bool sflash_app_is_busy();
void sflash_app_init(void);
void sflash_app_exec(void);

#ifdef __cplusplus
}
#endif


#endif /*SFLASH_TASK_H_ */
