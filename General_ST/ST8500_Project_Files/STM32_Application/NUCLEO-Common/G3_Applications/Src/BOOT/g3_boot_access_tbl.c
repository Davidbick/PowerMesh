/******************************************************************************
*   @file    g3_boot_access_tbl.c
*   @author  AMG/IPC Application Team
*   @brief   This code includes the function to read the boot access table.
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
#include <debug_print.h>
#include <utils.h>
#include <g3_app_config.h>
#include <g3_boot_access_tbl.h>
#include <main.h>


/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot
  * @{
  */

/** @addtogroup G3_App_Boot_Private_Code
  * @{
  */

/* Macros */

#if IS_COORD

/* Extended addresses in the boot access table */
#define DEVICE_1_EXT_ADDR	{0x00, 0x80, 0xE1, 0xFF, 0xFE, 0x00, 0x00, 0x00}
#define DEVICE_2_EXT_ADDR	{0x00, 0x80, 0xE1, 0xFF, 0xFE, 0x00, 0x00, 0x00}

/* Private variables */

/* G3 Boot access table  */
/* If black list mode is enabled, it acts like a blacklist, otherwise it acts as a whitelist */
static g3_boot_data_t g3_boot_access_table[] = {
    {
        .short_addr = 1,	/* 1 is the first short address for devices */
        .ext_addr 	= DEVICE_1_EXT_ADDR,
        .psk 		= DEFAULT_PSK
    },
    {
        .short_addr = 2,
        .ext_addr 	= DEVICE_2_EXT_ADDR,
        .psk 		= DEFAULT_PSK
    },

    /* Add more devices here... short addresses must be consecutive
     * {
     *     .short_addr = 3U,
     *     .ext_addr   = {0x00, 0x80, 0xE1, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF},
     *     .psk 	   = DEFAULT_PSK
     * }
     */
};

/* Private Functions */

/**
  * @brief Compares the extended addresses of two elements inside a G3 Boot table.
  * @param first Pointer to the first element of the G3 Boot table.
  * @param second Pointer to the second element of the G3 Boot table.
  * @return 1 if the first element has an higher extended address,
  * -1 if the first element has a lower extended address,
  * 0 if the two elements have the same extended address.
  */
static int g3_boot_data_tbl_compare_ext_addr(const void *first, const void *second)
{
    const g3_boot_data_t *a = first;
    const g3_boot_data_t *b = second;

    for (int32_t i = MAC_ADDR64_SIZE - 1; i >= 0; i--)
    {
        if (a->ext_addr[i] == b->ext_addr[i])
        {
            continue;
        }
        else if (a->ext_addr[i] > b->ext_addr[i])
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Exported_Code
  * @{
  */

/* Public functions */

/**
  * @brief Initializes the G3 Boot access table.
  * @param None
  * @retval None
  */
void g3_boot_access_table_init(void)
{
	/* Sorts by extended address */
	qsort(g3_boot_access_table, NUM_OF_ELEM(g3_boot_access_table), sizeof(g3_boot_access_table[0]), g3_boot_data_tbl_compare_ext_addr);
}

/**
  * @brief Looks for a device with a specific extended address inside the G3 Boot access table (which is read only).
  * @param extended_address Pointer to the extended address of the desired device.
  * @return Pointer to the desired device, if found, NULL otherwise.
  */
g3_boot_data_t *g3_boot_access_table_find(const uint8_t *extended_address)
{
	g3_boot_data_t *data_entry;
    g3_boot_data_t key = { 0 };

    memcpy(key.ext_addr, extended_address, sizeof(key.ext_addr));

    data_entry = bsearch(&key, g3_boot_access_table, NUM_OF_ELEM(g3_boot_access_table), sizeof(g3_boot_access_table[0]), g3_boot_data_tbl_compare_ext_addr);

    return data_entry;
}

#endif /* IS_COORD */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2021 STMicroelectronics *******************/
