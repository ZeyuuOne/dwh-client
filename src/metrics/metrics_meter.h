#pragma once
#include "mutex"

class MetricsMeter{
public:
    size_t count;
    std::mutex mtx;

    MetricsMeter();
    void reset();
    void mark();
    void mark(size_t n);
    void gather(MetricsMeter& another);
};

MetricsMeter::MetricsMeter(){
    reset();
}

void MetricsMeter::reset(){
    count = 0;
}

void MetricsMeter::mark(){
    std::unique_lock <std::mutex> lck(mtx);
    count++;
}

void MetricsMeter::mark(size_t n){
    std::unique_lock <std::mutex> lck(mtx);
    count += n;
}

void MetricsMeter::gather(MetricsMeter& another){
    std::unique_lock <std::mutex> lck(another.mtx);
    count += another.count;
    another.reset();
}
