#pragma once
#include "vector"
#include "future"

enum class ActionResult{
    SUCCESS = 0,        // The action has been successfully executed.
    FAIL = 1            // The action has failed to be exected.
};

template <class Record ,class Connector>
class Action{
    Connector& connector;
    std::promise<ActionResult> result;
    std::vector<Record> records;

public:
    Action(Connector& _connector);
    std::promise<ActionResult>& getResultPromise();
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
std::promise<ActionResult>& Action<Record, Connector>::getResultPromise(){
    return result;
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
    connector.write(records);
    result.set_value(ActionResult::SUCCESS);
}