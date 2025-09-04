#include "Slave2Master.h"

namespace WhtsProtocol {
namespace Slave2Master {

// SetTimeResponseMessage 实现
std::vector<uint8_t> SetTimeResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
    result.push_back(timestamp & 0xFF);
    result.push_back((timestamp >> 8) & 0xFF);
    result.push_back((timestamp >> 16) & 0xFF);
    result.push_back((timestamp >> 24) & 0xFF);
    result.push_back((timestamp >> 32) & 0xFF);
    result.push_back((timestamp >> 40) & 0xFF);
    result.push_back((timestamp >> 48) & 0xFF);
    result.push_back((timestamp >> 56) & 0xFF);
    return result;
}

bool SetTimeResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 9) return false;
    status = data[0];
    timestamp = static_cast<uint64_t>(data[1]) |
                (static_cast<uint64_t>(data[2]) << 8) |
                (static_cast<uint64_t>(data[3]) << 16) |
                (static_cast<uint64_t>(data[4]) << 24) |
                (static_cast<uint64_t>(data[5]) << 32) |
                (static_cast<uint64_t>(data[6]) << 40) |
                (static_cast<uint64_t>(data[7]) << 48) |
                (static_cast<uint64_t>(data[8]) << 56);
    return true;
}

// ConductionConfigResponseMessage 实现
std::vector<uint8_t> ConductionConfigResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
    result.push_back(timeSlot);
    result.push_back(interval);
    result.push_back(totalConductionNum & 0xFF);
    result.push_back((totalConductionNum >> 8) & 0xFF);
    result.push_back(startConductionNum & 0xFF);
    result.push_back((startConductionNum >> 8) & 0xFF);
    result.push_back(conductionNum & 0xFF);
    result.push_back((conductionNum >> 8) & 0xFF);
    return result;
}

bool ConductionConfigResponseMessage::deserialize(
    const std::vector<uint8_t> &data) {
    if (data.size() < 9) return false;
    status = data[0];
    timeSlot = data[1];
    interval = data[2];
    totalConductionNum = data[3] | (data[4] << 8);
    startConductionNum = data[5] | (data[6] << 8);
    conductionNum = data[7] | (data[8] << 8);
    return true;
}

// ResistanceConfigResponseMessage 实现
std::vector<uint8_t> ResistanceConfigResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
    result.push_back(timeSlot);
    result.push_back(interval);
    result.push_back(totalConductionNum & 0xFF);
    result.push_back((totalConductionNum >> 8) & 0xFF);
    result.push_back(startConductionNum & 0xFF);
    result.push_back((startConductionNum >> 8) & 0xFF);
    result.push_back(conductionNum & 0xFF);
    result.push_back((conductionNum >> 8) & 0xFF);
    return result;
}

bool ResistanceConfigResponseMessage::deserialize(
    const std::vector<uint8_t> &data) {
    if (data.size() < 9) return false;
    status = data[0];
    timeSlot = data[1];
    interval = data[2];
    totalConductionNum = data[3] | (data[4] << 8);
    startConductionNum = data[5] | (data[6] << 8);
    conductionNum = data[7] | (data[8] << 8);
    return true;
}

// ClipConfigResponseMessage 实现
std::vector<uint8_t> ClipConfigResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
    result.push_back(interval);
    result.push_back(mode);
    result.push_back(clipPin & 0xFF);
    result.push_back((clipPin >> 8) & 0xFF);
    return result;
}

bool ClipConfigResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 5) return false;
    status = data[0];
    interval = data[1];
    mode = data[2];
    clipPin = data[3] | (data[4] << 8);
    return true;
}

// RstResponseMessage 实现
std::vector<uint8_t> RstResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
    return result;
}

bool RstResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1) return false;
    status = data[0];
    return true;
}

// PingRspMessage 实现
std::vector<uint8_t> PingRspMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(sequenceNumber & 0xFF);
    result.push_back((sequenceNumber >> 8) & 0xFF);
    result.push_back(timestamp & 0xFF);
    result.push_back((timestamp >> 8) & 0xFF);
    result.push_back((timestamp >> 16) & 0xFF);
    result.push_back((timestamp >> 24) & 0xFF);
    return result;
}

bool PingRspMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 6) return false;
    sequenceNumber = data[0] | (data[1] << 8);
    timestamp = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
    return true;
}

// AnnounceMessage 实现
std::vector<uint8_t> AnnounceMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(deviceId & 0xFF);
    result.push_back((deviceId >> 8) & 0xFF);
    result.push_back((deviceId >> 16) & 0xFF);
    result.push_back((deviceId >> 24) & 0xFF);
    result.push_back(versionMajor);
    result.push_back(versionMinor);
    result.push_back(versionPatch & 0xFF);
    result.push_back((versionPatch >> 8) & 0xFF);
    return result;
}

bool AnnounceMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 8) return false;
    deviceId = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    versionMajor = data[4];
    versionMinor = data[5];
    versionPatch = data[6] | (data[7] << 8);
    return true;
}

// ShortIdConfirmMessage 实现
std::vector<uint8_t> ShortIdConfirmMessage::serialize() const {
    return {status, shortId};
}

bool ShortIdConfirmMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2) return false;
    status = data[0];
    shortId = data[1];
    return true;
}

// SlaveControlResponseMessage 实现
std::vector<uint8_t> SlaveControlResponseMessage::serialize() const {
    return {static_cast<uint8_t>(status)};
}

bool SlaveControlResponseMessage::deserialize(
    const std::vector<uint8_t> &data) {
    if (data.size() < 1) return false;
    status = static_cast<ResponseStatusCode>(data[0]);
    return true;
}

}    // namespace Slave2Master
}    // namespace WhtsProtocol