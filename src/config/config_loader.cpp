#include "config_loader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::map<std::string, std::string> ConfigLoader::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filename);
    }
    std::map<std::string, std::string> result;
    std::string line;
    std::string section;
    while (std::getline(file, line)) {
        line = trim(line);
        // Ignore empty line
        if (line.empty()) continue;
        // Ignore comment
        if (line[0] == '#') continue;
        // Section: [llm]
        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        // key=value
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        // Store as section.key
        if (!section.empty()) {
            result[section + "." + key] = value;
        } else {
            result[key] = value;
        }
    }
    return result;
}