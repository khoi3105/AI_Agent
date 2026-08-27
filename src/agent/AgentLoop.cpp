#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <format>
#include <iostream>
#include <chrono> // time
#include <stop_token>

#include "AgentLoop.h"
#include "tool_call_parser.h"
#include "../tools/tool_registry.h"

std::expected<std::string, std::string> AgentLoop::run(
    const std::string& user_task, 
    const std::shared_ptr<LLMClient>& client, 
    const std::vector<std::string>& image_paths,
    std::stop_token stop_token) 
{ 
    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
    }

    ToolRegistry registry;
    _conversationHistory.clear();
    _loopdetector.reset();
    
    // Lưu lại lịch sử 
    std::string system_prompt = prepareSystemPrompt(registry, user_task);
    _conversationHistory.push_back({{"role", "system"}, {"content", system_prompt }});
    _conversationHistory.push_back({{"role", "user"}, {"content", user_task}});

    int step = 0;
    ToolCallRequest request;

    // VÒNG LẶP REACT (Observe -> Think -> Act)
    while ( step < AgentLoop::_maxstep ) {  
        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
        }

        step++;
        auto step_start_time = std::chrono::steady_clock::now();

        std::expected<std::string,std::string> llm_response = client->chat(_conversationHistory, image_paths);
        
        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
        }

        if (llm_response.has_value()) {
            std::cout << "========================================================================\n";
            std::cout << std::format("--> Cau tra loi goc tu AI (Buoc {}):\n{}\n", step, *llm_response);
            std::cout << "========================================================================\n";
        } else {
            return std::unexpected(std::format("[ERROR]: Khong nhan phan hoi tu LLM - {} !", llm_response.error()));
        }

        request = parseStepResponse(*llm_response);
        _conversationHistory.push_back({{"role","assistant"},{"content",*llm_response}});

        // Khởi tạo StepData để chứa dữ liệu nhật ký của lượt này
        StepData current_step_data;
        current_step_data.stepNumber = step;
        current_step_data.thought = *llm_response;

        if (request.is_valid && request.tool_name != "null" && !request.tool_name.empty()) {

            current_step_data.actionName = request.tool_name;
            current_step_data.actionArgs = request.args;

            LoopCheckResult _detectLoop = _loopdetector.checkLoop(request.tool_name,request.args);

            if ( _detectLoop.status == LoopStatus::CRITICAL ) {
                return std::unexpected(_detectLoop.message);
            }
            else if ( _detectLoop.status == LoopStatus::WARNING ) {
                _conversationHistory.push_back({
                    {"role","user"},
                    {"content","CANH BAO TU HE THONG: Ban dang goi cung 1 Tool voi cung tham so nhieu lan. Vui long chon cach khac hoac dua ra cau tra loi cuoi cung!"}
                });

                // Tính latency và bắn Hook trước khi continue
                auto step_end_time = std::chrono::steady_clock::now();
                current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();
                current_step_data.observation = "[CẢNH BÁO LẶP TỪ HỆ THỐNG]";
                
                if (_stepHook) {
                    _stepHook(current_step_data);
                }
                continue;
            }

            if (stop_token.stop_requested()) {
                return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
            }

            std::cout << "[Act]: Goi cong cu '" << request.tool_name << "'...\n";

            auto checkToolRegistry = act(registry, request.tool_name, request.args);
            
            observe(request.tool_name, checkToolRegistry, current_step_data);

            auto step_end_time = std::chrono::steady_clock::now();
            current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();
            if (_stepHook) {
                _stepHook(current_step_data);
            }

            continue; 
        }

        current_step_data.actionName = "finish";
        current_step_data.actionArgs = request.args;
        current_step_data.observation = "Completed final answer.";

        auto step_end_time = std::chrono::steady_clock::now();
        current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();

        if (_stepHook) {
            _stepHook(current_step_data);
        }

        break;
    }

    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
    }

    if (step >= AgentLoop::_maxstep) {
        return std::unexpected("[ERROR]: Da dat so buoc toi da!");
    }

    return formatFinalResponse(request);
}

