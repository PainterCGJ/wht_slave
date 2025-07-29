#ifndef WHTS_PROTOCOL_FRAME_H
#define WHTS_PROTOCOL_FRAME_H

#include "Common.h"
#include <cstdint>
#include <vector>


namespace WhtsProtocol {

// 帧结构
struct Frame {
    uint8_t delimiter1;
    uint8_t delimiter2;
    uint8_t packetId;
    uint8_t fragmentsSequence;
    uint8_t moreFragmentsFlag;
    uint16_t packetLength;
    std::vector<uint8_t> payload;

    Frame();
    bool isValid() const;
    std::vector<uint8_t> serialize() const;
    static bool deserialize(const std::vector<uint8_t> &data, Frame &frame);
};

} // namespace WhtsProtocol

#endif // WHTS_PROTOCOL_FRAME_H