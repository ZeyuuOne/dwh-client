#pragma once
#include "metrics/metrics_meter.h"
#include "metrics/metrics_histogram.h"
#include "vector"
#include "memory"
#include "spdlog/spdlog.h"

class Metrics{
public:
    MetricsMeter numRequests;
    MetricsMeter numRecords;
    MetricsHistogram deliverDelayMs;
    MetricsHistogram actionExecTimeMs;
    std::chrono::steady_clock::time_point lastResetTime;
    std::chrono::steady_clock::time_point lastLoggingTime;
    std::vector<std::shared_ptr<Metrics>> affliatedMetrics;

    Metrics();
    void reset();
    void gather(Metrics& another);
    void gatherAffliatedMetrics();
    void log();
};

Metrics::Metrics(){
    reset();
}

void Metrics::reset(){
    numRequests.reset();
    numRecords.reset();
    deliverDelayMs.reset();
    actionExecTimeMs.reset();
    lastResetTime = std::chrono::steady_clock::now();
    lastLoggingTime = lastResetTime;
}

void Metrics::gather(Metrics& another){
    numRequests.gather(another.numRequests);
    numRecords.gather(another.numRecords);
    deliverDelayMs.gather(another.deliverDelayMs);
    actionExecTimeMs.gather(another.actionExecTimeMs);
}

void Metrics::gatherAffliatedMetrics(){
    for (size_t i = 0; i < affliatedMetrics.size(); i++){
        gather(*(affliatedMetrics[i]));
    }
}

void Metrics::log(){
    lastLoggingTime = std::chrono::steady_clock::now();
    size_t timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(lastLoggingTime - lastResetTime).count();
    spdlog::info("------------------------- Last {}ms -------------------------", timeMs);
    spdlog::info("Number of requests \tCNT: {}  \tCPS: {}  \t", numRequests.count, numRequests.count * 1000 / timeMs);
    spdlog::info("Number of records  \tCNT: {}  \tCPS: {}  \t", numRecords.count, numRecords.count * 1000 / timeMs);
    if (deliverDelayMs.count != 0){
        spdlog::info("Deliver delay      \tAVR: {}  \tMIN: {}  \tMAX: {}  \t", deliverDelayMs.total/deliverDelayMs.count, deliverDelayMs.min, deliverDelayMs.max);
    }
    if (actionExecTimeMs.count != 0){
        spdlog::info("Action execute time\tAVR: {}  \tMIN: {}  \tMAX: {}  \t", actionExecTimeMs.total/actionExecTimeMs.count, actionExecTimeMs.min, actionExecTimeMs.max);
    }
}
