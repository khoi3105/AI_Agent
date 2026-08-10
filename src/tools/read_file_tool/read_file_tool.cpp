#include "read_file_tool.h"

std::string ReadFileTool::getName() const {
    return "read_file";
}

std::string ReadFileTool::getDescription() const {
    return "Read text from txt, csv, xml, md, json, log, ini, yaml, yml, cpp, h, py, c, cc, java, js, html, css, sql, sh, bat and pdf files.";
}
std::string ReadFileTool::execute(const nlohmann::json& args) {
    try {
        // 1. Kiểm tra sự tồn tại của khóa "path"
        if (!args.contains("path")) {
            return "[Lỗi ReadFileTool]: Thiếu tham số 'path' trong đối số truyền vào.";
        }

        // 2. Trích xuất trực tiếp giá trị path từ args (JSON Object)
        std::string path = args["path"].get<std::string>();

        // 3. Gọi hàm load của _loader
        auto result = _loader.load(path);
        if (!result) {
            return result.error();
        }

        return result.value();
    }
    catch (const std::exception& e) {
        // Bắt các ngoại lệ liên quan đến JSON (như sai kiểu dữ liệu) để tránh làm crash chương trình
        return std::string("[Lỗi ReadFileTool]: ") + e.what();
    }
}

nlohmann::json ReadFileTool::get_schema() const {
    return {
        {"type", "function"},
        {"function", {
            {"name", getName()},
            {"description", getDescription()},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "Path of file to read"}
                    }}
                }},
                {"required", {"path"}}
            }}
        }}
    };
}