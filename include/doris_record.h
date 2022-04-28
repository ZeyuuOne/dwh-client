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
    size_t size();
};

std::string DorisRecord::getTableIdentifier(){
    std::string tableIdentifier(database);
    tableIdentifier.reserve(database.length() + table.length() + 2);
    tableIdentifier += '\n';
    tableIdentifier += table;
    return std::move(tableIdentifier);
}

size_t DorisRecord::hash(size_t upperBound){
    return (size_t)stoi(values[0]) % upperBound;  // Suppose the unique id is the first column.
}

size_t DorisRecord::size(){
    size_t ret = 0;
    for (auto i = values.begin(); i != values.end(); i++){
        ret += i->size() + 1;
    }
    return ret;
}