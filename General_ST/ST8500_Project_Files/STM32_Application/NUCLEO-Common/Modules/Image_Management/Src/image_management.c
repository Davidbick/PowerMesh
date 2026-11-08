/**
  ******************************************************************************
  * @file    image_management.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains source code that implements ST8500 image
  *          management functions.
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
#include <debug_print.h>
#include <mem_pool.h>
#include <crc.h>
#include <utils.h>
#include <image_management.h>
#include <sflash.h>

/* Definitions */
#define CRC_BLOCK_SIZE	1024

/* Public Functions */

/**
  * @brief  Translate a ST8500 image type value to its name string.
  * @param  type ST8500 image type value to translate.
  * @retval String containing the ST8500 image type value name.
  */
const char* translateImageType(uint32_t type)
{
    switch(type)
    {
    case FW_PE_IMAGE:
        return "PE";
        break;
    case FW_RTE_IMAGE:
        return "RTE";
        break;
    default:
        return "unknown type";
        break;
    }
}

/**
  * @brief  Translate a ST8500 image validity value to its description string.
  * @param  validity ST8500 image validity value to translate.
  * @retval String containing the ST8500 image validity value description.
  */
const char* translateImageValidity(uint32_t validity)
{
    switch(validity)
    {
    case IMG_NOT_VALIDATED_YET:
        return "image yet to be validated";
        break;
    case IMG_VALIDATED_FIRST:
        return "valid first image";
        break;
    case IMG_VALIDATED_SECOND:
        return "valid second image";
        break;
    case IMG_INVALIDATED:
        return "invalid image";
        break;
    default:
        return "corrupted image";
        break;
    }
}

/**
  * @brief  Reads the SFLASH to get the image type.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The image type (FW_PE_IMAGE or FW_RTE_IMAGE).
  */
uint32_t getImageType(uint32_t image_address)
{
    uint32_t valueToGet;

    assert(SFLASH_READ((uint8_t*) &valueToGet, image_address + IMAGE_TYPE_OFFSET, sizeof(valueToGet)));

    return valueToGet;
}

/**
  * @brief  Reads the SFLASH to get the image validity.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The image validity (IMG_NOT_VALIDATED_YET,
  * IMG_VALIDATED_FIRST, IMG_VALIDATED_SECOND or IMG_INVALIDATED).
  */
uint32_t getImageValidity(uint32_t image_address)
{
    uint32_t valueToGet;

    assert(SFLASH_READ((uint8_t*) &valueToGet, image_address + VALIDITY_STATUS_OFFSET, sizeof(valueToGet)));

    return valueToGet;
}

/**
  * @brief  Reads the SFLASH to get the image validity.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The image size
  */
uint32_t getFwSize(uint32_t image_address)
{
    uint32_t fw_size;

    assert(SFLASH_READ((uint8_t*) &fw_size, image_address + FIRMWARE_SIZE_OFFSET, sizeof(fw_size)));

    /* Adds padding */
    if ((fw_size % 16) != 0)
    {
        fw_size += (fw_size % 16);
    }

    return fw_size;
}

/**
  * @brief  Reads the SFLASH to get the section number.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The number of sections.
  */
uint32_t getImageSectNum(uint32_t image_address)
{
    uint32_t valueToGet;

    assert(SFLASH_READ((uint8_t*) &valueToGet, image_address + SECTION_NUMBER_OFFSET, sizeof(valueToGet)));

    return valueToGet;
}

/**
  * @brief  Reads the SFLASH to calculate the image header size.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The image header size (in bytes).
  */
uint32_t getHeaderSize(uint32_t image_address)
{
    uint32_t header_size;
    uint32_t sect_num = getImageSectNum(image_address);

    header_size = sizeof(image_header_t) - ((DE_MAX_SECTION_NUM - sect_num)*sizeof(sect_header_t));

    /* Adds padding */
    if ((header_size % 16) != 0)
    {
        header_size += (header_size % 16);
    }

    return header_size;
}

/**
  * @brief  Reads the SFLASH to calculate the image size.
  * @param  image_address Address of the image in the SFLASH.
  * @retval The image size (in bytes).
  */
uint32_t getImageSize(uint32_t image_address)
{
    uint32_t image_size;
    uint32_t header_size = getHeaderSize(image_address);
    uint32_t fw_size     = getFwSize(image_address);

    image_size = header_size + fw_size;

    return image_size;
}

