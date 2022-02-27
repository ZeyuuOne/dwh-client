#pragma once
#include "config.h"
#include "unordered_map"
#include "vector"

template <class Record>
class ShardCollector{
    std::vector<Record> records;

public:
    void apply(Record record);
    bool shouldFlush(CollectorConfig& config);
    std::vector<Record>&& flush();
};

template <class Record>
void ShardCollector<Record>::apply(Record record){
    records.push_back(record);
}

template <class Record>
bool ShardCollector<Record>::shouldFlush(CollectorConfig& config){
    if (records.size() == config.targetNumRecords) return true;
    return false;
}

template <class Record>
std::vector<Record>&& ShardCollector<Record>::flush(){
    return std::move(records);
}
