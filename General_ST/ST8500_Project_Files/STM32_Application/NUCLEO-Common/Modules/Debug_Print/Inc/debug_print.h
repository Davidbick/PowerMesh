/**
  ******************************************************************************
  * @file    debug_print.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define/enable debug log prints.
  *   *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/


#ifndef DEBUG_PRINT_H_
#define DEBUG_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Debug_Utility Debug Utility
  * @{
  */

/**
  * @}
  */

/* Inclusions */
#include <user_if.h>
#include <settings.h>

/** @addtogroup Debug_Utility
  * @{
  */

/* Debug prints of the terminal */
#define PRINT_GENERAL_INFO(label, format, args...)     		user_if_printf(true, color_default,	label, format, ## args)
#define PRINT_GENERAL_WARNING(label, format, args...)		user_if_printf(true, color_yellow,	label, format, ## args)
#define PRINT_GENERAL_CRITICAL(label, format, args...)		user_if_printf(true, color_red,		label, format, ## args)

#if (DEBUG_G3_MSG >= DEBUG_LEVEL_CRITICAL)
	extern const char label_g3_msg[];
	#define PRINT_G3_MSG_CRITICAL(format, args...)  		PRINT_GENERAL_CRITICAL(label_g3_msg, format, ## args)
	#if (DEBUG_G3_MSG >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_MSG_WARNING(format, args...)   	PRINT_GENERAL_WARNING(label_g3_msg, format, ## args)
	#endif
	#if (DEBUG_G3_MSG >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_MSG_INFO(format, args...)       	PRINT_GENERAL_INFO(label_g3_msg, format, ## args)
	#endif
#endif

#if (DEBUG_G3_CONF >= DEBUG_LEVEL_CRITICAL)
	extern const char label_g3_conf[];
	#define PRINT_G3_CONF_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_g3_conf, format, ## args)
	#if (DEBUG_G3_CONF >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_CONF_WARNING(format, args...)  	PRINT_GENERAL_WARNING(label_g3_conf, format, ## args)
	#endif
	#if (DEBUG_G3_CONF >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_CONF_INFO(format, args...)      	PRINT_GENERAL_INFO(label_g3_conf, format, ## args)
	#endif
#endif
    
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_CRITICAL)
	extern const char label_g3_boot[];
	#define PRINT_G3_BOOT_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_g3_boot, format, ## args)
	#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_BOOT_WARNING(format, args...)  	PRINT_GENERAL_WARNING(label_g3_boot, format, ## args)
	#endif
	#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_BOOT_INFO(format, args...)      	PRINT_GENERAL_INFO(label_g3_boot, format, ## args)
	#endif
#endif

#if ((DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_CRITICAL)  && IS_COORD)
	extern const char label_g3_boot_srv[];
	#define PRINT_G3_BOOT_SRV_CRITICAL(format, args...)		PRINT_GENERAL_CRITICAL(label_g3_boot_srv, format, ## args)
	#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_BOOT_SRV_WARNING(format, args...)	PRINT_GENERAL_WARNING(label_g3_boot_srv, format, ## args)
	#endif
	#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_BOOT_SRV_INFO(format, args...)   	PRINT_GENERAL_INFO(label_g3_boot_srv, format, ## args)
	#endif
#endif

#if ((DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_CRITICAL)  && !IS_COORD)
	extern const char label_g3_boot_clt[];
	#define PRINT_G3_BOOT_CLT_CRITICAL(format, args...)		PRINT_GENERAL_CRITICAL(label_g3_boot_clt, format, ## args)
	#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_BOOT_CLT_WARNING(format, args...)	PRINT_GENERAL_WARNING(label_g3_boot_clt, format, ## args)
	#endif
	#if (DEBUG_G3_BOOT_CLT >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_BOOT_CLT_INFO(format, args...)   	PRINT_GENERAL_INFO(label_g3_boot_clt, format, ## args)
	#endif
#endif

