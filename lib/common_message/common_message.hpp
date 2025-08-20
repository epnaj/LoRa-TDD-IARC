#pragma once

#include <variant>
#include <optional>
#include <iostream>

#include "logger.hpp"
#include "base_message.hpp"
#include "message_ack.hpp"
#include "message_random_access.hpp"
#include "message_mine.hpp"
#include "message_sync.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using MessageVariant = std::variant <
    BadMessage,
    MessageACK,
    MessageMine,
    MessageRandomAccess,
    MessageRandomAccessResponse,
    MessageSynchronise
>;

inline MessageVariant decodeFromBytes(const std::vector <uint8_t> &data) {
    static constexpr auto TAG{"DECODE-FROM-BYTES"};
    Logger::debug(TAG, "DECODE FROM BYTES START");

    // here we override and exit if message is killswitch message!

    // if message doesn't contain these fields it means message is faulty
    if (data.size() < BaseMessage::envelopeSize) {
        return BadMessage();
    }

    MessageVariant messageVariant = BadMessage();
    // switch (*(reinterpret_cast <const messageIdType *> (data.data() + 2 * sizeof(addressType))))
    switch (*(reinterpret_cast <const messageIdType *> (&data[BaseMessage::addressingSize])))
    {
    case MESSAGE_ACK:
        if (data.size() >= MessageACK::EXPECTED_SIZE) {
            messageVariant = MessageACK();
        }
        break;
    case MESSAGE_MINE:
        if (data.size() >= MessageMine::EXPECTED_SIZE) {
            messageVariant = MessageMine();
        }
        break;
        
    case MESSAGE_RAND_ACCESS:
        if (data.size() >= MessageRandomAccess::EXPECTED_SIZE) {
            messageVariant = MessageRandomAccess();
        }
        break;
    
    case MESSAGE_RAND_ACCESS_RESP:
        if (data.size() >= MessageRandomAccessResponse::EXPECTED_SIZE) {
            messageVariant = MessageRandomAccessResponse();
        }
        break;
    
    case MESSAGE_SYNCH:
        if (data.size() >= MessageSynchronise::EXPECTED_SIZE) {
            // this message should be handled here or in LoRaManager
            messageVariant = MessageSynchronise();
        }
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