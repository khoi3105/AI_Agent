#ifndef GUI_AGENT_LOOP_H
#define GUI_AGENT_LOOP_H

#include "AgentLoop.h"
#include "message_queue.h"
#include <string>
#include <vector>
#include <memory>
#include <expected>
#include <stop_token>
#include <nlohmann/json.hpp>

/**
 * @brief Cấu trúc mô tả Subtask cho GUI Multi-Agent Coordination
 */
struct GuiMultiAgentSubtask {
    std::string agentId;       // "GUI_Worker", "Tool_Worker", v.v.
    std::string role;          // "gui_specialist" hoặc "tool_specialist"
    std::string instruction;   // Chỉ thị chi tiết
    int maxSteps{8};
};

/**
 * @brief Cấu trúc bài toán điều phối Đa Agent kết hợp GUI
 */
struct GuiMultiAgentTask {
    std::string id;
    std::string description;
    std::vector<GuiMultiAgentSubtask> subtasks;
    std::string aggregationInstruction;
    int timeoutSeconds{120};
};

/**
 * @brief Lớp GUIAgentLoop kế thừa AgentLoop, chuyên biệt hóa cho tác vụ điều khiển máy tính (Desktop GUI Agent)
 * và Tích hợp Cơ chế Điều phối Đa Agent Đa phương thức (Multi-Agent GUI Coordination).
 * Tự động chụp ảnh màn hình trước mỗi bước, nạp ảnh vào VLM (Vision-Language Model), 
 * và thực thi các hành động click/type/key_press trên màn hình.
 */
class GUIAgentLoop : public AgentLoop {
private:
    std::string _screenshotPath;
    int _actionDelayMs{1000}; // Thời gian nghỉ giữa các action để UI kịp load

public:
    explicit GUIAgentLoop(
        int max_steps = 10, 
        std::string screenshot_path = "/tmp/gui_agent_screen.png",
        std::string skills_dir = "skills"
    );

    ~GUIAgentLoop() override = default;

    // Chạy vòng lặp GUI Agent đơn lẻ (tự động chụp ảnh màn hình mỗi bước)
    std::expected<std::string, std::string> run(
        const std::string& user_task, 
        const std::shared_ptr<LLMClient>& client, 
        const std::vector<std::string>& image_paths = {},
        std::stop_token stop_token = {}
    );

    // ==========================================
    // MULTI-AGENT COORDINATION CHO GUI AGENT
    // ==========================================

    // Spawn một GUI Sub-Agent chuyên tương tác màn hình, gửi kết quả vào MessageQueue
    void spawnGuiSubAgent(
        const std::string& agentId,
        const std::string& subtaskInstruction,
        int maxSteps,
        std::shared_ptr<AgentMessageQueue> messageQueue,
        std::shared_ptr<LLMClient> client,
        std::stop_token stopToken = {}
    );

    // Spawn một Tool/File Sub-Agent chuyên xử lý dữ liệu và công cụ hậu kỳ (write_file, calculator...)
    void spawnWorkerSubAgent(
        const std::string& agentId,
        const std::string& subtaskInstruction,
        int maxSteps,
        std::shared_ptr<AgentMessageQueue> messageQueue,
        std::shared_ptr<LLMClient> client,
        std::stop_token stopToken = {}
    );

    // Điều phối một GuiMultiAgentTask đã xác định
    std::expected<std::string, std::string> runMultiAgentTask(
        const GuiMultiAgentTask& task,
        const std::shared_ptr<LLMClient>& coordinatorClient,
        const std::shared_ptr<LLMClient>& workerClient = nullptr,
        std::stop_token stopToken = {}
    );

    // Tự động phân rã bài toán phức tạp (Dynamic Fan-out) -> Chạy song song/pipeline GUI Agent & Tool Workers -> Tổng hợp báo cáo
    std::expected<std::string, std::string> coordinateTask(
        const std::string& complexUserTask,
        const std::shared_ptr<LLMClient>& coordinatorClient,
        const std::shared_ptr<LLMClient>& workerClient = nullptr,
        std::stop_token stopToken = {}
    );

    void setActionDelayMs(int delay_ms) {
        _actionDelayMs = delay_ms;
    }

    int getActionDelayMs() const {
        return _actionDelayMs;
    }

    const std::string& getScreenshotPath() const {
        return _screenshotPath;
    }

protected:
    std::string prepareSystemPrompt(const ToolRegistry& registry, const std::string& user_task = "") override;
    bool captureScreen();
};

#endif // GUI_AGENT_LOOP_H
