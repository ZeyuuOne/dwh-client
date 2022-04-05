#pragma once
#include "config.h"
#include "unordered_map"
#include "unordered_set"
#include "vector"
#include "collector/table_collector.h"
#include "collector/shard_collector.h"

template <class Record>
class Collector{
    CollectorConfig& collectorConfig;
    std::unordered_map<std::string, TableCollector<Record>> tableCollectors;
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>> shardCollectors;
    
public:
    Collector(CollectorConfig& _collectorConfig);
    ShardCollector<Record>& match(Record& record);
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& getShardCollectors();
};

template <class Record>
Collector<Record>::Collector(CollectorConfig& _collectorConfig):
    collectorConfig(_collectorConfig)
{
}

template <class Record>
ShardCollector<Record>& Collector<Record>::match(Record& record){
    if (tableCollectors.find(record.getTableIdentifier()) == tableCollectors.end()) {
        tableCollectors.insert({record.getTableIdentifier(), TableCollector<Record>(record.numShards, collectorConfig)});
        std::vector<std::shared_ptr<ShardCollector<Record>>>& shardCollectorsInTable = tableCollectors[record.getTableIdentifier()].getShardCollectors();
        for (size_t i = 0; i < shardCollectorsInTable.size();i++){
            shardCollectors.insert(shardCollectorsInTable[i]);
        }
    }
    return tableCollectors[record.getTableIdentifier()].match(record);
}

template <class Record>
std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& Collector<Record>::getShardCollectors(){
    return shardCollectors;
}
