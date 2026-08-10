#include "exec_tool.h"

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include "windows_shell_executor.h"
#else
#include "linux_shell_executor.h"
#endif

#include <memory>

ExecTool::ExecTool()
{
#ifdef _WIN32
    _executor = std::make_unique<WindowsShellExecutor>();
#else
    _executor = std::make_unique<LinuxShellExecutor>();
#endif
}

std::string ExecTool::getName() const{
    return "exec";
}

std::string ExecTool::getDescription() const {
    return "Execute a shell command and return stdout, stderr and exit code.";
}

std::string ExecTool::execute(const nlohmann::json& args)
{
    try {
        if (!args.contains("command") || !args["command"].is_string()) {
            return "[Lỗi ExecTool]: Tham số 'command' không hợp lệ hoặc bị thiếu.";
        }

        std::string command = args["command"].get<std::string>();

        auto result = _executor->execute(command);
        if (!result) {
            return result.error();
        }

        nlohmann::json response = {
            {"exit_code", result->exit_code},
            {"stdout", result->stdout_text},
            {"stderr", result->stderr_text}
        };
        return response.dump(4);
    }
    catch (const std::exception& e) {
        return std::string("[ExecTool Exception]: ") + e.what();
    }
}

nlohmann::json ExecTool::get_schema() const
{
    return {
        {
            "type",
            "function"
        },
        {
            "function",
            {
                {
                    "name",
                    getName()
                },
                {
                    "description",
                    getDescription()
                },
                {
                    "parameters",
                    {
                        {
                            "type",
                            "object"
                        },
                        {
                            "properties",
                            {
                                {
                                    "command",
                                    {
                                        {
                                            "type",
                                            "string"
                                        },
                                        {
                                            "description",
                                            "Shell command to execute."
                                        }
                                    }
                                }
                            }
                        },
                        {
                            "required",
                            {
                                "command"
                            }
                        }
                    }
                }
            }
        }
    };
}