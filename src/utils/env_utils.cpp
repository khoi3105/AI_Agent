#include "env_utils.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>

static inline std::string trimEnv(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n\"'");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n\"'");
    return s.substr(start, end - start + 1);
}

std::string getEnvVar(const char* name) {
    // 1. Kiểm tra trực tiếp từ Environment Variables của hệ thống
    const char* val = std::getenv(name);
    if (val != nullptr && std::string(val) != "") {
        return std::string(val);
    }

    // 2. Fallback: Đọc từ file .env nếu có
    std::ifstream envFile(".env");
    if (!envFile.is_open()) {
        envFile.open("../.env"); // Thử tìm ở thư mục cha nếu chạy từ build/
    }

    if (envFile.is_open()) {
        std::string line;
        while (std::getline(envFile, line)) {
            line = trimEnv(line);
            if (line.empty() || line[0] == '#') continue;

            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = trimEnv(line.substr(0, pos));
                std::string value = trimEnv(line.substr(pos + 1));
                if (key == name) {
                    return value;
                }
            }
        }
    }

    return "";
}