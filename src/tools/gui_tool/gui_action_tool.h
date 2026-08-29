#ifndef GUI_ACTION_TOOL_H
#define GUI_ACTION_TOOL_H

#include "../tool.h"
#include "xdo_executor.h"
#include <memory>
#include <utility>

/**
 * @brief Công cụ thực thi hành động tương tác GUI (chuột và phím) qua thư viện libxdo.
 * Hỗ trợ tự động quy đổi hệ tọa độ chuẩn hóa [0, 1000] sang độ phân giải thực tế của màn hình.
 */
class GuiActionTool : public Tool {
private:
    XdoExecutor _executor;

    // Quy đổi tọa độ chuẩn hóa [0, 1000] sang pixel thực tế của màn hình
    std::pair<int, int> resolveCoordinates(int raw_x, int raw_y) const;

public:
    explicit GuiActionTool(const char* display = nullptr);
    ~GuiActionTool() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;
};

#endif // GUI_ACTION_TOOL_H
