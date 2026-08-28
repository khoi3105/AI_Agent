#include "task_plan.h"
#include "tool_call_parser.h"
#include <format>
#include <sstream>
#include <iostream>

TaskPlan TaskPlan::parse(const std::string& raw_response) {
    TaskPlan plan;
    
    // 1. Cố gắng trích xuất khối JSON từ phản hồi của LLM
    auto json_blocks = ToolCallParser::extract_all_jsons(raw_response);
    
    for (const auto& json_str : json_blocks) {
        try {
            auto j = nlohmann::json::parse(json_str);
            
            if (j.contains("type") && j["type"] == "plan") {
                plan._goal = j.value("goal", "");
                plan._reasoning = j.value("reasoning", "");
                
                if (j.contains("steps") && j["steps"].is_array()) {
                    int idx = 1;
                    for (const auto& item : j["steps"]) {
                        PlanStep step;
                        step.stepIndex = idx++;
                        if (item.is_string()) {
                            step.description = item.get<std::string>();
                            step.suggestedTool = "auto";
                        } else if (item.is_object()) {
                            step.description = item.value("description", "");
                            step.suggestedTool = item.value("tool", "auto");
                        }
                        plan._steps.push_back(step);
                    }
                }
                plan._isValid = true;
                return plan;
            }
        } catch (const nlohmann::json::exception& e) {
            // Tiếp tục thử khối JSON tiếp theo nếu có lỗi parse
        }
    }

    // 2. Fallback: Nếu LLM không trả về đúng JSON plan, coi toàn bộ text là reasoning
    plan._goal = "Thực hiện yêu cầu của người dùng";
    plan._reasoning = raw_response;
    plan._steps.push_back(PlanStep{1, "Phân tích và thực thi trực tiếp qua các công cụ phù hợp", "auto", false});
    plan._isValid = !raw_response.empty();
    return plan;
}

std::string TaskPlan::toPromptContext() const {
    std::ostringstream oss;
    oss << "\n=== KẾ HOẠCH HÀNH ĐỘNG ĐÃ ĐƯỢC THIẾT LẬP (TASK EXECUTION PLAN) ===\n";
    oss << "Mục tiêu: " << _goal << "\n";
    oss << "Phân tích chiến lược: " << _reasoning << "\n";
    oss << "Các bước cần thực hiện:\n";
    for (const auto& step : _steps) {
        oss << std::format("  [Bước {}]: {} (Công cụ dự kiến: {})\n", 
                           step.stepIndex, step.description, step.suggestedTool);
    }
    oss << "\nQUY TẮC: Hãy bám sát kế hoạch trên. Bắt đầu bằng việc gọi công cụ cho bước đầu tiên.\n";
    return oss.str();
}

nlohmann::json TaskPlan::toJson() const {
    nlohmann::json j;
    j["goal"] = _goal;
    j["reasoning"] = _reasoning;
    j["is_valid"] = _isValid;
    j["steps"] = nlohmann::json::array();
    for (const auto& step : _steps) {
        j["steps"].push_back(step.toJson());
    }
    return j;
}

std::string TaskPlan::toString() const {
    std::ostringstream oss;
    oss << "[TaskPlan]\n";
    oss << "  • Mục tiêu: " << _goal << "\n";
    oss << "  • Chiến lược: " << _reasoning << "\n";
    oss << "  • Kế hoạch chi tiết:\n";
    for (const auto& step : _steps) {
        oss << std::format("    {}. {} [Tool: {}]\n", step.stepIndex, step.description, step.suggestedTool);
    }
    return oss.str();
}
