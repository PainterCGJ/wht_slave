#include "ltlp_api.h"
#include "ltlp_common.h"
#include "ltlp_kernel.h"
#include "ltlp_sender.h"
void ltlpInit(LtlpBasicSettingDef* setting) {
    g_setting = *setting;
    for(int i=0;i<LTLP_CALLBACK_NUM;i++){
        g_callbacks[i] = NULL;
        g_callbackParm[i] = NULL;
    }
    ltlpKernelReset();
    ltlpSenderReset();
}
void ltlpSetLogger(LtlpLogCallback callback) { ltlpSetLogCallback(callback); }

bool ltlpSendData(uint8_t msgType, LtlpID_t destID, uint8_t* pData, uint32_t len) {
    return ltlpKernelStartToSend(msgType, destID, pData, len);
}

void ltlpSetCallback(LtlpCallbackTypeDef type, LtlpCallback callback, void* usrParm){
    if(type>=LTLP_CALLBACK_NUM){
        return;
    }
    g_callbacks[type] = callback;
    g_callbackParm[type] = usrParm;
}

void ltlpParse(uint8_t* pData, uint32_t len){
    ltlpKernelParse(pData, len);
}
void ltlpRun(){
    ltlpKernelRun();
}
void ltlpSetLocalID(LtlpID_t id){
    g_setting.localID = id;
}
