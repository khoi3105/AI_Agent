#include "config.h"
#include "config_loader.h"

#include <string>

Config::Config() {
    auto data = ConfigLoader::load("config.ini");

    // =========================
    // LLM CONFIG
    // =========================

    if (data.contains("llm.BASE_URL")) _llm.baseUrl = data.at("llm.BASE_URL");
    else if (data.contains("llm.base_url")) _llm.baseUrl = data.at("llm.base_url");

    if (data.contains("llm.MODEL")) _llm.model = data.at("llm.MODEL");
    else if (data.contains("llm.model")) _llm.model = data.at("llm.model");

    if (data.contains("llm.TEMPERATURE")) _llm.temperature = std::stod(data.at("llm.TEMPERATURE"));
    else if (data.contains("llm.temperature")) _llm.temperature = std::stod(data.at("llm.temperature"));

    if (data.contains("llm.TOP_P")) _llm.topP = std::stod(data.at("llm.TOP_P"));
    else if (data.contains("llm.top_p")) _llm.topP = std::stod(data.at("llm.top_p"));

    if (data.contains("llm.MAX_TOKENS")) _llm.maxTokens = std::stoi(data.at("llm.MAX_TOKENS"));
    else if (data.contains("llm.max_tokens")) _llm.maxTokens = std::stoi(data.at("llm.max_tokens"));

    if (data.contains("llm.STREAM")) _llm.stream = (data.at("llm.STREAM") == "true");
    else if (data.contains("llm.stream")) _llm.stream = (data.at("llm.stream") == "true");

    if (data.contains("llm.RPM")) _llm.rpm = std::stoi(data.at("llm.RPM"));
    else if (data.contains("llm.rpm")) _llm.rpm = std::stoi(data.at("llm.rpm"));

    // =========================
    // MULTI-AGENT WORKER CONFIG
    // =========================

    if (data.contains("multi.WORKER_BASE_URL")) _multi.workerBaseUrl = data.at("multi.WORKER_BASE_URL");
    else if (data.contains("multi.worker_base_url")) _multi.workerBaseUrl = data.at("multi.worker_base_url");

    if (data.contains("multi.WORKER_MODEL")) _multi.workerModel = data.at("multi.WORKER_MODEL");
    else if (data.contains("multi.worker_model")) _multi.workerModel = data.at("multi.worker_model");

    if (data.contains("multi.WORKER_API_KEY")) _multi.workerApiKey = data.at("multi.WORKER_API_KEY");
    else if (data.contains("multi.worker_api_key")) _multi.workerApiKey = data.at("multi.worker_api_key");

    if (data.contains("multi.WORKER_TEMPERATURE") && !data.at("multi.WORKER_TEMPERATURE").empty()) 
        _multi.workerTemperature = std::stod(data.at("multi.WORKER_TEMPERATURE"));
    else if (data.contains("multi.worker_temperature") && !data.at("multi.worker_temperature").empty()) 
        _multi.workerTemperature = std::stod(data.at("multi.worker_temperature"));

    if (data.contains("multi.WORKER_TOP_P") && !data.at("multi.WORKER_TOP_P").empty()) 
        _multi.workerTopP = std::stod(data.at("multi.WORKER_TOP_P"));
    else if (data.contains("multi.worker_top_p") && !data.at("multi.worker_top_p").empty()) 
        _multi.workerTopP = std::stod(data.at("multi.worker_top_p"));

    if (data.contains("multi.WORKER_MAX_TOKENS") && !data.at("multi.WORKER_MAX_TOKENS").empty()) 
        _multi.workerMaxTokens = std::stoi(data.at("multi.WORKER_MAX_TOKENS"));
    else if (data.contains("multi.worker_max_tokens") && !data.at("multi.worker_max_tokens").empty()) 
        _multi.workerMaxTokens = std::stoi(data.at("multi.worker_max_tokens"));

    if (data.contains("multi.WORKER_RPM") && !data.at("multi.WORKER_RPM").empty()) 
        _multi.workerRpm = std::stoi(data.at("multi.WORKER_RPM"));
    else if (data.contains("multi.worker_rpm") && !data.at("multi.worker_rpm").empty()) 
        _multi.workerRpm = std::stoi(data.at("multi.worker_rpm"));

    // =========================
    // TOOL CONFIG
    // =========================

    if (data.contains("tool.MEMORY")) _tool.memoryTool = data.at("tool.MEMORY");
    else if (data.contains("tool.memory")) _tool.memoryTool = data.at("tool.memory");

    // =========================
    // AGENT CONFIG
    // =========================
    
    if (data.contains("agent.MAX_STEP")) _agent.maxStep = std::stoi(data.at("agent.MAX_STEP"));
    else if (data.contains("agent.max_step")) _agent.maxStep = std::stoi(data.at("agent.max_step"));

    // =========================
    // HTTP CONFIG
    // =========================
    if (data.contains("http.POST_TIMEOUT")) _http.postTimeout = std::stol(data.at("http.POST_TIMEOUT"));
    else if (data.contains("http.post_timeout")) _http.postTimeout = std::stol(data.at("http.post_timeout"));

    if (data.contains("http.GET_TIMEOUT")) _http.getTimeout = std::stol(data.at("http.GET_TIMEOUT"));
    else if (data.contains("http.get_timeout")) _http.getTimeout = std::stol(data.at("http.get_timeout"));

    if (data.contains("http.RPM")) _http.rpm = std::stoi(data.at("http.RPM"));
    else if (data.contains("http.rpm")) _http.rpm = std::stoi(data.at("http.rpm"));
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

const MultiAgentConfig& Config::multi() const
{
    return _multi;
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