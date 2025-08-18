#pragma once

#include <variant>
#include <optional>
#include <iostream>

#include "logger.hpp"
#include "base_message.hpp"
#include "message_ok.hpp"
#include "message_address.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using MessageVariant = std::variant <
    BadMessage,
    OKMessage,
    MessageAddress
>;

inline MessageVariant decodeFromBytes(const std::vector <uint8_t> &data) {
    Logger::debug(__func__, "DECODE FROM BYTES");
    if (!data.size()) {
        return BadMessage();
    }
    
    MessageVariant messageVariant = BadMessage();
    switch (*(reinterpret_cast <const messageIdType *> (data.data())))
    {
    case MESSAGE_OK:
        messageVariant = OKMessage();
        break;

    case MESSAGE_ADDRESS:
        messageVariant = MessageAddress();
        break;
        
    default:
        Logger::error(__func__, "COULDN'T DECODE MESSAGE, RETURNING BAD MESSAGE!");
        break;
    }
    std::visit(
        [&data](auto &&variant){
            Logger::debug(__func__, variant.name());
            variant.decodeFromBytes(data);
        },
        messageVariant
    );
    return messageVariant;
}