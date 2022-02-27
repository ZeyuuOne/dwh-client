#pragma once
#include "config.h"
#include "worker_pool.h"
#include "collector.h"
#include "spdlog/spdlog.h"
#include "exception.h"

template <class Record ,class Connector>
class Client{
    Config<Connector> config;
    WorkerPool<Record, Connector> workerPool;
    Collector<Record> collector;

public:
    Client(Config<Connector> config);
    ~Client();
    void put(Record& record);

private:
    void deliver(std::vector<Record>&& records);
};

template <class Record ,class Connector>
Client<Record, Connector>::Client(Config<Connector> _config):
    config(_config)
{
    if (!config.valid()){
        throw new ConfigNotValidException;
    }
    workerPool = WorkerPool<Record, Connector>(config.numWorkers);
    spdlog::info("DWH Client running...");
}

template <class Record ,class Connector>
Client<Record, Connector>::~Client(){
    spdlog::info("DWH Client closing...");
    std::vector<std::vector<std::vector<Record>>> collectorRecords = collector.flush();
    for (auto i = collectorRecords.begin(); i != collectorRecords.end(); i++){
        for (auto j = i->begin(); j != i->end(); j++){
            deliver(std::move(*j));
        }
    }
}

template <class Record ,class Connector>
void Client<Record, Connector>::put(Record& record){
    ShardCollector<Record>& shardCollector = collector.apply(record);
    if (shardCollector.shouldFlush(config.collectorConfig)) {
        std::vector<Record> records = shardCollector.flush();
        deliver(std::move(records));
    }
}

template <class Record ,class Connector>
void Client<Record, Connector>::deliver(std::vector<Record>&& records){
    if (records.empty()) return;
    std::shared_ptr<Action<Record, Connector>> action(new Action<Record, Connector>(config.connector));
    action->setRecords(std::move(records));
    workerPool.apply(action);
}
