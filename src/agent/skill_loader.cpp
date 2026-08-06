#include "skill_loader.h"
#include "../utils/read_write_textfile.h"
#include <algorithm>
#include <cctype>

std::string SkillLoader::toLower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

bool SkillLoader::isKeywordMatched(const std::string& user_task, const std::string& skill_content) {
    std::string lower_task = toLower(user_task);
    std::string lower_content = toLower(skill_content);

    size_t kw_pos = lower_content.find("keywords:");
    if (kw_pos == std::string::npos) {
        return false;
    }

    size_t line_end = lower_content.find("\n", kw_pos);
    std::string kw_line = lower_content.substr(kw_pos + 9, line_end - (kw_pos + 9));

    size_t start = 0;
    size_t end = kw_line.find(",");
    while (end != std::string::npos) {
        std::string token = kw_line.substr(start, end - start);
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);

        if (!token.empty() && lower_task.find(token) != std::string::npos) {
            return true;
        }
        start = end + 1;
        end = kw_line.find(",", start);
    }

    std::string last_token = kw_line.substr(start);
    last_token.erase(0, last_token.find_first_not_of(" \t\r\n"));
    last_token.erase(last_token.find_last_not_of(" \t\r\n") + 1);

    return (!last_token.empty() && lower_task.find(last_token) != std::string::npos);
}

// 1. Load toàn bộ skill .md
std::expected<std::string, std::string> SkillLoader::loadAllSkills() {
    if (!std::filesystem::exists(_skillsDir)) {
        return std::unexpected("[SkillLoader] Thư mục " + _skillsDir.string() + " không tồn tại!");
    }

    std::string combined_skills = "";
    bool has_loaded_any = false;

    for (const auto& entry : std::filesystem::directory_iterator(_skillsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            auto read_res = FileUtils::readFile(entry.path());
            
            if (read_res.has_value()) {
                combined_skills += "\n\n=== SKILL: " + entry.path().stem().string() + " ===\n";
                combined_skills += read_res.value();
                has_loaded_any = true;
            } else {
                return std::unexpected("[SkillLoader] Lỗi đọc file " + entry.path().string() + ": " + read_res.error());
            }
        }
    }

    if (!has_loaded_any) {
        return std::unexpected("[SkillLoader] Không tìm thấy file .md nào trong thư mục " + _skillsDir.string());
    }

    return combined_skills;
}

// 2. Load 1 file skill chỉ định
std::expected<std::string, std::string> SkillLoader::loadSkill(const std::string& filename) {
    std::filesystem::path file_path = _skillsDir / filename;
    if (file_path.extension() != ".md") {
        file_path += ".md";
    }

    auto read_res = FileUtils::readFile(file_path);
    if (!read_res.has_value()) {
        return std::unexpected("[SkillLoader] " + read_res.error());
    }
    
    return read_res.value();
}

// 3. Quét task từ User và chỉ load những Skill có Keyword phù hợp
std::expected<std::string, std::string> SkillLoader::selectSkillsForTask(const std::string& user_task) {
    if (!std::filesystem::exists(_skillsDir)) {
        return std::unexpected("[SkillLoader] Thư mục " + _skillsDir.string() + " không tồn tại!");
    }

    std::string selected_skills = "";

    for (const auto& entry : std::filesystem::directory_iterator(_skillsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            auto read_res = FileUtils::readFile(entry.path());
            
            if (read_res.has_value()) {
                std::string content = read_res.value();
                if (isKeywordMatched(user_task, content)) {
                    selected_skills += "\n\n=== APPLIED SKILL: " + entry.path().stem().string() + " ===\n";
                    selected_skills += content;
                }
            } else {
                return std::unexpected("[SkillLoader] Lỗi đọc file " + entry.path().string() + ": " + read_res.error());
            }
        }
    }

    // Nếu không khớp skill nào -> return chuỗi rỗng
    return selected_skills;
}