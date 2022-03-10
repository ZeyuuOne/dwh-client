#pragma once
#include "config.h"
#include "worker_pool.h"
#include "chrono"
#include "collector.h"
#include "spdlog/spdlog.h"
#include "exception.h"

enum class ClientStatus{
    UNAVAILABLE = 0,    // The client has not been initialized or has been destroyed.
    RUNNING = 1         // The client is running.
};

template <class Record ,class Connector>
class Client{
    ClientStatus status;
    Config<Connector> config;
    WorkerPool<Record, Connector> workerPool;
    Collector<Record> collector;
    std::thread watcher;

public:
    Client(Config<Connector> config);
    ~Client();
    void put(Record& record);
    void flush();

private:
    void watcherRun();
    void deliver(std::vector<Record>&& records);
    void tryFlushShardCollector(ShardCollector<Record>& shardCollector);
};

template <class Record ,class Connector>
Client<Record, Connector>::Client(Config<Connector> _config):
    config(_config)
{
    if (!config.valid()){
        throw new ConfigNotValidException;
    }
    workerPool = WorkerPool<Record, Connector>(config.numWorkers);
    watcher = std::thread(&Client::watcherRun, this);
    status = ClientStatus::RUNNING;
    spdlog::info("DWH Client running...");
}

template <class Record ,class Connector>
Client<Record, Connector>::~Client(){
    spdlog::info("DWH Client closing...");
    status = ClientStatus::UNAVAILABLE;
    flush();
    watcher.join();
    spdlog::info("Watcher closed.");
}

template <class Record ,class Connector>
void Client<Record, Connector>::put(Record& record){
    ShardCollector<Record>& shardCollector = collector.match(record);
    std::unique_lock <std::mutex> lck(shardCollector.mtx);
    shardCollector.apply(record);
    tryFlushShardCollector(shardCollector);
}

template <class Record ,class Connector>
void Client<Record, Connector>::flush(){
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
    for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
        std::vector<Record> records = (*i)->flush();
        deliver(std::move(records));
    }
}

template <class Record ,class Connector>
void Client<Record, Connector>::watcherRun(){
    spdlog::info("Watcher running...");
    while (status != ClientStatus::UNAVAILABLE){
        std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
        for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
            if (!(*i)->shouldFlush(config.collectorConfig)) continue;
            std::unique_lock <std::mutex> lck((*i)->mtx, std::try_to_lock);
            tryFlushShardCollector(**i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.watcherWakeUpIntervalMs));
    }
}

template <class Record ,class Connector>
void Client<Record, Connector>::deliver(std::vector<Record>&& records){
    if (records.empty()) return;
    std::shared_ptr<Action<Record, Connector>> action(new Action<Record, Connector>(config.connector));
    action->setRecords(std::move(records));
    workerPool.apply(action);
}

template <class Record ,class Connector>
void Client<Record, Connector>::tryFlushShardCollector(ShardCollector<Record>& shardCollector){
    if (shardCollector.shouldFlush(config.collectorConfig)) {
        std::vector<Record> records = shardCollector.flush();
        deliver(std::move(records));
    }
}
