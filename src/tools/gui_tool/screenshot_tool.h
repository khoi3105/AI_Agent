#ifndef SCREENSHOT_TOOL_H
#define SCREENSHOT_TOOL_H

#include "../tool.h"
#include <string>

/**
 * @brief Công cụ chụp ảnh màn hình Desktop (Screenshot) phục vụ cho VLM UI Interaction.
 */
class ScreenshotTool : public Tool {
public:
    ScreenshotTool() = default;
    ~ScreenshotTool() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;
};

#endif // SCREENSHOT_TOOL_H
