#ifndef PDF_READER_H
#define PDF_READER_H

#include "ifile_reader.h"
class PdfReader : public IFileReader {
public:
    PdfReader() = default;
    ~PdfReader() override = default;
    std::expected<std::string, std::string>
    read(const std::filesystem::path& path) override;
};
#endif