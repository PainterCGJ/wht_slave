#include "MasterComm.h"

#include "cmsis_os2.h"
#include "config.h"
#include <memory>

// #include "deca_device_api.h"
// #include "deca_regs.h"
// #include "port.h"
#include "CX310.hpp"
#include "uwb_interface.hpp"
#if ENABLE_OTA_TASK
#include "uwb_ltlp_queue.h"
#endif

// 外部声明全局指针
extern CX310_SlaveSpiAdapter *g_uwbAdapter;

#include "elog.h"

#define TX_QUEUE_SIZE 10
#define RX_QUEUE_SIZE 20

// 静态包装函数，用于FreeRTOS任务创建
void MasterComm::UwbCommTaskWrapper(void *argument)
{
    MasterComm *instance = static_cast<MasterComm *>(argument);
    instance->UwbCommTask();
}

// 实际的任务实现方法
void MasterComm::UwbCommTask()
{
    static auto TAG = "uwb_comm";

    // 将大对象从栈改为堆分配，减少栈空间占用
    auto rxMsg = std::make_unique<uwbRxMsg>();
    auto uwb = std::make_unique<CX310<CX310_SlaveSpiAdapter>>();
    osDelay(100);
    // 设置全局指针，用于中断处理
    g_uwbAdapter = &uwb->get_interface();

    std::vector<uint8_t> buffer = {0};
    std::vector<uint8_t> tx_data; // 用于临时存储发送数据
    tx_data.reserve(FRAME_LEN_MAX);

    elog_i(TAG, "UWB task started, initializing UWB...");
    if (uwb->init())
    {
        elog_i(TAG, "uwb.init success");
    }
    else
    {
        elog_e(TAG, "uwb.init failed");
    }
    osDelay(3);
    uwb->set_recv_mode();
    elog_i(TAG, "UWB set to receive mode");
    // 读取uwb配置
    uint8_t channel;
    uwb->get_channel(channel);
    elog_i(TAG, "UWB channel: %d", channel);
    uint8_t prf_mode;
    uwb->get_prf_mode(prf_mode);
    elog_i(TAG, "UWB prf mode: %d", prf_mode);
    uint8_t sfd_id;
    uwb->get_sfd_id(sfd_id);
    elog_i(TAG, "UWB sfd id: %d", sfd_id);
    uint8_t preamble_length;
    uwb->get_preamble_length(preamble_length);
    elog_i(TAG, "UWB preamble length: %d", preamble_length);
    uint8_t preamble_index;
    uwb->get_preamble_index(preamble_index);
    elog_i(TAG, "UWB preamble index: %d", preamble_index);
    uint8_t psdu_data_rate;
    uwb->get_psdu_data_rate(psdu_data_rate);
    elog_i(TAG, "UWB psdu data rate: %d", psdu_data_rate);
    uint8_t phr_mode;
    uwb->get_phr_mode(phr_mode);
    elog_i(TAG, "UWB phr mode: %d", phr_mode);

    // if DIP1 reset, set uwb channel to 6
    // else if DIP2 reset, set uwb channel to 7
    // else if DIP3 reset, set uwb channel to 8
    // else if DIP4 reset, set uwb channel to 9
    // else if DIP5 reset, set uwb channel to 10
    // else set uwb channel to 5
    if (HAL_GPIO_ReadPin(DIP1_GPIO_Port, DIP1_Pin) == GPIO_PIN_RESET)
    {
        uwb->set_channel(6);
    }
    else if (HAL_GPIO_ReadPin(DIP2_GPIO_Port, DIP2_Pin) == GPIO_PIN_RESET)
    {
        uwb->set_channel(7);
    }
    else if (HAL_GPIO_ReadPin(DIP3_GPIO_Port, DIP3_Pin) == GPIO_PIN_RESET)
    {
        uwb->set_channel(8);
    }
    else if (HAL_GPIO_ReadPin(DIP4_GPIO_Port, DIP4_Pin) == GPIO_PIN_RESET)
    {
        uwb->set_channel(9);
    }
    else if (HAL_GPIO_ReadPin(DIP5_GPIO_Port, DIP5_Pin) == GPIO_PIN_RESET)
    {
        uwb->set_channel(10);
    }
    else
    {
        uwb->set_channel(5);
    }

    for (;;)
    {
        // 等待发送信号量，有数据需要发送时被唤醒
        if (osSemaphoreAcquire(uwbTxSemaphore, 0) == osOK)
        {
            // 获取互斥锁，从全局buffer读取数据
            if (osMutexAcquire(uwbTxMutex, 10) == osOK)
            {
                // 直接从全局buffer读取，避免拷贝
                tx_data.assign(txBuffer, txBuffer + txBufferLen);
                uint32_t currentTxCount = ++txCount;  // 增加发送计数
                
                osMutexRelease(uwbTxMutex);
                
                // 只输出关键信息：发送第几包
                elog_i(TAG, "tx #%lu", currentTxCount);
                uwb->update();
                uwb->data_transmit(tx_data);
            }
        }

#if ENABLE_OTA_TASK
        // 只有在非导通检测模式下，才检查LTLP->UWB队列是否有数据
        if (!uwb_ltlp_is_conducting())
        {
            osEventFlagsId_t event_flags = uwb_ltlp_get_event_flags();
            osMessageQueueId_t ltlp_to_uwb_queue = uwb_ltlp_get_ltlp_to_uwb_queue();

            if (event_flags != NULL && ltlp_to_uwb_queue != NULL)
            {
                // 检查LTLP数据就绪事件标志
                uint32_t flags = osEventFlagsGet(event_flags);
                if ((flags & UWB_LTLP_EVENT_LTLP_DATA_READY) != 0)
                {
                    uwb_ltlp_clear_ltlp_data_ready();
                    elog_d(TAG, "LTLP data ready, reading data from LTLP->UWB queue");
                    // 从LTLP->UWB队列读取数据并发送
                    std::vector<uint8_t> ltlp_data;
                    uint8_t byte;
                    const uint32_t max_read_per_cycle = 256; // 每次最多读取256字节
                    uint32_t read_count = 0;

                    // 读取队列中的数据
                    while (read_count < max_read_per_cycle)
                    {
                        if (osMessageQueueGet(ltlp_to_uwb_queue, &byte, NULL, 0) != osOK)
                        {
                            break; // 队列为空，退出循环
                        }
                        ltlp_data.push_back(byte);
                        read_count++;
                    }

                    // 如果有数据，通过UWB发送
                    if (!ltlp_data.empty())
                    {
                        elog_d(TAG, "Sending %d bytes from LTLP to UWB", ltlp_data.size());
                        uwb->update();
                        uwb->data_transmit(ltlp_data);

                        // 检查队列是否已空，如果为空则清除事件标志
                        if (osMessageQueueGetCount(ltlp_to_uwb_queue) == 0)
                        {
                            uwb_ltlp_clear_ltlp_data_ready();
                        }
                    }
                }
            }
        }
#endif

        if (uwb->get_recv_data(buffer))
        {
            size_t bufferSize = buffer.size();
            uint32_t timestamp = osKernelGetTickCount();

            // 只处理第一帧数据（通常就一帧）
            size_t dataSize = (bufferSize > FRAME_LEN_MAX) ? FRAME_LEN_MAX : bufferSize;

            // 获取互斥锁，写入全局RX buffer
            if (osMutexAcquire(uwbRxMutex, 10) == osOK)
            {
                memcpy(rxBuffer, buffer.data(), dataSize);
                rxBufferLen = dataSize;
                rxTimestamp = timestamp;
                
                osMutexRelease(uwbRxMutex);
                
                // 释放信号量，通知有接收数据
                osSemaphoreRelease(uwbRxSemaphore);
            }

            // 如果有回调函数，准备rxMsg并调用
            if (uwbRxCallback != nullptr)
            {
                rxMsg->dataLen = dataSize;
                memcpy(rxMsg->data, buffer.data(), dataSize);
                rxMsg->timestamp = timestamp;
                rxMsg->statusReg = 0;
                uwbRxCallback(rxMsg.get());
            }

#if ENABLE_OTA_TASK
            // 如果不在导通检测状态，将数据入队到UWB->LTLP队列
            if (!uwb_ltlp_is_conducting())
            {
                osMessageQueueId_t uwb_to_ltlp_queue = uwb_ltlp_get_uwb_to_ltlp_queue();
                if (uwb_to_ltlp_queue != NULL)
                {
                    // 将接收到的数据逐字节入队
                    for (size_t i = 0; i < dataSize; i++)
                    {
                        uint8_t byte = buffer[i];
                        if (osMessageQueuePut(uwb_to_ltlp_queue, &byte, 0, 0) != osOK)
                        {
                            break;
                        }
                    }
                }
            }
#endif
        }

        uwb->update();
        osDelay(1);
    }
}

