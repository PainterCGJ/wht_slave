#ifndef WHTS_PROTOCOL_BACKEND2MASTER_H
#define WHTS_PROTOCOL_BACKEND2MASTER_H

#include "../Common.h"
#include "../utils/ByteUtils.h"
#include "Message.h"

namespace WhtsProtocol {
namespace Backend2Master {

class SlaveConfigMessage : public Message {
   public:
    struct SlaveInfo {
        uint32_t id;
        uint8_t conductionNum;
        uint8_t resistanceNum;
        uint8_t clipMode;
        uint16_t clipStatus;
    };

    uint8_t slaveNum;
    std::vector<SlaveInfo> slaves;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::SLAVE_CFG_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Slave Config";
    }
};

class ModeConfigMessage : public Message {
   public:
    uint8_t mode;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::MODE_CFG_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Mode Config";
    }
};

class RstMessage : public Message {
   public:
    struct SlaveRstInfo {
        uint32_t id;
        uint8_t lock;
        uint16_t clipStatus;
    };

    uint8_t slaveNum;
    std::vector<SlaveRstInfo> slaves;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::SLAVE_RST_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Reset";
    }
};

class CtrlMessage : public Message {
   public:
    uint8_t runningStatus;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::CTRL_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Control";
    }
};

class PingCtrlMessage : public Message {
   public:
    uint8_t pingMode;
    uint16_t pingCount;
    uint16_t interval;
    uint32_t destinationId;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::PING_CTRL_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Ping Control";
    }
};

class IntervalConfigMessage : public Message {
   public:
    uint8_t intervalMs;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Backend2MasterMessageId::INTERVAL_CFG_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Interval Config";
    }
};

class DeviceListReqMessage : public Message {
   public:
    uint8_t reserve;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Backend2MasterMessageId::DEVICE_LIST_REQ_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Device List Request";
    }
};

}    // namespace Backend2Master
}    // namespace WhtsProtocol

#endif    // WHTS_PROTOCOL_BACKEND2MASTER_H