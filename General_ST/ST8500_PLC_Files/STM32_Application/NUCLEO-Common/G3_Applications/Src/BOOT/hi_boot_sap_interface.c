/*****************************************************************************
*   @file    hi_boot_sap_interface.c
*   @author  AMG/IPC Application Team
*   @brief   This code include utilities for the interface between Bootstrap Handler and SAP levels.
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
#include <hi_boot_sap_interface.h>

/** @addtogroup HI_DRIVER ST8500 HI DRIVER
  * @{
  */

/** @defgroup hif_boot_drv HIF BOOT DRV
  * @{
  */

static char unknown_str[] =  "Unknown";

/**
  * @brief Returns the string corresponding to the device type.
  * @param dev_type Indicates the device type to translate (must be a value of @ref plcType_t).
  * @retval String corresponding to the device type.
  */
char* hi_boot_sap_translate_dev_type(plcType_t dev_type)
{
	switch (dev_type)
	{
	case PLC_G3_DEVICE: return "DEVICE";
	case PLC_G3_COORD:  return "COORDINATOR";
	default: 			return unknown_str;
	}
}

/**
  * @brief Returns the string corresponding to the bandplan.
  * @param bandplan Indicates the bandplan type to translate (must be a value of @ref BOOT_Bandplan_t).
  * @retval String corresponding to the bandplan.
  */
char* hi_boot_sap_translate_bandplan(BOOT_Bandplan_t bandplan)
{
	switch (bandplan)
	{
	case BOOT_BANDPLAN_CENELEC_A:		return "CENELEC A";
	case BOOT_BANDPLAN_CENELEC_B:		return "CENELEC B";
#if 0
	case BOOT_BANDPLAN_ARIB:			return "ARIB";
#endif
	case BOOT_BANDPLAN_FCC:				return "FCC";
#if 0
	case BOOT_BANDPLAN_IEEE_1901_2_FCC:	return "FCC above CENELEC";

		break;
#endif
	default:							return unknown_str;
	}
}

/**
  * @brief Returns the string corresponding to the platform.
  * @param platform Indicates the platform type to translate (must be a value of @ref platform_t).
  * @retval String corresponding to the platform.
  */
char* hi_boot_sap_translate_platform(uint32_t platform)
{
	switch (platform)
	{
	case PLAT_INVALID:			return "Invalid";
	case PLAT_STCOMET_STCOM:	return "STCOMET/STCOM";
	case PLAT_ST8500:			return "ST8500";
	case PLAT_UNSPEC:			return "Unspecified";
	default:					return unknown_str;
	}
}

/**
  * @brief Returns the string corresponding to the PLC mode.
  * @param platform Indicates the PLC mode to translate (must be a value of @ref plc_mode_t).
  * @retval String corresponding to the PLC mode.
  */
char* hi_boot_sap_translate_mode(uint32_t plc_mode)
{
	switch (plc_mode)
	{
	case PLC_MODE_PHY: 		 return "PHY";
	case PLC_MODE_MAC: 		 return "MAC";
	case PLC_MODE_ADP: 		 return "ADP";
	case PLC_MODE_ADP_BOOT:  return "ADP BOOT";
	case PLC_MODE_IPV6_ADP:	 return "IPV6 ADP";
	case PLC_MODE_IPV6_BOOT: return "IPV6 BOOT";
	default:				 return unknown_str;
	}
}


/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
