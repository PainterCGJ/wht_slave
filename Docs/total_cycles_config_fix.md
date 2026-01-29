# TotalCycles 配置变化检测修复

## 问题描述

### 现象

从用户日志看到：
```
[第一次同步]
Total Pin Count (All Devices): 73
TestCount (This Device): 20
data: 183 bytes  ← (73 × 20) / 8 = 182.5 → 183 bytes ✓

[第二次同步]
Total Pin Count (All Devices): 23  ← 变化了！
TestCount (This Device): 20       ← 没变
data: 183 bytes  ← 应该是 (23 × 20) / 8 = 57.5 → 58 bytes ✗
```

**问题**：Total Pin Count 从 73 变为 23，但数据大小还是 183 bytes（应该是 58 bytes）

### 根本原因

配置比较逻辑**不完整**，只比较了本设备的 `testCount`，没有比较系统的 `totalCycles`（总引脚数）。

#### 修复前的代码

```cpp
// 只比较本设备的配置
if (oldConfig.mode != newMode || 
    oldConfig.interval != syncMsg->interval || 
    oldConfig.timeSlot != newTimeSlot ||
    oldConfig.testCount != newTestCount)  // ← 只比较本设备引脚数
{
    configChanged = true;
}
```

**问题**：
- testCount (本设备) = 20 → 20 (没变)
- totalCycles (系统总引脚数) = 73 → 23 (变了)
- 结果：`configChanged = false` ✗

因为配置被认为没变，所以：
1. 不会清空缓存数据 `lastCollectionData`
2. 不会重新配置采集器
3. 发送的是旧的 183 字节数据（应该是 58 字节）

## 修复方案

### 1. 添加 totalCycles 的计算和比较

```cpp
// 5. 计算新的totalCycles（所有从机的testCount之和）
uint16_t newTotalCycles = 0;
for (const auto &cfg : syncMsg->slaveConfigs)
{
    newTotalCycles += cfg.testCount;
}

// 计算旧的totalCycles（如果已配置）
uint16_t oldTotalCycles = 0;
if (device->m_isConfigured && device->m_slotManager)
{
    oldTotalCycles = device->m_slotManager->GetTotalSlots();
}

// 6. 比较配置是否改变（包括totalCycles）
bool configChanged = false;
if (oldConfig.mode != newMode || 
    oldConfig.interval != syncMsg->interval || 
    oldConfig.timeSlot != newTimeSlot ||
    oldConfig.testCount != newTestCount || 
    oldTotalCycles != newTotalCycles)  // ← 新增：比较总引脚数
{
    configChanged = true;
    elog_d("SyncMessageHandler",
           "Configuration changed - ..., TotalCycles: %d->%d",
           ..., oldTotalCycles, newTotalCycles);
}
```

### 2. 配置改变时的处理

```cpp
// 7. 如果配置改变，清除缓存数据
if (configChanged)
{
    elog_d("SyncMessageHandler", "Config changed, clearing cached data");
    
    // 清除分片发送状态
    device->m_isFragmentSendingInProgress = false;
    device->m_pendingFragments.clear();
    device->m_currentFragmentIndex = 0;
    
    // 清除缓存的采集数据 ← 重要！
    device->lastCollectionData.clear();
    device->m_hasDataToSend = false;
    
    elog_d("SyncMessageHandler", "Cached data cleared, next transmission will use new data size");
}
```

### 3. 使用新的 totalCycles 配置采集器

```cpp
// 配置采集器时使用新计算的 newTotalCycles
CollectorConfig collectorConfig(device->currentConfig.testCount, newTotalCycles);

// 计算预期的数据量
size_t expectedDataBits = newTotalCycles * device->currentConfig.testCount;
size_t expectedDataBytes = (expectedDataBits + 7) / 8;

elog_d("SyncMessageHandler", "Expected data: %d bytes (%d bits), ~%d frags", 
       expectedDataBytes, expectedDataBits, (expectedDataBytes + 799) / 800);
```

