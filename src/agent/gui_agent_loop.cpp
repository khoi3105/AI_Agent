#include "gui_agent_loop.h"
#include <iostream>
#include <format>
#include <chrono>
#include <thread>
#include <filesystem>
#include <cstdlib>

GUIAgentLoop::GUIAgentLoop(int max_steps, std::string screenshot_path, std::string skills_dir)
    : AgentLoop(max_steps, std::move(skills_dir)), _screenshotPath(std::move(screenshot_path)) {}

bool GUIAgentLoop::captureScreen() {
    try {
        std::filesystem::path p(_screenshotPath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {}

    std::string cmd = std::format("maim \"{}\" 2>/dev/null", _screenshotPath);
    int code = std::system(cmd.c_str());
    if (code != 0 || !std::filesystem::exists(_screenshotPath) || std::filesystem::file_size(_screenshotPath) == 0) {
        std::string fallback = std::format("import -window root \"{}\" 2>/dev/null || scrot \"{}\" 2>/dev/null", _screenshotPath, _screenshotPath);
        code = std::system(fallback.c_str());
    }
    return std::filesystem::exists(_screenshotPath) && std::filesystem::file_size(_screenshotPath) > 0;
}

std::string GUIAgentLoop::prepareSystemPrompt(const ToolRegistry& registry, const std::string& user_task) {
    nlohmann::json tools_schema = registry.getAllSchemas();

    std::string skills_section = "";
    if (!user_task.empty()) {
        auto skill_res = _skillLoader.selectSkillsForTask(user_task);
        if (skill_res.has_value() && !skill_res.value().empty()) {
            skills_section = "\n\n=== KỸ NĂNG CHUYÊN BIỆT KÍCH HOẠT ===\n" + skill_res.value() + "\n";
        }
    }

    return R"(You are an Autonomous GUI Desktop Automation Agent (Computer Use).
Your mission is to perform tasks by DIRECTLY INTERACTING with the Linux desktop (GUI applications, mouse, keyboard) based on the attached screenshot.

=== AVAILABLE GUI & OS TOOLS (JSON) ===
)" + tools_schema.dump(2) + skills_section + R"(

=== CRITICAL OPERATING RULES & COORDINATE SYSTEM ===
1. You MUST respond with ONLY ONE valid JSON object per turn. Do NOT include markdown text outside JSON.
2. NORMALIZED COORDINATES [0 to 1000]:
   - All (x, y) coordinates MUST be in the normalized range [0, 1000].
   - (0, 0) is top-left corner; (1000, 1000) is bottom-right corner.
   - Example: Center of screen is (500, 500). Top address bar / top bar is around (500, 80).
3. KEYBOARD & CLI SHORTCUTS (PREFER WHEN APPLICABLE):
   - Fast web search: Use "exec" with direct URL, e.g.:
     {"type": "tool_call", "tool": "exec", "args": {"command": "firefox \"https://www.google.com/search?q=HCMUS\" &"}}
   - Focus address bar: Use "gui_action" with "key_press" -> "Control_L+l"
   - Submit search/forms: Use "gui_action" with "key_press" -> "Return"
   - Select all text & copy: "Control_L+a", then "Control_L+c"
   - Scroll page down/up: "Page_Down" or "Page_Up"
4. GUI CLICKS & TYPING:
   - Click on elements: {"type": "tool_call", "tool": "gui_action", "args": {"action": "click", "x": 500, "y": 350}}
   - Type text: {"type": "tool_call", "tool": "gui_action", "args": {"action": "type_text", "text": "Đại học Khoa học Tự nhiên"}}
5. OBSERVE SCREENSHOT:
   - Carefully inspect the updated screenshot each turn to verify the result of your previous action.
6. FINAL RESPONSE:
   - When the user's task is fully accomplished and visible on screen, output "response":
     {"type": "response", "text": "<summary of final information found on the GUI>"}

=== STRICT OUTPUT FORMAT ===
Format 1 (GUI / OS Tool Action):
{
  "type": "tool_call",
  "tool": "<gui_action | exec | capture_screenshot | read_file | write_file>",
  "args": { <parameters> }
}

Format 2 (Final Answer):
{
  "type": "response",
  "text": "<summary of final answer>"
}
)";
}

std::expected<std::string, std::string> GUIAgentLoop::run(
    const std::string& user_task, 
    const std::shared_ptr<LLMClient>& client, 
    const std::vector<std::string>& image_paths,
    std::stop_token stop_token) 
{
    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac Timeout!");
    }

    // Khởi tạo ToolRegistry chỉ chứa các công cụ điều khiển GUI và OS
    ToolRegistry registry;
    registry.unregisterTool("web_search");
    registry.unregisterTool("weather");
    registry.unregisterTool("calculator");
    registry.unregisterTool("memory_save");
    registry.unregisterTool("memory_search");

    _conversationHistory.clear();
    _loopdetector.reset();

    std::string system_prompt = prepareSystemPrompt(registry, user_task);
    _conversationHistory.push_back({{"role", "system"}, {"content", system_prompt}});

    std::string initial_user_prompt = user_task + "\n\n[INSTRUCTION]: Look at the attached screenshot of the current desktop. Perform the next GUI action (open browser via exec, click, type) by outputting a single JSON tool_call.";
    _conversationHistory.push_back({{"role", "user"}, {"content", initial_user_prompt}});

    int step = 0;
    ToolCallRequest request;

    while (step < AgentLoop::_maxstep) {
        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac Timeout!");
        }

        step++;
        auto step_start_time = std::chrono::steady_clock::now();

        // 1. Tự động chụp ảnh màn hình mới nhất trước mỗi lượt suy luận
        std::vector<std::string> current_images = image_paths;
        if (captureScreen()) {
            current_images.push_back(_screenshotPath);
            std::cout << std::format("[GUIAgent] Da chup anh man hinh buoc {}: {}\n", step, _screenshotPath);
        } else {
            std::cout << "[GUIAgent Warning] Khong the chup anh man hinh hien tai.\n";
        }

        // 2. Gửi ảnh và lịch sử cho VLM
        std::expected<std::string, std::string> llm_response = client->chat(_conversationHistory, current_images);

        if (stop_token.stop_requested()) {
            return std::unexpected("[ERROR]: Task bi huy hoac Timeout!");
        }

        if (llm_response.has_value()) {
            std::cout << "========================================================================\n";
            std::cout << std::format("--> [GUIAgent] Phan hoi AI (Buoc {}):\n{}\n", step, *llm_response);
            std::cout << "========================================================================\n";
        } else {
            return std::unexpected(std::format("[ERROR]: Khong nhan duoc phan hoi tu VLM - {}!", llm_response.error()));
        }

        request = parseStepResponse(*llm_response);
        _conversationHistory.push_back({{"role", "assistant"}, {"content", *llm_response}});

        StepData current_step_data;
        current_step_data.stepNumber = step;
        current_step_data.thought = *llm_response;

        if (request.is_valid && request.tool_name != "null" && !request.tool_name.empty()) {
            current_step_data.actionName = request.tool_name;
            current_step_data.actionArgs = request.args;

            LoopCheckResult _detectLoop = _loopdetector.checkLoop(request.tool_name, request.args);

            if (_detectLoop.status == LoopStatus::CRITICAL) {
                return std::unexpected(_detectLoop.message);
            }
            else if (_detectLoop.status == LoopStatus::WARNING) {
                _conversationHistory.push_back({
                    {"role", "user"},
                    {"content", "WARNING: You are repeating the same action. Observe the updated screenshot and proceed with the next step or output the final response."}
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
                return std::unexpected("[ERROR]: Task bi huy hoac Timeout!");
            }

            std::cout << std::format("[GUIAgent Act]: Thuc thi cong cu '{}'...\n", request.tool_name);

            auto checkToolRegistry = act(registry, request.tool_name, request.args);

            // Nghỉ một khoảng thời gian nhỏ sau action để giao diện kịp cập nhật
            if (_actionDelayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(_actionDelayMs));
            }

            // Ghi nhận Observation kèm lời nhắc hành động tiếp theo
            if (checkToolRegistry.has_value()) {
                std::string res_str = *checkToolRegistry;
                std::cout << "[GUIAgent Observe]: " << res_str << std::endl;
                current_step_data.observation = res_str;

                std::string observation_msg = std::format("Tool '{}' executed successfully. Output: {}\nObserve the updated desktop screenshot and output the next JSON tool_call or final response.", request.tool_name, res_str);
                _conversationHistory.push_back({{"role", "user"}, {"content", observation_msg}});
            } else {
                std::string error_msg = checkToolRegistry.error();
                current_step_data.observation = "[ERROR]: " + error_msg;

                std::string observation_error = std::format("[ERROR] Tool '{}' failed: {}. Look at the screenshot and try an alternative action.", request.tool_name, error_msg);
                _conversationHistory.push_back({{"role", "user"}, {"content", observation_error}});
            }

            auto step_end_time = std::chrono::steady_clock::now();
            current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();
            if (_stepHook) {
                _stepHook(current_step_data);
            }

            continue;
        }

        current_step_data.actionName = "finish";
        current_step_data.actionArgs = request.args;
        current_step_data.observation = "Hoàn thành tác vụ GUI.";

        auto step_end_time = std::chrono::steady_clock::now();
        current_step_data.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(step_end_time - step_start_time).count();

        if (_stepHook) {
            _stepHook(current_step_data);
        }

        break;
    }

    if (stop_token.stop_requested()) {
        return std::unexpected("[ERROR]: Task bi huy hoac Timeout!");
    }

    if (step >= AgentLoop::_maxstep) {
        return std::unexpected("[ERROR]: Da dat so buoc GUI toi da!");
    }

    return formatFinalResponse(request);
}
