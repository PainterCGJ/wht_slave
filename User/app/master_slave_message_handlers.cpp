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

    // 使用线程安全的方法设置时间偏移量
    device->SetTimeOffset(timeOffset);

    // 更新sync消息接收时间，进入TDMA模式
    device->m_lastSyncMessageTime = localTimestamp;
    device->m_inTdmaMode = true;

    elog_v("SyncMessageHandler", "Time sync - Local: %lu us, Master: %lu us, Offset: %ld us",
           static_cast<unsigned long>(localTimestamp), static_cast<unsigned long>(syncMsg->currentTime),
           static_cast<long>(timeOffset));

    // 2. 保存旧配置用于比较，并记录是否是首次配置
    SlaveDeviceConfig oldConfig = device->currentConfig;
    bool wasConfigured = device->m_isConfigured;

    // 3. 根据模式设置采集配置（临时设置，用于后续比较）
    CollectionMode newMode;
    switch (syncMsg->mode)
    {
    case 0: // 导通检测
        newMode = CollectionMode::CONDUCTION;
        break;
    case 1: // 阻值检测
        newMode = CollectionMode::RESISTANCE;
        break;
    case 2: // 卡钉检测
        newMode = CollectionMode::CLIP;
        break;
    default:
        elog_w("SyncMessageHandler", "Unknown collection mode: %d", syncMsg->mode);
        return nullptr;
    }

    // 4. 查找本从机的配置
    bool configFound = false;
    bool resetRequested = false;
    uint8_t newTimeSlot = 0;
    uint8_t newTestCount = 0;

    for (const auto &config : syncMsg->slaveConfigs)
    {
        if (config.slaveId == device->m_deviceId)
        {
            newTimeSlot = config.timeSlot;
            newTestCount = config.testCount;
            configFound = true;
            resetRequested = (config.reset == 1);

            elog_v("SyncMessageHandler", "Found config for device 0x%08X - TimeSlot: %d, TestCount: %d, Reset: %d",
                   device->m_deviceId, config.timeSlot, config.testCount, config.reset);
            
            // 添加详细的引脚数配置信息
            elog_d("SyncMessageHandler", "=== Pin Configuration Debug Info ===");
            elog_d("SyncMessageHandler", "Device ID: 0x%08X", device->m_deviceId);
            elog_d("SyncMessageHandler", "TestCount (Pin Count): %d", config.testCount);
            elog_d("SyncMessageHandler", "This device will have %d active slots for data transmission", config.testCount);
            elog_d("SyncMessageHandler", "====================================");
            break;
        }
    }

    if (!configFound)
    {
        elog_w("SyncMessageHandler", "No configuration found for device 0x%08X", device->m_deviceId);
        device->m_isConfigured = false;
        return nullptr;
    }

    // 5. 比较配置是否改变（比较所有配置项）
    bool configChanged = false;
    if (oldConfig.mode != newMode || oldConfig.interval != syncMsg->interval || oldConfig.timeSlot != newTimeSlot ||
        oldConfig.testCount != newTestCount)
    {
        configChanged = true;
        elog_v("SyncMessageHandler",
               "Configuration changed - Mode: %d->%d, Interval: %d->%d, TimeSlot: %d->%d, TestCount: %d->%d",
               static_cast<int>(oldConfig.mode), static_cast<int>(newMode), oldConfig.interval, syncMsg->interval,
               oldConfig.timeSlot, newTimeSlot, oldConfig.testCount, newTestCount);
    }

    // 6. 如果配置改变，清除之前的分片发送状态和缓存数据
    if (configChanged)
    {
        elog_v("SyncMessageHandler", "Clearing cached data and fragment sending state due to config change");

        // 清除分片发送状态（重新计算分包数量和发送分包所需要的时隙）
        device->m_isFragmentSendingInProgress = false;
        device->m_pendingFragments.clear();
        device->m_currentFragmentIndex = 0;

        // 清除缓存的采集数据
        device->lastCollectionData.clear();
        device->m_hasDataToSend = false;
    }

    // 7. 更新配置（无论是否改变都更新）
    device->currentConfig.mode = newMode;
    device->currentConfig.interval = syncMsg->interval;
    device->currentConfig.timeSlot = newTimeSlot;
    device->currentConfig.testCount = newTestCount;
    device->m_isConfigured = true;

    // 8. 处理复位请求
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

    // 9. 设置延迟启动时间
    device->m_scheduledStartTime = syncMsg->startTime;
    device->m_isScheduledToStart = true;

    // 10. 配置采集器和时隙管理器（仅在配置改变或首次配置时重新配置）
    // 注意：首次配置时configChanged可能为true（因为oldConfig是默认值），
    // 但如果之前已经配置过且配置未改变，则不需要重新配置
    if (configChanged || !wasConfigured)
    {
        // 10.1 配置采集器
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
            
            // 计算预期的数据量
            size_t expectedDataBits = totalCycles * device->currentConfig.testCount;
            size_t expectedDataBytes = (expectedDataBits + 7) / 8;
            
            if (configChanged)
            {
                elog_d("SyncMessageHandler", "Continuity collector reconfigured - TestCount: %d, TotalCycles: %d",
                       device->currentConfig.testCount, totalCycles);
            }
            else
            {
                elog_d("SyncMessageHandler", "Continuity collector configured - TestCount: %d, TotalCycles: %d",
                       device->currentConfig.testCount, totalCycles);
            }
            
            elog_d("SyncMessageHandler", "Expected data size: %d bits = %d bytes", expectedDataBits, expectedDataBytes);
            elog_d("SyncMessageHandler", "With MTU=800, expected fragments: ~%d", (expectedDataBytes + 799) / 800);
        }

        // 10.2 配置时隙管理器（先停止再配置）
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
            if (configChanged)
            {
                elog_d("SyncMessageHandler",
                       "Slot manager reconfigured (single cycle) - StartSlot: %d, TotalSlots: %d, Interval: %d ms",
                       startSlot, totalSlotCount, slotIntervalMs);
            }
            else
            {
                elog_d("SyncMessageHandler",
                       "Slot manager configured (single cycle) - StartSlot: %d, TotalSlots: %d, Interval: %d ms",
                       startSlot, totalSlotCount, slotIntervalMs);
            }

            // 打印详细的时隙信息
            elog_d("SyncMessageHandler", "=== Slot Manager Configuration ===");
            elog_d("SyncMessageHandler", "StartSlot: %d (0x%04X)", startSlot, startSlot);
            elog_d("SyncMessageHandler", "DeviceSlotCount (Active Slots): %d", deviceSlotCount);
            elog_d("SyncMessageHandler", "TotalSlotCount: %d", totalSlotCount);
            elog_d("SyncMessageHandler", "SlotInterval: %d ms", slotIntervalMs);
            elog_d("SyncMessageHandler", "Active Pin Range: 0 to %d", deviceSlotCount - 1);
            elog_d("SyncMessageHandler", "==================================");
        }
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
        // uint64_t delayUs = syncMsg->startTime - currentSyncTime;
        elog_v("SyncMessageHandler", "Collection scheduled to start in %lu ms",
               static_cast<unsigned long>(syncMsg->startTime) / 1000);
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