std::string AgentLoop::prepareSystemPrompt(const ToolRegistry& registry, const std::string& user_task){
    nlohmann::json tools_schema = registry.getAllSchemas();

    std::string skills_section = "";
    if (!user_task.empty()) {
        auto skill_res = _skillLoader.selectSkillsForTask(user_task);
        if (skill_res.has_value() && !skill_res.value().empty()) {
            skills_section = "\n\n    === KỸ NĂNG HƯỚNG DẪN CHUYÊN BIỆT ĐƯỢC KÍCH HOẠT (APPLIED SKILLS) ===\n" + skill_res.value() + "\n";
            std::cout << "[SkillLoader] Da kich hoat va nap ky nang cho nhiem vu hien tai.\n";
        }
    }

    return R"(Bạn là một Trợ lý AI hệ thống thông minh, hoạt động theo cơ chế chọn lọc công cụ chính xác.

    === DANH SÁCH CÔNG CỤ ĐƯỢC PHÉP SỬ DỤNG (JSON SCHEMA) ===
    )" + tools_schema.dump(2) + skills_section + R"(

    === QUY TẮC RÀNG BUỘC NGHIÊM NGẶT (CRITICAL RULES) ===
    1. MỖI LƯỢT CHỈ ĐƯỢC TRẢ VỀ DUY NHẤT 01 KHỐI JSON. KHÔNG VIẾT BẤT KỲ LỜI VĂN MỞ ĐẦU HAY GIẢI THÍCH NÀO KHÁC.
    2. KHÔNG TỰ TÍNH TOÁN HAY GIẢ LẬP KẾT QUẢ TOOL. Nếu tác vụ có phép tính hoặc tra cứu, BẮT BUỘC trả về "tool_call" cho bước đó.
    3. Nếu người dùng hỏi nhiều câu: Hãy gọi 01 Tool cho câu hỏi đầu tiên. Sau khi nhận được kết quả (Observation), bạn mới tiếp tục gọi Tool cho câu tiếp theo hoặc tổng hợp câu trả lời và cách nhau bởi định dạng: ```json <...> ```
    
    Bạn cần kiểm tra ý định của người dùng và tuân thủ chặt chẽ 2 định dạng đầu ra sau:

    1. ĐỊNH DẠNG 1: GỌI CÔNG CỤ (Sử dụng khi và chỉ khi câu hỏi yêu cầu thực thi hoặc tính toán liên quan đến các công cụ trong danh sách trên)
    {   
    "type": "tool_call",
    "tool": "<tên_tool_chính_xác_trong_schema>",
    "args": { <các_tham_số_đúng_định_dạng_properties_trong_schema> }
    }

    2. ĐỊNH DẠNG 2: TRẢ LỜI TRỰC TIẾP (Mặc định cho mọi câu hỏi kiến thức, trò chuyện, hoặc khi KHÔNG CÓ công cụ nào phù hợp)
    {
    "type": "response",
    "text": "<nội_dung_trả_lời_dựa_trên_kiến_thức_của_bạn>"
    }

    === RÀNG BUỘC NGHIÊM NGẶT (CRITICAL RULES) ===
    - KHÔNG TỰ TÍNH TOÁN HAY GIẢI BÀI TOÁN. Khi phát hiện phép tính, PHẢI tạo "tool_call" ngay lập tức để Tool xử lý.
    - KHÔNG viết lời mở đầu, KHÔNG giải thích các bước tính toán (ví dụ: "Tôi sẽ dùng calculator...", "20 + 160 = 180...").
    )";
}

ToolCallRequest AgentLoop::parseStepResponse(const std::string& raw_response) {
    return ToolCallParser::parse(raw_response);
}

std::expected<std::string, std::string> AgentLoop::act(ToolRegistry& registry, const std::string& tool_name, const nlohmann::json& args) {
    return registry.execute(tool_name, args);
}

void AgentLoop::observe(const std::string& tool_name, const std::expected<std::string, std::string>& tool_result, StepData& out_step_data) {
    if (tool_result.has_value()) {
        std::string res_str = *tool_result;
        std::cout << "[Observe]: Ket qua Tool: " << res_str << std::endl;
        
        out_step_data.observation = res_str;

        std::string observation_msg = std::format("Ket qua tu cong cu '{}': {}", tool_name, res_str);
        _conversationHistory.push_back({{"role", "user"}, {"content", observation_msg}});
    } else {
        std::string error_msg = tool_result.error();
        out_step_data.observation = "[ERROR]: " + error_msg;

        std::string observation_error = std::format("[ERROR] Thuc thi cong cu '{}' that bai: {}", tool_name, error_msg);
        _conversationHistory.push_back({{"role", "user"}, {"content", observation_error}});
    }
}

std::expected<std::string, std::string> AgentLoop::formatFinalResponse(const ToolCallRequest& request) {
    try {
        if (request.args.is_object() && request.args.contains("text")) {
            return std::format("[4] Tra loi: {}\n", request.args["text"].get<std::string>());
        }
        else if (request.args.is_string()) {
            return std::format("[4] Tra loi: {}\n", request.args.get<std::string>());
        }
    }
    catch (const nlohmann::json::exception& e) {
        return std::unexpected(std::format("[JSON Exception - Output Fallback]: {}", e.what()));
    }

    if (!_conversationHistory.empty()) {
        std::string raw_fallback = _conversationHistory.back()["content"].get<std::string>();
        return std::format("[4] Tra loi: {}\n", raw_fallback);
    }

    return std::unexpected("[ERROR]: Lịch sử hội thoại rỗng, không thể lấy phản hồi.");
}
