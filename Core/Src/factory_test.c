/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : factory_test.c
 * @brief          : Factory test protocol implementation
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
#include "factory_test.h"

#include <stdio.h>
#include <string.h>

#include "elog.h"
#include "main.h"
#include "usart.h"

/* Private variables ---------------------------------------------------------*/
static const char* TAG = "factory_test";
static factory_test_state_t test_state = FACTORY_TEST_DISABLED;

/* Ring buffer for interrupt-safe data transfer */
#define RING_BUFFER_SIZE 256
static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint16_t ring_head =
    0;    // Write pointer (updated in interrupt)
static volatile uint16_t ring_tail = 0;    // Read pointer (updated in task)

/* Frame processing buffer */
static uint8_t frame_buffer[FACTORY_TEST_BUFFER_SIZE];
static uint16_t frame_index = 0;

/* CRC16-CCITT-FALSE lookup table */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108,
    0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B,
    0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
    0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6,
    0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D,
    0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5,
    0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC,
    0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4,
    0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD,
    0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13,
    0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
    0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E,
    0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1,
    0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB,
    0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
    0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8,
    0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
    0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9,
    0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882,
    0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E,
    0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07,
    0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D,
    0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
    0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

/* Private function prototypes -----------------------------------------------*/
static void factory_test_reset_frame_buffer(void);
static bool factory_test_find_frame_start(void);
static uint16_t factory_test_get_frame_length(const uint8_t* buffer);
static void factory_test_create_response_frame(factory_test_frame_t* response,
                                               uint8_t msg_id,
                                               const uint8_t* payload,
                                               uint16_t payload_len);

/* Ring buffer functions */
static bool ring_buffer_put(uint8_t data);
static bool ring_buffer_get(uint8_t* data);
static uint16_t ring_buffer_available(void);
static void ring_buffer_reset(void);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Initialize factory test mode
 * @retval None
 */
void factory_test_init(void) {
    test_state = FACTORY_TEST_DISABLED;
    ring_buffer_reset();
    factory_test_reset_frame_buffer();

    elog_i(TAG, "Factory test initialized");
}

/**
 * @brief  Check if DIP6 pin is low to enter factory test mode
 * @retval true if should enter test mode, false otherwise
 */
bool factory_test_check_entry_condition(void) {
    // Check DIP6_Pin (PF9) state
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(DIP6_GPIO_Port, DIP6_Pin);

    if (pin_state == GPIO_PIN_RESET) {
        elog_i(TAG, "DIP6 pin detected low, entering factory test mode");
        return true;
    }

    return false;
}

/**
 * @brief  Enter factory test mode
 * @retval None
 */
