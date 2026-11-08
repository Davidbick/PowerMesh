/******************************************************************************
*   @file    g3_app_boot.c
*   @author  AMG/IPC Application Team
*   @brief   This code includes the state machines and the functions related to LoWPAN Bootstrap.
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
#include <debug_print.h>
#include <mem_pool.h>
#include <utils.h>
#include <g3_boot_access_tbl.h>
#include <g3_app_config.h>
#include <g3_app_boot.h>
#include <hi_msgs_impl.h>
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
#if DEBUG_G3_BOOT
#define HANDLE_CNF_ERROR(cnf_id, status) if (status != G3_SUCCESS) PRINT_G3_BOOT_CRITICAL("ERROR, received negative CNF (%u=%s) for %s\n", status, g3_app_translate_g3_result(status), translateG3cmd(cnf_id));
#else
#define HANDLE_CNF_ERROR(cnf_id, status)
#endif

/* Custom Types */

/* Private Variables */

/* Global variables */

/* External variables */
#if IS_COORD
extern boot_server_t boot_server;
#else
extern osTimerId_t bootTimerHandle;
#endif /* IS_COORD */

/* Private Functions */
#if !IS_COORD
/**
  * @brief Compares two elements inside a G3 PAN descriptor.
  * @param first Pointer to the first element of the G3 PAN descriptor.
  * @param second Pointer to the second element of the G3 PAN descriptor.
  * @return 1, 0 or -1 depending on the comparison (see 'PAN sorting criteria' in settings.h).
  */
static int g3_pansort_compare(const void *first, const void *second)
{
	int ret;
    const ADP_PanDescriptor_t *a = first;
    const ADP_PanDescriptor_t *b = second;

    uint16_t value_a, value_b, value_a2, value_b2;
#if PANSORT_CRITERION_PRIMARY == PANSORT_CRITERION_LINK_QUALITY
    value_a = a->lq;
    value_b = b->lq;
#elif PANSORT_CRITERION_PRIMARY == PANSORT_CRITERION_SHORT_ADDRESS
    value_a = a->lba_addr;
    value_b = b->lba_addr;
#elif PANSORT_CRITERION_PRIMARY == PANSORT_CRITERION_ROUTE_COST
    value_a = a->route_cost;
    value_b = b->route_cost;
#else
#error "A primary PAN sorting criterion must be specified"
#endif

    if (value_a < value_b)
	{
    	ret = PANSORT_ORDER_PRIMARY;
	}
	else if (value_a > value_b)
    {
		ret = -1 * PANSORT_ORDER_PRIMARY;
    }
    else
    {
#if PANSORT_CRITERION_SECONDARY == PANSORT_CRITERION_LINK_QUALITY
    	value_a2 = a->lq;
    	value_b2 = b->lq;
#elif PANSORT_CRITERION_SECONDARY == PANSORT_CRITERION_SHORT_ADDRESS
    	value_a2 = a->lba_addr;
    	value_b2 = b->lba_addr;
#elif PANSORT_CRITERION_SECONDARY == PANSORT_CRITERION_ROUTE_COST
    	value_a2 = a->route_cost;
    	value_b2 = b->route_cost;
#else
    	value_a2 = 0;
		value_b2 = 0;
#endif

    	if (value_a2 < value_b2)
		{
    		ret = PANSORT_ORDER_SECONDARY;
		}
		else if (value_a2 > value_b2)
		{
			ret = -1 * PANSORT_ORDER_SECONDARY;
		}
		else
		{
			ret = 0;
		}
    }

    return ret;
}
#endif /* IS_COORD */