### 4. 配置时隙管理器时使用新的 totalCycles

```cpp
uint16_t totalSlotCount = newTotalCycles;  // 使用新计算的值
```

## 修复后的行为

### 场景：totalCycles 从 73 变为 23

**第一次同步**：
```
[D] Total Pin Count (All Devices): 73
[D] TestCount (This Device): 20
[D] Expected data: 183 bytes (1460 bits), ~1 frags
[I] data: 183 bytes  ← 正确
```

**第二次同步**：
```
[D] Total Pin Count (All Devices): 23  ← 检测到变化
[D] Configuration changed - TotalCycles: 73->23  ← 识别为配置改变
[D] Config changed, clearing cached data  ← 清空缓存
[D] Cached data cleared, next transmission will use new data size
[D] TestCount (This Device): 20
[D] Expected data: 58 bytes (460 bits), ~1 frags  ← 预期58字节
[I] data: 58 bytes  ← 实际58字节 ✓
```

## 数据大小计算公式

```
数据位数 = totalCycles × testCount
数据字节数 = (数据位数 + 7) / 8  // 向上取整

示例1: (73 × 20) / 8 = 1460 / 8 = 182.5 → 183 bytes
示例2: (23 × 20) / 8 = 460 / 8 = 57.5 → 58 bytes
```

## 新增的日志输出

### 配置变化检测

```
[D] SyncMessageHandler: Configuration changed - Mode: 0->0, Interval: 100->100, 
    TimeSlot: 0->0, TestCount: 20->20, TotalCycles: 73->23
```

### 缓存清理确认

```
[D] SyncMessageHandler: Config changed, clearing cached data
[D] SyncMessageHandler: Cached data cleared, next transmission will use new data size
```

### 预期数据大小

```
[D] SyncMessageHandler: Expected data: 58 bytes (460 bits), ~1 frags
```

### 实际数据大小

```
[I] DataCollectionTask: data: 58 bytes  ← 应该与预期匹配
```

## 验证方法

### 1. 检查配置变化日志

如果 totalCycles 变化，应该看到：
```
[D] Configuration changed - ..., TotalCycles: XX->YY
```

### 2. 检查缓存清理日志

配置变化后，应该看到：
```
[D] Config changed, clearing cached data
[D] Cached data cleared, next transmission will use new data size
```

### 3. 验证数据大小

计算预期值：
```
预期字节数 = (totalCycles × testCount + 7) / 8
```

对比实际值：
```
[D] Expected data: XX bytes
[I] data: XX bytes  ← 应该与预期值一致
```

## 可能出现配置变化的场景

### 1. 从机数量变化

```
场景1: 3个从机 (58+58+58) → 2个从机 (58+58)
totalCycles: 174 → 116
数据大小: 1305 bytes → 870 bytes
```

### 2. 单个从机的 testCount 变化

```
场景2: 从机A testCount从58变为20
totalCycles: 174 (58+58+58) → 136 (20+58+58)
数据大小: 1305 bytes → 1020 bytes
```

### 3. 从机加入或离开

```
场景3: 新增一个从机 testCount=20
totalCycles: 174 (58+58+58) → 194 (58+58+58+20)
数据大小: 1305 bytes → 1455 bytes
```

## 相关代码文件

- `User/app/master_slave_message_handlers.cpp` - 修复配置比较逻辑
- `User/app/slave_device.cpp` - 使用缓存数据发送

## 总结

### 问题

❌ 只比较 testCount，不比较 totalCycles  
❌ 配置变化时不清空缓存  
❌ 使用旧数据发送，大小不匹配  

### 修复

✅ 添加 totalCycles 比较  
✅ 配置变化时清空缓存  
✅ 重新配置采集器  
✅ 数据大小与配置匹配  

### 效果

修复后，totalCycles 变化会被正确检测，缓存会被清空，下次发送的数据大小将与新配置匹配。
