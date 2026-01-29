#ifndef UWB_TASK_H
#define UWB_TASK_H

#include "cmsis_os2.h"
#include <stdint.h>

#define FRAME_LEN_MAX 1016

// UWB接收消息结构体（保留用于兼容性）
typedef struct
{
    uint16_t dataLen;
    uint8_t data[FRAME_LEN_MAX];
    uint32_t timestamp; // 接收时间戳
    uint32_t statusReg; // 状态寄存器值
} uwbRxMsg;

// 接收数据回调函数指针
typedef void (*UwbRxCallback)(const uwbRxMsg *msg);

class MasterComm
{
  public:
    MasterComm();
    ~MasterComm();

    int SendData(const uint8_t *data, uint16_t len, uint32_t delayMs);
    int ReceiveData(uwbRxMsg *msg, uint32_t timeoutMs);
    void SetRxCallback(UwbRxCallback callback);

  private:
    int Initialize(void);

    // 任务相关私有方法
    void UwbCommTask();
    static void UwbCommTaskWrapper(void *argument);

    // 私有成员变量 - 使用全局buffer代替队列
    osThreadId_t uwbCommTaskHandle;
    osMutexId_t uwbTxMutex;         // 发送buffer互斥锁
    osMutexId_t uwbRxMutex;         // 接收buffer互斥锁
    osSemaphoreId_t uwbTxSemaphore; // 发送数据信号量（通知有数据）
    osSemaphoreId_t uwbRxSemaphore; // 接收数据信号量（通知有数据）
    UwbRxCallback uwbRxCallback;    // 接收数据回调函数指针

    // 全局buffer（避免队列拷贝）
    uint8_t txBuffer[FRAME_LEN_MAX];
    uint16_t txBufferLen;
    uint8_t rxBuffer[FRAME_LEN_MAX];
    uint16_t rxBufferLen;
    uint32_t rxTimestamp;

    // 统计信息（用于日志输出）
    uint32_t txCount; // 已发送包计数
};
#endif /* UWB_TASK_H */