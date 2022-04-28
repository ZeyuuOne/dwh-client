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
    void put(Record record);
    void flush();

private:
    void watcherRun();
    std::future<ActionResult> deliver(std::vector<Record>&& records);
    void flushShardCollectorIfReachTarget(ShardCollector<Record>& shardCollector);
    void tryFlushShardCollectorIfTimeOut(ShardCollector<Record>& shardCollector);
};

template <class Record ,class Connector>
Client<Record, Connector>::Client(Config<Connector> _config):
    config(_config),
    workerPool(WorkerPool<Record, Connector>(config.numWorkers)),
    collector(Collector<Record>(config.collectorConfig))
{
    if (!config.valid()){
        throw new ConfigNotValidException;
    }
    metrics.registerMeter("numRequests", "Number of requests ");
    metrics.registerMeter("numRecords", "Number of records  ");
    metrics.registerMeter("numTimeouts", "Number of timeouts ");
    metrics.registerHistogram("actionApplyDelayMs", "Action apply delay ");
    metrics.registerHistogram("actionExecTimeMs", "Action execute time");
    metrics.registerHistogram("workerIdleTimeMs", "Worker idle time   ");
    metrics.registerHistogram("acquireSemaphoreDelayMs", "Wait 4 worker delay");
    metrics.registerHistogram("waitForFutureDelayMs", "Wait 4 future delay");
    metrics.setAffliatedMetrics(workerPool.getWorkerMetrics());
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
            (*i)->result.wait();
        }
    }
    metrics.gatherAffliatedMetrics();
    metrics.log();
    watcher.join();
    spdlog::info("Watcher closed.");
}

template <class Record ,class Connector>
void Client<Record, Connector>::put(Record record){
    ShardCollector<Record>& shardCollector = collector.match(record);
    std::unique_lock<std::mutex> lck(shardCollector.mtx);
    shardCollector.apply(std::move(record));
    flushShardCollectorIfReachTarget(shardCollector);
}

template <class Record ,class Connector>
void Client<Record, Connector>::flush(){
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& shardCollectors = collector.getShardCollectors();
    for (auto i = shardCollectors.begin(); i != shardCollectors.end(); i++){
        std::vector<Record> records = (*i)->flush();
        if (records.empty()) continue;
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        if ((*i)->result.valid()){
            (*i)->result.wait();
        }
        metrics.getHistogram("waitForFutureDelayMs").update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
        startTime = std::chrono::steady_clock::now();
        workerPool.availableWorkers.acquire();
        metrics.getHistogram("acquireSemaphoreDelayMs").update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
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
            if (!(*i)->timeOut()) continue;
            std::unique_lock<std::mutex> lck((*i)->mtx, std::try_to_lock);
            if (!lck) continue;
            tryFlushShardCollectorIfTimeOut(**i);
        }
        if (metrics.getWaitingTimeMs() >= config.metricsLoggingIntervalMs){
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
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    workerPool.apply(action);
    metrics.getHistogram("actionApplyDelayMs").update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
    return std::move(future);
}

template <class Record ,class Connector>
void Client<Record, Connector>::flushShardCollectorIfReachTarget(ShardCollector<Record>& shardCollector){
    if (!shardCollector.reachTarget()) return;
    std::vector<Record> records = shardCollector.flush();
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    if (shardCollector.result.valid()){
        shardCollector.result.wait();
    }
    metrics.getHistogram("waitForFutureDelayMs").update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
    startTime = std::chrono::steady_clock::now();
    workerPool.availableWorkers.acquire();
    metrics.getHistogram("acquireSemaphoreDelayMs").update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
    std::future<ActionResult> result = deliver(std::move(records));
    shardCollector.result = std::move(result);
}

template <class Record ,class Connector>
void Client<Record, Connector>::tryFlushShardCollectorIfTimeOut(ShardCollector<Record>& shardCollector){
    if (!shardCollector.timeOut()) return;
    if (shardCollector.result.valid()){
        if (shardCollector.result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    }
    if (!workerPool.availableWorkers.try_acquire()) return;
    std::vector<Record> records = shardCollector.flush();
    std::future<ActionResult> result = deliver(std::move(records));
    shardCollector.result = std::move(result);
    metrics.getMeter("numTimeouts").mark();
}
