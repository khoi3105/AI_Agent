#ifndef GUI_ACTION_TOOL_H
#define GUI_ACTION_TOOL_H

#include "../tool.h"
#include "xdo_executor.h"
#include <memory>

/**
 * @brief Công cụ thực thi hành động tương tác GUI (chuột và phím) qua thư viện libxdo.
 */
class GuiActionTool : public Tool {
private:
    XdoExecutor _executor;

public:
    explicit GuiActionTool(const char* display = nullptr);
    ~GuiActionTool() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;
};

#endif // GUI_ACTION_TOOL_H
