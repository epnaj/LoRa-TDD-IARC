#pragma once

#include <cstdint>
#include <map>
#include <atomic>
#include <chrono>

#include "common_message.hpp"
#include "thread_safe_queue.hpp"

class LoRaDevice {
public:
    // virtual void setTransmitMode() = 0;
    // virtual void setReceiveMode() = 0;
    virtual void registerReceivedMessagesQueue(
        ThreadSafeQueue <std::vector <uint8_t>> &queue 
    ) = 0;

    virtual void unregisterReceivedMessagesQueue() = 0;

    virtual bool send(
        const std::vector <uint8_t> &bytes, 
        const std::chrono::milliseconds timeoutMs
    ) = 0;
    // virtual const std::vector <std::vector <uint8_t>> tryReceive(const std::chrono::milliseconds timeoutMs) = 0; 
};

class LoRaManager {
public:
    LoRaManager(LoRaDevice &loraDevice, const addressType myAddress);
    ~LoRaManager();
    inline void addToSendQueue(const MessageVariant &message) {
        std::visit(
            [&](auto &msg) {
                Logger::debug("ADD-TO-SEND-Q", "Message " + msg.name() + " added to send queue.");
            },
            message
        );
        toSendQueue.push(message);
    }

    inline MessageVariant receiveMessage() {
        if (!receivedQueue.empty()) {
            MessageVariant &message = receivedQueue.front();
            receivedQueue.pop();
            return std::move(message);
        }
        return BadMessage();
    }

    inline addressType getMyAddress() const {
        return myAddress;
    }

    inline uint8_t getTotalNumberOfDevicesInNetwork() {
        return totalNumberOfDevicesInNetwork;
    }

    inline uint8_t getFrameCounter() {
        return frameCounter;
    }

    void randomAccess(std::atomic <bool> &run);

private:
    using messageUUID = uint16_t;
    using timeToLive  = uint8_t;

    static constexpr std::chrono::milliseconds FRAME_UL_DL_MS{100};
    static constexpr std::chrono::milliseconds FRAME_ACK_MS{50};
    static constexpr std::chrono::milliseconds FRAME_WINDOW_MS = FRAME_UL_DL_MS + FRAME_ACK_MS;

    static constexpr timeToLive DEFAULT_TTL{5};

    std::atomic <bool> runInternalThread{false};
    std::thread internalThread;
    std::chrono::milliseconds internalThreadDelay{0};
    static constexpr std::chrono::milliseconds ZERO_MS{0};
    std::atomic <bool> imFirstNode{false};

    void threadLogic();
    
    inline messageUUID makeUUID(const MessageVariant &messageVariant) {
        return std::visit(
            [ & ](const auto &message) -> messageUUID {
                return makeUUID(
                    message.getSenderAddress(),
                    message.getMessageNo()
                );
            },
            messageVariant
        );
    }

    inline messageUUID makeUUID(const BaseMessage &message) {
        return makeUUID(message.getSenderAddress(), message.getMessageNo());
    }

    inline messageUUID makeUUID(const addressType senderAddress, const messageNoType messageNumber) {
        return static_cast <messageUUID> (
            ((senderAddress & 0xFF) << 8) + (messageNumber & 0xFF)
        );
    }

    // returns time left for uplink frame
    std::chrono::milliseconds sendNotAcked(
        ThreadSafeQueue <std::tuple<MessageVariant, messageUUID, timeToLive>> &queue,
        const std::chrono::milliseconds &timeToUse
    );

    std::chrono::milliseconds sendNewMessages(
        ThreadSafeQueue <MessageVariant> &queue,
        const std::chrono::milliseconds &timeToUse 
    );

    std::chrono::milliseconds receiveMessages(
        ThreadSafeQueue <std::vector <uint8_t>> &queue,
        const std::chrono::milliseconds &timeToUse
    );

    std::chrono::milliseconds sendAck(
        ThreadSafeQueue <MessageACK> &queue,
        const std::chrono::milliseconds &timeToUse
    );

    void sendThreadFunction();

    // if false it means it's uplink mode
    std::atomic <bool> isDownlink{true};
    ThreadSafeQueue <MessageVariant> toSendQueue{};
    ThreadSafeQueue <std::tuple<MessageVariant, messageUUID, timeToLive>> notAckedQueue{};
    
    ThreadSafeQueue <std::vector <uint8_t>> receivedBytesQueue{};
    ThreadSafeQueue <MessageVariant> receivedQueue{};
    ThreadSafeQueue <MessageACK> toAckQueue{};

    LoRaDevice &loraDevice;
    addressType myAddress{0};
    addressType totalNumberOfDevicesInNetwork{1};
    addressType frameCounter{0};
    messageNoType sentMessageCounter{0};
};