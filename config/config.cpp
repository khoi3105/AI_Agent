#include "config.h"
#include "config_loader.h"

#include <string>

Config::Config() {
    auto data = ConfigLoader::load("config.txt");

    // =========================
    // LLM CONFIG
    // =========================

    if (data.contains("llm.base_url")) _llm.baseUrl = data.at("llm.base_url");

    if (data.contains("llm.model")) _llm.model = data.at("llm.model");

    if (data.contains("llm.temperature")) _llm.temperature = std::stod(data.at("llm.temperature"));

    if (data.contains("llm.top_p")) _llm.topP = std::stod(data.at("llm.top_p"));

    if (data.contains("llm.max_tokens")) _llm.maxTokens = std::stoi(data.at("llm.max_tokens"));

    if (data.contains("llm.stream")) _llm.stream = (data.at("llm.stream") == "true");

    // =========================
    // TOOL CONFIG
    // =========================

    if (data.contains("tool.memory")) _tool.memoryTool = data.at("tool.memory");

    // =========================
    // AGENT CONFIG
    // =========================
    
    if (data.contains("agent.max_step")) _agent.maxStep = std::stoi(data.at("agent.max_step"));

    // =========================
    // HTTP CONFIG
    // =========================
    if (data.contains("http.post_timeout")) _http.postTimeout = std::stol(data.at("http.post_timeout"));

    if (data.contains("http.get_timeout")) _http.getTimeout = std::stol(data.at("http.get_timeout"));
}

// =========================
// SINGLETON
// =========================

Config* Config::instance() {
    if (_instance == nullptr){
        _instance = new Config();
    }
    return _instance;
}


// =========================
// GETTERS
// =========================

const LLMConfig& Config::llm() const
{
    return _llm;
}

const ToolConfig& Config::tool() const
{
    return _tool;
}

const AgentConfig& Config::agent() const
{
    return _agent;
}

const HttpConfig& Config::http() const
{
    return _http;
}