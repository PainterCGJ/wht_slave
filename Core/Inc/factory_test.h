/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : factory_test.h
 * @brief          : Factory test protocol header
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

#ifndef __FACTORY_TEST_H__
#define __FACTORY_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Private defines -----------------------------------------------------------*/
#define FACTORY_TEST_SOF            0xAA55  // Start of Frame (Little Endian: 55 AA)
#define FACTORY_TEST_EOF            0x66BB  // End of Frame (Little Endian: BB 66)
#define FACTORY_TEST_MAX_PAYLOAD    64      // Maximum payload size
#define FACTORY_TEST_BUFFER_SIZE    128     // RX buffer size

/* Frame structure offsets */
#define FRAME_SOF_OFFSET            0
#define FRAME_SOURCE_OFFSET         2
#define FRAME_TARGET_OFFSET         3
#define FRAME_MSG_ID_OFFSET         4
#define FRAME_PAYLOAD_LEN_OFFSET    5
#define FRAME_PAYLOAD_OFFSET        7

/* Message IDs */
#define MSG_ID_GPIO_CONTROL        0x10
#define MSG_ID_64WAY_IO_CONTROL    0x11
#define MSG_ID_DIP_SWITCH_CONTROL  0x12
#define MSG_ID_ADDITIONAL_TEST     0x20
#define MSG_ID_ENTER_TEST          0x21 
#define MSG_ID_HEARTBEAT           0x22

/* Sub-command IDs for Additional Test (MSG_ID = 0x30) */
#define TEST_SUB_UART_LOOPBACK     0x01
#define TEST_SUB_WRITE_SN          0x02
#define TEST_SUB_READ_UID          0x10
#define TEST_SUB_READ_SN           0x11
#define TEST_SUB_READ_FW_VERSION   0x12

/* Sub-command IDs for GPIO control (MSG_ID = 0x10) */
#define GPIO_SUB_SET_MODE          0x01
#define GPIO_SUB_SET_PULL          0x02
#define GPIO_SUB_WRITE_LEVEL       0x03
#define GPIO_SUB_READ_LEVEL        0x04

/* Device addresses */
#define DEVICE_ADDR_TOOLING        0x01
#define DEVICE_ADDR_BOARD          0x02

/* Port IDs for GPIO control */
#define PORT_ID_A                  0x00
#define PORT_ID_B                  0x01
#define PORT_ID_C                  0x02
#define PORT_ID_D                  0x03
#define PORT_ID_E                  0x04
#define PORT_ID_F                  0x05
#define PORT_ID_G                  0x06
#define PORT_ID_H                  0x07

/* GPIO modes */
#define FACTORY_GPIO_MODE_INPUT    0x00
#define FACTORY_GPIO_MODE_OUTPUT   0x01
#define FACTORY_GPIO_MODE_ANALOG   0x02

/* GPIO pull configurations */
#define GPIO_PULL_DOWN             0x00
#define GPIO_PULL_UP               0x01
#define GPIO_PULL_NONE             0x02

/* GPIO levels */
#define GPIO_LEVEL_LOW             0x00
#define GPIO_LEVEL_HIGH            0x01

/* SN Storage Constants */
#define SN_FLASH_SECTOR            22                    // Flash扇区22用于存储SN
#define SN_FLASH_ADDRESS           0x081C0000UL          // Sector 22起始地址
#define SN_MAX_LENGTH              64                    // SN最大长度
#define SN_MAGIC_NUMBER            0x5AA5A55AUL          // SN存储的魔数

/* Device UID Constants */
#define DEVICE_UID_SIZE            12                    // 设备UID长度(字节)

/* Test Constants */
#define UART_LOOPBACK_TEST_STRING  "test"                // 串口回环测试字符串
#define UART_LOOPBACK_TIMEOUT      1000                  // 串口测试超时时间(ms)

/* Status codes */
typedef enum {
    DEVICE_OK                      = 0x00,
    DEVICE_ERR_INVALID_PORT        = 0x01,
    DEVICE_ERR_INVALID_PIN         = 0x02,
    DEVICE_ERR_INVALID_SUB_ID      = 0x03,
    DEVICE_ERR_EXECUTION           = 0x04
} device_status_t;

/* Heartbeat status */
typedef enum {
    HEARTBEAT_OK                   = 0x00,
    HEARTBEAT_BUSY                 = 0x01,
    HEARTBEAT_ERROR                = 0xFF
} heartbeat_status_t;

