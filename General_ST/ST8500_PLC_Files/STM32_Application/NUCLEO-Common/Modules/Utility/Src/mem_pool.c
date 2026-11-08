/**
  ******************************************************************************
  * @file    mem_pool.c
  * @author  AMG/IPC Application Team
  * @brief   Static memory pools implementation.
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
#include <stdbool.h>
#include <main.h>
#include <mem_pool.h>

/** @addgroup Memory_Pool_Utility
  * @{
  */

/* Definitions */

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
#define FILENAME_LEN_MAX					24
#endif

/* Special size value */
#define MEM_BLOCK_FREE						-1

/* Macros */
#define IS_BLOCK(block_address, array)		(	((((uint32_t) block_address) - ((uint32_t) array)) % sizeof(array[0]) == 0)	\
											 &&   ((uint32_t) block_address >= ((uint32_t) array))							\
											 &&   ((uint32_t) block_address <= ((uint32_t) array + sizeof(array) -1))		)

/* Custom types */

/* Block header */
typedef struct mem_block_info_str
{
	int32_t used_size; /* If different from MEM_BLOCK_FREE, the memory pool is used and the value is the occupied size. Always use 4 bytes alignment */
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
	char 	 *alloc_file;
	uint32_t alloc_line;
	char 	 *free_file;
	uint32_t free_line;
#endif
} mem_block_info_t;

/* All blocks must have the same header as the small block */

/* Small block */
typedef struct mem_block_s_str
{
	mem_block_info_t 	info;
	uint8_t 			buffer[MEM_BLOCK_SIZE_SMALL];
} mem_block_s_t;

/* Medium block */
typedef struct mem_block_m_str
{
	mem_block_info_t 	info;
	uint8_t 			buffer[MEM_BLOCK_SIZE_MEDIUM];
} mem_block_m_t;

/* Big block */
typedef struct mem_block_b_str
{
	mem_block_info_t 	info;
	uint8_t 			buffer[MEM_BLOCK_SIZE_BIG];
} mem_block_b_t;

/* Generic block */
typedef union mem_block_uni
{
	mem_block_s_t s;
	mem_block_m_t m;
	mem_block_b_t b;
} mem_block_t;

/* Memory pool definition */
typedef struct mem_pool_str
{
	mem_block_s_t small[MEM_BLOCK_NUM_SMALL];
	mem_block_m_t medium[MEM_BLOCK_NUM_MEDIUM];
	mem_block_b_t big[MEM_BLOCK_NUM_BIG];
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_LOW)
	uint32_t malloc_count;
	uint32_t free_count;
#endif
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_HIGH)
	uint8_t small_used;
	uint8_t medium_used;
	uint8_t big_used;
	uint8_t small_record;
	uint8_t medium_record;
	uint8_t big_record;
#endif
} mem_pool_t;


/* Memory pool instance */
static mem_pool_t mem_pool;

/* Private functions */
#pragma GCC push_options
#pragma GCC optimize("-Ofast")

/**
  * @brief  Function that finds a memory block of at least the given size.
  * @param  [in] mem_size The minimum required size for the block.
  * @retval Pointer to the found memory block (NULL if not found)
  */
static mem_block_t* mem_pool_find_block(const uint32_t mem_size)
{
	mem_block_t* block = NULL;
	uint32_t i;

	assert(mem_size <= MEM_BLOCK_SIZE_BIG);

	if (mem_size <= MEM_BLOCK_SIZE_SMALL)
	{
		for (i = 0; i < MEM_BLOCK_NUM_SMALL; i++)
		{
			if (mem_pool.small[i].info.used_size == MEM_BLOCK_FREE)
			{
				mem_pool.small[i].info.used_size = mem_size;
				block = (mem_block_t*) &(mem_pool.small[i]);
				break;
			}
		}

#if !USE_BIGGER_POOL_IF_NEEDED
		if (block == NULL)
		{
			return NULL;
		}
#endif
	}

	if ((block == NULL) && (mem_size <= MEM_BLOCK_SIZE_MEDIUM))
	{
		for (i = 0; i < MEM_BLOCK_NUM_MEDIUM; i++)
		{
			if (mem_pool.medium[i].info.used_size == MEM_BLOCK_FREE)
			{
				mem_pool.medium[i].info.used_size = mem_size;
				block = (mem_block_t*) &(mem_pool.medium[i]);
				break;
			}
		}

#if !USE_BIGGER_POOL_IF_NEEDED
		if (block == NULL)
		{
			return NULL;
		}
#endif
	}

	if ((block == NULL) && (mem_size <= MEM_BLOCK_SIZE_BIG))
	{
		for (i = 0; i < MEM_BLOCK_NUM_BIG; i++)
		{
			if (mem_pool.big[i].info.used_size == MEM_BLOCK_FREE)
			{
				mem_pool.big[i].info.used_size = mem_size;
				block = (mem_block_t*) &(mem_pool.big[i]);
				break;
			}
		}

#if !USE_BIGGER_POOL_IF_NEEDED
		if (block == NULL)
		{
			return NULL;
		}
#endif
	}

	return block;
}

/**
  * @brief  Function that checks if pointer is pointing a memory block.
  * @param  [in] block The pointer to check.
  * @retval True if the pointer is pointing a memory block, false otherwise
  */
