#include <iostream>
#include <atomic>
#include <random>

#include "lora_manager.hpp"
#include "thread_safe_queue.hpp"
#include "logger.hpp"

class MockLoRaDevice;

// realy ugly
std::vector <MockLoRaDevice *> devices;
void registerInAir(MockLoRaDevice &device);
void sendToOthers(const std::vector <uint8_t> &bytes, MockLoRaDevice &devRef);

// struct Air {
//     std::vector <MockLoRaDevice &> devices;
    
//     void registerInAir(MockLoRaDevice &device) {
//         devices.push_back(device);
//     }
    
//     void sendToOthers(const std::vector <uint8_t> &bytes, MockLoRaDevice &devRef) {
//         for (MockLoRaDevice &device : devices) {
//             if (&device != &devRef) {
//                 device.receive(bytes);
//             }
//         }
//     }
// };

class MockLoRaDevice : public LoRaDevice {
    public:
    // MockLoRaDevice(Air &air): air(air) {}
    
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
        Logger::debug("MOCK-LORA-DEV", "Sending some data");
        // 10% so the packet is not properly sent
        if (!dist(gen)) {
            return false;
        }
        randomSmallSleep(bytes.size());
        Logger::debug("MOCK-LORA-DEV", "Sending some data almost successfull!");
        sendToOthers(bytes, *this);
        return true;
    }
    
    void receive(const std::vector <uint8_t> &data) {
        receivedBytesQueue->push(data);
    }
    
    
private:
    // Air &air;
    
    ThreadSafeQueue <std::vector <uint8_t>> *receivedBytesQueue;
    
    std::random_device rd{};
    std::mt19937_64 gen{rd()};
    std::uniform_int_distribution <int> dist{0, 10};
    
    void randomSmallSleep(const std::size_t &numberOfBytes) {
        // assuming we have a speed of at least 10 bytes per ms (~kbps)
        std::this_thread::sleep_for(
            std::chrono::milliseconds(numberOfBytes / 10 + dist(gen))
        );
    }
};

void registerInAir(MockLoRaDevice &device) {
    devices.push_back(&device);
}

void sendToOthers(const std::vector <uint8_t> &bytes, MockLoRaDevice &devRef) {
    for (MockLoRaDevice *devicePointer : devices) {
        if (devicePointer != &devRef) {
            Logger::debug("OTA", "DATA SENT!");
            devicePointer->receive(bytes);
        }
    }
}

std::atomic <bool> runSimulation{true};

void mainLogic() {
    
    uint8_t loraNo{0};
    std::vector <std::thread> simulatedDevices(2);
    Logger::debug("MAIN-LOGIC", "Main logic started.");

    for (auto &simDev : simulatedDevices) {
        simDev = std::thread([&](uint8_t threadNumber){
            std::random_device rd{};
            std::mt19937_64 gen{rd()};
            std::uniform_int_distribution <int> dist{0, 1000};
            std::this_thread::sleep_for(
                std::chrono::milliseconds(dist(gen) + 1000 * threadNumber)
            );
            Logger::registerMyThreadName("T-LORA-" + std::to_string(threadNumber));
            Logger::debug("MAIN-TH-FUNC", "REGISTERED THREAD NAME");

            MockLoRaDevice mockLoRaDevice;
            LoRaManager loraManager(
                mockLoRaDevice,
                threadNumber
            );
            registerInAir(mockLoRaDevice);
            Logger::debug("MAIN-TH-FUNC", "starting rand access");
            // std::atomic <bool> run{true};
            loraManager.randomAccess(runSimulation);
            while (runSimulation.load()) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(dist(gen))
                );
                MessageMine msgMine(
                    // static_cast <float> (loraManager.getMyAddress()),
                    // static_cast <float> (loraManager.getMyAddress())
                    loraManager.getMyAddress(),
                    loraManager.getMyAddress()
                );
                msgMine.setDestinationAddress(broadcastAddress);
                msgMine.setSenderAddress(loraManager.getMyAddress());
                loraManager.addToSendQueue(msgMine);
            }
        },
        loraNo++
        );
    }

    char symbol{0};
    while (symbol != 'q') {
        std::cin >> symbol;
    }

    runSimulation.store(false);

    for (auto &simDev : simulatedDevices) {
        simDev.join();
    }
}

int main() {
    // Disabling stream sync, as I'm using many of them and want them performant.
    std::cout.sync_with_stdio(false);
    Logger::startWithStdout("test_logs.log");
    Logger::registerMyThreadName("T-MAIN");

    // THIS MUST BE KEPT AS SEPARATE SCOPE DUE TO RACE CONDITIONS
    mainLogic();

    Logger::stop();
    return 0;
}