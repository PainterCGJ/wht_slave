## Device Address
| 节点 | 地址码 |
| :--- | :--- |
| 测试工装 (Tooling) | `0x01` |
| 被测底板 (Device) | `0x02` |


## Protocol Format
| 字段 | | | 长度 (Bytes) | 描述 |
| --- | --- | --- | :--- | :--- |
| SOF | | | 2 | 起始帧 (Start of Frame): `0xAA55`<br/>按字节打包时遵循小端模式为 55 AA |
| MSG | info | Source | 1 | 源地址 |
| | | Target | 1 | 目标地址 |
| | | MSG ID | 1 | 命令码，定义操作的大类 |
| | | payload length | 2 | `Payload` 字段的字节长度 (小端模式) |
| | payload | | N | 数据负载，用于传递具体参数或子命令 |
| CRC16 | | | 2 | 对 MSG 字段进行 CRC16-MODBUS 校验 |
| EOF | | | 2 | 结束帧 (End of Frame): `0x66BB`<br/>按字节打包时遵循小端模式为 BB 66 |


## Tooling(0x01)<->Device(0x02)
| MSG | MSG ID | Description |
| --- | --- | --- |
| General IO Control | 0x10 | 普通 IO 控制 |
| 64-Way IO Control | 0x11 | 64 路 IO 控制 |
| Dip Swtich Control | 0x12 | 拨码开关控制 |
| Additional Test | 0x20 | 附加测试 |
| Enter Test Mode | 0x21 | 进入测试模式 |
| Heartbeat | 0x22 | 心跳查询 |


### General IO Control
#### Tooling -> Device
+ Payload

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Port ID | u8 | 端口号 |
| Pin Mask | u16 | 引脚掩码 |
| Value | u8 | 值 |


+ Sub-ID Description

| `Sub-ID` | 描述 | `Pin Mask` (u16) | `Value` (u8) 参数 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| `0x01` | **设置引脚模式** | 16位掩码，`1`代表选中该引脚 | `0x00`: 输入, `0x01`: (推挽)输出, `0x02`: 模拟 | 将掩码选中的所有引脚设为同一种模式(payload长5B) |
| `0x02` | **配置上下拉** | 16位掩码，`1`代表选中该引脚 | `0x00`: 下拉, `0x01`: 上拉, `0x02`: 无/浮空 | 将掩码选中的所有引脚设为同一种上下拉状态(payload长5B) |
| `0x03` | **写输出电平** | 16位掩码，`1`代表选中该引脚 | `0x00`: Low, `0x01`: High | 将掩码选中的所有输出引脚设为同一种电平(payload长5B) |
| `0x04` | **读输入电平** | (无) | (无) | 读取该端口下全部16个引脚的电平状态(payload长2B) |


#### Device -> Tooling
+ Sub-ID from 0x01 - 0x03

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Status | u8 | 状态 |


| `Status` 值 | 枚举名 | 描述 |
| :--- | :--- | :--- |
| `0x00` | `DEVICE_OK` | 指令成功执行 |
| `0x01` | `DEVICE_ERR_INVALID_PORT` | 无效的端口号 |
| `0x02` | `DEVICE_ERR_INVALID_PIN` | 无效的引脚掩码 |
| `0x03` | `DEVICE_ERR_INVALID_SUB_ID` | 无效的子命令ID |
| `0x04` | `DEVICE_ERR_EXECUTION` | 其他执行时错误 |


+ Sub-ID 0x04

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Port ID | u8 | 端口 ID |
| Levels | u16 | 16 pins level in 1 port |


### 64-Way IO Control
#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Pin Mask | u64 | 掩码 |
| Value | u8 | 值 |


| `Sub-ID` | 描述 | `Pin Mask` (u64/u8) | `Value` (u8) 参数 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| `0x01` | **设置引脚模式** | 引脚掩码，`1`代表选中该引脚 | `0x00`: 输入, `0x01`: (推挽)输出, `0x02`: 模拟 | 将掩码选中的所有引脚设为同一种模式 |
| `0x02` | **配置上下拉** | 引脚掩码，`1`代表选中该引脚 | `0x00`: 下拉, `0x01`: 上拉, `0x02`: 无/浮空 | 将掩码选中的所有引脚设为同一种上下拉状态 |
| `0x03` | **写输出电平** | 引脚掩码，`1`代表选中该引脚 | `0x00`: Low, `0x01`: High | 将掩码选中的所有输出引脚设为同一种电平 |
| `0x04` | **读输入电平** | (无) | (无) | 读取该端口下全部16个引脚的电平状态 |


