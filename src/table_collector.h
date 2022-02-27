#pragma once
#include "config.h"
#include "unordered_map"
#include "vector"
#include "shard_collector.h"

template <class Record>
class TableCollector{
    size_t numShards;
    std::vector<ShardCollector<Record>> shardCollectors;

public:
    TableCollector();
    TableCollector(size_t _numShards);
    ShardCollector<Record>& apply(Record record);
    std::vector<std::vector<Record>> flush();
};


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
std::vector<std::vector<Record>> TableCollector<Record>::flush(){
    std::vector<std::vector<Record>> tableRecords(numShards);
    for (size_t i = 0;i < numShards;i++){
        tableRecords[i] = shardCollectors[i].flush();
    }
    return std::move(tableRecords);
}
