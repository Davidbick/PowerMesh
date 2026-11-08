/**
  ******************************************************************************
  * @file    image_download.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements ST8500 image download.
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
#include <stdlib.h>
#include <string.h>
#include <usart.h>
#include <mem_pool.h>
#include <utils.h>
#include <user_if.h>
#include <image_download.h>
#include <image_management.h>


#if ENABLE_DOWNLOAD

/* Imported functions */
extern void     sendRequest(    UART_HandleTypeDef* huart, ud_req_t* req);
extern uint32_t receiveResponse(UART_HandleTypeDef* huart, ud_res_t* res, uint32_t timeout);

/**
  * @brief  Sets UART parameters for the STM<->ST8500 communication.
  * @param  baudrate: baudrate to use for the communication (baud/s).
  * @param  parity: parity bit to use for the communication (baud/s).
  * @param  stop: stop bits to use for the communication (baud/s).
  * @param  req: pointer to the request message buffer.
  * @param  res: pointer to the response message buffer.
  * @retval 'true' if a positive response is received, 'false' otherwise.
  */
static bool setUartConfig(uint32_t baudrate, uint8_t parity, uint8_t stop, ud_req_t* req, ud_res_t* res)
{
    assert(req != NULL);
    assert(res != NULL);
    
    bool result = false;
    
    com_init_req_payload_t      payload;        /* Payload to send */
    
    payload.baudrate = baudrate;
    payload.parity   = parity;
    payload.stop     = stop;
    
    memset(req, 0, sizeof(ud_req_t));
    memset(res, 0, sizeof(ud_res_t));

    req->preamb.cmd_id  = UD_CMD_ID_COM_INIT;
    req->preamb.msg_len = sizeof(payload);
    memcpy(req->payload, &payload, req->preamb.msg_len);
    
    sendRequest(&huartHostIf, req);
    
    receiveResponse(&huartHostIf, res, RESPONSE_TIMEOUT);
    
    if (res->preamb.cmd_id == req->preamb.cmd_id)
    {
        if (res->preamb.state == UD_STATE_ACK)
        {
            result = true;
        }
    }
    
    if (result == true)
    {
    	/* If successful, changes the baudrate of the PLC UART */
    	uint32_t parity_uart;
    	uint32_t stop_uart;
    	/* Translates parity bit */
		switch(parity)
		{
		case UD_PARITY_EVEN:
			parity_uart = UART_PARITY_EVEN;
			break;
		case UD_PARITY_ODD:
			parity_uart = UART_PARITY_ODD;
			break;
		case UD_PARITY_NONE:
		default:
			parity_uart = UART_PARITY_NONE;
			break;
		}

		/* Translates stop bit */
		switch(stop)
		{
		case UD_STOP_2:
			stop_uart = UART_STOPBITS_2;
			break;
		case UD_STOP_1:
		default:
			stop_uart = UART_STOPBITS_1;
			break;
		}

    	USART_changeSettings(&huartHostIf, baudrate, parity_uart, stop_uart);
    }

    return result;
}

/**
  * @brief  Starts a image download for the ST8500.
  * @param  header: pointer to the header structure of the image to download.
  * @param  header_size: header size of the image to download (in bytes).
  * @param  req: pointer to the request message buffer.
  * @param  res: pointer to the response message buffer.
  * @retval 'true' if a positive response is received, 'false' otherwise.
  */
static bool initDownload(image_header_t *header, uint32_t header_size, ud_req_t* req, ud_res_t* res)
{
    assert(req != NULL);
    assert(res != NULL);
    
    bool result = false;
    
    memset(req, 0, sizeof(ud_req_t));
    memset(res, 0, sizeof(ud_res_t));

    req->preamb.cmd_id  = UD_CMD_ID_IMG_INIT;
    req->preamb.msg_len = header_size;
    memcpy(req->payload, header, header_size);
    
    sendRequest(&huartHostIf, req);
    
    receiveResponse(&huartHostIf, res, RESPONSE_TIMEOUT);
    
    if (res->preamb.cmd_id == req->preamb.cmd_id)
    {
        if (res->preamb.state == UD_STATE_ACK)
        {
            result = true;
        }
    }
    
    return result;
}

