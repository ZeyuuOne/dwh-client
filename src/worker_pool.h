#pragma once
#include "vector"
#include "worker.h"

template <class Record ,class Connector>
class WorkerPool{
    size_t numWorkers;
    std::vector<std::unique_ptr<Worker<Record, Connector>>> workers;

public:
    WorkerPool();
    WorkerPool(size_t _numWorkers);
    void apply(std::shared_ptr<Action<Record, Connector>> action);
    bool tryApply(std::shared_ptr<Action<Record, Connector>> action);
};

template <class Record ,class Connector>
WorkerPool<Record, Connector>::WorkerPool():
    numWorkers(-1)
{
}

template <class Record ,class Connector>
WorkerPool<Record, Connector>::WorkerPool(size_t _numWorkers):
    numWorkers(_numWorkers)
{
    workers.resize(numWorkers);
    for (size_t i = 0;i < numWorkers;i++){
        workers[i] = std::unique_ptr<Worker<Record, Connector>>(new Worker<Record, Connector>(i));
    }
}

template <class Record ,class Connector>
void WorkerPool<Record, Connector>::apply(std::shared_ptr<Action<Record, Connector>> action){
    while (!tryApply(action)) {
    }
}

template <class Record ,class Connector>
bool WorkerPool<Record, Connector>::tryApply(std::shared_ptr<Action<Record, Connector>> action){
    for (auto i = workers.begin();i != workers.end();i++){
        if ((*i)->tryApply(action)){
            return true;
        }
    }
    return false;
}