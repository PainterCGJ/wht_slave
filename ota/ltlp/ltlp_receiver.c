#include "ltlp_receiver.h"
#include "ltlp_common.h"
LtlpRecvCrtl g_recvCtrl;

ReceiverSateDef g_receiverState = RECEIVER_IDLE;
void ltlpReceiverReset() {
    memset(&g_recvCtrl, 0, sizeof(g_recvCtrl));
    g_receiverState = RECEIVER_IDLE;
}
void ltlpReceiverHandleFrame(LtlpFrame* pFrame) {
    LtlpSendCrtl ackCtrl = {0};
    g_recvCtrl.time = ltlpNowTime();
    switch (g_receiverState) {
        case RECEIVER_IDLE: {
            // 空闲状态
            // 等待握手
            if (pFrame->header.info.frameType == LTLP_SYS_FRAME) {
                if (pFrame->header.info.msgType == LTLP_SYS_FRAME_HANDSHAKE) {
                    // 握手成功，开始接收数据帧
                    ltlpLogger("ltlp receiver handshake success");
                    ltlpSendAck(&ackCtrl, pFrame->header.info.srcID);
                    g_receiverState = RECEIVER_RECVING;
                }
            }
            break;
        }
        case RECEIVER_RECVING: {
            // 接收状态
            if (pFrame->header.info.frameType == LTLP_APP_FRAME) {
                // 接收数据帧
                g_recvCtrl.recvFrameCnt++;
                ltlpLogger("ltlp receiver recv frame %d/%d", pFrame->header.info.seqNum + 1,
                           pFrame->header.info.totalFrames);
                uint32_t minBufferSize =
                    pFrame->header.info.payloadLen + pFrame->header.info.seqNum * LTLP_MAX_PAYLOAD_LEN;
                if (g_setting.pAssambleBuffer != NULL && g_setting.assambleBufferSize >= minBufferSize) {
                    // 有组包缓存，且缓存足够大
                    memcpy(g_setting.pAssambleBuffer + pFrame->header.info.seqNum * LTLP_MAX_PAYLOAD_LEN,
                           pFrame->pFramePayload, pFrame->header.info.payloadLen);
                }
                if (g_callbacks[LTLP_CLLBACK_ON_RECV_ONE_FRAME] != NULL) {
                    g_callbacks[LTLP_CLLBACK_ON_RECV_ONE_FRAME](pFrame, g_callbackParm[LTLP_CLLBACK_ON_RECV_ONE_FRAME]);
                }
                ltlpSendAck(&ackCtrl, pFrame->header.info.srcID);
                if (pFrame->header.info.totalFrames == g_recvCtrl.recvFrameCnt) {
                    // 接收完成
                    ltlpLogger("ltlp receiver recv all data done");
                    if (g_callbacks[LTLP_CLLBACK_ON_RECV_ALL_DATA] != NULL) {
                        g_callbacks[LTLP_CLLBACK_ON_RECV_ALL_DATA](pFrame,
                                                                   g_callbackParm[LTLP_CLLBACK_ON_RECV_ALL_DATA]);
                    }
                    ltlpReceiverReset();
                }
            }
            break;
        }
        default:
            break;
    }
}
ReceiverSateDef ltlpReceiverRun() {
    switch (g_receiverState) {
        case RECEIVER_IDLE: {
            // 空闲状态，等待接收
            break;
        }
        case RECEIVER_RECVING: {
            // 接收状态
            if (ltlpNowTime() - g_recvCtrl.time >= LTLP_TIMEOUT_MS * (LTLP_MAX_RETRY_TIMES + 1)) {
                // 超时未接收完成
                ltlpLogger("ltlp receiver recv timeout");
                if (g_callbacks[LTLP_CALBACK_ON_RECV_TIMEOUT] != NULL) {
                    g_callbacks[LTLP_CALBACK_ON_RECV_TIMEOUT](NULL, g_callbackParm[LTLP_CALBACK_ON_RECV_TIMEOUT]);
                }
                ltlpReceiverReset();
            }
            break;
        }
        default: {
            break;
        }
    }
    return g_receiverState;
}
