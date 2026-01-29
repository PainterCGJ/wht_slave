# testCount 配置与激活时隙关系分析

## 问题描述

设备有 58 个物理引脚，但数据上传时只显示上传了 5/6 包，最后一包没有上传。

## testCount 的含义

根据代码分析，`testCount` 参数有**双重作用**：

### 1. 决定采集器检测的引脚数量

```cpp
// User/app/slave_device.cpp:746
CollectorConfig collectorConfig(parent.currentConfig.testCount, totalCycles);
```

在 `CollectorConfig` 中：
- 第一个参数 `m_num` = `testCount` → **本设备负责检测的引脚数量**
- 数据矩阵大小 = `totalCycles × m_num`

### 2. 决定设备的激活时隙数量

```cpp
// User/app/master_slave_message_handlers.cpp:198
uint8_t deviceSlotCount = device->currentConfig.testCount;
```

- `deviceSlotCount` = `testCount` → **本设备的激活时隙数量**
- 激活时隙用于：
  1. 控制引脚输出电平进行导通检测
  2. 发送数据分片到主机

## 问题分析

### 理论上的情况

如果设备有 58 个引脚需要检测：
- **应该配置**：`testCount = 58`
- **结果**：
  - 检测 58 个引脚
  - 有 58 个激活时隙可用
  - 数据分片数量即使是 6 个也能完整发送

### 实际观察到的情况

数据只发送了 5/6 包，说明：
- **实际配置**：`testCount = 5`（推测）
- **结果**：
  - 只有 5 个激活时隙可用
  - 无法发送第 6 个分片

### 可能的原因

#### 原因 1：配置错误
主机端配置错误，`testCount` 应该设置为 58 而不是 5。

#### 原因 2：多设备共享引脚检测
可能的架构：
- 系统中有多个从机（比如 58/5 ≈ 12 个从机）
- 每个从机负责检测 5 个引脚
- 总共检测 58 个引脚

但这种情况下，每个从机的数据量应该很小：
```
数据量 = totalCycles × 5引脚 ÷ 8 = totalCycles × 0.625 字节
```

要达到 6 个分片（假设每个分片 ~800 字节），需要：
```
totalCycles ≈ 7680 次
```

这个周期数异常大，不太合理。

#### 原因 3：testCount 实际配置与预期不符

需要检查实际运行时的 `testCount` 值。

## 诊断方法

### 1. 查看日志输出

在设备启动后，查找以下日志：

```
[V] SyncMessageHandler: Found config for device 0xXXXXXXXX - TimeSlot: X, TestCount: X, Reset: X
[V] SyncMessageHandler: Continuity collector configured - TestCount: X, TotalCycles: X
[V] SlotManager: Configured - Start: X, Count: X, Total: X, Interval: Xms
```

关键信息：
- **TestCount** 的实际值
- **TotalCycles** 的实际值
- **SlotManager 的 Count**（应该等于 TestCount）

### 2. 查看数据分片日志

在数据发送时，查找以下日志：

```
[I] Total conduction test data bytes: XXX bytes
[V] Data split into X fragments, will send one fragment per slot
[D] Sending fragment X/X in active slot X (pin X)
```

关键信息：
- **数据总字节数**
- **分片总数量**
- **实际发送到第几个分片**

### 3. 计算数据量

根据日志中的信息计算：

```
数据矩阵大小 = TotalCycles × TestCount
数据字节数 = (TotalCycles × TestCount + 7) / 8
理论分片数 = 数据字节数 / MTU大小 (约800字节)
```

## 解决方案

### 方案 1：修正 testCount 配置（推荐）

如果确实需要检测 58 个引脚，在主机端配置中设置：

```cpp
slaveConfig.testCount = 58;  // 设置为实际需要检测的引脚数量
```

**优点**：
- 从根本上解决问题
- 每个引脚有独立的激活时隙
- 发送效率最高

### 方案 2：使用当前的临时修复（已实现）

已修改 `OnSlotChanged()` 函数，在最后一个激活时隙发送所有剩余分片。

**优点**：
- 无需修改配置
- 确保所有数据都能发送

**缺点**：
- 最后一个时隙可能发送多个分片，处理时间较长
- 治标不治本

### 方案 3：优化数据打包

如果数据量确实很大，考虑：
1. 增加数据压缩
2. 减少 totalCycles（降低采样频率）
3. 分批次发送数据

## 建议的检查步骤

1. **立即检查**：查看设备日志，确认 `TestCount` 的实际配置值
2. **对比配置**：检查主机端配置文件，确认 `testCount` 是否正确
3. **计算验证**：根据实际的 TestCount 和 TotalCycles 计算理论数据量
4. **对比实际**：将计算值与日志中的实际数据量对比
5. **确定方案**：根据对比结果选择合适的解决方案

## 日志示例分析

### 示例 1：testCount = 5 的情况

```
[V] SyncMessageHandler: Found config - TestCount: 5, TotalCycles: 58
[I] Total conduction test data bytes: 37 bytes  // (58×5+7)/8 = 37
[V] Data split into 1 fragments                 // 37字节不需要分片
[D] Sending fragment 1/1 in active slot 0 (pin 0)
```

**分析**：数据量小，不会出现问题。

### 示例 2：testCount = 5 但数据量异常大

```
[V] SyncMessageHandler: Found config - TestCount: 5, TotalCycles: 7680
[I] Total conduction test data bytes: 4800 bytes  // (7680×5+7)/8 = 4800
[V] Data split into 6 fragments                   // 需要6个分片
[D] Sending fragment 1/6 in active slot 0 (pin 0)
...
[D] Sending fragment 5/6 in active slot 4 (pin 4)
[W] Last active slot reached but 1 fragments remaining
[D] Sending fragment 6/6 (补发)
```

**分析**：totalCycles 异常大（7680），需要确认是否合理。

### 示例 3：testCount = 58 的正常情况

```
[V] SyncMessageHandler: Found config - TestCount: 58, TotalCycles: 100
[I] Total conduction test data bytes: 725 bytes   // (100×58+7)/8 = 725
[V] Data split into 1 fragments                   // 或2个分片
[D] Sending fragment 1/1 in active slot 0 (pin 0)
```

**分析**：配置正确，运行正常。

## 结论

当前问题很可能是 `testCount` 配置不当导致的。建议：

1. **优先**：检查实际运行日志，确认 testCount 和数据量
2. **然后**：如果 testCount 确实配置错误，修正配置
3. **最后**：如果配置无法修改，使用已实现的临时修复方案

临时修复已经实现，可以确保所有数据都能发送，但建议从根本上解决配置问题。
