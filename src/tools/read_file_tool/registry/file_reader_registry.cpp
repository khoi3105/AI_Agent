#include "file_reader_registry.h"

#include "../reader/txt_reader.h"
#include "../reader/csv_reader.h"
#include "../reader/xml_reader.h"
#include "../reader/pdf_reader.h"

FileReaderRegistry::FileReaderRegistry() {
    //TXT
    _registry[".txt"] = []()
    {
        return std::make_unique<TxtReader>();
    };
    //CSV
    _registry[".csv"] = []()
    {
        return std::make_unique<CsvReader>();
    };
    //XML
    _registry[".xml"] = []()
    {
        return std::make_unique<XmlReader>();
    };
    //PDF
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