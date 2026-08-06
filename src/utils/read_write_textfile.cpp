#include "read_write_textfile.h"
#include <fstream>
#include <sstream>

std::expected<std::string, std::string>
FileUtils::readFile(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected("Không thể mở file: " + file_path.string());
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad())
    {
        return std::unexpected("Gặp lỗi trong khi đọc file: " + file_path.string());
    }
    return buffer.str();
}

std::expected<void, std::string>
FileUtils::writeFile(const std::filesystem::path& file_path, const std::string& content)
{
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected("Không thể mở file: " + file_path.string());
    }
    file << content;
    if (!file)
    {
        return std::unexpected("Thất bại trong khi ghi file: " + file_path.string());
    }
    return {};
}