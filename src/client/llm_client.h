#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>
#include <nlohmann/json.hpp>
#include <expected>
#include <vector>

class LLMClient {
protected:
    int _lastTokensUsed{0};

public:
    virtual std::expected<std::string, std::string> chat(nlohmann::json& message, const std::vector<std::string>& image_paths = {}) = 0;
    virtual int getLastTokensUsed() const { return _lastTokensUsed; }
    virtual void setLastTokensUsed(int tokens) { _lastTokensUsed = tokens; }
    virtual ~LLMClient() = default;
};

#endif