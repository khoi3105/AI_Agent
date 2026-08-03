#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib> //std::env
// #include "tools/tool_registry.h"
// #include "agent/tool_call_parser.h"
#include <nlohmann/json.hpp>

// Include các file header trong dự án
#include "agent/AgentLoop.h"
#include "client/ollama_client.h"
#include "utils/env_utils.h"

int main() {
    // 1. Khai báo thông tin API
    std::string model = "meta/llama-3.2-11b-vision-instruct";
    std::string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    std::string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    std::cout << "[1] Dang khoi tao LLM Client..." << std::endl;
    std::shared_ptr<LLMClient> client = std::make_shared<OllamaClient>(model, base_url, api_key);

    // 3. Khởi tạo ToolRegistry (Phiên bản POC đã hardcode CalculatorTool)
    std::cout << "[2] Dang khoi tao ToolRegistry..." << std::endl;
    // ToolRegistry registry;

    // 4. Chuẩn bị câu hỏi (Task) từ người dùng
    // std::string user_task = "Thực hiện phép tính 20 + 10 * ( 10 + 6 )= ?";
    std::string user_task = " Thực hiện phép tính 20 + 10. Và cho tôi hỏi thời tiết Hồ Chí Minh hôm nay như thế nào?";
    // std::string user_task = "Doraemon là ai vậy? ";
    // std::string user_task = "tính biểu thức trong bức ảnh ";
    // std::string user_task;
    // std::cout << "Nhap prompt cua ban: ";
    // getline(std::cin, user_task);

    std::cout << "[3] User Task: \"" << user_task << "\"" << std::endl << std::endl;
    std::vector<std::string> images_path = {
        "build/Untitled.png",
    };

    AgentLoop agent;
    std::cout << *agent.run(user_task,client,images_path);

    return 0;
}