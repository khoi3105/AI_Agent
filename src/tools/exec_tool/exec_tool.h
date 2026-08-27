#ifndef EXEC_TOOL_H
#define EXEC_TOOL_H

#include "../tool.h"
#include "../../environment/environment.h"

#include <memory>

class ExecTool : public Tool
{
public:
    explicit ExecTool(std::shared_ptr<Environment> env = nullptr);
    ~ExecTool() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;

    void setEnvironment(std::shared_ptr<Environment> env) {
        _env = std::move(env);
    }

private:
    std::shared_ptr<Environment> _env;
};

#endif // EXEC_TOOL_H