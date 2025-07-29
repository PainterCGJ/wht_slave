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
};

} // namespace WhtsProtocol

#endif // WHTS_PROTOCOL_MESSAGE_H