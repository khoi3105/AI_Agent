#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <format>
#include <iostream>
#include <print>
#include <ranges>
#include <chrono>
#include <stop_token>

#include "AgentLoop.h"
#include "tool_call_parser.h"
#include "../tools/tool_registry.h"
#include "../tools/tool_policy.h"
#include "../environment/native_environment.h"

AgentLoop::AgentLoop(
    int max_steps, 
    std::string skills_dir,
    std::shared_ptr<ToolRegistry> registry,
    std::shared_ptr<ToolPolicy> policy,
    std::shared_ptr<Environment> env)
    : _maxstep(max_steps), 
      _skillLoader(std::move(skills_dir)), 
      _conversationHistory(nlohmann::json::array()),
      _registry(std::move(registry)),
      _toolPolicy(std::move(policy)),
      _env(std::move(env))
{
    if (!_env) {
        _env = std::make_shared<NativeEnvironment>();
    }
    if (!_registry) {
        _registry = std::make_shared<ToolRegistry>();
    }
    if (!_toolPolicy) {
        _toolPolicy = std::make_shared<ToolPolicy>();
    }
}

std::string AgentLoop::preparePlanningPrompt(const ToolRegistry& registry, const std::string& user_task) {
    nlohmann::json tools_schema = registry.getAllSchemas();

    std::string skills_section = "";
    if (!user_task.empty()) {
        auto skill_res = _skillLoader.selectSkillsForTask(user_task);
        if (skill_res.has_value() && !skill_res.value().empty()) {
            skills_section = "\n\n    === KỸ NĂNG HƯỚNG DẪN CHUYÊN BIỆT (APPLIED SKILLS) ===\n" + skill_res.value() + "\n";
        }
    }

    return R"(Bạn là một Chuyên gia Lập kế hoạch AI (AI Task Planner).
Nhiệm vụ của bạn là phân tích yêu cầu của người dùng, suy nghĩ chiến lược và lập kế hoạch các bước thực hiện tuần tự.

=== CÁC CÔNG CỤ HIỆN CÓ ĐỂ SỬ DỤNG ===
)" + tools_schema.dump(2) + skills_section + R"(

=== QUY TẮC PHẢN HỒI (BẮT BUỘC) ===
QUY TẮC CỐT LÕI: Mỗi lượt CHỈ ĐƯỢC sinh DUY NHẤT 01 khối JSON công cụ cho bước hiện tại. TUYỆT ĐỐI KHÔNG tự giả lập kết quả trả về của tool,    
KHÔNG viết nhiều tool call trong cùng 1 lượt. Phải dừng lại đợi hệ thống trả về kết quả quan sát (Observation) rồi mới thực hiện bước tiếp theo.
Bạn PHẢI trả về DUY NHẤT 01 khối JSON có cấu trúc chính xác như sau, KHÔNG viết bất kỳ lời dẫn nào khác:
```json
{
  "type": "plan",
  "goal": "<Mục tiêu chính xác của nhiệm vụ>",
  "reasoning": "<Phân tích yêu cầu, tư duy chiến lược và cách tiếp cận bài toán>",
  "steps": [
    {
      "description": "<Mô tả chi tiết bước 1 cần làm>",
      "tool": "<Tên công cụ dự kiến dùng hoặc 'none'>"
    },
    {
      "description": "<Mô tả chi tiết bước 2 cần làm>",
      "tool": "<Tên công cụ dự kiến dùng hoặc 'none'>"
    }
  ]
}
```
)";
}

std::expected<TaskPlan, std::string> AgentLoop::plan(
    const std::string& user_task,
    const std::shared_ptr<LLMClient>& client,
    const std::vector<std::string>& image_paths,
    std::stop_token stop_token)
{
    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac Timeout truoc khi lap ke hoach!");
    }

    std::string plan_system_prompt = preparePlanningPrompt(*_registry, user_task);
    nlohmann::json plan_history = nlohmann::json::array();
    plan_history.push_back({{"role", "system"}, {"content", plan_system_prompt}});
    plan_history.push_back({{"role", "user"}, {"content", user_task}});

    std::println("[Planner]: Dang suy nghi va lap ke hoach (TaskPlan) cho nhiem vu...");
    auto plan_response = client->chat(plan_history, image_paths);
    
    if (!plan_response.has_value()) {
        return std::unexpected("[Planner ERROR]: Khong the lay ke hoach tu LLM: " + plan_response.error());
    }

    std::println("========================================================================");
    std::println("--> Ke hoach goc tu AI Planner:\n{}", *plan_response);
    std::println("========================================================================");

    TaskPlan task_plan = TaskPlan::parse(*plan_response);
    std::println("{}", task_plan.toString());
    return task_plan;
}

