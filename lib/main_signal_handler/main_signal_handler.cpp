#include "main_signal_handler.hpp"

MainSignalHandler::MainSignalHandler(LoRaManager &loraManager):
    loraManager(loraManager) {}

void MainSignalHandler::startLoop() {
    auto receiveOverload = overloaded {
        [](BadMessage &messageBad) {
            // this means queue is empty or message damaged
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        },
        [](MessageACK &messageAck) {
            messageAck.getMessageNo();
        },
        [&](MessageRandomAccess &messageRandAccess) {
            // if this message is received it means that we are already in the
            // network, so we should provide master clock
            loraManager.addToSendQueue(
                MessageRandomAccessResponse(
                    loraManager.getMyAddress(),
                    loraManager.getTotalNumberOfDevicesInNetwork() + 1
                    // loraManager.getFrameCounter() % loraManager.getTotalNumberOfDevicesInNetwork()
                )
            );
        },
        [&](MessageRandomAccessResponse &MessageRandomAccessResponse) {
            // if this message is received it means we received master clock
            // we should wait for synchronisation with ID 0 device now
            
        },
        [](auto &message) {
            // default, do nothing
            return;
        }
    };

    std::atomic <bool> shouldRandomAccess{true};
    loraManager.randomAccess(shouldRandomAccess);

    while (shouldRun.load()) {
        MessageVariant message = loraManager.receiveMessage();
        std::visit(
            [](auto &msg) {
                Logger::debug("MAIN-RECEIVE-LOOP", "Message received: " + msg.name());
            },
            message
        );
        std::visit(
            receiveOverload,
            message
        );
    }
}

// void MainSignalHandler::stopLoop() {

// }