#### Device -> Tooling
+ Sub-ID from 0x01 - 0x03

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Status | u8 | 状态 |


| `Status` 值 | 枚举名 | 描述 |
| :--- | :--- | :--- |
| `0x00` | `DEVICE_OK` | 指令成功执行 |
| `0x01` | `DEVICE_ERR_INVALID_PORT` | 无效的端口号 |
| `0x02` | `DEVICE_ERR_INVALID_PIN` | 无效的引脚掩码 |
| `0x03` | `DEVICE_ERR_INVALID_SUB_ID` | 无效的子命令ID |
| `0x04` | `DEVICE_ERR_EXECUTION` | 其他执行时错误 |


+ Sub-ID 0x04

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Status | u8 | 状态 |
| Levels | u64 | 电平值 |


### Dip Switch Control
#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Pin Mask | u8 | 掩码 |
| Value | u8 | 值 |


| `Sub-ID` | 描述 | `Pin Mask` (u64/u8) | `Value` (u8) 参数 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| `0x01` | **设置引脚模式** | 引脚掩码，`1`代表选中该引脚 | `0x00`: 输入, `0x01`: (推挽)输出, `0x02`: 模拟 | 将掩码选中的所有引脚设为同一种模式 |
| `0x02` | **配置上下拉** | 引脚掩码，`1`代表选中该引脚 | `0x00`: 下拉, `0x01`: 上拉, `0x02`: 无/浮空 | 将掩码选中的所有引脚设为同一种上下拉状态 |
| `0x03` | **写输出电平** | 引脚掩码，`1`代表选中该引脚 | `0x00`: Low, `0x01`: High | 将掩码选中的所有输出引脚设为同一种电平 |
| `0x04` | **读输入电平** | (无) | (无) | 读取该端口下全部16个引脚的电平状态 |


#### Device -> Tooling
+ Sub-ID from 0x01 - 0x03

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Status | u8 | 状态 |


| `Status` 值 | 枚举名 | 描述 |
| :--- | :--- | :--- |
| `0x00` | `DEVICE_OK` | 指令成功执行 |
| `0x01` | `DEVICE_ERR_INVALID_PORT` | 无效的端口号 |
| `0x02` | `DEVICE_ERR_INVALID_PIN` | 无效的引脚掩码 |
| `0x03` | `DEVICE_ERR_INVALID_SUB_ID` | 无效的子命令ID |
| `0x04` | `DEVICE_ERR_EXECUTION` | 其他执行时错误 |


+ Sub-ID 0x04

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 子 ID |
| Status | u8 | 状态 |
| Levels | u8 | 电平值 |


### Additional Test
| `Sub-ID` | 描述 |
| :--- | :--- |
| `0x01` | 串口回环自测 |
| `0x02` | 下发SN码 |
| `0x10` | 读取唯一ID |
| `0x11` | 读取SN码 |
| `0x12` | 读取固件版本 |


#### Tooling -> Device
+ Sub ID = 0x01

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |


+ Sub-ID = 0x02

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |
| SN Length | u8 | SN 长度 |
| SN Value | u8[ ] | SN 值 |


+ Sub ID = 0x10

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |


+ Sub ID = 0x11

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |


+ Sub ID = 0x12

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |


#### Device -> Tooling
+ Sub ID = 0x01

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x01 |
| Status | u8 | 状态 |


+ Sub-ID = 0x02

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x02 |
| Status | u8 | 状态 |


+ Sub ID = 0x10

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x10 |
| Status | u8 | 状态 |
| ID Length | u8 | ID 长度 |
| ID Value | u8[ ] | ID 值 |


+ Sub ID = 0x11

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x11 |
| SN Length | u8 | SN 长度 |
| SN Value | u8[ ] | SN 值 |


+ Sub ID = 0x12

| Payload | Type | Description |
| --- | --- | --- |
| Sub-ID | u8 | 0x12 |
| FW Length | u8 | 固件版本长度 |
| FW Value | u8[ ] | 固件版本值 |


### Enter Test Mode
#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| None |  | 无 |


#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| Status |  | `0x00`:成功<br/>`0x01`:失败 |


### Heartbeat
#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| None |  | 无 |


#### Tooling -> Device
| Payload | Type | Description |
| --- | --- | --- |
| Status |  | `0x00`:空闲<br/>`0x01`:忙碌<br/>`0xFF`: 错误 |


