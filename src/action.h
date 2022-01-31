#pragma once
#include "vector"

template <class Record ,class Connector>
class Action{
    Connector& connector;
    std::vector<Record> records;

public:
    Action(Connector& _connector);
    void addRecord(Record& record);
    void exec();
};

template <class Record ,class Connector>
Action<Record, Connector>::Action(Connector& _connector):
    connector(_connector)
{
}

template <class Record ,class Connector>
void Action<Record, Connector>::addRecord(Record& record){
    records.push_back(record);
}

template <class Record ,class Connector>
void Action<Record, Connector>::exec(){
    connector.exec(records);
}