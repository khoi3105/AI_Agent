#ifndef WRITE_FILE_TOOL_H
#define WRITE_FILE_TOOL_H

#include "../tool.h"
#include <string>


class WriteFileTool : public Tool {
public:
    WriteFileTool() = default;
    ~WriteFileTool() override = default;
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& args) override;
    nlohmann::json get_schema() const override;
};

#endif // WRITE_FILE_TOOL_H