# Slave2Master 协议文档

## 1. 概述

Slave2Master协议用于从机（Slave）向主机（Master）发送消息。该协议定义了多种消息类型，包括复位响应、心跳、入网请求、导通数据等。

## 2. 协议常量

- **帧分隔符1**: `0xAB`
- **帧分隔符2**: `0xCD`
- **Packet ID**: `0x01` (SLAVE_TO_MASTER)
- **默认MTU**: 100字节

## 3. 帧结构

### 3.1 完整帧格式

所有Slave2Master消息均采用统一的帧格式，帧头固定为7字节：

```
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| Delimiter1 (1B)  | Delimiter2 (1B)  | Packet ID (1B)   | Frag Seq (1B)    | More Frag (1B)   | Length Low (1B)  | Length High (1B) | Payload (N B)    |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| 0xAB             | 0xCD             | 0x01             | 0-255            | 0/1              | Little Endian    | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
```

### 3.2 字段说明

| 字段名 | 位置 | 长度 | 说明 |
|--------|------|------|------|
| Delimiter1 | 0 | 1字节 | 固定值 `0xAB`，帧起始标识 |
| Delimiter2 | 1 | 1字节 | 固定值 `0xCD`，帧起始标识 |
| Packet ID | 2 | 1字节 | 固定值 `0x01`，标识为Slave2Master消息 |
| Fragments Sequence | 3 | 1字节 | 分片序列号，用于分片重组。单帧时为0 |
| More Fragments Flag | 4 | 1字节 | 更多分片标志：`0`=最后一帧，`1`=还有后续分片 |
| Packet Length Low | 5 | 1字节 | 载荷长度低字节（小端序），不包括帧头7字节 |
| Packet Length High | 6 | 1字节 | 载荷长度高字节（小端序） |
| Payload | 7+ | N字节 | 载荷数据，具体格式见下文 |

**重要说明**：帧头总长度固定为7字节，Packet Length字段使用小端序表示载荷长度（不包括帧头）。

## 4. 载荷结构

### 4.1 标准消息载荷格式

对于大多数消息类型（RST_RSP_MSG, PING_RSP_MSG, JOIN_REQUEST_MSG, SHORT_ID_CONFIRM_MSG, HEARTBEAT_MSG），载荷格式为：

```
+------------------+------------------+------------------+
| Message ID (1B)  | Slave ID (4B)    | Message Data (N) |
+------------------+------------------+------------------+
| 0x30-0x52        | Little Endian    | Variable         |
+------------------+------------------+------------------+
```

**字段说明**：
- **Message ID** (1字节): 消息类型标识，见消息类型章节
- **Slave ID** (4字节): 从机ID（小端序）
- **Message Data** (N字节): 消息数据，格式取决于消息类型

### 4.2 COND_DATA_MSG 特殊载荷格式

对于导通数据消息（COND_DATA_MSG），载荷格式为：

```
+------------------+------------------+------------------+------------------+
| Message ID (1B)  | Slave ID (4B)    | Device Status(2B)| Conduction Data  |
+------------------+------------------+------------------+------------------+
| 0x53             | Little Endian    | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+
```

**字段说明**：
- **Message ID** (1字节): 固定值 `0x53`，标识为导通数据消息
- **Slave ID** (4字节): 从机ID（小端序）
- **Device Status** (2字节): 设备状态（小端序），位域定义见下文
- **Conduction Data** (N字节): 导通数据内容，长度从包长度推算（Packet Length - 7字节）

**重要说明**：
- COND_DATA_MSG 消息**不需要**在消息数据中包含长度字段，因为长度可以从包长度（Packet Length）字段推算
- 导通数据长度 = Packet Length - 7字节（Message ID + Slave ID + Device Status）
- 在分包时，**每一包的载荷都必须包含 Message ID + Slave ID + Device Status**，确保后续包在没有消息头部的情况下也能知道数据来自哪个从机

## 5. 设备状态（Device Status）

设备状态使用16位（2字节）表示，采用小端序传输。位域定义如下：