std::expected<std::string, std::string> AgentLoop::run(
    const std::string& user_task, 
    const std::shared_ptr<LLMClient>& client, 
    const std::vector<std::string>& image_paths,
    std::stop_token stop_token) 
{ 
    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
    }

    _conversationHistory.clear();
    _loopdetector.reset();

    // =========================================================================
    // VÒNG 0: SUY NGHĨ & LẬP KẾ HOẠCH (TASKPLAN)
    // =========================================================================
    std::string plan_context = "";
    if (_enablePlanning) {
        auto plan_start_time = std::chrono::steady_clock::now();
        auto plan_result = plan(user_task, client, image_paths, stop_token);
        auto plan_end_time = std::chrono::steady_clock::now();

        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
        }

        if (plan_result.has_value() && plan_result->isValid()) {
            plan_context = plan_result->toPromptContext();

            // Ghi nhận vòng suy nghĩ vào StepData & kích hoạt Observer Hook
            StepData plan_step;
            plan_step.stepNumber = 0;
            plan_step.thought = plan_result->getReasoning();
            plan_step.actionName = "planning";
            plan_step.actionArgs = plan_result->toJson();
            plan_step.observation = plan_result->toString();
            plan_step.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(plan_end_time - plan_start_time).count();
            plan_step.tokensUsed = client->getLastTokensUsed();

            if (_stepHook) {
                _stepHook(plan_step);
            }
        }
    }

    // =========================================================================
    // KHỞI TẠO LỊCH SỬ HỘI THOẠI CHO VÒNG LẶP REACT
    // =========================================================================
    std::string system_prompt = prepareSystemPrompt(*_registry, user_task);
    _conversationHistory.push_back({{"role", "system"}, {"content", system_prompt}});

    std::string initial_user_content = user_task;
    if (!plan_context.empty()) {
        initial_user_content += "\n" + plan_context;
    }
    _conversationHistory.push_back({{"role", "user"}, {"content", initial_user_content}});

    int step = 0;
    ToolCallRequest request;

    // VÒNG LẶP REACT (Observe -> Think -> Act)
    while (step < _maxstep) {  
        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
        }

        step++;
        auto step_start_time = std::chrono::steady_clock::now();

        // Chỉ truyền image_paths ở turn 1 nếu chưa lập plan trước đó để tối ưu token
        std::vector<std::string> current_images = (step == 1 && !_enablePlanning) ? image_paths : std::vector<std::string>{};
        std::expected<std::string, std::string> llm_response = client->chat(_conversationHistory, current_images);
        int step_tokens = client->getLastTokensUsed();
        
        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
        }

        if (llm_response.has_value()) {
            std::println("========================================================================");
            std::println("--> Cau tra loi goc tu AI (Buoc {}):\n{}", step, *llm_response);
            std::println("========================================================================");
        } else {
            return std::unexpected(std::format("[ERROR]: Khong nhan phan hoi tu LLM - {} !", llm_response.error()));
        }

        request = parseStepResponse(*llm_response);
        _conversationHistory.push_back({{"role", "assistant"}, {"content", *llm_response}});

        // Khởi tạo StepData để chứa dữ liệu nhật ký của lượt này
        StepData current_step_data;
        current_step_data.stepNumber = step;
        current_step_data.thought = *llm_response;
        current_step_data.tokensUsed = step_tokens;

        if (request.is_valid && request.tool_name != "null" && !request.tool_name.empty()) {
            current_step_data.actionName = request.tool_name;
            current_step_data.actionArgs = request.args;

            LoopCheckResult detectLoop = _loopdetector.checkLoop(request.tool_name, request.args);

            if (detectLoop.status == LoopStatus::CRITICAL) {
                return std::unexpected(detectLoop.message);
            } else if (detectLoop.status == LoopStatus::WARNING) {
                _conversationHistory.push_back({
                    {"role", "user"},
                    {"content", "CANH BAO TU HE THONG: Ban dang goi cung 1 Tool voi cung tham so nhieu lan. Vui long chon cach khac hoac dua ra cau tra loi cuoi cung!"}
                });

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

            std::println("[Act]: Goi cong cu '{}'...", request.tool_name);

            // ================================
            // TOOL POLICY
            // ================================
            auto policyResult = _toolPolicy->validate(
                request.tool_name,
                request.args
            );

            if (!policyResult) {
                std::println("[ToolPolicy] BLOCKED: {}", policyResult.error());

                current_step_data.observation = "[POLICY BLOCKED] " + policyResult.error();

                _conversationHistory.push_back({
                    {"role", "user"},
                    {"content",
                        "[TOOL POLICY] Tool call bị từ chối: " +
                        policyResult.error() +
                        "\nHãy chọn hành động khác."
                    }
                });

                auto step_end_time = std::chrono::steady_clock::now();
                current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();

                if (_stepHook) {
                    _stepHook(current_step_data);
                }

                continue;
            }

            auto checkToolRegistry = act(*_registry, request.tool_name, request.args);
            
            observe(request.tool_name, checkToolRegistry, current_step_data);

            auto step_end_time = std::chrono::steady_clock::now();
            current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();
            if (_stepHook) {
                _stepHook(current_step_data);
            }

            continue; 
        }

        // =====================================================================
        // AI HOÀN THÀNH CÂU TRẢ LỜI (FINISH / DIRECT RESPONSE)
        // =====================================================================
        current_step_data.actionName = "finish";
        current_step_data.actionArgs = request.args;
        current_step_data.observation = "Completed final answer.";

        auto step_end_time = std::chrono::steady_clock::now();
        current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();

        if (_stepHook) {
            _stepHook(current_step_data);
        }

        // Trả về kết quả thành công ngay khi AI hoàn tất
        return formatFinalResponse(request);
    }

    // Kiểm tra Timeout / Cancel
    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac da qua thoi gian cho (Timeout)!");
    }

    // =========================================================================
    // GRACEFUL DEGRADATION: KHI HẾT BƯỚC (MAX STEPS REACHED)
    // =========================================================================
    std::println("[AgentLoop Warning]: Đã đạt số bước tối đa ({}). Tổng hợp kết quả tốt nhất hiện có...", _maxstep);
    return formatFinalResponse(request);
}

