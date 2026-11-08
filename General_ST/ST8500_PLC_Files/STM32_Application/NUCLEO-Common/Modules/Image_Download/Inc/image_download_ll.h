/**
  ******************************************************************************
  * @file    image_download_ll.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define ST8500 image download macros/definitions for
  *          low level functions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef IMAGE_DOWNLOAD_LL_H_
#define IMAGE_DOWNLOAD_LL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INCUDE_USER_DOWNLOAD_LL
#error "This header file cannot be included without INCUDE_USER_DOWNLOAD_LL defined"
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>

/* Definitions */

/* MIB ID list */
#define UD_MIB_ID_RTE_BOOT_STATUS       0x00
#define UD_MIB_ID_PE_BOOT_STATUS        0x01
#define UD_MIB_ID_SECURITY_BITS         0x02
#define UD_MIB_ID_BOOT_MODE             0x03
#define UD_MIB_ID_BOOT_VERSION          0x04
#define UD_MIB_ID_EUI64                 0x07

/* RTE/PE status bits description */
#define UD_BOOT_STS_INITIATED_BIT               0x0001
#define UD_BOOT_STS_COMPLETE_BIT                0x0002
#define UD_BOOT_STS_SUCCESS_BIT                 0x0004
#define UD_BOOT_STS_RTE_LOAD_ERR_BIT            0x0008
#define UD_BOOT_STS_PE_LOAD_ERR_BIT             0x0010
#define UD_BOOT_STS_RTE_IMG_INIT_ERR_BIT        0x0020
#define UD_BOOT_STS_PE_IMG_INIT_ERR_BIT         0x0040
#define UD_BOOT_STS_RTE_IMG_WRITE_ERR_BIT       0x0080
#define UD_BOOT_STS_PE_IMG_ERR_BIT              0x0100
#define UD_BOOT_STS_NO_RTE_IMG_ERR_BIT          0x0200
#define UD_BOOT_STS_NO_PE_IMG_ERR_BIT           0x0400
#define UD_BOOT_STS_OTP_READ_ERR_BIT            0x0800
#define UD_BOOT_STS_FW_SIZE_ERR_BIT             0x1000
#define UD_BOOT_STS_START_ADDR_ERR_BIT          0x2000
#define UD_BOOT_STS_START_TOUT_ERR_BIT          0x4000
#define UD_BOOT_STS_INVALID_IMG_ERR_BIT         0x8000

#define UD_BOOT_STSDOWNLOAD_SUCCESSFUL 			(UD_BOOT_STS_INITIATED_BIT | UD_BOOT_STS_COMPLETE_BIT | UD_BOOT_STS_SUCCESS_BIT)

/* Functions */
void     sendRequest(    UART_HandleTypeDef* huart, ud_req_t* req);
uint32_t receiveResponse(UART_HandleTypeDef* huart, ud_res_t* res, uint32_t timeout);

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

#endif /* IMAGE_DOWNLOAD_LL_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
