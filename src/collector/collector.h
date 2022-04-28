#pragma once
#include "config.h"
#include "unordered_map"
#include "unordered_set"
#include "vector"
#include "collector/table_collector.h"
#include "collector/shard_collector.h"

template <class Record, class Connector>
class Collector{
    CollectorConfig& collectorConfig;
    Connector& connector;
    std::unordered_map<std::string, TableCollector<Record>> tableCollectors;
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>> shardCollectors;
    
public:
    Collector(CollectorConfig& _collectorConfig, Connector& _connector);
    ShardCollector<Record>& match(Record& record);
    std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& getShardCollectors();
};

template <class Record, class Connector>
Collector<Record, Connector>::Collector(CollectorConfig& _collectorConfig, Connector& _connector):
    collectorConfig(_collectorConfig),
    connector(_connector)
{
}

template <class Record, class Connector>
ShardCollector<Record>& Collector<Record, Connector>::match(Record& record){
    std::string tableIdentifier = record.getTableIdentifier();
    if (tableCollectors.find(tableIdentifier) == tableCollectors.end()) {
        tableCollectors.insert({tableIdentifier, TableCollector<Record>(connector.getNumShards(tableIdentifier), collectorConfig)});
        std::vector<std::shared_ptr<ShardCollector<Record>>>& shardCollectorsInTable = tableCollectors[tableIdentifier].getShardCollectors();
        for (auto i = shardCollectorsInTable.begin(); i != shardCollectorsInTable.end();i++){
            shardCollectors.insert(*i);
        }
    }
    return tableCollectors[std::move(tableIdentifier)].match(record);
}

template <class Record, class Connector>
std::unordered_set<std::shared_ptr<ShardCollector<Record>>>& Collector<Record, Connector>::getShardCollectors(){
    return shardCollectors;
}
