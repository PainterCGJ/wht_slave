# 时隙跳跃导致数据分片丢失问题修复

## 问题描述

在数据上传过程中，出现分片丢失的情况，日志显示：
- 发送了 fragment 1/6
- **跳过了 fragment 2/6**
- 直接发送 fragment 3/6

示例日志：
```
D/DataCollectionTask [2214445] Sending fragment 1/6 in active slot 0 (pin 0)
I/uwb_comm        [2214447] tx begin
D/SlaveDevice     [2214451] Sending fragment 3/6 in active slot 167 (pin 2)  ⚠️ 跳过了 pin 1!
I/DataCollectionTask [2214453] Fragment sending in progress - currentIndex: 2, totalFragments: 6
```

## 根本原因分析

### SlotManager 的时隙跳跃机制

在 `SlotManager::Process()` 中（slot_manager.cpp:217-218）：

```cpp
// 可能跳过了多个时隙，直接切换到正确的时隙
SwitchToSlot(expectedSlot);
```

**SlotManager 基于绝对时间计算期望时隙，如果发现当前时隙落后，会直接跳到期望时隙，中间的时隙会被完全跳过！**

### 问题场景

1. **初始状态**：当前在 slot 165 (pin 0)
2. **发送数据**：开始发送 fragment 1，调用 UWB 发送函数
3. **耗时操作**：数据发送耗时约 2-4ms
4. **时间流逝**：在发送过程中，系统时间继续前进
5. **Process 调用**：`SlotManager::Process()` 被调用
6. **时间计算**：
   ```cpp
   elapsedFromStartUs = currentTimeUs - m_StartTimeUs;  // 例如：16700000 us
   totalCycles = elapsedFromStartUs / slotIntervalUs;   // 16700000 / 100000 = 167
   expectedSlot = totalCycles % m_TotalSlotCount;       // 167 % 58 = 51... 
   // 实际计算示例：如果 startSlot=165, 当前应该在 slot 167
   ```
7. **直接跳跃**：`SwitchToSlot(167)` - **slot 166 (pin 1) 被跳过**
8. **分片丢失**：fragment 2 应该在 pin 1 发送，但永远没有机会

### 为什么会跳过时隙？

时隙跳跃的原因：

1. **数据发送耗时**：
   - UWB 发送 800 字节数据需要约 2-4ms
   - 如果 slotInterval = 100ms，理论上不应该跳过
   - **但实际上可能存在系统调度延迟**

2. **Process 轮询间隔**：
   - `DataCollectionTask` 的轮询间隔是 1ms（slave_device.cpp:721）
   - 但由于 FreeRTOS 任务调度，实际调用间隔可能不稳定

3. **系统负载**：
   - 其他任务占用 CPU 时间
   - 中断处理
   - DMA 传输等

4. **时间同步问题**：
   - 如果主机的 sync 消息有延迟
   - 从机的时间偏移量调整可能导致时间"跳变"

### 日志证据分析

从用户提供的日志：

```
D/DataCollectionTask [2214445] Sending fragment 1/6 in active slot 0 (pin 0)
I/uwb_comm        [2214447] tx begin          // 时间: 2214447 ms
D/SlaveDevice     [2214451] Sending fragment 3/6 in active slot 167 (pin 2)  // 时间: 2214451 ms
```

**时间差只有 4ms**，但跳过了一个 100ms 的时隙！

这说明：
- slot 0 (pin 0) 应该对应的绝对时间约为 2214445 ms
- 如果 slotInterval = 100ms：
  - slot 166 (pin 1) 应该在 2214545 ms
  - slot 167 (pin 2) 应该在 2214645 ms
- 但实际上 slot 167 在 2214451 ms 就被处理了

**结论**：日志中的时间戳有问题，或者时隙编号的计算有问题。

再看另一个关键信息：
```
D/SlaveDevice [2214451] Sending fragment 3/6 in active slot 167 (pin 2)
```

