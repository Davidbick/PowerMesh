/******************************************************************************
 * @file    rtos_config.c
 * @author  AMG/IPC Application Team
 * @brief   This file contains source code that implements the configuration of the RTOS.
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
#include <cmsis_os.h>
#include <task_comm.h>
#include <main.h>
#include <rtos_settings.h>

/* Definitions */
#define BINARY_SEM_MAX_VALUE		1
#define BINARY_SEM_FREE_AT_STARTUP	1
#define BINARY_SEM_BUSY_AT_STARTUP	0

/* Macros */
#define STR_VALUE(__arg)      		#__arg
#define CONVERT_TO_STRING(__name) 	STR_VALUE(__name)
#define HANDLE(__name)				__name ## Handle
#define BUFFER(__name)				__name ## Buffer
#define CONTROL_BLOCK(__name)		__name ## ControlBlock
#define ATTRIBUTES(__name)			__name ## _attributes
#define CALLBACK(__name)			__name ## Callback

/**
 * @brief Static allocation of a FreeRTOS task
 * @param task_name Name of the task to allocate, must be a string
 * @param stack_words Size of the stack dedicated to the task to allocate, in words (4 bytes each), must be an integer
 * @param task_priority Priority of the task to allocate, must be a osPriority_t value
 */
#define ALLOC_STATIC_THREAD(task_name, stack_words, task_priority)	osThreadId_t HANDLE(task_name);						\
																	uint32_t BUFFER(task_name)[stack_words]; 			\
																	osStaticThreadDef_t CONTROL_BLOCK(task_name);		\
																	const osThreadAttr_t ATTRIBUTES(task_name) = {		\
																		.name       = CONVERT_TO_STRING(task_name),		\
																		.stack_mem  = &BUFFER(task_name)[0],			\
																		.stack_size = sizeof(BUFFER(task_name)),		\
																		.cb_mem     = &CONTROL_BLOCK(task_name),		\
																		.cb_size    = sizeof(CONTROL_BLOCK(task_name)),	\
																		.priority   = (osPriority_t) task_priority,		\
																	}

/**
 * @brief Static allocation of a FreeRTOS queue
 * @param queue_name Name of the queue to allocate, must be a string
 * @param elem_number Maximum number of elements that can fit inside the queue to allocate, must be an integer
 * @param elem_size Size of the queue element, must be an integer
 */
#define ALLOC_STATIC_QUEUE(queue_name, elem_number, elem_size)		osMessageQueueId_t HANDLE(queue_name);					\
																	uint8_t BUFFER(queue_name)[elem_number*elem_size];		\
																	osStaticMessageQDef_t CONTROL_BLOCK(queue_name);		\
																	const osMessageQueueAttr_t ATTRIBUTES(queue_name) = {	\
																			.name = CONVERT_TO_STRING(queue_name),			\
																			.cb_mem = &CONTROL_BLOCK(queue_name),			\
																			.cb_size = sizeof(CONTROL_BLOCK(queue_name)),	\
																			.mq_mem = BUFFER(queue_name),					\
																			.mq_size = sizeof(BUFFER(queue_name))			\
																	}

/**
 * @brief Static allocation of a FreeRTOS timer
 * @param timer_name Name of the timer to allocate, must be a string
 */
#define ALLOC_STATIC_TIMER(timer_name)								osTimerId_t HANDLE(timer_name);							\
																	osStaticTimerDef_t CONTROL_BLOCK(timer_name);			\
																	const osTimerAttr_t ATTRIBUTES(timer_name) = {			\
																			.name = CONVERT_TO_STRING(timer_name),			\
																			.cb_mem = &CONTROL_BLOCK(timer_name),			\
																			.cb_size = sizeof(CONTROL_BLOCK(timer_name)),	\
																	}

/**
 * @brief Static allocation of a FreeRTOS mutex
 * @param mutex_name Name of the mutex to allocate, must be a string
 */
#define ALLOC_STATIC_MUTEX(mutex_name)								osMutexId_t HANDLE(mutex_name);							\
																	osStaticMutexDef_t CONTROL_BLOCK(mutex_name);			\
																	const osMutexAttr_t ATTRIBUTES(mutex_name) = {			\
																			.name = CONVERT_TO_STRING(mutex_name),			\
																			.cb_mem = &CONTROL_BLOCK(mutex_name),			\
																			.cb_size = sizeof(CONTROL_BLOCK(mutex_name)),	\
																	}

/**
 * @brief Static allocation of a FreeRTOS semaphore
 * @param sem_name Name of the semaphore to allocate, must be a string
 */
#define ALLOC_STATIC_SEMAPHORE(sem_name)							osSemaphoreId_t HANDLE(sem_name);						\
																	osStaticSemaphoreDef_t CONTROL_BLOCK(sem_name);			\
																	const osSemaphoreAttr_t ATTRIBUTES(sem_name) = {		\
																			.name = CONVERT_TO_STRING(sem_name),			\
																			.cb_mem = &CONTROL_BLOCK(sem_name),				\
																			.cb_size = sizeof(CONTROL_BLOCK(sem_name)),		\
																	}

