#include "Backend2Master.h"

namespace WhtsProtocol {
namespace Backend2Master {

// SlaveConfigMessage 实现
std::vector<uint8_t> SlaveConfigMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(slaveNum);

    for (const auto &slave : slaves) {
        // Write slave ID (4 bytes, little endian)
        ByteUtils::writeUint32LE(result, slave.id);
        result.push_back(slave.conductionNum);
        result.push_back(slave.resistanceNum);
        result.push_back(slave.clipMode);
        // Write clip status (2 bytes, little endian)
        ByteUtils::writeUint16LE(result, slave.clipStatus);
    }

    return result;
}

bool SlaveConfigMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;

    slaveNum = data[0];
    slaves.clear();

    size_t offset = 1;
    for (uint8_t i = 0; i < slaveNum; ++i) {
        if (offset + 9 > data.size())
            return false; // Each slave info is 9 bytes

        SlaveInfo slave;
        slave.id = ByteUtils::readUint32LE(data, offset);
        slave.conductionNum = data[offset + 4];
        slave.resistanceNum = data[offset + 5];
        slave.clipMode = data[offset + 6];
        slave.clipStatus = ByteUtils::readUint16LE(data, offset + 7);

        slaves.push_back(slave);
        offset += 9;
    }

    return true;
}

// ModeConfigMessage 实现
std::vector<uint8_t> ModeConfigMessage::serialize() const { return {mode}; }

bool ModeConfigMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;
    mode = data[0];
    return true;
}

// RstMessage 实现
std::vector<uint8_t> RstMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(slaveNum);

    for (const auto &slave : slaves) {
        // Write slave ID (4 bytes, little endian)
        ByteUtils::writeUint32LE(result, slave.id);
        result.push_back(slave.lock);
        // Write clip status (2 bytes, little endian)
        ByteUtils::writeUint16LE(result, slave.clipStatus);
    }

    return result;
}

bool RstMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;

    slaveNum = data[0];
    slaves.clear();

    size_t offset = 1;
    for (uint8_t i = 0; i < slaveNum; ++i) {
        if (offset + 7 > data.size())
            return false; // Each slave rst info is 7 bytes

        SlaveRstInfo slave;
        slave.id = ByteUtils::readUint32LE(data, offset);
        slave.lock = data[offset + 4];
        slave.clipStatus = ByteUtils::readUint16LE(data, offset + 5);

        slaves.push_back(slave);
        offset += 7;
    }

    return true;
}

// CtrlMessage 实现
std::vector<uint8_t> CtrlMessage::serialize() const { return {runningStatus}; }

bool CtrlMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;
    runningStatus = data[0];
    return true;
}

// PingCtrlMessage 实现
std::vector<uint8_t> PingCtrlMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(pingMode);

    // Write ping count (2 bytes, little endian)
    ByteUtils::writeUint16LE(result, pingCount);

    // Write interval (2 bytes, little endian)
    ByteUtils::writeUint16LE(result, interval);

    // Write destination ID (4 bytes, little endian)
    ByteUtils::writeUint32LE(result, destinationId);

    return result;
}

bool PingCtrlMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 9)
        return false;

    pingMode = data[0];
    pingCount = ByteUtils::readUint16LE(data, 1);
    interval = ByteUtils::readUint16LE(data, 3);
    destinationId = ByteUtils::readUint32LE(data, 5);

    return true;
}

// IntervalConfigMessage 实现
std::vector<uint8_t> IntervalConfigMessage::serialize() const {
    return {intervalMs};
}

bool IntervalConfigMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;
    intervalMs = data[0];
    return true;
}

// DeviceListReqMessage 实现
std::vector<uint8_t> DeviceListReqMessage::serialize() const {
    return {reserve};
}

bool DeviceListReqMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;
    reserve = data[0];
    return true;
}

} // namespace Backend2Master
} // namespace WhtsProtocol