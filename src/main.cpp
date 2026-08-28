#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib>
#include <format>
#include <nlohmann/json.hpp>

// Include các file header trong dự án
#include "agent/AgentLoop.h"
#include "agent/gui_agent_loop.h"
#include "client/llm_client.h"
#include "client/ollama_client.h"
#include "environment/environment.h"
#include "environment/native_environment.h"
#include "environment/sandbox_environment.h"
#include "utils/env_utils.h"
#include "harness/harness_runner.h"

using namespace std;

int main(int argc, char* argv[]) {
    // 1. Khai báo thông tin API
    string model = "meta/llama-3.2-11b-vision-instruct";
    string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    cout << "[1] Dang khoi tao LLM Client (Multimodal VLM)..." << endl;
    shared_ptr<LLMClient> client = make_shared<OllamaClient>(model, base_url, api_key);

    // 3. Khởi tạo Môi trường thực thi (Environment Abstraction: Native hoặc Sandbox)
    shared_ptr<Environment> env = make_shared<NativeEnvironment>();

    // =========================================================================
    // LỰA CHỌN CHẾ ĐỘ CHẠY:
    // 1: BENCHMARK HARNESS
    // 2: AGENT TEXT REACT THÔNG THƯỜNG
    // 3: GUI AGENT DEMO (Tự động chụp màn hình, phân tích UI, click/type/press)
    // =========================================================================
    string mode = "gui"; // Mặc định chạy GUI Agent hoặc đọc từ argv
    if (argc > 1) {
        mode = argv[1];
    }

    if (mode == "benchmark") {
        cout << "\n======================================================\n";
        cout << "           KHOI DONG BENCHMARK EVALUATION             \n";
        cout << "======================================================\n";

        // Khởi tạo HarnessRunner với Environment và chỉ định thư mục xuất kết quả JSON
        HarnessRunner runner(client, env, "benchmark/results");

        // Nạp tập task benchmark
        string benchmark_file = "benchmark/tasks.json";
        cout << format("Dang nap file benchmark: {}...\n", benchmark_file);

        auto load_res = runner.loadTasks(benchmark_file);
        if (!load_res.has_value()) {
            cerr << format("[ERROR]: Khong the nap tasks: {}\n", load_res.error());
            return 1;
        }

        cout << format("-> Da nap thanh cong {} tasks!\n", runner.getTasks().size());

        // Chạy toàn bộ Benchmark Batch (Tự động tiêm StepHook, Timeout, Evaluator & xuất Trajectory)
        auto batch_results = runner.runBatch();

        cout << "\n-> Tat ca file log chi tiet da duoc luu tai: benchmark/results/\n";
        return 0;
    } 
    else if (mode == "gui") {
        // =========================================================================
        // CHẾ ĐỘ 3: GUI AGENT DEMO (TÍNH NĂNG 10.1: +8đ)
        // =========================================================================
        cout << "\n======================================================\n";
        cout << "     KHOI DONG GUI AGENT (SCREENSHOT + ACTION)        \n";
        cout << "======================================================\n";

        string gui_task = "Quan sát màn hình Desktop hiện tại, mở terminal hoặc trình duyệt web, tìm kiếm thông tin về 'Trường Đại học Khoa học Tự nhiên ĐHQG-HCM', sao chép và tổng kết kết quả.";
        if (argc > 2) {
            gui_task = argv[2];
        }

        cout << "[Task]: \"" << gui_task << "\"\n\n";

        GUIAgentLoop gui_agent(8, "/tmp/agent_screenshot.png");
        gui_agent.setActionDelayMs(1000); // 1s giữa các thao tác

        auto result = gui_agent.run(gui_task, client);
        if (result.has_value()) {
            cout << "\n[Ket qua GUI Agent]:\n" << result.value() << endl;
        } else {
            cerr << "\n[Loi GUI Agent]: " << result.error() << endl;
        }
        return 0;
    }
    else {
        // =========================================================================
        // CHẾ ĐỘ 2: CHẠY TƯƠNG TÁC THỦ CÔNG (SINGLE RUN)
        // =========================================================================
        string user_task = "Tìm kiếm trên web xem ai là hiệu trưởng hiện tại của Trường Đại học Khoa học Tự nhiên ĐHQG-HCM. Sau đó ghi vào file hcmus.txt";
        cout << "[3] User Task: \"" << user_task << "\"" << endl << endl;
        vector<string> images_path = {};

        AgentLoop agent;
        auto result = agent.run(user_task, client, images_path);
        if (result.has_value()) {
            cout << result.value() << endl;
        } else {
            cout << result.error() << endl;
        }
    }
    return 0;
}