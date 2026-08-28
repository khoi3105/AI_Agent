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
#include "environment/environment.h"
#include "environment/native_environment.h"
#include "environment/sandbox_environment.h"
#include "utils/env_utils.h"
#include "harness/harness_runner.h"
#include "config/config.h"

using namespace std;

int main(int argc, char* argv[]) {
    // 1. Khai báo thông tin API
    // string model = "meta/llama-3.2-11b-vision-instruct";
    // string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    string model = Config::instance()->llm().model;
    string base_url = Config::instance()->llm().baseUrl;
    string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    cout << "[1] Dang khoi tao LLM Client..." << endl;
    shared_ptr<LLMClient> client = make_shared<OllamaClient>(model, base_url, api_key);

    // 3. Khởi tạo Môi trường thực thi (Environment Abstraction: Native hoặc Sandbox)
    shared_ptr<Environment> env = make_shared<NativeEnvironment>();

    // =========================================================================
    // CHẾ ĐỘ 1: CHẠY BENCHMARK HARNESS TỰ ĐỘNG
    // =========================================================================
    bool run_benchmark_mode = true; // Đặt false nếu muốn chạy tương tác 1 câu lẻ bên dưới

    if (run_benchmark_mode) {
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
    } else {
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