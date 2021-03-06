#include "client.h"
#include "doris_record.h"
#include "doris_connector.h"

Config<DorisConnector<DorisRecord>> getConfig(){
    Config<DorisConnector<DorisRecord>> config;

    config.numWorkers = 1;
    config.watcherWakeUpIntervalMs = 20000;
    config.metricsLoggingIntervalMs = 40000;

    config.collectorConfig.numShardsFactor = 2;
    config.collectorConfig.minNumShards = 0;
    config.collectorConfig.targetNumRecords = 5000000;
    config.collectorConfig.targetMemorySize = 100 * 1024 * 1024;
    config.collectorConfig.maxWaitingTimeMs = 1000000;

    config.connector.ip = "172.17.0.3";
    config.connector.port = "8030";
    config.connector.user = "root";
    config.connector.password = "password";

    return std::move(config);
}

int main(){
    Client<DorisRecord, DorisConnector<DorisRecord>> client(getConfig());

    client.exec("test", "truncate table test;");

    for (size_t i = 0; i < 40000000; i++){
        DorisRecord record;
        record.database = "test";
        record.table = "test";
        record.values.reserve(6);
        std::string value(std::to_string(i));
        for (int j = 0; j < 6; j++){
            record.values.push_back(value);
        }
        client.put(record);
    }

    return 0;
}