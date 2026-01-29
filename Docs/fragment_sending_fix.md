# 数据分片上传问题修复文档

## 问题描述

当从机采集完数据后需要上传 6 包数据时，日志只显示上传了 5/6 包，最后一包没有上传。

## 问题原因分析

### 根本原因

数据分片数量超过了设备的激活时隙数量，导致最后几个分片无法发送。

### 详细分析

1. **激活时隙数量由 `testCount` 决定**
   - 在 `master_slave_message_handlers.cpp:198` 行：
     ```cpp
     uint8_t deviceSlotCount = device->currentConfig.testCount;
     ```
   - 如果 `testCount = 5`，则设备只有 5 个激活时隙（activePin 从 0 到 4）

2. **分片发送设计**
   - 原始设计：每个激活时隙发送一个数据分片
   - 在 `slave_device.cpp:442-480` 的 `OnSlotChanged()` 函数中实现

3. **问题场景**
   - 当数据被分成 6 个分片时，需要 6 个激活时隙才能全部发送
   - 但如果只有 5 个激活时隙（testCount=5），则：
     - activePin=0: 发送分片 0
     - activePin=1: 发送分片 1
     - activePin=2: 发送分片 2
     - activePin=3: 发送分片 3
     - activePin=4: 发送分片 4
     - **分片 5（第6个）永远不会被发送**（因为不存在 activePin=5 的时隙）

## 解决方案

### 修改内容

在 `User/app/slave_device.cpp` 的 `OnSlotChanged()` 函数中添加逻辑：
- 检测当前是否为最后一个激活时隙
- 如果是最后一个激活时隙且还有剩余分片未发送
- 则在当前时隙中循环发送所有剩余的分片

### 修改代码位置

文件：`User/app/slave_device.cpp`  
函数：`SlaveDevice::OnSlotChanged()`  
行号：约 442-480 行

### 核心修改逻辑

```cpp
// 发送当前时隙对应的分片
if (m_dataCollectionTask)
{
    elog_d(TAG, "Sending fragment %d/%d in active slot %d (pin %d)", 
           m_currentFragmentIndex + 1, m_pendingFragments.size(), 
           slotInfo.m_currentSlot, slotInfo.m_activePin);
    m_dataCollectionTask->sendDataToBackend();
}

// 检查是否还有剩余分片未发送，且当前是最后一个激活时隙
if (m_slotManager)
{
    uint8_t deviceSlotCount = m_slotManager->GetTotalSlots() > 0 ? currentConfig.testCount : 0;
    uint8_t lastActivePin = deviceSlotCount > 0 ? deviceSlotCount - 1 : 0;

    // 如果当前是最后一个激活时隙，并且还有剩余分片未发送
    if (slotInfo.m_activePin == lastActivePin && 
        m_currentFragmentIndex < m_pendingFragments.size())
    {
        elog_w(TAG, "Last active slot reached but %d fragments remaining, "
                    "sending all remaining fragments now",
               m_pendingFragments.size() - m_currentFragmentIndex);

        // 循环发送所有剩余分片
        while (m_currentFragmentIndex < m_pendingFragments.size())
        {
            m_dataCollectionTask->sendDataToBackend();
        }
    }
}
```

## 修复效果

修复后的行为：
- 前 N-1 个激活时隙：每个时隙发送一个分片（N = deviceSlotCount）
- 最后一个激活时隙：发送对应的分片后，如果还有剩余分片，继续发送所有剩余分片

### 示例场景

**场景：6 个分片，5 个激活时隙**

修复前：
```
activePin=0: 发送分片 1/6
activePin=1: 发送分片 2/6
activePin=2: 发送分片 3/6
activePin=3: 发送分片 4/6
activePin=4: 发送分片 5/6
[分片 6/6 未发送] ❌
```

修复后：
```
activePin=0: 发送分片 1/6
activePin=1: 发送分片 2/6
activePin=2: 发送分片 3/6
activePin=3: 发送分片 4/6
activePin=4: 发送分片 5/6，然后继续发送分片 6/6 ✓
```

## 日志变化

修复后会在日志中看到类似以下输出：

```
[W] Last active slot reached but 1 fragments remaining, sending all remaining fragments now
[I] Sending fragment 6/6 (size: XXX bytes)
[I] Fragment 6/6 sent successfully
[I] All fragments (6) successfully sent to backend
```

## 注意事项

1. **性能影响**：在最后一个激活时隙可能会发送多个分片，这会增加该时隙的处理时间
2. **建议**：尽量优化数据打包效率，使分片数量不超过激活时隙数量
3. **替代方案**：如果需要发送大量数据，可以考虑增加 `testCount` 值以获得更多激活时隙

## 相关文件

- `User/app/slave_device.cpp` - 主要修改文件
- `User/app/master_slave_message_handlers.cpp` - testCount 配置处理
- `User/slot_manager/slot_manager.cpp` - 时隙管理实现
- `protocol/ProtocolProcessor.cpp` - 数据打包和分片逻辑

## 修改日期

2026-01-28
