#include "ifile_reader.h"
#include <expected>

class TextFileReader : public IFileReader
{
public:
    std::expected<std::string, std::string>
    read(const std::filesystem::path& path) override;
};