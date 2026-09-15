#pragma once

#include<condition_variable>
#include<functional>
#include<future>
#include<mutex>
#include<queue>
#include<thread>
#include<vector>

// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

//! @todo: make the ThreadPool a Singleton

class ThreadPool{
public:
    explicit ThreadPool(std::size_t thread_count=std::thread::hardware_concurrency());

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool();

    std::size_t size(void) const noexcept;

    template<typename Function, typename... Args>
    auto submit(
        Function&& function,
        Args&&... args
    ) -> std::future<std::invoke_result_t<Function, Args...>>;

private:
    using Task = std::function<void()>;

    std::vector<std::thread> m_workers;
    std::queue<Task> m_tasks;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopping = false;

    void worker_loop(void);
};

static inline ThreadPool& global_thread_pool(void){
    static ThreadPool pool;
    return pool;
}


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------