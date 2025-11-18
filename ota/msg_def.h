#ifndef __MSG_DEF_H__
#define __MSG_DEF_H__

#include <stdint.h>
#define HOST_ID 0x00
typedef enum {
    MSG_LINK = 0,
    MSG_ENTER_UPGRADE,
    MSG_FIRMWARE_INFO,
    MSG_FIRMWARE_DATA,
    MSG_RESPONCE_LINK,
    MSG_RESPONCE_INFO,
}MsgType;

#pragma pack(push, 1)


typedef struct{
    uint32_t firmwareSize;
    uint32_t version;
}FirmwarwInfo;
#pragma pack(pop)

#endif // __MSG_DEF_H__
