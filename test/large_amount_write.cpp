#include "client.h"
#include "doris_record.h"
#include "doris_connector.h"

Config<DorisConnector<DorisRecord>> getConfig(){
    Config<DorisConnector<DorisRecord>> config;

    config.numWorkers = 4;
    config.watcherWakeUpIntervalMs = 20000;
    config.metricsLoggingIntervalMs = 40000;

    config.collectorConfig.targetNumRecords = 1000000;
    config.collectorConfig.maxWaitingTimeMs = 1000000;

    config.connector.ip = "172.17.0.3";
    config.connector.port = "8030";
    config.connector.user = "root";
    config.connector.password = "password";

    return std::move(config);
}

int main(){
    Client<DorisRecord, DorisConnector<DorisRecord>> client(getConfig());

    for (int i = 0;i < 200000000;i++){
        DorisRecord record;
        record.database = "test";
        record.table = "test";
        record.values.reserve(6);
        for (int j = 0; j < 6; j++){
            record.values.push_back(std::to_string(i));
        }
        record.numShards = 4;
        client.put(record);
    }

    return 0;
}