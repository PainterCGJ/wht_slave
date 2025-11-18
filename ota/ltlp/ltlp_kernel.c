#include "ltlp_kernel.h"

#include "ltlp_common.h"
#include "ltlp_def.h"
#include "ltlp_receiver.h"
#include "ltlp_sender.h"
// static struct {
//     LtlpHeaderDef header[LTLP_FRAME_TYPE_NUM];
//     uint8_t* pFramePayload[LTLP_FRAME_TYPE_NUM];
//     uint8_t* pTotalPayload[LTLP_FRAME_TYPE_NUM];
//     uint32_t totalPayloadLen[LTLP_FRAME_TYPE_NUM];
//     uint16_t sendFrameCnt;
// } g_sendCtrl = {0};

typedef enum {
    KERNEL_IDLE = 0,
    KERNEL_SENDING,
    KERNEL_RECEIVING,
} KernelStateDef;
typedef enum {
    KERNEL_PARSE_HEADER = 0,
    KERNEL_PARSE_PAYLOAD,
} KernelParseStateDef;

struct WaitToSend{
    uint8_t msgType;
    LtlpID_t destID;
    uint8_t* payload;
    uint32_t payloadLen;
    uint8_t waitFlag;// 0: 未等待 1: 等待中
}g_waitToSend;

KernelStateDef g_kernelState = KERNEL_IDLE;
KernelParseStateDef g_kernelParseState = KERNEL_PARSE_HEADER;
LtlpPaserCtrl g_parserCtrl = {0};

void ltlpResetParser() {
    memset(&g_parserCtrl, 0, sizeof(g_parserCtrl));
    g_kernelParseState = KERNEL_PARSE_HEADER;
    g_parserCtrl.frame.pFramePayload = g_parserCtrl.payload;
}

void ltlpKernelReset() {
    g_kernelState = KERNEL_IDLE;
    memset(&g_waitToSend, 0, sizeof(g_waitToSend));
    ltlpResetParser();
}
bool ltlpKernelStartToSend(uint8_t msgType, LtlpID_t destID, const uint8_t* payload, uint32_t payloadLen) {
    if (g_kernelState != KERNEL_IDLE) {
        if(g_waitToSend.waitFlag==0){
            g_waitToSend.msgType = msgType;
            g_waitToSend.destID = destID;
            g_waitToSend.payload = (uint8_t*)payload;
            g_waitToSend.payloadLen = payloadLen;
            g_waitToSend.waitFlag = 1;
            ltlpLogger("warning: ltlp kernel start to send, wait to send");
            return true;
        }
        return false;
    }
    ltlpStartToSend(msgType, destID, payload, payloadLen);
    g_kernelState = KERNEL_SENDING;
    return true;
}

