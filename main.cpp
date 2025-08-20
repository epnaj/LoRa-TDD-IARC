#include <iostream>
#include <atomic>

#include "lora_manager.hpp"
#include "thread_safe_queue.hpp"
#include "logger.hpp"

class MockLoRaDevice : public LoRaDevice {
public:
    void registerReceivedMessagesQueue(ThreadSafeQueue <std::vector <uint8_t>> &queue) override {
        receivedBytesQueue = &queue;
    }

    void unregisterReceivedMessagesQueue() override {
        if (receivedBytesQueue) {
            receivedBytesQueue = nullptr;
        }
    }

    bool send(
        const std::vector <uint8_t> &bytes, 
        const std::chrono::milliseconds timeoutMs
    ) {
        // send away
    }

    
private:
    ThreadSafeQueue <std::vector <uint8_t>> *receivedBytesQueue;
};

void mainLogic() {
    MockLoRaDevice mockLoRaDevice;
    LoRaManager loraManager(
        mockLoRaDevice,
        0
    );
    
}

int main() {
    // Disabling stream sync, as I'm using many of them and want them performant.
    std::cout.sync_with_stdio(false);
    Logger::startWithStdout("test_logs.log");
    Logger::registerMyThreadName("MAIN-THREAD");

    // THIS MUST BE KEPT AS SEPARATE SCOPE DUE TO RACE CONDITIONS
    mainLogic();

    Logger::stop();
    return 0;
}