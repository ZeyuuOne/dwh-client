#include "string"
#include "vector"
#include "random"
#include "cpr/cpr.h"

template<class Record>
class DorisConnector{
public:
    std::string ip;
    std::string port;
    std::string user;
    std::string password;

    void exec(std::vector<Record> records);
};

template<class Record>
void DorisConnector<Record>::exec(std::vector<Record> records){
    if (records.empty()) return;

    std::string& database = records[0].database;
    std::string& table = records[0].table;

    std::string body;
    for (auto i = records.begin();i != records.end(); i++){
        for (auto j = i->values.begin();j != i->values.end(); j++){
            if (j != i->values.begin()) body += ',';
            body += *j;
        }
        body += '\n';
    }
    
    std::string url("http://" + ip + ":" + port + "/api/" + database + "/" + table + "/_stream_load");
    std::string label(std::to_string(time(0)) + std::to_string(rand()));

    cpr::Response r = cpr::Put(cpr::Url{url},
        cpr::Body{body},
        cpr::Authentication{user, password},
        cpr::Redirect{-1,true,true,cpr::PostRedirectFlags::POST_ALL},
        cpr::Header{
            {"label", label},
            {"column_separator", ","},
            {"expect", "100-continue"}
        }
    );
}