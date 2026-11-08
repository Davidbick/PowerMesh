/**
  ******************************************************************************
  * @file    image_management.h
  * @author  AMG/IPC Application Team
  * @brief   Header file to define ST8500 image management macros/definitions.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef IMAGE_MANAGEMENT_H_
#define IMAGE_MANAGEMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>


/* Definitions */

/* Constants */
#define KB_SIZE 				1024

/* Sizes in image header */
#define DE_MAX_HEADER_SIZE      288
#define DE_MAX_BLOCK_SIZE       1024
#define DE_EUI64_ATTR_VAL_SIZE  8
#define DE_MAX_ATTR_VAL_SIZE    16
#define DE_APP_PARAM_SIZE       32
#define DE_AES_IV_SIZE          16
#define DE_AUTH_TAG_SIZE        16
#define DE_MAX_SECTION_NUM      24

/* Image partitioning */
#define IMAGE_CRC_SIZE				sizeof(uint16_t)
#define IMAGE_SLOTS_NUM         	4
#define IMAGE_SLOT_SIZE         	(262144) /* in bytes (256 kB) */
#define IMAGE_SIZE         			(IMAGE_SLOT_SIZE - IMAGE_CRC_SIZE) /* in bytes */
#define IMAGE_NOT_FOUND				0xFFFFFFFF

/* Images must be located at multiple addresses of IMAGE_SLOT_SIZE */
#define SFLASH_SLOT(x)							(x * IMAGE_SLOT_SIZE)

#define IMG_TYPE_IS_VALID(type)					(	(type == FW_PE_IMAGE	) || \
													(type == FW_RTE_IMAGE	) )

#define IMG_VALIDITY_IS_VALID(validity)			(	(validity == IMG_NOT_VALIDATED_YET	) || \
													(validity == IMG_VALIDATED_FIRST	) || \
													(validity == IMG_VALIDATED_SECOND	) )

#define IMG_VALIDITY_NOT_CORRUPTED(validity)	(	(validity == IMG_NOT_VALIDATED_YET	) || \
													(validity == IMG_VALIDATED_FIRST	) || \
													(validity == IMG_VALIDATED_SECOND	) || \
													(validity == IMG_INVALIDATED	    ) )

#define IMG_SIZE_IS_VALID(size)					(size < IMAGE_SLOT_SIZE)

/* Image header offsets*/
#define IMAGE_START_OFFSET		(0)
#define IMAGE_TYPE_OFFSET       (IMAGE_START_OFFSET + 0)
#define VALIDITY_STATUS_OFFSET  (IMAGE_START_OFFSET + 4)
#define FIRMWARE_SIZE_OFFSET    (IMAGE_START_OFFSET + 40)
#define SECTION_NUMBER_OFFSET   (IMAGE_START_OFFSET + 84)

/* Header values */
#define FW_PE_IMAGE             0x0000F1F1
#define FW_RTE_IMAGE            0x0000A3A3

#define IMG_NOT_VALIDATED_YET   0xFFFFFFFE
#define IMG_VALIDATED_FIRST     0xFFFFFFFC
#define IMG_VALIDATED_SECOND    0xFFFFFFF8
#define IMG_INVALIDATED         0xFFFFFFF0


/* Structures and Unions */
#pragma pack(push, 1)

/* Image Header */
typedef struct sect_header_struct
{
  uint32_t fw_dest_addr;
  uint32_t fw_section_size;
} sect_header_t;

typedef struct image_header_struct
{
  uint32_t              image_type;
  uint32_t              validity_status;
  uint8_t               app_params[DE_APP_PARAM_SIZE];
  uint32_t              fw_size;
  uint32_t              security_mode;
  uint8_t               aes_iv[DE_AES_IV_SIZE];
  uint8_t               auth_tag[DE_AUTH_TAG_SIZE];
  uint32_t              fw_entry_addr;
  uint32_t              sect_number;
  sect_header_t         sect[DE_MAX_SECTION_NUM];
} image_header_t;

#pragma pack(pop)

typedef struct slot_info_str
{
	bool     free;
	uint32_t address;
	uint32_t validity;
	uint32_t type;
	uint32_t size;
	uint16_t crc16;
} slot_info_t;

/* Functions */

/* Print utility */
const char* translateImageType(		uint32_t type);
const char* translateImageValidity(	uint32_t validity);

/* Image reading/management */
uint32_t getImageType(       uint32_t image_address);
uint32_t getImageValidity(   uint32_t image_address);
uint32_t getFwSize(          uint32_t image_address);
uint32_t getImageSectNum(    uint32_t image_address);
uint32_t getHeaderSize(      uint32_t image_address);
uint32_t getImageSize(       uint32_t image_address);
uint16_t getImageCRC(        uint32_t image_address);
bool     getImgHeader(       uint32_t image_address, image_header_t *header, uint16_t *header_size_ptr);
bool     getDataBlock(       uint32_t image_address, uint8_t *block, uint16_t block_size, uint32_t offset);
uint16_t calculateImageCRC(  uint32_t image_address, uint32_t image_size);

/* Image writing/programming */
bool prepareImageSlot(			uint32_t image_address);
bool setDataBlock(           	uint32_t image_address, uint8_t *block, uint16_t block_size, uint32_t offset);
bool setImageValidity(       	uint32_t image_address, uint32_t new_validity);
bool setImageCRC(            	uint32_t image_address, uint16_t crc16);

/* Validity management */
bool downgradeImageValidity( uint32_t image_address);
bool invalidateImageValidity(uint32_t image_address);
void regradeImagesInMemory(  uint32_t image_address, uint32_t validity);
bool scanForImagesToValidate(void);

bool findImagesToLoad(uint32_t *pe_img_address_ptr, uint32_t *rte_img_address_ptr, uint32_t validity);
void checkMemoryContent(slot_info_t *slot_vect);
bool eraseMemorySector(uint32_t slot_index);
bool eraseMemory(void);

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_MANAGEMENT_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
