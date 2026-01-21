# Slave2Master 协议文档

## 1. 概述

Slave2Master协议用于从机（Slave）向主机（Master）发送消息。该协议定义了多种消息类型，包括复位响应、心跳、入网请求、导通数据等。

## 2. 协议常量

- **帧分隔符1**: `0xAB`
- **帧分隔符2**: `0xCD`
- **Packet ID**: `0x01` (SLAVE_TO_MASTER)

## 3. 帧结构

### 3.1 完整帧格式

```
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| Delimiter1 (1B)  | Delimiter2 (1B)  | Packet ID (1B)   | Frag Seq (1B)    | More Frag (1B)   | Packet Len (2B)  | Payload (N B)    |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| 0xAB             | 0xCD             | 0x01             | 0-255            | 0/1              | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
```

### 3.2 字段说明

| 字段名 | 长度 | 说明 |
|--------|------|------|
| Delimiter1 | 1字节 | 固定值 `0xAB`，帧起始标识 |
| Delimiter2 | 1字节 | 固定值 `0xCD`，帧起始标识 |
| Packet ID | 1字节 | 固定值 `0x01`，标识为Slave2Master消息 |
| Fragments Sequence | 1字节 | 分片序列号，用于分片重组。单帧时为0 |
| More Fragments Flag | 1字节 | 更多分片标志：`0`=最后一帧，`1`=还有后续分片 |
| Packet Length | 2字节 | 载荷长度（小端序），不包括帧头7字节 |
| Payload | N字节 | 载荷数据，具体格式见下文 |

## 4. 载荷结构

### 4.1 标准消息载荷格式

对于大多数消息类型（RST_RSP_MSG, PING_RSP_MSG, JOIN_REQUEST_MSG, SHORT_ID_CONFIRM_MSG, HEARTBEAT_MSG），载荷格式为：

```
+------------------+------------------+------------------+
| Message ID (1B)  | Slave ID (4B)   | Message Data (N)|
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
+------------------+------------------+------------------+------------------+------------------+
| Message ID (1B)  | Slave ID (4B)   | Device Status(2B)| Conduction Data  |
+------------------+------------------+------------------+------------------+------------------+
| 0x53             | Little Endian   | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+------------------+
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

| Message ID | 消息类型 | 说明 |
|------------|----------|------|
| 0x30 | RST_RSP_MSG | 复位响应消息 |
| 0x41 | PING_RSP_MSG | Ping响应消息 |
| 0x50 | JOIN_REQUEST_MSG | 入网请求消息 |
| 0x51 | SHORT_ID_CONFIRM_MSG | 短ID确认消息 |
| 0x52 | HEARTBEAT_MSG | 心跳消息 |
| 0x53 | COND_DATA_MSG | 导通数据消息 |

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
+------------------+------------------+------------------+
| Sequence (2B)   | Timestamp (4B)   |
+------------------+------------------+------------------+
| Little Endian    | Little Endian    |
+------------------+------------------+------------------+
```

**字段说明**：
- `Sequence` (2字节，小端序): 序列号
- `Timestamp` (4字节，小端序): 时间戳

### 7.3 入网请求消息 (JOIN_REQUEST_MSG)

**Message ID**: `0x50`

**消息数据格式**：
```
+------------------+------------------+------------------+------------------+
| Version Major(1B)| Version Minor(1B)| Version Patch(2B)|
+------------------+------------------+------------------+------------------+
|                  |                  | Little Endian    |
+------------------+------------------+------------------+------------------+
```

**字段说明**：
- `Version Major` (1字节): 主版本号
- `Version Minor` (1字节): 次版本号
- `Version Patch` (2字节，小端序): 补丁版本号

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

### 7.5 心跳消息 (HEARTBEAT_MSG)

**Message ID**: `0x52`

**消息数据格式**：
```
+------------------+
| Battery Level(1B)|
+------------------+
```

**字段说明**：
- `Battery Level` (1字节): 电池电量百分比 (0-100%)

### 7.6 导通数据消息 (COND_DATA_MSG)

**Message ID**: `0x53`

**消息数据格式**：
```
+------------------+------------------+------------------+
| Device Status(2B)| Conduction Data  |
+------------------+------------------+------------------+
| Little Endian     | N bytes         |
+------------------+------------------+------------------+
```

**字段说明**：
- `Device Status` (2字节，小端序): 设备状态，位域定义见第5节
- `Conduction Data` (N字节): 导通数据内容，**不包含长度字段**，长度从包长度推算

**完整载荷示例**（假设 Slave ID = 0x00000001，Device Status = 0x0001，Conduction Data = [0x01, 0x02, 0x03, 0x04]）：
```
53 01 00 00 00 01 00 01 02 03 04
│  └─────────┘  └─────┘  └─────────────┘
│  Slave ID     Status    Conduction Data
│
Message ID
```

**注意**：导通数据长度 = Packet Length - 7字节（Message ID + Slave ID + Device Status）

## 8. 完整帧示例

### 8.1 心跳消息完整帧

假设：
- Slave ID = `0x12345678`
- Battery Level = `50`

**完整帧**：
```
AB CD 01 00 00 06 52 78 56 34 12 32
│  │  │  │  │  │  │  └─────────┘  │
│  │  │  │  │  │  │  Slave ID      Battery Level
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x52)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (6 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag
│  │  │  │
│  │  │  Fragments Sequence
│  │  │
│  │  Packet ID (SLAVE_TO_MASTER)
│  │
│  Delimiter2
│
Delimiter1
```

