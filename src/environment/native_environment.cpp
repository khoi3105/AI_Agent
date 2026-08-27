#include "native_environment.h"

#ifdef _WIN32
#include "../tools/exec_tool/windows_shell_executor.h"
#else
#include "../tools/exec_tool/linux_shell_executor.h"
#endif

#include <iostream>
#include <format>

NativeEnvironment::NativeEnvironment() {
#ifdef _WIN32
    _executor = std::make_unique<WindowsShellExecutor>();
#else
    _executor = std::make_unique<LinuxShellExecutor>();
#endif
}

std::expected<void, std::string> NativeEnvironment::setup(const std::string& setup_script) {
    if (setup_script.empty()) {
        return {};
    }

    auto res = _executor->execute(setup_script);
    if (!res.has_value()) {
        return std::unexpected(std::format("[NativeEnvironment Setup Error]: {}", res.error()));
    }

    if (res->exit_code != 0) {
        return std::unexpected(std::format("[NativeEnvironment Setup Failed] Exit code {}: {}", 
                                            res->exit_code, res->stderr_text));
    }

    return {};
}

std::expected<ExecResult, std::string> NativeEnvironment::execute(const std::string& command) {
    return _executor->execute(command);
}

void NativeEnvironment::teardown() {
    // Với NativeEnvironment, không tự động xoá file để người dùng kiểm tra kết quả nếu cần
}

std::filesystem::path NativeEnvironment::getWorkingDirectory() const {
    return std::filesystem::current_path();
}
