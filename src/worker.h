#pragma once
#include "thread"
#include "mutex"
#include "condition_variable"
#include "memory"
#include "action.h"

enum WorkerStatus{
    UNAVAILABLE = 0,    //The worker has not been initialized or has been destroyed.
    IDLE = 1,           //The worker is idling.
    BUSY = 2            //The worker is busy.
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
    status = IDLE;
    action = nullptr;
    thd = std::thread(&Worker::run, this);
}

template <class Record ,class Connector>
Worker<Record, Connector>::~Worker(){
    std::unique_lock <std::mutex> lck(mtx);
    if (status == BUSY){
        exec();
    }
    status = UNAVAILABLE;
    cv.notify_all();
    lck.unlock();
    thd.join();
}

template <class Record ,class Connector>
void Worker<Record, Connector>::run(){
    std::unique_lock <std::mutex> lck(mtx);
    while (status != UNAVAILABLE){
        while (status == IDLE){
            cv.wait(lck);
        }
        if (status == BUSY) {
            exec();
            status = IDLE;
        }
    }
}

template <class Record ,class Connector>
bool Worker<Record, Connector>::tryApply(std::shared_ptr<Action<Record, Connector>> _action){
    std::unique_lock<std::mutex> lck(mtx, std::try_to_lock);
    if (lck && status == IDLE){
        action = _action;
        status = BUSY;
        cv.notify_all();
        return true;
    }
    return false;
}

template <class Record ,class Connector>
void Worker<Record, Connector>::exec(){
    action->exec();
}