# 🤖 C++ AI AGENT WITH OLLAMA / OPENAI API

> **Đồ Án Lập Trình Hướng Đối Tượng (OOP) - Năm Học 2025-2026**  
> **Trường Đại Học Khoa Học Tự Nhiên - ĐHQG-HCM (HCMUS)**  
> **Tác Giả:** Khôi, Nhân, Huân  
> **Ngôn Ngữ:** C++ (Tiêu chuẩn hiện đại C++20/C++23/C++26)

---

## 📖 1. Giới Thiệu Tổng Quan

Dự án xây dựng một **Hệ thống AI Agent tự trị (Autonomous AI Agent)** bằng C++ hiện đại, có khả năng:
- Tương tác với LLM thông qua chuẩn **Ollama / OpenAI Compatible API**.
- Hoạt động theo mô hình suy luận phản xạ **ReAct Loop** (*Observe → Think → Act → Observe*).
- Hỗ trợ **Adaptive Planning** (Lập kế hoạch đa bước cho bài toán phức tạp và Fast Path cho bài toán đơn giản).
- Tích hợp hệ thống **Tool Registry** động với **8+ công cụ** (Toán học, Đọc/Ghi file & PDF, Tìm kiếm Web, Bộ nhớ SQLite 2 tầng, Thực thi lệnh hệ thống, Tra cứu thời tiết).
- Cơ chế **Bảo mật & Kiểm duyệt** câu lệnh bằng `ToolPolicy`.
- Hệ sinh thái **Skill System** tự động nạp hướng dẫn chuyên biệt từ file Markdown.
- Hệ thống đánh giá tự động **Harness Benchmark** với 10 bài toán phân loại 3 cấp độ độ khó, xuất nhật ký quỹ đạo (**Trajectory Log**) chi tiết theo chuẩn JSON.

---

## 🏛️ 2. Kiến Trúc Hệ Thống & Design Patterns

Hệ thống được thiết kế theo đúng các nguyên lý **SOLID** và áp dụng 4 mẫu thiết kế Hướng đối tượng bắt buộc:

```
                  +-----------------------------------+
                  |         main() CLI Entry          |
                  +-----------------+-----------------+
                                    |
          +-------------------------+-------------------------+
          | (Interactive Mode)                                | (Benchmark Mode: --eval)
          v                                                   v
+-------------------+                               +--------------------+
|     AgentLoop     |<====== [StepHook Observer] ===|   HarnessRunner    |
+---------+---------+                               +---------+----------+
          |                                                   |
          +---> [1. LLMClient (Ollama/OpenAI)]                +---> [Evaluator Strategy]
          |                                                   |      ├── KeywordEvaluator
          +---> [2. TaskPlan (Adaptive Planning)]             |      └── FunctionalEvaluator
          |                                                   |
          +---> [3. LoopDetector (Repeat/PingPong)]           +---> [Environment]
          |                                                          ├── NativeEnvironment
          +---> [4. SkillLoader (Markdown Skills)]                   └── SandboxEnvironment
          |
          +---> [5. ToolRegistry (Dynamic Factory)]
                ├── calculator_tool (exprtk)
                ├── exec_tool (Shell + ToolPolicy)
                ├── read_file_tool (Text + Poppler PDF)
                ├── write_file_tool (STL)
                ├── web_search_tool (libcurl + gumbo HTML)
                ├── memory_save_tool (SQLite3)
                ├── memory_search_tool (SQLite3 2-Tier Relevance Ranking)
                └── weather_tool (OpenWeather API)
```

### Các Mẫu Thiết Kế (Design Patterns) Áp Dụng:
1. **Strategy Pattern:**
   - Đánh giá kết quả trong `Evaluator` (`KeywordEvaluator`, `FunctionalEvaluator`).
   - Bộ thực thi Shell đa nền tảng (`LinuxShellExecutor`, `WindowsShellExecutor`).
2. **Template Method Pattern:**
   - Khung điều phối vòng lặp `AgentLoop::run()` với các bước mở rộng trừu tượng: `plan()`, `parseStepResponse()`, `act()`, `observe()`, `formatFinalResponse()`.
