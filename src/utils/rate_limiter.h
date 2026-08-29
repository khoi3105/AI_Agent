#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <deque>
#include <chrono>
#include <mutex>
#include <string>

namespace agent::utils {

/**
 * @brief Thread-safe Sliding Window Rate Limiter đo và điều tiết RPM (Requests Per Minute).
 * Nếu vượt quá giới hạn RPM cấu hình, hàm acquire() sẽ tự động tạm dừng (sleep) cho đến khi có slot trống.
 */
class RateLimiter {
private:
    int _maxRPM; // 0: không giới hạn (unlimited)
    std::deque<std::chrono::steady_clock::time_point> _requestTimestamps;
    mutable std::mutex _mutex;

    void cleanupExpired(const std::chrono::steady_clock::time_point& now);

public:
    explicit RateLimiter(int maxRPM = 0);

    // Cập nhật cấu hình RPM tối đa
    void setMaxRPM(int maxRPM);

    // Lấy giới hạn RPM đã cấu hình
    int getMaxRPM() const;

    // Đo số lượng Request thực tế đã gửi trong cửa sổ 60 giây gần nhất
    int getCurrentRPM();

    // Xin quyền gửi request: Nếu vượt RPM, tự động sleep và log thông báo
    void acquire();

    // Singleton instance cho toàn bộ ứng dụng
    static RateLimiter& instance();
};

} // namespace agent::utils

#endif // RATE_LIMITER_H
