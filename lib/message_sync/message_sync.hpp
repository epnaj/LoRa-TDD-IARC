#pragma once

#include "base_message.hpp"

static constexpr messageIdType MESSAGE_SYNCH {5};

class MessageSynchronise : public BaseMessage {
public:
    MessageSynchronise() = default;
    
    MessageSynchronise(addressType myAddress, messageNoType messageNo) {
        setSenderAddress(myAddress);
        setDestinationAddress(broadcastAddress);
        setMessageNo(messageNo);
    };

    inline messageIdType messageId() const override {
        return MESSAGE_SYNCH;
    }

    inline std::string name() const override {
        return "MessageSynchronise";
    }
    
    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize, 0);
        encodeEnvelope(output);
        return std::move(output);
    }

    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        decodeEnvelope(bytes);
    }

    static constexpr std::size_t EXPECTED_SIZE {BaseMessage::envelopeSize};
private:
    // static constexpr uint8_t messageIdOK{0};
};