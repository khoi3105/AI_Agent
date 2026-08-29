#include "rate_limiter.h"
#include "../config/config.h"
#include <iostream>
#include <print>
#include <thread>
#include <format>
#include <algorithm>

namespace agent::utils {

RateLimiter::RateLimiter(int geminiRPM, int workerRPM) 
    : _geminiRPM(geminiRPM), _workerRPM(workerRPM) {}

RateLimiter& RateLimiter::instance() {
    static RateLimiter globalInstance;
    static bool initialized = false;
    if (!initialized) {
        int gRpm = Config::instance()->llm().rpm;
        if (gRpm > 0) globalInstance._geminiRPM = gRpm;

        int wRpm = Config::instance()->multi().workerRpm;
        globalInstance._workerRPM = wRpm;

        initialized = true;
    }
    return globalInstance;
}

void RateLimiter::setGeminiRPM(int rpm) {
    std::unique_lock<std::mutex> lock(_mutex);
    _geminiRPM = rpm;
}

void RateLimiter::setWorkerRPM(int rpm) {
    std::unique_lock<std::mutex> lock(_mutex);
    _workerRPM = rpm;
}

int RateLimiter::getGeminiRPM() const {
    std::unique_lock<std::mutex> lock(_mutex);
    return _geminiRPM;
}

int RateLimiter::getWorkerRPM() const {
    std::unique_lock<std::mutex> lock(_mutex);
    return _workerRPM;
}

std::string RateLimiter::resolveProvider(const std::string& url) const {
    if (url.empty()) return "gemini";
    std::string lower = url;
    for (auto& c : lower) c = std::tolower(c);

    if (lower.find("generativelanguage.googleapis.com") != std::string::npos || lower.find("gemini") != std::string::npos) {
        return "gemini";
    }
    if (lower.find("nvidia.com") != std::string::npos || lower.find("11434") != std::string::npos || lower.find("localhost") != std::string::npos || lower.find("127.0.0.1") != std::string::npos) {
        return "worker_llama";
    }
    return "other";
}

void RateLimiter::cleanupExpired(const std::string& provider, const std::chrono::steady_clock::time_point& now) {
    auto& deq = _timestampsByProvider[provider];
    while (!deq.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - deq.front());
        if (elapsed.count() >= 60) {
            deq.pop_front();
        } else {
            break;
        }
    }
}

int RateLimiter::getCurrentRPM(const std::string& provider) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    cleanupExpired(provider, now);
    return static_cast<int>(_timestampsByProvider[provider].size());
}

void RateLimiter::acquire(const std::string& url) {
    std::string provider = resolveProvider(url);

    std::unique_lock<std::mutex> lock(_mutex);
    auto now = std::chrono::steady_clock::now();
    cleanupExpired(provider, now);

    int maxRPM = (provider == "gemini") ? _geminiRPM : ((provider == "worker_llama") ? _workerRPM : 0);

    // Nếu cấu hình RPM > 0 và số request trong 60s qua đã chạm ngưỡng giới hạn
    if (maxRPM > 0 && static_cast<int>(_timestampsByProvider[provider].size()) >= maxRPM) {
        auto oldest = _timestampsByProvider[provider].front();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldest);
        auto wait_ms = std::chrono::milliseconds(60000) - elapsed + std::chrono::milliseconds(150); // Buffer 150ms

        if (wait_ms.count() > 0) {
            std::println("[RateLimiter - {}]: Đạt giới hạn {} RPM (Hiện tại: {} reqs/60s). Tạm dừng {:.2f}s để tránh HTTP 429...", 
                         provider, maxRPM, _timestampsByProvider[provider].size(), wait_ms.count() / 1000.0);
            
            lock.unlock();
            std::this_thread::sleep_for(wait_ms);
            lock.lock();

            now = std::chrono::steady_clock::now();
            cleanupExpired(provider, now);
        }
    }

    // Ghi nhận mốc thời gian gửi request
    _timestampsByProvider[provider].push_back(now);

    if (provider == "gemini" && maxRPM > 0) {
        std::println("[RateLimiter - Gemini]: Lưu lượng Gemini API: {}/{} RPM (trong 60s qua).", 
                     _timestampsByProvider[provider].size(), maxRPM);
    } else if (provider == "worker_llama") {
        if (maxRPM > 0) {
            std::println("[RateLimiter - Llama]: Lưu lượng Llama Worker: {}/{} RPM.", 
                         _timestampsByProvider[provider].size(), maxRPM);
        } else {
            std::println("[Worker - Llama]: Gửi request tới Llama (Không giới hạn RPM - Request #{}/60s).",
                         _timestampsByProvider[provider].size());
        }
    }
}

} // namespace agent::utils
