/**
  ******************************************************************************
  * @file    crc.h
  * @author  AMG/IPC Application Team
  * @brief   Header for CRC16 (XMODEM or CCITT) functionalities.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef CRC_H_
#define CRC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>

/** @defgroup CRC16_Utility Calculation of the CRC16 (CRC16-CCITT or CRC16-XMODEM)
  * @{
  */

/* Definitions */
#define CRC16_CCITT_START_VALUE 		0xFFFF	/* Standard initial value for CRC16-CCITT */
#define CRC16_XMODEM_START_VALUE		0x0000  /* Standard initial value for CRC16-XMODEM */

/* CRC16 type */
typedef uint16_t crc16_t;

/* Macros*/
#define CRC16_CCITT( buf, len)		crc16_generic(buf, len, CRC16_CCITT_START_VALUE)	/* Calculates the CRC16-CCITT */
#define CRC16_XMODEM(buf, len)		crc16_generic(buf, len, CRC16_XMODEM_START_VALUE)	/* Calculates the CRC16-XMODEM */

/* Public functions */
crc16_t crc16_generic(const void *buf, const uint32_t len, const crc16_t initial_value);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* CRC_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
