#pragma once
#include "thread"
#include "mutex"
#include "condition_variable"
#include "memory"
#include "action.h"
#include "spdlog/spdlog.h"

enum class WorkerStatus{
    UNAVAILABLE = 0,    // The worker has not been initialized or has been destroyed.
    IDLE = 1,           // The worker is idling.
    BUSY = 2            // The worker is busy.
};

template <class Record ,class Connector>
class Worker{
    size_t id;
    WorkerStatus status;
    std::shared_ptr<Action<Record, Connector>> action;
    std::thread thd;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Worker(size_t _id);
    ~Worker();
    void run();
    bool tryApply(std::shared_ptr<Action<Record, Connector>> _action);
    void exec();
};

template <class Record ,class Connector>
Worker<Record, Connector>::Worker(size_t _id){
    std::unique_lock <std::mutex> lck(mtx);
    id = _id;
    status = WorkerStatus::IDLE;
    action = nullptr;
    thd = std::thread(&Worker::run, this);
    spdlog::info("Worker {} created.", id);
}

template <class Record ,class Connector>
Worker<Record, Connector>::~Worker(){
    std::unique_lock <std::mutex> lck(mtx);
    if (status == WorkerStatus::BUSY){
        exec();
    }
    status = WorkerStatus::UNAVAILABLE;
    cv.notify_all();
    lck.unlock();
    thd.join();
    spdlog::info("Worker {} closed.", id);
}

template <class Record ,class Connector>
void Worker<Record, Connector>::run(){
    std::unique_lock <std::mutex> lck(mtx);
    while (status != WorkerStatus::UNAVAILABLE){
        while (status == WorkerStatus::IDLE){
            cv.wait(lck);
        }
        if (status == WorkerStatus::BUSY) {
            exec();
            status = WorkerStatus::IDLE;
        }
    }
}

template <class Record ,class Connector>
bool Worker<Record, Connector>::tryApply(std::shared_ptr<Action<Record, Connector>> _action){
    std::unique_lock<std::mutex> lck(mtx, std::try_to_lock);
    if (lck && status == WorkerStatus::IDLE){
        action = _action;
        status = WorkerStatus::BUSY;
        cv.notify_all();
        spdlog::info("Worker {} applied action.", id);
        return true;
    }
    return false;
}

template <class Record ,class Connector>
void Worker<Record, Connector>::exec(){
    action->exec();
}