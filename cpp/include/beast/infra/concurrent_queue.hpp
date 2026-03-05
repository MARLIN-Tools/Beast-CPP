#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace beast::infra {

template <typename T>
class ConcurrentQueue {
   public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        condvar_.notify_one();
    }

    [[nodiscard]] T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condvar_.wait(lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

   private:
    std::mutex mutex_;
    std::condition_variable condvar_;
    std::queue<T> queue_;
};

}  // namespace beast::infra
