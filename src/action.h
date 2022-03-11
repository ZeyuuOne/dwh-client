#pragma once
#include "vector"

template <class Record ,class Connector>
class Action{
    Connector& connector;
    std::vector<Record> records;

public:
    Action(Connector& _connector);
    size_t getNumRecords();
    void setRecords(std::vector<Record>&& _records);
    void exec();
};

template <class Record ,class Connector>
Action<Record, Connector>::Action(Connector& _connector):
    connector(_connector)
{
}

template <class Record ,class Connector>
size_t Action<Record, Connector>::getNumRecords(){
    return records.size();
}

template <class Record ,class Connector>
void Action<Record, Connector>::setRecords(std::vector<Record>&& _records){
    records = _records;
}

template <class Record ,class Connector>
void Action<Record, Connector>::exec(){
    connector.exec(records);
}