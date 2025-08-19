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
    virtual bool send(
        const std::vector <uint8_t> &bytes, 
        const std::chrono::milliseconds timeoutMs
    ) = 0;
    virtual const std::vector <std::vector <uint8_t>> tryReceive(const std::chrono::milliseconds timeoutMs) = 0; 
};

enum class SendResult : uint8_t {
    ERROR,
    SENT_NO_ACK,
    SENT_ACK
};

class LoRaManager {
public:
    LoRaManager(LoRaDevice &loraDevice);
    ~LoRaManager();
    void addToSendQueue(const BaseMessage &message);
    MessageVariant receiveMessage();
    std::vector <MessageVariant> receiveMessages();

private:
    using messageUUID = uint16_t;

    static constexpr std::chrono::milliseconds FRAME_UL_DL_MS{100};
    static constexpr std::chrono::milliseconds FRAME_ACK_MS{50};
    static constexpr std::chrono::milliseconds FRAME_WINDOW_MS = FRAME_UL_DL_MS + FRAME_ACK_MS;

    std::atomic <bool> runInternalThread{false};
    std::thread internalThread;

    void threadLogic();

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
        ThreadSafeQueue <std::pair<BaseMessage, messageUUID>> &queue,
        const std::chrono::milliseconds &timeToUse
    );
    std::chrono::milliseconds LoRaManager::sendNewMessages(
        ThreadSafeQueue <BaseMessage> &queue,
        const std::chrono::milliseconds &timeToUse 
    );

    void sendThreadFunction();

    // if false it means it's uplink mode
    std::atomic <bool> isDownlink{true};
    ThreadSafeQueue <BaseMessage> toSendQueue{};
    ThreadSafeQueue <std::pair<BaseMessage, messageUUID>> notAckedQueue{};
    ThreadSafeQueue <MessageVariant> receivedQueue{};


    LoRaDevice &loraDevice;
};