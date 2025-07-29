#include "ByteUtils.h"
#include <iomanip>
#include <sstream>

namespace WhtsProtocol {

void ByteUtils::writeUint16LE(std::vector<uint8_t> &buffer, uint16_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
}

void ByteUtils::writeUint32LE(std::vector<uint8_t> &buffer, uint32_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

uint16_t ByteUtils::readUint16LE(const std::vector<uint8_t> &buffer,
                                 size_t offset) {
    if (offset + 1 >= buffer.size())
        return 0;
    return buffer[offset] | (buffer[offset + 1] << 8);
}

uint32_t ByteUtils::readUint32LE(const std::vector<uint8_t> &buffer,
                                 size_t offset) {
    if (offset + 3 >= buffer.size())
        return 0;
    return buffer[offset] | (buffer[offset + 1] << 8) |
           (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
}

// std::string ByteUtils::bytesToHexString(const std::vector<uint8_t> &data,
//                                         size_t maxBytes) {
//     std::stringstream ss;
//     size_t count = std::min(data.size(), maxBytes);
//     for (size_t i = 0; i < count; ++i) {
//         ss << std::hex << std::setw(2) << std::setfill('0')
//            << static_cast<int>(data[i]);
//         if (i < count - 1)
//             ss << " ";
//     }
//     if (data.size() > maxBytes) {
//         ss << "...";
//     }
//     return ss.str();
// }

} // namespace WhtsProtocol