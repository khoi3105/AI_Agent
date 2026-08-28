#include "http_client.h"
#include <curl/curl.h>
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
    CURL* curl = curl_easy_init();
    if (!curl) return std::unexpected("Không thể khởi tạo libcurl handle");

    std::string response_string;
    struct curl_slist* headers = nullptr;

    // 1. Set default headers
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    // 2. Thêm các custom headers (ví dụ Authorization: Bearer <Key>)
    for (const auto& header : extra_headers) {
        headers = curl_slist_append(headers, header.c_str());
    }

    // 3. Setup cURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
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

    if (http_code >= 400) {
        return std::unexpected("HTTP Error Code: " + std::to_string(http_code) + " | Response: " + response_string);
    }

    return response_string;
}

std::expected<std::string, std::string> HttpClient::get(
    const std::string& url,
    const std::vector<std::string>& extra_headers,
    long timeout_seconds)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return std::unexpected("Không thể khởi tạo libcurl handle");

    std::string response_string;
    struct curl_slist* headers = nullptr;

    // Default header
    headers = curl_slist_append(headers, "Accept: application/json");

    // Custom headers
    for (const auto& header : extra_headers)
    {
        headers = curl_slist_append(headers, header.c_str());
    }

    // Setup request
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
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
} // namespace agent::utils