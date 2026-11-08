/**
  ******************************************************************************
  * @file    host_if.h
  * @author  AMG/IPC Application Team
  * @brief   This file contains declarations for the Host Interface handling.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef HOST_IF_H_
#define HOST_IF_H_

/* Inclusions */
#include <stdint.h>

/* Definitions */
#define HIF_PREAMBLE_BYTE_VALUE			0x16U
#define HIF_PREAMBLE_FIELD_VALUE		0x1616U

/* G3 HIF FRAMES */
#define HIF_SYNC_LSB_POS				0
#define HIF_SYNC_MSB_POS				1
#define HIF_CMD_ID_POS					2
#define HIF_LEN_LSB_POS					3
#define HIF_LEN_MSB_POS					4

#define HIF_SYNC_LEN        			2 		/* Sync field length */
#define HIF_CMD_ID_LEN        			1		/* Command ID field length */
#define HIF_MSGLEN_LEN        			2		/* Message length field length */
#define HIF_MODE_LEN       				1		/* Mode field length */
#define HIF_COUNTER_LEN       			4		/* Counter field length */
#define HIF_EC_LEN						1		/* Error Code field length (only in confirms and indications) */
#define HIF_CRC_LEN           			2		/* CRC16 field length */

#define HIF_PREAMBLE_LEN				10U		/* HIF_SYNC_LEN + HIF_CMD_ID_LEN + HIF_MSGLEN_LEN + HIF_MODE_LEN + HIF_COUNTER_LEN  */
#define HIF_PREAMBLE_AND_EC_LEN			11U		/* HIF_SYNC_LEN + HIF_CMD_ID_LEN + HIF_MSGLEN_LEN + HIF_MODE_LEN + HIF_COUNTER_LEN + HIF_EC_LEN */
#define HIF_REM_PREAMBLE_AND_EC_LEN		10U		/* HIF_SYNC_LEN + HIF_CMD_ID_LEN + HIF_MSGLEN_LEN + HIF_MODE_LEN + HIF_COUNTER_LEN + HIF_EC_LEN - 1 sync field byte */

/* For confirms and indications */
#define HIF_GET_SYNC(hif_msg)			ASSEMBLE_U16(hif_msg->preamble[HIF_SYNC_MSB_POS], hif_msg->preamble[HIF_SYNC_LSB_POS])
#define HIF_GET_CMD_ID(hif_msg)			(hif_msg->preamble[HIF_CMD_ID_POS])
#define HIF_GET_MSG_LEN(hif_msg)		ASSEMBLE_U16(hif_msg->preamble[HIF_LEN_MSB_POS], hif_msg->preamble[HIF_LEN_LSB_POS])
#define HIF_GET_REP_EC(hif_msg)			(hif_msg->preamble[HIF_PREAMBLE_LEN])
#define HIF_GET_REP_PAYLOAD(hif_msg)	(&hif_msg->payload_crc[0])
#define HIF_GET_REP_CRC16(hif_msg)		ASSEMBLE_U16(hif_msg->payload_crc[hif_msg->payload_len+1], hif_msg->payload_crc[hif_msg->payload_len])

/* Custom types */
typedef struct host_if_msg_rx_str
{
	uint8_t 	preamble[HIF_PREAMBLE_AND_EC_LEN]; 	/* Used to store the preamble */
	uint8_t 	*payload_crc;						/* Points to the memory pool where payload + CRC are allocated */
	uint16_t 	payload_len;						/* Payload length, Error code (EC) is excluded */
} host_if_msg_rx_t;

/* Public Functions */
void	 host_if_init(void);
void	 host_if_rx_start(void);
void	 host_if_rx_stop(void);
void	 host_if_rx_handler(void);
void	 host_if_tx_handler(void);
uint32_t host_if_send_message(uint8_t cmd_id, void *payload, uint16_t payload_len);

#endif /* HOST_IF_H_ */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
