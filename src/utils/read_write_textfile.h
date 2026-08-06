#ifndef READ_WRITE_TEXTFILE_H
#define READ_WRITE_TEXTFILE_H

#include <expected>
#include <string>
#include <filesystem>

class FileUtils {
public:
    // Đọc toàn bộ nội dung file văn bản
    static std::expected<std::string, std::string> readFile(const std::filesystem::path& file_path);

    // Ghi nội dung vào file
    static std::expected<void, std::string> writeFile(const std::filesystem::path& file_path, const std::string& content);
};

#endif