| 位 | 字段名 | 说明 |
|----|--------|------|
| 0 | clrSensor | 清洁传感器状态 |
| 1 | sleeveLimit | 套筒限位状态 |
| 2 | electromagnetUnlockButton | 电磁解锁按钮状态 |
| 3 | batteryLowAlarm | 电池低电量告警 |
| 4 | pressureSensor | 压力传感器状态 |
| 5 | electromagneticLock1 | 电磁锁1状态 |
| 6 | electromagneticLock2 | 电磁锁2状态 |
| 7 | accessory1 | 附件1状态 |
| 8 | accessory2 | 附件2状态 |
| 9-15 | reserved | 保留位，当前为0 |

**示例**：
- 如果 `clrSensor = 1`，则 `Device Status = 0x0001`
- 如果 `batteryLowAlarm = 1`，则 `Device Status = 0x0008`
- 如果 `clrSensor = 1` 且 `pressureSensor = 1`，则 `Device Status = 0x0011`

## 6. 消息类型

### 6.1 消息类型枚举

| Message ID | 消息类型 | 说明 | 状态 |
|------------|----------|------|------|
| 0x30 | RST_RSP_MSG | 复位响应消息 | 已实现 |
| 0x41 | PING_RSP_MSG | Ping响应消息 | 已实现 |
| 0x50 | JOIN_REQUEST_MSG | 入网请求消息 | 已禁用 |
| 0x51 | SHORT_ID_CONFIRM_MSG | 短ID确认消息 | 已实现 |
| 0x52 | HEARTBEAT_MSG | 心跳消息 | 已禁用 |
| 0x53 | COND_DATA_MSG | 导通数据消息 | **已实现（主要功能）** |

## 7. 消息详细格式

### 7.1 复位响应消息 (RST_RSP_MSG)

**Message ID**: `0x30`

**消息数据格式**：
```
+------------------+
| Status (1B)      |
+------------------+
```

**字段说明**：
- `Status` (1字节): 复位状态，0=复位成功，1=复位异常

**完整载荷示例**（假设 Slave ID = 0x00000001，Status = 0）：
```
30 01 00 00 00 00
│  └─────────┘  │
│  Slave ID     Status
│
Message ID
```

### 7.2 Ping响应消息 (PING_RSP_MSG)

**Message ID**: `0x41`

**消息数据格式**：
```
+------------------+------------------+
| Sequence (2B)    | Timestamp (4B)   |
+------------------+------------------+
| Little Endian    | Little Endian    |
+------------------+------------------+
```

**字段说明**：
- `Sequence` (2字节，小端序): 序列号
- `Timestamp` (4字节，小端序): 时间戳

### 7.3 入网请求消息 (JOIN_REQUEST_MSG) - 已禁用

**Message ID**: `0x50`

**消息数据格式**：
```
+------------------+------------------+------------------+------------------+
| Device ID (4B)   | Ver Major (1B)   | Ver Minor (1B)   | Ver Patch (2B)   |
+------------------+------------------+------------------+------------------+
| Little Endian    |                  |                  | Little Endian    |
+------------------+------------------+------------------+------------------+
```

**字段说明**：
- `Device ID` (4字节，小端序): 设备ID
- `Version Major` (1字节): 主版本号
- `Version Minor` (1字节): 次版本号
- `Version Patch` (2字节，小端序): 补丁版本号

**注意**：
- 当前系统中入网请求功能已被禁用，此消息类型保留但未使用
- 代码实现中Device ID字段与外层Slave ID存在冗余，这是历史遗留问题

### 7.4 短ID确认消息 (SHORT_ID_CONFIRM_MSG)

**Message ID**: `0x51`

**消息数据格式**：
```
+------------------+------------------+
| Status (1B)      | Short ID (1B)    |
+------------------+------------------+
```

**字段说明**：
- `Status` (1字节): 状态
- `Short ID` (1字节): 短ID

### 7.5 心跳消息 (HEARTBEAT_MSG) - 已禁用

**Message ID**: `0x52`

**消息数据格式**：
```
+------------------+
| Battery Level(1B)|
+------------------+
```

