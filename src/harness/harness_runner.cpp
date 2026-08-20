#include "harness_runner.h"
#include "evaluator.h" // Tích hợp Evaluator Strategy & Factory

#include <iostream>
#include <fstream>
#include <chrono>
#include <future>
#include <cstdlib>
#include <filesystem>
#include <format>

// ==========================================
// Implementation for BenchmarkTask
// ==========================================

BenchmarkTask BenchmarkTask::fromJson(const nlohmann::json& j) {
    BenchmarkTask task;
    task.id = j.value("id", "task_unknown");
    task.difficulty = j.value("difficulty", "simple");
    task.description = j.value("description", "");
    task.instruction = j.value("instruction", "");
    task.evalType = j.value("eval_type", "keyword");
    task.evalScript = j.value("eval_script", "");
    task.setupScript = j.value("setup_script", "");
    task.maxSteps = j.value("max_steps", 10);
    task.timeoutSeconds = j.value("timeout_seconds", 30);

    if (j.contains("expected_keywords") && j["expected_keywords"].is_array()) {
        task.expectedKeywords = j["expected_keywords"].get<std::vector<std::string>>();
    }
    return task;
}

// ==========================================
// Implementation for HarnessRunner
// ==========================================

HarnessRunner::HarnessRunner(std::shared_ptr<LLMClient> client, std::string outputDir)
    : _client(std::move(client)), _outputDir(std::move(outputDir)) {
    // Đảm bảo thư mục lưu trữ kết quả tồn tại
    if (!std::filesystem::exists(_outputDir)) {
        std::filesystem::create_directories(_outputDir);
    }
}

std::expected<bool, std::string> HarnessRunner::loadTasks(const std::string& tasksFilePath) {
    std::ifstream file(tasksFilePath);
    if (!file.is_open()) {
        return std::unexpected(std::format("Cannot open tasks file: {}", tasksFilePath));
    }

    try {
        nlohmann::json root;
        file >> root;

        _tasks.clear();
        if (root.is_array()) {
            for (const auto& item : root) {
                _tasks.push_back(BenchmarkTask::fromJson(item));
            }
        } else {
            return std::unexpected("Tasks JSON root must be an array.");
        }
    } catch (const std::exception& e) {
        return std::unexpected(std::format("JSON parse error: {}", e.what()));
    }

    return true;
}

Trajectory HarnessRunner::runTask(const BenchmarkTask& task) {
    std::cout << "\n======================================================\n";
    std::cout << std::format("[HarnessRunner] Running Task: {} ({})\n", task.id, task.difficulty);
    std::cout << std::format("Instruction: {}\n", task.instruction);
    std::cout << "======================================================\n";

    // 1. Chạy setup_script nếu có yêu cầu chuẩn bị môi trường/file mẫu
    if (!task.setupScript.empty()) {
        std::cout << std::format("[Setup] Executing: {}\n", task.setupScript);
        std::system(task.setupScript.c_str());
    }

    // 2. Khởi tạo đối tượng Trajectory để lưu vết
    std::string modelName = "meta/llama-3.2-11b-vision-instruct";
    Trajectory trajectory(task.id, modelName, task.instruction);

    // 3. Khởi tạo AgentLoop và tiêm Hook Callback (Observer Pattern)
    AgentLoop agent;
    agent.setStepHook([&trajectory](const StepData& step) {
        std::cout << std::format("  -> [Hook Captured] Step {}: Tool '{}' (Latency: {}ms)\n", 
                                 step.stepNumber, step.actionName, step.latencyMs);
        trajectory.addStep(step);
    });

    auto startTime = std::chrono::steady_clock::now();

    // 4. Quản lý Timeout qua std::async
    auto agentFuture = std::async(std::launch::async, [&]() {
        return agent.run(task.instruction, _client);
    });

    std::future_status status = agentFuture.wait_for(std::chrono::seconds(task.timeoutSeconds));

    auto endTime = std::chrono::steady_clock::now();
    int64_t totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    trajectory.setTotalTimeMs(totalDurationMs);

    // 5. Thu thập kết quả & Thực hiện Đánh giá (Evaluate)
    if (status == std::future_status::timeout) {
        std::cerr << std::format("[ERROR]: Task {} timed out after {}s!\n", task.id, task.timeoutSeconds);
        trajectory.setFinalOutput(std::format("[ERROR]: Timeout exceeded ({}s)", task.timeoutSeconds));
        trajectory.setSuccess(false);
    } else {
        auto runResult = agentFuture.get();
        if (runResult.has_value()) {
            trajectory.setFinalOutput(*runResult);
            std::cout << std::format("[HarnessRunner] Agent finished execution.\n");

            // ĐÁNH GIÁ TỰ ĐỘNG BẰNG EVALUATOR STRATEGY
            auto evaluator = EvaluatorFactory::create(task.evalType);
            
            nlohmann::json taskConfig;
            taskConfig["expected_keywords"] = task.expectedKeywords;
            taskConfig["eval_script"] = task.evalScript;

            bool isPassed = evaluator->evaluate(trajectory.getFinalOutput(), taskConfig);
            trajectory.setSuccess(isPassed);

            std::cout << std::format("[HarnessRunner] Evaluation Result: {}\n", isPassed ? "PASSED (SUCCESS)" : "FAILED");
        } else {
            trajectory.setFinalOutput(std::format("[AgentLoop Failed]: {}", runResult.error()));
            trajectory.setSuccess(false);
            std::cerr << std::format("[ERROR]: AgentLoop error: {}\n", runResult.error());
        }
    }

    // 6. Xuất báo cáo vết thực thi ra file JSON (trajectory_{task_id}.json)
    std::string exportPath = std::format("{}/trajectory_{}.json", _outputDir, task.id);
    auto exportRes = trajectory.exportToJson(exportPath);
    if (exportRes.has_value()) {
        std::cout << std::format("[HarnessRunner] Log exported: {}\n", exportPath);
    } else {
        std::cerr << std::format("[ERROR]: Failed to export JSON: {}\n", exportRes.error());
    }

    return trajectory;
}

std::vector<Trajectory> HarnessRunner::runBatch() {
    std::vector<Trajectory> results;
    results.reserve(_tasks.size());

    int passedCount = 0;
    std::cout << std::format("\n>>> Starting Benchmark Batch (Total: {} tasks) <<<\n", _tasks.size());

    for (const auto& task : _tasks) {
        Trajectory traj = runTask(task);
        if (traj.isSuccess()) {
            passedCount++;
        }
        results.push_back(std::move(traj));
    }

    // Tính tỷ lệ thành công
    double successRate = _tasks.empty() ? 0.0 : (static_cast<double>(passedCount) / _tasks.size()) * 100.0;

    std::cout << "\n======================================================\n";
    std::cout << "               BENCHMARK BATCH SUMMARY                \n";
    std::cout << "======================================================\n";
    std::cout << std::format("Total Tasks Executed : {}\n", results.size());
    std::cout << std::format("Tasks Passed         : {}\n", passedCount);
    std::cout << std::format("Tasks Failed         : {}\n", results.size() - passedCount);
    std::cout << std::format("Success Rate (Acc)   : {:.2f}%\n", successRate);
    std::cout << "======================================================\n";

    return results;
}

const std::vector<BenchmarkTask>& HarnessRunner::getTasks() const {
    return _tasks;
}

void HarnessRunner::setOutputDir(const std::string& outputDir) {
    _outputDir = outputDir;
    if (!std::filesystem::exists(_outputDir)) {
        std::filesystem::create_directories(_outputDir);
    }
}