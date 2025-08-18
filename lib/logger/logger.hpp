#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <ctime>
#include <iomanip>
#include <atomic>
#include <iostream>
#include <map>
#include <algorithm>
#include <execution>

#include "thread_safe_queue.hpp"

class Logger {
public:
    // Logger(const std::string &filename);
    // ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    
    static void start(const std::string &filename);
    static void startWithStdout(const std::string &filename);
    static void stop();

    static void registerMyThreadName(const std::string &threadName);
    
    static void info(const std::string &tag, const std::string &logData);
    static void debug(const std::string &tag, const std::string &logData);
    static void error(const std::string &tag, const std::string &logData);

    // little hacky, but should do the trick
    // static std::string defTag(const std::string &superFunc = __func__) {
    //     return superFunc;
    // }

private:
    Logger() = default;
    static inline std::fstream fileHandler;

    static inline ThreadSafeQueue <std::string> loggerQueue;
    static inline std::thread loggingThread;
    static inline std::atomic <bool> runLogThread{false};
    static inline std::map <std::thread::id, std::string> threadIdNameMap{};
    static inline std::mutex threadIdNameMapMutex;
    static inline std::size_t maxTIDsize{0};

    static inline void log(
        const std::string &tag, 
        const std::string &logData, 
        const std::string &level
    );
        
    static inline std::string getFormattedTime();
};