#pragma once 
#include <string> 
struct LLMConfig { 
    std::string baseUrl = "https://integrate.api.nvidia.com/v1/chat/completions";
    std::string model = "meta/llama-3.2-11b-vision-instruct";
    double temperature = 0.1;
    double topP = 1.0;
    int maxTokens = 16384;
    bool stream = false;
    int rpm = 0; // 0 = unlimited, >0 = giới hạn số request trên 1 phút
};