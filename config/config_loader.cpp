#include "config_loader.h" 
#include <fstream> 
#include <sstream> 
#include <stdexcept> 
std::map<std::string, std::string> ConfigLoader::load(const std::string& filename) { 
    std::ifstream file(filename); 
    if (!file.is_open()) { 
        throw std::runtime_error( "Cannot open config file: " + filename ); 
    } 
    std::map<std::string, std::string> result; std::string line; 
    std::string section; 
    while (std::getline(file, line)) { 
        // Ignore empty line 
        if (line.empty()) continue; 
        // Ignore comment 
        if (line[0] == '#') continue; 
        // Section: [llm] 
        if (line.front() == '[' && line.back() == ']') { 
            section = line.substr(1, line.size() - 2); 
            continue; 
        } 
        // key=value 
        size_t pos = line.find('='); 
        if (pos == std::string::npos) continue; 
        std::string key = line.substr(0, pos); 
        std::string value = line.substr(pos + 1); 
        // Store as section.key 
        result[section + "." + key] = value; } 
        return result; 
    }