void factory_test_enter_mode(void) {
    test_state = FACTORY_TEST_ENABLED;
    ring_buffer_reset();
    factory_test_reset_frame_buffer();
    elog_set_filter_tag_lvl(TAG, ELOG_LVL_ERROR);

    elog_i(TAG, "Entered factory test mode");

    // Send confirmation message
    const char* msg = "Factory test mode activated (DIP6 detected)\r\n";
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/**
 * @brief  Exit factory test mode
 * @retval None
 */
void factory_test_exit_mode(void) {
    test_state = FACTORY_TEST_DISABLED;
    ring_buffer_reset();
    factory_test_reset_frame_buffer();

    elog_i(TAG, "Exited factory test mode");
}

/**
 * @brief  Check if factory test mode is enabled
 * @retval true if enabled, false otherwise
 */
bool factory_test_is_enabled(void) {
    return (test_state != FACTORY_TEST_DISABLED);
}

/**
 * @brief  Store incoming data byte in ring buffer (called from interrupt)
 * @param  data: Received data byte
 * @retval None
 */
void factory_test_process_data(uint8_t data) {
    if (test_state != FACTORY_TEST_ENABLED) {
        return;    // Silently ignore data when not in test mode
    }

    // Store data in ring buffer (interrupt-safe)
    ring_buffer_put(data);
}

/**
 * @brief  Process frames from ring buffer (called from task)
 * @retval None
 */
void factory_test_task_process(void) {
    if (test_state != FACTORY_TEST_ENABLED) {
        return;
    }

    uint8_t data;

    // Process all available data from ring buffer
    while (ring_buffer_get(&data)) {
        // Store received byte in frame buffer
        if (frame_index < FACTORY_TEST_BUFFER_SIZE - 1) {
            frame_buffer[frame_index++] = data;
        } else {
            // Buffer overflow, reset
            factory_test_reset_frame_buffer();
            frame_buffer[frame_index++] = data;
        }

        // Try to find and parse a complete frame
        if (factory_test_find_frame_start()) {
            uint16_t frame_len = factory_test_get_frame_length(frame_buffer);

            if (frame_len > 0 && frame_index >= frame_len) {
                factory_test_frame_t frame;
                if (factory_test_parse_frame(frame_buffer, frame_len, &frame)) {
                    test_state = FACTORY_TEST_PROCESSING;

                    // Process the frame based on message ID
                    switch (frame.msg_id) {
                        case MSG_ID_HEARTBEAT:
                            elog_v(TAG, "heartbeat");
                            factory_test_handle_heartbeat(&frame);
                            break;
                        case MSG_ID_GPIO_CONTROL:
                            elog_v(TAG, "gpio_control");
                            factory_test_handle_gpio_control(&frame);
                            break;
                        case MSG_ID_SPECIAL_IO_CONTROL:
                            elog_v(TAG, "special_io_control");
                            factory_test_handle_special_io_control(&frame);
                            break;
                        case MSG_ID_EXECUTE_TEST:
                            elog_v(TAG, "execute_test");
                            factory_test_handle_execute_test(&frame);
                            break;
                        default:
                            elog_w(TAG, "Unknown message ID: 0x%02X",
                                   frame.msg_id);
                            break;
                    }

                    test_state = FACTORY_TEST_ENABLED;
                }

                // Remove processed frame from buffer
                if (frame_index > frame_len) {
                    memmove(frame_buffer, &frame_buffer[frame_len],
                            frame_index - frame_len);
                    frame_index -= frame_len;
                } else {
                    factory_test_reset_frame_buffer();
                }
            } else {
                // elog_v(TAG, "frame_index: %d, frame_len: %d", frame_index,
                //        frame_len);
            }
        }
    }
}

/**
 * @brief  Parse received frame
 * @param  buffer: Buffer containing frame data
 * @param  length: Length of frame data
 * @param  frame: Pointer to frame structure to fill
 * @retval true if frame parsed successfully, false otherwise
 */
bool factory_test_parse_frame(const uint8_t* buffer, uint16_t length,
                              factory_test_frame_t* frame) {
    if (length < 9) {    // Minimum frame size: SOF(2) + MSG(3) + LEN(2) +
                         // CRC(2) + EOF(2)
        elog_w(TAG, "length error");
        return false;
    }

    // Parse SOF (Little Endian)
    frame->sof = buffer[0] | (buffer[1] << 8);
    if (frame->sof != FACTORY_TEST_SOF) {
        elog_w(TAG, "sof error");
        return false;
    }

    // Parse message info
    frame->source = buffer[FRAME_SOURCE_OFFSET];
    frame->target = buffer[FRAME_TARGET_OFFSET];
    frame->msg_id = buffer[FRAME_MSG_ID_OFFSET];

    // Parse payload length (Little Endian)
    frame->payload_len = buffer[FRAME_PAYLOAD_LEN_OFFSET] |
                         (buffer[FRAME_PAYLOAD_LEN_OFFSET + 1] << 8);

    if (frame->payload_len > FACTORY_TEST_MAX_PAYLOAD) {
        elog_w(TAG, "payload_len error");
        return false;
    }

    // Check if we have enough data for the complete frame
    uint16_t expected_length = FRAME_PAYLOAD_OFFSET + frame->payload_len +
                               2;    // +2 for CRC(2) + EOF(2) - 2 for SOF(2)
    if (length < expected_length) {
        elog_w(TAG, "expected_length error");
        return false;
    }

    // Copy payload
    memcpy(frame->payload, &buffer[FRAME_PAYLOAD_OFFSET], frame->payload_len);

    // Parse CRC16 (Little Endian)
    uint16_t crc_offset = FRAME_PAYLOAD_OFFSET + frame->payload_len;
    frame->crc16 = buffer[crc_offset] | (buffer[crc_offset + 1] << 8);

    // Parse EOF (Little Endian)
    uint16_t eof_offset = crc_offset + 2;
    frame->eof = buffer[eof_offset] | (buffer[eof_offset + 1] << 8);
    if (frame->eof != FACTORY_TEST_EOF) {
        elog_w(TAG, "eof error");
        return false;
    }

    // Verify CRC16
    uint16_t calculated_crc = factory_test_calculate_crc16(
        &buffer[FRAME_SOURCE_OFFSET],
        3 + 2 + frame->payload_len);    // MSG(3) + LEN(2) + PAYLOAD
    if (calculated_crc != frame->crc16) {
        elog_w(TAG, "crc error, calculated_crc: %04X, frame->crc16: %04X",
               calculated_crc, frame->crc16);
        return false;
    }

    return true;
}

/**
 * @brief  Calculate CRC16-CCITT-FALSE
 * @param  data: Pointer to data
 * @param  length: Length of data
 * @retval CRC16 value
 */
uint16_t factory_test_calculate_crc16(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }

    return crc;
}

