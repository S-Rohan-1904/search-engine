#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

// A fixed set of worker threads pulling tasks off one queue.
//
// Threads are expensive to create and cheap to reuse, so the pool is built once
// and handed work repeatedly. Every task runs exactly once, on whichever worker
// takes it, and submit() hands back a future for the result.
//
// The pool joins its workers in the destructor, so no task outlives it and
// nothing needs to be shut down by hand.
class ThreadPool {
public:
    // Starts `threads` workers. A count of 0 is treated as 1, so a caller can
    // pass a computed value without guarding it.
    explicit ThreadPool(std::size_t threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::size_t size() const { return workers_.size(); }

    // Queues `task` and returns a future for its result.
    //
    // Tasks start in submission order but finish in whatever order they finish,
    // so a caller wanting determinism must impose it on the results rather than
    // expect it from the pool.
    template <typename Task>
    auto submit(Task task) -> std::future<decltype(task())> {
        using Result = decltype(task());

        auto packaged = std::make_shared<std::packaged_task<Result()>>(std::move(task));
        std::future<Result> future = packaged->get_future();

        {
            const std::lock_guard<std::mutex> lock(mutex_);
            queue_.push([packaged] { (*packaged)(); });
        }
        available_.notify_one();

        return future;
    }

private:
    void run_worker();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> queue_;
    std::mutex mutex_;
    std::condition_variable available_;
    bool stopping_ = false;
};
