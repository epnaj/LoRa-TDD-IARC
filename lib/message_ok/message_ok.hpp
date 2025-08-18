#pragma once

#include "base_message.hpp"

constexpr messageIdType MESSAGE_OK {1};

class OKMessage : public BaseMessage {
public:
    inline messageIdType messageId() const override {
        return MESSAGE_OK;
    }

    inline std::string name() const override {
        return "OKMessage";
    }

    inline std::vector <uint8_t> sendableBytes() const override {
        return {MESSAGE_OK};
    }

    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        
    }

private:
    // static constexpr uint8_t messageIdOK{0};
};