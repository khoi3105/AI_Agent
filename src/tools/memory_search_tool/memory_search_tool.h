#ifndef MEMORY_SEARCH_TOOL_H
#define MEMORY_SEARCH_TOOL_H

#include "../tool.h"
#include <sqlite3.h>

class MemorySearchTool : public Tool {
private:
    sqlite3* _db;

public:
    explicit MemorySearchTool();
    ~MemorySearchTool() override;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& args) override;
    nlohmann::json get_schema() const override;
};

#endif // MEMORY_SEARCH_TOOL_H