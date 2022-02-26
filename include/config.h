#pragma once
#include "cstddef"
#include "spdlog/spdlog.h"

class CollectorConfig{
public:
    int32_t targetNumRecords;

    CollectorConfig();
    bool valid();
};

template<class Connector>
class Config{
public:
    int32_t numWorkers;
    CollectorConfig collectorConfig;
    Connector connector;

    Config();
    bool valid();
};

CollectorConfig::CollectorConfig():
    targetNumRecords(1)
{
}

bool CollectorConfig::valid(){
    bool valid = true;
    if (targetNumRecords <= 0) {
        spdlog::error("Target number of records in config should be positive, which is currently {}.", targetNumRecords);
        valid = false;
    }
    return valid;
}

template<class Connector>
Config<Connector>::Config():
    numWorkers(1)
{
}

template<class Connector>
bool Config<Connector>::valid(){
    bool valid = true;
    if (numWorkers <= 0) {
        spdlog::error("Number of workers in config should be positive, which is currently {}.", numWorkers);
        valid = false;
    }
    if (!collectorConfig.valid()) {
        valid = false;
    }
    return valid;
}