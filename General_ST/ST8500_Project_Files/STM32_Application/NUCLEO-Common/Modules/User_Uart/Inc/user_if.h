/**
  ******************************************************************************
  * @file    user_if.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for the handling of the User Interface.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef USER_IF_H_
#define USER_IF_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup User_If Terminal - User Interface
  * @{
  */
  
/**
  * @}
  */  
  
/** @defgroup User_If_Exported_Code Exported Code
  * @{
  */
  
/**
  * @}
  */  

/** @defgroup User_If_Private_Code Private Code
  * @{
  */
  
/**
  * @}
  */

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <usart.h>
#include <settings.h>

/** @addtogroup User_App
  * @{
  */  
  
/** @addtogroup User_If
  * @{  
  */
  
/** @addtogroup User_If_Exported_Code
  * @{
  */
    
/* Definitions */
#define USERIF_INPUT_MAX_SIZE		(128)    /*!< Maximum user input length, in bytes  */
#define USERIF_PRINT_MAX_SIZE		(512)    /*!< Maximum user interface print length, in bytes */

/* Escape sequences */

#define RESET_DISPLAY_STRING		"\e[2J\e[0;0H"	/* Clean terminal screen and sets cursor position to (0,0) */

/* Change printed color */
#define COLOR_DEFAULT_STR			"\e[0m"			/* Default terminal color */
#define COLOR_RED_STR				"\e[0;31m"		/* Red */
#define COLOR_GREEN_STR				"\e[0;32m"		/* Green */
#define COLOR_YELLOW_STR			"\e[0;33m"		/* Yellow */
#define COLOR_BLUE_STR				"\e[0;34m"		/* Blue */
#define COLOR_MAGENTA_STR			"\e[0;35m"		/* Magenta */
#define COLOR_CYAN_STR				"\e[0;36m"		/* Cyan */
#define COLOR_WHITE_STR				"\e[0;37m"		/* White */


/* Normal prints on the terminal (with or without time stamp) */
#define PRINT(format, args...)          					user_if_printf(true,  color_default, NULL, format, ## args)
#define PRINT_NOTS(format, args...)     					user_if_printf(false, color_default, NULL, format, ## args)

/* Print ASCII string directly */
#define PRINT_RAW(string, length)          					user_if_print_raw(string, length)

/* Colored prints on the terminal (with or without time stamp) */
#define PRINT_COLOR(format, color, args...)					user_if_printf(true,  color, NULL, format, ## args)
#define PRINT_COLOR_NOTS(format, color, args...)			user_if_printf(false, color, NULL, format, ## args)

/* Custom types */

typedef struct userif_cmd_str
{
	uint32_t length;
	uint8_t  payload[USERIF_INPUT_MAX_SIZE];
} user_input_t;

typedef enum color_enum
{
	color_default,
	color_red,
	color_green,
	color_yellow,
	color_blue,
	color_magenta,
	color_cyan,
	color_white,
} color_t;

/* Public functions */
void 			user_if_init(void);
void 			user_if_init_rx_fifo(void);
void			user_if_rx_start(void);
void			user_if_rx_stop(void);

user_input_t*	user_if_get_input(void);
bool 			user_if_search_char(uint8_t byte_char);
uint32_t 		user_if_printf(bool include_ts, color_t color, const char *label, const char *format, ...);
uint32_t 		user_if_print_raw(const char *string, const uint16_t length);

/* Callback functions */
void 			user_if_rx_handler(void);
void 			user_if_tx_handler(void);

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

#endif /* USER_IF_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
