#include "textfile_reader.h"
#include "../../../utils/read_write_textfile.h"

std::expected<std::string, std::string>
TextFileReader::read(const std::filesystem::path& path)
{
    return FileUtils::readFile(path);
}