### 8.2 导通数据消息完整帧（单帧）

假设：
- Slave ID = `0x12345678`
- Device Status = `0x0005` (clrSensor=1, pressureSensor=1)
- Conduction Data = `[0xAA, 0xBB, 0xCC]`

**完整帧**：
```
AB CD 01 00 00 0A 53 78 56 34 12 05 00 AA BB CC
│  │  │  │  │  │  │  └─────────┘  └─────┘  └─────┘
│  │  │  │  │  │  │  Slave ID      Status   Data
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x53)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (10 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag
│  │  │  │
│  │  │  Fragments Sequence
│  │  │
│  │  Packet ID (SLAVE_TO_MASTER)
│  │
│  Delimiter2
│
Delimiter1
```

## 9. 分片机制

当消息长度超过MTU（默认100字节）时，协议支持自动分片。

### 9.1 标准消息分片规则

对于标准消息（非COND_DATA_MSG），分片规则如下：

1. **分片序列号** (Fragments Sequence): 从0开始递增，每个分片都有唯一的序列号
2. **更多分片标志** (More Fragments Flag):
   - `0`: 这是最后一个分片
   - `1`: 还有后续分片
3. **分片超时**: 分片重组超时时间为5000毫秒
4. **分片载荷**: 每个分片包含原始载荷的一部分，第一个分片包含完整的 Message ID + Slave ID + Message Data 的一部分

### 9.2 COND_DATA_MSG 特殊分片规则

对于导通数据消息（COND_DATA_MSG），分片规则有特殊要求：

1. **每包必须包含完整头部**: 每个分片的载荷都必须包含：
   - Message ID (1字节)
   - Slave ID (4字节)
   - Device Status (2字节)
   - 部分导通数据

2. **分片载荷结构**: 
   ```
   第一包: [Message ID][Slave ID][Device Status][Conduction Data Part 1]
   第二包: [Message ID][Slave ID][Device Status][Conduction Data Part 2]
   第三包: [Message ID][Slave ID][Device Status][Conduction Data Part 3]
   ...
   ```

3. **导通数据长度计算**: 
   - 每包的导通数据长度 = Packet Length - 7字节（Message ID + Slave ID + Device Status）
   - 总导通数据长度 = 所有分片的导通数据长度之和

4. **分片示例**:

假设：
- Slave ID = `0x12345678`
- Device Status = `0x0005`
- Conduction Data = `[0x01, 0x02, ..., 0xFF]` (200字节)
- MTU = 100字节

**第一帧**（还有后续分片）：
```
AB CD 01 00 01 64 53 78 56 34 12 05 00 [91 bytes conduction data...]
│  │  │  │  │  │  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  │  │  Slave ID      Status   Conduction Data Part 1
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x53)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (100 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 1 (还有后续)
│  │  │  │
│  │  │  Fragments Sequence = 0
```

**第二帧**（还有后续分片）：
```
AB CD 01 01 01 64 53 78 56 34 12 05 00 [91 bytes conduction data...]
│  │  │  │  │  │  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  │  │  Slave ID      Status   Conduction Data Part 2
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x53)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (100 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 1 (还有后续)
│  │  │  │
│  │  │  Fragments Sequence = 1
```

**第三帧**（最后一帧）：
```
AB CD 01 02 00 1A 53 78 56 34 12 05 00 [18 bytes conduction data...]
│  │  │  │  │  │  │  └─────────┘  └─────┘  └─────────────────────┘
│  │  │  │  │  │  │  Slave ID      Status   Conduction Data Part 3
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x53)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (26 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 0 (最后一帧)
│  │  │  │
│  │  │  Fragments Sequence = 2
```

**重要说明**：
- 每个分片都包含完整的 Message ID + Slave ID + Device Status，这样即使在没有消息头部的情况下，接收端也能知道数据来自哪个从机
- 接收端需要将所有分片的导通数据部分拼接起来，得到完整的导通数据

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
   - 添加载荷长度
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
   - 添加分片信息（序列号和标志）
   - 添加载荷长度
   - 添加载荷数据
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
   - 读取载荷长度
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
   - 读取载荷长度
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

## 13. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024 | 初始版本，定义基本消息类型 |
| 2.0 | 2024 | 新增COND_DATA_MSG消息类型，集成原Slave2Backend的导通数据功能 |

## 14. 附录

### 14.1 相关协议

- **Master2Slave协议**: 主机到从机的通信协议
- **Backend2Master协议**: 后台到主机的通信协议
- **Master2Backend协议**: 主机到后台的通信协议

### 14.2 从Slave2Backend迁移到Slave2Master

原Slave2Backend协议中的导通数据消息已迁移到Slave2Master协议中，主要变化：

1. **Packet ID**: 从 `0x04` (SLAVE_TO_BACKEND) 改为 `0x01` (SLAVE_TO_MASTER)
2. **Message ID**: 从 `0x00` (CONDUCTION_DATA_MSG) 改为 `0x53` (COND_DATA_MSG)
3. **载荷结构**: 
   - 原格式：`[Message ID][Slave ID][Device Status][Conduction Length][Conduction Data]`
   - 新格式：`[Message ID][Slave ID][Device Status][Conduction Data]`（**移除了长度字段**）
4. **分片处理**: 新协议要求每个分片都包含 Message ID + Slave ID + Device Status

---

**文档维护**: 请根据协议变更及时更新本文档。
