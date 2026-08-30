
#include "tool_policy.h"

#include <algorithm>
#include <cctype>
#include <vector>

std::expected<void, std::string> ToolPolicy::validate(const std::string& tool_name, const nlohmann::json& args) const {
    // Kiem tra tool
    if (!isAllowedTool(tool_name)) {
        return std::unexpected("[ToolPolicy] Tool '" + tool_name + "' không được phép sử dụng.");
    }

    // Kiểm tra args
    

    if (!args.is_object()) {
        return std::unexpected("[ToolPolicy] args phải là JSON object.");
    }

    auto result = validateArgs(tool_name, args);

    if (!result) {
        return result;
    }

    // ==============================
    // 3. Policy riêng cho exec
    // ==============================

    if (tool_name == "exec") {

        return validateExecCommand(args["command"].get<std::string>());
    }

    return {};
}

bool ToolPolicy::isAllowedTool(const std::string& tool_name) const {
    return
        tool_name == "calculator" ||
        tool_name == "read_file" ||
        tool_name == "write_file" ||
        tool_name == "exec" ||
        tool_name == "weather" ||
        tool_name == "memory_save" ||
        tool_name == "memory_search" ||
        tool_name == "web_search" ||
        tool_name == "capture_screenshot" ||
        tool_name == "gui_action";
}

std::expected<void, std::string> ToolPolicy::validateArgs(const std::string& tool_name, const nlohmann::json& args) const {
    if (tool_name == "calculator") {

        if (!args.contains("expression")) {
            return std::unexpected(
                "[ToolPolicy] calculator thiếu 'expression'."
            );
        }

        if (!args["expression"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] calculator.expression "
                "phải là string."
            );
        }
    }

    else if (tool_name == "read_file") {

        if (!args.contains("path")) {
            return std::unexpected(
                "[ToolPolicy] read_file thiếu 'path'."
            );
        }

        if (!args["path"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] read_file.path "
                "phải là string."
            );
        }
    }

    else if (tool_name == "write_file") {

        if (!args.contains("path")) {
            return std::unexpected(
                "[ToolPolicy] write_file thiếu 'path'."
            );
        }

        if (!args.contains("content")) {
            return std::unexpected(
                "[ToolPolicy] write_file thiếu 'content'."
            );
        }

        if (!args["path"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] write_file.path "
                "phải là string."
            );
        }

        if (!args["content"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] write_file.content "
                "phải là string."
            );
        }
    }

    else if (tool_name == "exec") {

        if (!args.contains("command")) {
            return std::unexpected(
                "[ToolPolicy] exec thiếu 'command'."
            );
        }

        if (!args["command"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] exec.command "
                "phải là string."
            );
        }

        if (args["command"].get<std::string>().empty()) {
            return std::unexpected(
                "[ToolPolicy] exec.command "
                "không được rỗng."
            );
        }
    }

    else if (tool_name == "weather") {

        if (!args.contains("city")) {
            return std::unexpected(
                "[ToolPolicy] weather thiếu 'city'."
            );
        }

        if (!args["city"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] weather.city "
                "phải là string."
            );
        }
    }

    else if (tool_name == "memory_save") {

        if (!args.contains("content")) {
            return std::unexpected(
                "[ToolPolicy] memory_save thiếu 'content'."
            );
        }

        if (!args["content"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] memory_save.content "
                "phải là string."
            );
        }
    }

    else if (tool_name == "memory_search") {

        if (!args.contains("query")) {
            return std::unexpected(
                "[ToolPolicy] memory_search thiếu 'query'."
            );
        }

        if (!args["query"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] memory_search.query "
                "phải là string."
            );
        }
    }

    else if (tool_name == "web_search") {

        if (!args.contains("query")) {
            return std::unexpected(
                "[ToolPolicy] web_search thiếu 'query'."
            );
        }

        if (!args["query"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] web_search.query "
                "phải là string."
            );
        }
    }

    else if (tool_name == "capture_screenshot") {
        if (args.contains("output_path") && !args["output_path"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] capture_screenshot.output_path phải là string."
            );
        }
    }

    else if (tool_name == "gui_action") {
        if (!args.contains("action") || !args["action"].is_string()) {
            return std::unexpected(
                "[ToolPolicy] gui_action thiếu tham số 'action'."
            );
        }
    }

    return {};
}

std::expected<void, std::string> ToolPolicy::validateExecCommand(const std::string& command) const {
    // Chuyển command về lowercase
    std::string lower = command;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char c) {
            return static_cast<char>(
                std::tolower(c)
            );
        }
    );

    // Các command / pattern nguy hiểm

    static const std::vector<std::string> forbidden = {

        // Power
        "shutdown",
        "poweroff",
        "reboot",
        "halt",

        // Disk / filesystem
        "mkfs",
        "fdisk",
        "parted",
        "diskpart",
        "dd if=",

        // Dangerous delete
        "rm -rf",
        "rm -r",
        "rmdir",

        // Permission / ownership
        "chmod 777",
        "chmod -r 777",
        "chown",

        // User / group management
        "userdel",
        "groupdel",
        "passwd",

        // Service management
        "systemctl",
        "service",

        // Network firewall
        "iptables",
        "ip6tables",
        "nft",

        // Process killing
        "kill -9",
        "pkill",
        "killall",

        // Mounting
        "mount",
        "umount"
    };

    for (const auto& forbiddenCommand : forbidden) {

        if (lower.find(forbiddenCommand)
            != std::string::npos) {

            return std::unexpected(
                "[ExecPolicy] Command bị cấm: '" +
                forbiddenCommand + "'"
            );
        }
    }

    // Fork bomb

    if (lower.find(":(){") != std::string::npos ||
        lower.find(":()") != std::string::npos) {

        return std::unexpected(
            "[ExecPolicy] Phát hiện pattern "
            "fork bomb."
        );
    }

    return {};
}