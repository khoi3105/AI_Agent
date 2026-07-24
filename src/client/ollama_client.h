#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include "llm_client.h"
#include <string>

class OLLAMAClient : public LLMClient {
private:
    std::string _modelName;
    std::string _baseURL;
public:
    std::string chat(std::string prompt) override;
};
#endif