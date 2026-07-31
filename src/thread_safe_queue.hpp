#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
    std::queue<T> _queue;
    std::mutex _mutex;

public:
    size_t size() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(value);
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
};

#endif // THREAD_SAFE_QUEUE_HPP