/**
 * @brief Static allocation of a FreeRTOS event flag
 * @param ef_name Name of the event flag to allocate, must be a string
 */
#define ALLOC_STATIC_EVENT_FLAG(ef_name)							osEventFlagsId_t HANDLE(ef_name);					\
																	osStaticEventGroupDef_t CONTROL_BLOCK(ef_name);		\
																	const osEventFlagsAttr_t ATTRIBUTES(ef_name) = {	\
																			.name = CONVERT_TO_STRING(ef_name),			\
																			.cb_mem = &CONTROL_BLOCK(ef_name),			\
																			.cb_size = sizeof(CONTROL_BLOCK(ef_name)),	\
																	}

/**
 * @brief Initialization of a previously allocated static FreeRTOS task
 * @param task_name Name of the previously allocated static FreeRTOS task, must be a string
 * @param task_routine Function to be executed by the task
 */
#define CREATE_STATIC_THREAD(task_name, task_routine)						HANDLE(task_name) = osThreadNew(task_routine, NULL, &ATTRIBUTES(task_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS queue
 * @param queue_name Name of the previously allocated static FreeRTOS task, must be a string
 * @param elem_number Maximum number of elements that can fit inside the queue, must be an integer
 * @param elem_size Size of the queue element, must be an integer
 */
#define CREATE_STATIC_QUEUE(queue_name, elem_number, elem_size)				HANDLE(queue_name) = osMessageQueueNew(elem_number, elem_size, &ATTRIBUTES(queue_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS timer
 * @param timer_name Name of the previously allocated static FreeRTOS timer, must be a string
 * @param timer_type Type of the timer (one-shot or periodic), must be a osTimerType_t value
 */
#define CREATE_STATIC_TIMER(timer_name, timer_type)							HANDLE(timer_name) = osTimerNew(CALLBACK(timer_name), timer_type, NULL, &ATTRIBUTES(timer_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS mutex
 * @param mutex_name Name of the previously allocated static FreeRTOS mutex, must be a string
 */
#define CREATE_STATIC_MUTEX(mutex_name)										HANDLE(mutex_name) = osMutexNew(&ATTRIBUTES(mutex_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS semaphore as binary
 * @param sem_name Name of the previously allocated static FreeRTOS semaphore, must be a string
 * @param initial_value Initial value of the static FreeRTOS semaphore, must be a 0 (busy) or 1 (free)
 */
#define CREATE_STATIC_BINARY_SEMAPHORE(sem_name, initial_value)				HANDLE(sem_name) = osSemaphoreNew(BINARY_SEM_MAX_VALUE, initial_value, &ATTRIBUTES(sem_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS semaphore as counting
 * @param sem_name Name of the previously allocated static FreeRTOS semaphore, must be a string
 * @param count_total Total number of resources of the static FreeRTOS semaphore, must be an integer
 * @param count_free Number of free resources of the static FreeRTOS semaphore at startup, must be an integer, must be less than or equal to 'count_total'
 */
#define CREATE_STATIC_COUNTING_SEMAPHORE(sem_name, count_total, count_free)	HANDLE(sem_name) = osSemaphoreNew(count_total, count_free, &ATTRIBUTES(sem_name))

/**
 * @brief Initialization of a previously allocated static FreeRTOS event flag
 * @param ef_name Name of the previously allocated static FreeRTOS event flag, must be a string
 */
#define CREATE_STATIC_EVENT_FLAGS(ef_name)									HANDLE(ef_name) = osEventFlagsNew(&ATTRIBUTES(ef_name))

/* Custom types */
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTimer_t osStaticTimerDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;

/* Global variables */

/* Allocation of handles, control blocks and attributes */

/* Mutexes */
ALLOC_STATIC_MUTEX(mutexPrint);

/* Semaphores */
ALLOC_STATIC_SEMAPHORE(semHostIfTxComplete);
ALLOC_STATIC_SEMAPHORE(semUserIfTxComplete);
ALLOC_STATIC_SEMAPHORE(semStartPrint);
ALLOC_STATIC_SEMAPHORE(semConfirmation);
ALLOC_STATIC_SEMAPHORE(semSPI);

/* Timers */

/* G3 timers */
ALLOC_STATIC_TIMER(bootTimer);
#if ENABLE_ICMP_KEEP_ALIVE
ALLOC_STATIC_TIMER(kaTimer);
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
ALLOC_STATIC_TIMER(serverTimer);
#endif

/* User timers */
ALLOC_STATIC_TIMER(commTimer);
ALLOC_STATIC_TIMER(userTimeoutTimer);
#if ENABLE_IMAGE_TRANSFER
ALLOC_STATIC_TIMER(transferTimer);
#endif

/* Queues */
ALLOC_STATIC_QUEUE(host_if_queue,	HOST_IF_QUEUE_LENGTH,	HOST_IF_QUEUE_SIZE);
ALLOC_STATIC_QUEUE(g3_queue,		G3_QUEUE_LENGTH,		G3_QUEUE_SIZE);
ALLOC_STATIC_QUEUE(user_queue,		USER_QUEUE_LENGTH,		USER_QUEUE_SIZE);
ALLOC_STATIC_QUEUE(sflash_queue,	SFLASH_QUEUE_LENGTH,	SFLASH_QUEUE_SIZE);

