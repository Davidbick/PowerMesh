/**
  ******************************************************************************
  * @file    g3_comm.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the G3 communication.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef G3_COMM_H_
#define G3_COMM_H_

#include <task_comm.h>

/* G3 Message Structure */
typedef struct g3_msg_str
{
	hif_cmd_id_t	command_id;
    size_t			size;
    void			*payload;
} g3_msg_t;

/* Public Functions */
void g3_send_message(msg_type_t msg_type, hif_cmd_id_t msg_id, void *payload_pool, uint16_t payload_len);
void g3_copy_and_send_message(msg_type_t msg_type, hif_cmd_id_t msg_id, void *payload, uint16_t payload_len);
void g3_discard_message(g3_msg_t *g3_msg);

#endif /* G3_COMM_H_ */
