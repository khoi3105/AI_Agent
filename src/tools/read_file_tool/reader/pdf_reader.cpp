#include "pdf_reader.h"

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

std::expected<std::string, std::string>
PdfReader::read(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return std::unexpected("File không tồn tại: " + path.string());
    }
    auto document = poppler::document::load_from_file(path.string());
    if (!document) {
        return std::unexpected("Thất bại khi mở filr PDF: " + path.string());
    }
    std::string text;
    text.reserve(document->pages() * 1024);   
    for (int i = 0; i < document->pages(); ++i) {
        std::unique_ptr<poppler::page> page(document->create_page(i));
        if (!page) continue;
        text += "\n===== PAGE ";
        text += std::to_string(i + 1);
        text += " =====\n";
        auto utf8 = page->text().to_utf8();
        text.append(utf8.begin(), utf8.end());
        text += '\n';
        text += '\n';
    }
    if (text.empty()) {
        return std::unexpected("PDF không chứa nội dung bao gồm có văn bản(text)!");
    }
    return text;
}