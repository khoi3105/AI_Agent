#include "exec_tool.h"
#include "../../environment/native_environment.h"

#include <nlohmann/json.hpp>
#include <memory>

ExecTool::ExecTool(std::shared_ptr<Environment> env)
    : _env(std::move(env))
{
    if (!_env) {
        _env = std::make_shared<NativeEnvironment>();
    }
}

std::string ExecTool::getName() const {
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

        auto result = _env->execute(command);
        if (!result.has_value()) {
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
            "tool_call"
        },
        {
            "tool_call",
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