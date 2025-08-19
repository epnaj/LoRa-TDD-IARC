#pragma once

#include "base_message.hpp"

constexpr messageIdType MESSAGE_ACK {1};

class MessageACK : public BaseMessage {
public:
    MessageACK() = default;
    MessageACK(
        const addressType myAddress,
        const addressType ackDestination,
        const messageNoType ackMessageNumber
    ) {
        setSenderAddress(myAddress);
        setDestinationAddress(ackDestination);
        setMessageNo(ackMessageNumber);
    }

    inline messageIdType messageId() const override {
        return MESSAGE_ACK;
    }

    inline std::string name() const override {
        return "MessageACK";
    }

    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize, 0);
        encodeEnvelope(output);
        return std::move(output);
    }

    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        decodeEnvelope(bytes);
    }

private:
    // static constexpr uint8_t messageIdOK{0};
};