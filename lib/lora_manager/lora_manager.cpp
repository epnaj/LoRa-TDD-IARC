#include "lora_manager.hpp"

LoRaManager::LoRaManager(LoRaDevice &loraDevice):
    loraDevice(loraDevice), 
    isDownlink{true},
    runInternalThread{true}
{    
    internalThread = std::thread(
        &LoRaManager::threadLogic,
        *this
    );
}

LoRaManager::~LoRaManager() {
    if (runInternalThread.load()) {
        runInternalThread.store(false);
    }
    internalThread.join();
}

void LoRaManager::threadLogic() {
    while (runInternalThread.load()) {
        // this will be divided in next patches
        std::chrono::milliseconds timeLeftForUplinkSubFrame = sendNotAcked(notAckedQueue, FRAME_UL_DL_MS);
        timeLeftForUplinkSubFrame = sendNewMessages(toSendQueue, timeLeftForUplinkSubFrame);

        // switch to downlink to receive acks
        isDownlink.store(true);

        
    }
}


std::chrono::milliseconds LoRaManager::sendNotAcked(
    ThreadSafeQueue <std::pair<BaseMessage, messageUUID>> &queue,
    const std::chrono::milliseconds &timeToUse 
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        !isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > std::chrono::milliseconds(0)
    ) {
        auto &[message, uuid] = queue.front();
        loraDevice.send(
            message.sendableBytes(),
            timeLeftInFrame
        );
        
        queue.push(std::make_pair(message, uuid));
        queue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    return timeLeftInFrame;
}

std::chrono::milliseconds LoRaManager::sendNewMessages(
    ThreadSafeQueue <BaseMessage> &queue,
    const std::chrono::milliseconds &timeToUse 
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        !isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > std::chrono::milliseconds(0)
    ) {
        BaseMessage &message = queue.front();
        loraDevice.send(
            message.sendableBytes(),
            timeLeftInFrame
        );
        
        notAckedQueue.push(std::make_pair(message, makeUUID(message)));
        queue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    return timeLeftInFrame;
}

/*
This function should be executed within FRAME_WINDOW_MS
*/
void LoRaManager::sendThreadFunction() {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{FRAME_WINDOW_MS};
    
    // loraDevice.setTransmitMode();
    uint32_t expectedAckCount{0};
    // unordered map doesn't work here
    // std::map <
    //     std::pair <addressType, uint16_t>, bool
    // > acknowledgedMap;
    while (
        !isDownlink.load() && 
        !toSendQueue.empty() &&
        // std::chrono::duration_cast <std::chrono::milliseconds> (
        //     std::chrono::high_resolution_clock::now() - startTime
        // ) < FRAME_UL_DL_MS
        timeLeftInFrame > std::chrono::milliseconds(0)
    ) {
        // for (uint16_t i = 0; i < messages.size(); ++i) {
        BaseMessage &message = toSendQueue.front();
        // acknowledgedMap[std::make_pair(message.getDestinationAddress(), i)] = false;

        loraDevice.send(
            message.sendableBytes(), 
            // this strategy might require adjustments - 
            // we can either limit message send time or 
            // we can send them in order - if some are not sent, 
            // leave it for next time
            // FRAME_UL_DL_MS / messages.size()

            // we'll do just it ;)
            timeLeftInFrame
        );

        ++expectedAckCount;
        notAckedQueue.push(std::make_pair(message, makeUUID(message)));
        toSendQueue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;

    // loraDevice.setReceiveMode();
    for (const auto &messageBytes : loraDevice.tryReceive(FRAME_ACK_MS)) {
        // we should retrevie only ACKS here
        std::visit(overloaded {
                [ & ](MessageACK &message) -> void {
                    // acknowledgedMap[message.getSenderAddress()] = true;
                    --expectedAckCount;
                },
                [ ](auto &&message) -> void {
                    // ignore other messages
                    return;
                }
            },
            decodeFromBytes(messageBytes)
        );
    }

    // for (auto &[peerAddress, ack] : acknowledgedMap) {
    //     if (!ack) {
            
    //     }
    // }

    // return expectedAckCount ? SendResult::SENT_NO_ACK : SendResult::SENT_ACK;
}

/*
This function should be executed within FRAME_WINDOW_MS
*/
std::vector <MessageVariant> LoRaManager::receiveMessages() {

}