#ifndef MEMORY_SEARCH_TOOL_H
#define MEMORY_SEARCH_TOOL_H

#include "../tool.h"
#include <sqlite3.h>
#include <string>
#include <vector>

class MemorySearchTool : public Tool {
private:
    sqlite3* _db;
    bool initDatabase();
    std::vector<std::string> tokenize(const std::string& text) const;

public:
    explicit MemorySearchTool();
    ~MemorySearchTool() override;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;
};

#endif // MEMORY_SEARCH_TOOL_H
