#ifndef WHTS_PROTOCOL_BYTE_UTILS_H
#define WHTS_PROTOCOL_BYTE_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace WhtsProtocol {

// 字节序转换工具类
class ByteUtils {
  public:
    // 写入小端序数据
    static void writeUint16LE(std::vector<uint8_t> &buffer, uint16_t value);
    static void writeUint32LE(std::vector<uint8_t> &buffer, uint32_t value);

    // 读取小端序数据
    static uint16_t readUint16LE(const std::vector<uint8_t> &buffer,
                                 size_t offset);
    static uint32_t readUint32LE(const std::vector<uint8_t> &buffer,
                                 size_t offset);

    // // 字节数组转十六进制字符串
    // static std::string bytesToHexString(const std::vector<uint8_t> &data,
    //                                     size_t maxBytes = 16);
};

} // namespace WhtsProtocol

#endif // WHTS_PROTOCOL_BYTE_UTILS_H