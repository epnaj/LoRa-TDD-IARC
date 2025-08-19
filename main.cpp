#include <iostream>
#include <atomic>

#include "manager.hpp"
#include "thread_safe_queue.hpp"
#include "logger.hpp"

void mainLogic() {
    Manager manager{};
    
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