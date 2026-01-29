# 引脚数配置诊断日志说明

## 概述

为了诊断数据上传不完整的问题，已在关键位置添加 debug 级别的日志输出，用于显示：
1. testCount（引脚数）的配置值
2. 激活时隙的数量
3. 预期的数据量和分片数量
4. 分片数量与激活时隙数量的对比

## 添加的日志位置

### 1. 接收同步消息时 (master_slave_message_handlers.cpp)

**位置**：`SyncMessageHandler::ProcessMessage()` - 找到设备配置后

**日志输出示例**：
```
[V] SyncMessageHandler: Found config for device 0x12345678 - TimeSlot: 0, TestCount: 5, Reset: 0
[D] SyncMessageHandler: === Pin Configuration Debug Info ===
[D] SyncMessageHandler: Device ID: 0x12345678
[D] SyncMessageHandler: TestCount (Pin Count): 5
[D] SyncMessageHandler: This device will have 5 active slots for data transmission
[D] SyncMessageHandler: ====================================
```

**关键信息**：
- `TestCount (Pin Count)`: 实际配置的引脚数量
- `active slots for data transmission`: 可用于发送数据的激活时隙数量

### 2. 配置采集器时

**位置**：`SyncMessageHandler::ProcessMessage()` - 配置 ContinuityCollector 后

**日志输出示例**：
```
[D] SyncMessageHandler: Continuity collector configured - TestCount: 5, TotalCycles: 58
[D] SyncMessageHandler: Expected data size: 290 bits = 37 bytes
[D] SyncMessageHandler: With MTU=800, expected fragments: ~1
```

**关键信息**：
- `Expected data size`: 预期产生的数据量（字节）
- `expected fragments`: 根据 MTU 计算的预期分片数量

**数据量计算公式**：
```
数据位数 = TotalCycles × TestCount
数据字节数 = (数据位数 + 7) / 8
预期分片数 = (数据字节数 + MTU - 1) / MTU
```

### 3. 配置时隙管理器时

**位置**：`SyncMessageHandler::ProcessMessage()` - 配置 SlotManager 后

**日志输出示例**：
```
[D] SyncMessageHandler: Slot manager configured (single cycle) - StartSlot: 0, TotalSlots: 58, Interval: 100 ms
[D] SyncMessageHandler: === Slot Manager Configuration ===
[D] SyncMessageHandler: StartSlot: 0 (0x0000)
[D] SyncMessageHandler: DeviceSlotCount (Active Slots): 5
[D] SyncMessageHandler: TotalSlotCount: 58
[D] SyncMessageHandler: SlotInterval: 100 ms
[D] SyncMessageHandler: Active Pin Range: 0 to 4
[D] SyncMessageHandler: ==================================
```

**关键信息**：
- `DeviceSlotCount (Active Slots)`: 本设备的激活时隙数量
- `Active Pin Range`: 激活引脚的范围（从 0 开始）
- `TotalSlotCount`: 整个系统的总时隙数

### 4. 数据分片时 (slave_device.cpp)

**位置**：`DataCollectionTask::sendDataToBackend()` - 打包数据后

**日志输出示例**：
```
[I] Total conduction test data bytes: 4800 bytes
[D] === Data Fragmentation Info ===
[D] Total data bytes: 4800
[D] Total fragments: 6
[D] Available active slots: 5 (testCount)
[W] WARNING: Fragment count (6) exceeds active slots (5)!
[W] Extra fragments will be sent in the last active slot
[D] ===============================
```

**关键信息**：
- `Total data bytes`: 实际产生的数据字节数
- `Total fragments`: 实际分片数量
- `Available active slots`: 可用的激活时隙数量（来自 testCount）
- **WARNING**: 如果分片数超过激活时隙数，会显示警告

## 如何使用这些日志诊断问题

### 步骤 1：查看 testCount 配置

在日志中搜索：
```
Pin Configuration Debug Info
```

查看输出的 `TestCount (Pin Count)` 值，例如：
- 如果显示 `TestCount (Pin Count): 5` → 只配置了 5 个引脚
- 如果显示 `TestCount (Pin Count): 58` → 配置了 58 个引脚

### 步骤 2：查看预期数据量

在日志中搜索：
```
Expected data size
```

查看预期的数据量和分片数，例如：
```
Expected data size: 4800 bits = 600 bytes
With MTU=800, expected fragments: ~1
```

### 步骤 3：查看实际数据量

在日志中搜索：
```
Data Fragmentation Info
```

查看实际的数据量和分片数，例如：
```
Total data bytes: 4800
Total fragments: 6
Available active slots: 5
```

### 步骤 4：对比分析

对比预期值和实际值：

#### 场景 1：testCount = 5，但数据量异常大

```
TestCount: 5
TotalCycles: 7680  // 异常大！
Expected data size: 4800 bytes
Total fragments: 6
WARNING: Fragment count (6) exceeds active slots (5)!
```

**问题**：TotalCycles 配置异常大（7680），导致数据量超出预期。

**原因分析**：
- 如果系统中有多个从机，TotalCycles = 所有从机的 testCount 之和
- 如果有 1536 个从机，每个 testCount=5，则 TotalCycles=7680
- 或者主机端配置错误

**解决方案**：检查主机端的从机配置数量和参数。

#### 场景 2：testCount = 5，数据量正常

```
TestCount: 5
TotalCycles: 58
Expected data size: 37 bytes
Total fragments: 1
```

**结论**：配置正常，数据量不大，不会出现上传不完整的问题。

#### 场景 3：testCount = 58，数据量正常

```
TestCount: 58
TotalCycles: 100
Expected data size: 725 bytes
Total fragments: 1
Available active slots: 58
```

**结论**：配置正确，有足够的激活时隙发送数据。

#### 场景 4：testCount 配置错误

```
TestCount: 5  // 错误！应该是 58
TotalCycles: 58
Expected data size: 37 bytes
```

**问题**：testCount 应该配置为 58，但实际配置为 5。

**影响**：
- 只检测 5 个引脚而不是 58 个
- 只有 5 个激活时隙

**解决方案**：修改主机端配置，将 testCount 设置为 58。

## 日志级别说明

所有诊断日志使用 `elog_d()` (DEBUG 级别)，在发布版本中可以关闭。

关键警告使用 `elog_w()` (WARNING 级别)，始终显示。

## 常见诊断结果

### 正常情况

```
TestCount: 58
TotalCycles: 58 (或更大)
Expected fragments: 1-2
Available active slots: 58
```
✓ 配置正确，运行正常

### 问题情况 1：配置错误

```
TestCount: 5
TotalCycles: 58
WARNING: Fragment count (6) exceeds active slots (5)!
```
⚠️ testCount 配置错误，应该是 58

### 问题情况 2：数据量异常

```
TestCount: 5
TotalCycles: 7680  // 异常！
Expected data size: 4800 bytes
Total fragments: 6
```
⚠️ TotalCycles 配置异常，需要检查主机端配置

## 建议的诊断流程

1. **查看日志** → 记录 testCount、TotalCycles、fragments 的值
2. **计算验证** → 用公式验证数据量是否合理
3. **对比配置** → 确认 testCount 是否与实际引脚数匹配
4. **检查警告** → 如果有 "Fragment count exceeds active slots" 警告，说明配置有问题
5. **修正配置** → 根据分析结果修正主机端的配置

## 总结

通过这些详细的 debug 日志，可以清楚地看到：
- 设备实际配置了多少个引脚（testCount）
- 有多少个激活时隙可用
- 数据被分成了多少个分片
- 分片数量是否超过激活时隙数量

这些信息足以帮助诊断和定位数据上传不完整的根本原因。
