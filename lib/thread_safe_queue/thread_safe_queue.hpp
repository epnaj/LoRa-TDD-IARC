#pragma once

#include <queue>
#include <mutex>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Disable copy and assignment
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Push (copy)
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(internalQueueMutex);
        q.push(value);
    }

    // Push (move)
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(internalQueueMutex);
        q.push(std::move(value));
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(internalQueueMutex);
        q.emplace(std::forward<Args>(args)...);
    }

    void pop() {
        std::lock_guard<std::mutex> lock(internalQueueMutex);
        q.pop();
    }

    // Front (non-thread-safe)
    T& front() {
        return q.front();
    }

    const T& front() const {
        return q.front();
    }

    // Back (non-thread-safe)
    T& back() {
        return q.back();
    }

    const T& back() const {
        return q.back();
    }

    // Empty (non-thread-safe)
    bool empty() const {
        return q.empty();
    }

    // Size (non-thread-safe)
    size_t size() const {
        return q.size();
    }

protected:
    std::queue<T> q;
    mutable std::mutex internalQueueMutex;
};