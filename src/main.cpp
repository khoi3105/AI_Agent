#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib>
#include <format>
#include <nlohmann/json.hpp>

// Include các file header trong dự án
#include "agent/AgentLoop.h"
#include "client/llm_client.h"
#include "client/ollama_client.h"
#include "utils/env_utils.h"
#include "harness/harness_runner.h"

using namespace std;

int main(int argc, char* argv[]) {
    // 1. Khai báo thông tin API
    string model = "meta/llama-3.2-11b-vision-instruct";
    string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    cout << "[1] Dang khoi tao LLM Client..." << endl;
    shared_ptr<LLMClient> client = make_shared<OllamaClient>(model, base_url, api_key);

    // =========================================================================
    // CHẾ ĐỘ 1: CHẠY BENCHMARK HARNESS TỰ ĐỘNG
    // =========================================================================
    bool run_benchmark_mode = true; // Đặt false nếu muốn chạy tương tác 1 câu lẻ bên dưới

    if (run_benchmark_mode) {
        cout << "\n======================================================\n";
        cout << "           KHOI DONG BENCHMARK EVALUATION             \n";
        cout << "======================================================\n";

        // Khởi tạo HarnessRunner và chỉ định thư mục xuất kết quả JSON
        HarnessRunner runner(client, "benchmark/results");

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
    } else {
        // =========================================================================
        // CHẾ ĐỘ 2: CHẠY TƯƠNG TÁC THỦ CÔNG (SINGLE RUN)
        // =========================================================================
        string user_task = "Thực hiện phép tính 20 + 10. Và cho tôi hỏi thời tiết Hồ Chí Minh hôm nay như thế nào?";
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