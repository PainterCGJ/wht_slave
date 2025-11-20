#include "ota_task.h"
#include "cmsis_os2.h"
#include "elog.h"
#include "get_id.h"
#include "msg_def.h"
#include "ota.h"
// 先包含uart_cmd_handler.h（它只有前向声明），再包含usart.h（完整定义）
#include "uart_cmd_handler.h"
#include "usart.h"
#include <string.h>

OtaTask::OtaTask() : TaskClassS("OtaTask", static_cast<TaskPriority>(TASK_PRIORITY_OTA)), m_ltlpReceiverBufferLen(0)
{
}

void OtaTask::task()
{
    m_upgradeFlag = 0;
    LtlpBasicSettingDef ltlpSetting = {0};
    ltlpSetting.sendPortFunc = ltlpSendFunc;
    ltlpSetting.nowTimeFunc = ltlpNowTimeFunc;
    ltlpSetting.nowTimeUsrParm = NULL;
    ltlpSetting.localID = get_device_id();
    ltlpSetting.pAssambleBuffer = NULL;
    ltlpSetting.assambleBufferSize = 0;
    ltlpInit(&ltlpSetting);
    ltlpSetCallback(LTLP_CLLBACK_ON_RECV_ONE_FRAME, onLtlpRecvOneFrame, this);
    ltlpSetLogger(ltlpLogCallback);
    elog_d(TAG, "OTA task started");
    elog_d(TAG, "Device ID: %08X", ltlpSetting.localID);
    for (;;)
    {
        ltlpRun();
        // 处理OTA相关逻辑
        processOta();
        if (m_upgradeFlag == 1)
        {
            m_upgradeFlag = 0;
            ota_enter_bootloader();
        }

        // 延时，避免CPU占用过高
        TaskBase::delay(PROCESS_INTERVAL_MS);
    }
}
void OtaTask::ltlpLogCallback(const char *message)
{
    if (message == NULL)
    {
        return;
    }

    // 根据消息内容选择合适的日志级别
    // 检查是否包含错误关键词
    if (strstr(message, "error") != NULL || strstr(message, "Error") != NULL || strstr(message, "ERROR") != NULL)
    {
        elog_e(TAG, "LTLP: %s", message);
    }
    // 检查是否包含警告关键词
    else if (strstr(message, "warning") != NULL || strstr(message, "Warning") != NULL ||
             strstr(message, "WARNING") != NULL || strstr(message, "timeout") != NULL ||
             strstr(message, "Timeout") != NULL || strstr(message, "TIMEOUT") != NULL)
    {
        elog_w(TAG, "LTLP: %s", message);
    }
    // 其他消息使用debug级别
    else
    {
        elog_d(TAG, "LTLP: %s", message);
    }
}
void OtaTask::processOta()
{
    // 获取RS485接收队列
    osMessageQueueId_t rs485_rx_queue = uart_cmd_handler_get_rs485_rx_queue();
    if (rs485_rx_queue == NULL)
    {
        elog_w(TAG, "RS485 RX queue not created");
        return; // 队列未创建，直接返回
    }

    // 从队列中读取数据（非阻塞方式）
    uint8_t data;
    uint32_t receivedCount = 0;
    const uint32_t maxReadPerCycle = 256; // 每次最多读取256字节，避免长时间占用

    // 读取队列中的数据，直到队列为空或达到最大读取数量
    while (receivedCount < maxReadPerCycle)
    {
        osStatus_t status = osMessageQueueGet(rs485_rx_queue, &data, NULL, 0); // 非阻塞
        if (status != osOK)
        {
            break; // 队列为空，退出循环
        }

        // 检查缓冲区是否还有空间
        if (m_ltlpReceiverBufferLen >= sizeof(m_ltlpReceiverBuffer))
        {
            // 缓冲区已满，记录警告并清空缓冲区
            elog_w(TAG, "LTLP receiver buffer overflow, clearing buffer");
            m_ltlpReceiverBufferLen = 0;
        }

        // 将数据存入缓冲区
        m_ltlpReceiverBuffer[m_ltlpReceiverBufferLen++] = data;
        receivedCount++;
    }
    // elog_d(TAG, "Received %d bytes", receivedCount);
    // 如果有数据，调用ltlpParse解析
    if (m_ltlpReceiverBufferLen > 0)
    {
        elog_d(TAG, "Received %d bytes, parsing", m_ltlpReceiverBufferLen);
        ltlpParse(m_ltlpReceiverBuffer, m_ltlpReceiverBufferLen);
        // 解析后清空缓冲区（ltlpParse会处理数据，我们只需要提供原始数据）
        // 注意：如果ltlpParse需要保留部分数据用于帧重组，应该在ltlp内部处理
        // 这里我们每次都将所有数据传给ltlpParse，由它决定如何处理
        m_ltlpReceiverBufferLen = 0;
    }
}

void OtaTask::ltlpSendFunc(uint8_t *pData, uint32_t len, void *usrParm)
{
    (void)usrParm; // 未使用的参数

    // 切换到RS485发送模式
    RS485_TX_EN();

    // 通过RS485发送数据
    HAL_UART_Transmit(&RS485_UART, pData, len, HAL_MAX_DELAY);

    // 切换回RS485接收模式
    RS485_RX_EN();
}

uint32_t OtaTask::ltlpNowTimeFunc(void *usrParm)
{
    (void)usrParm; // 未使用的参数

    // 获取FreeRTOS系统时间（毫秒）
    // osKernelGetTickCount() 返回自系统启动以来的毫秒数
    return osKernelGetTickCount();
}

void OtaTask::onLtlpRecvOneFrame(LtlpFrame *pFrame, void *usrParm)
{
    (void)usrParm; // 未使用的参数
    OtaTask *pThis = static_cast<OtaTask *>(usrParm);
    switch (pFrame->header.info.msgType)
    {
    case MSG_LINK: {
        elog_d(TAG, "Link");
        break;
    }
    case MSG_ENTER_UPGRADE: {
        elog_d(TAG, "Enter upgrade");
        TaskBase::delay(10);
        pThis->m_upgradeFlag = 1;
        break;
    }
    default:
        break;
    }
}