#if IS_COORD
/**
  * @brief Handles the reception of a BOOT-SERVER-LEAVE indication.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_leave_ind(const void *payload)
{
	const BOOT_ServerLeaveIndication_t *leave_ind = payload;
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_FULL)
    ALLOC_STATIC_HEX_STRING(ext_addr_str, leave_ind->ext_addr, sizeof(leave_ind->ext_addr));
    PRINT_G3_BOOT_INFO("Leave IND from %s\n", ext_addr_str);
#endif
	if (!g3_app_boot_remove_connected_device(leave_ind->ext_addr))
	{
		PRINT_G3_BOOT_WARNING("Unregistered device left\n");
	}
}

/**
  * @brief Handles the reception of a BOOT-SERVER-KICK confirm.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_kick_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_CRITICAL)
	const BOOT_ServerKickConfirm_t *kick_cnf = payload;
	HANDLE_CNF_ERROR(HIF_BOOT_SRV_KICK_CNF, kick_cnf->status);
#else
	UNUSED(payload);
#endif
}

/**
  * @brief Handles the reception of a BOOT-SERVER-JOIN indication.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_join_ind(const void *payload)
{
	const BOOT_ServerJoinIndication_t *join_ind = payload;

	boot_device_t* device = g3_app_boot_find_device(join_ind->ext_addr, join_ind->short_addr, boot_state_connected);

	if (device == NULL)
	{
#if !ENABLE_BOOT_SERVER_ON_HOST
		g3_app_boot_add_connected_device(join_ind);
#endif
	}
	else
	{
		device->short_addr = join_ind->short_addr;
	}
}

#if ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Handles the reception of a BOOT-SERVER-REKEYING confirm.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_rekeying_cnf(const void *payload)
{
	const BOOT_ServerRekeyingConfirm_t *rekeying_cnf = payload;

	if ((rekeying_cnf->status != G3_SUCCESS) && (rekeying_cnf->error != rekeying_error_none))
	{
		PRINT_G3_BOOT_CRITICAL("Re-keying terminated with error: %u\n", rekeying_cnf->error);
	}
}

/**
  * @brief Handles the reception of a BOOT-SERVER-ABORT-RK-CNF confirm.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_abort_rekeying_cnf(const void *payload)
{
	const BOOT_ServerAbortRekeyingConfirm_t *abort_rk_cnf = payload;

	if (abort_rk_cnf->status != G3_SUCCESS)
	{
		PRINT_G3_BOOT_CRITICAL("Re-keying abort error: %u\n", abort_rk_cnf->status);
	}
}
#endif

/**
  * @brief Handles the reception of a BOOT-SERVER-GETPSK indication.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_getpsk_ind(const void *payload)
{
	const BOOT_ServerGetPSKIndication_t *getpsk_ind = payload;
	uint16_t len;

	BOOT_ServerSetPSKRequest_t	*setpsk_req = MEMPOOL_MALLOC(sizeof(BOOT_ServerSetPSKRequest_t));

#if (SELECTED_LIST_MODE == BLACK_LIST_MODE)
	uint8_t default_psk[ADP_EAP_PSK_ID_MAX_LEN] = DEFAULT_PSK;
#endif
	uint8_t  empty_psk[ADP_EAP_PSK_ID_MAX_LEN] = {0};
	uint8_t *psk_to_assign;

	uint8_t* psk = empty_psk;
	uint16_t short_address = MAC_BROADCAST_SHORT_ADDR;

#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_FULL)
    ALLOC_STATIC_HEX_STRING(idp_str, getpsk_ind->idp, getpsk_ind->idp_len);
    ALLOC_STATIC_HEX_STRING(ext_addr_str, getpsk_ind->ext_addr, sizeof(getpsk_ind->ext_addr));
    PRINT_G3_BOOT_INFO("Connecting device with IdP: %s, ext addr: %s\n", idp_str, ext_addr_str);
#endif

    /* Looks for the device with the given extended address in the table */
	g3_boot_data_t *access_table_entry = g3_boot_access_table_find(getpsk_ind->ext_addr);

#if (SELECTED_LIST_MODE == BLACK_LIST_MODE)
	if (access_table_entry == NULL)
#else
	if (access_table_entry != NULL)