3. **Registry / Dynamic Factory Pattern:**
   - `ToolRegistry`: Quản lý, đăng ký và điều phối thực thi các công cụ động theo chuỗi JSON Schema.
   - `EvaluatorFactory`: Khởi tạo bộ đánh giá phù hợp dựa trên cấu hình task.
4. **Observer / Hook Pattern:**
   - `StepHook`: Cơ chế callback đăng ký từ `HarnessRunner` vào `AgentLoop` để giám sát thời gian thực từng bước suy luận mà không gây ràng buộc cứng (Loose Coupling).

---

## 🛠️ 3. Yêu Cầu Cài Đặt & Môi Trường

### Yêu Cầu Hệ Thống:
- **Hệ điều hành:** Linux (Ubuntu 22.04+ khuyến nghị) / macOS / Windows (WSL2).
- **Trình biên dịch:** GCC 13+ (hoặc `g++-16`) hỗ trợ C++20 / C++23 / C++26.
- **Công cụ build:** CMake 3.25 trở lên, Make / Ninja.

### Cài Đặt Thư Viện Phụ Thuộc (Ubuntu / Debian):
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
                    libcurl4-openssl-dev \
                    libsqlite3-dev \
                    libpoppler-cpp-dev \
                    libgumbo-dev
```

---

## 🚀 4. Hướng Dẫn Biên Dịch & Chạy Chương Trình

### 4.1. Cấu hình biến môi trường
Tạo file `.env` tại thư mục gốc của dự án:
```env
OLLAMA_BASE_URL=http://localhost:11434
OLLAMA_MODEL=meta/llama-3.2-11b-vision-instruct
OLLAMA_API_KEY=your_api_key_if_needed
```

### 4.2. Biên dịch với CMake
```bash
cd /home/nhann/VSC/Agent/AI_Agent

# Tạo thư mục build và cấu hình
cmake -B build

# Biên dịch
cmake --build build -j$(nproc)
```

### 4.3. Các Chế Độ Chạy Chương Trình

#### A. Chế độ Giao diện Web GUI Dashboard (Web Browser):
```bash
export $(cat .env | xargs) && ./build/main --web
```
*Mở trình duyệt tại `http://localhost:8080` để trải nghiệm giao diện Web Dashboard trực quan tiếng Việt.*

#### B. Chế độ Desktop GUI Agent Đa Tác Tử (Multi-Agent GUI Automation):
```bash
DISPLAY=:0 XAUTHORITY=/home/kali/.Xauthority ./build/main gui "Mở trình duyệt Microsoft Edge, tìm kiếm thông tin về giá iphone 17 pro max. Sau đó, viết vào file ip17.txt"
```
*Hệ thống điều phối Master Coordinator (Gemini) phân rã bài toán, giao cho GUI Agent quan sát màn hình trích xuất dữ liệu, và giao cho Tool Worker ghi kết quả ra file.*

#### C. Chạy trực tiếp một câu lệnh CLI Prompt:
```bash
export $(cat .env | xargs) && ./build/main "Tính (125 * 37) + (940 / 5) và ghi kết quả vào file kq.txt"
```

#### D. Chạy Toàn Bộ Bộ Đánh Giá Benchmark (10 Tasks):
```bash
export $(cat .env | xargs) && ./build/main --eval
```

#### E. Chạy Kiểm Tra Riêng 01 Task Cụ Thể:
```bash
export $(cat .env | xargs) && ./build/main --task task_001
export $(cat .env | xargs) && ./build/main --task task_010
```

---

## 📊 5. Bộ Đánh Giá Tự Động (Benchmark Suite)

Dự án thiết kế bộ 10 bài toán thực tế theo 3 cấp độ trong `benchmark/tasks.json`:

| Task ID | Độ Khó | Mô Tả Tác Vụ | Phương Pháp Đánh Giá | Giới Hạn Bước | Timeout |
|:---:|:---:|:---|:---:|:---:|:---:|
| `task_001` | Simple | Tính toán biểu thức số học phức tạp | Keyword (`4813`) | 5 | 45s |
| `task_002` | Simple | Ghi nội dung văn bản ra file | Functional (Shell check) | 5 | 45s |
| `task_003` | Simple | Đọc file cấu hình lấy giá trị biến | Keyword (`8080`) | 5 | 45s |
| `task_004` | Simple | Lưu trữ trí nhớ vào SQLite | Keyword | 5 | 45s |
| `task_005` | Medium | Tính toán kết hợp ghi file kết quả | Functional | 8 | 90s |
| `task_006` | Medium | Truy vấn thông tin trong bộ nhớ | Keyword | 8 | 90s |
| `task_007` | Medium | Thực thi lệnh shell kiểm tra compiler | Keyword | 8 | 90s |
| `task_008` | Medium | Tìm kiếm thông tin web thực tế | Keyword | 8 | 90s |
| `task_009` | Hard | Đọc file dữ liệu, tính tổng và lưu file | Functional | 12 | 180s |
| `task_010` | Hard | Pipeline: Tra cứu Web → Lưu Memory → Ghi File | Functional | 12 | 120s |

*Tất cả kết quả chạy được tự động ghi lại tại thư mục `benchmark/results/trajectory_task_XXX.json` bao gồm toàn bộ Thought, Action, Observation, Latency và Trạng thái hoàn thành.*

---

## 📁 6. Cấu Trúc Mã Nguồn (Project Structure)

```
AI_Agent/
├── CMakeLists.txt              # File cấu hình build hệ thống
├── README.md                   # Tài liệu hướng dẫn dự án
├── .env                        # File cấu hình API & Model
├── benchmark/                  # Thư mục benchmark & kết quả
│   ├── tasks.json              # Dataset 10 tasks đánh giá
│   └── results/                # Quỹ đạo Trajectory log chi tiết
├── skills/                     # Các file kỹ năng chuyên biệt (.md)
│   ├── error_recovery.md
│   ├── file_management.md
│   └── web_research.md
├── docs/                       # Sơ đồ UML & tài liệu thiết kế
│   ├── class_diagram.puml
│   ├── sequence_react.puml
│   └── sequence_harness.puml
└── src/                        # Mã nguồn C++
    ├── main.cpp                # Điểm vào chính của ứng dụng
    ├── agent/                  # Điều phối Agent Loop & Planning
    │   ├── AgentLoop.cpp/.h
    │   ├── task_plan.cpp/.h
    │   ├── loop_detector.cpp/.h
    │   ├── skill_loader.cpp/.h
    │   ├── tool_call_parser.cpp/.h
    │   └── step_data.h
    ├── client/                 # Tầng giao tiếp HTTP LLM
    │   ├── llm_client.h
    │   └── ollama_client.cpp/.h
    ├── tools/                  # Tầng công cụ (Tools) & Policy
    │   ├── tool.h
    │   ├── tool_policy.cpp/.h
    │   ├── tool_registry.cpp/.h
    │   ├── calculator_tool/
    │   ├── exec_tool/
    │   ├── read_file_tool/
    │   ├── write_file_tool/
    │   ├── web_search_tool/
    │   ├── memory_save_tool/
    │   ├── memory_search_tool/
    │   └── weather_tool/
    ├── environment/            # Tầng môi trường (Environment Abstraction)
    │   ├── environment.h
    │   ├── native_environment.cpp/.h
    │   └── sandbox_environment.cpp/.h
    └── harness/                # Tầng đánh giá tự động (Benchmark Harness)
        ├── harness_runner.cpp/.h
        ├── evaluator.h
        ├── keyword_evaluator.cpp
        ├── functional_evaluator.cpp
        └── trajectory.cpp/.h
```

---

## 📜 7. Giấy Phép & Bản Quyền
Dự án được phát triển phục vụ mục đích học tập và nghiên cứu môn học **Lập Trình Hướng Đối Tượng (OOP)** tại Trường ĐH Khoa học Tự nhiên ĐHQG-HCM. Mọi đóng góp và mã nguồn tuân thủ giấy phép mã nguồn mở MIT.