MasterComm::MasterComm()
    : uwbCommTaskHandle(nullptr), uwbTxMutex(nullptr), uwbRxMutex(nullptr), uwbTxSemaphore(nullptr),
      uwbRxSemaphore(nullptr), uwbRxCallback(nullptr), txBufferLen(0), rxBufferLen(0), rxTimestamp(0), txCount(0)
{
    Initialize();
}
MasterComm::~MasterComm()
{
    // 清理资源
    if (uwbTxMutex != nullptr)
    {
        osMutexDelete(uwbTxMutex);
    }
    if (uwbRxMutex != nullptr)
    {
        osMutexDelete(uwbRxMutex);
    }
    if (uwbTxSemaphore != nullptr)
    {
        osSemaphoreDelete(uwbTxSemaphore);
    }
    if (uwbRxSemaphore != nullptr)
    {
        osSemaphoreDelete(uwbRxSemaphore);
    }
}

int MasterComm::Initialize(void)
{
    static const char *TAG = "uwb_init";

    // 创建互斥锁保护全局buffer
    uwbTxMutex = osMutexNew(nullptr);
    if (uwbTxMutex == nullptr)
    {
        elog_e(TAG, "Failed to create UWB TX mutex");
        return -1;
    }

    uwbRxMutex = osMutexNew(nullptr);
    if (uwbRxMutex == nullptr)
    {
        elog_e(TAG, "Failed to create UWB RX mutex");
        return -1;
    }

    // 创建信号量（二值信号量，初始值为0）
    uwbTxSemaphore = osSemaphoreNew(1, 0, nullptr);
    if (uwbTxSemaphore == nullptr)
    {
        elog_e(TAG, "Failed to create UWB TX semaphore");
        return -1;
    }

    uwbRxSemaphore = osSemaphoreNew(1, 0, nullptr);
    if (uwbRxSemaphore == nullptr)
    {
        elog_e(TAG, "Failed to create UWB RX semaphore");
        return -1;
    }

    // 创建UWB通信任务
    constexpr osThreadAttr_t uwbTaskAttributes = {
        .name = "uwbCommTask",
        .stack_size = TASK_STACK_SIZE_UWB_COMM,
        .priority = (osPriority_t)TASK_PRIORITY_UWB_COMM,
    };

    uwbCommTaskHandle = osThreadNew(UwbCommTaskWrapper, this, &uwbTaskAttributes);
    if (uwbCommTaskHandle == nullptr)
    {
        elog_e(TAG, "Failed to create UWB communication task");
        return -1;
    }

    elog_i(TAG, "UWB communication task created successfully");
    return 0;
}