#endif
	{
#if (SELECTED_LIST_MODE == BLACK_LIST_MODE)
		/* Uses default PSK and assigns short address based on the position index in the table (leave short_address = MAC_BROADCAST_SHORT_ADDR) */
		psk_to_assign = default_psk;
#else
		/* Uses short address and PSK values associated to the device, inside the table */
		psk_to_assign = access_table_entry->psk;
		short_address = access_table_entry->short_addr;
#endif

		/* Looks for the device in the connected entries of the connected device table, using the extended address */
		boot_device_t* boot_device = g3_app_boot_find_device(getpsk_ind->ext_addr, MAC_BROADCAST_SHORT_ADDR, boot_state_connected);

		/* If not found */
		if (boot_device == NULL)
		{
			/* Adds entry in the connected device table, so preserve a slot */
			boot_device = g3_app_boot_add_bootstrapping_device(getpsk_ind->ext_addr, short_address);

			/* If it was added */
			if (boot_device == NULL)
			{
				/* The connected device table is full, cannot connect */
				PRINT_G3_BOOT_CRITICAL("Error: connected device table is full\n");
			}
		}

		/* If added or found */
		if (boot_device != NULL)
		{
			/* Assigns the chosen PSK to the pointer used to fill the request */
			psk = psk_to_assign;
			/* Updates the short address with the one assigned inside g3_app_boot_add_bootstrapping_device */
			short_address = boot_device->short_addr;
		}
    }
    else
    {
    	/* Device found in black list or not found in white list */
# if (SELECTED_LIST_MODE == BLACK_LIST_MODE)
    	PRINT_G3_BOOT_WARNING("Forbidden: device not allowed to connect (black listed)\n");
#else
    	PRINT_G3_BOOT_WARNING("Forbidden: device not allowed to connect (not white listed)\n");
#endif
    }

#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_FULL)
	ALLOC_STATIC_HEX_STRING(psk_str, psk, ADP_EAP_PSK_KEY_LEN);
	PRINT_G3_BOOT_INFO("Sending PSK request for device %u using PSK: %s\n", short_address, psk_str);
#endif

	len = hi_boot_srvsetpskreq_fill(setpsk_req, getpsk_ind->ext_addr, psk, short_address);
	g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_SETPSK_REQ, setpsk_req, len);
}

/**
  * @brief Handles the reception of a BOOT-SERVER-SETPSK confirmation.
  * @param payload Pointer to the payload of the message structure.
  * @retval None
  */
static void g3_boot_handle_server_setpsk_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_CRITICAL)
	const BOOT_ServerSetPSKConfirm_t *setpsk_cnf = payload;
	HANDLE_CNF_ERROR(HIF_BOOT_SRV_SETPSK_CNF, setpsk_cnf->status);
#else
	UNUSED(payload);
#endif
}

#else

/**
  * @brief Function that handles the reception of a device start confirmation (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_handle_dev_start_cnf(const void *payload)
{
    const BOOT_DeviceStartConfirm_t *dev_start = payload;

    if (dev_start->status != G3_SUCCESS)
    {
    	HANDLE_CNF_ERROR(HIF_BOOT_DEV_START_CNF, dev_start->status);
    	PRINT_G3_BOOT_INFO("Retrying to connect in %u s\n", DEVICE_RECONN_TIME/configTICK_RATE_HZ);
		osTimerStart(bootTimerHandle, DEVICE_RECONN_TIME);
    }
}

/**
  * @brief Function that handles the reception of a device leave confirmation (device only).
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_handle_dev_leave_cnf(const void *payload)
{
    const BOOT_DeviceLeaveConfirm_t *dev_leave = payload;

    if (dev_leave->status != G3_SUCCESS)
    {
    	HANDLE_CNF_ERROR(HIF_BOOT_DEV_LEAVE_CNF, dev_leave->status);
    }
}

/**
  * @brief Function that handles the reception of a device leave indication (device only).
  * @param payload Unused (no payload)
  * @retval None
  */
static void g3_boot_handle_dev_leave_ind(const void *payload)
{
	UNUSED(payload);

	PRINT_G3_BOOT_INFO("Received leave indication\n");
}

/**
  * @brief Function that handles the reception of a PAN sort indication.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_handle_pansort_ind(const void *payload)
{
    const BOOT_DevicePANSortIndication_t *pansort_ind = payload;

    if (pansort_ind->pan_count > 0)
    {
    	BOOT_DevicePANSortRequest_t	*pan_sort_req = MEMPOOL_MALLOC(sizeof(BOOT_DevicePANSortRequest_t));

    	uint16_t len = hi_boot_devpansortreq_fill(pan_sort_req, pansort_ind);

    	/* Sorts the descriptors, following the criteria specified in settings.h */
        qsort(pan_sort_req->pan_descriptor, pan_sort_req->pan_count, sizeof(pan_sort_req->pan_descriptor[0]), g3_pansort_compare);

