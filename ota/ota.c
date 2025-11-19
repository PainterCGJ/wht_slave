/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : ota.c
 * @brief          : OTA (Over-The-Air) update implementation
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
#include "ota.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef flash_erase_flag_sector(void);
static HAL_StatusTypeDef flash_write_flag_data(const bootloader_flag_t* flag_data);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Erase the Flash sector containing bootloader flag7
 * @retval HAL_OK if successful, HAL_ERROR otherwise
 */
static HAL_StatusTypeDef flash_erase_flag_sector(void) {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;

    // 解锁Flash
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        printf("Flash unlock failed: %d\r\n", status);
        return status;
    }

    // 配置擦除参数
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // 2.7V-3.6V
    EraseInitStruct.Sector = BOOTLOADER_FLAG_SECTOR;
    EraseInitStruct.NbSectors = 1;

    // 执行扇区擦除
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    if (status != HAL_OK) {
        printf("Flash erase failed: %d, sector error: %u\r\n", status, (unsigned int)SectorError);
        HAL_FLASH_Lock();
        return status;
    }

    // 锁定Flash
    HAL_FLASH_Lock();
    
    printf("Flash sector %lu erased successfully\r\n", (unsigned long)BOOTLOADER_FLAG_SECTOR);
    return HAL_OK;
}

/**
 * @brief  Write bootloader flag data to Flash
 * @param  flag_data: Pointer to flag data structure
 * @retval HAL_OK if successful, HAL_ERROR otherwise
 */
static HAL_StatusTypeDef flash_write_flag_data(const bootloader_flag_t* flag_data) {
    HAL_StatusTypeDef status;
    uint32_t address = BOOTLOADER_FLAG_ADDRESS;

    // 解锁Flash
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        printf("Flash unlock failed: %d\r\n", status);
        return status;
    }

    // 写入magic字段 (32位)
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, flag_data->magic);
    if (status != HAL_OK) {
        printf("Flash write magic failed: %d\r\n", status);
        HAL_FLASH_Lock();
        return status;
    }

    // 写入flag字段 (32位)
    address += 4;
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, flag_data->flag);
    if (status != HAL_OK) {
        printf("Flash write flag failed: %d\r\n", status);
        HAL_FLASH_Lock();
        return status;
    }

    // 锁定Flash
    HAL_FLASH_Lock();
    
    printf("Bootloader flag written to Flash at 0x%08lX\r\n", (unsigned long)BOOTLOADER_FLAG_ADDRESS);
    return HAL_OK;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Enter bootloader mode by setting flag in Flash
 * @note   This function erases the bootloader flag sector, writes APP_FLAG to indicate
 *         application mode, and prepares the system for bootloader entry
 * @retval None
 */
void ota_enter_bootloader(void) {
    bootloader_flag_t flag_data;
    HAL_StatusTypeDef status;

    // 准备标志位数据
    flag_data.magic = BOOTLOADER_FLAG_MAGIC;
    flag_data.flag = APP_FLAG;

    printf("Entering bootloader mode...\r\n");
    printf("Setting bootloader flag: magic=0x%08lX, flag=0x%08lX\r\n", 
           (unsigned long)flag_data.magic, (unsigned long)flag_data.flag);

    // 先擦除Flash扇区
    status = flash_erase_flag_sector();
    if (status != HAL_OK) {
        printf("Failed to erase Flash sector for bootloader flag\r\n");
        return;
    }

    // 写入标志位数据
    status = flash_write_flag_data(&flag_data);
    if (status != HAL_OK) {
        printf("Failed to write bootloader flag to Flash\r\n");
        return;
    }

    printf("Bootloader flag set successfully at address 0x%08lX\r\n", 
           (unsigned long)BOOTLOADER_FLAG_ADDRESS);
}


