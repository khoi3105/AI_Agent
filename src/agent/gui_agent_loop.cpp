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
1. STRICT PROHIBITION: Do NOT use curl, wget, lynx, python web scraping, or terminal commands to fetch website content.
   You are a pure GUI Agent: You MUST open the graphical browser (e.g. firefox) and interact with the screen using "gui_action" (click, type_text, key_press) or use "exec" ONLY to launch GUI desktop applications (e.g. 'firefox https://... &').
2. You MUST respond with ONLY ONE valid JSON object per turn. Do NOT include markdown text outside JSON.
3. NORMALIZED COORDINATES [0 to 1000]:
   - All (x, y) coordinates MUST be in the normalized range [0, 1000].
   - (0, 0) is top-left corner; (1000, 1000) is bottom-right corner.
   - Example: Center of screen is (500, 500). Top address bar / top bar is around (500, 80).
4. KEYBOARD & CLI SHORTCUTS (PREFER WHEN APPLICABLE):
   - Fast web search: Use "exec" with direct URL, e.g.:
     {"type": "tool_call", "tool": "exec", "args": {"command": "firefox \"https://www.google.com/search?q=HCMUS\" &"}}
   - Focus address bar: Use "gui_action" with "key_press" -> "Control_L+l"
   - Submit search/forms: Use "gui_action" with "key_press" -> "Return"
   - Select all text & copy: "Control_L+a", then "Control_L+c"
   - Scroll page down/up: "Page_Down" or "Page_Up"
5. GUI CLICKS & TYPING:
   - Click on elements: {"type": "tool_call", "tool": "gui_action", "args": {"action": "click", "x": 500, "y": 350}}
   - Type text: {"type": "tool_call", "tool": "gui_action", "args": {"action": "type_text", "text": "Đại học Khoa học Tự nhiên"}}
6. OBSERVE SCREENSHOT:
   - Carefully inspect the updated screenshot each turn to verify the result of your previous action.
7. FINAL RESPONSE:
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
    registry.registerGuiTools();
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
        int step_tokens = client->getLastTokensUsed();

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
        current_step_data.tokensUsed = step_tokens;

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

// ==========================================
// MULTI-AGENT COORDINATION CHO GUI AGENT
// ==========================================

void GUIAgentLoop::spawnGuiSubAgent(
    const std::string& agentId,
    const std::string& subtaskInstruction,
    int maxSteps,
    std::shared_ptr<AgentMessageQueue> messageQueue,
    std::shared_ptr<LLMClient> client,
    std::stop_token stopToken) 
{
    std::cout << std::format("[GUI Spawner] GUI Sub-Agent [{}] bat dau tren Thread ID: {}\n", 
                             agentId, std::this_thread::get_id());
    
    GUIAgentLoop guiAgent(maxSteps, std::format("/tmp/gui_agent_{}.png", agentId));
    guiAgent.setActionDelayMs(_actionDelayMs);
    
    auto result = guiAgent.run(subtaskInstruction, client, {}, stopToken);
    std::string outputContent = result.has_value() ? *result : std::format("[GUI Worker Error]: {}", result.error());
    
    messageQueue->push(AgentMessage{
        .senderId = agentId,
        .receiverId = "Coordinator",
        .content = outputContent,
        .data = {},
        .timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    });
    
    std::cout << std::format("[GUI Spawner] Sub-Agent [{}] da hoan thanh va gui du lieu vao MessageQueue.\n", agentId);
}

void GUIAgentLoop::spawnWorkerSubAgent(
    const std::string& agentId,
    const std::string& subtaskInstruction,
    int maxSteps,
    std::shared_ptr<AgentMessageQueue> messageQueue,
    std::shared_ptr<LLMClient> client,
    std::stop_token stopToken) 
{
    std::cout << std::format("[Worker Spawner] Tool/File Worker [{}] bat dau tren Thread ID: {}\n", 
                             agentId, std::this_thread::get_id());
    
    AgentLoop toolAgent(maxSteps);
    toolAgent.setEnablePlanning(false);
    
    auto result = toolAgent.run(subtaskInstruction, client, {}, stopToken);
    std::string outputContent = result.has_value() ? *result : std::format("[Tool Worker Error]: {}", result.error());
    
    messageQueue->push(AgentMessage{
        .senderId = agentId,
        .receiverId = "Coordinator",
        .content = outputContent,
        .data = {},
        .timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    });
    
    std::cout << std::format("[Worker Spawner] Worker [{}] da hoan thanh va gui ket qua vao MessageQueue.\n", agentId);
}

