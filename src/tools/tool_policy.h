#pragma once

#include <expected>
#include <string>
#include <nlohmann/json.hpp>

class ToolPolicy {
public:
    std::expected<void, std::string> validate(const std::string& tool_name, const nlohmann::json& args) const;

private:
    bool isAllowedTool(const std::string& tool_name) const;

    std::expected<void, std::string> validateArgs(const std::string& tool_name, const nlohmann::json& args) const;

    std::expected<void, std::string> validateExecCommand( const std::string& command) const;
};