/* Frame structure */
typedef struct {
    uint16_t sof;           // Start of Frame
    uint8_t source;         // Source address
    uint8_t target;         // Target address
    uint8_t msg_id;         // Message ID
    uint16_t payload_len;   // Payload length (Little Endian)
    uint8_t payload[FACTORY_TEST_MAX_PAYLOAD];  // Payload data
    uint16_t crc16;         // CRC16 checksum
    uint16_t eof;           // End of Frame
} factory_test_frame_t;

/* GPIO control command structure */
typedef struct {
    uint8_t sub_id;         // Sub-command ID
    uint8_t port_id;        // Port ID
    uint16_t pin_mask;      // Pin mask (Little Endian)
    uint8_t value;          // Value parameter
} gpio_control_cmd_t;

/* GPIO read response structure */
typedef struct {
    uint8_t sub_id;         // Sub-command ID (0x04)
    uint8_t port_id;        // Port ID
    uint16_t levels;        // Pin levels (Little Endian)
} gpio_read_response_t;

/* SN storage structure in Flash */
typedef struct {
    uint32_t magic;         // 魔数验证
    uint8_t sn_length;      // SN长度
    uint8_t sn_data[SN_MAX_LENGTH];  // SN数据
    uint8_t reserved[3];    // 保留字节，用于32位对齐
} sn_storage_t;

/* Factory test mode state */
typedef enum {
    FACTORY_TEST_DISABLED,
    FACTORY_TEST_ENABLED,
    FACTORY_TEST_PROCESSING
} factory_test_state_t;

/* Factory test entry command */
#define FACTORY_TEST_ENTRY_CMD_LEN    11
extern const uint8_t FACTORY_TEST_ENTRY_CMD[FACTORY_TEST_ENTRY_CMD_LEN];

#define FACTORY_TEST_DETECTION_TIMEOUT_MS  1000  // 1 second timeout

/* Function prototypes -------------------------------------------------------*/
void factory_test_init(void);
void factory_test_start_entry_detection(void);  // Start entry command detection
bool factory_test_process_entry_byte(uint8_t data);  // Process byte for entry detection
bool factory_test_blocking_check_entry(void);   // Blocking check for entry command (1 second)
void factory_test_enter_mode(void);
void factory_test_exit_mode(void);
bool factory_test_is_enabled(void);

void factory_test_process_data(uint8_t data);  // Store data in ring buffer (interrupt)
void factory_test_task_process(void);          // Process frames from ring buffer (task)
bool factory_test_parse_frame(const uint8_t* buffer, uint16_t length, factory_test_frame_t* frame);
uint16_t factory_test_calculate_crc16(const uint8_t* data, uint16_t length);  // CRC16-MODBUS
void factory_test_send_response(const factory_test_frame_t* response);

void factory_test_handle_heartbeat(const factory_test_frame_t* frame);
void factory_test_handle_gpio_control(const factory_test_frame_t* frame);
void factory_test_handle_64way_io_control(const factory_test_frame_t* frame);
void factory_test_handle_dip_switch_control(const factory_test_frame_t* frame);
void factory_test_handle_additional_test(const factory_test_frame_t* frame);

device_status_t factory_test_gpio_set_mode(uint8_t port_id, uint16_t pin_mask, uint8_t mode);
device_status_t factory_test_gpio_set_pull(uint8_t port_id, uint16_t pin_mask, uint8_t pull);
device_status_t factory_test_gpio_write_level(uint8_t port_id, uint16_t pin_mask, uint8_t level);
device_status_t factory_test_gpio_read_level(uint8_t port_id, uint16_t* levels);

GPIO_TypeDef* factory_test_get_gpio_port(uint8_t port_id);
uint16_t factory_test_convert_pin_mask(uint16_t mask);

/* SN management functions */
device_status_t factory_test_write_sn(const uint8_t* sn_data, uint8_t sn_length);
device_status_t factory_test_read_sn(uint8_t* sn_data, uint8_t* sn_length);

/* Additional test functions */
device_status_t factory_test_uart_loopback(void);
device_status_t factory_test_read_device_uid(uint8_t* uid_data);
device_status_t factory_test_read_firmware_version(uint8_t* version_data);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_TEST_H__ */