/**
 * @brief  Send response frame
 * @param  response: Pointer to response frame
 * @retval None
 */
void factory_test_send_response(const factory_test_frame_t* response) {
    uint8_t tx_buffer[FACTORY_TEST_BUFFER_SIZE];
    uint16_t tx_index = 0;

    // SOF (Little Endian)
    tx_buffer[tx_index++] = response->sof & 0xFF;
    tx_buffer[tx_index++] = (response->sof >> 8) & 0xFF;

    // Message info
    tx_buffer[tx_index++] = response->source;
    tx_buffer[tx_index++] = response->target;
    tx_buffer[tx_index++] = response->msg_id;

    // Payload length (Little Endian)
    tx_buffer[tx_index++] = response->payload_len & 0xFF;
    tx_buffer[tx_index++] = (response->payload_len >> 8) & 0xFF;

    // Payload
    memcpy(&tx_buffer[tx_index], response->payload, response->payload_len);
    tx_index += response->payload_len;

    // Calculate and add CRC16
    uint16_t crc = factory_test_calculate_crc16(&tx_buffer[2],
                                                3 + 2 + response->payload_len);
    tx_buffer[tx_index++] = crc & 0xFF;
    tx_buffer[tx_index++] = (crc >> 8) & 0xFF;

    // EOF (Little Endian)
    tx_buffer[tx_index++] = response->eof & 0xFF;
    tx_buffer[tx_index++] = (response->eof >> 8) & 0xFF;

    // Send via UART
    HAL_UART_Transmit(&DEBUG_UART, tx_buffer, tx_index, HAL_MAX_DELAY);

    elog_d(TAG, "Sent response frame, MSG_ID=0x%02X, len=%d", response->msg_id,
           tx_index);
}

/**
 * @brief  Handle heartbeat message
 * @param  frame: Pointer to received frame
 * @retval None
 */
void factory_test_handle_heartbeat(const factory_test_frame_t* frame) {
    factory_test_frame_t response;
    uint8_t status = HEARTBEAT_OK;

    factory_test_create_response_frame(&response, MSG_ID_HEARTBEAT, &status, 1);
    factory_test_send_response(&response);

    elog_d(TAG, "Heartbeat response sent");
}

