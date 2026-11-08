/**
  ******************************************************************************
  * @file    image_download.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define ST8500 image download macros/definitions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef IMAGE_DOWNLOAD_H_
#define IMAGE_DOWNLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <g3_app_config.h>
#include <image_management.h>

/* Definitions */

/* Baudrates */
#define UD_BAUDRATE_9600        9600
#define UD_BAUDRATE_14400       14400
#define UD_BAUDRATE_19200       19200
#define UD_BAUDRATE_38400       38400
#define UD_BAUDRATE_57600       57600
#define UD_BAUDRATE_115200      115200
#define UD_BAUDRATE_230400      230400
#define UD_BAUDRATE_921600      921600

#define UD_BAUDRATE_USED		UD_BAUDRATE_921600

/* Parity */
#define UD_PARITY_NONE          0x00
#define UD_PARITY_ODD           0x01
#define UD_PARITY_EVEN          0x02

/* Stop bits */
#define UD_STOP_1               0x01
#define UD_STOP_2               0x02

/* System commands */
#define UD_CMD_ID_COM_INIT      0x01
#define UD_CMD_ID_SYS_RESET     0x02

/* Image download commands */
#define UD_CMD_ID_IMG_INIT      0x11
#define UD_CMD_ID_IMG_WRITE     0x12
#define UD_CMD_ID_IMG_START     0x13

/* Management Information Base command */
#define UD_CMD_ID_MIB_GET       0x21

/* Fixed value fields */
#define UD_SYNCHRO_VALUE        0x1616
#define UD_MODE_VALUE           0xFE

/* Request/Response state */
#define UD_STATE_ACK            0x00
#define UD_STATE_NACK           0x01
#define UD_STATE_ABORT          0x02
#define UD_STATE_ERROR          0xFF

/* Timing */
#define REQUEST_TIMEOUT         1000	/* In ms */
#define RESPONSE_TIMEOUT        1000	/* In ms */

/* Custom types */

#pragma pack(push, 1)

/* Structures and Unions */

/* Messages (Requests and Responses) */

typedef struct com_init_req_payload_struct {
  uint32_t baudrate;
  uint8_t  parity;
  uint8_t  stop;
} com_init_req_payload_t;

typedef struct img_init_req_payload_struct {
  uint8_t  img_header[DE_MAX_HEADER_SIZE];
} img_init_req_payload_t;

typedef struct img_write_req_payload_struct {
  uint8_t  img_data_block[DE_MAX_BLOCK_SIZE];
} img_write_req_payload_t;

typedef struct mib_get_req_payload_struc {
  uint8_t  attributeId;
} mib_get_req_payload_t;

typedef union mib_get_res_payload_union {
  uint16_t rte_boot_status;
  uint16_t pe_boot_status;
  uint16_t secutity_bits;
  uint16_t boot_mode;
  uint32_t boot_version;
  uint8_t  eui64[DE_EUI64_ATTR_VAL_SIZE];
  uint8_t  attributeValue[DE_MAX_ATTR_VAL_SIZE];
} mib_get_res_payload_t;

typedef union {
	com_init_req_payload_t		 com_init_req;
	img_init_req_payload_t 		 img_init_req;
	img_write_req_payload_t 	 img_write_req;
	mib_get_req_payload_t 		 mib_get_req;
} req_payload_t;

typedef union {
	mib_get_res_payload_t 		 mib_get_res;
} res_payload_t;

typedef struct preamb_struc {
  uint16_t sync;
  uint8_t  cmd_id;
  uint16_t msg_len;
  uint8_t  mode;
  uint32_t state;
} preamb_t;

/* For variable length messages, maximum length is used for structures */
/* Do not use 'sizeof' for message length */
typedef struct ud_req_struc {
  preamb_t preamb;
  uint8_t  payload[sizeof(req_payload_t)];
  uint16_t crc16;
} ud_req_t;

typedef struct ud_res_struc {
  preamb_t preamb;
  uint8_t  payload[sizeof(res_payload_t)];
  uint16_t crc16;
} ud_res_t;

#pragma pack(pop)

/* Public functions */

/* Handle images transfer from the STM32 to the ST8500 */
bool downloadImageToST8500(void);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_DOWNLOAD_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
