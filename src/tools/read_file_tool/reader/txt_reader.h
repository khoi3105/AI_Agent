#ifndef TXT_READER_H
#define TXT_READER_H

#include "ifile_reader.h"

class TxtReader : public IFileReader {
public:
    TxtReader() = default;
    ~TxtReader() override = default;

    std::expected<std::string, std::string>
    read(const std::filesystem::path& path) override;
};

#endif // TXT_READER_H