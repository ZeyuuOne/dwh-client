#pragma once
#include "config.h"
#include "unordered_map"
#include "vector"
#include "chrono"
#include "action.h"

template <class Record>
class ShardCollector{
    CollectorConfig& collectorConfig;
    std::vector<Record> records;
    std::chrono::steady_clock::time_point lastFlushTime;
    size_t size;

public:
    std::mutex mtx;
    std::future<ActionResult> result;

    ShardCollector(CollectorConfig& _collectorConfig);
    void apply(Record&& record);
    bool reachTarget();
    bool timeOut();
    std::vector<Record>&& flush();
};

template <class Record>
ShardCollector<Record>::ShardCollector(CollectorConfig& _collectorConfig):
    collectorConfig(_collectorConfig)
{
    lastFlushTime = std::chrono::steady_clock::now();
    size = 0;
}

template <class Record>
void ShardCollector<Record>::apply(Record&& record){
    if (records.empty()){
        records.reserve(collectorConfig.targetNumRecords);
        lastFlushTime = std::chrono::steady_clock::now();
    }
    size += record.size();
    records.push_back(std::move(record));
}

template <class Record>
bool ShardCollector<Record>::reachTarget(){
    if (records.empty()) return false;
    if (records.size() >= collectorConfig.targetNumRecords) return true;
    if (size >= collectorConfig.targetMemorySize) return true;
    return false;
}

template <class Record>
bool ShardCollector<Record>::timeOut(){
    if (records.empty()) return false;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastFlushTime).count() >= collectorConfig.maxWaitingTimeMs) return true;
    return false;
}

template <class Record>
std::vector<Record>&& ShardCollector<Record>::flush(){
    lastFlushTime = std::chrono::steady_clock::now();
    size = 0;
    return std::move(records);
}
