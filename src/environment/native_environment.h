#ifndef NATIVE_ENVIRONMENT_H
#define NATIVE_ENVIRONMENT_H

#include "environment.h"
#include <memory>

/**
 * @brief Môi trường thực thi trực tiếp trên Host OS
 */
class NativeEnvironment : public Environment {
private:
    std::unique_ptr<ShellExecutor> _executor;

public:
    NativeEnvironment();
    ~NativeEnvironment() override = default;

    std::expected<void, std::string> setup(const std::string& setup_script) override;
    std::expected<ExecResult, std::string> execute(const std::string& command) override;
    void teardown() override;
    std::filesystem::path getWorkingDirectory() const override;
};

#endif // NATIVE_ENVIRONMENT_H
