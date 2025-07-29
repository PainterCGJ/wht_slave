#ifndef WHTS_PROTOCOL_SLAVE2BACKEND_H
#define WHTS_PROTOCOL_SLAVE2BACKEND_H

#include "../Common.h"
#include "Message.h"

namespace WhtsProtocol {
namespace Slave2Backend {

class ConductionDataMessage : public Message {
  public:
    uint16_t conductionLength;
    std::vector<uint8_t> conductionData;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2BackendMessageId::CONDUCTION_DATA_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Conduction Data";
    }
};

class ResistanceDataMessage : public Message {
  public:
    uint16_t resistanceLength;
    std::vector<uint8_t> resistanceData;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(
            Slave2BackendMessageId::RESISTANCE_DATA_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Resistance Data";
    }
};

class ClipDataMessage : public Message {
  public:
    uint16_t clipData;

    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t> &data) override;
    uint8_t getMessageId() const override {
        return static_cast<uint8_t>(Slave2BackendMessageId::CLIP_DATA_MSG);
    }
    const char* getMessageTypeName() const override {
        return "Clip Data";
    }
};

} // namespace Slave2Backend
} // namespace WhtsProtocol

#endif // WHTS_PROTOCOL_SLAVE2BACKEND_H