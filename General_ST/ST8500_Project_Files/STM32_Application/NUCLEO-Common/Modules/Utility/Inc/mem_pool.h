/**
  ******************************************************************************
  * @file    mem_pool.h
  * @author  AMG/IPC Application Team
  * @brief   Header file for handling static memory pools.
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
  * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
  * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
  * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *******************************************************************************/

#ifndef MEM_POOL_H_
#define MEM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <settings.h>

/** @defgroup Memory_Pool_Utility Static memory pool
  * @{
  */

/* Definitions */
/* Memory pool numbers */
#define MEM_BLOCK_NUM_SMALL					16
#define MEM_BLOCK_NUM_MEDIUM       			8
#define MEM_BLOCK_NUM_BIG					4

/* Memory pool sizes */
#define MEM_BLOCK_SIZE_SMALL				32
#define MEM_BLOCK_SIZE_MEDIUM				512
#define MEM_BLOCK_SIZE_BIG					1536

/* Macros */
#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
#define MEMPOOL_MALLOC(size) 		mem_pool_alloc(size, __FILE__, __LINE__)		/* Allocates memory pool like "malloc", registers call location */
#define MEMPOOL_FREE(mem) 			mem = mem_pool_free(mem, __FILE__, __LINE__)	/* De-allocates memory pool like "free", registers call location */
#else
#define MEMPOOL_MALLOC(size) 		mem_pool_alloc(size)							/* Allocates memory pool like "malloc" */
#define MEMPOOL_FREE(mem) 			mem = mem_pool_free(mem)						/* De-allocates memory pool like "free" */
#endif

/* Public functions */
void mem_pool_init();

#if (MEMPOOL_DEBUG >= MEMPOOL_DEBUG_MAX)
void *mem_pool_alloc(const uint32_t mem_size, char *file, uint32_t line);
void *mem_pool_free(void *mem_address, char *file, uint32_t line);
#else
void *mem_pool_alloc(const uint32_t mem_size);
void *mem_pool_free(void *mem_address);
#endif
bool mem_pool_check(const void *mem_address);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* MEM_POOL_H_ */

/*********************** (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
