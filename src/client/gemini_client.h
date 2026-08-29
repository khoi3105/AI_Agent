#ifndef GEMINI_CLIENT_H
#define GEMINI_CLIENT_H

#include "llm_client.h"
#include <string>
#include <vector>
#include <expected>
#include <nlohmann/json.hpp>

/**
 * @brief LLMClient implementation for Google Gemini models (Gemini 2.0 Flash, Gemini 1.5 Pro, etc.)
 * Supports both:
 * 1. Native Google AI Studio REST API (`generateContent`)
 * 2. Google AI Studio OpenAI-compatible endpoint (`/v1beta/openai/chat/completions`)
 */
class GeminiClient : public LLMClient {
private:
    std::string _modelName;
    std::string _baseURL;
    std::string _APIKey;

    std::string getMimeType(const std::string& filepath);
    
    // Gửi qua Native Gemini generateContent API
    std::expected<std::string, std::string> chatNative(nlohmann::json& message, const std::vector<std::string>& image_paths);
    
    // Gửi qua OpenAI-compatible endpoint
    std::expected<std::string, std::string> chatOpenAICompatible(nlohmann::json& message, const std::vector<std::string>& image_paths);

public:
    GeminiClient(std::string modelName, std::string baseURL = "", std::string APIKey = "");
    
    std::expected<std::string, std::string> chat(nlohmann::json& message, const std::vector<std::string>& image_paths = {}) override;
    
    ~GeminiClient() override = default;
};

#endif // GEMINI_CLIENT_H
