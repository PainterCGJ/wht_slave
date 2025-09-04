#ifndef WHTS_PROTOCOL_SLAVE2MASTER_H
#define WHTS_PROTOCOL_SLAVE2MASTER_H

#include "../Common.h"
#include "Message.h"

namespace WhtsProtocol {
namespace Slave2Master {

class SetTimeResponseMessage : public Message {
   public:
    uint8_t status;
    uint64_t timestamp;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2MasterMessageId::SET_TIME_RSP_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Set Time Response";
    }
};

class ConductionConfigResponseMessage : public Message {
   public:
    uint8_t status;
    uint8_t timeSlot;
    uint8_t interval;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;
    uint16_t conductionNum;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2MasterMessageId::CONDUCTION_CFG_RSP_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Conduction Config Response";
    }
};

class ResistanceConfigResponseMessage : public Message {
   public:
    uint8_t status;
    uint8_t timeSlot;
    uint8_t interval;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;
    uint16_t conductionNum;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2MasterMessageId::RESISTANCE_CFG_RSP_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Resistance Config Response";
    }
};

class ClipConfigResponseMessage : public Message {
   public:
    uint8_t status;
    uint8_t interval;
    uint8_t mode;
    uint16_t clipPin;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2MasterMessageId::CLIP_CFG_RSP_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Clip Config Response";
    }
};

class RstResponseMessage : public Message {
   public:
    uint8_t status;         // 0：复位成功，1：复位异常

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2MasterMessageId::RST_RSP_MSG);
    }
    const char* getMessageTypeName() const override { return "Reset Response"; }
};

class PingRspMessage : public Message {
   public:
    uint16_t sequenceNumber;
    uint32_t timestamp;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2MasterMessageId::PING_RSP_MSG);
    }
    const char* getMessageTypeName() const override { return "Ping Response"; }
};

class AnnounceMessage : public Message {
   public:
    uint32_t deviceId;
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint16_t versionPatch;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2MasterMessageId::ANNOUNCE_MSG);
    }
    const char* getMessageTypeName() const override { return "Announce"; }
};

class ShortIdConfirmMessage : public Message {
   public:
    uint8_t status;
    uint8_t shortId;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2MasterMessageId::SHORT_ID_CONFIRM_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Short ID Confirm";
    }
};

// 为响应消息新增状态码枚举
enum class ResponseStatusCode : uint8_t {
    SUCCESS = 0,
    FAILURE = 1,
    UNSUPPORTED_MODE = 2
};

// 新增 SlaveControlResponseMessage 类
class SlaveControlResponseMessage : public Message {
   public:
    ResponseStatusCode status;    // 响应状态码

    // 必须实现的虚函数
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2MasterMessageId::SLAVE_CONTROL_RSP_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Slave Control Response";
    }
};

}    // namespace Slave2Master
}    // namespace WhtsProtocol

#endif    // WHTS_PROTOCOL_SLAVE2MASTER_H