std::expected<std::string, std::string> GUIAgentLoop::runMultiAgentTask(
    const GuiMultiAgentTask& task,
    const std::shared_ptr<LLMClient>& coordinatorClient,
    const std::shared_ptr<LLMClient>& workerClient,
    std::stop_token stopToken) 
{
    std::cout << "\n======================================================\n";
    std::cout << std::format("[GUI Multi-Agent] KHOI CHAY TASK: {}\n", task.id);
    std::cout << std::format("Mo ta: {}\n", task.description);
    std::cout << std::format("So luong Sub-Agents: {}\n", task.subtasks.size());
    std::cout << "======================================================\n";

    if (task.subtasks.empty()) {
        return std::unexpected("[GUI Multi-Agent Error]: Danh sach subtasks rong!");
    }

    auto effectiveWorkerClient = workerClient ? workerClient : coordinatorClient;
    auto messageQueue = std::make_shared<AgentMessageQueue>();
    std::unordered_map<std::string, std::string> subResults;

    for (const auto& subtask : task.subtasks) {
        if (stopToken.stop_requested()) {
            return std::unexpected("[GUI Multi-Agent Timeout/Cancelled]");
        }

        std::string enrichedInstruction = subtask.instruction;
        if (!subResults.empty()) {
            enrichedInstruction += "\n\n[Du lieu thu thap tu cac Sub-Agents truoc do]:\n";
            for (const auto& [prevId, prevRes] : subResults) {
                enrichedInstruction += std::format("--- [{}] ---\n{}\n", prevId, prevRes);
            }
        }

        std::cout << std::format("\n>>> [Coordinator] Dieu phoi thuc thi Sub-Agent [{}] (Vai tro: {}) <<<\n", 
                                 subtask.agentId, subtask.role);

        if (subtask.role == "gui_specialist" || subtask.role == "gui") {
            spawnGuiSubAgent(subtask.agentId, enrichedInstruction, subtask.maxSteps, messageQueue, coordinatorClient, stopToken);
        } else {
            spawnWorkerSubAgent(subtask.agentId, enrichedInstruction, subtask.maxSteps, messageQueue, effectiveWorkerClient, stopToken);
        }

        auto msgOpt = messageQueue->pop(std::chrono::milliseconds(500));
        if (msgOpt.has_value()) {
            subResults[msgOpt->senderId] = msgOpt->content;
        }
    }

    std::cout << "\n[Coordinator] 🧠 Tong hop ket qua toan bo GUI Multi-Agent Workflow qua Coordinator...\n";
    
    std::string subFindings = "";
    for (const auto& subtask : task.subtasks) {
        subFindings += std::format("=== Ket qua tu {} ({}) ===\n{}\n\n", 
                                   subtask.agentId, subtask.role, subResults[subtask.agentId]);
    }

    std::string synthesisPrompt = std::format(
        "Nhiem vu tong quat: {}\n\n"
        "{}"
        "Chi thi tong hop: {}",
        task.description,
        subFindings,
        task.aggregationInstruction
    );

    AgentLoop aggregatorAgent(5);
    aggregatorAgent.setEnablePlanning(false);

    return aggregatorAgent.run(synthesisPrompt, coordinatorClient, {}, stopToken);
}

