#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <thread>

#include "thread_safe_queue.hpp"
#include "common_message.hpp"
#include "logger.hpp"

enum class SendMessageResponse : uint8_t {
    SENT,                // Message sent, didn't wait for response
    ERROR_WHILE_SENDING, // Message not sent - some error occured while sending
    SENT_NOT_OK,         // Message sent, but received not OK
    SENT_OK              // Message sent, received OK back
};

/*
This is base for every Manager's strategy - it must implement 
- send  
- listenAndReceive  
LISTEN AND RECEIVE MUST NOT BE BLOCKING
this is the only class that should interact with outside world.
*/
class ManagerStrategyBase {
public:
    virtual void send(const BaseMessage &message) = 0;
    virtual void listenAndReceive(ThreadSafeQueue <MessageVariant> &receivedMessagesQueue) = 0;
};

class Manager {
public:
    Manager(ManagerStrategyBase &managerStrategy);

    SendMessageResponse sendMessage(const BaseMessage &message);
    MessageVariant receiveMessage();

private:
    static constexpr uint8_t queueBreakTimeMs{1};

    ThreadSafeQueue <MessageVariant> receivedMessagesQueue;
    ManagerStrategyBase &managerStrategy;
};