/**
  * @brief  Copies a image header from the SFLASH to a header structure.
  * @param  image_address Address of the image in the SFLASH.
  * @param  header Pointer to the header structure.
  * @param  header_size_ptr Pointer to the header size variable.
  * @retval 'true' if the SFLASH read operation is successful, 'false' otherwise.
  */
bool getImgHeader(uint32_t image_address, image_header_t *header, uint16_t *header_size_ptr)
{
    uint32_t header_size = getHeaderSize(image_address);

    *header_size_ptr = header_size;

    return SFLASH_READ((uint8_t*) header, image_address + IMAGE_START_OFFSET, header_size);
}

/**
  * @brief  Copies a data block of an image to a buffer.
  * @param  image_address Address of the image in the SFLASH.
  * @param  block Pointer to the buffer.
  * @param  block_size Size of the buffer.
  * @param  offset Offset of the data to copy (relative to 'image_address').
  * @retval 'true' if the SFLASH read operation is successful, 'false' otherwise.
  */
bool getDataBlock(uint32_t image_address, uint8_t *block, uint16_t block_size, uint32_t offset)
{
    return SFLASH_READ(block, image_address + IMAGE_START_OFFSET + offset, block_size);
}

/**
  * @brief  Calculates the image CRC16-CCITT of the image by using a limited buffer.
  * @param  image_address Address of the image in the SFLASH.
  * @param  image_size Size of the image.
  * @retval The CRC16-CCITT of the image.
  */
uint16_t calculateImageCRC(uint32_t image_address, uint32_t image_size)
{
    bool result;
    uint32_t block_size;
    uint32_t bytes_done    = 0;
    uint16_t crc_calc = CRC16_CCITT_START_VALUE; /* Needs to be CCITT */

    uint8_t *data_buffer = MEMPOOL_MALLOC(CRC_BLOCK_SIZE);

    /* Calculate checksum */
    while (bytes_done < image_size)
    {
        if ((image_size - bytes_done) >= CRC_BLOCK_SIZE)
        {
            block_size = CRC_BLOCK_SIZE;
        }
        else
        {
            block_size = image_size - bytes_done;
        }

        result = getDataBlock(image_address, data_buffer, block_size, bytes_done);

        /* Calculates the CRC16 as if the validity was always 0xFFFFFFFF (validity is not constant!) */
        if ( ((VALIDITY_STATUS_OFFSET - IMAGE_TYPE_OFFSET) >= (bytes_done)              ) &&
        	 ((VALIDITY_STATUS_OFFSET - IMAGE_TYPE_OFFSET) <  (bytes_done + block_size) ) )
        {
        	/* executed only for the validity field */
        	memset(&data_buffer[VALIDITY_STATUS_OFFSET - IMAGE_TYPE_OFFSET - bytes_done], 0xFF, sizeof(uint32_t));
        }

        if (result == true)
        {
            crc_calc = crc16_generic(data_buffer, block_size, crc_calc);
            bytes_done += block_size;
        }
        else
        {
            crc_calc = CRC16_CCITT_START_VALUE;
            break;
        }
    }

    MEMPOOL_FREE(data_buffer);

    return crc_calc;
}

/**
  * @brief  Erases an image slot in SFLASH memory.
  * @param  image_address Address of the image to erase in the SFLASH.
  * @retval 'true' if the SFLASH operations are successful, 'false' otherwise.
  */
bool prepareImageSlot(uint32_t image_address)
{
    return SFLASH_ERASE(image_address, IMAGE_SLOT_SIZE);
}

/**
  * @brief  Copies a data block of an image from a buffer.
  * @param  image_address Address of the image in the SFLASH.
  * @param  block Pointer to the buffer.
  * @param  block_size Size of the buffer.
  * @param  offset Offset of the data to copy (relative to 'image_address').
  * @retval 'true' if the SFLASH write operation is successful, 'false' otherwise.
  */
bool setDataBlock(uint32_t image_address, uint8_t *block, uint16_t block_size, uint32_t offset)
{
    return SFLASH_WRITE(image_address + IMAGE_START_OFFSET + offset, block, block_size);
}

/**
  * @brief  Changes the validity field of an image in SFLASH. The image must not be validated yet.
  * @param  image_address Address of the image in the SFLASH.
  * @param  new_validity Validity value to set.
  * @retval 'true' if the SFLASH operations are successful, 'false' otherwise.
  */
