#pragma once
#include "cstddef"

template<class Connector>
class Config{
public:
    size_t numWorkers;
    Connector connector;

    Config();
    bool valid();
};

template<class Connector>
Config<Connector>::Config():
    numWorkers(1)
{
}

template<class Connector>
bool Config<Connector>::valid(){
    if (numWorkers <= 0) return false;
    return true;
}