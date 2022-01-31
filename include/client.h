#pragma once
#include "config.h"
#include "worker_pool.h"

template <class Record ,class Connector>
class Client{
    Config<Connector> config;
    WorkerPool<Record, Connector> workerPool;

public:
    Client(Config<Connector> config);
    void put(Record& record);
};

template <class Record ,class Connector>
Client<Record, Connector>::Client(Config<Connector> _config):
    config(_config),
    workerPool(WorkerPool<Record, Connector>(config.numWorkers))
{
}

template <class Record ,class Connector>
void Client<Record, Connector>::put(Record& record){
    std::shared_ptr<Action<Record, Connector>> action(new Action<Record, Connector>(config.connector));
    action->addRecord(record);
    workerPool.apply(action);
}