#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

#include "../agent/AgentLoop.h"
#include "../client/llm_client.h"
#include "../environment/environment.h"
#include "../tools/tool_registry.h"
#include "../harness/harness_runner.h"

// Lớp quản lý C++ Web Server cung cấp Web GUI Dashboard & REST/SSE API
class WebServer {
private:
    std::shared_ptr<LLMClient> _client;
    std::shared_ptr<Environment> _env;
    ToolRegistry _registry;
    int _port{8080};
    std::string _webDir{"web"};
    std::atomic<bool> _isAgentRunning{false};
    std::atomic<bool> _isBenchmarkRunning{false};

public:
    explicit WebServer(
        std::shared_ptr<LLMClient> client,
        std::shared_ptr<Environment> env = nullptr,
        int port = 8080,
        std::string webDir = "web"
    );

    // Khởi động Web Server (Lắng nghe cổng HTTP)
    void start();

private:
    void setupRoutes(class httplib_ServerWrapper& server);
    nlohmann::json getMemoriesJson();
    nlohmann::json getTasksJson();
    nlohmann::json getToolsJson();
};

#endif // WEB_SERVER_H
