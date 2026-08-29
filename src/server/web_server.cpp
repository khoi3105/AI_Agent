#include "web_server.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include <filesystem>
#include <thread>
#include <queue>
#include <condition_variable>
#include <stop_token>
#include <unordered_map>
#include <sqlite3.h>

#include "httplib.h"
#include "../utils/env_utils.h"
#include "../environment/native_environment.h"
#include "../client/llm_client_factory.h"
#include "../agent/message_queue.h"

using namespace std;
namespace fs = std::filesystem;

WebServer::WebServer(shared_ptr<LLMClient> client, shared_ptr<Environment> env, int port, string webDir, shared_ptr<LLMClient> workerClient)
    : _client(client), _workerClient(workerClient), _env(env), _port(port), _webDir(webDir) {
    if (!_env) {
        _env = make_shared<NativeEnvironment>();
    }
    if (!_workerClient) {
        _workerClient = _client;
    }
}

nlohmann::json WebServer::getMemoriesJson() {
    nlohmann::json arr = nlohmann::json::array();
    sqlite3* db = nullptr;
    if (sqlite3_open("memory.db", &db) != SQLITE_OK) {
        return arr;
    }

    const char* sql = "SELECT id, content, created_at FROM memories ORDER BY id DESC LIMIT 100;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* contentTxt = sqlite3_column_text(stmt, 1);
            const unsigned char* dateTxt = sqlite3_column_text(stmt, 2);

            arr.push_back({
                {"id", id},
                {"content", contentTxt ? reinterpret_cast<const char*>(contentTxt) : ""},
                {"created_at", dateTxt ? reinterpret_cast<const char*>(dateTxt) : ""}
            });
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return arr;
}

nlohmann::json WebServer::getTasksJson() {
    ifstream f("benchmark/tasks.json");
    if (!f.is_open()) return nlohmann::json::array();
    try {
        nlohmann::json j;
        f >> j;
        return j;
    } catch (...) {
        return nlohmann::json::array();
    }
}

nlohmann::json WebServer::getToolsJson() {
    return _registry.getAllSchemas();
}

