#ifndef IFILE_READER_H
#define IFILE_READER_H

#include <expected>
#include <filesystem>
#include <string>

class IFileReader {
public:
    virtual ~IFileReader() = default;

    virtual std::expected<std::string,std::string>
    read(const std::filesystem::path& path) = 0;
};

#endif