#ifndef __LTLP_API_H__
#define __LTLP_API_H__

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include "ltlp_def.h"
#include <stdbool.h>
void ltlpInit(LtlpBasicSettingDef* setting);
bool ltlpSendData(uint8_t msgType, LtlpID_t destID, uint8_t* pData, uint32_t len);
void ltlpSetCallback(LtlpCallbackTypeDef type, LtlpCallback callback, void* usrParm);
void ltlpParse(uint8_t* pData, uint32_t len);
void ltlpSetLogger(LtlpLogCallback callback);
void ltlpSetLocalID(LtlpID_t id);
void ltlpRun(void);
#ifdef __cplusplus
}
#endif    // __cplusplus
#endif    // __LTLP_API_H__
