#ifndef SANDBOX_ENVIRONMENT_H
#define SANDBOX_ENVIRONMENT_H

#include "environment.h"
#include <memory>
#include <filesystem>

/**
 * @brief Môi trường thực thi cô lập trong thư mục sandbox chuyên biệt
 */
class SandboxEnvironment : public Environment {
private:
    std::filesystem::path _sandboxDir;
    std::unique_ptr<ShellExecutor> _executor;

public:
    explicit SandboxEnvironment(std::filesystem::path sandbox_dir = "sandbox_workspace");
    ~SandboxEnvironment() override;

    std::expected<void, std::string> setup(const std::string& setup_script) override;
    std::expected<ExecResult, std::string> execute(const std::string& command) override;
    void teardown() override;
    std::filesystem::path getWorkingDirectory() const override;
};

#endif // SANDBOX_ENVIRONMENT_H
