#include "client.h"
#include "doris_record.h"
#include "doris_connector.h"

Config<DorisConnector<DorisRecord>> getConfig(){
    Config<DorisConnector<DorisRecord>> config;

    config.connector.ip = "172.17.0.3";
    config.connector.port = "8030";
    config.connector.user = "root";
    config.connector.password = "password";

    return std::move(config);
}

int main(){
    Client<DorisRecord, DorisConnector<DorisRecord>> client(getConfig());

    client.exec("test", "truncate table test;");
    
    return 0;
}