#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H
#define g_dispatcher EventDispatcher::getInstance()
#include <functional>
#include <mutex>
#include <queue>
#include <future>
#include <iostream>

class EventDispatcher {
protected:
    EventDispatcher();
    ~EventDispatcher();
public:
    EventDispatcher(EventDispatcher const&) = delete;
    void operator=(const EventDispatcher&) = delete;
    static EventDispatcher* getInstance();

    template<typename Func>
    auto scheduleEventEx(Func &&func) -> std::invoke_result_t<Func>
    {
        using ResultType = std::invoke_result_t<Func>;
        auto promise = std::make_shared<std::promise<ResultType>>();
        std::future<ResultType> future = promise->get_future();
        auto task = [func = std::forward<Func>(func), promise]() mutable
        {
            try
            {
                if constexpr (std::is_void_v<ResultType>)
                {
                    func();
                    promise->set_value();
                }
                else
                {
                    promise->set_value(func());
                }
            }
            catch (...)
            {
                std::cout << "Error" << std::endl;
                promise->set_exception(std::current_exception());
            }
        };
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push(std::move(task));
        }
        return future.get();
    }

    void executeEvent();

private:
    static EventDispatcher* instance;
    static std::mutex mutex;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
};



#endif //TASKSCHEDULER_H
