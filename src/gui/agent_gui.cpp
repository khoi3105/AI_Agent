#include "agent_gui.h"

#include <iostream>
#include <format>
#include <cstring>
#include <chrono>
#include <sqlite3.h>

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "../utils/env_utils.h"
#include "../environment/native_environment.h"

using namespace std;

// Callback xử lý lỗi từ GLFW
static void glfw_error_callback(int error, const char* description) {
    cerr << format("[GLFW Error {}]: {}\n", error, description);
}

AgentGUI::AgentGUI(shared_ptr<LLMClient> client, shared_ptr<Environment> env)
    : _client(client), _env(env) {
    if (!_env) {
        _env = make_shared<NativeEnvironment>();
    }

    // Nạp giá trị mặc định từ môi trường
    string defaultModel = getEnvVar("OLLAMA_MODEL");
    if (defaultModel.empty()) defaultModel = "meta/llama-3.2-11b-vision-instruct";
    strncpy(_modelBuffer, defaultModel.c_str(), sizeof(_modelBuffer) - 1);

    string defaultUrl = getEnvVar("OLLAMA_BASE_URL");
    if (defaultUrl.empty()) defaultUrl = "https://integrate.api.nvidia.com/v1/chat/completions";
    strncpy(_baseUrlBuffer, defaultUrl.c_str(), sizeof(_baseUrlBuffer) - 1);

    string defaultKey = getEnvVar("LLAMA_API_KEY");
    if (defaultKey.empty()) defaultKey = getEnvVar("OLLAMA_API_KEY");
    strncpy(_apiKeyBuffer, defaultKey.c_str(), sizeof(_apiKeyBuffer) - 1);

    string defaultPrompt = "Tính (125 * 37) + (940 / 5) sau đó ghi kết quả vào file result.txt";
    strncpy(_promptBuffer, defaultPrompt.c_str(), sizeof(_promptBuffer) - 1);

    refreshMemories();
}

AgentGUI::~AgentGUI() {
}

void AgentGUI::initStyles() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Dark Theme - Cyberpunk / Developer Modern Palette
    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.53f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.11f, 0.12f, 0.15f, 0.96f);
    colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.25f, 0.32f, 0.80f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.26f, 0.33f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.33f, 0.42f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.37f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.47f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.35f, 0.78f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.35f, 0.78f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.28f, 0.38f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.16f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 0.32f, 0.44f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.22f, 0.25f, 0.32f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.32f, 0.45f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.26f, 0.38f, 1.00f);

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 6.0f);
    style.ItemSpacing       = ImVec2(8.0f, 8.0f);
}

int AgentGUI::run() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        cerr << "[ERROR]: Khong the khoi tao GLFW!\n";
        return 1;
    }

    // OpenGL 3.3 Core Profile
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1440, 880, "Autonomous C++ AI Agent - Desktop GUI Dashboard (OOP 2026)", nullptr, nullptr);
    if (!window) {
        cerr << "[ERROR]: Khong the tao cua so GLFW!\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    // Khởi tạo ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    initStyles();

    // Khởi tạo Backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Vòng lặp Render chính
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Thiết lập kích thước cửa sổ toàn màn hình ứng dụng
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("AI Agent MainWindow", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // =========================================================================
        // 3-PANEL RESPONSIVE LAYOUT
        // =========================================================================
        float totalWidth = io.DisplaySize.x;
        float totalHeight = io.DisplaySize.y - 24.0f;
        float leftWidth = 320.0f;
        float rightWidth = 360.0f;
        float centerWidth = totalWidth - leftWidth - rightWidth - 32.0f;

        // PANEL 1: Cấu hình & Điều khiển (Trái)
        ImGui::BeginChild("ControlPanel", ImVec2(leftWidth, totalHeight), true);
        renderControlPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        // PANEL 2: Chat & Live ReAct Trajectory (Giữa)
        ImGui::BeginChild("CenterPanel", ImVec2(centerWidth, totalHeight), true);
        renderMainChatAndTrajectory();
        ImGui::EndChild();

        ImGui::SameLine();

        // PANEL 3: Telemetry, TaskPlan & SQLite Inspector (Phải)
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
        renderTelemetryAndMemoryPanel();
        ImGui::EndChild();

        ImGui::End();

        // Render OpenGL
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Dọn dẹp
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void AgentGUI::renderControlPanel() {
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "⚙️ HỆ THỐNG CẤU HÌNH");
    ImGui::Separator();

    ImGui::Text("Tên Model:");
    ImGui::InputText("##ModelInput", _modelBuffer, sizeof(_modelBuffer));

    ImGui::Text("API Base URL:");
    ImGui::InputText("##UrlInput", _baseUrlBuffer, sizeof(_baseUrlBuffer));

    ImGui::Text("API Key:");
    ImGui::InputText("##KeyInput", _apiKeyBuffer, sizeof(_apiKeyBuffer), ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "🎛️ THAM SỐ SUY LUẬN");

    ImGui::SliderInt("Max Steps", &_maxSteps, 1, 20);
    ImGui::Checkbox("Kích hoạt Adaptive Planning", &_enablePlanning);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "🚀 ĐIỀU KHIỂN TÁC VỤ");

    if (_isRunning.load()) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
        ImGui::Button("⏳ ĐANG XỬ LÝ (RUNNING)...", ImVec2(-1, 38));
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.35f, 1.0f));
        if (ImGui::Button("▶ CHẠY TÁC VỤ (RUN AGENT)", ImVec2(-1, 38))) {
            startAgentTask(_promptBuffer);
        }
        ImGui::PopStyleColor();
    }

    if (_isBenchmarkRunning.load()) {
        ImGui::Button("⏳ BENCHMARK IN PROGRESS...", ImVec2(-1, 32));
    } else {
        if (ImGui::Button("📊 CHẠY BATCH BENCHMARK (10 TASKS)", ImVec2(-1, 32))) {
            startBenchmarkBatch();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "🛠️ DANH SÁCH TOOLS (8)");
    
    ImGui::BeginChild("ToolsListChild", ImVec2(0, 150), true);
    ImGui::BulletText("calculator (exprtk)");
    ImGui::BulletText("exec (shell + policy)");
    ImGui::BulletText("read_file (text + poppler)");
    ImGui::BulletText("write_file (STL)");
    ImGui::BulletText("web_search (gumbo HTML)");
    ImGui::BulletText("memory_save (SQLite3)");
    ImGui::BulletText("memory_search (2-Tier)");
    ImGui::BulletText("weather (OpenWeather)");
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextDisabled("Trạng thái: %s", _statusMessage.c_str());
}

