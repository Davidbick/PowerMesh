/**
  ******************************************************************************
  * @file    image_download_ll.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements ST8500 image download
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

/* Inclusions */
#include <string.h>
#include <cmsis_os.h>
#include <main.h>

/* Utility */
#include <crc.h>
#include <utils.h>

/* Modules */
#include <image_management.h>
#include <image_download.h>

/* Reserved Inclusions */
#define INCUDE_USER_DOWNLOAD_LL
#include <image_download_ll.h>
#undef INCUDE_USER_DOWNLOAD_LL

#if ENABLE_DOWNLOAD

/* Private functions */

/* Public functions */

/**
  * @brief  Sends a request message through the given UART.
  * @param  huart: UART handle.
  * @param  req: pointer to the request message buffer.
  * @retval None.
  */
void sendRequest(UART_HandleTypeDef* huart, ud_req_t* req)
{
    if (req->preamb.msg_len <= sizeof(req->payload))
    {
    	uint16_t crc16;

        req->preamb.sync  = UD_SYNCHRO_VALUE;
        req->preamb.mode  = UD_MODE_VALUE;
        req->preamb.state = 0;

        crc16 = CRC16_XMODEM(req, sizeof(req->preamb) + req->preamb.msg_len);

		req->payload[req->preamb.msg_len]     = LOW_BYTE(crc16);
		req->payload[req->preamb.msg_len + 1] = HIGH_BYTE(crc16);

        if ((osKernelGetState() == osKernelInactive) || (osKernelGetState() == osKernelReady))
		{
        	HAL_UART_Transmit(huart, (uint8_t*) req, sizeof(req->preamb) + req->preamb.msg_len + sizeof(crc16), REQUEST_TIMEOUT);
		}
		else
		{
			Error_Handler(); /* Must be called before starting FreeRTOS */
		}
    }
}

/**
  * @brief  Receives a response message through the given UART.
  * @param  huart: UART handle.
  * @param  res: pointer to the response message buffer.
  * @param  timeout: reception timeout (in ms).
  * @retval None.
  */
uint32_t receiveResponse(UART_HandleTypeDef* huart, ud_res_t* res, uint32_t timeout)
{
	bool ret = true;
	uint32_t state = UD_STATE_ERROR;

	uint16_t crc16_recv;
	uint16_t crc16_calc;

	preamb_t *preamb  = (preamb_t*) res;
	uint8_t  *payload = ((uint8_t*) res) + sizeof(*preamb);

	uint16_t minumum_len = sizeof(preamb_t) + sizeof(crc16_recv);

	if ((osKernelGetState() == osKernelInactive) || (osKernelGetState() == osKernelReady))
	{
		/* Clean previous data */
		memset(res, 0, sizeof(*res));

		/* Receive first part (message header) */
		if (HAL_UART_Receive(huart, (uint8_t*) preamb, sizeof(*preamb), timeout) == HAL_OK)
		{
			if ((preamb->sync    != (UD_SYNCHRO_VALUE          )) ||
				(preamb->msg_len  > (sizeof(*res) - minumum_len)) )
			{
				ret = false;
			}
			else
			{
				if (preamb->msg_len > 0)
				{
					/* Receive second part (message payload) */
					if (HAL_UART_Receive(huart, &payload[0], preamb->msg_len, REQUEST_TIMEOUT) != HAL_OK)
					{
						ret = false;
					}
				}

				/* Receive second part (message payload) */
				if (HAL_UART_Receive(huart, &payload[preamb->msg_len], sizeof(crc16_recv), REQUEST_TIMEOUT) != HAL_OK)
				{
					ret = false;
				}
				else
				{
					crc16_recv = ASSEMBLE_U16(payload[preamb->msg_len + 1], payload[preamb->msg_len]);
					crc16_calc = CRC16_XMODEM(res, sizeof(*preamb) + preamb->msg_len);

					if (crc16_recv != crc16_calc)
					{
						ret = false;
					}
				}
			}
		}
		else
		{
			ret = false;
		}
	}
	else
	{
		Error_Handler(); /* Must be called before starting FreeRTOS */
	}

	if (ret == true)
	{
		state = res->preamb.state;
	}

	return state;
}

#endif /* ENABLE_DOWNLOAD */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
