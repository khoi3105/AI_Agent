#ifndef WINDOWS_SHELL_EXECUTOR_H
#define WINDOWS_SHELL_EXECUTOR_H

#include "shell_executor.h"

class WindowsShellExecutor : public ShellExecutor
{
public:
    std::expected<ExecResult, std::string>
    execute(const std::string& command) override;
};

#endif