#pragma once
#include "cstddef"

class CollectorConfig{
public:
    size_t targetNumRecords;

    CollectorConfig();
    bool valid();
};

template<class Connector>
class Config{
public:
    size_t numWorkers;
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
    if (targetNumRecords <= 0) return false;
    return true;
}

template<class Connector>
Config<Connector>::Config():
    numWorkers(1)
{
}

template<class Connector>
bool Config<Connector>::valid(){
    if (numWorkers <= 0) return false;
    if (!collectorConfig.valid()) return false;
    return true;
}