void AgentGUI::renderMainChatAndTrajectory() {
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "💬 NHẬP YÊU CẦU & NHẬT KÝ QUỸ ĐẠO REACT");
    ImGui::Separator();

    ImGui::Text("Nội dung câu hỏi / Tác vụ cần AI thực thi:");
    ImGui::InputTextMultiline("##PromptInput", _promptBuffer, sizeof(_promptBuffer), ImVec2(-1, 70));

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), " VẾT SUY LUẬN & THỰC THI THỜI GIAN THỰC (LIVE RE-ACT TRACE):");

    // Khung cuộn chứa các Step Cards
    ImGui::BeginChild("TrajectoryScrollRegion", ImVec2(-1, -160), true);

    lock_guard<mutex> lock(_dataMutex);
    if (_steps.empty()) {
        ImGui::TextDisabled("Chưa có bước thực thi nào. Hãy bấm 'CHẠY TÁC VỤ' để bắt đầu.");
    } else {
        for (const auto& s : _steps) {
            string headerTitle = format("Bước {}: [Tool: {}] - Latency: {} ms", 
                                        s.stepNumber, 
                                        s.toolName.empty() ? "None" : s.toolName, 
                                        s.latencyMs);

            if (ImGui::CollapsingHeader(headerTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                // 1. Thought
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), " Thought:");
                ImGui::Indent();
                ImGui::TextWrapped("%s", s.thought.c_str());
                ImGui::Unindent();

                // 2. Action / Tool Call
                if (!s.toolName.empty() && s.toolName != "finish") {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "⚙️ Action (Tool Call):");
                    ImGui::Indent();
                    ImGui::TextWrapped("Tool: %s\nArgs: %s", s.toolName.c_str(), s.toolArgs.c_str());
                    ImGui::Unindent();
                }

                // 3. Observation
                if (!s.observation.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "👁️ Observation:");
                    ImGui::Indent();
                    ImGui::TextWrapped("%s", s.observation.c_str());
                    ImGui::Unindent();
                }
                ImGui::Spacing();
            }
        }
    }
    ImGui::EndChild();

    // Khung kết quả hoàn thành cuối cùng
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "🏁 KẾT QUẢ CUỐI CÙNG (FINAL OUTPUT):");
    ImGui::BeginChild("FinalOutputRegion", ImVec2(-1, 110), true);
    if (_finalOutput.empty()) {
        ImGui::TextDisabled("Đang chờ kết quả tổng hợp...");
    } else {
        ImGui::TextWrapped("%s", _finalOutput.c_str());
    }
    ImGui::EndChild();
}