/**
 * @brief  Handle GPIO control message (MSG_ID = 0x10)
 * @param  frame: Pointer to received frame
 * @retval None
 */
void factory_test_handle_gpio_control(const factory_test_frame_t* frame) {
    if (frame->payload_len < 2) {
        elog_w(TAG, "GPIO control payload too short");
        return;
    }

    gpio_control_cmd_t* cmd = (gpio_control_cmd_t*)frame->payload;
    factory_test_frame_t response;
    device_status_t status = DEVICE_OK;

    switch (cmd->sub_id) {
        case GPIO_SUB_SET_MODE:
            if (frame->payload_len >= 5) {
                status = factory_test_gpio_set_mode(cmd->port_id, cmd->pin_mask,
                                                    cmd->value);
                uint8_t response_payload[2] = {cmd->sub_id, status};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            } else {
                uint8_t response_payload[2] = {cmd->sub_id,
                                               DEVICE_ERR_EXECUTION};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            }
            break;

        case GPIO_SUB_SET_PULL:
            if (frame->payload_len >= 5) {
                status = factory_test_gpio_set_pull(cmd->port_id, cmd->pin_mask,
                                                    cmd->value);
                uint8_t response_payload[2] = {cmd->sub_id, status};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            } else {
                uint8_t response_payload[2] = {cmd->sub_id,
                                               DEVICE_ERR_EXECUTION};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            }
            break;

        case GPIO_SUB_WRITE_LEVEL:
            if (frame->payload_len >= 5) {
                status = factory_test_gpio_write_level(
                    cmd->port_id, cmd->pin_mask, cmd->value);
                uint8_t response_payload[2] = {cmd->sub_id, status};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            } else {
                uint8_t response_payload[2] = {cmd->sub_id,
                                               DEVICE_ERR_EXECUTION};
                factory_test_create_response_frame(
                    &response, MSG_ID_GPIO_CONTROL, response_payload, 2);
            }
            break;

        case GPIO_SUB_READ_LEVEL: {
            uint16_t levels = 0;
            status = factory_test_gpio_read_level(cmd->port_id, &levels);
            gpio_read_response_t read_response = {.sub_id = GPIO_SUB_READ_LEVEL,
                                                  .port_id = cmd->port_id,
                                                  .levels = levels};
            factory_test_create_response_frame(&response, MSG_ID_GPIO_CONTROL,
                                               (uint8_t*)&read_response,
                                               sizeof(read_response));
        } break;

        default: {
            uint8_t response_payload[2] = {cmd->sub_id,
                                           DEVICE_ERR_INVALID_SUB_ID};
            factory_test_create_response_frame(&response, MSG_ID_GPIO_CONTROL,
                                               response_payload, 2);
        } break;
    }

    factory_test_send_response(&response);
    elog_d(TAG, "GPIO control response sent, sub_id=0x%02X, status=%d",
           cmd->sub_id, status);
}

/**
 * @brief  Handle special IO control message (MSG_ID = 0x11)
 * @param  frame: Pointer to received frame
 * @retval None
 */
void factory_test_handle_special_io_control(const factory_test_frame_t* frame) {
    // TODO: Implement special IO control (64-way IO, DIP switches, etc.)
    elog_w(TAG, "Special IO control not implemented yet");

    factory_test_frame_t response;
    uint8_t response_payload[3] = {
        0x01, 0x01, DEVICE_ERR_EXECUTION};    // Sub-ID, Target-ID, Status
    factory_test_create_response_frame(&response, MSG_ID_SPECIAL_IO_CONTROL,
                                       response_payload, 3);
    factory_test_send_response(&response);
}

/**
 * @brief  Handle execute test message (MSG_ID = 0x30)
 * @param  frame: Pointer to received frame
 * @retval None
 */
