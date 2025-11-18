#include "ltlp_sender.h"

#include "ltlp_common.h"
LtlpSendCrtl g_sendCtrl[LTLP_FRAME_TYPE_NUM] = {0};
uint32_t g_lastSendTime = 0;
SenderSateDef g_senderState = SENDER_IDLE;
void ltlpSenderReset() {
    g_senderState = SENDER_IDLE;
    memset(g_sendCtrl, 0, sizeof(g_sendCtrl));
}
void ltlpResetSendCtrl(LtlpSendCrtl* pCtrl) { memset(pCtrl, 0, sizeof(LtlpSendCrtl)); }
void ltlpSenderHandleFrame(LtlpFrame* pFrame) {}
void ltlpStartToSend(uint8_t msgType, LtlpID_t destID, const uint8_t* payload, uint32_t payloadLen) {
    if (g_senderState != SENDER_IDLE) {
        return;
    }
    ltlpResetSendCtrl(&g_sendCtrl[LTLP_SYS_FRAME]);
    ltlpResetSendCtrl(&g_sendCtrl[LTLP_APP_FRAME]);
    ltlpGetReadyToSend(&g_sendCtrl[LTLP_APP_FRAME], LTLP_APP_FRAME, msgType, destID, payload, payloadLen);
    g_senderState = SENDER_HANDSHKE;
    g_lastSendTime = ltlpNowTime();
    ltlpHandshake(&g_sendCtrl[LTLP_SYS_FRAME], destID);
}
void ltlpSenderHandleSysFrame(LtlpFrame* pFrame) {
    switch (g_senderState) {
        case SENDER_IDLE: {
            // 空闲状态
            // 等待发送
            break;
        }
        case SENDER_HANDSHKE: {
            // 握手状态
            if (pFrame->header.info.msgType == LTLP_SYS_FRAME_ACK) {
                // 握手成功，开始发送数据帧
                g_senderState = SENDER_SENDING;
            } else if (pFrame->header.info.msgType == LTLP_SYS_FRAME_NACK) {
                // 握手失败
                ltlpLogger("ltlp sender handshake error");
                if (g_callbacks[LTLP_CALLBACK_ON_HANDSHAKE_ERROR] != NULL) {
                    g_callbacks[LTLP_CALLBACK_ON_HANDSHAKE_ERROR](pFrame,
                                                                  g_callbackParm[LTLP_CALLBACK_ON_HANDSHAKE_ERROR]);
                }
                ltlpSenderReset();
            }
            break;
        }
        case SENDER_SENDING: {
            break;
        }
        case SNEDER_WAIT_ACK: {
            if (pFrame->header.info.msgType == LTLP_SYS_FRAME_ACK) {
                // 收到ACK，发送下一个数据帧
                g_sendCtrl[LTLP_APP_FRAME].sendFrameCnt++;
                g_sendCtrl[LTLP_APP_FRAME].tryTimes = 0;

                if (g_callbacks[LTLP_CALLBACK_ON_RECV_ACK] != NULL) {
                    g_callbacks[LTLP_CALLBACK_ON_RECV_ACK](pFrame, g_callbackParm[LTLP_CALLBACK_ON_RECV_ACK]);
                }
                if (g_sendCtrl[LTLP_APP_FRAME].sendFrameCnt ==
                    g_sendCtrl[LTLP_APP_FRAME].frame.header.info.totalFrames) {
                    // 发送完成
                    ltlpLogger("ltlp sender send all data done");
                    if (g_callbacks[LTLP_CALLBACK_ON_SEND_ALL_DATA_DONE] != NULL) {
                        g_callbacks[LTLP_CALLBACK_ON_SEND_ALL_DATA_DONE](
                            pFrame, g_callbackParm[LTLP_CALLBACK_ON_SEND_ALL_DATA_DONE]);
                    }
                    ltlpSenderReset();
                } else {
                    g_senderState = SENDER_SENDING;
                }

            } else if (pFrame->header.info.msgType == LTLP_SYS_FRAME_NACK) {
                // 收到NACK，重发数据帧
                g_sendCtrl[LTLP_APP_FRAME].tryTimes++;
                if (g_sendCtrl[LTLP_APP_FRAME].tryTimes <= LTLP_MAX_RETRY_TIMES) {
                    if (g_callbacks[LTLP_CALLBACK_ON_RECV_NACK] != NULL) {
                        g_callbacks[LTLP_CALLBACK_ON_RECV_NACK](NULL, g_callbackParm[LTLP_CALLBACK_ON_RECV_NACK]);
                    }
                    g_senderState = SENDER_SENDING;
                } else {
                    if (g_callbacks[LTLP_CALLBACK_ON_SEND_ERROR] != NULL) {
                        g_callbacks[LTLP_CALLBACK_ON_SEND_ERROR](NULL, g_callbackParm[LTLP_CALLBACK_ON_SEND_ERROR]);
                    }
                    ltlpSenderReset();
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}
SenderSateDef ltlpSenderRun() {
    switch (g_senderState) {
        case SENDER_IDLE: {
            // 空闲状态
            // 等待发送
            break;
        }
        case SENDER_HANDSHKE: {
            // 握手状态
            if (ltlpNowTime() - g_lastSendTime >= LTLP_TIMEOUT_MS) {
                ltlpLogger("ltlp sender handshake timeout");
                if (g_callbacks[LTLP_CALLBACK_ON_HANDSHAKE_ERROR] != NULL) {
                    g_callbacks[LTLP_CALLBACK_ON_HANDSHAKE_ERROR](NULL,
                                                                  g_callbackParm[LTLP_CALLBACK_ON_HANDSHAKE_ERROR]);
                }
                ltlpSenderReset();
            }
            break;
        }
        case SENDER_SENDING: {
            ltlpSendOneFrame(&g_sendCtrl[LTLP_APP_FRAME]);
            if (g_callbacks[LTLP_CALLBACK_ON_SEND_ONE_FRAME_DONE] != NULL) {
                g_callbacks[LTLP_CALLBACK_ON_SEND_ONE_FRAME_DONE](&g_sendCtrl[LTLP_APP_FRAME].frame,
                                                                  g_callbackParm[LTLP_CALLBACK_ON_SEND_ONE_FRAME_DONE]);
            }
            g_lastSendTime = ltlpNowTime();
            g_senderState = SNEDER_WAIT_ACK;
            break;
        }
        case SNEDER_WAIT_ACK: {
            if (ltlpNowTime() - g_lastSendTime >= LTLP_TIMEOUT_MS) {
                if (g_sendCtrl[LTLP_APP_FRAME].tryTimes <= LTLP_MAX_RETRY_TIMES) {
                    ltlpLogger("ltlp sender wait ack timeout, retry %d", g_sendCtrl[LTLP_APP_FRAME].tryTimes);
                    g_senderState = SENDER_SENDING;
                } else {
                    ltlpLogger("ltlp sender wait ack timeout, send error, stop at frame %d",
                               g_sendCtrl[LTLP_APP_FRAME].sendFrameCnt);
                    if (g_callbacks[LTLP_CALLBACK_ON_SEND_ERROR] != NULL) {
                        g_callbacks[LTLP_CALLBACK_ON_SEND_ERROR](&g_sendCtrl[LTLP_APP_FRAME].frame,
                                                                 g_callbackParm[LTLP_CALLBACK_ON_SEND_ERROR]);
                    }
                    ltlpSenderReset();
                }
            }

            break;
        }
        default: {
            break;
        }
    }
    return g_senderState;
}
