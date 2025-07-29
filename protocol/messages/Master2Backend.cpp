#include "Master2Backend.h"

namespace WhtsProtocol {
namespace Master2Backend {

// SlaveConfigResponseMessage 实现
std::vector<uint8_t> SlaveConfigResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
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

bool SlaveConfigResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;

    status = data[0];
    slaveNum = data[1];
    slaves.clear();

    size_t offset = 2;
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

// ModeConfigResponseMessage 实现
std::vector<uint8_t> ModeConfigResponseMessage::serialize() const {
    return {status, mode};
}

bool ModeConfigResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    status = data[0];
    mode = data[1];
    return true;
}

// RstResponseMessage 实现
std::vector<uint8_t> RstResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(status);
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

bool RstResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;

    status = data[0];
    slaveNum = data[1];
    slaves.clear();

    size_t offset = 2;
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

// CtrlResponseMessage 实现
std::vector<uint8_t> CtrlResponseMessage::serialize() const {
    return {status, runningStatus};
}

bool CtrlResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    status = data[0];
    runningStatus = data[1];
    return true;
}

// PingResponseMessage 实现
std::vector<uint8_t> PingResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(pingMode);

    // Write total count (2 bytes, little endian)
    ByteUtils::writeUint16LE(result, totalCount);

    // Write success count (2 bytes, little endian)
    ByteUtils::writeUint16LE(result, successCount);

    // Write destination ID (4 bytes, little endian)
    ByteUtils::writeUint32LE(result, destinationId);

    return result;
}

bool PingResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 9)
        return false;

    pingMode = data[0];
    totalCount = ByteUtils::readUint16LE(data, 1);
    successCount = ByteUtils::readUint16LE(data, 3);
    destinationId = ByteUtils::readUint32LE(data, 5);

    return true;
}

// IntervalConfigResponseMessage 实现
std::vector<uint8_t> IntervalConfigResponseMessage::serialize() const {
    return {status, intervalMs};
}

bool IntervalConfigResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    status = data[0];
    intervalMs = data[1];
    return true;
}

// DeviceListResponseMessage 实现
std::vector<uint8_t> DeviceListResponseMessage::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(deviceCount);

    for (const auto &device : devices) {
        // Write device ID (4 bytes, little endian)
        ByteUtils::writeUint32LE(result, device.deviceId);
        result.push_back(device.shortId);
        result.push_back(device.online);
        result.push_back(device.versionMajor);
        result.push_back(device.versionMinor);
        // Write version patch (2 bytes, little endian)
        ByteUtils::writeUint16LE(result, device.versionPatch);
    }

    return result;
}

bool DeviceListResponseMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 1)
        return false;

    deviceCount = data[0];
    devices.clear();

    size_t offset = 1;
    for (uint8_t i = 0; i < deviceCount; ++i) {
        if (offset + 10 > data.size())
            return false; // Each device info is 10 bytes

        DeviceInfo device;
        device.deviceId = ByteUtils::readUint32LE(data, offset);
        device.shortId = data[offset + 4];
        device.online = data[offset + 5];
        device.versionMajor = data[offset + 6];
        device.versionMinor = data[offset + 7];
        device.versionPatch = ByteUtils::readUint16LE(data, offset + 8);

        devices.push_back(device);
        offset += 10;
    }

    return true;
}

} // namespace Master2Backend
} // namespace WhtsProtocol