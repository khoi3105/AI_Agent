#pragma once
#include <string>

struct MultiAgentConfig {
    std::string workerBaseUrl = "";
    std::string workerModel = "";
    std::string workerApiKey = "";
    double workerTemperature = 0.1;
    double workerTopP = 0.95;
    int workerMaxTokens = 8192;
    int workerRpm = 0;
};
