#include "file_reader_registry.h"

#include "../reader/textfile_reader.h"
#include "../reader/pdf_reader.h"

FileReaderRegistry::FileReaderRegistry()
{
    auto textReaderFactory = []()
    {
        return std::make_unique<TextFileReader>();
    };

    // Text files
    _registry[".txt"]  = textReaderFactory;
    _registry[".csv"]  = textReaderFactory;
    _registry[".xml"]  = textReaderFactory;
    _registry[".json"] = textReaderFactory;
    _registry[".md"]   = textReaderFactory;
    _registry[".log"]  = textReaderFactory;
    _registry[".ini"]  = textReaderFactory;
    _registry[".yaml"] = textReaderFactory;
    _registry[".yml"]  = textReaderFactory;
    _registry[".cpp"]  = textReaderFactory;
    _registry[".hpp"]  = textReaderFactory;
    _registry[".h"]    = textReaderFactory;
    _registry[".c"]    = textReaderFactory;
    _registry[".cc"]   = textReaderFactory;
    _registry[".py"]   = textReaderFactory;
    _registry[".java"] = textReaderFactory;
    _registry[".js"]   = textReaderFactory;
    _registry[".ts"]   = textReaderFactory;
    _registry[".html"] = textReaderFactory;
    _registry[".css"]  = textReaderFactory;
    _registry[".sql"]  = textReaderFactory;
    _registry[".sh"]   = textReaderFactory;
    _registry[".bat"]  = textReaderFactory;

    // PDF
    _registry[".pdf"] = []()
    {
        return std::make_unique<PdfReader>();
    };
}

std::unique_ptr<IFileReader> FileReaderRegistry::create( const std::string& extension) const {
    auto it = _registry.find(extension);
    if(it == _registry.end())
        return nullptr;
    return it->second();
}