#include "string"
#include "vector"
#include "unordered_map"
#include "doris_schema.h"
#include "random"
#include "cpr/cpr.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "exception"
#include "regex"

template<class Record>
class DorisConnector{
public:
    std::string ip;
    std::string port;
    std::string user;
    std::string password;

    std::unordered_map<std::string, DorisSchema> schemas;

    bool valid();
    void write(std::vector<Record> records);
    std::string exec(const std::string& database, const std::string& statement);
    size_t getNumShards(const std::string& tableIdentifier);

private:
    DorisSchema retrieveSchema(const std::string& database, const std::string& table);
};

template<class Record>
bool DorisConnector<Record>::valid(){
    bool valid = true;
    if (ip.empty()){
        spdlog::error("IP in connector config is empty.");
        valid = false;
    }
    if (port.empty()){
        spdlog::error("Port in connector config is empty.");
        valid = false;
    }
    if (user.empty()){
        spdlog::error("User in connector config is empty.");
        valid = false;
    }
    if (password.empty()){
        spdlog::error("Password in connector config is empty.");
        valid = false;
    }
    return valid;
}

template<class Record>
void DorisConnector<Record>::write(std::vector<Record> records){
    if (records.empty()) return;

    std::string& database = records[0].database;
    std::string& table = records[0].table;

    std::string body;
    body.reserve(records.size() * 10);
    for (auto i = records.begin();i != records.end(); i++){
        for (auto j = i->values.begin();j != i->values.end(); j++){
            if (j != i->values.begin()) body += ',';
            body += *j;
        }
        body += '\n';
    }
    
    std::string url("http://" + ip + ":" + port + "/api/" + database + "/" + table + "/_stream_load");
    std::string label(std::to_string(time(0)) + std::to_string(rand()));

    cpr::Response response = cpr::Put(cpr::Url{url},
        cpr::Body{body},
        cpr::Authentication{user, password},
        cpr::Redirect{-1,true,true,cpr::PostRedirectFlags::POST_ALL},
        cpr::Header{
            {"label", label},
            {"column_separator", ","},
            {"expect", "100-continue"}
        }
    );

    auto json = nlohmann::json::parse(response.text);
    if (json["Status"] != "Success"){
        spdlog::error(response.text);
    }
}

template<class Record>
std::string DorisConnector<Record>::exec(const std::string& database, const std::string& statement){
    std::string body("{\"stmt\":\"" + statement + "\"}");
    std::string url("http://" + ip + ":" + port + "/api/query/default_cluster/" + database);

    cpr::Response response = cpr::Post(cpr::Url{url},
        cpr::Body{body},
        cpr::Authentication{user, password},
        cpr::Header{
            {"content-type", "application/json"},
            {"expect", "100-continue"}
        }
    );

    auto json = nlohmann::json::parse(response.text);
    if (json["msg"] != "success"){
        spdlog::error(response.text);
    }

    return std::move(response.text);
}

template<class Record>
size_t DorisConnector<Record>::getNumShards(const std::string& tableIdentifier){
    if (schemas.find(tableIdentifier) == schemas.end()) {
        size_t pos = tableIdentifier.find('\n');
        if (pos == tableIdentifier.npos){ 
            spdlog::error("Inner Exception: Cannot find delimiter in tableIdentifier.");
            throw std::exception();
        }
        std::string database(tableIdentifier.substr(0, pos));
        std::string table(tableIdentifier.substr(pos + 1));
        DorisSchema schema = retrieveSchema(database, table);
        schemas.insert({tableIdentifier, schema});
        
    }
    return schemas[tableIdentifier].numShards;
}

template<class Record>
DorisSchema DorisConnector<Record>::retrieveSchema(const std::string& database, const std::string& table){
    std::string url("http://" + ip + ":" + port + "/api/_get_ddl?db=" + database + "&table=" + table);

    cpr::Response response = cpr::Get(cpr::Url{url},
        cpr::Authentication{user, password},
        cpr::Header{
            {"expect", "100-continue"}
        }
    );

    auto json = nlohmann::json::parse(response.text);
    if (json["msg"] != "success"){
        spdlog::error(response.text);
    }

    DorisSchema schema;
    std::string ddlStatement = json["data"]["create_table"][0];
    std::regex distributionRegex("DISTRIBUTED BY HASH\\((.*)\\) BUCKETS ([0-9]+)");
    std::smatch match;
    if (!std::regex_search(ddlStatement, match, distributionRegex)){
        schema.numShards = 1;
    }
    else {
        schema.numShards = stoi(match[2]);
    }

    return schema;
}
