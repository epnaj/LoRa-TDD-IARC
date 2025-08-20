#include "lora_manager.hpp"

LoRaManager::LoRaManager(LoRaDevice &loraDevice, const addressType myAddress):
    loraDevice(loraDevice), 
    isDownlink{true},
    runInternalThread{true},
    myAddress{myAddress}
{    
    loraDevice.registerReceivedMessagesQueue(receivedBytesQueue);
    internalThread = std::thread(
        &LoRaManager::threadLogic,
        this
    );
}

LoRaManager::~LoRaManager() {
    loraDevice.unregisterReceivedMessagesQueue();
    if (runInternalThread.load()) {
        runInternalThread.store(false);
    }
    internalThread.join();
}

void LoRaManager::randomAccess(std::atomic <bool> &run) {
    // random access procedure
    static constexpr auto TAG{"RANDOM-ACCESS"};
    Logger::debug(TAG, "Start of random access");
    uint8_t retries{0}, maxRetries{3};
    while (run.load() && retries < maxRetries) {
        MessageRandomAccess randomAccessRequest;
        randomAccessRequest.setMessageNo(sentMessageCounter++);
        
        isDownlink.store(false);
        Logger::debug(TAG, "Sending RAR directly");
        // addToSendQueue(randomAccessRequest);

        loraDevice.send(
            randomAccessRequest.sendableBytes(),
            FRAME_UL_DL_MS
        );

        isDownlink.store(true);
        // have some offset in case of perfect synch
        std::this_thread::sleep_for(
            FRAME_UL_DL_MS + (FRAME_UL_DL_MS * 0.33 * retries)
        );
        MessageVariant messageVariant = receiveMessage();
        std::visit(
            overloaded {
                [&](MessageRandomAccessResponse &messageRAR) ->void {
                    totalNumberOfDevicesInNetwork = messageRAR.getNumberOfNetworkUsers();
                    myAddress = totalNumberOfDevicesInNetwork;
                    Logger::debug(TAG, "RECEIVED MSG RAR!");
                },
                [&](auto &message) -> void {
                    // ignore, wait only for random access response
                    return;
                }
            },
            messageVariant
        );
        ++retries;
    }

    imFirstNode.store(retries == maxRetries);
    if (imFirstNode.load()) {
        myAddress                     = 0;
        frameCounter                  = 0;
        sentMessageCounter            = 0;
        totalNumberOfDevicesInNetwork = 1;
    } else {
        bool foundSynch{false};
        while (!foundSynch) {
            MessageVariant messageVariant = receiveMessage();
            std::visit(
                overloaded {
                    [&](BadMessage &badMessage) -> void {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    },
                    [&](auto &message) ->void {
                        // ignore other messages
                        return;
                    },
                    [&](MessageSynchronise &messageSynch) -> void {
                        // update synch data here

                        frameCounter = 0;
                        sentMessageCounter = 0;
                        foundSynch = true;
                    }
                },
                messageVariant
            );
        }
    }
    Logger::debug(TAG, "END OF RAC DATA DUMP: My address: " + 
        std::to_string(myAddress) +
        " NO dev in net: " + std::to_string(totalNumberOfDevicesInNetwork) + 
        " AM I FIRST? " + std::to_string(imFirstNode.load())
    );
}

void LoRaManager::threadLogic() {
    Logger::registerMyThreadName("T-LORA-" + std::to_string(myAddress) + "-INT");

    while (runInternalThread.load()) {
        if (internalThreadDelay > ZERO_MS) {
            std::this_thread::sleep_for(internalThreadDelay);
            internalThreadDelay = ZERO_MS;
        }
        
        std::chrono::milliseconds timeLeftForFrame = FRAME_UL_DL_MS;
        if (totalNumberOfDevicesInNetwork > 1 && (frameCounter % totalNumberOfDevicesInNetwork) == myAddress) {
            Logger::debug("THREAD-LOGIC", "Uplink mode!");
            // this is uplink frame procedure
            isDownlink.store(false);
            if (imFirstNode.load()) {
                auto startSync = std::chrono::high_resolution_clock::now();
                // talk directly to the driver
                loraDevice.send(
                    MessageSynchronise(myAddress, sentMessageCounter++).sendableBytes(),
                    FRAME_UL_DL_MS
                );
                auto endSync = std::chrono::high_resolution_clock::now();
                timeLeftForFrame = FRAME_UL_DL_MS - (
                    std::chrono::duration_cast <std::chrono::milliseconds> (
                        endSync - startSync
                    )
                );
            }
            
            timeLeftForFrame = sendNotAcked(notAckedQueue, timeLeftForFrame);
            timeLeftForFrame = sendNewMessages(toSendQueue, timeLeftForFrame);
            
            if (timeLeftForFrame > ZERO_MS) {
                std::this_thread::sleep_for(timeLeftForFrame);
                timeLeftForFrame = ZERO_MS;
            }
            
            // switch to downlink to receive acks
            isDownlink.store(true);
            timeLeftForFrame += FRAME_ACK_MS;
            timeLeftForFrame = receiveMessages(receivedBytesQueue, timeLeftForFrame);
            
            if (timeLeftForFrame > ZERO_MS) {
                std::this_thread::sleep_for(timeLeftForFrame);
                timeLeftForFrame = ZERO_MS;
            }
        } else {
            Logger::debug("THREAD-LOGIC", "Downlink mode!");
            // this is downlink frame procedure
            isDownlink.store(true);
            timeLeftForFrame = receiveMessages(receivedBytesQueue, timeLeftForFrame);
            if (timeLeftForFrame > ZERO_MS) {
                std::this_thread::sleep_for(timeLeftForFrame);
                timeLeftForFrame = ZERO_MS;
            }

            // switch to uplink to send acks 
            isDownlink.store(false);
            timeLeftForFrame += FRAME_ACK_MS;
            timeLeftForFrame = sendAck(toAckQueue, timeLeftForFrame);

            if (timeLeftForFrame > ZERO_MS) {
                std::this_thread::sleep_for(timeLeftForFrame);
                timeLeftForFrame = ZERO_MS;
            }
        }        
    }
}


