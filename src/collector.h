#pragma once
#include "config.h"
#include "unordered_map"
#include "unordered_set"
#include "vector"
#include "table_collector.h"
#include "shard_collector.h"

template <class Record>
class Collector{
    std::unordered_map<std::string, TableCollector<Record>> tableCollectors;
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>> shardCollectors;
    
public:
    ShardCollector<Record>& apply(Record record);
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& getShardCollectors();
};

template <class Record>
ShardCollector<Record>& Collector<Record>::apply(Record record){
    if (tableCollectors.find(record.getTableIdentifier()) == tableCollectors.end()) {
        tableCollectors.insert({record.getTableIdentifier(), TableCollector<Record>(record.numShards)});
        std::vector<std::shared_ptr<ShardCollector<Record>>>& shardCollectorsInTable = tableCollectors[record.getTableIdentifier()].getShardCollectors();
        for (size_t i = 0; i < shardCollectorsInTable.size();i++){
            shardCollectors.insert(shardCollectorsInTable[i]);
        }
    }
    return tableCollectors[record.getTableIdentifier()].apply(record);
}

template <class Record>
std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& Collector<Record>::getShardCollectors(){
    return shardCollectors;
}
