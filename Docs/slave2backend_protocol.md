# Slave2Backend 协议文档

## 1. 概述

Slave2Backend协议用于从机（Slave）向后台（Backend）发送数据。该协议定义了三种消息类型：导通数据、阻值数据和卡钉数据。

## 2. 协议常量

- **帧分隔符1**: `0xAB`
- **帧分隔符2**: `0xCD`
- **Packet ID**: `0x04` (SLAVE_TO_BACKEND)

## 3. 帧结构

### 3.1 完整帧格式

```
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| Delimiter1 (1B)  | Delimiter2 (1B)  | Packet ID (1B)   | Frag Seq (1B)    | More Frag (1B)   | Packet Len (2B)  | Payload (N B)    |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
| 0xAB             | 0xCD             | 0x04             | 0-255            | 0/1              | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+------------------+------------------+------------------+
```

### 3.2 字段说明

| 字段名 | 长度 | 说明 |
|--------|------|------|
| Delimiter1 | 1字节 | 固定值 `0xAB`，帧起始标识 |
| Delimiter2 | 1字节 | 固定值 `0xCD`，帧起始标识 |
| Packet ID | 1字节 | 固定值 `0x04`，标识为Slave2Backend消息 |
| Fragments Sequence | 1字节 | 分片序列号，用于分片重组。单帧时为0 |
| More Fragments Flag | 1字节 | 更多分片标志：`0`=最后一帧，`1`=还有后续分片 |
| Packet Length | 2字节 | 载荷长度（小端序），不包括帧头7字节 |
| Payload | N字节 | 载荷数据，具体格式见下文 |

## 4. 载荷结构

### 4.1 载荷格式

```
+------------------+------------------+------------------+------------------+
| Message ID (1B)  | Slave ID (4B)    | Device Status(2B)| Message Data (N) |
+------------------+------------------+------------------+------------------+
| 0x00/0x01/0x02   | Little Endian    | Little Endian    | Variable         |
+------------------+------------------+------------------+------------------+
```

### 4.2 载荷字段说明

| 字段名 | 长度 | 说明 |
|--------|------|------|
| Message ID | 1字节 | 消息类型标识，见消息类型章节 |
| Slave ID | 4字节 | 从机ID（小端序） |
| Device Status | 2字节 | 设备状态（小端序），位域定义见下文 |
| Message Data | N字节 | 消息数据，格式取决于消息类型 |

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
| 0x00 | CONDUCTION_DATA_MSG | 导通数据消息 |
| 0x01 | RESISTANCE_DATA_MSG | 阻值数据消息 |
| 0x02 | CLIP_DATA_MSG | 卡钉数据消息 |

## 7. 消息详细格式

### 7.1 导通数据消息 (CONDUCTION_DATA_MSG)

**Message ID**: `0x00`

**消息数据格式**：
```
+------------------+------------------+------------------+
| Conduction Length| Conduction Data  |
+------------------+------------------+------------------+
| 2 bytes (LE)     | N bytes          |
+------------------+------------------+------------------+
```

**字段说明**：
- `Conduction Length` (2字节，小端序): 导通数据长度（字节数）
- `Conduction Data` (N字节): 导通数据内容，长度为 `Conduction Length` 字节

**完整载荷示例**（假设 Slave ID = 0x00000001，Device Status = 0x0001，Conduction Length = 4，Data = [0x01, 0x02, 0x03, 0x04]）：
```
00 01 00 00 00 01 00 04 00 01 02 03 04
│  │  └─────────┘  │  └─────┘  └─────────────┘
│  │  Slave ID     │  Length   Data
│  │               │
│  │               Device Status
│  │
│  Message ID
```

### 7.2 阻值数据消息 (RESISTANCE_DATA_MSG)

**Message ID**: `0x01`

**消息数据格式**：
```
+------------------+------------------+------------------+
| Resistance Length| Resistance Data  |
+------------------+------------------+------------------+
| 2 bytes (LE)     | N bytes          |
+------------------+------------------+------------------+
```

**字段说明**：
- `Resistance Length` (2字节，小端序): 阻值数据长度（字节数）
- `Resistance Data` (N字节): 阻值数据内容，长度为 `Resistance Length` 字节

**完整载荷示例**（假设 Slave ID = 0x00000002，Device Status = 0x0008，Resistance Length = 6，Data = [0x10, 0x20, 0x30, 0x40, 0x50, 0x60]）：
```
01 02 00 00 00 08 00 06 00 10 20 30 40 50 60
│  │  └─────────┘  │  └─────┘  └─────────────────────┘
│  │  Slave ID     │  Length   Data
│  │               │
│  │               Device Status
│  │
│  Message ID
```

### 7.3 卡钉数据消息 (CLIP_DATA_MSG)

**Message ID**: `0x02`

**消息数据格式**：
```
+------------------+
| Clip Data        |
+------------------+
| 2 bytes (LE)     |
+------------------+
```

**字段说明**：
- `Clip Data` (2字节，小端序): 卡钉数据值

