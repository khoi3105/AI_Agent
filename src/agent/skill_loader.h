#ifndef SKILL_LOADER_H
#define SKILL_LOADER_H

#include <string>
#include <vector>
#include <filesystem>
#include <expected>

class SkillLoader {
private:
    std::filesystem::path _skillsDir;

    // Helper kiểm tra từ khóa (keyword matching không phân biệt hoa/thường)
    bool isKeywordMatched(const std::string& task, const std::string& skill_content);
    std::string toLower(const std::string& str);

public:
    explicit SkillLoader(const std::string& skills_dir = "skills") 
        : _skillsDir(skills_dir) {}

    // 1. Load tất cả file .md trong thư mục skills/
    std::expected<std::string, std::string> loadAllSkills();
    
    // 2. Load 1 skill cụ thể theo tên file
    std::expected<std::string, std::string> loadSkill(const std::string& filename);

    // 3. Quét task của User, lọc ra các file .md có keyword khớp và nối lại
    std::expected<std::string, std::string> selectSkillsForTask(const std::string& user_task);
};

#endif // SKILL_LOADER_H