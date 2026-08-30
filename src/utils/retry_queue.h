#pragma once
#include <string>
#include <vector>
#include <queue>
#include <chrono>

namespace agent::utils {

struct FailedRequestItem {
    std::string url;
    std::string body;
    std::vector<std::string> headers;
    int attempt = 0;
    int max_retries = 3;
    long timeout_seconds = 60;
    std::chrono::steady_clock::time_point failed_time;
};

class RetryQueue {
private:
    std::queue<FailedRequestItem> _queue;

public:
    void push(const FailedRequestItem& item);
    bool empty() const;
    FailedRequestItem pop();
    size_t size() const;
    void clear();

    /**
     * @brief Trích xuất thời gian chờ chính xác từ API response (retryDelay), Header Retry-After, hoặc tính từ RPM
     * @param response_body Nội dung JSON trả về từ API
     * @param response_headers Chuỗi raw headers trả về từ HTTP response
     * @param configured_rpm Số RPM đã cấu hình (ví dụ 15 RPM)
     * @param current_attempt Lần thử hiện tại (0-indexed)
     * @return Số giây cần chờ (đã bao gồm 1s buffer)
     */
    static int extractOrCalculateWaitSeconds(
        const std::string& response_body,
        const std::string& response_headers,
        int configured_rpm,
        int current_attempt
    );

    /**
     * @brief Chạy đếm ngược thời gian thực trên màn hình console (sử dụng \r để ghi đè dòng)
     * @param total_seconds Tổng số giây cần chờ
     * @param context_info Thông tin hiển thị ngữ cảnh (ví dụ "Gemini API - Lần 1/3")
     */
    static void runCountdown(int total_seconds, const std::string& context_info);
};

} // namespace agent::utils