void ltlpKernelHandleFrame(LtlpFrame* pFrame) {
    switch (g_kernelState) {
        case KERNEL_IDLE: {
            if (pFrame->header.info.frameType == LTLP_SYS_FRAME &&
                pFrame->header.info.msgType == LTLP_SYS_FRAME_HANDSHAKE) {
                // 收到握手请求，回复ACK
                ltlpReceiverHandleFrame(pFrame);
                g_kernelState = KERNEL_RECEIVING;
            }
            break;
        }
        case KERNEL_SENDING: {
            ltlpSenderHandleSysFrame(pFrame);
            break;
        }
        case KERNEL_RECEIVING: {
            ltlpReceiverHandleFrame(pFrame);
            break;
        }
    }
}
uint8_t data = 0, sof = 0;
void ltlpKernelParse(uint8_t* pData, uint32_t len) {
    g_parserCtrl.time = ltlpNowTime();
    for (uint32_t i = 0; i < len; i++) {
        data = pData[i];
        switch (g_kernelParseState) {
            case KERNEL_PARSE_HEADER: {
                ((uint8_t*)(&g_parserCtrl.frame.header))[g_parserCtrl.recvHeaderLen] = data;
                if (g_parserCtrl.recvHeaderLen == 0) {
                    // 寻找SOF
                    if (data == (LTLP_SOF & 0xFF)) {
                        g_parserCtrl.recvHeaderLen++;
                    }
                } else if (g_parserCtrl.recvHeaderLen == 1) {
                    sof = ((LTLP_SOF >> 8) & 0xFF);
                    if (data == sof) {
                        g_parserCtrl.recvHeaderLen++;
                    } else {
                        // 未找到，继续寻找SOF
                        g_parserCtrl.recvHeaderLen = 0;
                    }
                } else {
                    g_parserCtrl.recvHeaderLen++;
                    if (g_parserCtrl.recvHeaderLen == sizeof(LtlpHeaderDef)) {
                        if (g_parserCtrl.frame.header.headerCRC ==
                            ltlpCrc((uint8_t*)g_parserCtrl.frame.header.headerBytes,
                                    sizeof(g_parserCtrl.frame.header.headerBytes))) {
                            // 帧头接收完成，且CRC校验通过
                            if (g_parserCtrl.frame.header.info.destID != g_setting.localID) {
                                // 目标ID不匹配，丢弃该帧
                                ltlpResetParser();
                                ltlpLogger("ltlp kernel parse frame destID mismatch, drop frame");
                            } else {
                                if (g_parserCtrl.frame.header.info.payloadLen > 0) {
                                    g_kernelParseState = KERNEL_PARSE_PAYLOAD;
                                    g_parserCtrl.recvPayloadLen = 0;
                                } else {
                                    // 无负载，直接处理帧
                                    ltlpKernelHandleFrame(&g_parserCtrl.frame);
                                    ltlpResetParser();
                                }
                            }

                        } else {
                            // CRC校验失败，重置解析状态
                            ltlpLogger("ltlp frame header crc %x", g_parserCtrl.frame.header.headerCRC);
                            ltlpLogger("ltlp frame check crc %x", ltlpCrc((uint8_t*)g_parserCtrl.frame.header.headerBytes, sizeof(g_parserCtrl.frame.header.headerBytes)));
                            ltlpLogger("ltlp kernel parse header crc error");
                            ltlpResetParser();
                        }
                    }
                }
                break;
            }
            case KERNEL_PARSE_PAYLOAD: {
                // 解析payload
                if (g_parserCtrl.recvPayloadLen < g_parserCtrl.frame.header.info.payloadLen) {
                    g_parserCtrl.frame.pFramePayload[g_parserCtrl.recvPayloadLen++] = data;
                } else if (g_parserCtrl.recvPayloadLen < g_parserCtrl.frame.header.info.payloadLen + 2) {
                    // 解析CRC
                    uint32_t crcIndex = g_parserCtrl.recvPayloadLen - g_parserCtrl.frame.header.info.payloadLen;
                    ((uint8_t*)(&g_parserCtrl.frame.crc))[crcIndex] = data;
                    g_parserCtrl.recvPayloadLen++;
                    if (crcIndex == 1) {
                        // CRC解析完成，校验
                        if (g_parserCtrl.frame.crc ==
                            ltlpCrc(g_parserCtrl.frame.pFramePayload, g_parserCtrl.frame.header.info.payloadLen)) {
                            // CRC校验通过，处理帧
                            ltlpKernelHandleFrame(&g_parserCtrl.frame);
                            ltlpResetParser();
                        } else {
                            // CRC校验失败，重置解析状态
                            ltlpLogger("ltlp kernel parse frame crc error");
                            ltlpLogger("ltlp kernel parse frame crc %x", g_parserCtrl.frame.crc);
                            ltlpLogger("ltlp kernel parse frame check crc %x", ltlpCrc(g_parserCtrl.frame.pFramePayload, g_parserCtrl.frame.header.info.payloadLen));
                            LtlpSendCrtl sendCrtl = {0};
                            ltlpSendNack(&sendCrtl, g_parserCtrl.frame.header.info.srcID);
                            ltlpResetParser();
                        }
                    }
                }

                break;
            }
            default: {
                break;
            }
        }
    }
}

void ltlpKernelRun() {
    switch (g_kernelState) {
        case KERNEL_IDLE: {
            if (g_waitToSend.waitFlag == 1) {
                ltlpKernelStartToSend(g_waitToSend.msgType, g_waitToSend.destID, g_waitToSend.payload, g_waitToSend.payloadLen);
                g_waitToSend.waitFlag = 0;
                ltlpLogger("ltlp kernel run, start to send");
            }
            break;
        }
        case KERNEL_SENDING: {
            if (ltlpNowTime() - g_parserCtrl.time > LTLP_TIMEOUT_MS * (LTLP_MAX_RETRY_TIMES + 1)) {
                ltlpResetParser();
            }
            if (ltlpSenderRun() == SENDER_IDLE) {
                // 发送完成，回到空闲状态
                ltlpResetParser();
                g_kernelState = KERNEL_IDLE;
            }
            break;
        }
        case KERNEL_RECEIVING: {
            if (ltlpNowTime() - g_parserCtrl.time > LTLP_TIMEOUT_MS * (LTLP_MAX_RETRY_TIMES + 1)) {
                ltlpResetParser();
            }
            if (ltlpReceiverRun() == RECEIVER_IDLE) {
                // 接收完成，回到空闲状态
                ltlpResetParser();
                g_kernelState = KERNEL_IDLE;
            }
            break;
        }
    }
}
