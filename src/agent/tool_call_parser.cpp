#include <iostream>
using namespace std;

#include "tool_call_parser.h"

std::string ToolCallParser::extract_json(const std::string& text) {
    // 1. Ưu tiên tìm khối mã ```json ... ```
    size_t start_code = text.find("```json");
    if (start_code != std::string::npos) {
        start_code += 7;
        size_t end_code = text.find("```", start_code);
        if (end_code != std::string::npos) {
            return text.substr(start_code, end_code - start_code);
        }
    }

    // 2. Tìm Object JSON đầu tiên bằng thuật toán đếm ngoặc {}
    size_t first_brace = text.find('{');
    if (first_brace == std::string::npos) {
        return text;
    }

    int brace_count = 0;
    for (size_t i = first_brace; i < text.length(); ++i) {
        if (text[i] == '{') {
            brace_count++;
        } else if (text[i] == '}') {
            brace_count--;
            if (brace_count == 0) {
                // Đã tìm thấy dấu } đóng khớp hoàn toàn với dấu { đầu tiên
                return text.substr(first_brace, i - first_brace + 1);
            }
        }
    }

    return text;
}

ToolCallRequest ToolCallParser::parse(const std::string& llm_response) {
    ToolCallRequest req;
    req.is_valid = false;
    
    std::string clean_json = extract_json(llm_response);

    try{
        auto j = nlohmann::json::parse(clean_json);
        
        if ((j.contains("type") && j["type"] == "tool_call") || (j.contains("type") && j["type"] == "function")) {
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