#if ((DEBUG_G3_PANSORT >= DEBUG_LEVEL_CRITICAL) && !IS_COORD)
	extern const char label_g3_pansort[];
	#define PRINT_G3_PANSORT_CRITICAL(format, args...)   	PRINT_GENERAL_CRITICAL(label_g3_pansort, format, ## args)
	#if (DEBUG_G3_PANSORT >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_PANSORT_WARNING(format, args...)	PRINT_GENERAL_WARNING(label_g3_pansort, format, ## args)
	#endif
	#if (DEBUG_G3_PANSORT >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_PANSORT_INFO(format, args...)   	PRINT_GENERAL_INFO(label_g3_pansort, format, ## args)
	#endif
#endif

#if (DEBUG_G3_KA >= DEBUG_LEVEL_CRITICAL)
	extern const char label_g3_ka[];
	#define PRINT_G3_KA_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_g3_ka, format, ## args)
	#if (DEBUG_G3_KA >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_KA_WARNING(format, args...)		PRINT_GENERAL_WARNING(label_g3_ka, format, ## args)
	#endif
	#if (DEBUG_G3_KA >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_KA_INFO(format, args...)        	PRINT_GENERAL_INFO(label_g3_ka, format, ## args)
	#endif
#endif

#if (DEBUG_G3_LAST_GASP >= DEBUG_LEVEL_CRITICAL)
	extern const char label_g3_last_gasp[];
	#define PRINT_G3_LAST_GASP_CRITICAL(format, args...)	PRINT_GENERAL_CRITICAL(label_g3_last_gasp, format, ## args)
	#if (DEBUG_G3_LAST_GASP >= DEBUG_LEVEL_WARNING)
		#define PRINT_G3_LAST_GASP_WARNING(format, args...)	PRINT_GENERAL_WARNING(label_g3_last_gasp, format, ## args)
	#endif
	#if (DEBUG_G3_LAST_GASP >= DEBUG_LEVEL_INFO)
		#define PRINT_G3_LAST_GASP_INFO(format, args...)    PRINT_GENERAL_INFO(label_g3_last_gasp, format, ## args)
	#endif
#endif

#if (DEBUG_USER_G3 >= DEBUG_LEVEL_CRITICAL)
	extern const char label_user_g3[];
	#define PRINT_USER_G3_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_user_g3, format, ## args)
	#if (DEBUG_USER_G3 >= DEBUG_LEVEL_WARNING)
		#define PRINT_USER_G3_WARNING(format, args...)  	PRINT_GENERAL_WARNING(label_user_g3, format, ## args)
	#endif
	#if (DEBUG_USER_G3 >= DEBUG_LEVEL_INFO)
		#define PRINT_USER_G3_INFO(format, args...)      	PRINT_GENERAL_INFO(label_user_g3, format, ## args)
	#endif
#endif

#if (DEBUG_USER_IT >= DEBUG_LEVEL_CRITICAL)
	extern const char label_user_it[];
	#define PRINT_USER_IT_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_user_it, format, ## args)
	#if (DEBUG_USER_IT >= DEBUG_LEVEL_WARNING)
		#define PRINT_USER_IT_WARNING(format, args...)  	PRINT_GENERAL_WARNING(label_user_it, format, ## args)
	#endif
	#if (DEBUG_USER_IT >= DEBUG_LEVEL_INFO)
		#define PRINT_USER_IT_INFO(format, args...)      	PRINT_GENERAL_INFO(label_user_it, format, ## args)
	#endif
#endif

#if (DEBUG_SFLASH >= DEBUG_LEVEL_CRITICAL)
	extern const char label_sflash[];
	#define PRINT_SFLASH_CRITICAL(format, args...)			PRINT_GENERAL_CRITICAL(label_sflash, format, ## args)
	#if (DEBUG_SFLASH >= DEBUG_LEVEL_WARNING)
		#define PRINT_SFLASH_WARNING(format, args...) 		PRINT_GENERAL_WARNING(label_sflash, format, ## args)
	#endif
	#if (DEBUG_SFLASH >= DEBUG_LEVEL_INFO)
		#define PRINT_SFLASH_INFO(format, args...) 			PRINT_GENERAL_INFO(label_sflash, format, ## args)
	#endif
#endif

/* Defines unused macros to nothing */
#ifndef PRINT_G3_MSG_INFO
#define PRINT_G3_MSG_INFO(format, args...)
#endif

#ifndef PRINT_G3_MSG_WARNING
#define PRINT_G3_MSG_WARNING(format, args...)
#endif

