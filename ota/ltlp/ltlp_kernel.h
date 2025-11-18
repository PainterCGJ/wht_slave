#ifndef __LTLP_KERNEL_H__
#define __LTLP_KERNEL_H__

#ifdef __cplusplus
extern "C" {
#endif    // __cplusplus
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "ltlp_common.h"
#include "ltlp_def.h"
void ltlpKernelReset(void);
bool ltlpKernelStartToSend(uint8_t msgType, LtlpID_t destID, const uint8_t* payload, uint32_t payloadLen);
void ltlpKernelParse(uint8_t* pData, uint32_t len);
void ltlpKernelRun(void);
#ifdef __cplusplus
}
#endif    // __cplusplus

#endif    // __LTLP_KERNEL_H__
