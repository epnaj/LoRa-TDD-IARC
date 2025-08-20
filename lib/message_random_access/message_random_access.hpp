#pragma once

#include "base_message.hpp"

#include <chrono>

static constexpr messageIdType MESSAGE_RAND_ACCESS {3}; 

class MessageRandomAccess : public BaseMessage {
public:
    MessageRandomAccess() {
        setSenderAddress(broadcastAddress);
        setDestinationAddress(broadcastAddress);
    }
    
    inline messageIdType messageId() const override {
        return MESSAGE_RAND_ACCESS;
    }
    
    inline std::string name() const override {
        return "MessageRandomAccess";
    }
    
    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize, 0);
        encodeEnvelope(output);
        return std::move(output);
    }
    
    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        decodeEnvelope(bytes);
    }   
    static constexpr std::size_t EXPECTED_SIZE{BaseMessage::envelopeSize};
private:

};

static constexpr messageIdType MESSAGE_RAND_ACCESS_RESP {4}; 

class MessageRandomAccessResponse : public BaseMessage {
public:
    MessageRandomAccessResponse() = default;
    MessageRandomAccessResponse(
        addressType senderAddress,
        uint8_t numberOfNetworkUsers
        // uint8_t whoseTurnItIsNow
    ) {
        setDestinationAddress(broadcastAddress);
        setSenderAddress(senderAddress);
        setNumberOfNetworkUsers(numberOfNetworkUsers);
        // setWhoseTurnItIsNow(whoseTurnItIsNow);
    }
    
    inline messageIdType messageId() const override {
        return MESSAGE_RAND_ACCESS_RESP;
    }
    
    inline std::string name() const override {
        return "MessageRandomAccessResponse";
    }
    
    inline std::vector <uint8_t> sendableBytes() const override {
        std::vector <uint8_t> output(BaseMessage::envelopeSize, 0);
        encodeEnvelope(output);
        return std::move(output);
    }
    
    inline void decodeFromBytes(const std::vector <uint8_t> &bytes) override {
        decodeEnvelope(bytes);
    }  
    
    void setNumberOfNetworkUsers(const uint8_t numberOfNetworkUsers) {
        this->numberOfNetworkUsers;
    }

    uint8_t getNumberOfNetworkUsers() const {
        return numberOfNetworkUsers;
    }

    // void setWhoseTurnItIsNow(const uint8_t whoseTurnItIsNow) {
    //     this->whoseTurnItIsNow = whoseTurnItIsNow;
    // }

    // uint8_t getWhoseTurnItIsNow() const {
    //     return whoseTurnItIsNow;
    // }

    static constexpr std::size_t EXPECTED_SIZE{
        BaseMessage::envelopeSize +
        sizeof(uint8_t)
    };
private:
    uint8_t numberOfNetworkUsers{0};
    // uint8_t whoseTurnItIsNow{0};
};

