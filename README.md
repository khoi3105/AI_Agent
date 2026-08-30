# C++ AI AGENT

> **Đồ Án Lập Trình Hướng Đối Tượng (OOP) - Năm Học 2025-2026**  
> **Trường Đại Học Khoa Học Tự Nhiên - ĐHQG-HCM (HCMUS)**  
> **Tác Giả:** Ngô Đăng Khôi, Nguyễn Đăng Huỳnh Nhân, Nguyễn Minh Huân  
> **Ngôn Ngữ:** C++26


## 1. Yêu Cầu Cài Đặt & Môi Trường

### Cài Đặt Thư Viện Phụ Thuộc (Ubuntu / Debian):
```bash
sudo apt update && sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    ninja-build \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    libpoppler-cpp-dev \
    sqlite3 libsqlite3-dev \
    libgumbo-dev \
    libxdo-dev \
    xdotool \
    maim
```

---

## 2. Hướng Dẫn Biên Dịch & Chạy Chương Trình

### 2.1. Cấu hình biến môi trường
Tạo file `.env` tại thư mục gốc của dự án:
```env
LLAMA_API_KEY=
WEATHER_API_KEY=
GEMINI_API_KEY=
```

### 2.2. Biên dịch với CMake
```bash
# Tạo thư mục build và cấu hình
cmake -B build
# Biên dịch
cmake --build build -j$(nproc)
```

### 2.3. Các Chế Độ Chạy Chương Trình

Sử dụng --help để xem các option 
```bash
export $(cat .env | xargs) &&./build/main --help
```

#### A. Chế độ Giao diện Web GUI Dashboard (Web Browser):
```bash
export $(cat .env | xargs) && ./build/main --web
```
*Mở trình duyệt tại `http://localhost:8080` để trải nghiệm giao diện Web Dashboard trực quan tiếng Việt.*

#### B. Chế độ Desktop GUI Agent Đa Tác Tử (Multi-Agent GUI Automation):
```bash
DISPLAY=:0 XAUTHORITY=/home/kali/.Xauthority ./build/main gui "Mở trình duyệt Firefox, tìm kiếm thông tin về giá iphone 17 pro max. Sau đó, viết vào file ip17.txt"
```

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
