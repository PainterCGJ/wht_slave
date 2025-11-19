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

