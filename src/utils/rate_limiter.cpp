#include "rate_limiter.h"
#include "../config/config.h"
#include <iostream>
#include <thread>
#include <format>

namespace agent::utils {

RateLimiter::RateLimiter(int maxRPM) : _maxRPM(maxRPM) {}

RateLimiter& RateLimiter::instance() {
    static RateLimiter globalInstance;
    // Khởi tạo maxRPM từ file config nếu chưa được set
    static bool initialized = false;
    if (!initialized) {
        int configRpm = Config::instance()->llm().rpm;
        if (configRpm <= 0) {
            configRpm = Config::instance()->http().rpm;
        }
        if (configRpm > 0) {
            globalInstance.setMaxRPM(configRpm);
        }
        initialized = true;
    }
    return globalInstance;
}

void RateLimiter::setMaxRPM(int maxRPM) {
    std::unique_lock<std::mutex> lock(_mutex);
    _maxRPM = maxRPM;
}

int RateLimiter::getMaxRPM() const {
    std::unique_lock<std::mutex> lock(_mutex);
    return _maxRPM;
}

void RateLimiter::cleanupExpired(const std::chrono::steady_clock::time_point& now) {
    // Xóa các mốc thời gian cũ hơn 60 giây
    while (!_requestTimestamps.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _requestTimestamps.front());
        if (elapsed.count() >= 60) {
            _requestTimestamps.pop_front();
        } else {
            break;
        }
    }
}

int RateLimiter::getCurrentRPM() {
    std::unique_lock<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    cleanupExpired(now);
    return static_cast<int>(_requestTimestamps.size());
}

void RateLimiter::acquire() {
    std::unique_lock<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    cleanupExpired(now);

    // Nếu cấu hình RPM > 0 và số request trong 60s qua đã chạm ngưỡng giới hạn
    if (_maxRPM > 0 && static_cast<int>(_requestTimestamps.size()) >= _maxRPM) {
        auto oldest = _requestTimestamps.front();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldest);
        auto wait_ms = std::chrono::milliseconds(60000) - elapsed + std::chrono::milliseconds(150); // Buffer an toàn 150ms

        if (wait_ms.count() > 0) {
            std::cout << std::format("[RateLimiter]: ⚠️ Đạt giới hạn {} RPM (Hiện tại: {} reqs/60s). Tạm dừng {:.2f}s để tránh HTTP 429...\n", 
                                     _maxRPM, _requestTimestamps.size(), wait_ms.count() / 1000.0);
            
            // Mở lock trước khi ngủ để không block các thread đọc trạng thái khác nếu có
            lock.unlock();
            std::this_thread::sleep_for(wait_ms);
            lock.lock();

            // Cập nhật lại thời gian sau khi ngủ dậy
            now = std::chrono::steady_clock::now();
            cleanupExpired(now);
        }
    }

    // Ghi nhận mốc thời gian gửi request
    _requestTimestamps.push_back(now);

    if (_maxRPM > 0) {
        std::cout << std::format("[RateLimiter]: 📊 Lưu lượng hiện tại: {}/{} RPM (trong 60s qua).\n", 
                                 _requestTimestamps.size(), _maxRPM);
    }
}

} // namespace agent::utils
