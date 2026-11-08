/**
  ******************************************************************************
  * @file    modbus_task.c
  * @author  AMG/IPC Application Team
  * @brief   This file contains code that implements the task that handles ModBus
  *          communication.
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
#include <string.h>
#include <stdbool.h>
#include <cmsis_os.h>
#include <assert.h>
#include <utils.h>
#include <modbus.h>
#include <user_modbus.h>
#include <settings.h>

/** @addtogroup Modbus_App
  * @{
  */

/** @addtogroup Modbus_Task
  * @{
  */

/** @addtogroup Modbus_Task_Private_Code
  * @{
  */

#if ENABLE_MODBUS

/* Definitions */
#define MODBUS_TEST_MSG_N				10		/* Number of modbus queries for the test */

#define MODBUS_REGISTER_N				16		/* Number of modbus registers (16 bit each) */

/* Timing */
#define MODBUS_TEST_INTERVAL			10		/* In ms */

/* Private variables */
static modbusHandler_t	modbus_handler;
static uint16_t 		modbus_registers[MODBUS_REGISTER_N];

static char* modbus_translate_error(mb_errot_t error)
{
	switch(error)
	{
	case ERR_NOT_MASTER: return "NOT MASTER";
	case ERR_POLLING: return "POLLING";
	case ERR_BUFF_OVERFLOW: return "BUFF OVERFLOW";
	case ERR_BAD_CRC: return "BAD CRC";
	case ERR_EXCEPTION: return "EXCEPTION";
	case ERR_BAD_SIZE: return "BAD SIZE";
	case ERR_BAD_ADDRESS: return "BAD ADDRESS";
	case ERR_TIME_OUT: return "TIMEOUT";
	case ERR_BAD_SLAVE_ID: return "BAD SLAVE ID";
	case ERR_OK_QUERY: return "OK";
	default: return "UNKNOWN";
	}
}

static mb_errot_t modbus_request(uint8_t dest_id, mb_functioncode_t operation, uint16_t address, uint16_t size, uint16_t* data)
{
	uint32_t notification = ERR_NOT_MASTER;
	modbus_t telegram;

	telegram.u8id = dest_id;		// slave address
	telegram.u8fct = operation;		// function code
	telegram.u16RegAdd = address;	// start address in slave
	telegram.u16CoilsNo = size;		// number of elements (coils or registers) to read
	telegram.u16reg = data; 		// pointer to a memory array in the Arduino

	if (ModbusQuery(&modbus_handler, telegram)) // make a query
	{
		notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
#if 0
		if (notification == 0)
		{
			notification = ERR_TIME_OUT;
		}
#endif
	}

	return notification;
}



/**
  * @}
  */

/** @addtogroup Modbus_Task_Exported_Code
  * @{
  */

/* External variables */


/**
  * @brief This is the initialization of the modbus module.
  * @param role Role of the Modbus communication (Master or Slave)
  * @retval None
  * @note Two tasks, two timers, one queue and one semaphore are allocated on the OS (see "ModbusInit")
  */
void UserModbus_init(modbus_role_t role)
{
	modbus_handler.port =  &huartModbus;

#if ENABLE_MODBUS_USART_DMA
	modbus_handler.xTypeHW = USART_HW_DMA;
#else
	modbus_handler.xTypeHW = USART_HW;
#endif

	if (role == modbus_master)
	{
		modbus_handler.uModbusType = MB_MASTER;
		modbus_handler.u8id = MODBUS_MASTER_ID; //slave ID for master always 0
	}
	else
	{
		modbus_handler.uModbusType = MB_SLAVE;
		modbus_handler.u8id = MODBUS_SLAVE_ID; //slave ID, always different than zero
	}

	memset(modbus_registers, 0, sizeof(modbus_registers));

	modbus_handler.EN_Port = MODBUS_DE_GPIO_Port;
	modbus_handler.EN_Pin = MODBUS_DE_Pin;
	modbus_handler.u16timeOut = TIMEOUT_MODBUS;

	modbus_handler.u16regs = modbus_registers;
	modbus_handler.u16regsize = sizeof(modbus_registers)/sizeof(modbus_registers[0]);

	 //Initialize Modbus library
	ModbusInit(&modbus_handler);
}

/**
  * @brief This starts the modbus module activity.
  * @param None
  * @retval None
  */
void UserModbus_start(void)
{
	//Start capturing traffic on serial Port
	ModbusStart(&modbus_handler);
}

/**
  * @brief This is the de-initialization of the modbus module.
  * @retval None
  */
void UserModbus_deinit(void)
{
	ModbusDeinit(&modbus_handler);
}


/**
  * @brief This is the main modbus routine.
  * @param None
  * @retval None
  */
void UserModbus_exec()
{
	if (modbus_handler.uModbusType == MB_MASTER)
	{
		uint32_t notification;
		bool query_ok = true;
		uint16_t reg_count = (sizeof(modbus_registers)/sizeof(modbus_registers[0]));

		uint32_t query_count;
		uint16_t query_ok_count = 0;

		PRINT("Testing modbus master...\n");

		/* Writes all registers with the same value, for N times, using 'query_count + 1' as value */
		for (query_count = 0; query_count < MODBUS_TEST_MSG_N; query_count++)
		{
			uint16_t test_value = query_count + 1;

			for (uint32_t i = 0; i < reg_count; i++)
			{
				modbus_registers[i] = test_value;
			}

			notification = modbus_request(MODBUS_SLAVE_ID, MB_FC_WRITE_MULTIPLE_REGISTERS, 0x0, reg_count, modbus_registers);

			if (notification == ERR_OK_QUERY)
			{
				/* Set to 0 to avoid false read */
				for (uint32_t i = 0; i < reg_count; i++)
				{
					modbus_registers[i] = 0;
				}

				notification = modbus_request(MODBUS_SLAVE_ID, MB_FC_READ_REGISTERS, 0x0, reg_count, modbus_registers);

				if (notification == ERR_OK_QUERY)
				{
					for (uint32_t i = 0; i < reg_count; i++)
					{
						if (modbus_registers[i] != test_value)
						{
							query_ok = false;
							PRINT("Wrong byte %u, got %u instead of %u\n", i, modbus_registers[i], test_value);
							break;
						}
					}
				}
				else
				{
					query_ok = false;
					PRINT("Error during read operation: %s (%u)\n", modbus_translate_error(notification), notification);
				}
			}
			else
			{
				query_ok = false;
				PRINT("Error during write operation: %s (%u)\n", modbus_translate_error(notification), notification);
			}

			if (query_ok)
			{
				query_ok_count++;
				osDelay(MODBUS_TEST_INTERVAL);
			}
			else
			{
				break;
			}
		}

		if (query_ok_count == MODBUS_TEST_MSG_N)
		{
			PRINT_COLOR("Master modbus test ok (%u/%u queries)\n", color_green, query_ok_count, query_count);
		}
		else
		{
			PRINT_COLOR("Master modbus test fail (%u/%u queries)\n", color_red, query_ok_count, query_count);
		}
	}
}

#endif /* ENABLE_MODBUS */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
