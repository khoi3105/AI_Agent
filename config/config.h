#pragma once

#include "llm_config.h"
#include "tool_config.h"
#include "agent_config.h"
#include "http_config.h"

class Config {
private:
    inline static Config* _instance = nullptr;
    LLMConfig _llm;
    ToolConfig _tool;
    AgentConfig _agent;
    HttpConfig _http;
private:
    Config();

public:
    static Config* instance();

    // Get config
    const LLMConfig& llm() const;
    const ToolConfig& tool() const;
    const AgentConfig& agent() const;
    const HttpConfig& http() const;
};