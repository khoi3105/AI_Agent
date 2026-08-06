#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>
#include <nlohmann/json.hpp>
#include <expected>
#include <vector>

class LLMClient {
public:
    virtual std::expected<std::string, std::string> chat(nlohmann::json& message, const std::vector<std::string>& image_paths = {}) = 0;
    virtual ~LLMClient() = default;
};

#endif