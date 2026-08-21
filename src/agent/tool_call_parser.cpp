#include <iostream>

#include "tool_call_parser.h"

std::vector<std::string> ToolCallParser::extract_all_jsons(const std::string& text) {
    std::vector<std::string> json_blocks;
    
    // Sử dụng thuật toán đếm ngoặc {} để trích xuất TẤT CẢ các JSON Object xuất hiện trong văn bản
    int brace_count = 0;
    size_t start_index = std::string::npos;

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '{') {
            if (brace_count == 0) {
                start_index = i; // Bắt đầu 1 JSON object mới
            }
            brace_count++;
        } else if (text[i] == '}') {
            if (brace_count > 0) {
                brace_count--;
                if (brace_count == 0 && start_index != std::string::npos) {
                    // Đã tìm thấy 1 JSON object hoàn chỉnh
                    json_blocks.push_back(text.substr(start_index, i - start_index + 1));
                    start_index = std::string::npos;
                }
            }
        }
    }

    return json_blocks;
}

std::vector<ToolCallRequest> ToolCallParser::parse_all(const std::string& llm_response) {
    std::vector<ToolCallRequest> requests;
    auto raw_jsons = extract_all_jsons(llm_response);

    for (const auto& json_str : raw_jsons) {
        ToolCallRequest req;
        req.is_valid = false;

        try {
            auto j = nlohmann::json::parse(json_str);

            if ((j.contains("type") && j["type"] == "tool_call") || (j.contains("type") && j["type"] == "function")) {
                req.tool_name = j.value("tool", "");
                req.args = j.value("args", nlohmann::json::object());
                req.is_valid = true;
            } else if (j.contains("type") && j["type"] == "response") {
                req.tool_name = "null";
                req.args = j;
                req.is_valid = false;
            }
        } catch (const nlohmann::json::exception& e) {
            req.is_valid = false;
        }

        if (req.is_valid || req.tool_name == "null") {
            requests.push_back(req);
        }
    }

    return requests;
}

ToolCallRequest ToolCallParser::parse(const std::string& llm_response) {
    auto requests = parse_all(llm_response);
    if (!requests.empty()) {
        return requests[0]; // Trả về request đầu tiên nếu dùng hàm đơn lẻ
    }
    
    ToolCallRequest invalid_req;
    invalid_req.is_valid = false;
    return invalid_req;
}