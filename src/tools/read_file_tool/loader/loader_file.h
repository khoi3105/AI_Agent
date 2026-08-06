#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "../registry/file_reader_registry.h"
#include <expected>
#include <string>
#include <filesystem>
class FileLoader {
public:
    std::expected<std::string, std::string> load(const std::filesystem::path& path);
private:
    FileReaderRegistry _registry;
};
#endif