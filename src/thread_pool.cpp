#include "thread_pool.hpp"

ThreadPool::ThreadPool(std::size_t threads) {
    const std::size_t count = threads == 0 ? 1 : threads;
    workers_.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        workers_.emplace_back([this] { run_worker(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    available_.notify_all();

    for (std::thread& worker : workers_) {
        worker.join();
    }
}

void ThreadPool::run_worker() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            available_.wait(lock, [this] { return stopping_ || !queue_.empty(); });

            if (queue_.empty()) {
                return;
            }

            task = std::move(queue_.front());
            queue_.pop();
        }

        task();
    }
}