#if (DEBUG_G3_PANSORT >= DEBUG_LEVEL_FULL)
        ADP_PanDescriptor_t *descr;

        for (uint16_t i = 0; i < pan_sort_req->pan_count; i++)
        {
        	descr = &(pan_sort_req->pan_descriptor[i]);

        	PRINT_G3_PANSORT_INFO("\t%u. PAN ID: 0x%X - LQI %u, LBA address %u, route cost %u, media type %s.\n", i+1, descr->pan_id, descr->lq, descr->lba_addr, descr->route_cost, g3_app_pansort_translate_media_type(descr->media_type));
        }
#endif
		g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_PANSORT_REQ, pan_sort_req, len);
    }
}

/**
  * @brief Function that handles the reception of a PAN sort confirmation.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_handle_pansort_cnf(const void *payload)
{
#if (DEBUG_G3_PANSORT >= DEBUG_LEVEL_CRITICAL)
    const BOOT_DevicePANSortConfirm_t *pansort_cnf = payload;

    if (pansort_cnf->status != G3_SUCCESS)
    {
    	PRINT_G3_PANSORT_CRITICAL("ERROR, received negative CNF (%u=%s) for DEVICE PANSORT.\n", pansort_cnf->status, g3_app_translate_g3_result(pansort_cnf->status));
    }
#else
    UNUSED(payload);
#endif
}

/**
  * @brief Function that handles the reception of a HOSTIF-NVM.Confirm.
  * @param payload Pointer to the payload of the received message.
  * @retval None
  */
static void g3_boot_handle_nvm_cnf(const void *payload)
{
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_CRITICAL)
    const hif_nvm_cnf_t *nvm_cnf = payload;
    HANDLE_CNF_ERROR(HIF_HI_NVM_CNF, nvm_cnf->status);
#endif
}

#endif /* IS_COORD */

/**
  * @}
  */

/** @addtogroup G3_App_Boot_Exported_Code
  * @{
  */

/* Public functions */

#if IS_COORD
/**
  * @brief Function that finds the first device inside the connected device list with status within the given mask.
  * @param status_mask Bit mask status to use to find the device (can be the bitwise 'or' of multiple 'boot_conn_state_t' values).
  * @return Pointer to the device found, NULL if no matching device was found.
  */
boot_device_t* g3_app_boot_find_first_device(const boot_conn_state_t status_mask)
{
	boot_device_t *device = NULL;

	/* Find connected device */
	for (uint16_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		if (boot_server.connected_devices[i].conn_state & status_mask)
		{
			device = &boot_server.connected_devices[i];
			break;
		}
	}

	return device;
}

/**
  * @brief Function that finds the device inside the connected device list with the given extended address
  * 	   and/or the given short address and status within the given mask.
  * @param ext_addr Extended address of the device to find, use NULL to ignore this argument.
  * @param short_addr Short address of the device to find, use MAC_BROADCAST_SHORT_ADDR to ignore this argument.
  * @param status_mask Bit mask status to use to find the device (can be the bitwise 'or' of multiple 'boot_conn_state_t' values).
  * @return Pointer to the device found, NULL if no matching device was found.
  */
boot_device_t* g3_app_boot_find_device(const uint8_t* ext_addr, const uint16_t short_addr, const boot_conn_state_t state_mask)
{
	boot_device_t *device    = NULL;
	bool check_short_address = (short_addr != MAC_BROADCAST_SHORT_ADDR);
	bool check_ext_address   = (ext_addr != NULL);

	/* Find connected device */
	for (uint16_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		if (boot_server.connected_devices[i].conn_state & state_mask)
		{
			if ((check_ext_address) && (check_short_address))
			{
				/*Extended and Short address checks */
				if (	(memcmp(boot_server.connected_devices[i].ext_addr, ext_addr, MAC_ADDR64_SIZE) != 0) ||
						(boot_server.connected_devices[i].short_addr != short_addr)							)
				{
					continue; /* No match */
				}
			}
			else if (check_ext_address)
			{
				/* Extended address check only */
				if (memcmp(boot_server.connected_devices[i].ext_addr, ext_addr, MAC_ADDR64_SIZE) != 0)
				{
					continue; /* No match */
				}
			}
			else if (check_short_address)
			{
				/* Short address check only */
				if (boot_server.connected_devices[i].short_addr != short_addr)
				{
					continue; /* No match */
				}
			}

			/* If the program reaches here, the check was successful */
			device = &boot_server.connected_devices[i];
			break;
		}
	}

	return device;
}

