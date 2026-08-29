#ifndef HARNESS_RUNNER_H
#define HARNESS_RUNNER_H

#include <string>
#include <vector>
#include <memory>
#include <expected>
#include <nlohmann/json.hpp>

#include "trajectory.h"
#include "../agent/AgentLoop.h"
#include "../agent/message_queue.h"
#include "../client/llm_client.h"
#include "../environment/environment.h"

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
    int timeoutSeconds{60};           // Thời gian chờ tối đa (giây)

    static BenchmarkTask fromJson(const nlohmann::json& j);
};

// Cấu trúc cho Subtask của từng Sub-Agent
struct SubTaskSpec {
    std::string agentId;
    std::string instruction;
    int maxSteps{5};
};

// Cấu trúc cho Task điều phối đa Agent (Multi-Agent Task - Phân giải động N Sub-Agents)
struct MultiAgentTask {
    std::string id;
    std::string description;
    std::vector<SubTaskSpec> subtasks; // Danh sách động N Subtasks do Coordinator quyết định
    std::string aggregationInstruction;
    int timeoutSeconds{120};
};

// Lớp điều phối kiểm thử tự động & Phối hợp Đa Agent toàn diện
class HarnessRunner {
private:
    std::shared_ptr<LLMClient> _client;        // Coordinator Client (mặc định / Gemini)
    std::shared_ptr<LLMClient> _workerClient;  // Worker Client (Llama / Ollama cho Sub-Agents)
    std::shared_ptr<Environment> _env;
    std::string _outputDir;
    std::vector<BenchmarkTask> _tasks;
    std::vector<BenchmarkTask> _extraTasks;

public:
    explicit HarnessRunner(
        std::shared_ptr<LLMClient> client, 
        std::shared_ptr<Environment> env = nullptr,
        std::string outputDir = "benchmark/results",
        std::shared_ptr<LLMClient> workerClient = nullptr
    );

    void addExtraTask(const BenchmarkTask& task);

    // Nạp danh sách test tasks từ file JSON
    std::expected<bool, std::string> loadTasks(const std::string& tasksFilePath);

    // Chạy 1 task đơn lẻ (kèm quản lý Timeout và export Trajectory)
    Trajectory runTask(const BenchmarkTask& task);

    // Chạy toàn bộ danh sách tasks đã nạp
    std::vector<Trajectory> runBatch();

    // Xuất toàn bộ kết quả Benchmark ra file JSON tổng hợp (results.json) kèm Success Rate & Token Metrics
    std::expected<bool, std::string> exportBatchResults(
        const std::vector<Trajectory>& results, 
        const std::string& filepath = "benchmark/results/results.json"
    ) const;

    // ==========================================
    // MULTI-AGENT COORDINATION (TÍNH NĂNG 10.3)
    // ==========================================

    // Spawn 1 sub-agent chạy trên thread riêng biệt và giao tiếp qua MessageQueue
    void spawnSubAgent(
        const std::string& agentId,
        const std::string& subtaskInstruction,
        int maxSteps,
        std::shared_ptr<AgentMessageQueue> messageQueue,
        std::stop_token stopToken
    );

    // Chạy 1 MultiAgentTask đã xác định trước (2 Sub-Agents chạy song song)
    std::expected<std::string, std::string> runMultiAgentTask(const MultiAgentTask& task);

    // Tự động phân rã bài toán phức tạp bằng Coordinator (Gemini) -> Chạy Workers song song (Llama) -> Tổng hợp báo cáo
    std::expected<std::string, std::string> coordinateTask(const std::string& complexUserTask);

    // Getters & Setters
    const std::vector<BenchmarkTask>& getTasks() const;
    void setOutputDir(const std::string& outputDir);
    void setEnvironment(std::shared_ptr<Environment> env);
    std::shared_ptr<Environment> getEnvironment() const;

    void setWorkerClient(std::shared_ptr<LLMClient> workerClient);
    std::shared_ptr<LLMClient> getWorkerClient() const;
};

#endif // HARNESS_RUNNER_H