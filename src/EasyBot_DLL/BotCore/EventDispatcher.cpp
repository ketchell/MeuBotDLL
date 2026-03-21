#include "EventDispatcher.h"

EventDispatcher* EventDispatcher::instance{nullptr};
std::mutex EventDispatcher::mutex;
EventDispatcher::EventDispatcher() = default;

EventDispatcher::~EventDispatcher() = default;

EventDispatcher *EventDispatcher::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new EventDispatcher();
    }
    return instance;
}

void EventDispatcher::executeEvent() {
    std::function<void()> task;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (task_queue_.empty()) return;
        task = std::move(task_queue_.front());
        task_queue_.pop();
    }
    try {
        if (task) task();
    } catch (const std::exception& e) {
        std::cout << "[EventDispatcher] Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "[EventDispatcher] Unknown exception" << std::endl;
    }
}