void AgentGUI::renderTelemetryAndMemoryPanel() {
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "📊 TELEMETRY & MEMORY");
    ImGui::Separator();

    // 1. Task Planning
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "🎯 Kế Hoạch Đa Bước (TaskPlan):");
    ImGui::BeginChild("PlanChild", ImVec2(0, 110), true);
    {
        lock_guard<mutex> lock(_dataMutex);
        if (_plannedSteps.empty()) {
            ImGui::TextDisabled("Không áp dụng lập kế hoạch cho tác vụ này.");
        } else {
            for (size_t i = 0; i < _plannedSteps.size(); ++i) {
                ImGui::BulletText("%s", _plannedSteps[i].c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Memory Database Inspector
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "💾 SQLite Memory Inspector:");
    ImGui::SameLine();
    if (ImGui::SmallButton("🔄 Refresh")) {
        refreshMemories();
    }

    ImGui::BeginChild("MemoryTableChild", ImVec2(0, 160), true);
    {
        lock_guard<mutex> lock(_dataMutex);
        if (_memories.empty()) {
            ImGui::TextDisabled("Chưa có bản ghi nào trong memory.db");
        } else {
            for (const auto& m : _memories) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[ID: %d]", m.id);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", m.content.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Benchmark Scoreboard
    ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.98f, 1.0f), "🏆 Bảng Điểm Benchmark (10 Tasks):");
    ImGui::BeginChild("BenchmarkTableChild", ImVec2(0, 0), true);
    {
        lock_guard<mutex> lock(_dataMutex);
        if (_benchmarkResults.empty()) {
            ImGui::TextDisabled("Chưa chạy benchmark. Nhấn 'CHẠY BATCH BENCHMARK' để bắt đầu.");
        } else {
            for (const auto& r : _benchmarkResults) {
                if (r.isSuccess()) {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "[PASS]");
                } else {
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "[FAIL]");
                }
                ImGui::SameLine();
                ImGui::Text("%s: %ld ms (%zu steps)", r.getTaskId().c_str(), r.getTotalTimeMs(), r.getStepCount());
            }
        }
    }
    ImGui::EndChild();
}

void AgentGUI::startAgentTask(const string& prompt) {
    if (_isRunning.load() || prompt.empty()) return;

    _isRunning.store(true);
    _statusMessage = "Đang thực thi nhiệm vụ...";
    
    {
        lock_guard<mutex> lock(_dataMutex);
        _steps.clear();
        _plannedSteps.clear();
        _finalOutput.clear();
    }

    // Chạy trên Background Thread để không làm treo UI
    thread([this, prompt]() {
        AgentLoop agent;
        agent.setMaxSteps(_maxSteps);
        agent.setEnablePlanning(_enablePlanning);

        // Đăng ký StepHook để nhận sự kiện từng bước thời gian thực
        agent.setStepHook([this](const StepData& sd) {
            lock_guard<mutex> lock(_dataMutex);
            StepUiData uiData;
            uiData.stepNumber = sd.stepNumber;
            uiData.thought = sd.thought;
            uiData.toolName = sd.actionName;
            uiData.toolArgs = sd.actionArgs.is_null() ? "" : sd.actionArgs.dump(2);
            uiData.observation = sd.observation;
            uiData.latencyMs = sd.latencyMs;
            _steps.push_back(uiData);

            if (sd.actionName == "planning") {
                if (sd.actionArgs.contains("steps")) {
                    for (const auto& st : sd.actionArgs["steps"]) {
                        string desc = st.value("description", "");
                        string tool = st.value("suggested_tool", "");
                        _plannedSteps.push_back(format("{}. {} [{}]", st.value("step", 0), desc, tool));
                    }
                }
            }
        });

        auto res = agent.run(prompt, _client);

        {
            lock_guard<mutex> lock(_dataMutex);
            if (res.has_value()) {
                _finalOutput = res.value();
                _statusMessage = "Hoàn tất thành công!";
            } else {
                _finalOutput = "[Lỗi]: " + res.error();
                _statusMessage = "Thực thi thất bại!";
            }
        }

        refreshMemories();
        _isRunning.store(false);
    }).detach();
}

void AgentGUI::startBenchmarkBatch() {
    if (_isBenchmarkRunning.load()) return;

    _isBenchmarkRunning.store(true);
    _statusMessage = "Đang chạy toàn bộ Benchmark Batch...";

    {
        lock_guard<mutex> lock(_dataMutex);
        _benchmarkResults.clear();
    }

    thread([this]() {
        HarnessRunner runner(_client, _env, "benchmark/results");
        if (runner.loadTasks("benchmark/tasks.json").has_value()) {
            auto results = runner.runBatch();
            lock_guard<mutex> lock(_dataMutex);
            _benchmarkResults = results;
        }
        _isBenchmarkRunning.store(false);
        _statusMessage = "Benchmark hoàn tất!";
    }).detach();
}

void AgentGUI::refreshMemories() {
    sqlite3* db = nullptr;
    if (sqlite3_open("memory.db", &db) != SQLITE_OK) {
        return;
    }

    const char* sql = "SELECT id, content, created_at FROM memories ORDER BY id DESC LIMIT 50;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        lock_guard<mutex> lock(_dataMutex);
        _memories.clear();
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MemoryRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const unsigned char* dateTxt = sqlite3_column_text(stmt, 2);
            rec.createdAt = dateTxt ? reinterpret_cast<const char*>(dateTxt) : "";
            _memories.push_back(rec);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