std::string AgentLoop::prepareSystemPrompt(const ToolRegistry& registry, const std::string& user_task) {
    nlohmann::json tools_schema = registry.getAllSchemas();

    std::string skills_section = "";
    if (!user_task.empty()) {
        auto skill_res = _skillLoader.selectSkillsForTask(user_task);
        if (skill_res.has_value() && !skill_res.value().empty()) {
            skills_section = "\n\n    === KỸ NĂNG HƯỚNG DẪN CHUYÊN BIỆT ĐƯỢC KÍCH HOẠT (APPLIED SKILLS) ===\n" + skill_res.value() + "\n";
            std::println("[SkillLoader] Da kich hoat va nap ky nang cho nhiem vu hien tai.");
        }
    }

    return R"(Bạn là một Trợ lý AI hệ thống thông minh, hoạt động theo cơ chế chọn lọc công cụ chính xác.

    === DANH SÁCH CÔNG CỤ ĐƯỢC PHÉP SỬ DỤNG (JSON SCHEMA) ===
    )" + tools_schema.dump(2) + skills_section + R"(

    === QUY TẮC RÀNG BUỘC NGHIÊM NGẶT (CRITICAL RULES) ===
    1. MỖI LƯỢT CHỈ ĐƯỢC TRẢ VỀ DUY NHẤT 01 KHỐI JSON. KHÔNG VIẾT BẤT KỲ LỜI VĂN MỞ ĐẦU HAY GIẢI THÍCH NÀO KHÁC.
    2. KHÔNG TỰ TÍNH TOÁN HAY GIẢ LẬP KẾT QUẢ TOOL. Nếu tác vụ có phép tính hoặc tra cứu, BẮT BUỘC trả về "tool_call" cho bước đó.
    3. Nếu người dùng hỏi nhiều câu hoặc có nhiều bước: Hãy gọi 01 Tool cho bước hiện tại. Sau khi nhận được kết quả (Observation), bạn mới tiếp tục gọi Tool cho bước tiếp theo hoặc tổng hợp câu trả lời và cách nhau bởi định dạng: ```json <...> ```
    
    Bạn cần kiểm tra ý định của người dùng và tuân thủ chặt chẽ 2 định dạng đầu ra sau:

    1. ĐỊNH DẠNG 1: GỌI CÔNG CỤ (Sử dụng khi và chỉ khi câu hỏi yêu cầu thực thi hoặc tính toán liên quan đến các công cụ trong danh sách trên)
    {   
    "type": "tool_call",
    "tool": "<tên_tool_chính_xác_trong_schema>",
    "args": { <các_tham_số_đúng_định_dạng_properties_trong_schema> }
    }

    2. ĐỊNH DẠNG 2: TRẢ LỜI TRỰC TIẾP (Mặc định cho mọi câu hỏi kiến thức, trò chuyện, hoặc khi hoàn thành nhiệm vụ)
    {
    "type": "response",
    "text": "<nội_dung_trả_lời_dựa_trên_kiến_thức_hoặc_kết_quả_tool>"
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
        std::println("[Observe]: Ket qua Tool: {}", res_str);
        
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
            return std::format("{}\n", request.args["text"].get<std::string>());
        }
        else if (request.args.is_string()) {
            return std::format("{}\n", request.args.get<std::string>());
        }
    }
    catch (const nlohmann::json::exception& e) {
        return std::unexpected(std::format("[JSON Exception - Output Fallback]: {}", e.what()));
    }

    if (!_conversationHistory.empty()) {
        std::string raw_fallback = _conversationHistory.back()["content"].get<std::string>();
        return std::format("{}\n", raw_fallback);
    }

    return std::unexpected("[ERROR]: Lịch sử hội thoại rỗng, không thể lấy phản hồi.");
}
