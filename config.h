/**
 ******************************************************************************
 * @file    config.h
 * @brief   项目配置文件，用于统一管理各种配置选项
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Note: Files using these macros must include "usart.h" first to get UART handle declarations */
/* Note: Files using priority macros must include "cmsis_os2.h" first for osPriority_t type */

/* Configuration Options -----------------------------------------------------*/

/**
 * @brief printf输出串口选择
 * 
 * 可选值：
 *   1 - UART1 (PA9/PA10) - 普通UART，可用于RS232
 *   4 - UART4 (PA0/PA1, DEBUG_UART) - 普通UART，可用于RS232
 *   7 - UART7 (PF6/PF7, RS485_UART) - RS485串口，带方向控制
 * 
 * 默认值：7 (UART7/RS485)
 */
#ifndef PRINTF_OUTPUT_UART
#define PRINTF_OUTPUT_UART    7
#endif

/**
 * @brief elog输出串口选择
 * 
 * 可选值：
 *   1 - UART1 (PA9/PA10) - 普通UART，可用于RS232
 *   4 - UART4 (PA0/PA1, DEBUG_UART) - 普通UART，可用于RS232
 *   7 - UART7 (PF6/PF7, RS485_UART) - RS485串口，带方向控制
 * 
 * 默认值：7 (UART7/RS485)
 */
#ifndef ELOG_OUTPUT_UART
#define ELOG_OUTPUT_UART      7
#endif

/* FreeRTOS Configuration ----------------------------------------------------*/

/**
 * @brief FreeRTOS堆大小配置（字节）
 * 
 * 默认值：512KB (500 * 1024)
 */
#ifndef FREERTOS_HEAP_SIZE
#define FREERTOS_HEAP_SIZE                    ((size_t)500 * 1024)
#endif

/* Task Enable/Disable Switches ----------------------------------------------*/

/**
 * @brief OTA任务启用开关
 * 、
 * 可选值：
 *   0 - 禁用OTA任务，不创建OTA任务
 *   1 - 启用OTA任务，创建OTA任务
 * 
 * 默认值：1 (启用)
 */
#ifndef ENABLE_OTA_TASK
#define ENABLE_OTA_TASK                       0
#endif

/* Task Stack Size Definitions -----------------------------------------------*/

/**
 * @brief 任务栈大小定义（字节）
 * 
 * 所有任务的栈大小统一在此定义，便于管理和调整
 */

/* 系统级任务 */
#ifndef TASK_STACK_SIZE_DEFAULT
#define TASK_STACK_SIZE_DEFAULT                (1024 * 16)  // 16KB - defaultTask主任务
#endif

#ifndef TASK_STACK_SIZE_ELOG
#define TASK_STACK_SIZE_ELOG                   (512 * 4)    // 2KB - 日志任务
#endif

/* 应用级任务 */
#ifndef TASK_STACK_SIZE_UWB_COMM
#define TASK_STACK_SIZE_UWB_COMM               (1024 * 16)  // 16KB - UWB通信任务
#endif

#ifndef TASK_STACK_SIZE_DATA_COLLECTION
#define TASK_STACK_SIZE_DATA_COLLECTION        (1024)       // 1KB - 数据采集任务
#endif

#ifndef TASK_STACK_SIZE_ACCESSORY
#define TASK_STACK_SIZE_ACCESSORY              (1024)       // 1KB - 外设管理任务
#endif

#ifndef TASK_STACK_SIZE_SLAVE_DATA_PROC
#define TASK_STACK_SIZE_SLAVE_DATA_PROC        (8 * 1024)   // 8KB - 从机数据处理任务
#endif

#ifndef TASK_STACK_SIZE_OTA
#define TASK_STACK_SIZE_OTA                    (1024)        // 1KB - OTA任务
#endif

/* Task Priority Definitions ------------------------------------------------*/

