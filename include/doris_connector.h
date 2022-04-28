#include "string"
#include "vector"
#include "random"
#include "cpr/cpr.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

template<class Record>
class DorisConnector{
public:
    std::string ip;
    std::string port;
    std::string user;
    std::string password;

    bool valid();
    void write(std::vector<Record> records);
    void exec(const std::string& database, const std::string& statement);
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
void DorisConnector<Record>::exec(const std::string& database, const std::string& statement){
    std::string body("{\"stmt\":\"" + statement + "\"}");
    std::string url("http://" + ip + ":" + port + "/api/query/default_cluster/" + database);

    cpr::Response response = cpr::Post(cpr::Url{url},
        cpr::Body{body},
        cpr::Authentication{user, password},
        cpr::Redirect{-1,true,true,cpr::PostRedirectFlags::POST_ALL},
        cpr::Header{
            {"content-type", "application/json"},
            {"expect", "100-continue"}
        }
    );

    auto json = nlohmann::json::parse(response.text);
    if (json["msg"] != "success"){
        spdlog::error(response.text);
    }
}
    