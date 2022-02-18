#pragma once
#include "config.h"
#include "worker_pool.h"
#include "collector.h"

template <class Record ,class Connector>
class Client{
    Config<Connector> config;
    WorkerPool<Record, Connector> workerPool;
    Collector<Record> collector;

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
    ShardCollector<Record>& shardCollector = collector.apply(record);
    if (shardCollector.shouldFlush(config.collectorConfig)) {
        std::vector<Record> records = shardCollector.flush();
        std::shared_ptr<Action<Record, Connector>> action(new Action<Record, Connector>(config.connector));
        action->setRecords(std::move(records));
        workerPool.apply(action);
    }
}