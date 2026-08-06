#include <expected>
#include <string>
#include <filesystem>

class FileUtils {
public:
    // Đọc toàn bộ nội dung file văn bản
    static std::expected<std::string, std::string> readFile(const std::filesystem::path& file_path);

    // Ghi nội dung vào file (sẽ cần dùng cho FileTool & Trajectory sau này)
    static std::expected<void, std::string> writeFile(const std::filesystem::path& file_path, const std::string& content);
};