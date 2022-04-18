#pragma once
#define _HAS_CXX20
#include "thread"
#include "mutex"
#include "condition_variable"
#include "memory"
#include "action.h"
#include "metrics/metrics.h"
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
    std::shared_ptr<Metrics> metrics;
    std::thread thd;
    std::mutex mtx;
    std::condition_variable cv;
    std::counting_semaphore<INT32_MAX>& availableWorkers;

public:
    Worker(size_t _id, std::counting_semaphore<INT32_MAX>& _availableWorkers);
    ~Worker();
    std::shared_ptr<Metrics> getMetrics();
    void run();
    bool tryApply(std::shared_ptr<Action<Record, Connector>> _action);
    void exec();
};

template <class Record ,class Connector>
Worker<Record, Connector>::Worker(size_t _id, std::counting_semaphore<INT32_MAX>& _availableWorkers):
    availableWorkers(_availableWorkers)
{
    std::unique_lock<std::mutex> lck(mtx);
    id = _id;
    status = WorkerStatus::IDLE;
    action = nullptr;
    metrics = std::shared_ptr<Metrics>(new Metrics);
    thd = std::thread(&Worker::run, this);
    spdlog::info("Worker {} created.", id);
}

template <class Record ,class Connector>
Worker<Record, Connector>::~Worker(){
    std::unique_lock<std::mutex> lck(mtx);
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
std::shared_ptr<Metrics> Worker<Record, Connector>::getMetrics(){
    return metrics;
}

template <class Record ,class Connector>
void Worker<Record, Connector>::run(){
    std::unique_lock<std::mutex> lck(mtx);
    while (status != WorkerStatus::UNAVAILABLE){
        while (status == WorkerStatus::IDLE){
            availableWorkers.release();
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
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    action->exec();
    metrics->actionExecTimeMs.update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
    metrics->numRequests.mark();
    metrics->numRecords.mark(action->getNumRecords());
}