int MasterComm::SendData(const uint8_t *data, uint16_t len, uint32_t delayMs)
{
    if (data == nullptr || len == 0 || len > FRAME_LEN_MAX)
    {
        return -1;
    }

    // 获取互斥锁，直接写入全局buffer（避免队列拷贝）
    if (osMutexAcquire(uwbTxMutex, 100) != osOK)
    {
        return -2; // 获取互斥锁超时
    }

    // 直接拷贝到全局buffer（只拷贝一次，不需要队列）
    memcpy(txBuffer, data, len);
    txBufferLen = len;

    osMutexRelease(uwbTxMutex);

    // 释放信号量通知UWB任务有数据需要发送
    osSemaphoreRelease(uwbTxSemaphore);

    return 0;
}

int MasterComm::ReceiveData(uwbRxMsg *msg, uint32_t timeoutMs)
{
    if (msg == nullptr)
    {
        return -1;
    }

    // 等待接收信号量
    if (osSemaphoreAcquire(uwbRxSemaphore, timeoutMs) != osOK)
    {
        return -1; // 超时或错误
    }

    // 获取互斥锁，从全局buffer读取数据
    if (osMutexAcquire(uwbRxMutex, 10) != osOK)
    {
        return -1;
    }

    // 从全局buffer读取数据
    msg->dataLen = rxBufferLen;
    memcpy(msg->data, rxBuffer, rxBufferLen);
    msg->timestamp = rxTimestamp;
    msg->statusReg = 0;

    osMutexRelease(uwbRxMutex);

    return 0; // 成功
}

void MasterComm::SetRxCallback(UwbRxCallback callback)
{
    this->uwbRxCallback = callback;
}

// static uint8_t rx_buffer[FRAME_LEN_MAX];
// static uint32_t status_reg = 0;
// static uint16_t frame_len = 0;
// /* Default communication configuration. */
// static dwt_config_t config = {
//     5,               // 通道号，推荐5或2，5抗干扰稍强
//     DWT_PRF_64M,     // 脉冲重复频率：64MHz 提高灵敏度（优于16M）
//     DWT_PLEN_1024,   // 前导码长度：越长越容易同步，稳定性更好（如1024）
//     DWT_PAC32,       // PAC大小：与PLEN匹配，PAC32适用于PLEN1024
//     9,               // TX前导码索引：通道5/64MHz下推荐用9
//     9,               // RX前导码索引：与TX一致
//     0,               // 使用标准SFD：标准SFD解码鲁棒性更好
//     DWT_BR_850K,     // 数据速率：110Kbps最稳定，误码率最低
//     DWT_PHRMODE_EXT, // 标准PHY头
//     (1025 + 64 - 32) // SFD超时时间：可按 PLEN + margin 设置
// };

