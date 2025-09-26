#pragma once
#include <stdint.h>

extern "C" {
    #include "../../factory_test/factory_test.h"
}

class DeviceUID
{
  public:
    static uint32_t get()
    {
        static uint32_t value = [] {
            // 从Flash中读取SN存储结构中的batch_id
            const auto *sn_storage = reinterpret_cast<const sn_storage_t *>(SN_FLASH_ADDRESS);
            
            // 检查魔数是否有效
            if (sn_storage->magic == SN_MAGIC_NUMBER) {
                return sn_storage->batch_id;
            }
            
            // 如果SN未编程或魔数无效，返回硬件UID作为备选
            const auto *uid_address = reinterpret_cast<const uint32_t *>(0x1FFF7A10);
            return *uid_address;
        }();
        return value;
    }
    ~DeviceUID() = delete;
    DeviceUID() = delete;
};
