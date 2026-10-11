#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;

public:
    size_t size() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(value);
        _condition.notify_all();
    }

    /*
     * return true if successfully popped and value and placed it in *value
     * else return false
     */
    bool tryPop(T *value) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) return false;
        (*value) = _queue.front();
        _queue.pop();
        return true;
    }

    void waitAndPop(T *value) {
        std::unique_lock<std::mutex> lock(_mutex);

        _condition.wait(lock, [this] {
            std::cerr << _queue.size() << "\n";
            return !_queue.empty();
        });

        std::cerr << &_queue.front() << "\n";

        (*value) = _queue.front();
        _queue.pop();
    }
};

#endif // THREAD_SAFE_QUEUE_HPP
