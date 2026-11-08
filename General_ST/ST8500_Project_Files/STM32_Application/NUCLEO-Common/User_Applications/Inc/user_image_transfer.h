/**
  ******************************************************************************
  * @file    user_image_transfer.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the User Image Transfer implementation.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/


#ifndef USER_IMG_TRANSFER_H
#define USER_IMG_TRANSFER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <settings.h>
#include <hi_msgs_impl.h>

#if ENABLE_IMAGE_TRANSFER

/* Custom types */
typedef enum user_image_transfer_error_code_enum
{
	uit_no_error,
	uit_info_timeout_error,
	uit_data_timeout_error,
	uit_crc_error,
	uit_aborted,
} user_image_transfer_error_code_t;

/* Public Functions */
void								UserImgTransfer_Init(void);
bool								UserImgTransfer_MsgNeeded(const g3_msg_t *msg);
void								UserImgTransfer_MsgHandler(const g3_msg_t *msg);
void								UserImgTransfer_FsmManager(void);

#if IS_COORD
void 								UserImgTransfer_StartSend(uint8_t image_slot, uint16_t dest_short_addr);
#else
void 								UserImgTransfer_StartReceive(uint8_t image_slot);
#endif

void								UserImgTransfer_Stop(void);

bool 								UserImgTransfer_IsComplete(void);
user_image_transfer_error_code_t 	UserImgTransfer_GetError(void);
void 								UserImgTransfer_TimeoutCallback(void *argument);

#endif /* ENABLE_IMAGE_TRANSFER */

#ifdef __cplusplus
}
#endif

#endif /* USER_IMG_TRANSFER_H */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
