#include "master_slave_message_handlers.h"

#include "elog.h"
#include "hptimer.hpp"
#include "slave_device.h"
#include "uwb_ltlp_queue.h"

using namespace WhtsProtocol;

namespace SlaveApp
{

// Sync Message Handler
std::unique_ptr<Message> SyncMessageHandler::ProcessMessage(const Message &message, SlaveDevice *device)
{
    auto syncMsg = dynamic_cast<const Master2Slave::SyncMessage *>(&message);
    if (!syncMsg)
        return nullptr;

    elog_v("SyncMessageHandler",
           "Processing new sync message - Mode: %d, Interval: %d ms, CurrentTime: %lu us, StartTime: %lu us",
           syncMsg->mode, syncMsg->interval, static_cast<unsigned long>(syncMsg->currentTime),
           static_cast<unsigned long>(syncMsg->startTime));

    // 1. 进行时间校准，计算与主机时间的偏移量
    uint64_t localTimestamp = HptimerGetUs();
    int64_t timeOffset = static_cast<int64_t>(syncMsg->currentTime) - static_cast<int64_t>(localTimestamp);
    device->m_timeOffset = timeOffset;

    // 更新sync消息接收时间，进入TDMA模式
    device->m_lastSyncMessageTime = localTimestamp;
    device->m_inTdmaMode = true;

    elog_v("SyncMessageHandler", "Time sync - Local: %lu us, Master: %lu us, Offset: %ld us",
           static_cast<unsigned long>(localTimestamp), static_cast<unsigned long>(syncMsg->currentTime),
           static_cast<long>(timeOffset));

    // 2. 存储采集间隔
    device->currentConfig.interval = syncMsg->interval;

    // 3. 根据模式设置采集配置
    switch (syncMsg->mode)
    {
    case 0: // 导通检测
        device->currentConfig.mode = CollectionMode::CONDUCTION;
        break;
    case 1: // 阻值检测
        device->currentConfig.mode = CollectionMode::RESISTANCE;
        break;
    case 2: // 卡钉检测
        device->currentConfig.mode = CollectionMode::CLIP;
        break;
    default:
        elog_w("SyncMessageHandler", "Unknown collection mode: %d", syncMsg->mode);
        return nullptr;
    }

    // 4. 查找本从机的配置
    bool configFound = false;
    bool resetRequested = false;
    for (const auto &config : syncMsg->slaveConfigs)
    {
        if (config.slaveId == device->m_deviceId)
        {
            device->currentConfig.timeSlot = config.timeSlot;
            device->currentConfig.testCount = config.testCount;
            device->m_isConfigured = true;
            configFound = true;
            resetRequested = (config.reset == 1);

            elog_v("SyncMessageHandler", "Found config for device 0x%08X - TimeSlot: %d, TestCount: %d, Reset: %d",
                   device->m_deviceId, config.timeSlot, config.testCount, config.reset);
            break;
        }
    }

    if (!configFound)
    {
        elog_w("SyncMessageHandler", "No configuration found for device 0x%08X", device->m_deviceId);
        device->m_isConfigured = false;
        return nullptr;
    }

    // 处理复位请求
    if (resetRequested)
    {
        elog_v("SyncMessageHandler", "Reset requested for device 0x%08X", device->m_deviceId);

        // 重置设备状态，但保留配置
        device->resetDevice();

        // 创建复位响应消息并排队等待发送
        auto resetResponse = std::make_unique<Slave2Master::RstResponseMessage>();
        resetResponse->status = 0; // 0：复位成功

        // 存储待回复的响应并设置标志位
        device->m_pendingResetResponse = std::move(resetResponse);
        device->m_hasPendingResetResponse = true;

        elog_v("SyncMessageHandler", "Reset completed, response queued for next active slot");
    }

    // 5. 设置延迟启动时间
    device->m_scheduledStartTime = syncMsg->startTime;
    device->m_isScheduledToStart = true;

    // 6. 配置采集器
    // totalCycles = sum of salve's testCount
    uint16_t totalCycles = 0;
    for (const auto &config : syncMsg->slaveConfigs)
    {
        totalCycles += config.testCount;
    }

    if (device->m_continuityCollector)
    {
        CollectorConfig collectorConfig(device->currentConfig.testCount, totalCycles);
        if (!device->m_continuityCollector->Configure(collectorConfig))
        {
            elog_e("SyncMessageHandler", "Failed to configure continuity collector");
            device->m_deviceState = SlaveDeviceState::DEV_ERR;
            return nullptr;
        }
        elog_v("SyncMessageHandler", "Continuity collector configured - TestCount: %d, TotalCycles: %d",
               device->currentConfig.testCount, totalCycles);
    }

    // 7. 配置时隙管理器（先停止再配置）
    if (device->m_slotManager)
    {
        // 先停止当前运行的时隙管理器
        device->m_slotManager->Stop();

        // calculate startSlot
        uint16_t startSlot = 0;
        for (const auto &config : syncMsg->slaveConfigs)
        {
            if (config.slaveId == device->m_deviceId)
            {
                break;
            }
            startSlot += config.testCount;
        }

        uint8_t deviceSlotCount = device->currentConfig.testCount; // 每个设备占用一个时隙
        uint16_t totalSlotCount = totalCycles;                     // 总时隙数等于从机数量
        uint32_t slotIntervalMs = device->currentConfig.interval;

        // 使用单周期模式配置SlotManager
        if (!device->m_slotManager->Configure(startSlot, deviceSlotCount, totalSlotCount, slotIntervalMs, true))
        {
            elog_e("SyncMessageHandler", "Failed to configure slot manager");
            device->m_deviceState = SlaveDeviceState::DEV_ERR;
            return nullptr;
        }
        elog_v("SyncMessageHandler",
               "Slot manager configured (single cycle) - StartSlot: %d, TotalSlots: %d, Interval: %d ms", startSlot,
               totalSlotCount, slotIntervalMs);
    }

    // 8. 检查是否立即启动或延迟启动
    uint64_t currentSyncTime = device->GetSyncTimestampUs();
    if (currentSyncTime >= syncMsg->startTime)
    {
        // 立即启动数据采集
        elog_v("SyncMessageHandler", "Starting collection immediately (start time already reached)");
        if (device->m_continuityCollector && device->m_slotManager &&
            device->m_continuityCollector->StartCollection() && device->m_slotManager->Start())
        {
            device->m_isCollecting = true;
            uwb_ltlp_set_conducting_state(true); // 更新全局导通检测状态标志
            device->m_deviceState = SlaveDeviceState::RUNNING;
            device->m_isFirstCollection = true;
            elog_v("SyncMessageHandler", "Data collection and slot management started successfully");
        }
        else
        {
            elog_e("SyncMessageHandler", "Failed to start data collection or slot management");
            device->m_deviceState = SlaveDeviceState::DEV_ERR;
        }
    }
    else
    {
        // 延迟启动
        device->m_deviceState = SlaveDeviceState::READY;
        uint64_t delayUs = syncMsg->startTime - currentSyncTime;
        elog_v("SyncMessageHandler", "Collection scheduled to start in %lu us", static_cast<unsigned long>(delayUs));
    }

    return nullptr; // SyncMessage 不需要响应
}

// Ping Request Message Handler
std::unique_ptr<Message> PingRequestHandler::ProcessMessage(const Message &message, SlaveDevice *device)
{
    const auto *pingMsg = dynamic_cast<const Master2Slave::PingReqMessage *>(&message);
    if (!pingMsg)
        return nullptr;

    elog_v("PingRequestHandler", "Processing Ping request - Sequence number: %u, Timestamp: %u",
           pingMsg->sequenceNumber, pingMsg->timestamp);

    auto response = std::make_unique<Slave2Master::PingRspMessage>();
    response->sequenceNumber = pingMsg->sequenceNumber;
    response->timestamp = SlaveApp::SlaveDevice::getCurrentTimestamp();
    return std::move(response);
}

// Short ID Assignment Message Handler
std::unique_ptr<Message> ShortIdAssignHandler::ProcessMessage(const Message &message, SlaveDevice *device)
{
    const auto *assignMsg = dynamic_cast<const Master2Slave::ShortIdAssignMessage *>(&message);
    if (!assignMsg)
        return nullptr;

    elog_v("ShortIdAssignHandler", "Processing short ID assignment - Short ID: %d",
           static_cast<int>(assignMsg->shortId));

    // 直接调用SlaveDevice的setShortId方法
    device->setShortId(assignMsg->shortId);

    auto response = std::make_unique<Slave2Master::ShortIdConfirmMessage>();
    response->status = 0; // Success
    response->shortId = assignMsg->shortId;
    return std::move(response);
}

} // namespace SlaveApp