#include <iostream>
using namespace std;

#include "tool_call_parser.h"

ToolCallRequest ToolCallParser::parse(const std::string& llm_response) {
    ToolCallRequest req;
    req.is_valid = false;

    try{
        auto j = nlohmann::json::parse(llm_response);
        auto& msg = j["choices"][0]["message"];

        if (msg.contains("tool_calls") && !msg["tool_calls"].is_null()) {
            auto& func = msg["tool_calls"][0]["function"];
            req.tool_name = func["name"].get<std::string>();
            
            // Parse chuỗi stringify "arguments" thành JSON object
            std::string args_str = func["arguments"].get<std::string>();
            req.args = nlohmann::json::parse(args_str);
            
            req.is_valid = true;
        }
        else if (msg.contains("content") && !msg["content"].is_null()) {
            req.tool_name = "null";
            req.args = msg["content"].get<std::string>(); // Lưu tạm response văn bản
            req.is_valid = false; // Không phải tool call
        }
    }
    catch ( const nlohmann::json::exception& e ) {
        req.is_valid = false;
    }

    return req;
}