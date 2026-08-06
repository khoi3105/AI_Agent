#ifndef CSV_READER_H
#define CSV_READER_H

#include "ifile_reader.h"

class CsvReader : public IFileReader {
public:
    CsvReader() = default;
    ~CsvReader() override = default;

    std::expected<std::string, std::string>
    read(const std::filesystem::path& path) override;
};

#endif