bool setImageValidity(uint32_t image_address, uint32_t new_validity)
{
    bool result = false;

    uint32_t validity;

    result = SFLASH_READ((uint8_t*) &validity, image_address + VALIDITY_STATUS_OFFSET, sizeof(validity));

    if (result == true)
    {
        if (validity == IMG_NOT_VALIDATED_YET)
        {
            validity = new_validity;
            result = SFLASH_WRITE(image_address + VALIDITY_STATUS_OFFSET, (uint8_t*) &validity, sizeof(validity));
        }
        else
        {
            result = false;
        }
    }

    return result;
}

/**
  * @brief  Changes the validity field of an image in SFLASH to secondary (must be primary).
  * @param  image_address Address of the image in the SFLASH.
  * @retval 'true' if the SFLASH write operation is successful, 'false' otherwise.
  */
bool downgradeImageValidity(uint32_t image_address)
{
    bool result = false;
    uint32_t validity = IMG_VALIDATED_SECOND;

    result = SFLASH_WRITE(image_address+VALIDITY_STATUS_OFFSET, (uint8_t*) &validity, sizeof(validity));

    return result;
}

/**
  * @brief  Changes the validity field of an image in SFLASH to invalid (must be primary or secondary).
  * @param  image_address Address of the image in the SFLASH.
  * @retval 'true' if the SFLASH write operation is successful, 'false' otherwise.
  */
bool invalidateImageValidity(uint32_t image_address)
{
    bool result = false;
    uint32_t validity = IMG_INVALIDATED;

    result = SFLASH_WRITE(image_address+VALIDITY_STATUS_OFFSET, (uint8_t*) &validity, sizeof(validity));

    return result;
}

/**
  * @brief  Checks for multiple primary images, down-grading primary images in excess.
  *         If the image at the specified address is primary, it is prioritized to
  *         remain the only primary. Otherwise, only the first image found in SFLASH
  *         is kept as primary image.
  * @param  image_address Address of the image that was just written in the SFLASH.
  * @param  validity Validity of the image that was just written in the SFLASH.
  * @retval None.
  */
void regradeImagesInMemory(uint32_t image_address, uint32_t validity)
{
    uint32_t type;
    uint32_t type_i;
    uint32_t validity_i;

    bool     found_primary_pe    = false;
    bool     found_primary_rte   = false;
    uint32_t address_primary_pe  = 0;
    uint32_t address_primary_rte = 0;

    /* Gives priority to new first image */
    if (validity == IMG_VALIDATED_FIRST)
    {
        type = getImageType(image_address);

        if (type == FW_PE_IMAGE)
        {
            found_primary_pe   = true;
            address_primary_pe = image_address;
        }
        else if (type == FW_RTE_IMAGE)
        {
            found_primary_rte   = true;
            address_primary_rte = image_address;
        }
    }

    /* For both PE and RTE, downgrade all other primary images, if detected */
    for (uint32_t i = 0; i < IMAGE_SLOTS_NUM; i++)
    {
    	image_address = SFLASH_SLOT(i);

        validity_i = getImageValidity(image_address);

        if (validity_i == IMG_VALIDATED_FIRST)
        {
            type_i = getImageType(image_address);

            if (type_i == FW_PE_IMAGE)
            {
            	/* For PE images */
                if (found_primary_pe == true)
                {
                    if (address_primary_pe != image_address)
                    {
                    	/* Downgrades to secondary */
                        downgradeImageValidity(image_address);
                    }
                }
                else
                {
                    found_primary_pe   = true;
                    address_primary_pe = image_address;
                }
            }
            else if (type_i == FW_RTE_IMAGE)
            {
            	/* For RTE images */
                if (found_primary_rte == true)
                {
                    if (address_primary_rte != image_address)
                    {
                        downgradeImageValidity(image_address);
                    }
                }
                else
                {
                    found_primary_rte   = true;
                    address_primary_rte = image_address;
                }
            }
        }
    }
}

/**
  * @brief  Checks for images to be validated, and sets them as primary or secondary,
  *         depending on the order.
  * @param  None
  * @retval 'true' if at least one modification occurred, 'false' otherwise.
  */
bool scanForImagesToValidate(void)
{
	uint32_t img_address;
	bool changed = false;

	/* Sets all images to be validated as primary */
	for (uint32_t i = 0; i < IMAGE_SLOTS_NUM; i++)
	{
		img_address = SFLASH_SLOT(i);

		if (getImageValidity(img_address) == IMG_NOT_VALIDATED_YET)
		{
			setImageValidity(img_address, IMG_VALIDATED_FIRST);
			changed = true;
		}
	}

	/* Downgrages all primary images in excess, in address order (no priority) */
	if (changed == true)
	{
		regradeImagesInMemory(0, IMG_VALIDATED_SECOND);
	}

	return changed;
}