/**
  * @brief Function that finds the device inside the connected device list with status within the given mask
  * 	   at a given position, ignoring the devices with status outside the mask.
  * @param index The position of the device to find
  * @param status_mask Bit mask status to use to find the device (can be the bitwise 'or' of multiple 'boot_conn_state_t' values).
  * @return Pointer to the device found, NULL if no matching device was found.
  */
boot_device_t* g3_app_boot_get_device_by_index(const uint16_t index, const boot_conn_state_t state_mask)
{
	boot_device_t *device = NULL;
	uint16_t next_starting_index = 0;

	if (index < BOOT_MAX_NUM_JOINING_NODES)
	{
		for (uint16_t i = 0; i <= index; i++)
		{
			for (uint16_t j = next_starting_index; j < BOOT_MAX_NUM_JOINING_NODES; j++)
			{
				if (boot_server.connected_devices[j].conn_state & state_mask)
				{
					next_starting_index = j + 1;
					if (i == index)
					{
						device = &boot_server.connected_devices[j];
					}
					break;
				}
			}
		}
	}

	return device;
}

/**
  * @brief Function that adds a new device (in bootstrapping state) to the connected device list.
  * @param ext_addr Extended address of the device to add, use NULL to ignore this argument.
  * @param short_addr Short address of the device to add, use MAC_BROADCAST_SHORT_ADDR to assign the position value as short address.
  * @return Pointer to the device added, NULL if the device could not be added (table full).
  */
boot_device_t* g3_app_boot_add_bootstrapping_device(const uint8_t* ext_addr, uint16_t short_address)
{
	/* Looks for the device in the disconnected entries of the connected device table, using the extended address */
	boot_device_t* boot_device = g3_app_boot_find_device(ext_addr, MAC_BROADCAST_SHORT_ADDR, boot_state_disconnected);

	if (boot_device == NULL)
	{
		/* For new devices finds the first empty entry */
		boot_device = g3_app_boot_find_first_device(boot_state_disconnected);
	}

	/* CHecks if a suitable disconnected entry was found */
	if (boot_device != NULL)
	{
		/* If the short_address is set to broadcast, replaces it with the index of the slot found + 1 */
		if (short_address == MAC_BROADCAST_SHORT_ADDR)
		{
			short_address = (uint16_t) (boot_device - boot_server.connected_devices) + 1;
		}

		/* Sets the found entry to bootstrapping state, and sets ext. and short addresses, increments the number of conn. devices */
		boot_device->conn_state = boot_state_bootstrapping;
		boot_device->short_addr = short_address;
		memcpy(boot_device->ext_addr, ext_addr, MAC_ADDR64_SIZE);

		/* Increments the number of conn. devices */
		boot_server.connected_devices_number++;
	}

	return boot_device;
}

#if ENABLE_BOOT_SERVER_ON_HOST
/**
  * @brief Function that changes a bootstrapping device to a connected device inside the connected device list (with Boot Server on host).
  * @param ext_addr Extended address of the device to change.
  * @param short_addr Short address of the device to change.
  * @param media_type Media type value of the device to change.
  * @param disable_bkp Disable backup value of the device to change.
  * @return Pointer to the device changed, NULL if the device could not be found.
  */
boot_device_t* g3_app_boot_add_connected_device(boot_join_entry_t* join)
#else
/**
  * @brief Function that changes a bootstrapping device to a connected device inside the connected device list (with Boot Server on platform).
  * @param ext_addr Extended address of the device to change.
  * @param short_addr Short address of the device to change.
  * @return Pointer to the device changed, NULL if the device could not be found.
  */
boot_device_t* g3_app_boot_add_connected_device(const BOOT_ServerJoinIndication_t* join)
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
{
	/* The device must be in the bootstrapping state */
	boot_device_t* boot_device = g3_app_boot_find_device(join->ext_addr, join->short_addr, boot_state_bootstrapping);

	/* Add new connection for completed bootstrap */

	if (boot_device != NULL)
	{
		/* Extended address and short address are already set at the reception of the GETPSK indication */
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_FULL)
		PRINT_G3_BOOT_INFO("Connected device %u\n", join->short_addr);
#endif
		boot_device->conn_state		= boot_state_connected;
#if ENABLE_ICMP_KEEP_ALIVE
		boot_device->lives			= KEEP_ALIVE_LIVES_N;
		boot_device->last_ka_ts		= HAL_GetTick();
#endif /* ENABLE_ICMP_KEEP_ALIVE */
#if ENABLE_BOOT_SERVER_ON_HOST
		boot_device->media_type		= join->media_type;
		boot_device->disable_bkp	= join->disable_bkp;
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
	}

	return boot_device;
}

