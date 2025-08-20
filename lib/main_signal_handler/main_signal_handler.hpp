#pragma once

#include <atomic>

#include "lora_manager.hpp"
#include "common_message.hpp"

class MainSignalHandler {
    public:
    MainSignalHandler(LoRaManager &loraManager);
    void startLoop();
    // void stopLoop();

private:
    LoRaManager &loraManager;
    std::atomic <bool> shouldRun{false};
};