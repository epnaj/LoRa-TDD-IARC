#include "lora_strategy.hpp"

void LoRaStrategy::send(const BaseMessage &message) {
    setTransmitMode();
    // lora interface send
    setReceiveMode();
    // lora interface wait for ACK
}

void LoRaStrategy::listenAndReceive()