#ifndef PRINT_G3_MSG_CRITICAL
#define PRINT_G3_MSG_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_CONF_INFO
#define PRINT_G3_CONF_INFO(format, args...)
#endif

#ifndef PRINT_G3_CONF_WARNING
#define PRINT_G3_CONF_WARNING(format, args...)
#endif

#ifndef PRINT_G3_CONF_CRITICAL
#define PRINT_G3_CONF_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_BOOT_INFO
#define PRINT_G3_BOOT_INFO(format, args...)
#endif

#ifndef PRINT_G3_BOOT_WARNING
#define PRINT_G3_BOOT_WARNING(format, args...)
#endif

#ifndef PRINT_G3_BOOT_CRITICAL
#define PRINT_G3_BOOT_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_BOOT_SRV_INFO
#define PRINT_G3_BOOT_SRV_INFO(format, args...)
#endif

#ifndef PRINT_G3_BOOT_SRV_WARNING
#define PRINT_G3_BOOT_SRV_WARNING(format, args...)
#endif

#ifndef PRINT_G3_BOOT_SRV_CRITICAL
#define PRINT_G3_BOOT_SRV_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_BOOT_CLT_INFO
#define PRINT_G3_BOOT_CLT_INFO(format, args...)
#endif

#ifndef PRINT_G3_BOOT_CLT_WARNING
#define PRINT_G3_BOOT_CLT_WARNING(format, args...)
#endif

#ifndef PRINT_G3_BOOT_CLT_CRITICAL
#define PRINT_G3_BOOT_CLT_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_PANSORT_INFO
#define PRINT_G3_PANSORT_INFO(format, args...)
#endif

#ifndef PRINT_G3_PANSORT_WARNING
#define PRINT_G3_PANSORT_WARNING(format, args...)
#endif

#ifndef PRINT_G3_PANSORT_CRITICAL
#define PRINT_G3_PANSORT_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_KA_INFO
#define PRINT_G3_KA_INFO(format, args...)
#endif

#ifndef PRINT_G3_KA_WARNING
#define PRINT_G3_KA_WARNING(format, args...)
#endif

#ifndef PRINT_G3_KA_CRITICAL
#define PRINT_G3_KA_CRITICAL(format, args...)
#endif

#ifndef PRINT_G3_LAST_GASP_INFO
#define PRINT_G3_LAST_GASP_INFO(format, args...)
#endif

#ifndef PRINT_G3_LAST_GASP_WARNING
#define PRINT_G3_LAST_GASP_WARNING(format, args...)
#endif

#ifndef PRINT_G3_LAST_GASP_CRITICAL
#define PRINT_G3_LAST_GASP_CRITICAL(format, args...)
#endif
#ifndef PRINT_USER_G3_INFO
#define PRINT_USER_G3_INFO(format, args...)
#endif

#ifndef PRINT_USER_G3_WARNING
#define PRINT_USER_G3_WARNING(format, args...)
#endif

#ifndef PRINT_USER_G3_CRITICAL
#define PRINT_USER_G3_CRITICAL(format, args...)
#endif

#ifndef PRINT_USER_IT_INFO
#define PRINT_USER_IT_INFO(format, args...)
#endif

#ifndef PRINT_USER_IT_WARNING
#define PRINT_USER_IT_WARNING(format, args...)
#endif

#ifndef PRINT_USER_IT_CRITICAL
#define PRINT_USER_IT_CRITICAL(format, args...)
#endif

#ifndef PRINT_SFLASH_INFO
#define PRINT_SFLASH_INFO(format, args...)
#endif

#ifndef PRINT_SFLASH_WARNING
#define PRINT_SFLASH_WARNING(format, args...)
#endif

#ifndef PRINT_SFLASH_CRITICAL
#define PRINT_SFLASH_CRITICAL(format, args...)
#endif


#if (DEBUG_G3_MSG || DEBUG_G3_BOOT || DEBUG_G3_BOOT_SRV || DEBUG_G3_BOOT_CLT || DEBUG_G3_CONF || DEBUG_G3_KA || DEBUG_G3_LAST_GASP)

/* Public functions */
char* translateG3cmd(uint8_t cmd_id);

#endif

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_PRINT_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
