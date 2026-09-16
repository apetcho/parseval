#include "parseval/core/thread_pool.hpp"


// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

ThreadPool::ThreadPool(std::size_t thread_count){
    if(thread_count==0){
        thread_count = 1;
    }
    this->m_workers.reserve(thread_count);

    for(std::size_t i=0; i < thread_count; ++i){
        this->m_workers.emplace_back([this](){ this->worker_loop();});
    }
}

/*
class ThreadPool{
public:


    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

ThreadPool::~ThreadPool();

std::size_t ThreadPool::size(void) const noexcept;

template<typename Function, typename... Args>
auto ThreadPool::submit(
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

void ThreadPool::worker_loop(void);

};

static inline ThreadPool& global_thread_pool(void){
    static ThreadPool pool;
    return pool;
}

*/

// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------

