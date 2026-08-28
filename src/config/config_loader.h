#pragma once
#include <map>
#include <string>
class ConfigLoader { 
public: 
    static std::map<std::string, std::string> load(const std::string& filename); 
};