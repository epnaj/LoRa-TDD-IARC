#pragma once

#include <cstdint>

#include "manager.hpp"

class LoRaStrategy : public ManagerStrategyBase {
public:
    void send(const BaseMessage &message);
    void listenAndReceive(ThreadSafeQueue <MessageVariant> &receivedMessagesQueue);
    void setTransmitMode();
    void setReceiveMode();

private:
    static constexpr uint32_t FRAME_UL_DL_MS{80};
    static constexpr uint32_t FRAME_ACK_MS{20};
    static constexpr uint32_t FRAME_WINDOW_MS = FRAME_UL_DL_MS + FRAME_ACK_MS;
    
};