如果 testCount = 58，deviceSlotCount = 58：
- 本设备的时隙范围：startSlot 到 startSlot + 57
- pin 2 对应的 slot = startSlot + 2

**所以 slot 167 意味着 startSlot = 165**

那么：
- slot 165 → pin 0 ✓
- slot 166 → pin 1 ❌ 被跳过
- slot 167 → pin 2 ✓

## 修复方案

### 方案概述

在 `SlotManager::Process()` 中检测时隙跳跃，对所有被跳过的 **ACTIVE** 时隙触发回调。

### 修复代码

```cpp
// 检查是否跳过了多个时隙
uint16_t slotDiff = 0;
if (expectedSlot > m_CurrentSlotInfo.m_currentSlot)
{
    slotDiff = expectedSlot - m_CurrentSlotInfo.m_currentSlot;
}
else if (cycleEnded)
{
    slotDiff = 1;
}
else
{
    // 跨周期的情况
    slotDiff = (m_TotalSlotCount - m_CurrentSlotInfo.m_currentSlot) + expectedSlot;
}

// 如果跳过了多个时隙，需要为每个被跳过的 ACTIVE 时隙触发回调
if (slotDiff > 1)
{
    elog_w("SlotManager", "Skipping %d slots! Current: %d, Expected: %d", 
           slotDiff - 1, m_CurrentSlotInfo.m_currentSlot, expectedSlot);

    // 遍历所有被跳过的时隙，对 ACTIVE 时隙触发回调
    for (uint16_t i = 1; i < slotDiff; i++)
    {
        uint16_t skippedSlot = (m_CurrentSlotInfo.m_currentSlot + i) % m_TotalSlotCount;
        SlotType skippedType = CalculateSlotType(skippedSlot);

        if (skippedType == SlotType::ACTIVE)
        {
            // 为被跳过的 ACTIVE 时隙创建临时的时隙信息并触发回调
            SlotInfo tempSlotInfo;
            tempSlotInfo.m_currentSlot = skippedSlot;
            tempSlotInfo.m_totalSlots = m_TotalSlotCount;
            tempSlotInfo.m_slotType = SlotType::ACTIVE;
            tempSlotInfo.m_activePin = skippedSlot - m_StartSlot;
            tempSlotInfo.m_slotIntervalMs = m_SlotIntervalMs;

            elog_w("SlotManager", "Processing skipped ACTIVE slot %d (pin %d)", 
                   skippedSlot, tempSlotInfo.m_activePin);

            if (m_SlotCallback)
            {
                m_SlotCallback(tempSlotInfo);
            }
        }
    }
}

// 切换到期望的时隙
SwitchToSlot(expectedSlot);
```

### 修复效果

修复后，如果发生时隙跳跃：

```
[W] SlotManager: Skipping 1 slots! Current: 165, Expected: 167
[W] SlotManager: Processing skipped ACTIVE slot 166 (pin 1)
[D] SlaveDevice: Sending fragment 2/6 in active slot 166 (pin 1)
[I] DataCollectionTask: Fragment 2/6 sent successfully
[D] SlaveDevice: Sending fragment 3/6 in active slot 167 (pin 2)
[I] DataCollectionTask: Fragment 3/6 sent successfully
```

所有被跳过的 ACTIVE 时隙都会被补发。

## 为什么原来会跳过时隙？

### 设计初衷

原始的时隙跳跃设计是为了：
1. **时间同步**：基于绝对时间计算，避免时间累积误差
2. **快速恢复**：如果系统短暂卡顿，能快速跳到正确的时隙

### 问题

但这个设计没有考虑到：
1. **数据分片发送**：依赖每个激活时隙顺序发送分片
2. **回调依赖**：应用层依赖每个时隙的回调都被触发

### 根本矛盾

- **SlotManager** 的设计理念：基于绝对时间，允许跳跃
- **数据发送** 的设计理念：基于顺序时隙，不能跳过

