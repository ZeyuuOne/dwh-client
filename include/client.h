#pragma once
#include "config.h"
#include "worker_pool.h"
#include "chrono"   
#include "collector/collector.h"
#include "spdlog/spdlog.h"
#include "exception.h"
#include "metrics/metrics.h"
#include "future"

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
    Metrics metrics;
    std::thread watcher;

public:
    Client(Config<Connector> config);
    ~Client();
    void put(Record& record);
    void flush();

private:
    void watcherRun();
    std::future<ActionResult> deliver(std::vector<Record>&& records);
    void flushShardCollectorIfShould(ShardCollector<Record>& shardCollector);
};

template <class Record ,class Connector>
Client<Record, Connector>::Client(Config<Connector> _config):
    config(_config),
    collector(Collector<Record>(config.collectorConfig))
{
    if (!config.valid()){
        throw new ConfigNotValidException;
    }
    workerPool = WorkerPool<Record, Connector>(config.numWorkers);
    metrics.affliatedMetrics = workerPool.getWorkerMetrics();
    watcher = std::thread(&Client::watcherRun, this);
    status = ClientStatus::RUNNING;
    spdlog::info("DWH Client running...");
}

template <class Record ,class Connector>
Client<Record, Connector>::~Client(){
    spdlog::info("DWH Client closing...");
    status = ClientStatus::UNAVAILABLE;
    flush();
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
    for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
        if ((*i)->result.valid()){
            (*i)->result.get();
        }
    }
    metrics.gatherAffliatedMetrics();
    metrics.log();
    watcher.join();
    spdlog::info("Watcher closed.");
}

template <class Record ,class Connector>
void Client<Record, Connector>::put(Record& record){
    ShardCollector<Record>& shardCollector = collector.match(record);
    std::unique_lock<std::mutex> lck(shardCollector.mtx);
    shardCollector.apply(record);
    flushShardCollectorIfShould(shardCollector);
}

template <class Record ,class Connector>
void Client<Record, Connector>::flush(){
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
    for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
        std::vector<Record> records = (*i)->flush();
        if (records.empty()) continue;
        if ((*i)->result.valid()){
            (*i)->result.get();
        }
        std::future<ActionResult> result = deliver(std::move(records));
        (*i)->result = std::move(result);
    }
}

template <class Record ,class Connector>
void Client<Record, Connector>::watcherRun(){
    spdlog::info("Watcher running...");
    while (status != ClientStatus::UNAVAILABLE){
        std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
        for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
            if (!(*i)->shouldFlush(config.collectorConfig)) continue;
            std::unique_lock<std::mutex> lck((*i)->mtx, std::try_to_lock);
            if (!lck) continue;
            flushShardCollectorIfShould(**i);
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - metrics.lastLoggingTime).count() >= config.metricsLoggingIntervalMs){
            metrics.gatherAffliatedMetrics();
            metrics.log();
            metrics.reset();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.watcherWakeUpIntervalMs));
    }
}

template <class Record ,class Connector>
std::future<ActionResult> Client<Record, Connector>::deliver(std::vector<Record>&& records){
    std::shared_ptr<Action<Record, Connector>> action(new Action<Record, Connector>(config.connector));
    std::promise<ActionResult>& promise = action->getResultPromise();
    std::future<ActionResult> future = promise.get_future();
    action->setRecords(std::move(records));
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    workerPool.apply(action);
    metrics.deliverDelayMs.update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count());
    return std::move(future);
}

template <class Record ,class Connector>
void Client<Record, Connector>::flushShardCollectorIfShould(ShardCollector<Record>& shardCollector){
    if (shardCollector.shouldFlush(config.collectorConfig)) {
        std::vector<Record> records = shardCollector.flush();
        if (shardCollector.result.valid()){
            shardCollector.result.get();
        }
        std::future<ActionResult> result = deliver(std::move(records));
        shardCollector.result = std::move(result);
    }
}
