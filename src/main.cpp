#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib> //env
#include <nlohmann/json.hpp>

// Include các file header trong dự án
#include "agent/AgentLoop.h"
#include "client/ollama_client.h"
#include "utils/env_utils.h"
using namespace std;

int main() {
    // 1. Khai báo thông tin API
    string model = "meta/llama-3.2-11b-vision-instruct";
    string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    cout << "[1] Dang khoi tao LLM Client..." << endl;
    shared_ptr<LLMClient> client = make_shared<OllamaClient>(model, base_url, api_key);

    // 3. Khởi tạo ToolRegistry (Phiên bản POC đã hardcode CalculatorTool)
    cout << "[2] Dang khoi tao ToolRegistry..." << endl;
    // ToolRegistry registry;

    // 4. Chuẩn bị câu hỏi (Task) từ người dùng
    // string user_task = "Thực hiện phép tính 20 + 10 * ( 10 + 6 )= ?";
    string user_task = " Thực hiện phép tính 20 + 10. Và cho tôi hỏi thời tiết Hồ Chí Minh hôm nay như thế nào?";
    // string user_task = "Hãy dùng công cụ calculator tính 15 * 87. Sau khi tính xong, hãy gọi lại calculator tính lại đúng phép tính 15 * 87 thêm 3 lần nữa để chắc chắn kết quả không bị sai";
    // string user_task = "Doraemon là ai vậy? ";
    // string user_task = "tính biểu thức trong bức ảnh ";
    // string user_task;
    // cout << "Nhap prompt cua ban: ";
    // getline(cin, user_task);

    cout << "[3] User Task: \"" << user_task << "\"" << endl << endl;
    vector<string> images_path = {
        // "build/Untitled.png",
    };

    AgentLoop agent;
    auto result = agent.run(user_task,client,images_path);
    if (result.has_value()) {
        cout << result.value() << endl;
    } else {
        cout << result.error();
    }
    return 0;
}