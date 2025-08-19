#pragma once

#include <variant>
#include <optional>
#include <iostream>

#include "logger.hpp"
#include "base_message.hpp"
#include "message_ack.hpp"
#include "message_address.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using MessageVariant = std::variant <
    BadMessage,
    MessageACK,
    MessageAddress
>;

inline MessageVariant decodeFromBytes(const std::vector <uint8_t> &data) {
    static constexpr auto TAG{"DECODE FROM BYTES"};
    Logger::debug(TAG, "DECODE FROM BYTES");
    // if message doesn't contain these fields it means message is faulty
    if (data.size() < BaseMessage::envelopeSize) {
        return BadMessage();
    }

    MessageVariant messageVariant = BadMessage();
    // switch (*(reinterpret_cast <const messageIdType *> (data.data() + 2 * sizeof(addressType))))
    switch (*(reinterpret_cast <const messageIdType *> (data[BaseMessage::addressingSize])))
    {
    case MESSAGE_ACK:
        messageVariant = MessageACK();
        break;

    case MESSAGE_ADDRESS:
        messageVariant = MessageAddress();
        break;
        
    default:
        Logger::error(TAG, "COULDN'T DECODE MESSAGE, RETURNING BAD MESSAGE!");
        break;
    }

    // we visit here in order to setup proper memory layout
    // it could be done later, but user would have to remember about it :/
    std::visit(
        [&data](auto &&variant){
            Logger::debug(TAG, "Decoding: " + variant.name());
            variant.decodeFromBytes(data);
        },
        messageVariant
    );
    return messageVariant;
}