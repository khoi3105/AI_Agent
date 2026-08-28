#ifndef TOOL_CALL_PARSER_H
#define TOOL_CALL_PARSER_H

#include "../client/ollama_client.h"
#include "../tools/tool_registry.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ToolCallRequest {
    std::string tool_name;
    nlohmann::json args;
    bool is_valid = false;
};

class ToolCallParser {
public:
    static std::vector<std::string> extract_all_jsons(const std::string& text);
    static std::vector<ToolCallRequest> parse_all(const std::string& llm_response);
    static ToolCallRequest parse(const std::string& llm_response);
    ~ToolCallParser() = default;
};

#endif // TOOL_CALL_PARSER_H
