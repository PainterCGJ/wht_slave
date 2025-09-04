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

// 从机配置信息结构体
struct SlaveConfig {
    uint32_t slaveId;       // 4字节从机ID
    uint8_t timeSlot;       // 分配的时隙
    uint8_t reset;          // 复位标志：0-默认值，1-执行复位
    uint8_t testCount;      // 检测数量（导通/阻值/卡钉数量）
};

class SyncMessage : public Message {
   public:
    uint8_t mode;               // 采集模式：0-导通检测，1-阻值检测，2-卡钉检测
    uint8_t interval;           // 采集间隔（ms）
    uint64_t currentTime;       // 当前时间戳（微秒）
    uint64_t startTime;         // 启动时间戳（微秒）
    std::vector<SlaveConfig> slaveConfigs;  // 所有从机配置

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Master2SlaveMessageId::SYNC_MSG);
    }
    const char* getMessageTypeName() const override { return "Sync"; }
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


}    // namespace Master2Slave
}    // namespace WhtsProtocol

#endif    // WHTS_PROTOCOL_MASTER2SLAVE_H