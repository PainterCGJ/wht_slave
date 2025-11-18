#include "get_id.h"
#include <stdint.h>

/* SN Storage Constants - 与factory_test.h保持一致 */
#define SN_FLASH_ADDRESS           0x081C0000UL          /* Sector 22起始地址 */
#define SN_MAGIC_NUMBER            0x5AA5A55AUL          /* SN存储的魔数 */
#define SN_MAX_LENGTH              64                    /* SN最大长度 */

/* SN storage structure in Flash - 与factory_test.h保持一致 */
typedef struct {
    uint32_t magic;         /* 魔数验证 */
    uint32_t batch_id;       /* 批次ID */
    uint8_t sn_length;      /* SN长度 */
    uint8_t sn_data[SN_MAX_LENGTH];  /* SN数据 */
    uint8_t reserved[3];    /* 保留字节，用于32位对齐 */
} sn_storage_t;

/**
 * @brief 获取32位设备ID
 * @return 32位设备ID
 * 
 * 获取优先级：
 * 1. 优先返回Flash中SN存储结构中的batch_id（如果SN已编程）
 * 2. 如果SN未编程或魔数无效，返回硬件UID作为备选
 */
uint32_t get_device_id(void)
{
    // 从Flash中读取SN存储结构
    const sn_storage_t *sn_storage = (const sn_storage_t *)SN_FLASH_ADDRESS;
    
    // 检查魔数是否有效
    if (sn_storage->magic == SN_MAGIC_NUMBER) {
        // 返回Flash中的batch_id
        return sn_storage->batch_id;
    }
    
    // 如果SN未编程或魔数无效，返回硬件UID作为备选
    // 硬件UID地址：0x1FFF7A10 (STM32F4xx UID_BASE)
    const uint32_t *uid_address = (const uint32_t *)0x1FFF7A10UL;
    return *uid_address;
}