void factory_test_handle_execute_test(const factory_test_frame_t* frame) {
    // TODO: Implement test execution (UART loopback, read UID, etc.)
    elog_w(TAG, "Execute test not implemented yet");

    factory_test_frame_t response;
    uint8_t response_payload[2] = {0x01,
                                   DEVICE_ERR_EXECUTION};    // Sub-ID, Status
    factory_test_create_response_frame(&response, MSG_ID_EXECUTE_TEST,
                                       response_payload, 2);
    factory_test_send_response(&response);
}

/* GPIO Control Functions --------------------------------------------------- */

/**
 * @brief  Set GPIO pin mode
 * @param  port_id: Port ID (0=PA, 1=PB, etc.)
 * @param  pin_mask: Pin mask (bit 0 = pin 0, etc.)
 * @param  mode: GPIO mode (0=input, 1=output, 2=analog)
 * @retval Device status
 */
device_status_t factory_test_gpio_set_mode(uint8_t port_id, uint16_t pin_mask,
                                           uint8_t mode) {
    GPIO_TypeDef* gpio_port = factory_test_get_gpio_port(port_id);
    if (gpio_port == NULL) {
        return DEVICE_ERR_INVALID_PORT;
    }

    if (pin_mask == 0) {
        return DEVICE_ERR_INVALID_PIN;
    }

    // Enable GPIO clock for the specific port
    switch (port_id) {
        case PORT_ID_A:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case PORT_ID_B:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case PORT_ID_C:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
        case PORT_ID_D:
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
        case PORT_ID_E:
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
        case PORT_ID_F:
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
        case PORT_ID_G:
            __HAL_RCC_GPIOG_CLK_ENABLE();
            break;
        case PORT_ID_H:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            return DEVICE_ERR_INVALID_PORT;
    }

    GPIO_InitTypeDef gpio_init = {0};

    // Set mode
    switch (mode) {
        case FACTORY_GPIO_MODE_INPUT:
            gpio_init.Mode = GPIO_MODE_INPUT;
            break;
        case FACTORY_GPIO_MODE_OUTPUT:
            gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
            break;
        case FACTORY_GPIO_MODE_ANALOG:
            gpio_init.Mode = GPIO_MODE_ANALOG;
            break;
        default:
            return DEVICE_ERR_EXECUTION;
    }

    gpio_init.Pin = factory_test_convert_pin_mask(pin_mask);
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(gpio_port, &gpio_init);

    elog_d(TAG, "GPIO mode set: port=%d, mask=0x%04X, mode=%d", port_id,
           pin_mask, mode);
    return DEVICE_OK;
}

/**
 * @brief  Set GPIO pin pull configuration
 * @param  port_id: Port ID
 * @param  pin_mask: Pin mask
 * @param  pull: Pull configuration (0=down, 1=up, 2=none)
 * @retval Device status
 */
device_status_t factory_test_gpio_set_pull(uint8_t port_id, uint16_t pin_mask,
                                           uint8_t pull) {
    GPIO_TypeDef* gpio_port = factory_test_get_gpio_port(port_id);
    if (gpio_port == NULL) {
        return DEVICE_ERR_INVALID_PORT;
    }

    if (pin_mask == 0) {
        return DEVICE_ERR_INVALID_PIN;
    }

    // Enable GPIO clock for the specific port
    switch (port_id) {
        case PORT_ID_A:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case PORT_ID_B:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case PORT_ID_C:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
        case PORT_ID_D:
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
        case PORT_ID_E:
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
        case PORT_ID_F:
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
        case PORT_ID_G:
            __HAL_RCC_GPIOG_CLK_ENABLE();
            break;
        case PORT_ID_H:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            return DEVICE_ERR_INVALID_PORT;
    }

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = factory_test_convert_pin_mask(pin_mask);
    gpio_init.Mode = GPIO_MODE_INPUT;    // Assume input mode for pull config
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    switch (pull) {
        case GPIO_PULL_DOWN:
            gpio_init.Pull = GPIO_PULLDOWN;
            break;
        case GPIO_PULL_UP:
            gpio_init.Pull = GPIO_PULLUP;
            break;
        case GPIO_PULL_NONE:
            gpio_init.Pull = GPIO_NOPULL;
            break;
        default:
            return DEVICE_ERR_EXECUTION;
    }

    HAL_GPIO_Init(gpio_port, &gpio_init);

    elog_d(TAG, "GPIO pull set: port=%d, mask=0x%04X, pull=%d", port_id,
           pin_mask, pull);
    return DEVICE_OK;
}

