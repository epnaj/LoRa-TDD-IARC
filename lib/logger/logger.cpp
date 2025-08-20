#include "logger.hpp"

/*
SPAWNS OWN THREAD
*/
void Logger::start(const std::string &filename) {
    if (Logger::runLogThread.load()) {
        return;
    }

    Logger::runLogThread.store(true);
    Logger::fileHandler   = std::fstream(filename, std::ios::out | std::ios::app);
    Logger::loggingThread = std::thread(
        []() {
            while (Logger::runLogThread.load()) {
                if (!Logger::loggerQueue.empty()) {
                    Logger::fileHandler << Logger::loggerQueue.front() << "\n";
                    Logger::loggerQueue.pop();
                }
            }
            Logger::fileHandler.flush();
        }
    );
}

/*
SPAWNS OWN THREAD
*/
void Logger::startWithStdout(const std::string &filename) {
    if (Logger::runLogThread.load()) {
        return;
    }

    Logger::runLogThread.store(true);
    Logger::fileHandler   = std::fstream(filename, std::ios::out | std::ios::app);
    Logger::loggingThread = std::thread(
        []() {
            Logger::fileHandler << "### LOGGER-THREAD | Logging thread started!" << std::endl;
            std::cout << "### LOGGER-THREAD | Logging thread started!" << std::endl;
            Logger::fileHandler.flush();
            std::cout << std::flush;
            while (Logger::runLogThread.load()) {
                if (!Logger::loggerQueue.empty()) {
                    std::string message = Logger::loggerQueue.front();
                    Logger::loggerQueue.pop();

                    // This is in separate thread so we can flush as much as we want
                    Logger::fileHandler << message << std::endl;
                    std::cout << message << std::endl;
                    
                    // Logger::fileHandler << message << "\n";
                    // std::cout << message << "\n";
                }
            }
            Logger::fileHandler << "### LOGGER-THREAD | Logging thread finished" << std::endl;
            std::cout << "### LOGGER-THREAD | Logging thread finished" << std::endl;
            Logger::fileHandler.flush();
            std::cout << std::flush;
        }
    );
}

void Logger::stop() {
    if (Logger::runLogThread.load()) {
        Logger::runLogThread.store(false);
        Logger::loggingThread.join();
    }

    if (Logger::fileHandler.is_open()) {
        Logger::fileHandler.close();
    }
}

void Logger::registerMyThreadName(const std::string &threadName) {
    std::lock_guard <std::mutex> mapLock(Logger::threadIdNameMapMutex);
    if (maxTIDsize < threadName.size()) {
        maxTIDsize = threadName.size();
        std::for_each(
            std::execution::par_unseq,
            std::begin(Logger::threadIdNameMap),
            std::end(Logger::threadIdNameMap),
            [](std::pair <const std::thread::id, std::string> &p) -> void {
                p.second += std::string(Logger::maxTIDsize - p.second.size(), ' ');
            }
        );
    }

    std::size_t lengthDifference = Logger::maxTIDsize - threadName.size();

    Logger::threadIdNameMap[std::this_thread::get_id()] = threadName + std::string(lengthDifference, ' ');
}

void Logger::info(const std::string &tag, const std::string &logData) {
    Logger::log(tag, logData, "INFO");
}

void Logger::debug(const std::string &tag, const std::string &logData) {
    Logger::log(tag, logData, "DEBUG");
}

void Logger::error(const std::string &tag, const std::string &logData) {
    Logger::log(tag, logData, "ERROR");
}

void Logger::log(
    const std::string &tag, 
    const std::string &logData, 
    const std::string &level
) {
    std::ostringstream  outputStream;
    auto thisThreadNameIter = Logger::threadIdNameMap.find(std::this_thread::get_id());
    std::string thisThreadName{};
    if (std::end(threadIdNameMap) != thisThreadNameIter) {
        thisThreadName = (*thisThreadNameIter).second;
    }

    outputStream << "[" << 
        Logger::getFormattedTime() << " THREAD: " << thisThreadName << 
        "] " << level << ": " << tag << " | " << logData;

    Logger::loggerQueue.push(outputStream.str());
}

std::string Logger::getFormattedTime() {
    // year - month - day ' ' hour : minute : second
    //  4   1   2   1  2   1   2   1   2    1    2 
    //  4 + 1 + 2 + 1 +2 + 1 + 2 + 1 + 2  + 1 +  2 = 19
    // 19 + 1 (end string character)
    static constexpr auto bufferSize {20};
    char buffer[bufferSize];
    auto time      = std::time(nullptr);
    auto timestamp = std::localtime(&time);
    strftime(
        buffer,
        bufferSize,
        "%Y-%m-%d %H:%M:%S", 
        timestamp
    );
    return std::move(std::string(buffer));
}