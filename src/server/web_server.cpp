#include "web_server.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include <filesystem>
#include <thread>
#include <queue>
#include <condition_variable>
#include <sqlite3.h>

#include "httplib.h"
#include "../utils/env_utils.h"
#include "../environment/native_environment.h"

using namespace std;
namespace fs = std::filesystem;

WebServer::WebServer(shared_ptr<LLMClient> client, shared_ptr<Environment> env, int port, string webDir)
    : _client(client), _env(env), _port(port), _webDir(webDir) {
    if (!_env) {
        _env = make_shared<NativeEnvironment>();
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
        string model = getEnvVar("OLLAMA_MODEL");
        if (model.empty()) model = "meta/llama-3.2-11b-vision-instruct";

        nlohmann::json resp = {
            {"status", "online"},
            {"model", model},
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

    // 5. API Chat & ReAct Stream (Server-Sent Events)
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

        // Khởi chạy AgentLoop trên Worker Thread
        thread([this, prompt, maxSteps, enablePlanning, eventQueue, queueMutex, queueCv, isDone]() {
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
                    {"latencyMs", sd.latencyMs}
                };

                {
                    lock_guard<mutex> lock(*queueMutex);
                    eventQueue->push({"step", stepJson});
                }
                queueCv->notify_one();
            });

            auto result = agent.run(prompt, _client);

            {
                lock_guard<mutex> lock(*queueMutex);
                if (result.has_value()) {
                    eventQueue->push({"final", {{"output", result.value()}, {"success", true}}});
                } else {
                    eventQueue->push({"final", {{"output", result.error()}, {"success", false}}});
                }
                eventQueue->push({"done", {}});
            }
            isDone->store(true);
            queueCv->notify_one();
            _isAgentRunning.store(false);
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