/**
 * @brief  Write GPIO pin levels
 * @param  port_id: Port ID
 * @param  pin_mask: Pin mask
 * @param  level: Level to write (0=low, 1=high)
 * @retval Device status
 */
device_status_t factory_test_gpio_write_level(uint8_t port_id,
                                              uint16_t pin_mask,
                                              uint8_t level) {
    GPIO_TypeDef* gpio_port = factory_test_get_gpio_port(port_id);
    if (gpio_port == NULL) {
        return DEVICE_ERR_INVALID_PORT;
    }

    if (pin_mask == 0) {
        return DEVICE_ERR_INVALID_PIN;
    }

    uint16_t hal_pin_mask = factory_test_convert_pin_mask(pin_mask);

    if (level == GPIO_LEVEL_HIGH) {
        HAL_GPIO_WritePin(gpio_port, hal_pin_mask, GPIO_PIN_SET);
    } else if (level == GPIO_LEVEL_LOW) {
        HAL_GPIO_WritePin(gpio_port, hal_pin_mask, GPIO_PIN_RESET);
    } else {
        return DEVICE_ERR_EXECUTION;
    }

    elog_d(TAG, "GPIO level written: port=%d, mask=0x%04X, level=%d", port_id,
           pin_mask, level);
    return DEVICE_OK;
}

/**
 * @brief  Read GPIO pin levels
 * @param  port_id: Port ID
 * @param  levels: Pointer to store read levels
 * @retval Device status
 */
device_status_t factory_test_gpio_read_level(uint8_t port_id,
                                             uint16_t* levels) {
    GPIO_TypeDef* gpio_port = factory_test_get_gpio_port(port_id);
    if (gpio_port == NULL) {
        return DEVICE_ERR_INVALID_PORT;
    }

    if (levels == NULL) {
        return DEVICE_ERR_EXECUTION;
    }

    *levels = (uint16_t)gpio_port->IDR;

    elog_d(TAG, "GPIO levels read: port=%d, levels=0x%04X", port_id, *levels);
    return DEVICE_OK;
}

/**
 * @brief  Get GPIO port from port ID
 * @param  port_id: Port ID
 * @retval GPIO port pointer or NULL if invalid
 */
GPIO_TypeDef* factory_test_get_gpio_port(uint8_t port_id) {
    switch (port_id) {
        case PORT_ID_A:
            return GPIOA;
        case PORT_ID_B:
            return GPIOB;
        case PORT_ID_C:
            return GPIOC;
        case PORT_ID_D:
            return GPIOD;
        case PORT_ID_E:
            return GPIOE;
        case PORT_ID_F:
            return GPIOF;
        case PORT_ID_G:
            return GPIOG;
        case PORT_ID_H:
            return GPIOH;
        default:
            return NULL;
    }
}

/**
 * @brief  Convert pin mask to HAL pin mask
 * @param  mask: Input pin mask (bit position format)
 * @retval HAL pin mask
 */
uint16_t factory_test_convert_pin_mask(uint16_t mask) {
    uint16_t hal_mask = 0;

    for (uint8_t i = 0; i < 16; i++) {
        if (mask & (1 << i)) {
            hal_mask |= (1 << i);
        }
    }

    return hal_mask;
}

/* Private functions ---------------------------------------------------------
 */

/**
 * @brief  Reset frame buffer
 * @retval None
 */
static void factory_test_reset_frame_buffer(void) {
    memset(frame_buffer, 0, sizeof(frame_buffer));
    frame_index = 0;
}

