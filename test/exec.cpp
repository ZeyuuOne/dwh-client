#include "client.h"
#include "doris_record.h"
#include "doris_connector.h"
#include "string"
#include "iostream"

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

    std::string ret = client.exec("test", "truncate table test;");
    std::cout << ret << std::endl;
    
    return 0;
}