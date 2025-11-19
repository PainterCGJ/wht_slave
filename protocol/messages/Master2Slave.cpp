#include "Master2Slave.h"

namespace WhtsProtocol {
namespace Master2Slave {

// SyncMessage 实现
std::vector<uint8_t> SyncMessage::serialize() const {
    auto& result = getReusableVector();
    
    // 序列化模式（1字节）
    result.push_back(mode);
    
    // 序列化间隔（1字节）
    result.push_back(interval);
    
    // 序列化当前时间戳（8字节，小端序）
    result.push_back(currentTime & 0xFF);
    result.push_back((currentTime >> 8) & 0xFF);
    result.push_back((currentTime >> 16) & 0xFF);
    result.push_back((currentTime >> 24) & 0xFF);
    result.push_back((currentTime >> 32) & 0xFF);
    result.push_back((currentTime >> 40) & 0xFF);
    result.push_back((currentTime >> 48) & 0xFF);
    result.push_back((currentTime >> 56) & 0xFF);
    
    // 序列化启动时间戳（8字节，小端序）
    result.push_back(startTime & 0xFF);
    result.push_back((startTime >> 8) & 0xFF);
    result.push_back((startTime >> 16) & 0xFF);
    result.push_back((startTime >> 24) & 0xFF);
    result.push_back((startTime >> 32) & 0xFF);
    result.push_back((startTime >> 40) & 0xFF);
    result.push_back((startTime >> 48) & 0xFF);
    result.push_back((startTime >> 56) & 0xFF);
    
    // 序列化从机配置
    for (const auto& config : slaveConfigs) {
        // 从机ID（4字节，小端序）
        result.push_back(config.slaveId & 0xFF);
        result.push_back((config.slaveId >> 8) & 0xFF);
        result.push_back((config.slaveId >> 16) & 0xFF);
        result.push_back((config.slaveId >> 24) & 0xFF);
        
        // 时隙（1字节）
        result.push_back(config.timeSlot);
        
        // 复位标志（1字节）
        result.push_back(config.reset);
        
        // 检测数量（1字节）
        result.push_back(config.testCount);
    }
    
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool SyncMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 18) return false; // 最小长度：1+1+8+8=18字节
    
    size_t offset = 0;
    
    // 反序列化模式
    mode = data[offset++];
    
    // 反序列化间隔
    interval = data[offset++];
    
    // 反序列化当前时间戳（8字节，小端序）
    currentTime = static_cast<uint64_t>(data[offset]) |
                  (static_cast<uint64_t>(data[offset + 1]) << 8) |
                  (static_cast<uint64_t>(data[offset + 2]) << 16) |
                  (static_cast<uint64_t>(data[offset + 3]) << 24) |
                  (static_cast<uint64_t>(data[offset + 4]) << 32) |
                  (static_cast<uint64_t>(data[offset + 5]) << 40) |
                  (static_cast<uint64_t>(data[offset + 6]) << 48) |
                  (static_cast<uint64_t>(data[offset + 7]) << 56);
    offset += 8;
    
    // 反序列化启动时间戳（8字节，小端序）
    startTime = static_cast<uint64_t>(data[offset]) |
                (static_cast<uint64_t>(data[offset + 1]) << 8) |
                (static_cast<uint64_t>(data[offset + 2]) << 16) |
                (static_cast<uint64_t>(data[offset + 3]) << 24) |
                (static_cast<uint64_t>(data[offset + 4]) << 32) |
                (static_cast<uint64_t>(data[offset + 5]) << 40) |
                (static_cast<uint64_t>(data[offset + 6]) << 48) |
                (static_cast<uint64_t>(data[offset + 7]) << 56);
    offset += 8;
    
    // 反序列化从机配置（每个从机配置7字节：4字节ID + 1字节时隙 + 1字节复位标志 + 1字节检测数量）
    slaveConfigs.clear();
    while (offset + 7 <= data.size()) {
        SlaveConfig config;
        
        // 从机ID（4字节，小端序）
        config.slaveId = static_cast<uint32_t>(data[offset]) |
                         (static_cast<uint32_t>(data[offset + 1]) << 8) |
                         (static_cast<uint32_t>(data[offset + 2]) << 16) |
                         (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        
        // 时隙
        config.timeSlot = data[offset++];
        
        // 复位标志
        config.reset = data[offset++];
        
        // 检测数量
        config.testCount = data[offset++];
        
        slaveConfigs.push_back(config);
    }
    
    return true;
}






// PingReqMessage 实现
std::vector<uint8_t> PingReqMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(sequenceNumber & 0xFF);
    result.push_back((sequenceNumber >> 8) & 0xFF);
    result.push_back(timestamp & 0xFF);
    result.push_back((timestamp >> 8) & 0xFF);
    result.push_back((timestamp >> 16) & 0xFF);
    result.push_back((timestamp >> 24) & 0xFF);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool PingReqMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 6) return false;
    sequenceNumber = data[0] | (data[1] << 8);
    timestamp = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
    return true;
}

// ShortIdAssignMessage 实现
std::vector<uint8_t> ShortIdAssignMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(shortId);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ShortIdAssignMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1) return false;
    shortId = data[0];
    return true;
}


}    // namespace Master2Slave
}    // namespace WhtsProtocol