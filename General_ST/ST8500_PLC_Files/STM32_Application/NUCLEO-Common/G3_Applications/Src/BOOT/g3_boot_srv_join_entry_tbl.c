/******************************************************************************
*   @file    g3_boot_srv_join_entry_tbl.c
*   @author  AMG/IPC Application Team
*   @brief   This file contains the implementation of the Boot joining entry table management.
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
#include <stdbool.h>
#include <string.h>
#include <utils.h>
#include <debug_print.h>
#include <hi_mac_message_catalog.h>
#include <g3_app_boot_constants.h>
#include <g3_boot_srv_join_entry_tbl.h>
#include <main.h>

/** @addtogroup G3_App
  * @{
  */

/** @addtogroup G3_App_Boot_Srv_Tbl
  * @{
  */

/** @addtogroup G3_App_Boot_Srv_Tbl_Private_Code
  * @{
  */

#if ENABLE_BOOT_SERVER_ON_HOST

/* Static variables */
static boot_join_entry_t boot_join_table[BOOT_MAX_NUM_JOINING_NODES];

/**
  * @brief Initialize a new Joining entry for the sender of LBP Joining message
  * @param [ext_addr The Extended Address of the Node that sent the LBP Joining message
  * @param LBAAddress The Extended Address of the agent that relayed the LBP Joining message, if any
  * @param MediaType Identifies the MediaType used for LBD � LBA communication (0x00 PLC, 0x01 RF)
  * @param Control use of backup media (0x00: backup media usage is enabled, 0x01: backup media usage is disabled)
  * @return The pointer to the added joining entry
  */
boot_join_entry_t* g3_boot_srv_join_entry_add(const uint8_t* ext_addr, const uint16_t lba_short_addr, const uint8_t media_type, const uint8_t disable_bkp, const bool rekeying)
{
	boot_join_entry_t* join_entry;

	/* Find an empty entry */
	join_entry = g3_boot_srv_join_entry_find_free();

	if (join_entry != NULL)
	{
		/* Add a new Joining entry in the Joining table */
		memcpy(join_entry->ext_addr, ext_addr, MAC_ADDR64_SIZE);

		/* Initialize the Encryption parameters used for EAP-PSK handshake */
		memset(&(join_entry->eap_psk_data), 0x00, sizeof(join_entry->eap_psk_data));

		/* Initializing the first Identifier with a random number rather than starting from zero is recommended, since it makes sequence attacks somewhat more difficult. */
		srand(HAL_GetTick());
		join_entry->eap_psk_data.eap_id = rand() & 0xFF;

		join_entry->join_time 			= HAL_GetTick();
		join_entry->short_addr 			= MAC_BROADCAST_SHORT_ADDR;
		join_entry->lba_addr 			= lba_short_addr;
		join_entry->curr_state 			= BOOT_SRV_EAP_ST_WAIT_JOIN;
		join_entry->curr_event			= BOOT_SRV_EAP_EV_NONE;
		join_entry->media_type 			= media_type;
		join_entry->accept_handle		= 0xFFFF;
		join_entry->disable_bkp 		= disable_bkp;
		join_entry->rekeying    		= rekeying;

#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
		ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, MAC_ADDR64_SIZE);
		PRINT_G3_BOOT_SRV_INFO("Added joining entry %u for %s\n", g3_boot_srv_join_entry_index(join_entry), ext_addr_str);
#endif
	}

	return join_entry;
}

/**
  * @brief Removes the specified joining entry
  * @param join_entry Pointer to the joining entry to remove
  * @return None
  */
