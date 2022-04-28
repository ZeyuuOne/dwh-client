#pragma once
#include "unordered_map"
#include "string"
#include "metrics/metrics_meter.h"
#include "metrics/metrics_histogram.h"
#include "vector"
#include "memory"
#include "spdlog/spdlog.h"

class Metrics{
private:
    std::unordered_map<std::string, MetricsMeter> meters;
    std::unordered_map<std::string, MetricsHistogram> histograms;
    std::chrono::steady_clock::time_point lastResetTime;
    std::chrono::steady_clock::time_point lastLoggingTime;
    std::vector<std::shared_ptr<Metrics>> affliatedMetrics;

public:
    Metrics();
    void reset();
    void registerMeter(const std::string& name, const std::string& displayName);
    void registerHistogram(const std::string& name, const std::string& displayName);
    MetricsMeter& getMeter(const std::string& name);
    MetricsHistogram& getHistogram(const std::string& name);
    void setAffliatedMetrics(std::vector<std::shared_ptr<Metrics>> _affliatedMetrics);
    size_t getWaitingTimeMs();
    void gather(Metrics& another);
    void gatherAffliatedMetrics();
    void log();
};

Metrics::Metrics(){
    reset();
}

void Metrics::reset(){
    for (auto i = meters.begin(); i != meters.end(); i++){
        i->second.reset();
    }
    for (auto i = histograms.begin(); i != histograms.end(); i++){
        i->second.reset();
    }
    lastResetTime = std::chrono::steady_clock::now();
    lastLoggingTime = lastResetTime;
}

void Metrics::registerMeter(const std::string& name, const std::string& displayName){
    meters.insert({name, MetricsMeter(displayName)});
}

void Metrics::registerHistogram(const std::string& name, const std::string& displayName){
    histograms.insert({name, MetricsHistogram(displayName)});
}
 
MetricsMeter& Metrics::getMeter(const std::string& name){
    return meters[name];
}

MetricsHistogram& Metrics::getHistogram(const std::string& name){
    return histograms[name];
}

void Metrics::setAffliatedMetrics(std::vector<std::shared_ptr<Metrics>> _affliatedMetrics){
    affliatedMetrics = _affliatedMetrics;
}

size_t Metrics::getWaitingTimeMs(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastLoggingTime).count();
}

void Metrics::gather(Metrics& another){
    for (auto i = meters.begin(); i != meters.end(); i++){
        i->second.gather(another.getMeter(i->first));
    }
    for (auto i = histograms.begin(); i != histograms.end(); i++){
        i->second.gather(another.getHistogram(i->first));
    }
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
    for (auto i = meters.begin(); i != meters.end(); i++){
        i->second.log(timeMs);
    }
    for (auto i = histograms.begin(); i != histograms.end(); i++){
        i->second.log();
    }
}
