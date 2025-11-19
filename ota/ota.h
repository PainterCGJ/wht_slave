#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_LOADER_START_ADDR 0x08000000
#define BOOT_LOADER_END_ADDR   0x08007FFF
#define BOOT_LOADER_SIZE       (BOOT_LOADER_END_ADDR - BOOT_LOADER_START_ADDR + 1)

#define APP_START_ADDR 0x08008000
#define APP_OFFSET     (APP_START_ADDR - BOOT_LOADER_START_ADDR)
#define APP_END_ADDR   0x0819FFFF    // sector 20

#define BOOTLOADER_FLAG_SECTOR 23    // Flash扇区23
#define BOOTLOADER_FLAG_ADDRESS \
    0x081FFF00UL    // 扇区23末尾预留256字节用于标志位
#define BOOTLOADER_FLAG_MAGIC   0x12345678UL    // 魔数，用于验证标志位的有效性
#define APP_FLAG        0x12345678UL    // 应用程序标志
#define BOOTLOADER_FLAG 0xABCDEF00UL    // 引导程序标志

typedef struct {
    uint32_t magic;    // 魔数，用于验证结构的有效性
    uint32_t flag;    // 标志位，用于指示当前程序是应用程序还是引导程序，只有等于APP_FLAG才进入应用程序
} bootloader_flag_t;

void ota_enter_bootloader(void);

#ifdef __cplusplus
}
#endif

#endif    // __OTA_H__