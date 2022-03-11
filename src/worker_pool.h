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
    std::vector<std::shared_ptr<Metrics>> getWorkerMetrics();
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
    for (size_t i = 0; i < numWorkers; i++){
        workers[i] = std::unique_ptr<Worker<Record, Connector>>(new Worker<Record, Connector>(i));
    }
}

template <class Record ,class Connector>
std::vector<std::shared_ptr<Metrics>> WorkerPool<Record, Connector>::getWorkerMetrics(){
    std::vector<std::shared_ptr<Metrics>> workerMetrics(numWorkers);
    for (size_t i = 0; i < numWorkers; i++){
        workerMetrics[i] = workers[i]->getMetrics();
    }
    return std::move(workerMetrics);
}

template <class Record ,class Connector>
void WorkerPool<Record, Connector>::apply(std::shared_ptr<Action<Record, Connector>> action){
    while (!tryApply(action)) {
    }
}

template <class Record ,class Connector>
bool WorkerPool<Record, Connector>::tryApply(std::shared_ptr<Action<Record, Connector>> action){
    for (auto i = workers.begin(); i != workers.end(); i++){
        if ((*i)->tryApply(action)){
            return true;
        }
    }
    return false;
}