/**
  * @brief Function that removes a device (bootstrapping or connected) from the connected device list.
  * @param ext_addr Extended address of the device to remove.
  * @return True if the device was found and removed, false if the device could not be found.
  */
bool g3_app_boot_remove_connected_device(const uint8_t* ext_addr)
{
	bool removed = false;
	boot_device_t *device = g3_app_boot_find_device(ext_addr, MAC_BROADCAST_SHORT_ADDR, boot_state_bootstrapping | boot_state_connected);

	if (device != NULL)
	{
#if (DEBUG_G3_BOOT >= DEBUG_LEVEL_FULL)
		PRINT_G3_BOOT_INFO("Disconnected device %u\n", device->short_addr);
#endif
		/* Sets the found entry to disconnected state */
		device->conn_state = boot_state_disconnected;

		/* Decrements the number of connected devices */
		boot_server.connected_devices_number--;

		removed = true;
	}

	return removed;
}

/**
  * @brief Sends a kick request to the Boot Server to kick out a device.
  * @param device_data Pointer to the device to remove.
  * @retval None
  */
void g3_app_boot_kick_device(boot_device_t *device)
{
    assert(device != NULL);

    PRINT_G3_BOOT_INFO("Kicking out device %u\n", device->short_addr);

    BOOT_ServerKickRequest_t *boot_kick_req = MEMPOOL_MALLOC(sizeof(BOOT_ServerKickRequest_t));

    uint16_t len = hi_boot_srvkickreq_fill(boot_kick_req, device);
	g3_send_message(BOOT_SERVER_MSG_TYPE, HIF_BOOT_SRV_KICK_REQ, boot_kick_req, len);

#if !ENABLE_BOOT_SERVER_ON_HOST
	g3_app_boot_remove_connected_device(device->ext_addr);
#endif
}

#else

/**
  * @brief Function that translates the media type of the PAN sort indication to a string.
  * @param media Media type variable to translate.
  * @return Pointer to a string representing the media type.
  */
char* g3_app_pansort_translate_media_type(uint8_t media)
{
	switch(media)
	{
	case ADP_MEDIATYPE_PLC:	return "PLC";
	case ADP_MEDIATYPE_RF:	return "RF";
	default:				return "unknown";
	}
}

/**
  * @brief Makes the device leave the PAN.
  * @param None
  * @retval None
  */
void g3_app_boot_leave()
{
    PRINT_G3_BOOT_INFO("Leaving PAN...\n");
    g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_LEAVE_REQ, NULL, 0);
}

/**
  * @brief Makes the device re-join the PAN.
  * @param None
  * @retval None
  */
void g3_app_boot_restart()
{
    PRINT_G3_BOOT_INFO("Re-connectiong to PAN...\n");

    BOOT_DeviceStartRequest_t *boot_start_req = MEMPOOL_MALLOC(sizeof(BOOT_DeviceStartRequest_t));

    uint16_t len = hi_boot_devstartreq_fill(boot_start_req, BOOT_START_NORMAL);
	g3_send_message(BOOT_CLIENT_MSG_TYPE, HIF_BOOT_DEV_START_REQ, boot_start_req, len);
}
#endif /* IS_COORD */

/**
  * @brief Initializes the G3 Boot application.
  * @param None
  * @retval None
  */
void g3_app_boot_init(void)
{
#if IS_COORD
	/* Boot access table initialization */
	g3_boot_access_table_init();

	/* Connected device list initialization */
	boot_server.connected_devices_number 	= 0;

	for (uint16_t i = 0; i < BOOT_MAX_NUM_JOINING_NODES; i++)
	{
		boot_server.connected_devices[i].conn_state = boot_state_disconnected;
#if ENABLE_ICMP_KEEP_ALIVE
		boot_server.connected_devices[i].lives = 0;
		boot_server.connected_devices[i].last_ka_ts = 0;
#endif /* ENABLE_ICMP_KEEP_ALIVE */
		boot_server.connected_devices[i].short_addr = MAC_BROADCAST_SHORT_ADDR;
		memset(boot_server.connected_devices[i].ext_addr, 0, MAC_ADDR64_SIZE);
#if ENABLE_BOOT_SERVER_ON_HOST
		boot_server.connected_devices[i].media_type = 0;
		boot_server.connected_devices[i].disable_bkp = 0;
#endif /* ENABLE_BOOT_SERVER_ON_HOST */
	}
#endif
}

