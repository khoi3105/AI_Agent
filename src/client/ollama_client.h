#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include "llm_client.h"
#include <string>

class OllamaClient : public LLMClient {
private:
    std::string _modelName;
    std::string _baseURL;
    std::string _APIKey;
public:
    OllamaClient(std::string modelName, std::string baseURL, std::string APIKey): _modelName(modelName), _baseURL (baseURL), _APIKey(APIKey) {}
    std::expected<std::string, std::string> chat(const std::string& user_prompt, const std::vector<std::string>& image_paths = {}) override;
    ~OllamaClient() override = default;
};
#endif