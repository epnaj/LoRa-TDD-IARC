#pragma once

#include "base_message.hpp"

static constexpr messageIdType MESSAGE_MINE{2};

class MessageMine : public BaseMessage {
public:
    MessageMine() = default;
    MessageMine(const float latitude, const float longtitude){
        this->lattitude  = latitude;
        this->longtitude = longtitude;
    }
    
    inline messageIdType messageId() const override {
        return MESSAGE_MINE;
    }
    
    inline std::string name() const override {
        return "MessageMine";
    }
    
    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize + coordsSize, 0);
        encodeEnvelope(output);
        *(reinterpret_cast <float *> (&output[BaseMessage::envelopeSize])) = lattitude;
        *(reinterpret_cast <float *> (&output[BaseMessage::envelopeSize + sizeof(float)])) = longtitude;
        return std::move(output);
    }
    
    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        decodeEnvelope(bytes);
        lattitude  = *(reinterpret_cast <const float *> (&bytes[BaseMessage::envelopeSize]));
        longtitude = *(reinterpret_cast <const float *> (&bytes[BaseMessage::envelopeSize + sizeof(float)]));
    }

    private:
    float lattitude{0};
    float longtitude{0};
    static constexpr auto coordsSize{sizeof(float) * 2};
    
public:
    static constexpr std::size_t EXPECTED_SIZE {BaseMessage::envelopeSize + MessageMine::coordsSize};
};