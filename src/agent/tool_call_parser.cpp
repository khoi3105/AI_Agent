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

            // 1. Kiểm tra nếu là response trực tiếp từ AI
            if (j.contains("type") && j["type"] == "response") {
                req.tool_name = "null";
                req.args = j;
                req.is_valid = false;
                requests.push_back(req);
                continue;
            }

            // 2. Trích xuất tên tool linh hoạt (hỗ trợ cả tool, name, tool_name, function.name)
            std::string tool_name = "";
            nlohmann::json args = nlohmann::json::object();

            if (j.contains("tool") && j["tool"].is_string()) {
                tool_name = j["tool"].get<std::string>();
            } else if (j.contains("name") && j["name"].is_string() && j.value("type", "") != "plan") {
                tool_name = j["name"].get<std::string>();
            } else if (j.contains("tool_name") && j["tool_name"].is_string()) {
                tool_name = j["tool_name"].get<std::string>();
            } else if (j.contains("function") && j["function"].is_object()) {
                if (j["function"].contains("name") && j["function"]["name"].is_string()) {
                    tool_name = j["function"]["name"].get<std::string>();
                }
                if (j["function"].contains("arguments")) {
                    args = j["function"]["arguments"];
                } else if (j["function"].contains("parameters")) {
                    args = j["function"]["parameters"];
                }
            }

            // 3. Trích xuất tham số linh hoạt (args, parameters, arguments)
            if (args.empty() || !args.is_object()) {
                if (j.contains("args")) {
                    args = j["args"];
                } else if (j.contains("parameters")) {
                    args = j["parameters"];
                } else if (j.contains("arguments")) {
                    args = j["arguments"];
                }
            }

            // 4. Nếu args là chuỗi (string), xử lý giải mã JSON con hoặc Auto-Wrap
            if (args.is_string()) {
                try {
                    // Thử parse nếu là chuỗi bị stringify (VD: "{\"expression\": \"15 * 17\"}")
                    args = nlohmann::json::parse(args.get<std::string>());
                } catch (const nlohmann::json::parse_error& e) {
                    // Nếu KHÔNG PHẢI JSON con mà là chuỗi thô (VD: "15 * 17" hoặc "ls -la")
                    // -> Tự động bọc (Auto-Wrap) thành Object tương ứng với Tool đó
                    std::string raw_str = args.get<std::string>();
                    if (tool_name == "calculator") {
                        args = { {"expression", raw_str} };
                    } else if (tool_name == "exec") {
                        args = { {"command", raw_str} };
                    } else if (tool_name == "read_file") {
                        args = { {"path", raw_str} };
                    } else if (tool_name == "memory_search") {
                        args = { {"query", raw_str} };
                    }
                }
            }

            // 5. Xác thực tên Tool hợp lệ
            if (!tool_name.empty() && tool_name != "null" && tool_name != "response" && tool_name != "plan") {
                req.tool_name = tool_name;
                req.args = args.is_object() ? args : nlohmann::json::object();
                req.is_valid = true;
                requests.push_back(req);
            }
        } catch (const nlohmann::json::exception& e) {
            req.is_valid = false;
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