**字段说明**：
- `Battery Level` (1字节): 电池电量百分比 (0-100%)

**注意**：当前系统中心跳功能已被禁用。

### 7.6 导通数据消息 (COND_DATA_MSG) - 主要功能

**Message ID**: `0x53`

**消息数据格式**：
```
+------------------+------------------+
| Device Status(2B)| Conduction Data  |
+------------------+------------------+
| Little Endian    | N bytes          |
+------------------+------------------+
```

**字段说明**：
- `Device Status` (2字节，小端序): 设备状态，位域定义见第5节
- `Conduction Data` (N字节): 导通数据内容，**不包含长度字段**，长度从包长度推算

**完整载荷示例**（假设 Slave ID = 0x02307615，Device Status = 0x0008，Conduction Data = [0x01, 0x02, 0x03, 0x04]）：
```
53 15 76 30 02 08 00 01 02 03 04
│  └─────────┘  └─────┘  └─────────────┘
│  Slave ID     Status    Conduction Data
│
Message ID (0x53)
```

**导通数据长度计算**：
- 导通数据长度 = Packet Length - 7字节（Message ID + Slave ID + Device Status）

## 8. 完整帧示例

### 8.1 心跳消息完整帧

假设：
- Slave ID = `0x12345678`
- Battery Level = `80` (0x50)

**完整帧**：
```
AB CD 01 00 00 06 00 52 78 56 34 12 50
│  │  │  │  │  └────┘  │  └─────────┘  │
│  │  │  │  │  Length  │  Slave ID     Battery
│  │  │  │  │          │
│  │  │  │  │          Message ID (0x52)
│  │  │  │  │
│  │  │  │  More Fragments Flag (0 = 最后一帧)
│  │  │  │
│  │  │  Fragments Sequence (0)
│  │  │
│  │  Packet ID (SLAVE_TO_MASTER = 0x01)
│  │
│  Delimiter2 (0xCD)
│
Delimiter1 (0xAB)
```

**字节解析**：
- 位置 0-1: 帧分隔符 `AB CD`
- 位置 2: Packet ID = `01`
- 位置 3: Fragments Sequence = `00`
- 位置 4: More Fragments Flag = `00`
- 位置 5-6: Packet Length = `06 00` (小端序，值=6)
- 位置 7: Message ID = `52`
- 位置 8-11: Slave ID = `78 56 34 12` (小端序，值=0x12345678)
- 位置 12: Battery Level = `50` (80%)

### 8.2 导通数据消息完整帧（单帧）

假设：
- Slave ID = `0x02307615`
- Device Status = `0x0008` (batteryLowAlarm=1)
- Conduction Data = `[0xAA, 0xBB, 0xCC]` (3字节)

**完整帧**：
```
AB CD 01 00 00 0A 00 53 15 76 30 02 08 00 AA BB CC
│  │  │  │  │  └────┘  │  └─────────┘  └─────┘  └─────┘
│  │  │  │  │  Length  │  Slave ID     Status   Data
│  │  │  │  │          │
│  │  │  │  │          Message ID (0x53)
│  │  │  │  │
│  │  │  │  More Fragments Flag (0 = 最后一帧)
│  │  │  │
│  │  │  Fragments Sequence (0)
│  │  │
│  │  Packet ID (SLAVE_TO_MASTER = 0x01)
│  │
│  Delimiter2
│
Delimiter1
```

**字节解析**：
- 位置 0-1: 帧分隔符 `AB CD`
- 位置 2: Packet ID = `01`
- 位置 3: Fragments Sequence = `00`
- 位置 4: More Fragments Flag = `00`
- 位置 5-6: Packet Length = `0A 00` (小端序，值=10)
- 位置 7: Message ID = `53`
- 位置 8-11: Slave ID = `15 76 30 02` (小端序，值=0x02307615)
- 位置 12-13: Device Status = `08 00` (小端序，值=0x0008)
- 位置 14-16: Conduction Data = `AA BB CC`

## 9. 分片机制

