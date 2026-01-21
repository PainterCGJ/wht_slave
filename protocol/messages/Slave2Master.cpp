#include "Slave2Master.h"

namespace WhtsProtocol {
namespace Slave2Master {





// RstResponseMessage 实现
std::vector<uint8_t> RstResponseMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(status);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool RstResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1) return false;
    status = data[0];
    return true;
}

// PingRspMessage 实现
std::vector<uint8_t> PingRspMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(sequenceNumber & 0xFF);
    result.push_back((sequenceNumber >> 8) & 0xFF);
    result.push_back(timestamp & 0xFF);
    result.push_back((timestamp >> 8) & 0xFF);
    result.push_back((timestamp >> 16) & 0xFF);
    result.push_back((timestamp >> 24) & 0xFF);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool PingRspMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 6) return false;
    sequenceNumber = data[0] | (data[1] << 8);
    timestamp = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
    return true;
}

// JoinRequestMessage 实现
std::vector<uint8_t> JoinRequestMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(deviceId & 0xFF);
    result.push_back((deviceId >> 8) & 0xFF);
    result.push_back((deviceId >> 16) & 0xFF);
    result.push_back((deviceId >> 24) & 0xFF);
    result.push_back(versionMajor);
    result.push_back(versionMinor);
    result.push_back(versionPatch & 0xFF);
    result.push_back((versionPatch >> 8) & 0xFF);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool JoinRequestMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 8) return false;
    deviceId = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    versionMajor = data[4];
    versionMinor = data[5];
    versionPatch = data[6] | (data[7] << 8);
    return true;
}

// ShortIdConfirmMessage 实现
std::vector<uint8_t> ShortIdConfirmMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(status);
    result.push_back(shortId);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ShortIdConfirmMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2) return false;
    status = data[0];
    shortId = data[1];
    return true;
}

// HeartbeatMessage 实现
std::vector<uint8_t> HeartbeatMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(batteryLevel);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool HeartbeatMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1) return false;
    batteryLevel = data[0];
    return true;
}

// ConductionDataMessage 实现
std::vector<uint8_t> ConductionDataMessage::serialize() const {
    auto& result = getReusableVector();
    // 直接序列化导通数据，不需要长度字段（长度从包长度推算）
    result.insert(result.end(), conductionData.begin(), conductionData.end());
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ConductionDataMessage::deserialize(const std::vector<uint8_t> &data) {
    // 直接反序列化所有数据为导通数据
    conductionData = data;
    return true;
}


}    // namespace Slave2Master
}    // namespace WhtsProtocol