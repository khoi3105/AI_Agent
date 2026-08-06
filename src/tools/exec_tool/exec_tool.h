#ifndef EXEC_TOOL_H
#define EXEC_TOOL_H

#include "../tool.h"
#include "shell_executor.h"

#include <memory>

class ExecTool : public Tool
{
public:
    ExecTool();
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& args) override;
    nlohmann::json get_schema() const override;

private:
    std::unique_ptr<ShellExecutor> _executor;
};

#endif