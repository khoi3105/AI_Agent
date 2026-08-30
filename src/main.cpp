#include <iostream>
#include <print>
#include <ranges>
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
#include "client/gemini_client.h"
#include "client/llm_client_factory.h"
#include "environment/environment.h"
#include "environment/native_environment.h"
#include "environment/sandbox_environment.h"
#include "utils/env_utils.h"
#include "harness/harness_runner.h"
#include "config/config.h"
#include "server/web_server.h"

using namespace std;

void printBanner() {
    println("=================================================================");
    println("          AUTONOMOUS C++ AI AGENT (OOP 2026 - HCMUS)             ");
    println("=================================================================");
}

void printHelp(const char* progName) {
    println("\nCách sử dụng:");
    println("  {} [tùy chọn] [yêu cầu]\n", progName);
    println("Các tùy chọn:");
    println("  --web, -w                     : Khởi chạy Web GUI Dashboard (Tiếng Việt, port 8080)");
    println("  --gui-agent, --gui, -g, -a, gui: Khởi chạy Desktop GUI Agent (Computer Use & Multi-Agent Coordination)");
    println("  --multi-agent, -m             : Khởi chạy Hybrid Multi-Agent Coordination (Gemini + Llama)");
    println("  --eval, -b                    : Chạy toàn bộ bộ đánh giá Benchmark (10 Tasks)");
    println("  --task <id>                   : Chạy riêng 01 Task Benchmark (ví dụ: --task task_001)");
    println("  --help, -h                    : Hiển thị hướng dẫn sử dụng này");
    println("  \"<nội dung câu hỏi>\"          : Chạy trực tiếp một tác vụ cho AI Agent");
    println("  (Không có tham số)            : Mở Menu tương tác trực quan\n");
}

