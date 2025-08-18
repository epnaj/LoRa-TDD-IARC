#pragma once

#include <array>

#include "base_message.hpp"

constexpr messageIdType MESSAGE_ADDRESS{2};

static constexpr uint8_t addressSize{8};

class MessageAddress : public BaseMessage {
public:
    inline messageIdType messageId() const override {
        return MESSAGE_ADDRESS;
    }

    inline std::string name() const override {
        return "MessageAddress";
    }

    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> buffer(sizeof(messageIdType) + addressSize, 0);
        *(reinterpret_cast <messageIdType *> (buffer.data())) = MESSAGE_ADDRESS;
        std::copy(
            std::begin(address),
            std::end(address),
            std::next(std::begin(buffer), sizeof(messageIdType))
        );
        return std::move(buffer);
    }

    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        std::copy(
            std::next(std::begin(bytes), sizeof(messageIdType)),
            std::end(bytes),
            std::begin(address)
        );
    }

    inline std::array <uint8_t, addressSize> &getArrayRef() {
        return address;
    }

private:
    std::array <uint8_t, addressSize> address;
};