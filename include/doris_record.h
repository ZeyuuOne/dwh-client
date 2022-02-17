#pragma once
#include "vector"
#include "string"

class DorisRecord{
public:
    std::string database;
    std::string table;
    std::vector<std::string> values;
    std::string getTableIdentifier();
    size_t hash(size_t upperBound);
};

std::string DorisRecord::getTableIdentifier(){
    return table;
}

size_t DorisRecord::hash(size_t upperBound){
    return (size_t)stoi(values[0]) % upperBound;  // Suppose the unique id is the first column.
}