/**
  * @brief  Finds the addresses of the PE and RTE images inside the SFLASH.
  * @param  pe_img_address_ptr Pointer to the address variable of the PE image.
  * @param  rte_img_address_ptr Pointer to the address variable of the RTE image.
  * @param  validity Validity value of the images to find
  *         (must be IMG_VALIDATED_FIRST or IMG_VALIDATED_SECOND).
  * @retval 'true' if both images were found, 'false' otherwise.
  */
bool findImagesToLoad(uint32_t *pe_img_address_ptr, uint32_t *rte_img_address_ptr, uint32_t validity)
{
	bool result = false;
	uint32_t img_address;

	if ((pe_img_address_ptr  != NULL) &&
		(rte_img_address_ptr != NULL) )
	{
		for (uint32_t i = 0; i < IMAGE_SLOTS_NUM; i++)
		{
			img_address = SFLASH_SLOT(i);

			if (getImageValidity(img_address) == validity)
			{
				if ((*pe_img_address_ptr       == IMAGE_NOT_FOUND) &&
					(getImageType(img_address) == FW_PE_IMAGE    ) )
				{
					*pe_img_address_ptr = img_address;
				}
				else if ((*rte_img_address_ptr      == IMAGE_NOT_FOUND) &&
						 (getImageType(img_address) == FW_RTE_IMAGE   ) )
				{
					*rte_img_address_ptr = img_address;
				}

				if ((*pe_img_address_ptr  != IMAGE_NOT_FOUND) &&
					(*rte_img_address_ptr != IMAGE_NOT_FOUND) )
				{
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

/**
  * @brief  Checks which slots are free and displays the SFLASH memory content.
  * @param  slot_vect Vector of slot info.
  * @retval None
  */
void checkMemoryContent(slot_info_t *slot_vect)
{
	assert_param(slot_vect != NULL);

	PRINT("  SFLASH memory content:\n");

	for (uint32_t i = 0; i < IMAGE_SLOTS_NUM; i++)
	{
		slot_vect[i].address = SFLASH_SLOT(i);

		slot_vect[i].type = getImageType(slot_vect[i].address);

		if (slot_vect[i].type != 0xFFFFFFFF)
		{
			slot_vect[i].free = false;

			slot_vect[i].validity = getImageValidity(slot_vect[i].address);

			if (IMG_VALIDITY_NOT_CORRUPTED(slot_vect[i].validity))
			{
				slot_vect[i].size = getImageSize(slot_vect[i].address);

				if (slot_vect[i].size <= IMAGE_SIZE)
				{
					slot_vect[i].crc16 = calculateImageCRC(slot_vect[i].address, slot_vect[i].size);
					PRINT("  > Slot %u: %s image (%s) - %u bytes, CRC16-CCITT: %04X.\n",
							i + 1,
							translateImageType(slot_vect[i].type),
							translateImageValidity(slot_vect[i].validity),
							slot_vect[i].size,
							slot_vect[i].crc16);
				}
				else
				{
					downgradeImageValidity(slot_vect[i].address);
					PRINT("  > Slot %u: invalid size of %u (corrupted).\n", i + 1, slot_vect[i].size);
				}
			}
			else
			{
				downgradeImageValidity(slot_vect[i].address);
				PRINT("  > Slot %u: %s.\n", i + 1, translateImageValidity(slot_vect[i].validity));
			}
		}
		else
		{
			slot_vect[i].free = true;
			PRINT("  > Slot %u free.\n", i + 1);
		}

	}
	PRINT_NOTS("\n");
}

/**
  * @brief  Erases a specific 256K slot of the SFLASH memory.
  * @param  slot_index Index of the 256K slot to erase.
  * @retval 'true' if the SFLASH write operation is successful, 'false' otherwise.
  */
bool eraseMemorySector(uint32_t slot_index)
{
	bool result = false;

	if (slot_index < IMAGE_SLOTS_NUM)
	{
		result = SFLASH_ERASE(SFLASH_SLOT(slot_index), IMAGE_SLOT_SIZE);
	}

	return result;
}

/**
  * @brief  Erases the entire SFLASH memory.
  * @param  None
  * @retval 'true' if the SFLASH write operation is successful, 'false' otherwise.
  */
bool eraseMemory(void)
{
	return SFLASH_BULK_ERASE();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
