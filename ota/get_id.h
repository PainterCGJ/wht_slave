#ifndef GET_ID_H
#define GET_ID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取32位设备ID
 * @return 32位设备ID
 * 
 * 获取优先级：
 * 1. 优先返回Flash中SN存储结构中的batch_id（如果SN已编程）
 * 2. 如果SN未编程或魔数无效，返回硬件UID作为备选
 */
uint32_t get_device_id(void);

#ifdef __cplusplus
}
#endif

#endif /* GET_ID_H */

