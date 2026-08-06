#ifndef FILE_READER_REGISTRY_H
#define FILE_READER_REGISTRY_H

#include "../reader/ifile_reader.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class FileReaderRegistry {
public:
    FileReaderRegistry();
    std::unique_ptr<IFileReader>
    create(const std::string& extension) const;

private:
    using Creator = std::function<std::unique_ptr<IFileReader>()>;
    std::unordered_map<std::string, Creator> _registry;
};

#endif