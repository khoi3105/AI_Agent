#ifndef AGENT_GUI_H
#define AGENT_GUI_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

#include "../agent/AgentLoop.h"
#include "../agent/step_data.h"
#include "../client/llm_client.h"
#include "../environment/environment.h"
#include "../tools/tool_registry.h"
#include "../harness/harness_runner.h"

// Cấu trúc chứa thông tin log bước hiển thị trên GUI
struct StepUiData {
    int stepNumber{0};
    std::string thought;
    std::string toolName;
    std::string toolArgs;
    std::string observation;
    long latencyMs{0};
};

// Cấu trúc hiển thị bản ghi trí nhớ SQLite
struct MemoryRecord {
    int id;
    std::string content;
    std::string createdAt;
};

// Lớp điều phối Giao diện Đồ họa Dear ImGui cho AI Agent
class AgentGUI {
private:
    std::shared_ptr<LLMClient> _client;
    std::shared_ptr<Environment> _env;
    ToolRegistry _registry;

    // Trạng thái nhập liệu của UI
    char _promptBuffer[2048]{};
    char _modelBuffer[128]{};
    char _baseUrlBuffer[256]{};
    char _apiKeyBuffer[256]{};
    int _maxSteps{8};
    bool _enablePlanning{true};

    // Luồng chạy ngầm và đồng bộ dữ liệu
    std::atomic<bool> _isRunning{false};
    std::atomic<bool> _isBenchmarkRunning{false};
    std::string _statusMessage{"Sẵn sàng (Idle)"};
    std::string _finalOutput;
    
    // Dữ liệu thời gian thực được bảo vệ bởi Mutex
    std::mutex _dataMutex;
    std::vector<StepUiData> _steps;
    std::vector<MemoryRecord> _memories;
    std::vector<Trajectory> _benchmarkResults;
    std::string _currentGoal;
    std::vector<std::string> _plannedSteps;

public:
    explicit AgentGUI(std::shared_ptr<LLMClient> client, std::shared_ptr<Environment> env = nullptr);
    ~AgentGUI();

    // Khởi chạy vòng lặp cửa sổ đồ họa Desktop
    int run();

private:
    void initStyles();
    void renderControlPanel();
    void renderMainChatAndTrajectory();
    void renderTelemetryAndMemoryPanel();

    // Các hàm kích hoạt tác vụ chạy nền
    void startAgentTask(const std::string& prompt);
    void startBenchmarkBatch();
    void refreshMemories();
};

#endif // AGENT_GUI_H
