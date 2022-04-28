#pragma once
#include "cstddef"
#include "spdlog/spdlog.h"

class CollectorConfig{
public:
    int32_t targetNumRecords;
    int32_t targetMemorySize;
    int32_t maxWaitingTimeMs;

    CollectorConfig();
    bool valid();
};

template<class Connector>
class Config{
public:
    int32_t numWorkers;
    int32_t watcherWakeUpIntervalMs;
    int32_t metricsLoggingIntervalMs;
    CollectorConfig collectorConfig;
    Connector connector;

    Config();
    bool valid();
};

CollectorConfig::CollectorConfig():
    targetNumRecords(1),
    targetMemorySize(1024),
    maxWaitingTimeMs(1000)
{
}

bool CollectorConfig::valid(){
    bool valid = true;
    if (targetNumRecords <= 0){
        spdlog::error("Target number of records in config should be positive, which is currently {}.", targetNumRecords);
        valid = false;
    }
    if (targetMemorySize <= 0){
        spdlog::error("Target memory size in config should be positive, which is currently {}.", targetNumRecords);
        valid = false;
    }
    if (maxWaitingTimeMs <= 0){
        spdlog::error("Maximum waiting time in config should be positive, which is currently {}.", targetNumRecords);
        valid = false;
    }
    return valid;
}

template<class Connector>
Config<Connector>::Config():
    numWorkers(1),
    watcherWakeUpIntervalMs(1000),
    metricsLoggingIntervalMs(10000)
{
}

template<class Connector>
bool Config<Connector>::valid(){
    bool valid = true;
    if (numWorkers <= 0){
        spdlog::error("Number of workers in config should be positive, which is currently {}.", numWorkers);
        valid = false;
    }
    if (watcherWakeUpIntervalMs <= 0){
        spdlog::error("Watcher wake up interval in config should be positive, which is currently {}.", numWorkers);
        valid = false;
    }
    if (metricsLoggingIntervalMs <= 0){
        spdlog::error("Metrics logging interval in config should be positive, which is currently {}.", metricsLoggingIntervalMs);
        valid = false;
    }
    if (!collectorConfig.valid()){
        valid = false;
    }
    if (!connector.valid()){
        valid = false;
    }
    return valid;
}