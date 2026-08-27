#include "evaluator.h"
#include "../environment/native_environment.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <format>

FunctionalEvaluator::FunctionalEvaluator(std::shared_ptr<Environment> env, int timeoutSeconds)
    : _env(std::move(env)), _timeoutSeconds(timeoutSeconds) {
    if (!_env) {
        _env = std::make_shared<NativeEnvironment>();
    }
}

bool FunctionalEvaluator::evaluate(const std::string& agentOutput, const nlohmann::json& taskConfig) {
    if (!taskConfig.contains("eval_script") || !taskConfig["eval_script"].is_string()) {
        std::cerr << "[FunctionalEvaluator] Warning: 'eval_script' not specified in task config.\n";
        return false;
    }

    std::string script = taskConfig["eval_script"].get<std::string>();
    if (script.empty()) {
        std::cerr << "[FunctionalEvaluator] Warning: Empty 'eval_script'.\n";
        return false;
    }

    std::cout << std::format("  [Eval: Functional] Running test script in environment: {}\n", script);

    // Thực thi script kiểm thử thông qua Environment
    auto execRes = _env->execute(script);

    if (execRes.has_value() && execRes->exit_code == 0) {
        std::cout << "  [Eval: Functional] Result: PASS (Exit code 0)\n";
        return true;
    } else {
        int exitCode = execRes.has_value() ? execRes->exit_code : -1;
        std::string err = execRes.has_value() ? execRes->stderr_text : execRes.error();
        std::cout << std::format("  [Eval: Functional] Result: FAIL (Exit code {}): {}\n", exitCode, err);
        return false;
    }
}

// ==========================================
// Factory Implementation
// ==========================================
std::unique_ptr<Evaluator> EvaluatorFactory::create(const std::string& evalType, std::shared_ptr<Environment> env) {
    if (evalType == "functional") {
        return std::make_unique<FunctionalEvaluator>(std::move(env));
    }
    // Mặc định trả về KeywordEvaluator
    return std::make_unique<KeywordEvaluator>();
}