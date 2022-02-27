#pragma once
#include "config.h"
#include "unordered_map"
#include "vector"
#include "table_collector.h"
#include "shard_collector.h"

template <class Record>
class Collector{
    std::unordered_map<std::string, TableCollector<Record>> tableCollectors;
    
public:
    ShardCollector<Record>& apply(Record record);
    std::vector<std::vector<std::vector<Record>>> flush();
};

template <class Record>
ShardCollector<Record>& Collector<Record>::apply(Record record){
    if (tableCollectors.find(record.getTableIdentifier()) == tableCollectors.end()) {
        tableCollectors[record.getTableIdentifier()] = TableCollector<Record>(record.numShards);
    }
    return tableCollectors[record.getTableIdentifier()].apply(record);
}

template <class Record>
std::vector<std::vector<std::vector<Record>>> Collector<Record>::flush(){
    std::vector<std::vector<std::vector<Record>>> collectorRecords(tableCollectors.size());
    size_t i = 0;
    for (auto itr = tableCollectors.begin(); itr != tableCollectors.end(); itr++){
        collectorRecords[i] = itr->second.flush();
        i++;
    }
    return std::move(collectorRecords);
}
