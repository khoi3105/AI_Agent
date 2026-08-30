#include "http_client.h"
#include "rate_limiter.h"
#include "retry_queue.h"
#include <curl/curl.h>
#include <thread>
#include <chrono>
#include <format>
#include <print>
#include <nlohmann/json.hpp>

namespace agent::utils {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::expected<std::string, std::string> HttpClient::postJson(
    const std::string& url, 
    const std::string& json_payload,
    const std::vector<std::string>& extra_headers,
    long timeout_seconds) 
{
    RetryQueue retryQueue;
    std::string current_url = url;
    std::string current_payload = json_payload;
    std::vector<std::string> current_headers = extra_headers;

    const int max_retries = 3;
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        // Kiểm soát tốc độ gửi request theo endpoint/domain
        RateLimiter::instance().acquire(current_url);

        CURL* curl = curl_easy_init();
        if (!curl) return std::unexpected("Không thể khởi tạo libcurl handle");

        std::string response_string;
        std::string response_headers;
        struct curl_slist* headers = nullptr;

        // 1. Set default headers
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");

        // 2. Thêm các custom headers (ví dụ Authorization: Bearer <Key>)
        for (const auto& header : current_headers) {
            headers = curl_slist_append(headers, header.c_str());
        }

        // 3. Setup cURL options
        curl_easy_setopt(curl, CURLOPT_URL, current_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, current_payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);

        // 4. Perform Request
        CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Dọn dẹp tài nguyên request
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return std::unexpected(std::string("Lỗi curl request: ") + curl_easy_strerror(res));
        }

        // Xử lý lỗi 429 Rate Limit
        if (http_code == 429 && attempt < max_retries - 1) {
            // 1. Lưu thông tin request vừa gặp lỗi vào hàng đợi retry
            retryQueue.push({
                current_url,
                current_payload,
                current_headers,
                attempt + 1,
                max_retries,
                timeout_seconds,
                std::chrono::steady_clock::now()
            });

            // 2. Trích xuất thời gian chờ chính xác từ API Response / Headers hoặc tính theo RPM
            int configured_rpm = RateLimiter::instance().getGeminiRPM();
            int wait_sec = RetryQueue::extractOrCalculateWaitSeconds(
                response_string, 
                response_headers, 
                configured_rpm, 
                attempt
            );

            // 3. Đếm ngược thời gian thực trên màn hình console
            RetryQueue::runCountdown(
                wait_sec, 
                std::format("Rate Limit 429 - Thử lại lần {}/{}", attempt + 1, max_retries - 1)
            );

            // 4. Lấy request ra khỏi hàng đợi để gửi lại ở vòng lặp kế tiếp
            auto nextReq = retryQueue.pop();
            current_url = nextReq.url;
            current_payload = nextReq.body;
            current_headers = nextReq.headers;
            continue;
        }

        if (http_code >= 400) {
            return std::unexpected("HTTP Error Code: " + std::to_string(http_code) + " | Response: " + response_string);
        }

        return response_string;
    }

    return std::unexpected("HTTP Error: Quá số lần thử lại (429 Rate Limit).");
}

std::expected<std::string, std::string> HttpClient::get(
    const std::string& url,
    const std::vector<std::string>& extra_headers,
    long timeout_seconds)
{
    std::string current_url = url;
    std::vector<std::string> current_headers = extra_headers;
    const int max_retries = 3;

    for (int attempt = 0; attempt < max_retries; ++attempt) {
        RateLimiter::instance().acquire(current_url);

        CURL* curl = curl_easy_init();
        if (!curl)
            return std::unexpected("Không thể khởi tạo libcurl handle");

        std::string response_string;
        std::string response_headers;
        struct curl_slist* headers = nullptr;

        // Default header
        headers = curl_slist_append(headers, "Accept: application/json");

        // Custom headers
        for (const auto& header : current_headers)
        {
            headers = curl_slist_append(headers, header.c_str());
        }

        // Setup request
        curl_easy_setopt(curl, CURLOPT_URL, current_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);

        CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            return std::unexpected(
                std::string("Lỗi curl request: ") +
                curl_easy_strerror(res));
        }

        if (http_code == 429 && attempt < max_retries - 1) {
            int configured_rpm = RateLimiter::instance().getGeminiRPM();
            int wait_sec = RetryQueue::extractOrCalculateWaitSeconds(
                response_string, 
                response_headers, 
                configured_rpm, 
                attempt
            );

            RetryQueue::runCountdown(
                wait_sec, 
                std::format("GET Rate Limit 429 - Thử lại lần {}/{}", attempt + 1, max_retries - 1)
            );
            continue;
        }

        if (http_code >= 400)
        {
            return std::unexpected(
                "HTTP Error Code: " +
                std::to_string(http_code) +
                " | Response: " +
                response_string);
        }
        return response_string;
    }

    return std::unexpected("HTTP Error: Quá số lần thử lại (429 Rate Limit).");
}

} // namespace agent::utils
