#include "evaluator.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <format>

FunctionalEvaluator::FunctionalEvaluator(int timeoutSeconds)
    : _timeoutSeconds(timeoutSeconds) {}

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

    std::cout << std::format("  [Eval: Functional] Running test script: {}\n", script);

    // Thực thi script kiểm thử thực tế trên OS (kiểm tra exit code 0 là PASS)
    int exitCode = std::system(script.c_str());

    if (exitCode == 0) {
        std::cout << "  [Eval: Functional] Result: PASS (Exit code 0)\n";
        return true;
    } else {
        std::cout << std::format("  [Eval: Functional] Result: FAIL (Exit code {})\n", exitCode);
        return false;
    }
}

// ==========================================
// Factory Implementation
// ==========================================
std::unique_ptr<Evaluator> EvaluatorFactory::create(const std::string& evalType) {
    if (evalType == "functional") {
        return std::make_unique<FunctionalEvaluator>();
    }
    // Mặc định trả về KeywordEvaluator
    return std::make_unique<KeywordEvaluator>();
}