int main(int argc, char* argv[]) {
    // 1. Khai báo thông tin API từ Config và Environment cho Coordinator (Gemini / Default)
    string model = Config::instance()->llm().model;
    string base_url = Config::instance()->llm().baseUrl;
    string api_key = getEnvVar("GEMINI_API_KEY");
    if (api_key.empty()) api_key = getEnvVar("LLAMA_API_KEY");
    if (api_key.empty()) api_key = getEnvVar("OLLAMA_API_KEY");

    // 2. Khởi tạo LLM Client thông qua Factory Pattern
    shared_ptr<LLMClient> client = LLMClientFactory::createClient(model, base_url, api_key);

    // 3. Khai báo thông tin API cho Worker Sub-Agents (Llama / Ollama) từ [multi] section & .env
    string worker_model = Config::instance()->multi().workerModel;
    string worker_base_url = Config::instance()->multi().workerBaseUrl;
    string worker_api_key = Config::instance()->multi().workerApiKey;
    if (worker_api_key.empty()) worker_api_key = getEnvVar("LLAMA_API_KEY");
    if (worker_api_key.empty()) worker_api_key = getEnvVar("OLLAMA_API_KEY");
    
    if (worker_base_url.empty()) {
        if (!worker_api_key.empty() && worker_api_key.rfind("nvapi-", 0) == 0) {
            worker_base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
        } else {
            worker_base_url = "http://localhost:11434/v1/chat/completions";
        }
    }
    if (worker_model.empty()) worker_model = "meta/llama-3.2-11b-vision-instruct";

    shared_ptr<LLMClient> worker_client = LLMClientFactory::createClient(worker_model, worker_base_url, worker_api_key);

    // 4. Khởi tạo Môi trường thực thi (Environment Abstraction)
    shared_ptr<Environment> env = make_shared<NativeEnvironment>();

    // 5. Xử lý tham số dòng lệnh (CLI Arguments Parsing)
    if (argc > 1) {
        string arg1 = argv[1];

        if (arg1 == "--help" || arg1 == "-h") {
            printBanner();
            printHelp(argv[0]);
            return 0;
        }

        if (arg1 == "--web" || arg1 == "-w") {
            printBanner();
            WebServer server(client, env, 8080, "web", worker_client);
            server.start();
            return 0;
        }

        if (arg1 == "--gui-agent" || arg1 == "-a" || arg1 == "gui" || arg1 == "--gui" || arg1 == "-g") {
            printBanner();
            cout << "\n======================================================\n";
            cout << "     KHOI DONG MULTI-AGENT GUI AGENT (COMPUTER USE)   \n";
            cout << "    (Gemini: Coordinator | GUI: Visual | Tool: File)  \n";
            cout << "======================================================\n";

            string gui_task = "Quan sát màn hình Desktop hiện tại, mở terminal hoặc trình duyệt web, tìm kiếm thông tin về 'Trường Đại học Khoa học Tự nhiên ĐHQG-HCM', sao chép và tổng kết kết quả.";
            if (argc > 2) {
                gui_task = argv[2];
            }

            cout << "[Task]: \"" << gui_task << "\"\n\n";

            GUIAgentLoop gui_agent(20, "/tmp/agent_screenshot.png");
            gui_agent.setActionDelayMs(1000); // 1s giữa các thao tác

            auto result = gui_agent.run(gui_task, client);
            if (result.has_value()) {
                cout << "\n[Ket qua GUI Agent]:\n" << result.value() << endl;
            } else {
                cerr << "\n[Loi GUI Agent]: " << result.error() << endl;
            }
            return 0;
        }

        if (arg1 == "--multi-agent" || arg1 == "-m" || arg1 == "multi") {
            printBanner();
            cout << "\n======================================================\n";
            cout << "     KHOI DONG HYBRID MULTI-AGENT COORDINATION        \n";
            cout << "  (Gemini: Coordinator/Planner | Llama: Sub-Workers) \n";
            cout << "======================================================\n";

            string multi_task = "Tính toán doanh thu của Chi nhánh A (120*450 USD) và Chi nhánh B (85*620 USD), so sánh xem chi nhánh nào cao hơn và lưu báo cáo vào file revenue_comparison.txt";
            if (argc > 2) {
                multi_task = argv[2];
            }

            cout << "[Yeu cau]: \"" << multi_task << "\"\n\n";

            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            auto res = runner.coordinateTask(multi_task);

            if (res.has_value()) {
                cout << "\n=== KET QUA HOAN TAT MULTI-AGENT ===\n" << *res << "\n";
            } else {
                cerr << "\n=== LOI THUC THI MULTI-AGENT ===\n" << res.error() << "\n";
            }
            return 0;
        }

        if (arg1 == "--eval" || arg1 == "-b" || arg1 == "benchmark") {
            printBanner();
            cout << "[Benchmark]: Dang khoi dong toan bo danh sach danh gia 10 Tasks...\n";
            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            auto load_res = runner.loadTasks("benchmark/tasks.json");
            if (!load_res.has_value()) {
                cerr << format("[ERROR]: Khong the nap file tasks.json: {}\n", load_res.error());
                return 1;
            }
            runner.runBatch();
            cout << "\n[Success]: Hoan tat! Tat ca Trajectory Logs da duoc luu tai benchmark/results/\n";
            return 0;
        }

        if (arg1 == "--task") {
            if (argc < 3) {
                cerr << "[ERROR]: Vui long chi dinh Task ID (vi du: --task task_001)\n";
                return 1;
            }
            string taskId = argv[2];
            printBanner();
            cout << format("[Benchmark]: Dang tim kiem va chay rieng Task: {}...\n", taskId);
            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            auto load_res = runner.loadTasks("benchmark/tasks.json");
            if (!load_res.has_value()) {
                cerr << format("[ERROR]: Khong the nap file tasks.json: {}\n", load_res.error());
                return 1;
            }

            bool found = false;
            for (const auto& task : runner.getTasks()) {
                if (task.id == taskId) {
                    found = true;
                    runner.runTask(task);
                    break;
                }
            }

            if (!found) {
                cerr << format("[ERROR]: Khong tim thay Task ID '{}' trong tasks.json!\n", taskId);
                return 1;
            }
            return 0;
        }

        // Chạy trực tiếp prompt được truyền qua tham số
        string user_task = arg1;
        printBanner();
        cout << format("[User Task]: \"{}\"\n\n", user_task);
        vector<string> images_path = {};

        AgentLoop agent;
        auto result = agent.run(user_task, client, images_path);
        if (result.has_value()) {
            cout << "\n=== KET QUA HOAN TAT ===\n" << result.value() << "\n";
        } else {
            cerr << "\n=== LOI THUC THI ===\n" << result.error() << "\n";
        }
        return 0;
    }

    // =========================================================================
    // CHẾ ĐỘ: MENU TƯƠNG TÁC (INTERACTIVE CONSOLE MENU)
    // =========================================================================
    while (true) {
        printBanner();
        cout << "1. Nhap yeu cau / cau hoi truc tiep cho AI Agent (CLI)\n";
        cout << "2. Chay toan bo bo danh gia Benchmark (10 Tasks)\n";
        cout << "3. Chay kiem tra rieng 01 Task cu the\n";
        cout << "4. Khoi chay GUI Agent Desktop Automation (Computer Use)\n";
        cout << "5. Khoi chay Web GUI Dashboard (Trình duyệt - Tiếng Việt)\n";
        cout << "6. Khoi chay Hybrid Multi-Agent Coordination (Gemini + Llama)\n";
        cout << "7. Thoat chuong trinh\n";
        cout << "-----------------------------------------------------------------\n";
        cout << "Lua chon cua ban [1-7]: ";

        int choice = 0;
        if (!(cin >> choice)) {
            cin.clear();
            string discard;
            getline(cin, discard);
            continue;
        }
        cin.ignore(); // Xoa newline con lai

        if (1 == choice) {
            cout << "\nNhap nhiem vu ban muon AI Agent thuc hien:\n> ";
            string task_input;
            getline(cin, task_input);
            if (task_input.empty()) continue;

            cout << "\n[Agent]: Dang khoi dong suy luan...\n";
            AgentLoop agent;
            auto res = agent.run(task_input, client);
            if (res.has_value()) {
                cout << "\n=== KET QUA CUOI CUNG ===\n" << res.value() << "\n\n";
            } else {
                cerr << "\n=== LOI ===\n" << res.error() << "\n\n";
            }
        } else if (2 == choice) {
            cout << "\n[Benchmark]: Bat dau danh gia toan bo 10 Tasks...\n";
            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            if (runner.loadTasks("benchmark/tasks.json").has_value()) {
                runner.runBatch();
            }
        } else if (3 == choice) {
            cout << "\nNhap Task ID (vi du: task_001, task_005, task_010):\n> ";
            string taskId;
            getline(cin, taskId);
            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            if (runner.loadTasks("benchmark/tasks.json").has_value()) {
                bool found = false;
                for (const auto& task : runner.getTasks()) {
                    if (task.id == taskId) {
                        found = true;
                        runner.runTask(task);
                        break;
                    }
                }
                if (!found) {
                    cout << format("[Warning]: Khong tim thay Task '{}'!\n", taskId);
                }
            }
        } else if (4 == choice) {
            cout << "\nNhap yeu cau cho GUI Agent dieu khien may tinh (hoac Enter de dung mac dinh):\n> ";
            string gui_task;
            getline(cin, gui_task);
            if (gui_task.empty()) {
                gui_task = "Quan sát màn hình Desktop hiện tại, mở terminal hoặc trình duyệt web, tìm kiếm thông tin và tổng kết kết quả.";
            }
            GUIAgentLoop gui_agent(8, "/tmp/agent_screenshot.png");
            gui_agent.setActionDelayMs(1000);
            auto res = gui_agent.run(gui_task, client);
            if (res.has_value()) {
                cout << "\n=== KET QUA GUI AGENT ===\n" << res.value() << "\n\n";
            } else {
                cerr << "\n=== LOI GUI AGENT ===\n" << res.error() << "\n\n";
            }
        } else if (5 == choice) {
            WebServer server(client, env, 8080, "web", worker_client);
            server.start();
        } else if (6 == choice) {
            cout << "\nNhap nhiem vu phuc tap can Multi-Agent phan chia xu ly (hoac Enter de dung demo mac dinh):\n> ";
            string multi_task;
            getline(cin, multi_task);
            if (multi_task.empty()) {
                multi_task = "Tính toán doanh thu của Chi nhánh A (120*450 USD) và Chi nhánh B (85*620 USD), so sánh xem chi nhánh nào cao hơn và lưu báo cáo vào file revenue_comparison.txt";
            }
            HarnessRunner runner(client, env, "benchmark/results", worker_client);
            auto res = runner.coordinateTask(multi_task);
            if (res.has_value()) {
                cout << "\n=== KET QUA HOAN TAT MULTI-AGENT ===\n" << *res << "\n\n";
            } else {
                cerr << "\n=== LOI MULTI-AGENT ===\n" << res.error() << "\n\n";
            }
        } else if (7 == choice) {
            cout << "\nTam biet! Cam on ban da su dung AI Agent.\n";
            break;
        }
    }

    return 0;
}
