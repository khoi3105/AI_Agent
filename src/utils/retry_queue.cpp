#include "retry_queue.h"
#include <iostream>
#include <print>
#include <thread>
#include <chrono>
#include <cmath>
#include <regex>
#include <nlohmann/json.hpp>

namespace agent::utils {

void RetryQueue::push(const FailedRequestItem& item) {
    _queue.push(item);
}

bool RetryQueue::empty() const {
    return _queue.empty();
}

FailedRequestItem RetryQueue::pop() {
    if (_queue.empty()) {
        return {};
    }
    auto item = _queue.front();
    _queue.pop();
    return item;
}

size_t RetryQueue::size() const {
    return _queue.size();
}

void RetryQueue::clear() {
    while (!_queue.empty()) {
        _queue.pop();
    }
}

int RetryQueue::extractOrCalculateWaitSeconds(
    const std::string& response_body,
    const std::string& response_headers,
    int configured_rpm,
    int current_attempt) 
{
    // 1. Ưu tiên cao nhất: Trích xuất "retryDelay" từ Gemini JSON response
    // Cấu trúc lỗi: error.details[].retryDelay = "14.500000000s" hoặc "15s"
    if (!response_body.empty()) {
        try {
            auto j = nlohmann::json::parse(response_body);
            if (j.contains("error")) {
                const auto& errObj = j["error"];
                
                // Kiểm tra trong error.details
                if (errObj.contains("details") && errObj["details"].is_array()) {
                    for (const auto& detail : errObj["details"]) {
                        if (detail.contains("retryDelay") && detail["retryDelay"].is_string()) {
                            std::string delay_str = detail["retryDelay"].get<std::string>();
                            std::regex reg(R"(([0-9]+(?:\.[0-9]+)?))");
                            std::smatch match;
                            if (std::regex_search(delay_str, match, reg)) {
                                double delay_val = std::stod(match.str(1));
                                int wait_sec = static_cast<int>(std::ceil(delay_val)) + 1; // Làm tròn lên + 1s buffer
                                if (wait_sec > 0) return wait_sec;
                            }
                        }
                    }
                }

                // Kiểm tra nếu có trường retryDelay trực tiếp trong error object
                if (errObj.contains("retryDelay") && errObj["retryDelay"].is_string()) {
                    std::string delay_str = errObj["retryDelay"].get<std::string>();
                    std::regex reg(R"(([0-9]+(?:\.[0-9]+)?))");
                    std::smatch match;
                    if (std::regex_search(delay_str, match, reg)) {
                        double delay_val = std::stod(match.str(1));
                        int wait_sec = static_cast<int>(std::ceil(delay_val)) + 1;
                        if (wait_sec > 0) return wait_sec;
                    }
                }
            }
        } catch (...) {
            // Không parse được JSON, tiếp tục thử Header
        }
    }

    // 2. Ưu tiên nhì: Trích xuất HTTP Header "Retry-After"
    if (!response_headers.empty()) {
        std::regex header_reg(R"(retry-after:\s*([0-9]+))", std::regex_constants::icase);
        std::smatch match;
        if (std::regex_search(response_headers, match, header_reg)) {
            int header_delay = std::stoi(match.str(1));
            if (header_delay > 0) return header_delay + 1; // + 1s buffer
        }
    }

    // 3. Ưu tiên ba: Tính toán thời gian chờ dựa trên cấu hình RPM (Sliding Window / Rate Limit interval)
    if (configured_rpm > 0) {
        int base_interval = (60 / configured_rpm) + 1;
        int wait_sec = base_interval * (1 << current_attempt); // Exponential backoff: base * 2^attempt
        if (wait_sec > 60) wait_sec = 60; // Tối đa 60 giây cho một lần chờ
        return wait_sec;
    }

    // 4. Fallback mặc định nếu không có thông tin nào
    return 5 * (current_attempt + 1);
}

void RetryQueue::runCountdown(int total_seconds, const std::string& context_info) {
    if (total_seconds <= 0) total_seconds = 1;

    for (int remaining = total_seconds; remaining > 0; --remaining) {
        std::print(stderr, "\r⏳ [{}] Gặp mã 429 (Rate Limit). Tự động gửi lại sau {:02d}s... ", context_info, remaining);
        std::fflush(stderr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::println(stderr, "\r🚀 [{}] Đã hết thời gian chờ, tiến hành gửi lại request...                       ", context_info);
}

} // namespace agent::utils
