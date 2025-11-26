#include "ota_task.h"
#include "cmsis_os2.h"
#include "elog.h"
#include "get_id.h"
#include "msg_def.h"
#include "ota.h"
// 先包含uart_cmd_handler.h（它只有前向声明），再包含usart.h（完整定义）
#include "uart_cmd_handler.h"
#include "usart.h"
#include "uwb_ltlp_queue.h"
#include <string.h>

// 用于跟踪LTLP发送状态
static bool s_ltlp_sending_to_uwb = false;

// LTLP发送完成回调函数（前向声明）
static void onLtlpSendAllDataDone(LtlpFrame *pFrame, void *usrParm);

OtaTask::OtaTask() : TaskClassS("OtaTask", static_cast<TaskPriority>(TASK_PRIORITY_OTA)), m_ltlpReceiverBufferLen(0)
{
}

void OtaTask::task()
{
    m_upgradeFlag = 0;
    LtlpBasicSettingDef ltlpSetting = {0};
    ltlpSetting.sendPortFunc = ltlpSendFunc;
    ltlpSetting.sendPortUsrParm = this;
    ltlpSetting.nowTimeFunc = ltlpNowTimeFunc;
    ltlpSetting.nowTimeUsrParm = this;
    ltlpSetting.localID = get_device_id();
    ltlpSetting.pAssambleBuffer = NULL;
    ltlpSetting.assambleBufferSize = 0;
    ltlpInit(&ltlpSetting);
    ltlpSetCallback(LTLP_CLLBACK_ON_RECV_ONE_FRAME, onLtlpRecvOneFrame, this);
    ltlpSetCallback(LTLP_CALLBACK_ON_SEND_ALL_DATA_DONE, onLtlpSendAllDataDone, this);
    ltlpSetLogger(ltlpLogCallback);
    elog_d(TAG, "OTA task started");
    elog_d(TAG, "Device ID: %08X", ltlpSetting.localID);
    for (;;)
    {
        // 处理OTA相关逻辑
        processOta();
        if (m_upgradeFlag == 1)
        {
            m_upgradeFlag = 0;
            ota_enter_bootloader();
        }
        ltlpRun();
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
    uint8_t data;
    m_ltlpReceiverBufferLen = 0;
    // 获取RS485接收队列
    osMessageQueueId_t rs485_rx_queue = uart_cmd_handler_get_rs485_rx_queue();
    if (rs485_rx_queue == NULL)
    {
        elog_w(TAG, "RS485 RX queue not created");
        return; // 队列未创建，直接返回
    }

    // 从UWB->LTLP队列读取数据（非阻塞方式）
    osMessageQueueId_t uwb_to_ltlp_queue = uwb_ltlp_get_uwb_to_ltlp_queue();
    if (uwb_to_ltlp_queue != NULL)
    {
        // 读取UWB->LTLP队列中的数据，直到队列为空或达到最大读取数量
        while (m_ltlpReceiverBufferLen < sizeof(m_ltlpReceiverBuffer))
        {
            if (m_ltlpReceiverBufferLen >= sizeof(m_ltlpReceiverBuffer))
            {
                // 缓冲区已满，记录警告并清空缓冲区
                elog_w(TAG, "LTLP receiver buffer overflow, clearing buffer");
                break;
            }
            osStatus_t status = osMessageQueueGet(uwb_to_ltlp_queue, &data, NULL, 0); // 非阻塞
            if (status != osOK)
            {
                break; // 队列为空，退出循环
            }

            // 将数据存入缓冲区
            m_ltlpReceiverBuffer[m_ltlpReceiverBufferLen++] = data;
        }
        m_ltlpPort = UWB_PORT;
        if (m_ltlpReceiverBufferLen > 0)
        {
            elog_d(TAG, "Received %d bytes from uwb, parsing", m_ltlpReceiverBufferLen);
            elog_d(TAG, "m_ltlpPort: %d", m_ltlpPort);
            ltlpParse(m_ltlpReceiverBuffer, m_ltlpReceiverBufferLen);
            m_ltlpReceiverBufferLen = 0;
        }
    }

    // 读取队列中的数据，直到队列为空或达到最大读取数量
    while (m_ltlpReceiverBufferLen < sizeof(m_ltlpReceiverBuffer))
    {
        // 检查缓冲区是否还有空间
        if (m_ltlpReceiverBufferLen >= sizeof(m_ltlpReceiverBuffer))
        {
            // 缓冲区已满，记录警告并清空缓冲区
            elog_w(TAG, "LTLP receiver buffer overflow, clearing buffer");
            break;
        }

        osStatus_t status = osMessageQueueGet(rs485_rx_queue, &data, NULL, 0); // 非阻塞
        if (status != osOK)
        {
            break; // 队列为空，退出循环
        }

        // 将数据存入缓冲区
        m_ltlpReceiverBuffer[m_ltlpReceiverBufferLen++] = data;
    }
    m_ltlpPort = DB9_PORT;
    // 如果有数据，调用ltlpParse解析
    if (m_ltlpReceiverBufferLen > 0)
    {
        elog_d(TAG, "Received %d bytes, parsing", m_ltlpReceiverBufferLen);
        elog_d(TAG, "m_ltlpPort: %d", m_ltlpPort);
        ltlpParse(m_ltlpReceiverBuffer, m_ltlpReceiverBufferLen);
        m_ltlpReceiverBufferLen = 0;
    }
}

void OtaTask::ltlpSendFunc(uint8_t *pData, uint32_t len, void *usrParm)
{
    OtaTask *pThis = static_cast<OtaTask *>(usrParm);

    // 获取LTLP->UWB队列
    if (pThis->m_ltlpPort == UWB_PORT)
    {
        osMessageQueueId_t ltlp_to_uwb_queue = uwb_ltlp_get_ltlp_to_uwb_queue();
        if (ltlp_to_uwb_queue != NULL)
        {
            // 将数据逐字节入队到LTLP->UWB队列
            for (uint32_t i = 0; i < len; i++)
            {
                uint8_t byte = pData[i];
                if (osMessageQueuePut(ltlp_to_uwb_queue, &byte, 0, 0) != osOK)
                {
                    // 队列满时记录警告，但不中断处理
                    elog_w(TAG, "LTLP->UWB queue full, dropping byte");
                    break;
                }
            }
            uwb_ltlp_set_ltlp_data_ready();
            elog_d(TAG, "Sent %d bytes to UWB", len);
        }
    }
    else if (pThis->m_ltlpPort == DB9_PORT)
    {
        // 同时保持原有的RS485发送功能（如果需要）
        // 切换到RS485发送模式
        RS485_TX_EN();

        // 通过RS485发送数据
        HAL_UART_Transmit(&RS485_UART, pData, len, HAL_MAX_DELAY);
        elog_d(TAG, "Sent %d bytes to RS485", len);
        // 切换回RS485接收模式
        RS485_RX_EN();
    }
    else
    {
        elog_w(TAG, "Invalid LTLP port");
    }
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

// LTLP发送完成回调函数
static void onLtlpSendAllDataDone(LtlpFrame *pFrame, void *usrParm)
{
    (void)pFrame;
    (void)usrParm;
}