/**
  * @brief Checks if a message is needed by the G3 Boot application.
  * @param g3_msg Pointer to the G3 message structure to evaluate
  * @return 'true' if the message is needed by the boot application, 'false' otherwise.
  */
bool g3_app_boot_msg_needed(const g3_msg_t *g3_msg)
{
	bool ret = false;

	switch (g3_msg->command_id)
	{
#if IS_COORD
	case HIF_BOOT_SRV_LEAVE_IND:
    case HIF_BOOT_SRV_KICK_CNF:
    case HIF_BOOT_SRV_JOIN_IND:
#if ENABLE_BOOT_SERVER_ON_HOST
	case HIF_BOOT_SRV_REKEYING_CNF:
	case HIF_BOOT_SRV_ABORT_RK_CNF:
#endif
	case HIF_BOOT_SRV_GETPSK_IND:
	case HIF_BOOT_SRV_SETPSK_CNF:
#else
	case HIF_HI_NVM_CNF:
	case HIF_BOOT_DEV_START_CNF:
	case HIF_BOOT_DEV_LEAVE_CNF:
	case HIF_BOOT_DEV_LEAVE_IND:
    case HIF_BOOT_DEV_PANSORT_IND:
    case HIF_BOOT_DEV_PANSORT_CNF:
#endif /* IS_COORD */
    	ret = true;
		break;
	default:
		break;
	}

	return ret;
}

/**
  * @brief Handles the reception of a G3 Boot application message.
  * @param g3_msg Pointer to the G3 message structure
  * @retval None
  */
void g3_app_boot_msg_handler(const g3_msg_t *g3_msg)
{
    switch (g3_msg->command_id)
    {
#if IS_COORD
	/* Messages that are always received by the coordinator */
    case HIF_BOOT_SRV_LEAVE_IND:
    	g3_boot_handle_server_leave_ind(g3_msg->payload);
    	break;
    case HIF_BOOT_SRV_KICK_CNF:
    	g3_boot_handle_server_kick_cnf(g3_msg->payload);
    	break;
    case HIF_BOOT_SRV_JOIN_IND:
        g3_boot_handle_server_join_ind(g3_msg->payload);
    	break;
#if ENABLE_BOOT_SERVER_ON_HOST
	case HIF_BOOT_SRV_REKEYING_CNF:
		g3_boot_handle_server_rekeying_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_SRV_ABORT_RK_CNF:
		g3_boot_handle_server_abort_rekeying_cnf(g3_msg->payload);
		break;
#endif
    case HIF_BOOT_SRV_GETPSK_IND:
        g3_boot_handle_server_getpsk_ind(g3_msg->payload);
        break;
    case HIF_BOOT_SRV_SETPSK_CNF:
        g3_boot_handle_server_setpsk_cnf(g3_msg->payload);
        break;
#else
	/* Messages that are always received by the device */
	case HIF_HI_NVM_CNF:
		g3_boot_handle_nvm_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_START_CNF:
		g3_boot_handle_dev_start_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_CNF:
		g3_boot_handle_dev_leave_cnf(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_LEAVE_IND:
		g3_boot_handle_dev_leave_ind(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_PANSORT_IND:
		g3_boot_handle_pansort_ind(g3_msg->payload);
		break;
	case HIF_BOOT_DEV_PANSORT_CNF:
		g3_boot_handle_pansort_cnf(g3_msg->payload);
		break;
#endif /* IS_COORD */
    default:
        break;
    }
}

#if !IS_COORD
/**
  * @brief Callback function of the bootTimer FreeRTOStimer for the PAN Device. Triggers the reconnection
  * @param argument Unused argument.
  * @retval None
  */
void g3_app_boot_dev_timeoutCallback(void *argument)
{
	UNUSED(argument);

	g3_app_boot_restart();
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
