#include "client.h"
#include "doris_record.h"
#include "doris_connector.h"

Config<DorisConnector<DorisRecord>> getConfig(){
    Config<DorisConnector<DorisRecord>> config;

    config.numWorkers = 4;
    config.watcherWakeUpIntervalMs = 500;
    config.metricsLoggingIntervalMs = 3000;

    config.collectorConfig.targetNumRecords = 10;
    config.collectorConfig.targetMemorySize = 1024;
    config.collectorConfig.maxWaitingTimeMs = 2000;

    config.connector.ip = "172.17.0.3";
    config.connector.port = "8030";
    config.connector.user = "root";
    config.connector.password = "password";

    return std::move(config);
}

int main(){
    Client<DorisRecord, DorisConnector<DorisRecord>> client(getConfig());

    client.exec("test", "truncate table test;");

    for (size_t i = 0; i < 100; i++){
        DorisRecord record;
        record.database = "test";
        record.table = "test";
        record.values.reserve(6);
        std::string value(std::to_string(i));
        for (size_t j = 0; j < 6; j++){
            record.values.push_back(value);
        }
        record.numShards = 10;
        client.put(record);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    for (size_t i = 100; i < 104; i++){
        DorisRecord record;
        record.database = "test";
        record.table = "test";
        record.values.reserve(6);
        std::string value(std::to_string(i));
        for (size_t j = 0; j < 6; j++){
            record.values.push_back(value);
        }
        record.numShards = 10;
        client.put(record);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }
    
    return 0;
}