void g3_boot_srv_join_entry_remove(boot_join_entry_t* join_entry)
{
#if (DEBUG_G3_BOOT_SRV >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(ext_addr_str, join_entry->ext_addr, MAC_ADDR64_SIZE);
	PRINT_G3_BOOT_SRV_INFO("Removed joining entry %u for %s\n", g3_boot_srv_join_entry_index(join_entry), ext_addr_str);
#endif
	/* Free Joining Entry */
	memset(join_entry->ext_addr, 0, MAC_ADDR64_SIZE);
	join_entry->short_addr 			= MAC_BROADCAST_SHORT_ADDR;
	join_entry->lba_addr 			= 0;
	join_entry->curr_state 			= BOOT_SRV_EAP_ST_WAIT_JOIN;
	join_entry->curr_event			= BOOT_SRV_EAP_EV_NONE;
	join_entry->media_type 			= 0;
	join_entry->accept_handle		= 0xFFFF;
	join_entry->disable_bkp 		= 0;
	join_entry->rekeying    		= 0;
}

/**
  * @brief Find the joining entry corresponding to the given extended address.
  * @param [in] ext_addr Extended address of the joining entry to find.
  * @return pointer to the entry found, NULL if not found
  */
boot_join_entry_t* g3_boot_srv_join_entry_find(const uint8_t ext_addr[MAC_ADDR64_SIZE])
{
	boot_join_entry_t* found_join_entry = NULL;

    // Check if an entry is already existing for the current EUI-64
    for (uint16_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		if (memcmp(boot_join_table[i].ext_addr, ext_addr, MAC_ADDR64_SIZE) == 0)
		{
			found_join_entry = &boot_join_table[i];
			break;
		}
	}

    return found_join_entry;
}

/**
  * @brief Find the joining entry corresponding to the given accept handle.
  * @param [in] handle Accept handle of the joining entry to find.
  * @return pointer to the entry found, NULL if not found
  */
boot_join_entry_t* g3_boot_srv_join_entry_find_handle(const uint16_t handle)
{
	boot_join_entry_t* found_join_entry = NULL;

    // Check if an entry is already existing for the current EUI-64
    for (uint16_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		if (boot_join_table[i].accept_handle == handle)
		{
			found_join_entry = &boot_join_table[i];
			break;
		}
	}

    return found_join_entry;
}

/**
  * @brief Find a free joining entry
  * @return pointer to the entry found, NULL if not found
  */
boot_join_entry_t* g3_boot_srv_join_entry_find_free()
{
	const uint8_t free_entry[MAC_ADDR64_SIZE] = {0};

    return g3_boot_srv_join_entry_find(free_entry);
}

/**
  * @brief Get the joining entry at a specific index
  * @param entry_index The index of the joining entry
  * @return The Joining Entry address
  */
boot_join_entry_t* g3_boot_srv_join_entry_get(uint32_t entry_index)
{
	assert(entry_index < BOOT_MAX_NUM_JOINING_NODES);

	return &boot_join_table[entry_index];
}

/**
  * @brief Get the index of a specific joining entry
  * @param join_entry The address of the joining entry
  * @return The joining entry index
  */
uint32_t g3_boot_srv_join_entry_index(boot_join_entry_t* join_entry)
{
	/* Make sure the joining entry is inside the joining entry table */
	assert((((uint32_t) join_entry) - ((uint32_t) &boot_join_table[0])) % sizeof(boot_join_table[0]) == 0);

    return ((uint32_t) (join_entry - &boot_join_table[0]));
}

/**
  * @brief Initialization of Bootstrapping Server Data
  * @return None
  */
void g3_boot_srv_init_tables(void)
{
    uint16_t i;

    // Initialize the table containing data of Nodes that are joining the PAN
    memset(boot_join_table, 0, sizeof(boot_join_table));

    for(i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
    {
    	boot_join_table[i].short_addr = MAC_BROADCAST_SHORT_ADDR;
    	boot_join_table[i].accept_handle = 0xFFFF; /* assigns value that cannot be mistaken for a 8 bit value */
    }
}

#endif /* ENABLE_BOOT_SERVER_ON_HOST */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */


/******************* (C) COPYRIGHT 2013 STMicroelectronics *******************/
