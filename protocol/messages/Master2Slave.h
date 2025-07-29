#ifndef WHTS_PROTOCOL_MASTER2SLAVE_H
#define WHTS_PROTOCOL_MASTER2SLAVE_H

#include "../Common.h"
#include "Message.h"

namespace WhtsProtocol {
namespace Master2Slave {

// 为运行模式新增枚举，代码更清晰
enum class SlaveRunMode : uint8_t {
    CONDUCTION_TEST = 0,    // 导通检测
    RESISTANCE_TEST = 1,    // 阻值检测
    CLIP_TEST = 2           // 卡钉检测
};

class SyncMessage : public Message {
   public:
    uint64_t timestamp;    // 时间戳（us）

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::SYNC_MSG);
    }
    const char* getMessageTypeName() const override { return "Sync"; }
};

class SetTimeMessage : public Message {
   public:
    uint64_t timestamp;    // 时间戳（us）

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::SET_TIME_MSG);
    }
    const char* getMessageTypeName() const override { return "Set Time"; }
};

class ConductionConfigMessage : public Message {
   public:
    uint8_t timeSlot;
    uint8_t interval;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;
    uint16_t conductionNum;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::CONDUCTION_CFG_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Conduction Config";
    }
};

class ResistanceConfigMessage : public Message {
   public:
    uint8_t timeSlot;
    uint8_t interval;
    uint16_t totalNum;
    uint16_t startNum;
    uint16_t num;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::RESISTANCE_CFG_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Resistance Config";
    }
};

class ClipConfigMessage : public Message {
   public:
    uint8_t interval;
    uint8_t mode;
    uint16_t clipPin;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::CLIP_CFG_MSG);
    }
    const char* getMessageTypeName() const override { return "Clip Config"; }
};

class RstMessage : public Message {
   public:
    uint8_t lockStatus;
    uint16_t clipLed;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::RST_MSG);
    }
    const char* getMessageTypeName() const override { return "Reset"; }
};

class PingReqMessage : public Message {
   public:
    uint16_t sequenceNumber;
    uint32_t timestamp;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::PING_REQ_MSG);
    }
    const char* getMessageTypeName() const override { return "Ping Request"; }
};

class ShortIdAssignMessage : public Message {
   public:
    uint8_t shortId;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::SHORT_ID_ASSIGN_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Short ID Assign";
    }
};

class SlaveControlMessage : public Message {
   public:
    SlaveRunMode mode;    // 运行模式
    uint8_t enable;       // 1：启动, 0：停止
    uint64_t startTime;   // 启动时间戳（微秒），用于同步启动

    // 必须实现的虚函数
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::SLAVE_CONTROL_MSG);
    }
    const char* getMessageTypeName() const override { return "Slave Control"; }
};

}    // namespace Master2Slave
}    // namespace WhtsProtocol

#endif    // WHTS_PROTOCOL_MASTER2SLAVE_H