当消息长度超过MTU（默认100字节）时，协议支持自动分片。

### 9.1 标准消息分片规则

对于标准消息（非COND_DATA_MSG），分片规则如下：

1. **分片序列号** (Fragments Sequence): 从0开始递增，每个分片都有唯一的序列号
2. **更多分片标志** (More Fragments Flag):
   - `0`: 这是最后一个分片
   - `1`: 还有后续分片
3. **分片超时**: 分片重组超时时间为5000毫秒
4. **分片载荷**: 每个分片包含原始载荷的一部分

### 9.2 COND_DATA_MSG 特殊分片规则

对于导通数据消息（COND_DATA_MSG），分片规则有特殊要求：

**关键特性**：每个分片都必须包含完整的识别信息（Message ID + Slave ID + Device Status），这样即使接收到单个分片，也能识别数据来源。

**分片载荷结构**：
```
第一包: [Message ID][Slave ID][Device Status][Conduction Data Part 1]
第二包: [Message ID][Slave ID][Device Status][Conduction Data Part 2]
第三包: [Message ID][Slave ID][Device Status][Conduction Data Part 3]
...
```

**导通数据长度计算**：
- 每包的导通数据长度 = Packet Length - 7字节（Message ID + Slave ID + Device Status）
- 总导通数据长度 = 所有分片的导通数据长度之和

**分片示例**：

假设：
- Slave ID = `0x02307615`
- Device Status = `0x0008`
- Conduction Data = 900字节
- MTU = 100字节
- 每个分片的导通数据 = 100 - 7(帧头) - 7(消息头) = 86字节

需要分片数 = ⌈900 / 86⌉ = 11个分片

**第一帧**（分片0，还有后续）：
```
AB CD 01 00 01 5D 00 53 15 76 30 02 08 00 [86 bytes conduction data...]
│  │  │  │  │  └────┘  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  Len=93  │  Slave ID     Status   Conduction Data Part 1
│  │  │  │  │          │
│  │  │  │  │          Message ID (0x53)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 1 (还有后续)
│  │  │  │
│  │  │  Fragments Sequence = 0
│  │  │
│  │  Packet ID (0x01)
│  │
│  Delimiter2
│
Delimiter1
```

**第二帧**（分片1，还有后续）：
```
AB CD 01 01 01 5D 00 53 15 76 30 02 08 00 [86 bytes conduction data...]
│  │  │  │  │  └────┘  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  Len=93  │  Slave ID     Status   Conduction Data Part 2
│  │  │  │  │          │
│  │  │  │  │          Message ID (0x53)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 1 (还有后续)
│  │  │  │
│  │  │  Fragments Sequence = 1
```

**最后一帧**（分片10，最后一帧）：
```
AB CD 01 0A 00 3A 00 53 15 76 30 02 08 00 [52 bytes conduction data...]
│  │  │  │  │  └────┘  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  Len=59  │  Slave ID     Status   Conduction Data Part 11
│  │  │  │  │          │
│  │  │  │  │          Message ID (0x53)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 0 (最后一帧)
│  │  │  │
│  │  │  Fragments Sequence = 10 (0x0A)
```

**重要说明**：
- 每个分片都包含完整的 Message ID + Slave ID + Device Status（7字节），确保数据可追溯
- 接收端需要将所有分片的导通数据部分拼接起来，得到完整的导通数据
- 分片序列号必须连续，从0开始
- 最后一个分片的 More Fragments Flag 必须为0

## 10. 字节序说明

协议中所有多字节数值均采用**小端序**（Little Endian）：
- 16位值：低字节在前，高字节在后
  - 例如：`0x1234` 传输为 `[0x34, 0x12]`
- 32位值：最低字节在前，最高字节在后
  - 例如：`0x12345678` 传输为 `[0x78, 0x56, 0x34, 0x12]`
- 64位值：最低字节在前，最高字节在后
  - 例如：`0x0123456789ABCDEF` 传输为 `[0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01]`

## 11. 协议处理流程

### 11.1 发送流程

#### 标准消息发送流程

