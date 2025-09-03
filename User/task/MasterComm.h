#ifndef UWB_TASK_H
#define UWB_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAME_LEN_MAX 1016
    typedef enum
    {
        UWB_MSG_TYPE_SEND_DATA = 1, // 应用任务发送数据
        UWB_MSG_TYPE_CONFIG,        // 配置信息
        UWB_MSG_TYPE_SET_MODE       // 设置工作模式
    } UwbMsgType;

    typedef struct
    {
        UwbMsgType type;
        uint16_t data_len;
        uint8_t data[FRAME_LEN_MAX];
        uint32_t delay_ms; // 发送延迟时间
    } UwbTxMsg;

    // UWB接收消息结构体
    typedef struct
    {
        uint16_t data_len;
        uint8_t data[FRAME_LEN_MAX];
        uint32_t timestamp;  // 接收时间戳
        uint32_t status_reg; // 状态寄存器值
    } uwbRxMsg;

    // 接收数据回调函数指针
    typedef void (*UwbRxCallback)(const uwbRxMsg *msg);

    class MasterComm
    {
      public:
        MasterComm();
        ~MasterComm();

        int Initialize(void);
        int SendData(const uint8_t *data, uint16_t len, uint32_t delayMs);
        int ReceiveData(uwbRxMsg *msg, uint32_t timeoutMs);
        void SetRxCallback(UwbRxCallback callback);
        int GetTxQueueCount();
        int GetRxQueueCount();
        void ClearTxQueue();
        void ClearRxQueue();
        int Reconfigure();
    };

#ifdef __cplusplus
}
#endif

#endif /* UWB_TASK_H */