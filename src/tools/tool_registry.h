#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include "tool.h"
#include "calculator_tool.h"
#include <unordered_map>
#include <string>
#include <iostream>

/**
 * @brief Lớp quản lý Tool đơn giản cho giai đoạn POC.
 * @note Hardcode khởi tạo sẵn các Tool cụ thể inside constructor.
 */
class ToolRegistry {
private:
    // Lưu trữ danh sách con trỏ Tool (Hardcode ở POC)
    std::unordered_map<std::string, Tool*> tools;
public:
    // Constructor POC: Tự động khởi tạo cứng (Hardcode) các Tool hiện có
    ToolRegistry();
    // Destructor: Giải phóng bộ nhớ các tool đã new trong constructor
    ~ToolRegistry();
    /**
     * @brief Lấy con trỏ Tool theo tên ("calculator")
     */
    Tool* getTool(const std::string& name);
    /**
     * @brief Hàm tiện ích chạy thẳng Tool cho POC
     */
    std::string executeTool(const std::string& name, const std::string& args);
};

#endif // TOOL_REGISTRY_H