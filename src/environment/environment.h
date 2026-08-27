#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <expected>
#include <filesystem>
#include "../tools/exec_tool/shell_executor.h"

/**
 * @brief Lớp trừu tượng (Interface) định nghĩa Môi trường thực thi (Environment)
 * Quản lý vòng đời: setup() -> execute() -> teardown()
 */
class Environment {
public:
    virtual ~Environment() = default;

    /**
     * @brief Thiết lập môi trường trước khi chạy task (ví dụ: tạo file giả lập, khởi tạo cấu hình)
     * @param setup_script Câu lệnh bash/shell cần thực thi
     */
    virtual std::expected<void, std::string> setup(const std::string& setup_script) = 0;

    /**
     * @brief Thực thi lệnh shell trong môi trường
     * @param command Câu lệnh shell
     * @return ExecResult chứa exit_code, stdout_text, stderr_text
     */
    virtual std::expected<ExecResult, std::string> execute(const std::string& command) = 0;

    /**
     * @brief Dọn dẹp môi trường sau khi task kết thúc (xóa file rác, reset trạng thái)
     */
    virtual void teardown() = 0;

    /**
     * @brief Lấy đường dẫn thư mục làm việc hiện tại của môi trường
     */
    virtual std::filesystem::path getWorkingDirectory() const = 0;
};

#endif // ENVIRONMENT_H