/**
 * @brief 任务优先级定义
 * 
 * 使用CMSIS-RTOS V2优先级值（osPriority_t）
 * 优先级范围：0-55 (configMAX_PRIORITIES = 56)
 * 
 * CMSIS-RTOS优先级映射：
 *   osPriorityLow = 8
 *   osPriorityNormal = 24
 *   osPriorityHigh = 40
 * 
 * TaskPriority枚举映射（configMAX_PRIORITIES = 56时）：
 *   TaskPrio_Idle = 0
 *   TaskPrio_Low = 1
 *   TaskPrio_HMI = 2
 *   TaskPrio_Mid = 28 (56/2)
 *   TaskPrio_High = 54
 *   TaskPrio_Highest = 55
 */

/* 系统级任务优先级 */
/* 注意：这些值对应 cmsis_os2.h 中的 osPriority_t 枚举值 */
#ifndef TASK_PRIORITY_DEFAULT
#define TASK_PRIORITY_DEFAULT                  (24)  // osPriorityNormal - defaultTask主任务
#endif

#ifndef TASK_PRIORITY_ELOG
#define TASK_PRIORITY_ELOG                     (8)   // osPriorityLow - 日志任务
#endif

/* 应用级任务优先级 */
#ifndef TASK_PRIORITY_UWB_COMM
#define TASK_PRIORITY_UWB_COMM                 (24)  // osPriorityNormal - UWB通信任务
#endif

#ifndef TASK_PRIORITY_DATA_COLLECTION
#define TASK_PRIORITY_DATA_COLLECTION          (28)  // TaskPrio_Mid (configMAX_PRIORITIES/2) - 数据采集任务
#endif

#ifndef TASK_PRIORITY_ACCESSORY
#define TASK_PRIORITY_ACCESSORY                (28)  // TaskPrio_Mid (configMAX_PRIORITIES/2) - 外设管理任务
#endif

#ifndef TASK_PRIORITY_SLAVE_DATA_PROC
#define TASK_PRIORITY_SLAVE_DATA_PROC          (28)  // TaskPrio_Mid (configMAX_PRIORITIES/2) - 从机数据处理任务
#endif

#ifndef TASK_PRIORITY_OTA
#define TASK_PRIORITY_OTA                      (24)  // osPriorityNormal - OTA任务
#endif

/* Helper Macros -------------------------------------------------------------*/

/**
 * @brief 根据串口编号获取UART句柄和控制宏
 * 
 * 注意：UART7需要RS485方向控制，UART1和UART4不需要
 */

/* printf串口配置 */
#if PRINTF_OUTPUT_UART == 1
    #define PRINTF_UART_HANDLE    (&huart1)
    #define PRINTF_UART_TX_EN()   do {} while(0)  // UART1不需要RS485控制
    #define PRINTF_UART_RX_EN()   do {} while(0)
#elif PRINTF_OUTPUT_UART == 4
    #define PRINTF_UART_HANDLE    (&huart4)
    #define PRINTF_UART_TX_EN()   do {} while(0)  // UART4不需要RS485控制
    #define PRINTF_UART_RX_EN()   do {} while(0)
#elif PRINTF_OUTPUT_UART == 7
    #define PRINTF_UART_HANDLE    (&huart7)
    #define PRINTF_UART_TX_EN()   RS485_TX_EN()   // UART7需要RS485控制
    #define PRINTF_UART_RX_EN()   RS485_RX_EN()
#else
    #error "Invalid PRINTF_OUTPUT_UART value. Must be 1, 4, or 7."
#endif

/* elog串口配置 */
#if ELOG_OUTPUT_UART == 1
    #define ELOG_UART_HANDLE       (&huart1)
    #define ELOG_UART_TX_EN()      do {} while(0)  // UART1不需要RS485控制
    #define ELOG_UART_RX_EN()      do {} while(0)
#elif ELOG_OUTPUT_UART == 4
    #define ELOG_UART_HANDLE       (&huart4)
    #define ELOG_UART_TX_EN()      do {} while(0)  // UART4不需要RS485控制
    #define ELOG_UART_RX_EN()      do {} while(0)
#elif ELOG_OUTPUT_UART == 7
    #define ELOG_UART_HANDLE       (&huart7)
    #define ELOG_UART_TX_EN()      RS485_TX_EN()   // UART7需要RS485控制
    #define ELOG_UART_RX_EN()      RS485_RX_EN()
#else
    #error "Invalid ELOG_OUTPUT_UART value. Must be 1, 4, or 7."
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */

