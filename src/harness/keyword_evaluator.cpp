#include "evaluator.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <format>

namespace {
    // Hàm phụ trợ chuyển chuỗi về dạng viết thường (lowercase)
    std::string toLower(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return result;
    }
}

KeywordEvaluator::KeywordEvaluator(bool caseSensitive)
    : _caseSensitive(caseSensitive) {}

bool KeywordEvaluator::evaluate(const std::string& agentOutput, const nlohmann::json& taskConfig) {
    if (!taskConfig.contains("expected_keywords") || !taskConfig["expected_keywords"].is_array()) {
        std::cerr << "[KeywordEvaluator] Warning: 'expected_keywords' missing or not an array.\n";
        return false;
    }

    std::vector<std::string> expectedKeywords = taskConfig["expected_keywords"].get<std::vector<std::string>>();
    if (expectedKeywords.empty()) {
        return true;
    }

    std::string textToSearch = _caseSensitive ? agentOutput : toLower(agentOutput);

    // Kiểm tra xem có ít nhất một hoặc tất cả các từ khóa xuất hiện
    // Mặc định: Agent thành công nếu chứa ít nhất 1 từ khóa hợp lệ trong tập expected_keywords
    for (const auto& kw : expectedKeywords) {
        std::string target = _caseSensitive ? kw : toLower(kw);
        if (textToSearch.find(target) != std::string::npos) {
            std::cout << std::format("  [Eval: Keyword] Matched keyword: '{}'\n", kw);
            return true;
        }
    }

    std::cout << "  [Eval: Keyword] No expected keywords found in output.\n";
    return false;
}