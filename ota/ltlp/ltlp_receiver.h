#ifndef __LTLP_RECEIVER_H__
#define __LTLP_RECEIVER_H__

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include <stdint.h>
#include "ltlp_def.h"
typedef enum {
    RECEIVER_IDLE = 0,
    RECEIVER_RECVING,
} ReceiverSateDef;
ReceiverSateDef ltlpReceiverRun(void);
void ltlpReceiverHandleFrame(LtlpFrame* pFrame);
#ifdef __cplusplus
}
#endif    // __cplusplus

#endif    // __LTLP_RECEIVER_H__