/**
 * @brief  Find frame start in buffer
 * @retval true if frame start found, false otherwise
 */
static bool factory_test_find_frame_start(void) {
    if (frame_index < 2) {
        return false;
    }

    // Look for SOF pattern (55 AA in little endian)
    for (uint16_t i = 0; i <= frame_index - 2; i++) {
        if (frame_buffer[i] == 0x55 && frame_buffer[i + 1] == 0xAA) {
            if (i > 0) {
                // Move frame to beginning of buffer
                memmove(frame_buffer, &frame_buffer[i], frame_index - i);
                frame_index -= i;
            }
            return true;
        }
    }

    // No SOF found, keep only the last byte in case it's the start of SOF
    if (frame_index > 1) {
        frame_buffer[0] = frame_buffer[frame_index - 1];
        frame_index = 1;
    }

    return false;
}

/**
 * @brief  Get expected frame length from buffer
 * @param  buffer: Buffer containing frame start
 * @retval Expected frame length or 0 if cannot determine
 */
static uint16_t factory_test_get_frame_length(const uint8_t* buffer) {
    if (frame_index < 7) {    // Need at least SOF(2) + MSG(3) + LEN(2)
        return 0;
    }

    // Extract payload length (Little Endian)
    uint16_t payload_len = buffer[FRAME_PAYLOAD_LEN_OFFSET] |
                           (buffer[FRAME_PAYLOAD_LEN_OFFSET + 1] << 8);

    if (payload_len > FACTORY_TEST_MAX_PAYLOAD) {
        return 0;
    }

    // Total frame length: SOF(2) + MSG(3) + LEN(2) + PAYLOAD + CRC(2) + EOF(2)
    return 11 + payload_len;
}

/**
 * @brief  Create response frame
 * @param  response: Pointer to response frame structure
 * @param  msg_id: Message ID
 * @param  payload: Payload data
 * @param  payload_len: Payload length
 * @retval None
 */
static void factory_test_create_response_frame(factory_test_frame_t* response,
                                               uint8_t msg_id,
                                               const uint8_t* payload,
                                               uint16_t payload_len) {
    response->sof = FACTORY_TEST_SOF;
    response->source = DEVICE_ADDR_BOARD;
    response->target = DEVICE_ADDR_TOOLING;
    response->msg_id = msg_id;
    response->payload_len = payload_len;

    if (payload && payload_len > 0) {
        memcpy(response->payload, payload, payload_len);
    }

    response->eof = FACTORY_TEST_EOF;
}

/* Ring buffer functions ---------------------------------------------------- */

/**
 * @brief  Put data into ring buffer (interrupt-safe)
 * @param  data: Data byte to store
 * @retval true if successful, false if buffer full
 */
static bool ring_buffer_put(uint8_t data) {
    uint16_t next_head = (ring_head + 1) % RING_BUFFER_SIZE;

    if (next_head == ring_tail) {
        // Buffer full
        return false;
    }

    ring_buffer[ring_head] = data;
    ring_head = next_head;
    return true;
}

/**
 * @brief  Get data from ring buffer
 * @param  data: Pointer to store retrieved data
 * @retval true if data available, false if buffer empty
 */
static bool ring_buffer_get(uint8_t* data) {
    if (ring_head == ring_tail) {
        // Buffer empty
        return false;
    }

    *data = ring_buffer[ring_tail];
    ring_tail = (ring_tail + 1) % RING_BUFFER_SIZE;
    return true;
}

/**
 * @brief  Get number of bytes available in ring buffer
 * @retval Number of bytes available
 */
__attribute__((unused)) static uint16_t ring_buffer_available(void) {
    return (ring_head + RING_BUFFER_SIZE - ring_tail) % RING_BUFFER_SIZE;
}

/**
 * @brief  Reset ring buffer
 * @retval None
 */
static void ring_buffer_reset(void) {
    ring_head = 0;
    ring_tail = 0;
}
