#include "loader_file.h"

std::expected<std::string, std::string>
FileLoader::load(const std::filesystem::path& path)
{
    auto reader = _registry.create(path.extension().string());
    if (!reader)
        return std::unexpected("Unsupported file type.");
    return reader->read(path);
}