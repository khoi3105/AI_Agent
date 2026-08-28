#include "screenshot_tool.h"
#include <cstdlib>
#include <filesystem>
#include <format>

std::string ScreenshotTool::getName() const {
    return "capture_screenshot";
}

std::string ScreenshotTool::getDescription() const {
    return "Chụp ảnh toàn bộ màn hình desktop hiện tại và lưu vào file ảnh (PNG). Trả về đường dẫn file ảnh để VLM phân tích giao diện UI.";
}

std::string ScreenshotTool::execute(const nlohmann::json& args) {
    std::string output_path = "/tmp/agent_screenshot.png";
    if (args.is_object() && args.contains("output_path") && args["output_path"].is_string()) {
        output_path = args["output_path"].get<std::string>();
    }

    // Đảm bảo thư mục cha tồn tại
    try {
        std::filesystem::path p(output_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (const std::exception& e) {
        return std::format("[Lỗi capture_screenshot]: Không thể tạo thư mục lưu ảnh: {}", e.what());
    }

    // Gọi lệnh maim để chụp màn hình desktop
    std::string cmd = std::format("maim \"{}\" 2>/dev/null", output_path);
    int exit_code = std::system(cmd.c_str());

    // Nếu maim thất bại, thử fallback sang scrot hoặc xwd / import
    if (exit_code != 0 || !std::filesystem::exists(output_path) || std::filesystem::file_size(output_path) == 0) {
        std::string fallback_cmd = std::format("import -window root \"{}\" 2>/dev/null || scrot \"{}\" 2>/dev/null", output_path, output_path);
        exit_code = std::system(fallback_cmd.c_str());
    }

    if (!std::filesystem::exists(output_path) || std::filesystem::file_size(output_path) == 0) {
        return std::format("[Lỗi capture_screenshot]: Chụp màn hình thất bại. Hãy kiểm tra biến môi trường DISPLAY hoặc quyền truy cập X11.");
    }

    return std::format("Chụp màn hình thành công! File ảnh lưu tại: {}", output_path);
}

nlohmann::json ScreenshotTool::get_schema() const {
    return {
        {"type", "tool_call"},
        {"tool_call", {
            {"name", getName()},
            {"description", getDescription()},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"output_path", {
                        {"type", "string"},
                        {"description", "Đường dẫn file lưu ảnh màn hình PNG (mặc định: '/tmp/agent_screenshot.png')"}
                    }}
                }}
            }}
        }}
    };
}
