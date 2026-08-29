#include "harness_runner.h"
#include "evaluator.h" // Tích hợp Evaluator Strategy & Factory
#include "../environment/native_environment.h"

#include <iostream>
#include <print>
#include <ranges> // Sử dụng C++26: concat
#include <fstream>
#include <chrono>
#include <future>
#include <thread>
#include <stop_token>
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
    task.timeoutSeconds = j.value("timeout_seconds", 60);

    if (j.contains("expected_keywords") && j["expected_keywords"].is_array()) {
        task.expectedKeywords = j["expected_keywords"].get<std::vector<std::string>>();
    }
    return task;
}

// ==========================================
// Implementation for HarnessRunner
// ==========================================

HarnessRunner::HarnessRunner(
    std::shared_ptr<LLMClient> client, 
    std::shared_ptr<Environment> env, 
    std::string outputDir,
    std::shared_ptr<LLMClient> workerClient
) : _client(std::move(client)), 
    _workerClient(std::move(workerClient)),
    _env(std::move(env)), 
    _outputDir(std::move(outputDir)) {
    if (!_env) {
        _env = std::make_shared<NativeEnvironment>();
    }
    if (!_workerClient) {
        _workerClient = _client;
    }

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
    std::cout << std::format("[HarnessRunner] Running Task: {} (Độ khó: {}, Max Steps: {}, Timeout: {}s)\n", 
                             task.id, task.difficulty, task.maxSteps, task.timeoutSeconds);
    std::cout << std::format("Description: {}\n", task.description);
    std::cout << std::format("Instruction: {}\n", task.instruction);
    std::cout << "======================================================\n";

    // 1. Chạy setup_script qua Environment
    if (!task.setupScript.empty()) {
        std::cout << std::format("[Environment Setup] Executing: {}\n", task.setupScript);
        auto setup_res = _env->setup(task.setupScript);
        if (!setup_res.has_value()) {
            std::cerr << std::format("[Environment Setup Warning]: {}\n", setup_res.error());
        }
    }

    // 2. Khởi tạo đối tượng Trajectory để lưu vết
    std::string modelName = Config::instance()->llm().model;
    Trajectory trajectory(task.id, modelName, task.instruction);

    // 3. Khởi tạo AgentLoop và cấu hình theo Max Steps & Độ khó của Task
    AgentLoop agent(task.maxSteps);
    
    // CƠ CHẾ ADAPTIVE PLANNING:
    // - Task "simple": Tắt planning để thực thi nhanh (Fast Path), tiết kiệm token
    // - Task "medium" / "hard": Bật TaskPlan (Deliberative Path) để lập kế hoạch suy nghĩ chiến lược
    if (task.difficulty == "simple") {
        agent.setEnablePlanning(false);
        std::cout << "[Adaptive Strategy]: Fast Path (Tắt Planning - Thực thi phản xạ nhanh)\n";
    } else {
        agent.setEnablePlanning(true);
        std::cout << "[Adaptive Strategy]: Deliberative Path (Bật TaskPlan - Suy nghĩ chiến lược vòng đầu)\n";
    }

    // Tiêm Hook Callback (Observer Pattern)
    agent.setStepHook([&trajectory](const StepData& step) {
        std::cout << std::format("  -> [Hook Captured] Step {}: Tool '{}' (Latency: {}ms)\n", 
                                 step.stepNumber, step.actionName, step.latencyMs);
        trajectory.addStep(step);
    });

    auto startTime = std::chrono::steady_clock::now();

    // 4. Quản lý Timeout qua std::stop_source & std::jthread (C++20 Cooperative Cancellation)
    std::stop_source stop_source;
    std::promise<std::expected<std::string, std::string>> agentPromise;
    auto agentFuture = agentPromise.get_future();

    {
        std::jthread workerThread([&agent, &task, this, token = stop_source.get_token(), &agentPromise]() {
            try {
                auto res = agent.run(task.instruction, _client, {}, token);
                agentPromise.set_value(res);
            } catch (const std::exception& e) {
                agentPromise.set_value(std::unexpected(std::format("[Exception]: {}", e.what())));
            } catch (...) {
                agentPromise.set_value(std::unexpected("[Unknown Exception during agent execution]"));
            }
        });

        std::future_status status = agentFuture.wait_for(std::chrono::seconds(task.timeoutSeconds));

        auto endTime = std::chrono::steady_clock::now();
        int64_t totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        trajectory.setTotalTimeMs(totalDurationMs);

        // 5. Thu thập kết quả & Thực hiện Đánh giá (Evaluate)
        if (status == std::future_status::timeout) {
            std::cerr << std::format("[ERROR]: Task {} timed out after {}s!\n", task.id, task.timeoutSeconds);
            stop_source.request_stop(); // Phát tín hiệu ngắt ngay lập tức cho AgentLoop
            trajectory.setFinalOutput(std::format("[ERROR]: Timeout exceeded ({}s)", task.timeoutSeconds));
            trajectory.setSuccess(false);
        } else {
            auto runResult = agentFuture.get();
            if (runResult.has_value()) {
                trajectory.setFinalOutput(*runResult);
                std::cout << std::format("[HarnessRunner] Agent finished execution.\n");

                // ĐÁNH GIÁ TỰ ĐỘNG BẰNG EVALUATOR STRATEGY & ENVIRONMENT
                auto evaluator = EvaluatorFactory::create(task.evalType, _env);
                
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
    } // workerThread tự động join sạch sẽ tại đây khi kết thúc scope

    // 6. Dọn dẹp môi trường (Teardown)
    _env->teardown();

    // 7. Xuất báo cáo vết thực thi ra file JSON (trajectory_{task_id}.json)
    std::string exportPath = std::format("{}/trajectory_{}.json", _outputDir, task.id);
    auto exportRes = trajectory.exportToJson(exportPath);
    if (exportRes.has_value()) {
        std::cout << std::format("[HarnessRunner] Log exported: {}\n", exportPath);
    } else {
        std::cerr << std::format("[ERROR]: Failed to export JSON: {}\n", exportRes.error());
    }

    return trajectory;
}

void HarnessRunner::addExtraTask(const BenchmarkTask& task) {
    _extraTasks.push_back(task);
}

std::vector<Trajectory> HarnessRunner::runBatch() {
    std::vector<Trajectory> results;
    size_t totalTasks = _tasks.size() + _extraTasks.size();
    results.reserve(totalTasks);

    int passedCount = 0;
    std::println("\n>>> Starting Benchmark Batch (Total: {} tasks) <<<", totalTasks);

    // C++26: std::views::concat kết hợp tập task chính và dynamic tasks mà không tốn chi phí sao chép
    for (const auto& task : std::views::concat(_tasks, _extraTasks)) {
        Trajectory traj = runTask(task);
        if (traj.isSuccess()) {
            passedCount++;
        }
        results.push_back(std::move(traj));
    }

    // Tính tỷ lệ thành công
    double successRate = totalTasks == 0 ? 0.0 : (static_cast<double>(passedCount) / totalTasks) * 100.0;

    std::println("\n======================================================");
    std::println("               BENCHMARK BATCH SUMMARY                ");
    std::println("======================================================");
    std::println("Total Tasks Executed : {}", results.size());
    std::println("Tasks Passed         : {}", passedCount);
    std::println("Tasks Failed         : {}", results.size() - passedCount);
    std::println("Success Rate (Acc)   : {:.2f}%", successRate);
    std::println("======================================================");

    // Xuất toàn bộ kết quả ra file results.json kèm Success Rate
    std::string resultsPath = std::format("{}/results.json", _outputDir);
    auto expRes1 = exportBatchResults(results, resultsPath);
    auto expRes2 = exportBatchResults(results, "benchmark/results.json");
    (void)expRes1;
    (void)expRes2;

    return results;
}

std::expected<bool, std::string> HarnessRunner::exportBatchResults(
    const std::vector<Trajectory>& results, 
    const std::string& filepath
) const {
    if (results.empty()) {
        return std::unexpected("Không có kết quả benchmark để export.");
    }

    int passedCount = 0;
    int totalTokensAll = 0;
    int64_t totalTimeMsAll = 0;
    std::string modelName = Config::instance()->llm().model;

    nlohmann::json tasksArray = nlohmann::json::array();

    for (const auto& traj : results) {
        if (traj.isSuccess()) {
            passedCount++;
        }
        totalTokensAll += traj.getTotalTokens();
        totalTimeMsAll += traj.getTotalTimeMs();
        if (modelName.empty() && !traj.getModelName().empty()) {
            modelName = traj.getModelName();
        }

        std::string difficulty = "unknown";
        std::string description = "";
        std::string evalType = "keyword";
        for (const auto& t : _tasks) {
            if (t.id == traj.getTaskId()) {
                difficulty = t.difficulty;
                description = t.description;
                evalType = t.evalType;
                break;
            }
        }

        std::string taskModel = !traj.getModelName().empty() ? traj.getModelName() : modelName;

        nlohmann::json taskItem = {
            {"task_id", traj.getTaskId()},
            {"model", taskModel},
            {"difficulty", difficulty},
            {"description", description},
            {"instruction", traj.getInstruction()},
            {"eval_type", evalType},
            {"success", traj.isSuccess()},
            {"step_count", traj.getStepCount()},
            {"total_tokens", traj.getTotalTokens()},
            {"total_time_ms", traj.getTotalTimeMs()},
            {"final_output", traj.getFinalOutput()},
            {"trajectory_file", std::format("trajectory_{}.json", traj.getTaskId())}
        };

        tasksArray.push_back(taskItem);
    }

    double successRate = (static_cast<double>(passedCount) / results.size()) * 100.0;

    nlohmann::json root = {
        {"benchmark_summary", {
            {"model", modelName},
            {"total_tasks", results.size()},
            {"tasks_passed", passedCount},
            {"tasks_failed", results.size() - passedCount},
            {"success_rate_percent", successRate},
            {"total_tokens_all_tasks", totalTokensAll},
            {"total_time_ms_all_tasks", totalTimeMsAll},
            {"average_tokens_per_task", results.empty() ? 0 : totalTokensAll / static_cast<int>(results.size())},
            {"average_time_ms_per_task", results.empty() ? 0 : totalTimeMsAll / static_cast<int64_t>(results.size())}
        }},
        {"results", tasksArray}
    };

    try {
        std::filesystem::path p(filepath);
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream outFile(filepath);
        if (!outFile.is_open()) {
            return std::unexpected(std::format("Không thể mở file '{}' để ghi!", filepath));
        }

        outFile << root.dump(2);
        if (!outFile.good()) {
            return std::unexpected(std::format("Lỗi ghi dữ liệu ra file '{}'!", filepath));
        }

        std::cout << std::format("[HarnessRunner] Đã xuất toàn bộ kết quả Benchmark ra: {}\n", filepath);
        return true;
    } catch (const std::exception& e) {
        return std::unexpected(std::format("Lỗi ngoại lệ khi export JSON: {}", e.what()));
    }
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

void HarnessRunner::setEnvironment(std::shared_ptr<Environment> env) {
    _env = std::move(env);
}

std::shared_ptr<Environment> HarnessRunner::getEnvironment() const {
    return _env;
}

void HarnessRunner::setWorkerClient(std::shared_ptr<LLMClient> workerClient) {
    _workerClient = std::move(workerClient);
}

std::shared_ptr<LLMClient> HarnessRunner::getWorkerClient() const {
    return _workerClient;
}

void HarnessRunner::spawnSubAgent(
    const std::string& agentId,
    const std::string& subtaskInstruction,
    int maxSteps,
    std::shared_ptr<AgentMessageQueue> messageQueue,
    std::stop_token stopToken) 
{
    std::cout << std::format("[Spawner] Agent [{}] bat dau chay tren Thread ID: {}\n", 
                             agentId, std::this_thread::get_id());
    
    AgentLoop agent(maxSteps);
    agent.setEnablePlanning(false); // Sub-agent chạy Fast Path để phản xạ nhanh
    
    // Sub-agent thực thi bằng _workerClient (Llama / Ollama)
    auto result = agent.run(subtaskInstruction, _workerClient, {}, stopToken);
    
    std::string outputContent = result.has_value() ? *result : std::format("[Worker Error]: {}", result.error());
    
    // Đẩy kết quả vào MessageQueue (Thread-safe)
    messageQueue->push(AgentMessage{
        .senderId = agentId,
        .receiverId = "Coordinator",
        .content = outputContent,
        .data = {},
        .timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    });
    
    std::cout << std::format("[Spawner] Agent [{}] da hoan thanh va gui thong diep vao MessageQueue.\n", agentId);
}

std::expected<std::string, std::string> HarnessRunner::runMultiAgentTask(const MultiAgentTask& task) {
    std::cout << "\n======================================================\n";
    std::cout << std::format("[Multi-Agent] KHOI CHAY TASK: {}\n", task.id);
    std::cout << std::format("Mo ta: {}\n", task.description);
    std::cout << std::format("So luong Sub-Agents duoc phan cong: {}\n", task.subtasks.size());
    std::cout << "======================================================\n";

    if (task.subtasks.empty()) {
        return std::unexpected("[Multi-Agent Error]: Danh sach subtasks rong!");
    }

    auto messageQueue = std::make_shared<AgentMessageQueue>();
    std::stop_source stopSource;

    // 1. Spawn đồng thời N Sub-Agents trên N thread riêng biệt (std::jthread C++20)
    std::vector<std::jthread> workerThreads;
    workerThreads.reserve(task.subtasks.size());

    for (const auto& subtask : task.subtasks) {
        workerThreads.emplace_back([this, subtask, messageQueue, token = stopSource.get_token()]() {
            spawnSubAgent(subtask.agentId, subtask.instruction, subtask.maxSteps, messageQueue, token);
        });
    }

    // 2. Chờ nhận kết quả từ MessageQueue từ toàn bộ N agents
    std::unordered_map<std::string, std::string> subResults;
    auto startTime = std::chrono::steady_clock::now();

    while (subResults.size() < task.subtasks.size()) {
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count() > task.timeoutSeconds) {
            stopSource.request_stop();
            return std::unexpected(std::format("[Timeout]: Qua thoi gian cho phep ({}s) cua Multi-Agent Task!", task.timeoutSeconds));
        }

        auto msgOpt = messageQueue->pop(std::chrono::milliseconds(200));
        if (msgOpt.has_value()) {
            std::cout << std::format("\n[Coordinator] Nhan thong diep tu [{}]:\n{}\n", 
                                     msgOpt->senderId, msgOpt->content);
            subResults[msgOpt->senderId] = msgOpt->content;
        }
    }

    // 3. HarnessRunner khởi chạy AgentLoop với Coordinator Client (Gemini) để tổng hợp kết quả và thực thi Tool (ghi file, v.v.)
    std::cout << "\n[Coordinator] 🧠 Khởi động AgentLoop tổng hợp kết quả và thực thi hành động qua Gemini...\n";
    
    std::string subFindings = "";
    for (const auto& subtask : task.subtasks) {
        subFindings += std::format("=== Kết quả từ {} ===\n{}\n\n", 
                                   subtask.agentId, subResults[subtask.agentId]);
    }

    std::string synthesisPrompt = std::format(
        "Nhiệm vụ tổng quát: {}\n\n"
        "{}"
        "Chỉ thị tổng hợp: {}",
        task.description,
        subFindings,
        task.aggregationInstruction
    );

    AgentLoop aggregatorAgent(5);
    aggregatorAgent.setEnablePlanning(false); // Không cần planning vòng 0 vì đã có đủ dữ liệu từ các sub-agents

    return aggregatorAgent.run(synthesisPrompt, _client);
}

std::expected<std::string, std::string> HarnessRunner::coordinateTask(const std::string& complexUserTask) {
    std::cout << "\n======================================================\n";
    std::cout << "[Coordinator] BẮT ĐẦU ĐIỀU PHỐI HYBRID MULTI-AGENT (DYNAMIC FAN-OUT)\n";
    std::cout << std::format("Nhiệm vụ: \"{}\"\n", complexUserTask);
    std::cout << "======================================================\n";

    // ------------------------------------------------------------------
    // GIAI ĐOẠN 1: Dùng Gemini (Coordinator) phân rã bài toán lớn thành N Subtasks động
    // (CHỈ TỐN 1 REQUEST GEMINI)
    // ------------------------------------------------------------------
    std::string decomposePrompt = std::format(
        "Bạn là AI Master Coordinator. Hãy phân tích bài toán lớn sau và tự động chia nhỏ thành các subtasks độc lập (từ 2 đến 4 subtasks tùy độ phức tạp) để các worker agents thực hiện song song.\n"
        "Yêu cầu trả về DUY NHẤT 01 khối JSON có cấu trúc sau:\n"
        "```json\n"
        "{{\n"
        "  \"subtasks\": [\n"
        "    {{\n"
        "      \"agent_id\": \"Worker_1\",\n"
        "      \"instruction\": \"<Chỉ thị chi tiết cụ thể cho Agent 1>\"\n"
        "    }},\n"
        "    {{\n"
        "      \"agent_id\": \"Worker_2\",\n"
        "      \"instruction\": \"<Chỉ thị chi tiết cụ thể cho Agent 2>\"\n"
        "    }}\n"
        "  ],\n"
        "  \"aggregation_goal\": \"<Chỉ thị tổng hợp và định dạng kết quả cuối cùng>\"\n"
        "}}\n"
        "```\n\n"
        "Bài toán: {}", complexUserTask
    );

    nlohmann::json planMsg = nlohmann::json::array({
        {{"role", "user"}, {"content", decomposePrompt}}
    });

    std::cout << "[Gemini Coordinator] Đang phân tích và tự quyết định số lượng Sub-Agents cần thiết...\n";
    auto planRes = _client->chat(planMsg);
    if (!planRes.has_value()) {
        return std::unexpected("[Gemini Decompose Error]: " + planRes.error());
    }

    MultiAgentTask multiTask{
        .id = "dynamic_multi_agent_task",
        .description = complexUserTask,
        .subtasks = {},
        .aggregationInstruction = "Tổng hợp và đối chiếu kết quả từ các worker để đưa ra kết luận hoàn chỉnh.",
        .timeoutSeconds = 120
    };

    try {
        std::string raw = *planRes;
        if (raw.find("```json") != std::string::npos) {
            raw = raw.substr(raw.find("```json") + 7);
            raw = raw.substr(0, raw.rfind("```"));
        } else if (raw.find("```") != std::string::npos) {
            raw = raw.substr(raw.find("```") + 3);
            raw = raw.substr(0, raw.rfind("```"));
        }
        
        size_t firstBrace = raw.find('{');
        size_t lastBrace = raw.rfind('}');
        if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace > firstBrace) {
            raw = raw.substr(firstBrace, lastBrace - firstBrace + 1);
        }

        nlohmann::json planJson = nlohmann::json::parse(raw);
        multiTask.aggregationInstruction = planJson.value("aggregation_goal", multiTask.aggregationInstruction);

        if (planJson.contains("subtasks") && planJson["subtasks"].is_array()) {
            int index = 1;
            for (const auto& item : planJson["subtasks"]) {
                std::string id = item.value("agent_id", std::format("Worker_{}", index));
                std::string instr = item.value("instruction", "");
                if (!instr.empty()) {
                    multiTask.subtasks.push_back({
                        .agentId = id,
                        .instruction = instr,
                        .maxSteps = 6
                    });
                    index++;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("[Warning]: Không thể parse JSON từ Gemini ({}), chuyển sang phân rã 2 Subtasks mặc định.\n", e.what());
    }

    // Fallback an toàn nếu không parse được mảng subtasks
    if (multiTask.subtasks.empty()) {
        multiTask.subtasks.push_back({
            .agentId = "Worker_1",
            .instruction = complexUserTask + " (Phân đoạn 1: Thu thập / Phân tích số liệu ban đầu)",
            .maxSteps = 6
        });
        multiTask.subtasks.push_back({
            .agentId = "Worker_2",
            .instruction = complexUserTask + " (Phân đoạn 2: Thu thập / Phân tích số liệu đối chiếu)",
            .maxSteps = 6
        });
    }

    std::cout << std::format("[Coordinator] Gemini quyết định tạo ra {} Sub-Agents song song:\n", multiTask.subtasks.size());
    for (const auto& st : multiTask.subtasks) {
        std::cout << std::format("   • [{}] -> {}\n", st.agentId, st.instruction);
    }
    std::cout << "\n";

    return runMultiAgentTask(multiTask);
}
