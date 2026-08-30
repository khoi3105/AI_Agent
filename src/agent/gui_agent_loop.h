#ifndef GUI_AGENT_LOOP_H
#define GUI_AGENT_LOOP_H

#include "AgentLoop.h"
#include <string>
#include <vector>

/**
 * @brief Lớp GUIAgentLoop kế thừa AgentLoop, chuyên biệt hóa cho tác vụ điều khiển máy tính (Desktop GUI Agent).
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

    // Chạy vòng lặp GUI Agent (tự động chụp ảnh màn hình mỗi bước)
    std::expected<std::string, std::string> run(
        const std::string& user_task, 
        const std::shared_ptr<LLMClient>& client, 
        const std::vector<std::string>& image_paths = {},
        std::stop_token stop_token = {}
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