/**
  ******************************************************************************
  * @file    version.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the STM32 firmware version and the evaluation kit name.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef VERSION_H_
#define VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

/* STM32 FW version in the vMAJOR.MINOR.SUB format (keep only a space between '#define' and "FW_...") */
#define FW_VERSION_MAJOR	2
#define FW_VERSION_MINOR	5
#define FW_VERSION_SUB		1

/* Evaluation kit name */
#define EVALKIT_NAME		"EVLKST8500GH-2"

#ifdef __cplusplus
}
#endif

#endif /* VERSION_H_ */
