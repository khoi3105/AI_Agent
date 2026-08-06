#include "csv_reader.h"

#include <fstream>
#include <sstream>

std::expected<std::string, std::string>
CsvReader::read(const std::filesystem::path& path){
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(
            "Không thể mở file: " + path.string()
        );
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}