/* Tasks/Threads  */
ALLOC_STATIC_THREAD(host_if_task,	HOST_IF_TASK_STACK_SIZE,	osPriorityHigh);
ALLOC_STATIC_THREAD(print_task,		PRINT_TASK_STACK_SIZE,		osPriorityLow);
ALLOC_STATIC_THREAD(sflash_task,	SFLASH_TASK_STACK_SIZE,		osPriorityBelowNormal);
ALLOC_STATIC_THREAD(g3_task,		G3_TASK_STACK_SIZE, 		osPriorityAboveNormal);
ALLOC_STATIC_THREAD(user_task,		USER_TASK_STACK_SIZE,		osPriorityNormal);

/* Event Flags */
ALLOC_STATIC_EVENT_FLAG(eventSync);

/* Private variables */

/* External variables */

/* External functions */

/* Task functions */
extern void start_default_task(void *argument);
extern void start_host_if_task(void *argument);
extern void start_print_task(void *argument);
extern void start_g3_task(void *argument);
extern void start_user_task(void *argument);
extern void start_sflash_task(void *argument);

/* Callbacks */

/* G3 timers */
extern void bootTimerCallback(void *argument);
#if ENABLE_ICMP_KEEP_ALIVE
extern void kaTimerCallback(void *argument);
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
extern void serverTimerCallback(void *argument);
#endif

/* User timers */
extern void commTimerCallback(void *argument);
extern void userTimeoutTimerCallback(void *argument);
#if ENABLE_IMAGE_TRANSFER
extern void transferTimerCallback(void *argument);
#endif

/* Public functions */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void FreeRTOS_DefaultInit(void)
{
	/* Mutexes */
	CREATE_STATIC_MUTEX(mutexPrint);

	/* Semaphores */
	CREATE_STATIC_BINARY_SEMAPHORE(semHostIfTxComplete, BINARY_SEM_FREE_AT_STARTUP);	/* Must start with count = 1 */
	CREATE_STATIC_BINARY_SEMAPHORE(semUserIfTxComplete, BINARY_SEM_BUSY_AT_STARTUP);	/* Must start with count = 0 */
	CREATE_STATIC_BINARY_SEMAPHORE(semStartPrint, 		BINARY_SEM_BUSY_AT_STARTUP);	/* Must start with count = 0 */
	CREATE_STATIC_BINARY_SEMAPHORE(semSPI, 				BINARY_SEM_BUSY_AT_STARTUP);	/* Must start with count = 0 */

	CREATE_STATIC_COUNTING_SEMAPHORE(semConfirmation, 	CONFIRMATION_SEMAPHORE_COUNT, CONFIRMATION_SEMAPHORE_COUNT);	/* Must start with count = 2 */

	/* Software Timers */
	CREATE_STATIC_TIMER(userTimeoutTimer,	osTimerOnce);
#if ENABLE_ICMP_KEEP_ALIVE
	CREATE_STATIC_TIMER(kaTimer, 			osTimerOnce);
#endif
	CREATE_STATIC_TIMER(bootTimer, 			osTimerOnce);
	CREATE_STATIC_TIMER(commTimer, 			osTimerOnce);
#if ENABLE_IMAGE_TRANSFER
	CREATE_STATIC_TIMER(transferTimer,		osTimerOnce);
#endif
#if IS_COORD && ENABLE_BOOT_SERVER_ON_HOST
	CREATE_STATIC_TIMER(serverTimer,		osTimerOnce);
#endif

	/* Queues */
	CREATE_STATIC_QUEUE(host_if_queue,	HOST_IF_QUEUE_LENGTH,	HOST_IF_QUEUE_SIZE);
	CREATE_STATIC_QUEUE(g3_queue,		G3_QUEUE_LENGTH,		G3_QUEUE_SIZE);
	CREATE_STATIC_QUEUE(user_queue,		USER_QUEUE_LENGTH,		USER_QUEUE_SIZE);
	CREATE_STATIC_QUEUE(sflash_queue,	SFLASH_QUEUE_LENGTH,	SFLASH_QUEUE_SIZE);

	/* Tasks */
	CREATE_STATIC_THREAD(host_if_task,	start_host_if_task);
	CREATE_STATIC_THREAD(print_task, 	start_print_task);
	CREATE_STATIC_THREAD(sflash_task,	start_sflash_task);
	CREATE_STATIC_THREAD(g3_task, 		start_g3_task);
	CREATE_STATIC_THREAD(user_task, 	start_user_task);

	/* Event Flags */
	CREATE_STATIC_EVENT_FLAGS(eventSync);
}

#if configGENERATE_RUN_TIME_STATS
void configureTimerForRunTimeStats(void){}

unsigned long getRunTimeCounterValue(void)
{
	return HAL_GetTick();
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
