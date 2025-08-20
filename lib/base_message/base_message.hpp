#pragma once

#include <cstdint>
#include <vector>
#include <string>

using messageIdType = uint8_t;
using addressType   = uint8_t;
using messageNoType = uint8_t;  
static constexpr addressType invalidAddress{255}; // == 255
static constexpr addressType broadcastAddress{254};

/*
The memory layout should be
| TO | FROM | MESSAGE ID | MESSAGE NO | DATA | 
   1    1         1            1          X    = 4 + X bytes
At least 4 bytes should be used in current setting.
The address and message id encoding might change in the future.
*/
class BaseMessage {
public:
    inline void setDestinationAddress(const addressType destinationAddress) {
        this->destinationAddress = destinationAddress;
    }

    inline void setSenderAddress(const addressType senderAddress) {
        this->senderAddress = senderAddress;
    }

    inline void setMessageNo(const messageNoType messageNumber) {
        this->messageNumber = messageNumber;
    }

    inline addressType getDestinationAddress() const {
        return destinationAddress;
    }
    
    inline addressType getSenderAddress() const {
        return senderAddress;
    }
    
    inline messageNoType getMessageNo() const {
        return messageNumber;
    }

    virtual messageIdType messageId() const = 0;
    virtual std::string name() const = 0;

    virtual std::vector <uint8_t> sendableBytes() const = 0;
    virtual void decodeFromBytes(const std::vector <uint8_t> &bytes) = 0;
    static constexpr auto addressingSize{sizeof(addressType) * 2};
    static constexpr auto envelopeSize{sizeof(addressType) * 2 + sizeof(messageIdType) + sizeof(messageNoType)};

protected:
    addressType destinationAddress{invalidAddress};
    addressType senderAddress{invalidAddress};
    messageNoType messageNumber{0};

    std::vector <uint8_t> &encodeAddress(std::vector <uint8_t> &data) const {
        if (data.size() < BaseMessage::addressingSize) {
            return data;
        }
        *(reinterpret_cast <addressType *> (&data[0]))                   = destinationAddress;
        *(reinterpret_cast <addressType *> (&data[sizeof(addressType)])) = senderAddress;

        return data;
    }

    void decodeAddresses(const std::vector <uint8_t> &data) {
        if (data.size() < BaseMessage::addressingSize) {
            return;
        }
        destinationAddress = *(reinterpret_cast <const addressType *> (&data[0]));
        senderAddress      = *(reinterpret_cast <const addressType *> (&data[sizeof(addressType)]));
    }

    void decodeMessageNumber(const std::vector <uint8_t> &data) {
        if (data.size() < BaseMessage::envelopeSize) {
            return;
        }
        messageNumber = *(reinterpret_cast <const messageNoType *> (
            &data[BaseMessage::addressingSize + sizeof(messageIdType)])
        );
    }

    void decodeEnvelope(const std::vector <uint8_t> &data) {
        if (data.size() < BaseMessage::envelopeSize) {
            return;
        }
        decodeAddresses(data);
        decodeMessageNumber(data);
    }

    std::vector <uint8_t> &encodeEnvelope(std::vector <uint8_t> &data) const {
        if (data.size() < BaseMessage::envelopeSize) {
            return data;
        }
        encodeAddress(data);
        *(reinterpret_cast <messageIdType *> (&data[addressingSize])) = messageId();
        *(reinterpret_cast <messageNoType *> (&data[addressingSize + sizeof(messageIdType)])) = getMessageNo();
        return data;
    }
};

static constexpr messageIdType MESSAGE_BAD {0};

class BadMessage : public BaseMessage {
    public:
    inline messageIdType messageId() const override {
        return 0;
    }
    
    inline std::string name() const override {
        return "BadMessage";
    }
    
    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize, 0);
        encodeEnvelope(output);
        return std::move(output);
    }
    
    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {}
    
    static constexpr std::size_t EXPECTED_SIZE {0};
};