/**
  * @brief  Sends a image data block to the ST8500. The data must be already stored in 'req->payload'.
  * @param  block_size: size of the image data block (in bytes).
  * @param  req: pointer to the request message buffer.
  * @param  res: pointer to the response message buffer.
  * @retval 'true' if a positive response is received, 'false' otherwise.
  */
static bool blockDownload(uint32_t block_size, ud_req_t* req, ud_res_t* res)
{
    assert(req != NULL);
    assert(res != NULL);
    
    bool result = false;
    
    /* Do not memset req to 0 */
    memset(res, 0, sizeof(ud_res_t));

    req->preamb.cmd_id = UD_CMD_ID_IMG_WRITE;
    req->preamb.msg_len = block_size;
    /* 'req->payload' filled previously outside the function */

    /* Payload and length set previously */
    sendRequest(&huartHostIf, req);
    
    receiveResponse(&huartHostIf, res, RESPONSE_TIMEOUT);
    
    if (res->preamb.state == UD_STATE_ACK)
    {
        if (res->preamb.cmd_id == req->preamb.cmd_id)
        {
            result = true;
        }
    }
    
    return result;
}

/**
  * @brief  Starts the execution of a image for the ST8500.
  * @param  req: pointer to the request message buffer.
  * @param  res: pointer to the response message buffer.
  * @retval 'true' if a positive response is received, 'false' otherwise.
  */
static bool startST8500(ud_req_t* req, ud_res_t* res)
{
    assert(req != NULL);
    assert(res != NULL);
    
    bool result = false;
    
    memset(req, 0, sizeof(ud_req_t));
    memset(res, 0, sizeof(ud_res_t));

    req->preamb.cmd_id  = UD_CMD_ID_IMG_START;
    req->preamb.msg_len = 0;

    sendRequest(&huartHostIf, req);
    
    receiveResponse(&huartHostIf, res, RESPONSE_TIMEOUT);
    
    if (res->preamb.state == UD_STATE_ACK)
    {
        if (res->preamb.cmd_id == req->preamb.cmd_id)
        {
            result = true;
        }
    }
    
    return result;
}

/**
  * @brief  Handle the full transfer of a single image to the ST8500.
  * @param  img_address: address of the image to transfer in SFLASH.
  * @param  req: pointer to the request message buffer.
  * @param  res: pointer to the response message buffer.
  * @retval 'true' if the image is successfully transfered and
  *          started, 'false' otherwise.
  */
static bool transferRtePeImage(uint32_t image_address, ud_req_t *req, ud_res_t *res)
{
	bool result;
	image_header_t      header;
	uint16_t            header_size;

	uint32_t total_bytes;
	uint32_t bytes_sent;
	uint32_t block_size;
	uint32_t download_blink_ts = 0;

	getImgHeader(image_address, &header, &header_size);

	total_bytes = getFwSize(image_address);
	bytes_sent  = 0;

	result = initDownload(&header, header_size, req, res);

	if (result == true)
	{
		while(bytes_sent < total_bytes)
		{
			if ((total_bytes - bytes_sent) >= sizeof(req->payload))
			{
				block_size = sizeof(req->payload);
			}
			else
			{
				block_size = total_bytes - bytes_sent;
			}

			/* Read flash memory and downloads block to ST8500, must start after the header */
			result = getDataBlock(image_address, &req->payload[0], block_size, header_size + bytes_sent);

			if (result == true)
			{
				result = blockDownload(block_size, req, res);
			}

			if (result == true)
			{
				bytes_sent += block_size;

				utils_blink_led(LED_DOWNLOAD_ST_GPIO_Port, LED_DOWNLOAD_ST_Pin, LED_DOWNLOAD_ST_SEMIPERIOD, &download_blink_ts);
			}
			else
			{
				break;
			}
		}

		if (bytes_sent == total_bytes)
		{
			/* Start downloaded image*/
			result = startST8500(req, res);
		}
		else
		{
			result = false;
		}
	}

	return result;
}