**完整载荷示例**（假设 Slave ID = 0x00000003，Device Status = 0x0010，Clip Data = 0x1234）：
```
02 03 00 00 00 10 00 34 12
│  │  └─────────┘  │  └─────┘
│  │  Slave ID     │  Clip Data
│  │               │
│  │               Device Status
│  │
│  Message ID
```

## 8. 完整帧示例

### 8.1 导通数据消息完整帧

假设：
- Slave ID = `0x12345678`
- Device Status = `0x0005` (clrSensor=1, pressureSensor=1)
- Conduction Length = `3`
- Conduction Data = `[0xAA, 0xBB, 0xCC]`

**完整帧**：
```
AB CD 04 00 00 0E 00 78 56 34 12 05 00 03 00 AA BB CC
│  │  │  │  │  │  │  └─────────┘  │  └─────┘  └─────┘
│  │  │  │  │  │  │  Slave ID      │  Length   Data
│  │  │  │  │  │  │                │
│  │  │  │  │  │  │                Device Status
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID
│  │  │  │  │  │
│  │  │  │  │  Packet Length (14 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag
│  │  │  │
│  │  │  Fragments Sequence
│  │  │
│  │  Packet ID (SLAVE_TO_BACKEND)
│  │
│  Delimiter2
│
Delimiter1
```

### 8.2 卡钉数据消息完整帧

假设：
- Slave ID = `0x87654321`
- Device Status = `0x0020` (electromagneticLock1=1)
- Clip Data = `0x5678`

**完整帧**：
```
AB CD 04 00 00 09 02 21 43 65 87 20 00 78 56
│  │  │  │  │  │  │  └─────────┘  │  └─────┘
│  │  │  │  │  │  │  Slave ID      │  Clip Data
│  │  │  │  │  │  │                │
│  │  │  │  │  │  │                Device Status
│  │  │  │  │  │  │
│  │  │  │  │  │  Message ID (0x02)
│  │  │  │  │  │
│  │  │  │  │  Packet Length (9 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag
│  │  │  │
│  │  │  Fragments Sequence
│  │  │
│  │  Packet ID
│  │
│  Delimiter2
│
Delimiter1
```

## 9. 分片机制

当消息长度超过MTU（默认100字节）时，协议支持自动分片。

### 9.1 分片规则

1. **分片序列号** (Fragments Sequence): 从0开始递增，每个分片都有唯一的序列号
2. **更多分片标志** (More Fragments Flag):
   - `0`: 这是最后一个分片
   - `1`: 还有后续分片
3. **分片超时**: 分片重组超时时间为5000毫秒

### 9.2 分片示例

假设一个大的导通数据消息需要分片：

**第一帧**（还有后续分片）：
```
AB CD 04 00 01 64 [payload 93 bytes...]
│  │  │  │  │  │
│  │  │  │  │  Packet Length (100 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 1 (还有后续)
│  │  │  │
│  │  │  Fragments Sequence = 0
```

**第二帧**（最后一帧）：
```
AB CD 04 01 00 1F [payload 25 bytes...]
│  │  │  │  │  │
│  │  │  │  │  Packet Length (31 bytes)
│  │  │  │  │
│  │  │  │  More Fragments Flag = 0 (最后一帧)
│  │  │  │
│  │  │  Fragments Sequence = 1
```

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

1. 准备消息数据（导通数据、阻值数据或卡钉数据）
2. 构建载荷：
   - 添加消息类型标识（Message ID）
   - 添加从机ID（Slave ID，4字节）
   - 添加设备状态（Device Status，2字节）
   - 添加消息数据（Message Data）
3. 构建帧：
   - 添加帧分隔符（0xAB 0xCD）
   - 添加Packet ID（0x04）
   - 添加分片信息（序列号和标志）
   - 添加载荷长度
   - 添加载荷数据
4. 分片处理：如果帧长度超过MTU（默认100字节），自动进行分片
5. 发送：将帧数据通过通信链路发送

### 11.2 接收流程

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
   - 提取设备状态（Device Status）
   - 根据消息类型解析消息数据
6. 消息处理：根据消息类型进行相应的业务处理

## 12. 错误处理

### 12.1 帧验证

- 帧分隔符必须为 `0xAB 0xCD`
- Packet ID 必须为 `0x04`
- Packet Length 必须与实际载荷长度匹配
- 帧总长度必须至少为7字节（帧头）

### 12.2 消息验证

- Message ID 必须在有效范围内（0x00-0x02）
- 消息数据长度必须符合消息类型的要求
- 对于变长消息，数据长度字段必须与实际数据长度匹配

### 12.3 分片处理

- 分片超时：如果5秒内未收到所有分片，丢弃该分片组
- 分片序列号必须连续
- 最后一个分片的 More Fragments Flag 必须为0

## 13. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024 | 初始版本，定义三种消息类型 |

## 14. 附录

### 14.1 相关协议

- **Master2Slave协议**: 主机到从机的通信协议
- **Slave2Master协议**: 从机到主机的通信协议
- **Backend2Master协议**: 后台到主机的通信协议
- **Master2Backend协议**: 主机到后台的通信协议

---

**文档维护**: 请根据协议变更及时更新本文档。
