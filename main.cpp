#include <iostream>
#include <atomic>

#include "example.hpp"
#include "manager.hpp"
#include "thread_safe_queue.hpp"
#include "logger.hpp"

class TestStrategy : public ManagerStrategyBase {
public:
    bool send(const BaseMessage &message) override {
        Logger::debug(__func__, "SENDING " + message.name());
        internalQueue.push(message.sendableBytes());
        return true;
    }

    void listenAndReceive(ThreadSafeQueue <MessageVariant> &receivedMessagesQueue) override {
        listenerThread = std::thread(
            [&](){
                Logger::registerMyThreadName("STRATEGY-THREAD");
                while (shouldListen.load()) {
                    if (!internalQueue.empty()) {
                        Logger::debug(__func__, "INTERNAL QUEUE NOT EMPTY");
                        receivedMessagesQueue.push(
                            decodeFromBytes(internalQueue.front())
                        );
                        internalQueue.pop();
                    }

                    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                Logger::debug("TestStrategy::listenAndReceive", "listener thread finished");
            }
        );
    }

    ~TestStrategy() {
        shouldListen.store(false);
        listenerThread.join();
    }

private:
    std::thread listenerThread;
    std::atomic <bool> shouldListen{true};
    ThreadSafeQueue <std::vector <uint8_t>> internalQueue;
};

void mainLogic() {
    TestStrategy managerTestStrategy;
    
    Manager manager{
        managerTestStrategy
    };

    manager.sendMessage(OKMessage());
    Logger::debug(__func__, "MESSAGE SENT!");
    auto receivedMEssage = manager.receiveMessage();
    Logger::debug(__func__, "RECEIVED MESSAGE: " + std::get <OKMessage> (receivedMEssage).name());
    manager.sendMessage(OKMessage());

    manager.sendMessage(MessageAddress());

    std::visit(
        [](auto &&msg) {
            Logger::debug(__func__, ":D " + msg.name());
        },
        manager.receiveMessage()
    );
    
    std::visit(
        [](auto &&msg) {
            Logger::debug(__func__, ":D " + msg.name());
        },
        manager.receiveMessage()
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