#pragma once
#include "config.h"
#include "unordered_map"
#include "vector"
#include "chrono"
#include "action.h"

template <class Record>
class ShardCollector{
    std::vector<Record> records;
    std::chrono::high_resolution_clock::time_point lastFlushTime;

public:
    std::mutex mtx;
    std::future<ActionResult> result;

    ShardCollector();
    void apply(Record record);
    bool shouldFlush(CollectorConfig& config);
    std::vector<Record>&& flush();
};

template <class Record>
ShardCollector<Record>::ShardCollector(){
    lastFlushTime = std::chrono::high_resolution_clock::now();
}

template <class Record>
void ShardCollector<Record>::apply(Record record){
    if (records.empty()) lastFlushTime = std::chrono::high_resolution_clock::now();
    records.push_back(record);
}

template <class Record>
bool ShardCollector<Record>::shouldFlush(CollectorConfig& config){
    if (records.empty()) return false;
    if (records.size() == config.targetNumRecords) return true;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastFlushTime).count() >= config.maxWaitingTimeMs) return true;
    return false;
}

template <class Record>
std::vector<Record>&& ShardCollector<Record>::flush(){
    lastFlushTime = std::chrono::high_resolution_clock::now();
    return std::move(records);
}
