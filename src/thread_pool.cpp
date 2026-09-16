#include "parseval/core/thread_pool.hpp"
#include "parseval/core/array.hpp"          // for ParsevalError

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

// -
ThreadPool::~ThreadPool(){
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_stopping = true;
    }
    this->m_condition.notify_all();
    for(auto& worker: this->m_workers){
        if(worker.joinable()){
            worker.join();
        }
    }
}

// -
std::size_t ThreadPool::size(void) const noexcept{
    return this->m_workers.size();
}

// -
template<typename Function, typename... Args>
auto ThreadPool::submit(
    Function&& function,
    Args&&... args
) -> std::future<std::invoke_result_t<Function, Args...>>{
    // -
    using Result = std::invoke_result_t<Function, Args...>;

    auto task = std::make_shared<std::packaged_task<Result()>(
        std::bind(
            std::forward<Function>(function),
            std::forward<Args>(args)...
        )
    );

    std::future<Result> result = task->get_fugure();
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if(this->m_stopping){
            throw ParsevalError("Cannot submit work to a stopped ThreadPool");
        }

        this->m_tasks.emplace([task](){
            (*task)();
        });
    }

    this->m_condition.notify_one();
    return result;
}

// -
void ThreadPool::worker_loop(void){
    while(true){
        Task task;
        {
            std::unique_lock<std::mutex> lock(this->m_mutex);
            this->m_condition.wait(
                lock,
                [this]{
                    return this->m_stopping || !this->m_tasks.empty();
                }
            );

            if(this->m_stopping && this->m_tasks.empty()){
                return;
            }

            task = std::move(this->m_tasks.front());
            this->m_tasks.pop();
        }

        task();
    }
}

/*
class ThreadPool{
public:


private:
    using Task = std::function<void()>;

    std::vector<std::thread> m_workers;
    std::queue<Task> m_tasks;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopping = false;

};

static inline ThreadPool& global_thread_pool(void){
    static ThreadPool pool;
    return pool;
}

*/

// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------