std::chrono::milliseconds LoRaManager::sendNotAcked(
    ThreadSafeQueue <std::tuple<MessageVariant, messageUUID, timeToLive>> &queue,
    const std::chrono::milliseconds &timeToUse 
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        !isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > ZERO_MS
    ) {
        auto &[message, uuid, ttl] = queue.front();
        std::visit(
            [&] (auto &msg) {
                loraDevice.send(
                    msg.sendableBytes(),
                    timeLeftInFrame
                );
            },
            message
        );
        
        
        if (--ttl > 0) {
            queue.push(std::make_tuple(message, uuid, ttl));
        }

        queue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    return timeLeftInFrame;
}

std::chrono::milliseconds LoRaManager::sendNewMessages(
    ThreadSafeQueue <MessageVariant> &queue,
    const std::chrono::milliseconds &timeToUse 
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        !isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > ZERO_MS
    ) {
        MessageVariant &message = queue.front();

        std::visit(
            [&](auto &msg) {
                msg.setMessageNo(sentMessageCounter++);
                loraDevice.send(
                    msg.sendableBytes(),
                    timeLeftInFrame
                );
            },
            message
        );
        
        notAckedQueue.push(
            std::make_tuple(message, makeUUID(message), DEFAULT_TTL)
        );

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
std::chrono::milliseconds LoRaManager::receiveMessages(
    ThreadSafeQueue <std::vector <uint8_t>> &queue,
    const std::chrono::milliseconds &timeToUse
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > ZERO_MS
    ) {
        MessageVariant messageVariant = decodeFromBytes(queue.front());
        
        receivedQueue.push(messageVariant);

        std::visit(
            overloaded {
                [](BadMessage &messageBad) -> void {
                    // ignore this message
                    return;
                },
                [](MessageACK &messageAck) -> void {
                    // do not ack the ack message
                    Logger::debug(
                        "LoRaManager::RECEIVE-MSG", 
                        "Acked from: " + std::to_string(messageAck.getSenderAddress()) + " message NO: " + std::to_string(messageAck.getMessageNo())
                    );
                    return;
                },
                [&](MessageRandomAccess &messageRandAccess) -> void {
                    // send MessageRandom Access Confirmation
                    MessageRandomAccessResponse msgRAR(
                        getMyAddress(),
                        getTotalNumberOfDevicesInNetwork()
                    );
                    msgRAR.setDestinationAddress(broadcastAddress);
                    if (totalNumberOfDevicesInNetwork <= 1) {
                        // edge case talk to driver
                        Logger::debug("LoRaManager::RECEIVE-MSG", "received RAC, talking to the driver");
                        loraDevice.send(msgRAR.sendableBytes(), timeLeftInFrame);
                    } else {
                        addToSendQueue(msgRAR);
                    }
                    ++totalNumberOfDevicesInNetwork;
                    return;
                },
                [ & ](auto &message) -> void {
                    Logger::debug("LoRaManager::RECEIVE-MSG", "MESSAGE TO ACK: " + message.name());
                    toAckQueue.push(
                        MessageACK(
                            myAddress,
                            message.getSenderAddress(),
                            message.getMessageNo()
                        )
                    );
                }
            },
            messageVariant
        );

        queue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    return timeLeftInFrame;
}

std::chrono::milliseconds LoRaManager::sendAck(
    ThreadSafeQueue <MessageACK> &queue,
    const std::chrono::milliseconds &timeToUse
) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds timeLeftInFrame{timeToUse};

    while (
        !isDownlink.load() && 
        !queue.empty() &&
        timeLeftInFrame > ZERO_MS
    ) {
        MessageACK &messageAck = queue.front();
        loraDevice.send(
            messageAck.sendableBytes(),
            timeLeftInFrame
        );
        
        queue.pop();

        timeLeftInFrame = FRAME_WINDOW_MS - std::chrono::duration_cast <std::chrono::milliseconds> (
            std::chrono::high_resolution_clock::now() - startTime
        );
    }

    return timeLeftInFrame;
}
