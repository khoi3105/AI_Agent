#ifndef LINUX_SHELL_EXECUTOR_H
#define LINUX_SHELL_EXECUTOR_H

#include "shell_executor.h"

class LinuxShellExecutor : public ShellExecutor
{
public:
    std::expected<ExecResult, std::string>
    execute(const std::string& command) override;
};

#endif