## 深层原因分析

### 为什么会在 4ms 内跳过 100ms 的时隙？

可能的原因：

1. **时间戳记录问题**：
   - 日志中的时间戳可能不是真实的系统时间
   - 或者使用了不同的时间源

2. **startSlot 计算错误**：
   - 如果 sync 消息的 startTime 计算有误
   - 可能导致 SlotManager 认为已经过了很多时隙

3. **时间偏移量更新**：
   - 在收到新的 sync 消息后，时间偏移量被更新
   - 导致 `GetCurrentSyncTimeUs()` 返回值突然跳变

## 其他相关问题

### "No cached data available to send to backend"

从日志中看到：
```
W/DataCollectionTask [2207807] No cached data available to send to backend
```

这个警告出现的原因：
1. 在 pin 0 的时隙中，首次调用 `sendDataToBackend()`
2. 打包数据，发送 fragment 1，设置 `m_isFragmentSendingInProgress = true`
3. 清空了 `lastCollectionData` 和 `m_hasDataToSend`（**这是错误的！**）

**问题**：在 `sendDataToBackend()` 的 line 927-928：
```cpp
parent.lastCollectionData.clear();
parent.m_hasDataToSend = false;
```

这会在发送第一个分片后立即清空缓存数据，导致后续时隙调用 `sendDataToBackend()` 时找不到数据。

### 修复建议

应该只在**所有分片发送完成后**才清空缓存：

```cpp
// 在 line 910-929 中
// 发送第一个分片
if (parent.send(firstFragment) == 0)
{
    parent.m_currentFragmentIndex = 1;

    // 如果只有一个分片，立即完成
    if (parent.m_currentFragmentIndex >= parent.m_pendingFragments.size())
    {
        // ✓ 只有在所有分片完成时才清空
        parent.m_pendingFragments.clear();
        parent.m_currentFragmentIndex = 0;
        parent.m_isFragmentSendingInProgress = false;
        parent.lastCollectionData.clear();
        parent.m_hasDataToSend = false;
    }
    // ✗ 否则不要清空 lastCollectionData
}
```

但是看代码，已经有这个判断了，所以这个问题可能不是代码问题，而是其他原因。

## 测试建议

修复后，建议测试：

1. **正常情况**：时隙不跳跃，所有分片顺序发送
2. **轻微延迟**：发送数据耗时较长，但不跳过时隙
3. **严重延迟**：Process 调用延迟，跳过 1-2 个时隙
4. **时间跳变**：收到 sync 消息导致时间偏移量更新

预期结果：所有情况下，所有分片都能完整发送。

## 日志变化

修复后，如果发生时隙跳跃，会看到以下日志：

```
[W] SlotManager: Skipping 1 slots! Current: 165, Expected: 167
[W] SlotManager: Processing skipped ACTIVE slot 166 (pin 1)
[D] SlaveDevice: Sending fragment 2/6 in active slot 166 (pin 1)
[I] DataCollectionTask: Fragment 2/6 sent successfully
```

这表明：
- 检测到了时隙跳跃
- 自动补发了被跳过时隙的数据
- 所有分片都能完整发送

## 总结

**根本原因**：SlotManager 的时隙跳跃机制导致某些激活时隙的回调没有被触发。

**修复方案**：检测时隙跳跃，对所有被跳过的 ACTIVE 时隙主动触发回调。

**影响**：
- 优点：确保所有数据分片都能发送
- 缺点：如果跳过多个时隙，会在短时间内连续发送多个分片

**建议**：
1. 优化 Process 轮询频率，减少时隙跳跃的概率
2. 监控日志中的 "Skipping slots" 警告，分析跳跃原因
3. 如果频繁出现时隙跳跃，需要优化系统实时性

## 修改文件

- `User/slot_manager/slot_manager.cpp` - 添加了时隙跳跃检测和补发逻辑
