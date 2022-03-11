#pragma once
#include "mutex"

class MetricsHistogram{
public:
    size_t total;
    size_t count;
    size_t min;
    size_t max;
    std::mutex mtx;

    MetricsHistogram();
    void reset();
    void update(size_t n);
    void gather(MetricsHistogram& another);
};

MetricsHistogram::MetricsHistogram(){
    reset();
}

void MetricsHistogram::reset(){
    total = 0;
    count = 0;
    min = SIZE_MAX;
    max = 0;
}

void MetricsHistogram::update(size_t n){
    std::unique_lock <std::mutex> lck(mtx);
    total += n;
    count++;
    if (n < min) min = n;
    if (n > max) max = n;
}

void MetricsHistogram::gather(MetricsHistogram& another){
    std::unique_lock <std::mutex> lck(another.mtx);
    total += another.total;
    count += another.count;
    if (another.min < min) min = another.min;
    if (another.max > max) max = another.max;
    another.reset();
}
