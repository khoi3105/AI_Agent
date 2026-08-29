#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <deque>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace agent::utils {

/**
 * @brief Thread-safe Sliding Window Rate Limiter đo và điều tiết RPM (Requests Per Minute) theo từng Endpoint/Provider.
 * Tách biệt hoàn toàn lưu lượng của Gemini (giới hạn 15 RPM) và Llama/Ollama (không giới hạn hoặc cấu hình riêng).
 */
class RateLimiter {
private:
    int _geminiRPM{15};
    int _workerRPM{0}; // 0 = unlimited
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> _timestampsByProvider;
    mutable std::mutex _mutex;

    void cleanupExpired(const std::string& provider, const std::chrono::steady_clock::time_point& now);
    std::string resolveProvider(const std::string& url) const;

public:
    explicit RateLimiter(int geminiRPM = 15, int workerRPM = 0);

    // Cập nhật cấu hình RPM tối đa
    void setGeminiRPM(int rpm);
    void setWorkerRPM(int rpm);

    int getGeminiRPM() const;
    int getWorkerRPM() const;

    // Đo số lượng Request thực tế đã gửi trong cửa sổ 60 giây gần nhất theo provider
    int getCurrentRPM(const std::string& provider = "gemini");

    // Xin quyền gửi request: Kiểm tra theo URL cụ thể
    void acquire(const std::string& url = "");

    // Singleton instance cho toàn bộ ứng dụng
    static RateLimiter& instance();
};

} // namespace agent::utils

#endif // RATE_LIMITER_H