std::expected<std::string, std::string> GUIAgentLoop::coordinateTask(
    const std::string& complexUserTask,
    const std::shared_ptr<LLMClient>& coordinatorClient,
    const std::shared_ptr<LLMClient>& workerClient,
    std::stop_token stopToken) 
{
    std::cout << "\n======================================================\n";
    std::cout << "[Coordinator] BAT DAU DIEU PHOI MULTI-AGENT GUI WORKFLOW\n";
    std::cout << std::format("Yeu cau: \"{}\"\n", complexUserTask);
    std::cout << "======================================================\n";

    std::string decomposePrompt = std::format(
        "Ban la Master AI Coordinator cho he thong Autonomous GUI Desktop Automation Agent.\n"
        "Hay phan tich yeu cau sau cua nguoi dung va tu dong chia thanh cac subtasks thich hop:\n"
        "1. GUI Subtask (role: 'gui_specialist'): Thao tac tren man hinh desktop (mo trinh duyet Edge/Firefox, go phim, click, tim kiem tren web, quan sat man hinh va trich xuat du lieu can thiet).\n"
        "2. Tool/File Subtask (role: 'tool_specialist'): Xu ly du lieu trich xuat duoc tu GUI (tinh toan, format noi dung, ghi ra file van ban bang tool write_file, luu database, v.v.).\n\n"
        "Yeu cau tra ve DUY NHAT 01 khoi JSON hop le co dinh dang:\n"
        "```json\n"
        "{{\n"
        "  \"subtasks\": [\n"
        "    {{\n"
        "      \"agent_id\": \"GUI_Navigator\",\n"
        "      \"role\": \"gui_specialist\",\n"
        "      \"instruction\": \"<Chi thi chi tiet cho GUI Agent>\"\n"
        "    }},\n"
        "    {{\n"
        "      \"agent_id\": \"Data_FileWriter\",\n"
        "      \"role\": \"tool_specialist\",\n"
        "      \"instruction\": \"<Chi thi chi tiet cho Tool/File Agent>\"\n"
        "    }}\n"
        "  ],\n"
        "  \"aggregation_goal\": \"<Chi thi tong hop va bao cao ket qua cuoi cung>\"\n"
        "}}\n"
        "```\n\n"
        "Yeu cau cua nguoi dung: {}", complexUserTask
    );

    nlohmann::json planMsg = nlohmann::json::array({
        {{"role", "user"}, {"content", decomposePrompt}}
    });

    std::cout << "[Gemini Coordinator] Dang phan tich va phan ra nhiem vu Desktop GUI & Data Workflow...\n";
    auto planRes = coordinatorClient->chat(planMsg);
    if (!planRes.has_value()) {
        return std::unexpected("[Gemini Decompose Error]: " + planRes.error());
    }

    GuiMultiAgentTask multiTask{
        .id = "gui_multi_agent_workflow",
        .description = complexUserTask,
        .subtasks = {},
        .aggregationInstruction = "Xac nhan thong tin da tim thay tren man hinh va kiem tra file da duoc ghi thanh cong.",
        .timeoutSeconds = 180
    };

    try {
        std::string raw = *planRes;
        if (raw.find("```json") != std::string::npos) {
            raw = raw.substr(raw.find("```json") + 7);
            raw = raw.substr(0, raw.rfind("```"));
        } else if (raw.find("```") != std::string::npos) {
            raw = raw.substr(raw.find("```") + 3);
            raw = raw.substr(0, raw.rfind("```"));
        }
        
        size_t firstBrace = raw.find('{');
        size_t lastBrace = raw.rfind('}');
        if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace > firstBrace) {
            raw = raw.substr(firstBrace, lastBrace - firstBrace + 1);
        }

        nlohmann::json planJson = nlohmann::json::parse(raw);
        multiTask.aggregationInstruction = planJson.value("aggregation_goal", multiTask.aggregationInstruction);

        if (planJson.contains("subtasks") && planJson["subtasks"].is_array()) {
            int index = 1;
            for (const auto& item : planJson["subtasks"]) {
                std::string id = item.value("agent_id", std::format("Worker_{}", index));
                std::string role = item.value("role", "tool_specialist");
                std::string instr = item.value("instruction", "");
                if (!instr.empty()) {
                    multiTask.subtasks.push_back(GuiMultiAgentSubtask{
                        .agentId = id,
                        .role = role,
                        .instruction = instr,
                        .maxSteps = (role == "gui_specialist" ? 15 : 6)
                    });
                    index++;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("[Warning]: Khong the parse JSON tu Coordinator ({}), dung cau hinh mac dinh.\n", e.what());
    }

    if (multiTask.subtasks.empty()) {
        multiTask.subtasks.push_back(GuiMultiAgentSubtask{
            .agentId = "GUI_Navigator",
            .role = "gui_specialist",
            .instruction = complexUserTask + " (Quan sát màn hình, mở trình duyệt/ứng dụng, tìm kiếm thông tin và trích xuất dữ liệu)",
            .maxSteps = 15
        });
        multiTask.subtasks.push_back(GuiMultiAgentSubtask{
            .agentId = "Data_FileWriter",
            .role = "tool_specialist",
            .instruction = "Nhận thông tin đã tìm thấy từ GUI_Navigator và thực thi các thao tác ghi file/tính toán theo yêu cầu: " + complexUserTask,
            .maxSteps = 6
        });
    }

    std::cout << std::format("[Coordinator] Ke hoach thuc thi {} Sub-Agents:\n", multiTask.subtasks.size());
    for (const auto& st : multiTask.subtasks) {
        std::cout << std::format("   • [{}] ({}) -> {}\n", st.agentId, st.role, st.instruction);
    }
    std::cout << "\n";

    return runMultiAgentTask(multiTask, coordinatorClient, workerClient, stopToken);
}
