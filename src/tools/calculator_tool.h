#ifndef CALCULATOR_TOOL_H
#define CALCULATOR_TOOL_H

#include "tool.h"

/**
 * @brief Công cụ tính toán biểu thức số học (Cụ thể hóa từ Lớp trừu tượng Tool)
 */
class CalculatorTool : public Tool {
public:
    CalculatorTool() = default;
    ~CalculatorTool() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    
    /**
     * @brief Thực thi tính toán biểu thức số học (ví dụ: "15 * 17" hoặc "2 + 3 * 4")
     */
    std::string execute(const std::string& args) override;
};

#endif // CALCULATOR_TOOL_H