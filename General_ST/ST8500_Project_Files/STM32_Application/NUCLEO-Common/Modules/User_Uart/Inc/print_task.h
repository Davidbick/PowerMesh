/**
  ******************************************************************************
  * @file    print_task.h
  * @author  AMG/IPC Application Team
  * @brief   This file contains declarations for the Print task.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef PRINT_TASK_H_
#define PRINT_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Print_Task Print Task
  * @{
  */

/**
  * @}
  */

/** @defgroup Print_Task_Exported_Code Exported Code
  * @{
  */

/**
  * @}
  */

/** @defgroup Print_Task_Private_Code Private Code
  * @{
  */

/**
  * @}
  */

/* Public Functions */
bool print_app_is_busy();
void print_app_init(void);
void print_app_exec(void);

#ifdef __cplusplus
}
#endif

#endif /* PRINT_TASK_H_ */
