#include "manager.hpp"

Manager::Manager(ManagerStrategyBase &managerStrategy):
    managerStrategy(managerStrategy) {
        managerStrategy.listenAndReceive(std::ref(receivedMessagesQueue));
}

SendMessageResponse Manager::sendMessage(const BaseMessage &message) {
    managerStrategy.send(message);
    return SendMessageResponse::SENT;
}

MessageVariant Manager::receiveMessage() {
    MessageVariant message = BadMessage();
    bool readyToReturn     = false;

    while (!readyToReturn)
    {
        if (!receivedMessagesQueue.empty()) {
            message       = receivedMessagesQueue.front();
            readyToReturn = std::visit([](auto &messageVariant) -> messageIdType {
                    return messageVariant.messageId();
                },
                message
            );
            Logger::debug(__func__, "ENTERED NOT EMPTY QUEUE, READY? " + std::to_string(readyToReturn));
            receivedMessagesQueue.pop();
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(queueBreakTimeMs));
    }
    return message;
}