#pragma once

#include <cstdint>
#include <vector>
#include <string>

using messageIdType = uint8_t;

class BaseMessage {
public:
    virtual messageIdType messageId() const = 0;
    virtual std::string name() const = 0;

    virtual std::vector <uint8_t> sendableBytes() const = 0;
    virtual void decodeFromBytes(const std::vector <uint8_t> &bytes) = 0;
};

constexpr messageIdType MESSAGE_BAD {0};

class BadMessage : public BaseMessage {
public:
    messageIdType messageId() const override {
        return 0;
    }

    std::string name() const override {
        return "BadMessage";
    }
    std::vector <uint8_t> sendableBytes() const override {
        return { MESSAGE_BAD };
    }

    void decodeFromBytes(const std::vector <uint8_t> &bytes) override {}
};