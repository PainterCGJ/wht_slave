#ifndef __LTLP_SENDER_H__
#define __LTLP_SENDER_H__

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include <stdint.h>

#include "ltlp_def.h"
typedef enum {
    SENDER_IDLE = 0,
    SENDER_HANDSHKE,
    SENDER_SENDING,
    SNEDER_WAIT_ACK,
} SenderSateDef;

void ltlpSenderReset(void);
SenderSateDef ltlpSenderRun(void);
void ltlpStartToSend(uint8_t msgType, LtlpID_t destID, LtlpNeedAckTypeDef needAck, const uint8_t* payload, uint32_t payloadLen);
void ltlpSenderHandleSysFrame(LtlpFrame* pFrame);
#ifdef __cplusplus
}
#endif    // __cplusplus

#endif    // __LTLP_SENDER_H__