// // UWB通信任务
// static void uwb_comm_task(void *argument)
// {
//     static const char *TAG = "uwb_comm";
//     uwb_tx_msg_t tx_msg;
//     uwb_rx_msg_t rx_msg;

//     // 初始化DW1000
//     reset_DW1000();
//     port_set_dw1000_slowrate();
//     if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
//     {
//         elog_e(TAG, "dwt_initialise failed");
//         osThreadExit();
//     }
//     port_set_dw1000_fastrate();

//     // 配置DW1000
//     dwt_configure(&config);
//     elog_i(TAG, "dwt_configure success");

//     // uint32_t device_id = dwt_readdevid();
//     // elog_i(TAG, "device_id: %08X", device_id);

//     // 启动接收模式
//     dwt_rxenable(DWT_START_RX_IMMEDIATE);

//     while (1)
//     {
//         // 等待发送信号量，确保队列中有完整的数据
//         if (osSemaphoreAcquire(uwb_txSemaphore, 0) == osOK)
//         {
//             // 从队列获取发送消息
//             if (osMessageQueueGet(uwb_txQueue, &tx_msg, NULL, 0) == osOK)
//             {
//                 switch (tx_msg.type)
//                 {
//                 case UWB_MSG_TYPE_SEND_DATA:
//                     dwt_forcetrxoff(); // 保证发送前DW1000已空闲

//                     // 发送UWB数据
//                     // DW1000会自动添加2字节CRC，所以实际写入的数据长度是用户数据长度
//                     // 但是dwt_writetxfctrl需要包含CRC的总长度
//                     dwt_writetxdata(tx_msg.data_len + 2, tx_msg.data, 0);
//                     dwt_writetxfctrl(tx_msg.data_len + 2, 0, 1);
//                     dwt_starttx(DWT_START_TX_IMMEDIATE);

//                     // 等待发送完成
//                     while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
//                     {
//                         osDelay(1);
//                     }
//                     dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

//                     // 发送完成后重新启动接收
//                     dwt_rxenable(DWT_START_RX_IMMEDIATE);

//                     // elog_i(TAG, "Sent %d bytes done", tx_msg.data_len);
//                     break;

//                 case UWB_MSG_TYPE_CONFIG:
//                     // 重新配置DW1000
//                     dwt_configure(&config);
//                     dwt_rxenable(DWT_START_RX_IMMEDIATE);
//                     elog_i(TAG, "Config updated");
//                     break;

//                 case UWB_MSG_TYPE_SET_MODE:
//                     // 设置工作模式（预留接口）
//                     elog_i(TAG, "Mode set");
//                     break;

//                 default:
//                     break;
//                 }
//             }
//         }

//         // 检查是否有接收数据
//         status_reg = dwt_read32bitreg(SYS_STATUS_ID);
//         if (status_reg & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR))
//         {
//             // elog_i(TAG, "status_reg: %08X", status_reg);
//             if (status_reg & SYS_STATUS_RXFCG)
//             {
//                 // 成功接收到数据
//                 frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
//                 // elog_i(TAG, "frame_len: %d", frame_len);

//                 // frame_len包含2字节CRC，需要减去CRC长度得到实际数据长度
//                 if (frame_len >= 2 && frame_len <= FRAME_LEN_MAX)
//                 {
//                     dwt_readrxdata(rx_buffer, frame_len, 0);

//                     // 构造接收消息，只包含用户数据，不包含CRC
//                     rx_msg.data_len = frame_len - 2; // 减去2字节CRC
//                     for (int i = 0; i < rx_msg.data_len; i++)
//                     {
//                         rx_msg.data[i] = rx_buffer[i];
//                     }
//                     rx_msg.timestamp = osKernelGetTickCount();
//                     rx_msg.status_reg = status_reg;

//                     // 将数据放入接收队列
//                     osMessageQueuePut(uwb_rxQueue, &rx_msg, 0, 0);

//                     // 如果有回调函数，调用它
//                     if (uwb_rx_callback != NULL)
//                     {
//                         uwb_rx_callback(&rx_msg);
//                     }
//                 }

//                 // 清除接收完成标志
//                 dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
//             }
//             else
//             {
//                 // 接收错误
//                 dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
//                 elog_e(TAG, "RX error: %08X", status_reg);
//             }

//             // 重新启动接收
//             dwt_rxenable(DWT_START_RX_IMMEDIATE);
//         }
//     }
// }