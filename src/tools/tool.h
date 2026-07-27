#ifndef TOOL_H
#define TOOL_H

#include <nlohmann/json.hpp>
#include <string>

/**
 * @brief Lớp trừu tượng cơ sở (Abstract Interface) đại diện cho mọi Tool trong hệ thống.
 * @note Nguyên tắc thiết kế: Tool hoàn toàn độc lập, KHÔNG phụ thuộc vào AgentLoop.
 */
class Tool {
public:
    virtual ~Tool() = default;

    /**
     * @brief Lấy tên định danh của Tool (Ví dụ: "calculator", "exec")
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Lấy mô tả chức năng để inject vào System Prompt cho LLM đọc
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Kích hoạt thực thi công cụ
     * @param args Chuỗi tham số hoặc dữ liệu dạng JSON do LLM truyền vào
     * @return Kết quả trả về dạng chuỗi văn bản (stdout/kết quả tính toán)
     */
    virtual std::string execute(const std::string& args) = 0;

    /**
     * @brief Lấy JSON Schema cấu trúc của Tool (theo chuẩn Ollama / OpenAI Tool Calling)
     * @return nlohmann::json chứa tên, mô tả và thông số tham số (parameters) 
     * để inject vào mảng "tools" trong payload gửi tới LLM.
     */
    virtual nlohmann::json get_schema() const = 0; 
};

#endif // TOOL_H