#ifndef __LTLP_COMMON_H__
#define __LTLP_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include <stdint.h>
#include <stdio.h>

#include "ltlp_def.h"

extern LtlpBasicSettingDef g_setting;
extern LtlpCallback g_callbacks[LTLP_CALLBACK_NUM];
extern void* g_callbackParm[LTLP_CALLBACK_NUM];

void ltlpGetReadyToSend(LtlpSendCrtl* sendCtrl, uint8_t frameType, uint8_t msgType, LtlpID_t destID,
                        const uint8_t* totalPayload, uint32_t totalLen);
void ltlpSendOneFrame(LtlpSendCrtl* sendCtrl);
void ltlpHandshake(LtlpSendCrtl* pSendCtrl, LtlpID_t destID);
void ltlpSetLogCallback(LtlpLogCallback callback);
void ltlpSendNack(LtlpSendCrtl* pSendCtrl, LtlpID_t destID);
void ltlpSendAck(LtlpSendCrtl* pSendCtrl, LtlpID_t destID);
uint16_t ltlpCrc(uint8_t* pData, uint32_t len);
void ltlpLogger(const char* format, ...);

uint32_t ltlpNowTime(void);

#ifdef __cplusplus
}
#endif    // __cplusplus
#endif    // __LTLP_COMMON_H__
