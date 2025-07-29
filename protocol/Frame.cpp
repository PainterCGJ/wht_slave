#include "Frame.h"
#include "utils/ByteUtils.h"

namespace WhtsProtocol {

Frame::Frame()
    : delimiter1(FRAME_DELIMITER_1), delimiter2(FRAME_DELIMITER_2), packetId(0),
      fragmentsSequence(0), moreFragmentsFlag(0), packetLength(0) {}

bool Frame::isValid() const {
    return delimiter1 == FRAME_DELIMITER_1 && delimiter2 == FRAME_DELIMITER_2;
}

std::vector<uint8_t> Frame::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(7 + payload.size());

    result.push_back(delimiter1);
    result.push_back(delimiter2);
    result.push_back(packetId);
    result.push_back(fragmentsSequence);
    result.push_back(moreFragmentsFlag);

    // 小端序写入长度
    result.push_back(packetLength & 0xFF);
    result.push_back((packetLength >> 8) & 0xFF);

    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool Frame::deserialize(const std::vector<uint8_t> &data, Frame &frame) {
    if (data.size() < 7)
        return false;

    frame.delimiter1 = data[0];
    frame.delimiter2 = data[1];
    frame.packetId = data[2];
    frame.fragmentsSequence = data[3];
    frame.moreFragmentsFlag = data[4];

    // 小端序读取长度
    frame.packetLength = data[5] | (data[6] << 8);

    if (data.size() < 7 + frame.packetLength)
        return false;

    frame.payload.assign(data.begin() + 7,
                         data.begin() + 7 + frame.packetLength);
    return frame.isValid();
}

} // namespace WhtsProtocol