/**
  * @brief  Handle the download procedure of a single image from the host.
  * @param  None
  * @retval 'true' if both images were successfully downloaded, 'false' otherwise.
  */
bool downloadImageToST8500(void)
{
    bool result = false;
    
    ud_req_t *req = MEMPOOL_MALLOC(sizeof(ud_req_t)); /* Request buffer */
    ud_res_t *res = MEMPOOL_MALLOC(sizeof(ud_res_t)); /* Response buffer */

    uint32_t prev_hif_baudrate = huartHostIf.Init.BaudRate;
    uint8_t  prev_hif_parity   = huartHostIf.Init.Parity;
    uint8_t  prev_hif_stop     = huartHostIf.Init.StopBits;

    uint32_t pe_img_address  = IMAGE_NOT_FOUND;
    uint32_t rte_img_address = IMAGE_NOT_FOUND;

    /* Cleans terminal screen */
    PRINT(RESET_DISPLAY_STRING);

    /* First, checks and fixes the validity field for all images */
	PRINT("MAIN:       Scanning for new images...\n");

	if (scanForImagesToValidate() == true)
	{
		PRINT("MAIN:       Validating new images...\n");
	}

	PRINT("MAIN:       Downloading images to ST8500...\n");
    /* Sets the baudrate of the HIF uart to the default value to communicate with the ST8500 */
    USART_changeSettings(&huartHostIf, ST8500_BOOTLOADER_BAUDRATE, UART_PARITY_NONE, UART_STOPBITS_1);
    
    LED_SET_ON(LED_DOWNLOAD_ST);

    do
	{
    	/* Looks for a primary PE image and a primary RTE image */
		result = findImagesToLoad(&pe_img_address, &rte_img_address, IMG_VALIDATED_FIRST);

		/* At least one primary image is missing, so it is necessary to look for a secondary one */
		if (result == false)
		{
			if (pe_img_address == IMAGE_NOT_FOUND)
			{
				PRINT("MAIN:       Missing PE primary image.\n");
			}

			if (rte_img_address == IMAGE_NOT_FOUND)
			{
				PRINT("MAIN:       Missing RTE primary image.\n");
			}

			PRINT("MAIN:       Scanning for secondary images...\n");
			result = findImagesToLoad(&pe_img_address, &rte_img_address, IMG_VALIDATED_SECOND);
		}

		if (result == false)
		{
			PRINT("MAIN:       Could not find PE/RTE images inside the SPI flash memory.\n");
			break;
		}

		/* Warning! 'setUartConfig' changes huartHif configuration, too! */
		result = setUartConfig(UD_BAUDRATE_USED, UD_PARITY_NONE, UD_STOP_1, req, res);

		if (result == false)
		{
			PRINT("MAIN:       Could not configure the UART port.\n");
			break;
		}

		/* RTE image download */
		result = transferRtePeImage(rte_img_address, req, res);

		if (result == false)
		{
			PRINT("MAIN:       Could not download the RTE image.\n");
			break;
		}

		/* PE image download */
		result = transferRtePeImage(pe_img_address, req, res);

		if (result == false)
		{
			PRINT("MAIN:       Could not download the PE image.\n");
			break;
		}
	} while(0);

    MEMPOOL_FREE(req);
    MEMPOOL_FREE(res);

    LED_SET_OFF(LED_DOWNLOAD_ST);

    /* Restores huartHif configuration (undoes 'setUartConfig' effects) */
    USART_changeSettings(&huartHostIf, prev_hif_baudrate, prev_hif_parity, prev_hif_stop);

    if (result == true)
	{
    	PRINT("MAIN:       Starting OS...\n");
	}
	else
	{
		PRINT("MAIN:       Failed to download images to ST8500.\n");
		Error_Handler();
	}

    return result;
}

#endif /* ENABLE_DOWNLOAD */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
