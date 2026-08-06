#ifndef SHELL_EXECUTOR_H
#define SHELL_EXECUTOR_H

#include <expected>
#include <string>

struct ExecResult
{
    int exit_code;
    std::string stdout_text;
    std::string stderr_text;
};

class ShellExecutor
{
public:
    virtual ~ShellExecutor() = default;

    virtual std::expected<ExecResult, std::string>
    execute(const std::string& command) = 0;
};

#endif