1. 准备消息数据
2. 构建载荷：
   - 添加消息类型标识（Message ID）
   - 添加从机ID（Slave ID，4字节）
   - 添加消息数据（Message Data）
3. 构建帧：
   - 添加帧分隔符（0xAB 0xCD）
   - 添加Packet ID（0x01）
   - 添加分片信息（序列号和标志）
   - 添加载荷长度（2字节，小端序）
   - 添加载荷数据
4. 分片处理：如果帧长度超过MTU（默认100字节），自动进行分片
5. 发送：将帧数据通过通信链路发送

#### COND_DATA_MSG 发送流程

1. 准备导通数据
2. 构建载荷：
   - 添加消息类型标识（Message ID = 0x53）
   - 添加从机ID（Slave ID，4字节）
   - 添加设备状态（Device Status，2字节）
   - 添加导通数据（**不包含长度字段**）
3. 构建帧：
   - 添加帧分隔符（0xAB 0xCD）
   - 添加Packet ID（0x01）
   - 添加分片序列号
   - 添加更多分片标志
   - 预留载荷长度字段（2字节）
   - 添加载荷数据
   - 回填载荷长度字段
4. 分片处理：如果帧长度超过MTU（默认100字节），进行特殊分片：
   - **每个分片都必须包含 Message ID + Slave ID + Device Status**
   - 每个分片包含部分导通数据
5. 发送：将帧数据通过通信链路发送

### 11.2 接收流程

#### 标准消息接收流程

1. 接收原始字节流
2. 帧识别：查找帧分隔符（0xAB 0xCD）
3. 帧解析：
   - 验证帧头（分隔符、Packet ID）
   - 读取载荷长度（2字节，小端序）
   - 提取载荷数据
4. 分片重组：如果收到分片，等待所有分片到达后重组
5. 载荷解析：
   - 提取消息类型（Message ID）
   - 提取从机ID（Slave ID）
   - 根据消息类型解析消息数据
6. 消息处理：根据消息类型进行相应的业务处理

#### COND_DATA_MSG 接收流程

1. 接收原始字节流
2. 帧识别：查找帧分隔符（0xAB 0xCD）
3. 帧解析：
   - 验证帧头（分隔符、Packet ID）
   - 读取载荷长度（2字节，小端序）
   - 提取载荷数据
4. 分片重组：如果收到分片，进行特殊重组：
   - 从每个分片中提取 Message ID + Slave ID + Device Status（验证一致性）
   - 提取每个分片的导通数据部分
   - 将所有分片的导通数据拼接起来
5. 载荷解析：
   - 提取消息类型（Message ID = 0x53）
   - 提取从机ID（Slave ID）
   - 提取设备状态（Device Status）
   - 计算导通数据长度：Packet Length - 7字节
   - 提取导通数据
6. 消息处理：根据消息类型进行相应的业务处理

## 12. 错误处理

### 12.1 帧验证

- 帧分隔符必须为 `0xAB 0xCD`
- Packet ID 必须为 `0x01`
- Packet Length 必须与实际载荷长度匹配
- 帧总长度必须至少为7字节（帧头）

### 12.2 消息验证

- Message ID 必须在有效范围内（0x30, 0x41, 0x50-0x53）
- 消息数据长度必须符合消息类型的要求
- 对于COND_DATA_MSG，导通数据长度必须与 Packet Length - 7 匹配

### 12.3 分片处理

- 分片超时：如果5秒内未收到所有分片，丢弃该分片组
- 分片序列号必须连续
- 最后一个分片的 More Fragments Flag 必须为0
- 对于COND_DATA_MSG，每个分片的 Message ID + Slave ID + Device Status 必须一致

## 13. 实现要点

### 13.1 帧头构建

**重要**：帧头长度字段（位置5-6）必须在构建帧时预留，避免后续设置时覆盖载荷数据。

