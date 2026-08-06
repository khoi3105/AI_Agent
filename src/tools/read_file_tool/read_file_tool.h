#ifndef READ_FILE_TOOL_H
#define READ_FILE_TOOL_H

#include "../tool.h"
#include "loader/loader_file.h"
#include <string>

class ReadFileTool : public Tool {
private:
    FileLoader _loader;
public:
    ReadFileTool() = default;
    ~ReadFileTool() override = default;

    std::string getName() const override;

    std::string getDescription() const override;

    std::string execute(const std::string& args) override;

    nlohmann::json get_schema() const override;
};

#endif // READ_FILE_TOOL_H