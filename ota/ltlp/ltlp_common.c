#include "ltlp_common.h"

#include "../printf/printf.h"
LtlpBasicSettingDef g_setting;
LtlpCallback g_callbacks[LTLP_CALLBACK_NUM] = {NULL};
void* g_callbackParm[LTLP_CALLBACK_NUM] = {NULL};
static LtlpLogCallback g_logCallback = NULL;
void ltlpSetLogCallback(LtlpLogCallback callback) { g_logCallback = callback; }
void ltlpLogger(const char* format, ...) {
    if (g_logCallback == NULL) {
        return;    // 没有设置回调，静默退出
    }

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // 调用回调函数
    g_logCallback(buffer);
}

static uint16_t crc16_modbus(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;    // CRC初始值
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
static uint16_t caculateFrameNum(uint32_t totalLen) {
    uint16_t totalFrameNum = 0;
    totalFrameNum = (totalLen + LTLP_MAX_PAYLOAD_LEN - 1) / LTLP_MAX_PAYLOAD_LEN;
    if (totalFrameNum == 0) {
        totalFrameNum = 1;
    }
    return totalFrameNum;
}
uint16_t ltlpCrc(uint8_t* pData, uint32_t len) { return crc16_modbus(pData, len); }
void ltlpGetReadyToSend(LtlpSendCrtl* pSendCtrl, uint8_t frameType, uint8_t msgType, LtlpID_t destID,
                        LtlpNeedAckTypeDef needAck, const uint8_t* pTotalPayload, uint32_t totalLen) {
    pSendCtrl->frame.header.sof = LTLP_SOF;
    pSendCtrl->frame.header.info.srcID = g_setting.localID;
    pSendCtrl->frame.header.info.destID = destID;
    pSendCtrl->frame.header.info.frameType = frameType;
    pSendCtrl->frame.header.info.msgType = msgType;
    pSendCtrl->frame.header.info.totalFrames = caculateFrameNum(totalLen);
    pSendCtrl->frame.header.info.seqNum = 0;
    pSendCtrl->frame.header.info.needAck = needAck;
    pSendCtrl->pTotalPayload = (uint8_t*)pTotalPayload;
    pSendCtrl->totalLen = totalLen;
    pSendCtrl->frame.pFramePayload = pSendCtrl->pTotalPayload;
    pSendCtrl->sendFrameCnt = 0;
}

void ltlpSendOneFrame(LtlpSendCrtl* pSendCtrl) {
    if (pSendCtrl->tryTimes == 0) {
        pSendCtrl->frame.header.info.seqNum = pSendCtrl->sendFrameCnt;
        if (pSendCtrl->frame.header.info.seqNum == pSendCtrl->frame.header.info.totalFrames - 1) {
            // 最后一帧
            uint32_t remainingLen = pSendCtrl->totalLen - pSendCtrl->sendFrameCnt * LTLP_MAX_PAYLOAD_LEN;
            pSendCtrl->frame.header.info.payloadLen = (uint16_t)remainingLen;
            pSendCtrl->frame.pFramePayload = pSendCtrl->pTotalPayload + pSendCtrl->sendFrameCnt * LTLP_MAX_PAYLOAD_LEN;
        } else {
            pSendCtrl->frame.header.info.payloadLen = LTLP_MAX_PAYLOAD_LEN;
            pSendCtrl->frame.pFramePayload = pSendCtrl->pTotalPayload + pSendCtrl->sendFrameCnt * LTLP_MAX_PAYLOAD_LEN;
        }

        pSendCtrl->frame.header.headerCRC =
            crc16_modbus((uint8_t*)pSendCtrl->frame.header.headerBytes, sizeof(pSendCtrl->frame.header.headerBytes));
        g_setting.sendPortFunc((uint8_t*)&pSendCtrl->frame.header, sizeof(LtlpHeaderDef), g_setting.sendPortUsrParm);
        g_setting.sendPortFunc(pSendCtrl->frame.pFramePayload, pSendCtrl->frame.header.info.payloadLen,
                               g_setting.sendPortUsrParm);

        if (pSendCtrl->frame.header.info.payloadLen > 0) {
            pSendCtrl->frame.crc =
                crc16_modbus(pSendCtrl->frame.pFramePayload, pSendCtrl->frame.header.info.payloadLen);
            g_setting.sendPortFunc((uint8_t*)&pSendCtrl->frame.crc, sizeof(uint16_t), g_setting.sendPortUsrParm);
        }
    } else {
        g_setting.sendPortFunc((uint8_t*)&pSendCtrl->frame.header, sizeof(LtlpHeaderDef), g_setting.sendPortUsrParm);
        g_setting.sendPortFunc(pSendCtrl->frame.pFramePayload, pSendCtrl->frame.header.info.payloadLen,
                               g_setting.sendPortUsrParm);
        if (pSendCtrl->frame.header.info.payloadLen > 0) {
            g_setting.sendPortFunc((uint8_t*)&pSendCtrl->frame.crc, sizeof(uint16_t), g_setting.sendPortUsrParm);
        }
    }
    
    pSendCtrl->tryTimes++;
}

void ltlpHandshake(LtlpSendCrtl* pSendCtrl, LtlpID_t destID) {
    ltlpGetReadyToSend(pSendCtrl, LTLP_SYS_FRAME, LTLP_SYS_FRAME_HANDSHAKE, destID, LTLP_NEED_ACK_YES, NULL, 0);
    ltlpSendOneFrame(pSendCtrl);
}

void ltlpSendNack(LtlpSendCrtl* pSendCtrl, LtlpID_t destID) {
    ltlpGetReadyToSend(pSendCtrl, LTLP_SYS_FRAME, LTLP_SYS_FRAME_NACK, destID, LTLP_NEED_ACK_NO, NULL, 0);
    ltlpSendOneFrame(pSendCtrl);
}

void ltlpSendAck(LtlpSendCrtl* pSendCtrl, LtlpID_t destID) {
    ltlpGetReadyToSend(pSendCtrl, LTLP_SYS_FRAME, LTLP_SYS_FRAME_ACK, destID, LTLP_NEED_ACK_NO, NULL, 0);
    ltlpSendOneFrame(pSendCtrl);
}

uint32_t ltlpNowTime() { return g_setting.nowTimeFunc(g_setting.nowTimeUsrParm); }
