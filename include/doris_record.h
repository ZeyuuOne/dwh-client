#pragma once
#include "vector"
#include "string"

class DorisRecord{
public:
    std::string database;
    std::string table;
    std::vector<std::string> values;
};