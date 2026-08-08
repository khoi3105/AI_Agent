#ifndef MEMORY_SAVE_TOOL_H
#define MEMORY_SAVE_TOOL_H

#include "../tool.h"
#include <sqlite3.h>

class MemorySaveTool : public Tool {
private:
    sqlite3* _db;
    bool initDatabase();

public:
    explicit MemorySaveTool();
    ~MemorySaveTool() override;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& args) override;
    nlohmann::json get_schema() const override;
};

#endif // MEMORY_SAVE_TOOL_H