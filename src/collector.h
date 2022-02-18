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
class TableCollector{
    size_t numShards;
    std::vector<ShardCollector<Record>> shardCollectors;

public:
    TableCollector();
    TableCollector(size_t _numShards);
    ShardCollector<Record>& apply(Record record);
};

template <class Record>
class Collector{
    std::unordered_map<std::string, TableCollector<Record>> tableCollectors;
    
public:
    ShardCollector<Record>& apply(Record record);
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

template <class Record>
TableCollector<Record>::TableCollector():
    numShards(0)
{
}

template <class Record>
TableCollector<Record>::TableCollector(size_t _numShards):
    numShards(_numShards)
{
}

template <class Record>
ShardCollector<Record>& TableCollector<Record>::apply(Record record){
    if (shardCollectors.empty()) shardCollectors.resize(numShards);
    ShardCollector<Record>& shardCollector = shardCollectors[record.hash(numShards)];
    shardCollector.apply(record);
    return shardCollector;
}

template <class Record>
ShardCollector<Record>& Collector<Record>::apply(Record record){
    if (tableCollectors.find(record.getTableIdentifier()) == tableCollectors.end()) {
        tableCollectors[record.getTableIdentifier()] = TableCollector<Record>(record.numShards);
    }
    return tableCollectors[record.getTableIdentifier()].apply(record);
}
