#ifndef TOOL_CALL_PARSER_H
#define TOOL_CALL_PARSER_H

#include "../client/ollama_client.h"

#include <nlohmann/json.hpp>
#include <string>

struct ToolCallRequest {
    std::string tool_name;
    std::string args;
    bool is_valid = false;
};

class ToolCallParser {
public:
    static ToolCallRequest parse(const std::string& llm_response);
};

#endif