正确的实现：
```cpp
packFrameBuffer_.push_back(FRAME_DELIMITER_1);  // 位置0
packFrameBuffer_.push_back(FRAME_DELIMITER_2);  // 位置1
packFrameBuffer_.push_back(packetId);           // 位置2
packFrameBuffer_.push_back(fragmentSeq);        // 位置3
packFrameBuffer_.push_back(moreFragmentsFlag);  // 位置4
packFrameBuffer_.push_back(0);                  // 位置5 - 长度低字节（稍后设置）
packFrameBuffer_.push_back(0);                  // 位置6 - 长度高字节（稍后设置）
// 然后从位置7开始添加payload
// ... 添加payload数据 ...
// 最后回填长度字段
uint16_t payloadLength = static_cast<uint16_t>(packFrameBuffer_.size() - 7);
packFrameBuffer_[5] = payloadLength & 0xFF;
packFrameBuffer_[6] = (payloadLength >> 8) & 0xFF;
```

### 13.2 COND_DATA_MSG 分片

在分片COND_DATA_MSG时，必须确保每个分片都包含完整的识别信息：

```cpp
// 从原始载荷中提取识别信息
uint8_t messageId = originalPayload[0];
uint32_t slaveId = readUint32LE(originalPayload, 1);
uint16_t deviceStatus = readUint16LE(originalPayload, 5);

// 构建每个分片
for (每个分片) {
    // 添加7字节帧头（包括预留的长度字段）
    // ...
    
    // 添加识别信息到分片载荷
    packFrameBuffer_.push_back(messageId);
    writeUint32LE(packFrameBuffer_, slaveId);
    writeUint16LE(packFrameBuffer_, deviceStatus);
    
    // 添加分片的导通数据部分
    packFrameBuffer_.insert(/* 当前分片的导通数据 */);
    
    // 回填长度字段
    uint16_t payloadLength = static_cast<uint16_t>(packFrameBuffer_.size() - 7);
    packFrameBuffer_[5] = payloadLength & 0xFF;
    packFrameBuffer_[6] = (payloadLength >> 8) & 0xFF;
}
```

## 14. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024 | 初始版本，定义基本消息类型 |
| 2.0 | 2025-01 | 新增COND_DATA_MSG消息类型，集成原Slave2Backend的导通数据功能 |
| 2.1 | 2025-01 | 修正帧头长度字段描述，明确7字节帧头结构；更新实现要点章节 |

## 15. 附录

### 15.1 相关协议

- **Master2Slave协议**: 主机到从机的通信协议
- **Backend2Master协议**: 后台到主机的通信协议
- **Master2Backend协议**: 主机到后台的通信协议

### 15.2 从Slave2Backend迁移到Slave2Master

原Slave2Backend协议中的导通数据消息已迁移到Slave2Master协议中，主要变化：

1. **Packet ID**: 从 `0x04` (SLAVE_TO_BACKEND) 改为 `0x01` (SLAVE_TO_MASTER)
2. **Message ID**: 从 `0x00` (CONDUCTION_DATA_MSG) 改为 `0x53` (COND_DATA_MSG)
3. **载荷结构**：
   - 原格式：`[Message ID][Slave ID][Device Status][Conduction Length][Conduction Data]`
   - 新格式：`[Message ID][Slave ID][Device Status][Conduction Data]`（**移除了长度字段**）
4. **分片处理**: 新协议要求每个分片都包含 Message ID + Slave ID + Device Status

### 15.3 常见问题

**Q: 为什么帧头是7字节而不是其他长度？**
A: 帧头包括：2字节分隔符 + 1字节Packet ID + 1字节分片序列号 + 1字节更多分片标志 + 2字节载荷长度 = 7字节

**Q: 为什么COND_DATA_MSG每个分片都要包含识别信息？**
A: 这样设计是为了在网络不稳定或分片乱序到达时，接收端仍然能够识别每个分片的来源，提高系统的鲁棒性。

**Q: 导通数据的最大长度是多少？**
A: 理论上受限于分片序列号的最大值（255），每个分片最多携带约93字节导通数据，因此最大约为 255 × 93 ≈ 23KB。实际使用中通常远小于此值。

---

**文档维护**: 本文档与源码保持同步更新。最后更新：2025-01
