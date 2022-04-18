#pragma once
#include "string"
#include "mutex"

class MetricsHistogram{
public:
    std::string displayName;
    size_t total;
    size_t count;
    size_t min;
    size_t max;
    std::mutex mtx;

    MetricsHistogram();
    MetricsHistogram(const MetricsHistogram& another);
    MetricsHistogram(std::string _displayName);
    void reset();
    void update(size_t n);
    void gather(MetricsHistogram& another);
    void log();
};

MetricsHistogram::MetricsHistogram(){
}

MetricsHistogram::MetricsHistogram(const MetricsHistogram& another):
    displayName(another.displayName)
{
}

MetricsHistogram::MetricsHistogram(std::string _displayName):
    displayName(_displayName)
{
    reset();
}

void MetricsHistogram::reset(){
    total = 0;
    count = 0;
    min = SIZE_MAX;
    max = 0;
}

void MetricsHistogram::update(size_t n){
    std::unique_lock<std::mutex> lck(mtx);
    total += n;
    count++;
    if (n < min) min = n;
    if (n > max) max = n;
}

void MetricsHistogram::gather(MetricsHistogram& another){
    std::unique_lock<std::mutex> lck(another.mtx);
    total += another.total;
    count += another.count;
    if (another.min < min) min = another.min;
    if (another.max > max) max = another.max;
    another.reset();
}

void MetricsHistogram::log(){
    if (count != 0){
        spdlog::info("{}\tAVR: {}  \tMIN: {}  \tMAX: {}  \t", displayName, total/count, min, max);
    }
}
