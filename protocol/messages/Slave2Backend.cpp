#include "Slave2Backend.h"

namespace WhtsProtocol {
namespace Slave2Backend {

// ConductionDataMessage 实现
std::vector<uint8_t> ConductionDataMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(conductionLength & 0xFF);
    result.push_back((conductionLength >> 8) & 0xFF);
    result.insert(result.end(), conductionData.begin(), conductionData.end());
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ConductionDataMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    conductionLength = data[0] | (data[1] << 8);
    if (data.size() < 2 + conductionLength)
        return false;
    conductionData.assign(data.begin() + 2,
                          data.begin() + 2 + conductionLength);
    return true;
}

// ResistanceDataMessage 实现
std::vector<uint8_t> ResistanceDataMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(resistanceLength & 0xFF);
    result.push_back((resistanceLength >> 8) & 0xFF);
    result.insert(result.end(), resistanceData.begin(), resistanceData.end());
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ResistanceDataMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    resistanceLength = data[0] | (data[1] << 8);
    if (data.size() < 2 + resistanceLength)
        return false;
    resistanceData.assign(data.begin() + 2,
                          data.begin() + 2 + resistanceLength);
    return true;
}

// ClipDataMessage 实现
std::vector<uint8_t> ClipDataMessage::serialize() const {
    auto& result = getReusableVector();
    result.push_back(clipData & 0xFF);
    result.push_back((clipData >> 8) & 0xFF);
    return result; // 返回副本，可复用的 vector 会在下次调用时被清空
}

bool ClipDataMessage::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2)
        return false;
    clipData = data[0] | (data[1] << 8);
    return true;
}

} // namespace Slave2Backend
} // namespace WhtsProtocol