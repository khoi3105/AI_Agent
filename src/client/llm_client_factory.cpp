#include "llm_client_factory.h"
#include "ollama_client.h"
#include "gemini_client.h"
#include <algorithm>

static std::string toLower(std::string s) {
    for (auto& c : s) c = std::tolower(c);
    return s;
}

std::shared_ptr<LLMClient> LLMClientFactory::createClient(
    const std::string& model,
    const std::string& baseURL,
    const std::string& apiKey
) {
    std::string lowerModel = toLower(model);
    std::string lowerURL = toLower(baseURL);

    // Kiểm tra nếu là mô hình Gemini hoặc endpoint Google API
    if (lowerModel.find("gemini") != std::string::npos || lowerURL.find("generativelanguage.googleapis.com") != std::string::npos) {
        return std::make_shared<GeminiClient>(model, baseURL, apiKey);
    }

    // Mặc định: Dùng Ollama/OpenAI/NIM client
    return std::make_shared<OllamaClient>(model, baseURL, apiKey);
}
