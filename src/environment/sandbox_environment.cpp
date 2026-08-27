#include "sandbox_environment.h"

#ifdef _WIN32
#include "../tools/exec_tool/windows_shell_executor.h"
#else
#include "../tools/exec_tool/linux_shell_executor.h"
#endif

#include <iostream>
#include <format>

SandboxEnvironment::SandboxEnvironment(std::filesystem::path sandbox_dir)
    : _sandboxDir(std::move(sandbox_dir)) {
#ifdef _WIN32
    _executor = std::make_unique<WindowsShellExecutor>();
#else
    _executor = std::make_unique<LinuxShellExecutor>();
#endif

    if (!std::filesystem::exists(_sandboxDir)) {
        std::filesystem::create_directories(_sandboxDir);
    }
}

SandboxEnvironment::~SandboxEnvironment() {
    teardown();
}

std::expected<void, std::string> SandboxEnvironment::setup(const std::string& setup_script) {
    if (!std::filesystem::exists(_sandboxDir)) {
        std::filesystem::create_directories(_sandboxDir);
    }

    if (setup_script.empty()) {
        return {};
    }

    // Thực thi lệnh setup bên trong thư mục sandbox
    std::string wrapped_cmd = std::format("cd \"{}\" && {}", _sandboxDir.string(), setup_script);
    auto res = _executor->execute(wrapped_cmd);
    if (!res.has_value()) {
        return std::unexpected(std::format("[SandboxEnvironment Setup Error]: {}", res.error()));
    }

    if (res->exit_code != 0) {
        return std::unexpected(std::format("[SandboxEnvironment Setup Failed] Exit code {}: {}", 
                                            res->exit_code, res->stderr_text));
    }

    return {};
}

std::expected<ExecResult, std::string> SandboxEnvironment::execute(const std::string& command) {
    if (!std::filesystem::exists(_sandboxDir)) {
        std::filesystem::create_directories(_sandboxDir);
    }

    // Luôn bao bọc lệnh thực thi bên trong thư mục sandbox
    std::string wrapped_cmd = std::format("cd \"{}\" && {}", _sandboxDir.string(), command);
    return _executor->execute(wrapped_cmd);
}

void SandboxEnvironment::teardown() {
    try {
        if (std::filesystem::exists(_sandboxDir)) {
            std::filesystem::remove_all(_sandboxDir);
            std::filesystem::create_directories(_sandboxDir);
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("[SandboxEnvironment Teardown Error]: {}\n", e.what());
    }
}

std::filesystem::path SandboxEnvironment::getWorkingDirectory() const {
    return std::filesystem::absolute(_sandboxDir);
}
