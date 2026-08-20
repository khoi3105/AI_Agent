#ifndef HARNESS_RUNNER_H
#define HARNESS_RUNNER_H

#include <string>
#include <vector>
#include <memory>
#include <expected>
#include <nlohmann/json.hpp>

#include "trajectory.h"
#include "../agent/AgentLoop.h"
#include "../client/llm_client.h"

// Cấu trúc đại diện cho một Task kiểm thử Benchmark
struct BenchmarkTask {
    std::string id;
    std::string difficulty;
    std::string description;
    std::string instruction;
    std::string evalType;             // "keyword" hoặc "functional"
    std::string evalScript;           // Dùng cho Functional Evaluation
    std::string setupScript;          // Thiết lập môi trường trước khi chạy
    std::vector<std::string> expectedKeywords; // Dùng cho Keyword Evaluation
    int maxSteps{10};
    int timeoutSeconds{30};           // Thời gian chờ tối đa (giây)

    static BenchmarkTask fromJson(const nlohmann::json& j);
};

// Lớp điều phối kiểm thử tự động toàn diện
class HarnessRunner {
private:
    std::shared_ptr<LLMClient> _client;
    std::string _outputDir;
    std::vector<BenchmarkTask> _tasks;

public:
    explicit HarnessRunner(std::shared_ptr<LLMClient> client, std::string outputDir = "benchmark/results");

    // Nạp danh sách test tasks từ file JSON
    std::expected<bool, std::string> loadTasks(const std::string& tasksFilePath);

    // Chạy 1 task đơn lẻ (kèm quản lý Timeout và export Trajectory)
    Trajectory runTask(const BenchmarkTask& task);

    // Chạy toàn bộ danh sách tasks đã nạp
    std::vector<Trajectory> runBatch();

    // Getters & Setters
    const std::vector<BenchmarkTask>& getTasks() const;
    void setOutputDir(const std::string& outputDir);
};

#endif // HARNESS_RUNNER_H