void WebServer::start() {
    httplib::Server svr;

    // CORS Headers
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // 1. Phục vụ trang chủ Web UI
    svr.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        string htmlPath = _webDir + "/index.html";
        ifstream f(htmlPath);
        if (f.is_open()) {
            stringstream ss;
            ss << f.rdbuf();
            res.set_content(ss.str(), "text/html; charset=utf-8");
        } else {
            res.set_content("<h1>Khong tim thay file web/index.html!</h1>", "text/html; charset=utf-8");
        }
    });

    // 2. API Trạng thái hệ thống
    svr.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
        string model = Config::instance()->llm().model;
        string workerModel = Config::instance()->multi().workerModel;
        if (workerModel.empty()) workerModel = "meta/llama-3.2-11b-vision-instruct";

        nlohmann::json resp = {
            {"status", "online"},
            {"model", model},
            {"worker_model", workerModel},
            {"multi_agent_supported", true},
            {"tools", getToolsJson()},
            {"is_running", _isAgentRunning.load()}
        };
        res.set_content(resp.dump(), "application/json; charset=utf-8");
    });

    // 3. API Danh sách bản ghi bộ nhớ SQLite
    svr.Get("/api/memories", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(getMemoriesJson().dump(), "application/json; charset=utf-8");
    });

    // 4. API Danh sách 10 bài toán Benchmark
    svr.Get("/api/tasks", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(getTasksJson().dump(), "application/json; charset=utf-8");
    });

    // 5. API Chat & ReAct Stream (Server-Sent Events) - Hỗ trợ Single-Agent & Multi-Agent
    svr.Post("/api/chat/stream", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON payload\"}", "application/json");
            return;
        }

        string prompt = body.value("prompt", "");
        if (prompt.empty()) {
            res.status = 400;
            res.set_content("{\"error\": \"Prompt cannot be empty\"}", "application/json");
            return;
        }

        string mode = body.value("mode", "single"); // "single" hoặc "multi"
        string selectedModel = body.value("model", "");
        int maxSteps = body.value("max_steps", 8);
        bool enablePlanning = body.value("enable_planning", true);

        // Chuẩn bị hàng đợi gửi SSE Thread-safe
        struct SseEvent {
            string eventType;
            nlohmann::json data;
        };

        auto eventQueue = make_shared<queue<SseEvent>>();
        auto queueMutex = make_shared<mutex>();
        auto queueCv = make_shared<condition_variable>();
        auto isDone = make_shared<atomic<bool>>(false);

        _isAgentRunning.store(true);

        // Khởi chạy AgentLoop hoặc MultiAgent Coordination trên Worker Thread
        thread([this, prompt, mode, selectedModel, maxSteps, enablePlanning, eventQueue, queueMutex, queueCv, isDone]() {
            if (mode == "multi") {
                // =========================================================================
                // CHẾ ĐỘ MULTI-AGENT COORDINATION (DYNAMIC FAN-OUT & PARALLEL WORKERS)
                // =========================================================================
                {
                    lock_guard<mutex> lock(*queueMutex);
                    eventQueue->push({"multi_start", {
                        {"message", "Master Coordinator (Gemini) đang phân tích và lập kế hoạch phân rã bài toán..."}
                    }});
                }
                queueCv->notify_one();

                string decomposePrompt = format(
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
                    "Bài toán: {}", prompt
                );

                nlohmann::json planMsg = nlohmann::json::array({
                    {{"role", "user"}, {"content", decomposePrompt}}
                });

                auto planRes = _client->chat(planMsg);
                if (!planRes.has_value()) {
                    lock_guard<mutex> lock(*queueMutex);
                    eventQueue->push({"final", {{"output", "[Gemini Decompose Error]: " + planRes.error()}, {"success", false}}});
                    eventQueue->push({"done", {}});
                    isDone->store(true);
                    queueCv->notify_one();
                    _isAgentRunning.store(false);
                    return;
                }

                MultiAgentTask multiTask{
                    .id = "dynamic_multi_agent_task",
                    .description = prompt,
                    .subtasks = {},
                    .aggregationInstruction = "Tổng hợp và đối chiếu kết quả từ các worker để đưa ra kết luận hoàn chỉnh.",
                    .timeoutSeconds = 120
                };

                try {
                    string raw = *planRes;
                    if (raw.find("```json") != string::npos) {
                        raw = raw.substr(raw.find("```json") + 7);
                        raw = raw.substr(0, raw.rfind("```"));
                    } else if (raw.find("```") != string::npos) {
                        raw = raw.substr(raw.find("```") + 3);
                        raw = raw.substr(0, raw.rfind("```"));
                    }
                    size_t firstBrace = raw.find('{');
                    size_t lastBrace = raw.rfind('}');
                    if (firstBrace != string::npos && lastBrace != string::npos && lastBrace > firstBrace) {
                        raw = raw.substr(firstBrace, lastBrace - firstBrace + 1);
                    }
                    nlohmann::json planJson = nlohmann::json::parse(raw);
                    multiTask.aggregationInstruction = planJson.value("aggregation_goal", multiTask.aggregationInstruction);

                    if (planJson.contains("subtasks") && planJson["subtasks"].is_array()) {
                        int index = 1;
                        for (const auto& item : planJson["subtasks"]) {
                            string id = item.value("agent_id", format("Worker_{}", index));
                            string instr = item.value("instruction", "");
                            if (!instr.empty()) {
                                multiTask.subtasks.push_back({
                                    .agentId = id,
                                    .instruction = instr,
                                    .maxSteps = maxSteps
                                });
                                index++;
                            }
                        }
                    }
                } catch (...) {}

                if (multiTask.subtasks.empty()) {
                    multiTask.subtasks.push_back({
                        .agentId = "Worker_1",
                        .instruction = prompt + " (Phân đoạn 1: Thu thập / Phân tích số liệu ban đầu)",
                        .maxSteps = maxSteps
                    });
                    multiTask.subtasks.push_back({
                        .agentId = "Worker_2",
                        .instruction = prompt + " (Phân đoạn 2: Thu thập / Phân tích số liệu đối chiếu)",
                        .maxSteps = maxSteps
                    });
                }

                nlohmann::json subtasksList = nlohmann::json::array();
                for (const auto& st : multiTask.subtasks) {
                    subtasksList.push_back({{"agent_id", st.agentId}, {"instruction", st.instruction}});
                }

                {
                    lock_guard<mutex> lock(*queueMutex);
                    eventQueue->push({"multi_plan", {
                        {"subtasks", subtasksList},
                        {"aggregation_goal", multiTask.aggregationInstruction}
                    }});
                }
                queueCv->notify_one();

                auto messageQueue = make_shared<AgentMessageQueue>();
                stop_source stopSource;
                vector<jthread> workerThreads;
                workerThreads.reserve(multiTask.subtasks.size());

                auto effectiveWorkerClient = _workerClient ? _workerClient : _client;

                for (const auto& subtask : multiTask.subtasks) {
                    workerThreads.emplace_back([subtask, messageQueue, effectiveWorkerClient, eventQueue, queueMutex, queueCv, token = stopSource.get_token()]() {
                        {
                            lock_guard<mutex> lock(*queueMutex);
                            eventQueue->push({"subagent_start", {
                                {"agentId", subtask.agentId},
                                {"instruction", subtask.instruction}
                            }});
                        }
                        queueCv->notify_one();

                        AgentLoop workerAgent(subtask.maxSteps);
                        workerAgent.setEnablePlanning(false);

                        workerAgent.setStepHook([&subtask, eventQueue, queueMutex, queueCv](const StepData& sd) {
                            nlohmann::json stepJson = {
                                {"agentId", subtask.agentId},
                                {"stepNumber", sd.stepNumber},
                                {"thought", sd.thought},
                                {"toolName", sd.actionName},
                                {"toolArgs", sd.actionArgs},
                                {"observation", sd.observation},
                                {"latencyMs", sd.latencyMs},
                                {"tokensUsed", sd.tokensUsed}
                            };
                            {
                                lock_guard<mutex> lock(*queueMutex);
                                eventQueue->push({"subagent_step", stepJson});
                            }
                            queueCv->notify_one();
                        });

                        auto res = workerAgent.run(subtask.instruction, effectiveWorkerClient, {}, token);
                        string outputContent = res.has_value() ? *res : format("[Worker Error]: {}", res.error());

                        messageQueue->push(AgentMessage{
                            .senderId = subtask.agentId,
                            .receiverId = "Coordinator",
                            .content = outputContent,
                            .data = {},
                            .timestampMs = chrono::duration_cast<chrono::milliseconds>(
                                chrono::system_clock::now().time_since_epoch()).count()
                        });

                        {
                            lock_guard<mutex> lock(*queueMutex);
                            eventQueue->push({"subagent_done", {
                                {"agentId", subtask.agentId},
                                {"content", outputContent},
                                {"success", res.has_value()}
                            }});
                        }
                        queueCv->notify_one();
                    });
                }

                unordered_map<string, string> subResults;
                auto startTime = chrono::steady_clock::now();

                while (subResults.size() < multiTask.subtasks.size()) {
                    if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - startTime).count() > multiTask.timeoutSeconds) {
                        stopSource.request_stop();
                        lock_guard<mutex> lock(*queueMutex);
                        eventQueue->push({"final", {{"output", "[Timeout]: Quá thời gian cho phép của Multi-Agent Task!"}, {"success", false}}});
                        eventQueue->push({"done", {}});
                        isDone->store(true);
                        queueCv->notify_one();
                        _isAgentRunning.store(false);
                        return;
                    }

                    auto msgOpt = messageQueue->pop(chrono::milliseconds(200));
                    if (msgOpt.has_value()) {
                        subResults[msgOpt->senderId] = msgOpt->content;
                    }
                }

                workerThreads.clear();

                {
                    lock_guard<mutex> lock(*queueMutex);
                    eventQueue->push({"multi_synthesis_start", {
                        {"message", "Master Coordinator (Gemini) đang tổng hợp toàn bộ kết quả từ các Sub-Agents..."}
                    }});
                }
                queueCv->notify_one();

                string subFindings = "";
                for (const auto& subtask : multiTask.subtasks) {
                    subFindings += format("=== Kết quả từ {} ===\n{}\n\n", subtask.agentId, subResults[subtask.agentId]);
                }

                string synthesisPrompt = format(
                    "Nhiệm vụ tổng quát: {}\n\n"
                    "{}"
                    "Chỉ thị tổng hợp: {}",
                    multiTask.description,
                    subFindings,
                    multiTask.aggregationInstruction
                );

                AgentLoop aggregatorAgent(5);
                aggregatorAgent.setEnablePlanning(false);

                aggregatorAgent.setStepHook([eventQueue, queueMutex, queueCv](const StepData& sd) {
                    nlohmann::json stepJson = {
                        {"stepNumber", sd.stepNumber},
                        {"thought", sd.thought},
                        {"toolName", sd.actionName},
                        {"toolArgs", sd.actionArgs},
                        {"observation", sd.observation},
                        {"latencyMs", sd.latencyMs},
                        {"tokensUsed", sd.tokensUsed}
                    };
                    {
                        lock_guard<mutex> lock(*queueMutex);
                        eventQueue->push({"step", stepJson});
                    }
                    queueCv->notify_one();
                });

                auto finalResult = aggregatorAgent.run(synthesisPrompt, _client);

                {
                    lock_guard<mutex> lock(*queueMutex);
                    if (finalResult.has_value()) {
                        eventQueue->push({"final", {{"output", finalResult.value()}, {"success", true}, {"is_multi", true}}});
                    } else {
                        eventQueue->push({"final", {{"output", finalResult.error()}, {"success", false}, {"is_multi", true}}});
                    }
                    eventQueue->push({"done", {}});
                }
                isDone->store(true);
                queueCv->notify_one();
                _isAgentRunning.store(false);
            } else {
                // =========================================================================
                // CHẾ ĐỘ SINGLE-AGENT (CHẠY 1 MÔ HÌNH TỰ CHỌN)
                // =========================================================================
                shared_ptr<LLMClient> activeClient = _client;

                if (!selectedModel.empty()) {
                    string defaultGemini = Config::instance()->llm().model;
                    string defaultWorker = Config::instance()->multi().workerModel;
                    if (selectedModel == defaultWorker && _workerClient) {
                        activeClient = _workerClient;
                    } else if (selectedModel != defaultGemini) {
                        string apiKey = getEnvVar("LLAMA_API_KEY");
                        if (apiKey.empty()) apiKey = getEnvVar("GEMINI_API_KEY");
                        string baseUrl = Config::instance()->llm().baseUrl;
                        activeClient = LLMClientFactory::createClient(selectedModel, baseUrl, apiKey);
                    }
                }

                AgentLoop agent;
                agent.setMaxSteps(maxSteps);
                agent.setEnablePlanning(enablePlanning);

                // Hook bắt sự kiện từng bước ReAct
                agent.setStepHook([eventQueue, queueMutex, queueCv](const StepData& sd) {
                    nlohmann::json stepJson = {
                        {"stepNumber", sd.stepNumber},
                        {"thought", sd.thought},
                        {"toolName", sd.actionName},
                        {"toolArgs", sd.actionArgs},
                        {"observation", sd.observation},
                        {"latencyMs", sd.latencyMs},
                        {"tokensUsed", sd.tokensUsed}
                    };

                    {
                        lock_guard<mutex> lock(*queueMutex);
                        eventQueue->push({"step", stepJson});
                    }
                    queueCv->notify_one();
                });

                auto result = agent.run(prompt, activeClient);

                {
                    lock_guard<mutex> lock(*queueMutex);
                    if (result.has_value()) {
                        eventQueue->push({"final", {{"output", result.value()}, {"success", true}, {"is_multi", false}}});
                    } else {
                        eventQueue->push({"final", {{"output", result.error()}, {"success", false}, {"is_multi", false}}});
                    }
                    eventQueue->push({"done", {}});
                }
                isDone->store(true);
                queueCv->notify_one();
                _isAgentRunning.store(false);
            }
        }).detach();

        // Thiết lập Streaming SSE
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");

        res.set_content_provider(
            "text/event-stream",
            [eventQueue, queueMutex, queueCv, isDone](size_t /*offset*/, httplib::DataSink& sink) {
                while (true) {
                    unique_lock<mutex> lock(*queueMutex);
                    queueCv->wait(lock, [&]() {
                        return !eventQueue->empty() || isDone->load();
                    });

                    while (!eventQueue->empty()) {
                        auto ev = eventQueue->front();
                        eventQueue->pop();
                        lock.unlock();

                        string sseMsg = format("event: {}\ndata: {}\n\n", ev.eventType, ev.data.dump());
                        if (!sink.write(sseMsg.c_str(), sseMsg.size())) {
                            return false;
                        }

                        if (ev.eventType == "done") {
                            return false; // Kết thúc stream
                        }

                        lock.lock();
                    }

                    if (isDone->load() && eventQueue->empty()) {
                        return false;
                    }
                }
                return true;
            }
        );
    });

    // 6. API Chạy Benchmark Batch hoặc 1 Task
    svr.Post("/api/benchmark/run", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body, nullptr, false);
        string taskId = body.is_object() ? body.value("task_id", "all") : "all";

        _isBenchmarkRunning.store(true);

        HarnessRunner runner(_client, _env, "benchmark/results");
        auto load_res = runner.loadTasks("benchmark/tasks.json");
        if (!load_res.has_value()) {
            res.status = 500;
            res.set_content(format("{{\"error\": \"{}\"}}", load_res.error()), "application/json");
            _isBenchmarkRunning.store(false);
            return;
        }

        nlohmann::json resultsArr = nlohmann::json::array();

        if (taskId == "all") {
            auto batch = runner.runBatch();
            for (const auto& r : batch) {
                resultsArr.push_back({
                    {"taskId", r.getTaskId()},
                    {"instruction", r.getInstruction()},
                    {"success", r.isSuccess()},
                    {"stepCount", r.getStepCount()},
                    {"totalTimeMs", r.getTotalTimeMs()},
                    {"finalOutput", r.getFinalOutput()}
                });
            }
        } else {
            for (const auto& task : runner.getTasks()) {
                if (task.id == taskId) {
                    auto r = runner.runTask(task);
                    resultsArr.push_back({
                        {"taskId", r.getTaskId()},
                        {"instruction", r.getInstruction()},
                        {"success", r.isSuccess()},
                        {"stepCount", r.getStepCount()},
                        {"totalTimeMs", r.getTotalTimeMs()},
                        {"finalOutput", r.getFinalOutput()}
                    });
                    break;
                }
            }
        }

        _isBenchmarkRunning.store(false);
        res.set_content(resultsArr.dump(), "application/json; charset=utf-8");
    });

    cout << "\n=================================================================\n";
    cout << "  🚀 C++ AI AGENT WEB GUI DASHBOARD DA KHOI DONG THANH CONG!     \n";
    cout << format("  🌐 Dia chi truy cap: http://localhost:{}\n", _port);
    cout << "  ✨ Ho tro Tieng Viet 100%, Giao dien Dark Mode Tham my cao     \n";
    cout << "  👉 Hay mo trinh duyet (Chrome/Firefox/Edge) de trai nghiem!    \n";
    cout << "=================================================================\n\n";

    svr.listen("0.0.0.0", _port);
}