static bool mem_pool_is_block(const mem_block_t* block)
{
	bool is_block = false;

	if ((IS_BLOCK(block, mem_pool.small)) || (IS_BLOCK(block, mem_pool.medium)) || (IS_BLOCK(block, mem_pool.big)))
	{
		is_block = true;
	}

	return is_block;
}

/* Public functions */

/**
  * @brief    This function is used to initialize the memory pool.
  * @return   None.
  */
void mem_pool_init()
{
	memset(&mem_pool, 0, sizeof(mem_pool));

	for (uint32_t i = 0; i < MEM_BLOCK_NUM_SMALL; i++)
	{
		mem_pool.small[i].info.used_size = MEM_BLOCK_FREE;
	}

	for (uint32_t i = 0; i < MEM_BLOCK_NUM_MEDIUM; i++)
	{
		mem_pool.medium[i].info.used_size = MEM_BLOCK_FREE;
	}

	for (uint32_t i = 0; i < MEM_BLOCK_NUM_BIG; i++)
	{
		mem_pool.big[i].info.used_size = MEM_BLOCK_FREE;
	}
}

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
/**
  * @brief    This function is used to allocate a memory buffer of given size
  * @param    mem_size Minimum required size of the buffer to allocate
  * @param    file File name string of the call location
  * @param    line Number of the line of the call location
  * @return   Pointer to the memory block buffer
  */
void *mem_pool_alloc(const uint32_t mem_size, char *file, uint32_t line)
#else
/**
  * @brief    This function is used to allocate a memory block buffer of given size
  * @param    mem_size Minimum required size of the buffer to allocate
  * @return   Pointer to the memory block buffer
  */
void *mem_pool_alloc(const uint32_t mem_size)
#endif
{
	void *mem_address = NULL;

	mem_block_t* block = mem_pool_find_block(mem_size);

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MEDIUM)
	assert(mem_pool_is_block(block));
#endif
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_LOW)
	mem_pool.malloc_count++;
#endif
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
	block->s.info.alloc_file = file;
	block->s.info.alloc_line = line;
#endif

	if (block != NULL)
	{
		mem_address = (void*) (block->s.buffer);

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_HIGH)
		if (IS_BLOCK(block, mem_pool.big))
		{
			mem_pool.big_used++;
			mem_pool.big_record = (mem_pool.big_used > mem_pool.big_record) ? mem_pool.big_used : mem_pool.big_record;
		}
		else if (IS_BLOCK(block, mem_pool.medium))
		{
			mem_pool.medium_used++;
			mem_pool.medium_record = (mem_pool.medium_used > mem_pool.medium_record) ? mem_pool.medium_used : mem_pool.medium_record;
		}
		else if (IS_BLOCK(block, mem_pool.small))
		{
			mem_pool.small_used++;
			mem_pool.small_record = (mem_pool.small_used > mem_pool.small_record) ? mem_pool.small_used : mem_pool.small_record;
		}
		else
		{
			Error_Handler();
		}
#endif
	}
	else
	{
		Error_Handler();
	}

	return mem_address;
}

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
/**
  * @brief    This function is used to free an allocated memory block.
  * @param    mem_address Pointer to the memory block buffer to be freed.
  * @param    file File name string of the call location
  * @param    line Number of the line of the call location
  * @return   NULL, if the block was freed, 'mem_address' otherwise.
  */
void *mem_pool_free(void *mem_address, char *file, uint32_t line)
#else
/**
  * @brief    This function is used to free an allocated memory block.
  * @param    [in] mem_address The pointer to the memory block buffer to be freed.
  * @return   NULL, if the block was freed, 'mem_address' otherwise.
  */
void *mem_pool_free(void *mem_address)
#endif
{
	void *ret = mem_address;

	mem_block_t* block = (mem_block_t*) (mem_address - sizeof(mem_block_info_t));

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MEDIUM)
	assert(mem_pool_is_block(block));
#endif
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_LOW)
	mem_pool.free_count++;
#endif
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
	block->s.info.free_file = file;
	block->s.info.free_line = line;
#endif

	if (block->s.info.used_size != MEM_BLOCK_FREE)
	{
		block->s.info.used_size = MEM_BLOCK_FREE;
		ret = NULL;

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_HIGH)
		if (IS_BLOCK(block, mem_pool.big))
		{
			mem_pool.big_used--;
		}
		else if (IS_BLOCK(block, mem_pool.medium))
		{
			mem_pool.medium_used--;
		}
		else if (IS_BLOCK(block, mem_pool.small))
		{
			mem_pool.small_used--;
		}
		else
		{
			Error_Handler();
		}
#endif
	}
	else
	{
		Error_Handler();
	}

	return ret;
}

/**
  * @brief    This function is used to check if at a given address a pool is allocated.
  * @param    [in] mem_address Address to check.
  * @return   True if a memory pool is allocated at the given address, false otherwise.
  */
bool mem_pool_check(const void *mem_address)
{
	mem_block_t* block = (mem_block_t*) (mem_address - sizeof(mem_block_info_t));

	return mem_pool_is_block(block);
}

#pragma GCC pop_options

/**
  * @}
  */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

