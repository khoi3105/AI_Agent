#include <iostream>
using namespace std;

#include "tool_call_parser.h"

ToolCallRequest ToolCallParser::parse(const std::string& llm_response) {
    ToolCallRequest req;
    req.is_valid = false;

    try{
        auto j = nlohmann::json::parse(llm_response);
        
        if (j.contains("type") && j["type"] == "tool_call") {
            req.tool_name = j["tool"].get<std::string>();
            req.args = j.value("args",nlohmann::json::object()); 
            req.is_valid = true;
        }
        else if (j.contains("type") && j["type"] == "response") {
            req.tool_name = "null";
            req.args = j; // Lưu tạm response văn bản
            req.is_valid = false; // Không phải tool call
        }
    }
    catch ( const nlohmann::json::exception& e ) {
        req.is_valid = false;
    }

    return req;
}