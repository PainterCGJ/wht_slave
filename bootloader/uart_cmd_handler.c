/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : uart_cmd_handler.c
 * @brief          : UART command handler implementation
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "uart_cmd_handler.h"

#include <string.h>

#include "bootloader_flag.h"
#include "cmsis_os2.h"
#include "elog.h"
#include "factory_test.h"
#include "usart.h"

/* Private variables ---------------------------------------------------------*/
static const char *TAG = "uart_cmd";
static char uart_cmd_buffer[UART_CMD_BUFFER_SIZE];
static uint8_t uart_cmd_index = 0;
 uint8_t uart_rx_char;

/* RS485数据接收队列 */
#define RS485_RX_QUEUE_SIZE 1024
static osMessageQueueId_t rs485_rx_queue = NULL;

/* Task handles */
osThreadId_t uartCmdTaskHandle;
const osThreadAttr_t uartCmdTask_attributes = {
    .name = "uartCmdTask",
    .stack_size = 1024 * 2,
    .priority = (osPriority_t)osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
static void trim_string(char* str);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Trim whitespace from string
 * @param  str: String to trim
 * @retval None
 */
static void trim_string(char* str) {
    char* end;

    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str &&
           (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        end--;

    // Write new null terminator
    *(end + 1) = 0;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Restart UART receive interrupt (call after baudrate change)
 * @retval None
 */
void uart_cmd_handler_restart_interrupt(void) {
    // 重新启动UART接收中断
    RS485_RX_EN();
    HAL_UART_Receive_IT(&RS485_UART, &uart_rx_char, 1);
}

/**
 * @brief  Initialize UART command handler
 * @retval None
 */
void uart_cmd_handler_init(void) {
    // 清空命令缓冲区
    memset(uart_cmd_buffer, 0, sizeof(uart_cmd_buffer));
    uart_cmd_index = 0;

    // 启动第一次UART接收中断
    RS485_RX_EN();
    HAL_UART_Receive_IT(&RS485_UART, &uart_rx_char, 1);
}

/**
 * @brief  创建UART命令处理任务（在FreeRTOS启动后调用）
 * @retval None
 */
void uart_cmd_handler_create_task(void) {
    // 创建UART命令处理任务
    uartCmdTaskHandle =
        osThreadNew(uart_cmd_handler_task, NULL, &uartCmdTask_attributes);

    if (uartCmdTaskHandle == NULL) {
        elog_e(TAG, "Failed to create UART command handler task");
    }
}

/**
 * @brief  UART command handler task
 * @param  argument: Not used
 * @retval None
 */
void uart_cmd_handler_task(void* argument) {
    // 等待elog系统完全启动，避免日志乱序
    osDelay(200);
    
    // 创建RS485数据接收队列（在FreeRTOS启动后）
    rs485_rx_queue = osMessageQueueNew(RS485_RX_QUEUE_SIZE, sizeof(uint8_t), NULL);
    if (rs485_rx_queue == NULL) {
        elog_e(TAG, "Failed to create RS485 RX queue");
    } else {
        elog_i(TAG, "RS485 RX queue created (size: %d)", RS485_RX_QUEUE_SIZE);
    }

    elog_i(TAG, "UART command handler initialized successfully");
    elog_i(TAG, "UART Command Handler Ready");
    elog_i(TAG, "Type 'help' for available commands");
    elog_d(TAG, "UART command handler task started");

    for (;;) {
        // 定期检查，避免过度占用CPU
        osDelay(100);
    }
}

/**
 * @brief  获取RS485数据接收队列句柄
 * @retval 队列句柄，如果未初始化则返回NULL
 */
osMessageQueueId_t uart_cmd_handler_get_rs485_rx_queue(void) {
    return rs485_rx_queue;
}

/**
 * @brief  从RS485接收队列中读取数据
 * @param  data: 存储读取数据的缓冲区指针
 * @param  timeout_ms: 超时时间（毫秒），0表示不阻塞，osWaitForever表示永久等待
 * @retval 0表示成功，-1表示失败或超时
 */
int uart_cmd_handler_receive_rs485_data(uint8_t *data, uint32_t timeout_ms) {
    if (rs485_rx_queue == NULL || data == NULL) {
        return -1;
    }

    if (osMessageQueueGet(rs485_rx_queue, data, NULL, timeout_ms) == osOK) {
        return 0;
    }

    return -1;
}

/**
 * @brief  获取RS485接收队列中当前数据数量
 * @retval 队列中的数据数量
 */
uint32_t uart_cmd_handler_get_rs485_rx_queue_count(void) {
    if (rs485_rx_queue == NULL) {
        return 0;
    }

    return osMessageQueueGetCount(rs485_rx_queue);
}

/**
 * @brief  Process received UART command
 * @param  command: Command string to process
 * @retval None
 */
void process_uart_command(char* command) {
    trim_string(command);

    elog_d(TAG, "Processing command: '%s'", command);

    if (strcmp(command, CMD_BOOTLOADER_UPGRADE) == 0) {
        elog_i(TAG, "Upgrading firmware... System will reset to bootloader mode.");
        elog_i(TAG, "Firmware upgrade command received");

        // 触发系统复位进入bootloader
        trigger_system_reset_to_bootloader();

    } else if (strcmp(command, CMD_HELP) == 0) {
        elog_i(TAG, "Available commands:");
        elog_i(TAG, "  upgrade  - Enter bootloader mode for firmware upgrade");
        elog_i(TAG, "  version  - Show firmware version");
        elog_i(TAG, "  help     - Show this help message");

    } else if (strcmp(command, CMD_VERSION) == 0) {
        elog_i(TAG, "Firmware Version: 1.0.0");
        elog_i(TAG, "Build Date: " __DATE__ " " __TIME__);
        elog_i(TAG, "MCU: STM32F429ZI");

    } else if (strlen(command) > 0) {
        elog_w(TAG, "Unknown command: '%s'. Type 'help' for available commands.", command);
    }
}

/**
 * @brief  UART receive complete callback
 * @param  huart: UART handle
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance == RS485_UART.Instance) {
        // 将接收到的数据存入队列（非阻塞方式，队列满时丢弃数据）
        if (rs485_rx_queue != NULL) {
            osMessageQueuePut(rs485_rx_queue, &uart_rx_char, 0, 0);
        }

        // 检查是否是工厂测试入口指令（仅在检测期间有效）
        factory_test_process_entry_byte(uart_rx_char);
        
        if (factory_test_is_enabled()) {
            factory_test_process_data(uart_rx_char);
        } else {
            // 正常的命令处理逻辑
            if (uart_rx_char == '\r' || uart_rx_char == '\n') {
                // 接收到回车或换行，处理命令
                if (uart_cmd_index > 0) {
                    uart_cmd_buffer[uart_cmd_index] = '\0';
                    process_uart_command(uart_cmd_buffer);
                    uart_cmd_index = 0;
                    memset(uart_cmd_buffer, 0, sizeof(uart_cmd_buffer));
                }
            } else if (uart_rx_char == '\b' || uart_rx_char == 127) {
                // 退格键处理
                if (uart_cmd_index > 0) {
                    uart_cmd_index--;
                    uart_cmd_buffer[uart_cmd_index] = '\0';
                    // 发送退格、空格、退格来清除终端上的字符
                    RS485_TX_EN();
                    HAL_UART_Transmit(&RS485_UART, (uint8_t*)"\b \b", 3,
                                      HAL_MAX_DELAY);
                    RS485_RX_EN();
                }
            } else if (uart_cmd_index < (UART_CMD_BUFFER_SIZE - 1)) {
                // 普通字符，添加到缓冲区
                uart_cmd_buffer[uart_cmd_index] = uart_rx_char;
                uart_cmd_index++;
            }
        }

        // 继续接收下一个字符
        HAL_UART_Receive_IT(&RS485_UART, &uart_rx_char, 1);
    }
}
