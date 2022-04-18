#pragma once
#include "string"
#include "mutex"

class MetricsMeter{
public:
    std::string displayName;
    size_t count;
    std::mutex mtx;

    MetricsMeter();
    MetricsMeter(const MetricsMeter& another);
    MetricsMeter(std::string _displayName);
    void reset();
    void mark();
    void mark(size_t n);
    void gather(MetricsMeter& another);
    void log(size_t timeMs);
};

MetricsMeter::MetricsMeter(){
}

MetricsMeter::MetricsMeter(const MetricsMeter& another):
    displayName(another.displayName)
{
}

MetricsMeter::MetricsMeter(std::string _displayName):
    displayName(_displayName)
{
    reset();
}

void MetricsMeter::reset(){
    count = 0;
}

void MetricsMeter::mark(){
    std::unique_lock<std::mutex> lck(mtx);
    count++;
}

void MetricsMeter::mark(size_t n){
    std::unique_lock<std::mutex> lck(mtx);
    count += n;
}

void MetricsMeter::gather(MetricsMeter& another){
    std::unique_lock<std::mutex> lck(another.mtx);
    count += another.count;
    another.reset();
}

void MetricsMeter::log(size_t timeMs){
    spdlog::info("{}\tCNT: {}  \tCPS: {}", displayName, count, count * 1000 / timeMs);
}
