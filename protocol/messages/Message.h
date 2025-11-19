#ifndef WHTS_PROTOCOL_MESSAGE_H
#define WHTS_PROTOCOL_MESSAGE_H

#include <cstdint>
#include <vector>
#include <string>

namespace WhtsProtocol {

// 基础消息类
class Message {
  public:
    virtual ~Message() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool deserialize(const std::vector<uint8_t> &data) = 0;
    virtual uint8_t getMessageId() const = 0;
    virtual const char* getMessageTypeName() const = 0;

  protected:
    // 获取可复用的 vector，用于序列化以减少内存碎片
    // 注意：此方法不是线程安全的，适用于单线程环境
    static std::vector<uint8_t>& getReusableVector() {
        static std::vector<uint8_t> reusableVector;
        reusableVector.clear();
        // 只在第一次或容量不足时才重新分配
        if (reusableVector.capacity() < 256) {
            reusableVector.reserve(256); // 预分配常用大小，减少重新分配
        }
        return reusableVector;
    }
};

} // namespace WhtsProtocol

#endif // WHTS_PROTOCOL_MESSAGE_H