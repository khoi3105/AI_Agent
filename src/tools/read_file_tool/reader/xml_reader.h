#ifndef XML_READER_H
#define XML_READER_H

#include "ifile_reader.h"

class XmlReader : public IFileReader {
public:
    XmlReader() = default;
    ~XmlReader() override = default;

    std::expected<std::string, std::string>
    read(const std::filesystem::path& path) override;
};
#endif