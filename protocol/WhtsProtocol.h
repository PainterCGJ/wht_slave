#ifndef WHTS_PROTOCOL_H
#define WHTS_PROTOCOL_H

// 包含所有子模块
#include "Common.h"
#include "DeviceStatus.h"
#include "Frame.h"
#include "ProtocolProcessor.h"

// 消息模块
#include "messages/Backend2Master.h"
#include "messages/Master2Backend.h"
#include "messages/Master2Slave.h"
#include "messages/Message.h"
#include "messages/Slave2Backend.h"
#include "messages/Slave2Master.h"


// 工具模块
#include "utils/ByteUtils.h"

// 标准库依赖
#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace WhtsProtocol {
}

#endif // WHTS_PROTOCOL_H