#ifndef __LTLP_FRAME_DEF_H__
#define __LTLP_FRAME_DEF_H__

/* 用户可配置参数 */
#define LTLP_MTU_SIZE 1000    // 最大传输单元大小
#define LTLP_MAX_PAYLOAD_LEN \
    (LTLP_MTU_SIZE - sizeof(LtlpHeaderDef) - 2)    // 最大负载长度
#define LTLP_SOF             0xAA55                // 帧头
#define LTLP_TIMEOUT_MS      3000                  // 超时时间1秒
#define LTLP_MAX_RETRY_TIMES 3                     // 最大重试次数
#define LTLP_BROADCAST_ID 0xFFFFFFFF              // 广播ID


#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t LtlpID_t;
typedef enum {
    LTLP_SYS_FRAME = 0x00,
    LTLP_APP_FRAME,

    LTLP_FRAME_TYPE_NUM,
} LtlpFrameTypeDef;

typedef enum{
    LTLP_NEED_ACK_NO = 0,    // 不需要ACK
    LTLP_NEED_ACK_YES,       // 需要ACK
}LtlpNeedAckTypeDef;

typedef enum {
    LTLP_SYS_FRAME_ACK = 0x00,
    LTLP_SYS_FRAME_NACK,
    LTLP_SYS_FRAME_HANDSHAKE,
} LtlpSysFrameMsgTypeDef;

typedef enum {
    LTLP_SENDER = 0,
    LTLP_RECEIVER,
} LtlpRoleDef;

typedef enum {
    // sender callbacks
    LTLP_CALLBACK_ON_HANDSHAKE_ERROR = 0,
    LTLP_CALLBACK_ON_SEND_ONE_FRAME_DONE,
    LTLP_CALLBACK_ON_SEND_ALL_DATA_DONE,
    LTLP_CALLBACK_ON_RECV_ACK,
    LTLP_CALLBACK_ON_WAIT_ACK_TIMEOUT,
    LTLP_CALLBACK_ON_RECV_NACK,
    LTLP_CALLBACK_ON_SEND_ERROR,

    // receiver callbacks
    LTLP_CLLBACK_ON_RECV_ONE_FRAME,
    LTLP_CLLBACK_ON_RECV_ALL_DATA,
    LTLP_CALBACK_ON_RECV_TIMEOUT,

    LTLP_CALLBACK_NUM
} LtlpCallbackTypeDef;

typedef uint32_t (*NowTime)(void* usrParm);
typedef void (*LtlpSendPort)(uint8_t* pData, uint32_t len, void* usrParm);
typedef void (*LtlpLogCallback)(const char* message);
#pragma pack(push, 1)
typedef struct {
    LtlpID_t srcID;          // 源ID
    LtlpID_t destID;         // 目标ID
    uint8_t frameType;       // 帧类型
    uint8_t msgType;         // 消息类型，用户自定义
    uint16_t totalFrames;    // 总帧数;
    uint8_t needAck;         // 是否需要ACK;
    uint16_t seqNum;         // 帧序号;
    uint16_t payloadLen;     // 有效数据长度;
} LtlpFrameInfoDef;
typedef struct {
    union {
        struct {
            uint16_t sof;
            LtlpFrameInfoDef info;
        };
        uint8_t headerBytes[sizeof(uint16_t) + sizeof(LtlpFrameInfoDef)];
    };
    uint16_t headerCRC;
} LtlpHeaderDef;    // 帧头定义

#pragma pack(pop)

typedef struct {
    LtlpSendPort sendPortFunc;      // 发送函数指针
    void* sendPortUsrParm;          // 发送函数参数
    NowTime nowTimeFunc;            // 获取当前时间函数指针
    void* nowTimeUsrParm;           // 获取当前时间函数参数
    LtlpID_t localID;                // 本地ID
    uint8_t* pAssambleBuffer;       // 组包缓存指针，若不需要组包，可设为NULL
    uint32_t assambleBufferSize;    // 组包缓存大小，若不需要组包，可设为0
} LtlpBasicSettingDef;

typedef struct {
    LtlpHeaderDef header;
    uint8_t* pFramePayload;
    uint16_t crc;
} LtlpFrame;

typedef struct {
    LtlpFrame frame;
    uint8_t* pTotalPayload;
    uint32_t totalLen;
    uint16_t sendFrameCnt;
    uint8_t tryTimes;
} LtlpSendCrtl;

typedef struct {
    LtlpFrame frame;
    uint32_t recvFrameCnt;
    uint32_t time;
} LtlpRecvCrtl;

typedef struct {
    LtlpFrame frame;
    uint8_t payload[LTLP_MAX_PAYLOAD_LEN];
    uint32_t recvHeaderLen;
    uint32_t recvPayloadLen;
    uint32_t time;
} LtlpPaserCtrl;

typedef void (*LtlpCallback)(LtlpFrame* pFrame, void* usrParm);

#ifdef __cplusplus
}
#endif    // __cplusplus

#endif    // __LTLP_FRAME_DEF_H__
